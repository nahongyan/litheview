#include "shared_texture_presenter.h"

#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <cstdint>
#include <unordered_map>

namespace litheview_demo {
namespace {

using Microsoft::WRL::ComPtr;

uint64_t AdapterId(const DXGI_ADAPTER_DESC1& descriptor) {
  return static_cast<uint64_t>(
             static_cast<uint32_t>(descriptor.AdapterLuid.HighPart))
             << 32 |
         descriptor.AdapterLuid.LowPart;
}

DXGI_FORMAT ToDxgiFormat(ltv_gpu_format_t format) {
  switch (format) {
    case LTV_GPU_FORMAT_BGRA8_UNORM:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
    case LTV_GPU_FORMAT_RGBA8_UNORM:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    case LTV_GPU_FORMAT_RGBA16_FLOAT:
      return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default:
      return DXGI_FORMAT_UNKNOWN;
  }
}

}  // namespace

class SharedTexturePresenter::Impl {
 public:
  explicit Impl(HWND target_window) : target_window_(target_window) {}

  bool Present(const ltv_gpu_frame_t& frame) {
    if (!target_window_ || !frame.native_handle || !frame.resource_id ||
        frame.handle_type != LTV_GPU_HANDLE_SHARED_TEXTURE ||
        frame.delivery != LTV_GPU_DELIVERY_GPU_COPY || frame.width == 0 ||
        frame.height == 0 || ToDxgiFormat(frame.format) == DXGI_FORMAT_UNKNOWN) {
      return false;
    }
    if (surface_generation_ != frame.surface_generation ||
        device_id_ != frame.device_id) {
      Reset();
      surface_generation_ = frame.surface_generation;
      device_id_ = frame.device_id;
    }
    ComPtr<ID3D11Texture2D> source = OpenTexture(frame);
    if (!source || !EnsureSwapChain(frame)) {
      return false;
    }
    ComPtr<ID3D11Texture2D> back_buffer;
    if (FAILED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
      return false;
    }
    D3D11_QUERY_DESC query_description = {D3D11_QUERY_EVENT, 0};
    ComPtr<ID3D11Query> completion;
    if (FAILED(device_->CreateQuery(&query_description, &completion))) {
      return false;
    }
    context_->CopyResource(back_buffer.Get(), source.Get());
    context_->End(completion.Get());
    context_->Flush();
    HRESULT completion_result = S_FALSE;
    while ((completion_result =
                context_->GetData(completion.Get(), nullptr, 0, 0)) == S_FALSE) {
      Sleep(0);
    }
    if (FAILED(completion_result)) {
      return false;
    }
    const HRESULT present_result =
        swap_chain_->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
    return SUCCEEDED(present_result) ||
           present_result == DXGI_ERROR_WAS_STILL_DRAWING;
  }

  void Reset() {
    textures_.clear();
    swap_chain_.Reset();
    context_.Reset();
    device_.Reset();
    adapter_.Reset();
    swap_width_ = 0;
    swap_height_ = 0;
    swap_format_ = DXGI_FORMAT_UNKNOWN;
    surface_generation_ = 0;
    device_id_ = 0;
  }

 private:
  ComPtr<ID3D11Texture2D> OpenTexture(const ltv_gpu_frame_t& frame) {
    if (auto found = textures_.find(frame.resource_id); found != textures_.end()) {
      return found->second;
    }
    if (!device_) {
      if (!OpenDeviceAndTexture(frame)) {
        return nullptr;
      }
      return textures_.find(frame.resource_id)->second;
    }
    ComPtr<ID3D11Device1> device1;
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device_.As(&device1)) ||
        FAILED(device1->OpenSharedResource1(
            reinterpret_cast<HANDLE>(frame.native_handle),
            IID_PPV_ARGS(&texture)))) {
      return nullptr;
    }
    textures_.emplace(frame.resource_id, texture);
    return texture;
  }

  bool OpenDeviceAndTexture(const ltv_gpu_frame_t& frame) {
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
      return false;
    }
    for (UINT index = 0;; ++index) {
      ComPtr<IDXGIAdapter1> adapter;
      const HRESULT enumerate_result = factory->EnumAdapters1(index, &adapter);
      if (enumerate_result == DXGI_ERROR_NOT_FOUND ||
          FAILED(enumerate_result)) {
        return false;
      }
      DXGI_ADAPTER_DESC1 descriptor = {};
      if (FAILED(adapter->GetDesc1(&descriptor)) ||
          (frame.device_id && AdapterId(descriptor) != frame.device_id)) {
        continue;
      }
      ComPtr<ID3D11Device> device;
      ComPtr<ID3D11DeviceContext> context;
      if (FAILED(D3D11CreateDevice(
              adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
              D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
              &device, nullptr, &context))) {
        continue;
      }
      ComPtr<ID3D11Device1> device1;
      ComPtr<ID3D11Texture2D> texture;
      if (SUCCEEDED(device.As(&device1)) &&
          SUCCEEDED(device1->OpenSharedResource1(
              reinterpret_cast<HANDLE>(frame.native_handle),
              IID_PPV_ARGS(&texture)))) {
        adapter_ = std::move(adapter);
        device_ = std::move(device);
        context_ = std::move(context);
        textures_.emplace(frame.resource_id, std::move(texture));
        return true;
      }
    }
  }

  bool EnsureSwapChain(const ltv_gpu_frame_t& frame) {
    const DXGI_FORMAT format = ToDxgiFormat(frame.format);
    if (swap_chain_ && swap_width_ == frame.width &&
        swap_height_ == frame.height && swap_format_ == format) {
      return true;
    }
    swap_chain_.Reset();
    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(device_.As(&dxgi_device)) ||
        FAILED(dxgi_device->GetAdapter(&adapter)) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(&factory)))) {
      return false;
    }
    DXGI_SWAP_CHAIN_DESC1 description = {};
    description.Width = frame.width;
    description.Height = frame.height;
    description.Format = format;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    if (FAILED(factory->CreateSwapChainForHwnd(
            device_.Get(), target_window_, &description, nullptr, nullptr,
            &swap_chain_))) {
      return false;
    }
    factory->MakeWindowAssociation(target_window_, DXGI_MWA_NO_ALT_ENTER);
    swap_width_ = frame.width;
    swap_height_ = frame.height;
    swap_format_ = format;
    return true;
  }

  HWND target_window_ = nullptr;
  uint64_t surface_generation_ = 0;
  uint64_t device_id_ = 0;
  UINT swap_width_ = 0;
  UINT swap_height_ = 0;
  DXGI_FORMAT swap_format_ = DXGI_FORMAT_UNKNOWN;
  ComPtr<IDXGIAdapter1> adapter_;
  ComPtr<ID3D11Device> device_;
  ComPtr<ID3D11DeviceContext> context_;
  ComPtr<IDXGISwapChain1> swap_chain_;
  std::unordered_map<uint64_t, ComPtr<ID3D11Texture2D>> textures_;
};

SharedTexturePresenter::SharedTexturePresenter(HWND target_window)
    : impl_(std::make_unique<Impl>(target_window)) {}

SharedTexturePresenter::~SharedTexturePresenter() = default;

bool SharedTexturePresenter::Present(const ltv_gpu_frame_t& frame) {
  return impl_->Present(frame);
}

void SharedTexturePresenter::Reset() {
  impl_->Reset();
}

}  // namespace litheview_demo
