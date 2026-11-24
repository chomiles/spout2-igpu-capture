// Validate the installed libobs renderer and the plug-in without opening a broadcast scene.
#include <obs-module.h>
#include <graphics/vec4.h>
#include "graphics-device.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;
constexpr unsigned W = 1280, H = 720;

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    spoutDX sender;
    int count = sender.GetSenderCount();
    for (int i=0; i<count; ++i) {
        char name[256]{};
        if (sender.GetSender(i,name)) std::printf("Available sender: %s\n", name);
    }
    if (argc > 2 && std::string(argv[2]) == "--list") return 0;
    const bool live = argc > 2 && std::string(argv[2]) != "--synthetic";
    // Creating a synthetic Spout sender changes the session's active-sender pointer.
    if (!live && count) {
        std::puts("Existing senders detected; use a live sender name or a separate logon session.");
        return 3;
    }
    if (!obs_startup("en-US", nullptr, nullptr)) return 4;
    obs_video_info video{};
    video.graphics_module = "./libobs-d3d11.dll";
    video.fps_num = 60; video.fps_den = 1;
    video.base_width = video.output_width = W;
    video.base_height = video.output_height = H;
    video.output_format = VIDEO_FORMAT_BGRA;
    video.colorspace = VIDEO_CS_709; video.range = VIDEO_RANGE_FULL;
    if (obs_reset_video(&video) != OBS_VIDEO_SUCCESS) return 5;
    obs_module_t *mod{};
    if (obs_open_module(&mod, argv[1], ".") != MODULE_SUCCESS || !obs_init_module(mod)) return 6;
    const char *name = live ? argv[2] : "Bridge pixel verification";
    ComPtr<ID3D11Device> fixtureDevice;
    // Spout holds a borrowed device pointer. Release its resources before fixtureDevice.
    spoutDX fixture;
    ComPtr<ID3D11Texture2D> input;
    std::vector<uint32_t> pixels;
    if (!live) {
        const std::string gpu = argc > 5 ? argv[5] : "Intel";
        const auto adapters = graphicsAdapters();
        for (const auto &entry : adapters)
            if (entry.label.find(gpu) != std::string::npos) {
                fixtureDevice = createAdapterDevice(entry.adapter.Get());
                break;
            }
        if (!fixtureDevice) return 7;
        fixture.OpenDirectX11(fixtureDevice.Get());
        fixture.SetSenderName(name);
        fixture.SetSenderFormat(DXGI_FORMAT_B8G8R8A8_UNORM);
        D3D11_TEXTURE2D_DESC d{};
        d.Width = W; d.Height = H; d.MipLevels = d.ArraySize = d.SampleDesc.Count = 1;
        d.Format = DXGI_FORMAT_B8G8R8A8_UNORM; d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(fixtureDevice->CreateTexture2D(&d,nullptr,&input))) return 8;
        pixels.resize(W*H, 0);
        for (unsigned y=0; y<H; ++y) {
            std::fill_n(&pixels[y*W + W/3],W/3,0x80800000); // half red, already premultiplied
            std::fill_n(&pixels[y*W + 2*W/3],W-2*W/3,0xff08c832);
        }
    }
    auto *settings = obs_data_create();
    obs_data_set_string(settings, "sender", name);
    if (argc > 3 && std::string(argv[3]) != "auto") {
        const auto adapters = graphicsAdapters();
        const auto selected = std::find_if(adapters.begin(), adapters.end(), [&](const auto &entry) {
            return entry.label.find(argv[3]) != std::string::npos;
        });
        if (selected == adapters.end()) return 14;
        obs_data_set_string(settings, "sender_adapter", selected->key.c_str());
    }
    if (argc > 4 && std::string(argv[4]) == "--e-cores")
        obs_data_set_bool(settings, "efficiency_cores", true);
    auto *src = obs_source_create_private("warudo_intel_spout_bridge", "Bridge check", settings);
    obs_data_release(settings);
    if (!src) return 9;
    auto *properties = obs_source_properties(src);
    auto *choices = obs_properties_get(properties, "sender_adapter");
    for (size_t i = 0; i < obs_property_list_item_count(choices); ++i)
        std::printf("Sender GPU option: %s\n", obs_property_list_item_name(choices, i));
    std::printf("%s\n", obs_property_description(obs_properties_get(properties, "obs_adapter_info")));
    std::printf("E-core checkbox available: %s\n",
                obs_property_enabled(obs_properties_get(properties, "efficiency_cores")) ? "yes" : "no");
    obs_properties_destroy(properties);
    obs_source_inc_showing(src);
    obs_enter_graphics();
    auto *target = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
    auto *readback = gs_stagesurface_create(W,H,GS_BGRA);
    obs_leave_graphics();
    if (!target || !readback) return 10;

    unsigned valid=0, received=0, distinct=0, previous=0;
    bool passedAlpha=false;
    const auto start=Clock::now();
    for (unsigned i=1; i<=180; ++i) {
        if (!live) {
            pixels[0] = 0xff000000 | i;
            fixture.GetDX11Context()->UpdateSubresource(input.Get(),0,nullptr,pixels.data(),4*W,0);
            if (!fixture.SendTexture(input.Get())) return 11;
        }
        if (obs_source_get_width(src)) {
            obs_enter_graphics();
            gs_texrender_reset(target);
            if (gs_texrender_begin(target,W,H)) {
                vec4 blue; vec4_set(&blue,0,0,1,1);
                gs_clear(GS_CLEAR_COLOR,&blue,0,0);
                gs_ortho(0,float(obs_source_get_width(src)),0,float(obs_source_get_height(src)),-100,100);
                obs_source_video_render(src);
                gs_texrender_end(target);
                gs_stage_texture(readback, gs_texrender_get_texture(target));
                uint8_t *data{}; uint32_t pitch{};
                if (gs_stagesurface_map(readback,&data,&pitch)) {
                    ++received;
                    const auto row=data+(H/2)*pitch;
                    const auto transparent=row+4*10, half=row+4*(W/2), opaque=row+4*(W-2);
                    passedAlpha=transparent[0]==255 && transparent[1]==0 && transparent[2]==0 &&
                        std::abs(int(half[0])-127)<=1 && half[1]==0 && std::abs(int(half[2])-128)<=1 &&
                        opaque[0]==50 && opaque[1]==200 && opaque[2]==8;
                    if (passedAlpha) ++valid;
                    if (!live && data[0]!=previous) {++distinct; previous=data[0];}
                    gs_stagesurface_unmap(readback);
                }
            }
            obs_leave_graphics();
        }
        std::this_thread::sleep_until(start+std::chrono::microseconds(16667LL*i));
    }
    std::printf("Source %ux%u; observed=%u, verified alpha/color=%u, distinct test frames=%u\n",
        obs_source_get_width(src),obs_source_get_height(src),received,valid,distinct);
    obs_source_dec_showing(src);
    obs_source_release(src);
    obs_enter_graphics(); gs_texrender_destroy(target); gs_stagesurface_destroy(readback); obs_leave_graphics();
    obs_shutdown();
    return live ? (received ? 0 : 12) : ((valid > 60 && valid==received && distinct>60) ? 0 : 13);
}
