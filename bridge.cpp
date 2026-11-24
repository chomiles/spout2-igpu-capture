// SPDX-License-Identifier: GPL-2.0-or-later
#include <obs-module.h>
#include "graphics-device.hpp"
#include "efficiency-cores.hpp"
#include <shlwapi.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

OBS_DECLARE_MODULE()
using Microsoft::WRL::ComPtr;
using namespace std::chrono_literals;

namespace {
struct Image {
    std::vector<uint8_t> pixels;
    uint32_t width = 0, height = 0;
    gs_color_format format = GS_UNKNOWN;
};

struct Settings {
    std::string sender;
    std::string adapter;
    bool efficiencyCores;
};

Settings readSettings(obs_data_t *settings)
{
    return {obs_data_get_string(settings, "sender"),
            obs_data_get_string(settings, "sender_adapter"),
            obs_data_get_bool(settings, "efficiency_cores")};
}

std::string obsAdapterName()
{
    DXGI_ADAPTER_DESC desc{};
    obs_enter_graphics();
    if (gs_get_device_type() == GS_DEVICE_DIRECT3D_11) {
        auto *device = static_cast<ID3D11Device *>(gs_get_device_obj());
        ComPtr<IDXGIDevice> dxgi;
        ComPtr<IDXGIAdapter> adapter;
        if (device && SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgi))) &&
            SUCCEEDED(dxgi->GetAdapter(&adapter)))
            adapter->GetDesc(&desc);
    }
    obs_leave_graphics();
    return desc.Description[0] ? adapterName(desc.Description) : "Unavailable";
}

ComPtr<ID3D11Device> openSenderDevice(HANDLE shared, const std::string &key, std::string &label)
{
    spoutDirectX directX;
    for (const auto &entry : graphicsAdapters()) {
        if (!key.empty() && key != entry.key)
            continue;
        auto device = createAdapterDevice(entry.adapter.Get());
        ComPtr<ID3D11Texture2D> texture;
        if (device && directX.OpenDX11shareHandle(device.Get(), texture.GetAddressOf(), shared)) {
            label = entry.label;
            return device;
        }
    }
    return {};
}

gs_color_format colorFormat(DXGI_FORMAT f)
{
    switch (f) {
    case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: case DXGI_FORMAT_R8G8B8A8_TYPELESS: return GS_RGBA;
    case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: case DXGI_FORMAT_B8G8R8A8_TYPELESS: return GS_BGRA;
    default: return GS_UNKNOWN;
    }
}

std::string findWarudo(spoutDX &receiver)
{
    const int count = receiver.GetSenderCount();
    for (int i = 0; i < count; ++i) {
        char name[256]{};
        if (receiver.GetSender(i, name) && StrStrIA(name, "Warudo"))
            return name;
    }
    return {};
}

struct Readback {
    ComPtr<ID3D11Texture2D> staging;
    D3D11_TEXTURE2D_DESC desc{};

    bool copy(spoutDX &receiver, ID3D11Texture2D *texture, Image &image) {
        D3D11_TEXTURE2D_DESC src{};
        texture->GetDesc(&src);
        const auto fmt = colorFormat(src.Format);
        if (fmt == GS_UNKNOWN || src.SampleDesc.Count != 1 || src.ArraySize != 1 ||
            src.MipLevels != 1 || !src.Width || !src.Height || src.Width > 8192 || src.Height > 8192)
            return false;
        if (!staging || src.Width != desc.Width || src.Height != desc.Height || src.Format != desc.Format) {
            desc = src;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = desc.MiscFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            staging.Reset();
            if (FAILED(receiver.GetDX11Device()->CreateTexture2D(&desc, nullptr, &staging)))
                return false;
        }
        const uint32_t pitch = src.Width * 4;
        image.pixels.resize(size_t(pitch) * src.Height);
        auto context = receiver.GetDX11Context();
        context->CopyResource(staging.Get(), texture);
        D3D11_MAPPED_SUBRESOURCE map{};
        // Keep this wait on the capture worker, outside the Spout texture lock.
        if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &map)))
            return false;
        auto from = static_cast<const uint8_t *>(map.pData);
        if (map.RowPitch == pitch)
            std::memcpy(image.pixels.data(), from, image.pixels.size());
        else
            for (uint32_t y = 0; y < src.Height; ++y)
                std::memcpy(image.pixels.data() + size_t(y)*pitch, from + size_t(y)*map.RowPitch, pitch);
        context->Unmap(staging.Get(), 0);
        image.width = src.Width;
        image.height = src.Height;
        image.format = fmt;
        return true;
    }
};

class Source {
    std::mutex mutex;
    std::condition_variable changed;
    Settings settings;
    bool visible = false, stopping = false, fresh = false;
    uint64_t generation = 0;
    Image mailbox, upload;
    gs_texture_t *texture = nullptr;
    std::atomic<uint32_t> width{0}, height{0};
    std::thread worker;

    void clearFrame() { mailbox.width = mailbox.height = 0; fresh = true; }

    void capture(uint64_t epoch, const Settings &requested, bool &adapterWarning) {
        // Keep the device alive until after Spout has released its context and textures.
        ComPtr<ID3D11Device> device;
        spoutDX receiver;
        std::string selected = requested.sender.empty() ? findWarudo(receiver) : requested.sender;
        std::string gpu;
        unsigned senderWidth = 0, senderHeight = 0;
        DWORD senderFormat = 0;
        HANDLE shared = nullptr;
        if (!selected.empty() &&
            receiver.GetSenderInfo(selected.c_str(), senderWidth, senderHeight, shared, senderFormat) && shared) {
            device = openSenderDevice(shared, requested.adapter, gpu);
            if (!device && !adapterWarning) {
                blog(LOG_WARNING, "[warudo-spout-bridge] No matching D3D11 adapter for %s (%s)",
                     selected.c_str(), requested.adapter.empty() ? "Automatic" : requested.adapter.c_str());
                adapterWarning = true;
            }
        }
        if (!device) {
            std::unique_lock lock(mutex);
            changed.wait_for(lock, shared ? 2s : 350ms, [&]{ return stopping || generation != epoch; });
            return;
        }
        adapterWarning = false;
        receiver.SetAdapterAuto(false);
        receiver.OpenDirectX11(device.Get());
        receiver.SetReceiverName(selected.c_str());
        Readback readback;
        Image scratch;
        bool online = false, warned = false;
        for (;;) {
            const auto start = std::chrono::steady_clock::now();
            const bool connected = receiver.ReceiveTexture();
            if (connected) {
                receiver.IsUpdated();
                auto *frame = receiver.GetSenderTexture();
                if (frame && receiver.IsFrameNew()) {
                    if (readback.copy(receiver, frame, scratch)) {
                        if (!online)
                            blog(LOG_INFO, "[warudo-spout-bridge] %s -> OBS: %s, %ux%u, DXGI %u (%s GPU selection)",
                                 gpu.c_str(), selected.c_str(), scratch.width, scratch.height,
                                 unsigned(readback.desc.Format), requested.adapter.empty() ? "automatic" : "manual");
                        online = true;
                        warned = false;
                        std::lock_guard lock(mutex);
                        if (stopping || generation != epoch) return;
                        std::swap(scratch, mailbox);
                        fresh = true;
                    } else {
                        if (!warned) blog(LOG_WARNING, "[warudo-spout-bridge] Readback failed for %s (DXGI %u); requires 8-bit RGBA/BGRA", selected.c_str(), unsigned(receiver.GetSenderFormat()));
                        warned = true;
                        if (online) {
                            std::lock_guard lock(mutex);
                            if (generation == epoch) clearFrame();
                            online = false;
                        }
                    }
                }
            }
            std::unique_lock lock(mutex);
            if (!connected) {
                if (online && generation == epoch) clearFrame();
                changed.wait_for(lock, 350ms, [&]{return stopping || generation != epoch;});
                return; // Re-resolve the adapter after sender loss or a GPU change.
            }
            if (changed.wait_until(lock, start + 16667us, [&]{return stopping || generation != epoch;}))
                return;
        }
    }

    void run() {
        std::unique_lock lock(mutex);
        bool efficiencyRequested = false, adapterWarning = false;
        uint64_t previousGeneration = 0;
        while (!stopping) {
            changed.wait(lock, [&]{ return visible || stopping; });
            if (stopping) break;
            auto epoch = generation;
            auto requested = settings;
            lock.unlock();
            if (epoch != previousGeneration) {
                adapterWarning = false;
                previousGeneration = epoch;
            }
            try {
                if (requested.efficiencyCores != efficiencyRequested) {
                    efficiencyRequested = requested.efficiencyCores;
                    const auto ids = efficiencyRequested ? intelEfficiencyCpuSets() : std::vector<ULONG>{};
                    if (!efficiencyRequested ||
                        ids.empty() ||
                        !SetThreadSelectedCpuSets(GetCurrentThread(), ids.data(), ULONG(ids.size()))) {
                        SetThreadSelectedCpuSets(GetCurrentThread(), nullptr, 0);
                        blog(LOG_INFO, "[warudo-spout-bridge] Windows default worker scheduling%s",
                             efficiencyRequested ? " (Intel E-cores unavailable)" : "");
                    } else {
                        blog(LOG_INFO, "[warudo-spout-bridge] Spout worker assigned to %zu Intel E-core CPU sets",
                             ids.size());
                    }
                }
                capture(epoch, requested, adapterWarning);
            }
            catch (const std::exception &e) {
                blog(LOG_ERROR, "[warudo-spout-bridge] %s", e.what());
                std::unique_lock guard(mutex);
                if (generation == epoch) clearFrame();
                changed.wait_for(guard, 2s, [&]{return stopping || generation != epoch;});
            }
            lock.lock();
        }
    }
public:
    explicit Source(Settings config): settings(std::move(config)), worker(&Source::run, this) {}
    ~Source() {
        { std::lock_guard lock(mutex); stopping = true; }
        changed.notify_one();
        worker.join();
        obs_enter_graphics();
        gs_texture_destroy(texture);
        obs_leave_graphics();
    }
    void select(Settings config) {
        std::lock_guard lock(mutex);
        if (settings.sender == config.sender && settings.adapter == config.adapter &&
            settings.efficiencyCores == config.efficiencyCores)
            return;
        settings = std::move(config);
        ++generation; clearFrame(); changed.notify_one();
    }
    std::string adapterKey() {
        std::lock_guard lock(mutex);
        return settings.adapter;
    }
    void show(bool enabled) {
        std::lock_guard lock(mutex);
        if (visible == enabled) return;
        visible = enabled; ++generation; clearFrame(); changed.notify_one();
    }
    void tick() {
        {
            std::lock_guard lock(mutex);
            if (!fresh) return;
            std::swap(upload, mailbox);
            fresh = false;
        }
        obs_enter_graphics();
        if (texture && (!upload.width || width != upload.width || height != upload.height ||
                        gs_texture_get_color_format(texture) != upload.format)) {
            gs_texture_destroy(texture); texture = nullptr;
        }
        if (upload.width) {
            if (!texture) texture = gs_texture_create(upload.width, upload.height, upload.format, 1, nullptr, GS_DYNAMIC);
            if (texture) gs_texture_set_image(texture, upload.pixels.data(), upload.width * 4, false);
        }
        width = texture ? upload.width : 0; height = texture ? upload.height : 0;
        obs_leave_graphics();
    }
    void draw() {
        if (!texture) return;
        gs_blend_state_push();
        gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
        auto effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
        while (gs_effect_loop(effect, "Draw")) obs_source_draw(texture, 0, 0, 0, 0, false);
        gs_blend_state_pop();
    }
    uint32_t w() const {return width.load();}
    uint32_t h() const {return height.load();}
};
}

bool obs_module_load()
{
    obs_source_info info{};
    info.id = "warudo_intel_spout_bridge";
    info.type = OBS_SOURCE_TYPE_INPUT;
    info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
    info.get_name = [](void*) {return "Spout2 iGPU Capture";};
    info.create = [](obs_data_t *s, obs_source_t*) -> void* {
        try { return new Source(readSettings(s)); }
        catch (...) { return nullptr; }
    };
    info.destroy = [](void *p){delete static_cast<Source*>(p);};
    info.update = [](void *p, obs_data_t *s){static_cast<Source*>(p)->select(readSettings(s));};
    info.get_properties = [](void *data) {
        auto *props = obs_properties_create();
        auto *list = obs_properties_add_list(props, "sender", "Spout Sender",
                                             OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
        obs_property_list_add_string(list, "Auto-select Warudo Sender", "");
        spoutDX enumerate;
        const int count = enumerate.GetSenderCount();
        for (int i=0; i<count; ++i) {
            char name[256]{};
            if (enumerate.GetSender(i,name)) obs_property_list_add_string(list,name,name);
        }
        auto *adapters = obs_properties_add_list(props, "sender_adapter", "Spout Sender GPU",
                                                  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
        obs_property_list_add_string(adapters, "Auto-detect Sender GPU", "");
        obs_property_set_long_description(adapters, "Select the GPU used by the app sending Spout frames.");
        const auto saved = data ? static_cast<Source *>(data)->adapterKey() : std::string{};
        bool found = saved.empty();
        for (const auto &entry : graphicsAdapters()) {
            obs_property_list_add_string(adapters, entry.label.c_str(), entry.key.c_str());
            found |= entry.key == saved;
        }
        if (!found) {
            const auto index = obs_property_list_add_string(adapters, "Saved GPU Unavailable", saved.c_str());
            obs_property_list_item_disable(adapters, index, true);
        }
        const auto destination = "OBS Render GPU: " + obsAdapterName();
        obs_properties_add_text(props, "obs_adapter_info", destination.c_str(), OBS_TEXT_INFO);
        auto *cores = obs_properties_add_bool(props, "efficiency_cores", "Use Intel E-cores for Capture");
        obs_property_set_long_description(cores, "Runs the Spout capture thread on Intel E-cores when available.");
        obs_property_set_enabled(cores, !intelEfficiencyCpuSets().empty());
        return props;
    };
    info.show = [](void *p){static_cast<Source*>(p)->show(true);};
    info.hide = [](void *p){static_cast<Source*>(p)->show(false);};
    info.video_tick = [](void *p,float){static_cast<Source*>(p)->tick();};
    info.video_render = [](void *p,gs_effect_t*){static_cast<Source*>(p)->draw();};
    info.get_width = [](void *p){return static_cast<Source*>(p)->w();};
    info.get_height = [](void *p){return static_cast<Source*>(p)->h();};
    obs_register_source(&info);
    return true;
}
