#include "pch.h"
#include "ImguiHandler.h"
#include "LuaHandler.h"
#include "Util/Instance.h"
#include "Memory/Memory.h"
#include "Util/Globals.h"

#include <skStr.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK overlay_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND find_roblox_window() {
    DWORD roblox_pid = mem.current_process.PID;
    if (!roblox_pid) {
        LOGE << skCrypt("[FindWindow] Roblox PID not set").decrypt();
        return nullptr;
    }

    struct hwnd_data { DWORD pid; HWND result; } data = { roblox_pid, nullptr };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        hwnd_data* d = reinterpret_cast<hwnd_data*>(lParam);
        DWORD win_pid;
        GetWindowThreadProcessId(hwnd, &win_pid);
        if (win_pid == d->pid && GetWindow(hwnd, GW_OWNER) == NULL && IsWindowVisible(hwnd)) {
            d->result = hwnd;
            return FALSE;
        }
        return TRUE;
        }, (LPARAM)&data);

    if (!data.result)
        LOGE << skCrypt("[FindWindow] Roblox window not found").decrypt();

    return data.result;
}

void setup_imgui(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);

    auto& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
}

void shutdown_imgui(ID3D11RenderTargetView* render_target, IDXGISwapChain* swap_chain,
    ID3D11DeviceContext* context, ID3D11Device* device, HWND hwnd, WNDCLASSEX& wc) {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    render_target->Release();
    swap_chain->Release();
    context->Release();
    device->Release();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
}

namespace ImguiHandler {
    static std::atomic<bool> imgui_running = false;
    static std::thread imgui_thread;

    bool start_window() {
        if (imgui_running.load()) {
            LOGE << skCrypt("[ImguiHandler] ImGui window already running").decrypt();
            return false;
        }

        try {
            imgui_thread = std::thread([] {
                HWND roblox_hwnd = find_roblox_window();
                if (!roblox_hwnd) return;

                RECT roblox_rect;
                if (!GetWindowRect(roblox_hwnd, &roblox_rect)) return;
                int width = roblox_rect.right - roblox_rect.left;
                int height = roblox_rect.bottom - roblox_rect.top;

                WNDCLASSEX wc = {
                    sizeof(WNDCLASSEX), CS_CLASSDC, overlay_wndproc, 0L, 0L,
                    GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                    L"ImGuiDX11Window", NULL
                };
                RegisterClassEx(&wc);

                HWND hwnd = CreateWindowEx(
                    WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                    wc.lpszClassName, L"", WS_POPUP,
                    roblox_rect.left, roblox_rect.top, width, height,
                    NULL, NULL, wc.hInstance, NULL);

                if (!hwnd) return;

                SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
                ShowWindow(hwnd, SW_SHOW);
                UpdateWindow(hwnd);

                DXGI_SWAP_CHAIN_DESC sd = {};
                sd.BufferCount = 2;
                sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                sd.OutputWindow = hwnd;
                sd.SampleDesc.Count = 1;
                sd.Windowed = TRUE;
                sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

                ID3D11Device* device = nullptr;
                ID3D11DeviceContext* context = nullptr;
                IDXGISwapChain* swap_chain = nullptr;
                ID3D11RenderTargetView* render_target = nullptr;

                if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                    nullptr, 0, D3D11_SDK_VERSION, &sd, &swap_chain, &device, nullptr, &context) != S_OK) return;

                ID3D11Texture2D* back_buffer = nullptr;
                if (FAILED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) return;
                device->CreateRenderTargetView(back_buffer, nullptr, &render_target);
                back_buffer->Release();

                setup_imgui(hwnd, device, context);

                imgui_running.store(true);
                static char buffer[16000] = {};
                MSG msg = {};
                bool last_active = false;

                while (msg.message != WM_QUIT) {
                    if (!GetWindowRect(roblox_hwnd, &roblox_rect)) break;

                    bool active = Globals::menu_open;
                    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
                    SetWindowLong(hwnd, GWL_EXSTYLE, active ?
                        (style & ~WS_EX_TRANSPARENT) :
                        (style | WS_EX_TRANSPARENT));

                    SetWindowPos(hwnd,
                        active ? HWND_TOPMOST : HWND_NOTOPMOST,
                        roblox_rect.left, roblox_rect.top,
                        roblox_rect.right - roblox_rect.left,
                        roblox_rect.bottom - roblox_rect.top,
                        SWP_SHOWWINDOW);

                    while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }

                    if (active && !last_active) {
                        SetForegroundWindow(hwnd);
                        SetFocus(hwnd);
                    }
                    last_active = active;

                    ImGui_ImplDX11_NewFrame();
                    ImGui_ImplWin32_NewFrame();
                    ImGui::NewFrame();

                    if (Globals::menu_open) {
                        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
                        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Once);

                        ImGui::Begin("TECHNO", nullptr, ImGuiWindowFlags_NoSavedSettings);
                        ImGui::Text("Lua");
                        ImGui::InputTextMultiline("##input", buffer, sizeof(buffer), ImVec2(-FLT_MIN, 200),
                            ImGuiInputTextFlags_AllowTabInput);
                        if (ImGui::Button("Execute", ImVec2(-1, 0))) {
                            std::string codeCopy = buffer;
                            std::thread([codeCopy]() {
                                try {
                                    LuaHandler::execute(codeCopy);
                                }
                                catch (const std::exception& e) {
                                    LOGE << "[Lua Thread] std::exception: " << e.what();
                                }
                                catch (...) {
                                    LOGE << "[Lua Thread] unknown exception or longjmp!";
                                }
                                }).detach();
                        }
                        ImGui::End();
                    }

                    ImGui::Render();
                    const float clear_color[4] = { 0, 0, 0, 0 };
                    context->OMSetRenderTargets(1, &render_target, nullptr);
                    context->ClearRenderTargetView(render_target, clear_color);
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                    swap_chain->Present(1, 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }

                shutdown_imgui(render_target, swap_chain, context, device, hwnd, wc);
                });

            imgui_thread.detach();
            return true;
        }
        catch (const std::exception& e) {
            LOGE << skCrypt("[ImguiHandler] Exception: ").decrypt() << e.what();
            return false;
        }
    }
}