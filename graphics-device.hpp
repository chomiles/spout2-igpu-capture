#pragma once
#include <SpoutDX.h>
#include <wrl/client.h>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

struct GraphicsAdapter {
    ComPtr<IDXGIAdapter1> adapter;
    std::string key;
    std::string label;
};

inline std::string adapterName(const wchar_t *name)
{
    char utf8[512]{};
    WideCharToMultiByte(CP_UTF8, 0, name, -1, utf8, sizeof(utf8), nullptr, nullptr);
    return utf8;
}

inline std::vector<GraphicsAdapter> graphicsAdapters()
{
    std::vector<GraphicsAdapter> adapters;
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return adapters;
    std::map<std::string, unsigned> duplicates;
    for (UINT i = 0; ; ++i) {
        ComPtr<IDXGIAdapter1> adapter;
        if (FAILED(factory->EnumAdapters1(i, &adapter)))
            break;
        DXGI_ADAPTER_DESC1 desc{};
        if (FAILED(adapter->GetDesc1(&desc)) || (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
            continue;
        // Use hardware identifiers because a DXGI LUID may change after a reboot.
        char hardware[48]{};
        std::snprintf(hardware, sizeof(hardware), "%04x:%04x:%08x:%02x",
                      desc.VendorId, desc.DeviceId, desc.SubSysId, desc.Revision);
        auto ordinal = ++duplicates[hardware];
        adapters.push_back({adapter, std::string(hardware) + ":" + std::to_string(ordinal),
                            "GPU " + std::to_string(i) + " - " + adapterName(desc.Description)});
    }
    return adapters;
}

inline ComPtr<ID3D11Device> createAdapterDevice(IDXGIAdapter *adapter)
{
    ComPtr<ID3D11Device> device;
    if (FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                                 D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                                 D3D11_SDK_VERSION, &device, nullptr, nullptr)))
        device.Reset();
    return device;
}
