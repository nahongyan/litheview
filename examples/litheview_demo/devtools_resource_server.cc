#include <winsock2.h>
#include <windows.h>

#include "devtools_resource_server.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <string>
#include <string_view>

namespace litheview_demo {

namespace {

constexpr int kDevToolsPakResourceId = 101;

struct ResourcePath {
  std::string_view path;
  uint16_t id;
};

constexpr ResourcePath kResourcePaths[] = {
#include <devtools_resource_index.inc>
};

struct ResourceData {
  const uint8_t* data = nullptr;
  size_t size = 0;
  std::string_view content_encoding;
};

uint16_t ReadUint16(const uint8_t* data) {
  return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

uint32_t ReadUint32(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

bool FindPackedResource(uint16_t id, ResourceData* resource) {
  HMODULE module = GetModuleHandleW(nullptr);
  HRSRC resource_info = FindResourceW(
      module, MAKEINTRESOURCEW(kDevToolsPakResourceId), RT_RCDATA);
  if (!resource_info) {
    return false;
  }
  HGLOBAL resource_handle = LoadResource(module, resource_info);
  const auto* pack = static_cast<const uint8_t*>(LockResource(resource_handle));
  const size_t pack_size = SizeofResource(module, resource_info);
  if (!pack || pack_size < 12 || ReadUint32(pack) != 5) {
    return false;
  }
  const uint16_t resource_count = ReadUint16(pack + 8);
  const uint16_t alias_count = ReadUint16(pack + 10);
  const size_t entries_size = (static_cast<size_t>(resource_count) + 1) * 6;
  const size_t aliases_offset = 12 + entries_size;
  if (aliases_offset + static_cast<size_t>(alias_count) * 4 > pack_size) {
    return false;
  }

  size_t entry_index = resource_count;
  size_t left = 0;
  size_t right = resource_count;
  while (left < right) {
    const size_t middle = left + (right - left) / 2;
    const uint16_t candidate = ReadUint16(pack + 12 + middle * 6);
    if (candidate < id) {
      left = middle + 1;
    } else {
      right = middle;
    }
  }
  if (left < resource_count && ReadUint16(pack + 12 + left * 6) == id) {
    entry_index = left;
  } else {
    for (size_t index = 0; index < alias_count; ++index) {
      const uint8_t* alias = pack + aliases_offset + index * 4;
      if (ReadUint16(alias) == id) {
        entry_index = ReadUint16(alias + 2);
        break;
      }
    }
  }
  if (entry_index >= resource_count) {
    return false;
  }
  const uint32_t begin = ReadUint32(pack + 12 + entry_index * 6 + 2);
  const uint32_t end = ReadUint32(pack + 12 + (entry_index + 1) * 6 + 2);
  if (begin > end || end > pack_size) {
    return false;
  }
  resource->data = pack + begin;
  resource->size = end - begin;
  if (resource->size >= 8 && resource->data[0] == 0x1e &&
      resource->data[1] == 0x9b) {
    resource->data += 8;
    resource->size -= 8;
    resource->content_encoding = "br";
  } else if (resource->size >= 2 && resource->data[0] == 0x1f &&
             resource->data[1] == 0x8b) {
    resource->content_encoding = "gzip";
  }
  return true;
}

bool DecodePath(std::string_view input, std::string* output) {
  output->clear();
  for (size_t index = 0; index < input.size(); ++index) {
    if (input[index] != '%') {
      output->push_back(input[index]);
      continue;
    }
    if (index + 2 >= input.size()) {
      return false;
    }
    unsigned int value = 0;
    const char* begin = input.data() + index + 1;
    if (std::from_chars(begin, begin + 2, value, 16).ec != std::errc()) {
      return false;
    }
    output->push_back(static_cast<char>(value));
    index += 2;
  }
  return output->find("..") == std::string::npos;
}

std::string_view MimeType(std::string_view path) {
  const size_t dot = path.rfind('.');
  const std::string_view extension =
      dot == std::string_view::npos ? std::string_view() : path.substr(dot);
  if (extension == ".html") {
    return "text/html; charset=utf-8";
  }
  if (extension == ".js" || extension == ".mjs") {
    return "text/javascript; charset=utf-8";
  }
  if (extension == ".css") {
    return "text/css; charset=utf-8";
  }
  if (extension == ".json") {
    return "application/json; charset=utf-8";
  }
  if (extension == ".svg") {
    return "image/svg+xml";
  }
  if (extension == ".png") {
    return "image/png";
  }
  if (extension == ".gif") {
    return "image/gif";
  }
  if (extension == ".webp") {
    return "image/webp";
  }
  if (extension == ".ico") {
    return "image/x-icon";
  }
  if (extension == ".woff2") {
    return "font/woff2";
  }
  if (extension == ".wasm") {
    return "application/wasm";
  }
  return "application/octet-stream";
}

bool SendAll(SOCKET socket_handle, const char* data, size_t size) {
  while (size > 0) {
    const int chunk =
        send(socket_handle, data,
             static_cast<int>(std::min<size_t>(size, 1 << 20)), 0);
    if (chunk <= 0) {
      return false;
    }
    data += chunk;
    size -= chunk;
  }
  return true;
}

void HandleRequest(SOCKET client) {
  std::array<char, 16384> buffer = {};
  const int received =
      recv(client, buffer.data(), static_cast<int>(buffer.size() - 1), 0);
  if (received <= 0) {
    return;
  }
  const std::string_view request(buffer.data(), received);
  if (!request.starts_with("GET ")) {
    return;
  }
  const size_t path_end = request.find(' ', 4);
  if (path_end == std::string_view::npos) {
    return;
  }
  std::string_view encoded_path = request.substr(4, path_end - 4);
  if (const size_t query = encoded_path.find('?');
      query != std::string_view::npos) {
    encoded_path = encoded_path.substr(0, query);
  }
  while (encoded_path.starts_with('/')) {
    encoded_path.remove_prefix(1);
  }
  if (encoded_path.empty()) {
    encoded_path = "inspector.html";
  }
  std::string path;
  if (!DecodePath(encoded_path, &path)) {
    return;
  }
  const auto iterator = std::lower_bound(
      std::begin(kResourcePaths), std::end(kResourcePaths), path,
      [](const ResourcePath& entry, std::string_view value) {
        return entry.path < value;
      });
  ResourceData resource;
  if (iterator == std::end(kResourcePaths) || iterator->path != path ||
      !FindPackedResource(iterator->id, &resource)) {
    constexpr std::string_view response =
        "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: "
        "close\r\n\r\n";
    SendAll(client, response.data(), response.size());
    return;
  }
  std::string headers =
      "HTTP/1.1 200 OK\r\nContent-Type: " + std::string(MimeType(path)) +
      "\r\nContent-Length: " + std::to_string(resource.size) +
      "\r\nCache-Control: no-cache\r\nConnection: close\r\n";
  if (!resource.content_encoding.empty()) {
    headers.append("Content-Encoding: ");
    headers.append(resource.content_encoding);
    headers.append("\r\n");
  }
  headers.append("\r\n");
  if (SendAll(client, headers.data(), headers.size())) {
    SendAll(client, reinterpret_cast<const char*>(resource.data),
            resource.size);
  }
}

}  // namespace

DevToolsResourceServer::~DevToolsResourceServer() {
  Stop();
}

bool DevToolsResourceServer::Start(int port) {
  if (port <= 0 || port > 65535 || thread_.joinable()) {
    return false;
  }
  WSADATA winsock_data = {};
  if (WSAStartup(MAKEWORD(2, 2), &winsock_data) != 0) {
    return false;
  }
  winsock_started_ = true;
  const SOCKET socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_handle == INVALID_SOCKET) {
    Stop();
    return false;
  }
  sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(static_cast<u_short>(port));
  if (bind(socket_handle, reinterpret_cast<const sockaddr*>(&address),
           sizeof(address)) != 0 ||
      listen(socket_handle, 16) != 0) {
    closesocket(socket_handle);
    Stop();
    return false;
  }
  listen_socket_ = socket_handle;
  thread_ = std::thread(&DevToolsResourceServer::Serve, this);
  return true;
}

void DevToolsResourceServer::Serve() {
  const SOCKET socket_handle = static_cast<SOCKET>(listen_socket_);
  while (!stopping_) {
    const SOCKET client = accept(socket_handle, nullptr, nullptr);
    if (client == INVALID_SOCKET) {
      break;
    }
    HandleRequest(client);
    shutdown(client, SD_BOTH);
    closesocket(client);
  }
}

void DevToolsResourceServer::Stop() {
  stopping_ = true;
  if (listen_socket_ != static_cast<uintptr_t>(INVALID_SOCKET)) {
    const SOCKET socket_handle = static_cast<SOCKET>(listen_socket_);
    shutdown(socket_handle, SD_BOTH);
    closesocket(socket_handle);
    listen_socket_ = static_cast<uintptr_t>(INVALID_SOCKET);
  }
  if (thread_.joinable()) {
    thread_.join();
  }
  if (winsock_started_) {
    WSACleanup();
    winsock_started_ = false;
  }
}

}  // namespace litheview_demo
