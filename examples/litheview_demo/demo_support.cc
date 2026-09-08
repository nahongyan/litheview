#include "demo_support.h"

#include <imm.h>

#include <algorithm>
#include <cctype>

namespace litheview_demo {

static_assert(sizeof(wchar_t) == sizeof(uint16_t));

HMENU ControlId(int id) {
  return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

std::string WideToUtf8(const std::wstring& value) {
  if (value.empty()) {
    return {};
  }
  const int length = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                         static_cast<int>(value.size()),
                                         nullptr, 0, nullptr, nullptr);
  std::string result(length, '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                      result.data(), length, nullptr, nullptr);
  return result;
}

std::wstring Utf8ToWide(const std::string& value) {
  if (value.empty()) {
    return {};
  }
  const int length = MultiByteToWideChar(
      CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  std::wstring result(length, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                      result.data(), length);
  return result;
}

std::vector<uint16_t> ReadImeUtf16(HIMC context, DWORD index) {
  const LONG byte_length = ImmGetCompositionStringW(context, index, nullptr, 0);
  if (byte_length <= 0 || byte_length % sizeof(uint16_t) != 0) {
    return {};
  }
  std::vector<uint16_t> result(byte_length / sizeof(uint16_t));
  if (ImmGetCompositionStringW(context, index, result.data(), byte_length) !=
      byte_length) {
    return {};
  }
  return result;
}

std::vector<uint8_t> ReadImeBytes(HIMC context, DWORD index) {
  const LONG byte_length = ImmGetCompositionStringW(context, index, nullptr, 0);
  if (byte_length <= 0) {
    return {};
  }
  std::vector<uint8_t> result(byte_length);
  if (ImmGetCompositionStringW(context, index, result.data(), byte_length) !=
      byte_length) {
    return {};
  }
  return result;
}

bool IsImeTargetAttribute(uint8_t attribute) {
  return attribute == ATTR_TARGET_CONVERTED ||
         attribute == ATTR_TARGET_NOTCONVERTED;
}

bool HasExplicitScheme(std::string_view url) {
  const size_t separator = url.find(':');
  if (separator == std::string_view::npos || separator == 0 ||
      !std::isalpha(static_cast<unsigned char>(url.front()))) {
    return false;
  }
  if (!std::all_of(url.begin() + 1, url.begin() + separator, [](char value) {
        const unsigned char character = static_cast<unsigned char>(value);
        return std::isalnum(character) || value == '+' || value == '-' ||
               value == '.';
      })) {
    return false;
  }

  // Treat name:port/path as a host even though the name alone is also a valid
  // RFC scheme token.
  const size_t port_end = url.find_first_of("/?#", separator + 1);
  const std::string_view possible_port =
      url.substr(separator + 1, port_end == std::string_view::npos
                                    ? std::string_view::npos
                                    : port_end - separator - 1);
  return possible_port.empty() ||
         !std::all_of(possible_port.begin(), possible_port.end(),
                      [](char value) {
                        return std::isdigit(static_cast<unsigned char>(value));
                      });
}

bool PreferHttpForHost(std::string_view url) {
  const size_t authority_end = url.find_first_of("/?#");
  std::string_view authority = url.substr(0, authority_end);
  if (const size_t user_info = authority.rfind('@');
      user_info != std::string_view::npos) {
    authority.remove_prefix(user_info + 1);
  }

  std::string_view host = authority;
  if (host.starts_with('[')) {
    const size_t closing_bracket = host.find(']');
    if (closing_bracket != std::string_view::npos) {
      host = host.substr(1, closing_bracket - 1);
    }
  } else if (const size_t port = host.rfind(':');
             port != std::string_view::npos && host.find(':') == port) {
    host = host.substr(0, port);
  }

  std::string lowercase_host(host);
  std::transform(lowercase_host.begin(), lowercase_host.end(),
                 lowercase_host.begin(), [](char value) {
                   return static_cast<char>(
                       std::tolower(static_cast<unsigned char>(value)));
                 });
  const bool is_localhost =
      lowercase_host == "localhost" ||
      (lowercase_host.size() > 10 && lowercase_host.ends_with(".localhost"));
  const bool is_ip_address =
      lowercase_host.find(':') != std::string::npos ||
      (!lowercase_host.empty() &&
       std::all_of(
           lowercase_host.begin(), lowercase_host.end(), [](char value) {
             const unsigned char character = static_cast<unsigned char>(value);
             return std::isdigit(character) || value == '.';
           }));
  return is_localhost || is_ip_address ||
         lowercase_host.find('.') == std::string::npos;
}

std::string FixupUrl(std::string url) {
  const size_t first = url.find_first_not_of(" \t\r\n");
  const size_t last = url.find_last_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  url = url.substr(first, last - first + 1);
  if (!HasExplicitScheme(url)) {
    if (url.starts_with("//")) {
      url.erase(0, 2);
    }
    url.insert(0, PreferHttpForHost(url) ? "http://" : "https://");
  }
  return url;
}

}  // namespace litheview_demo
