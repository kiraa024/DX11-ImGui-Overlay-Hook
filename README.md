# DX11 ImGui Overlay Hook

A simple DirectX 11 overlay using ImGui and MinHook. It hooks IDXGISwapChain::Present and renders an ImGui interface on top of a DX11 application. This project is intended for learning and testing purposes.

Features
- Hooks DX11 Present using MinHook
- Renders ImGui with imgui_impl_dx11 and imgui_impl_win32
- Custom window procedure for input handling
- Clean initialization and shutdown
- Unload hotkey (END)

Requirements
- Windows
- Visual Studio
- DirectX 11
- ImGui
- MinHook

Building
- Build as a DLL
- Link against d3d11.lib
- Use the correct MinHook library for x86 or x64
- The DLL architecture must match the target application

Usage
- Run a DirectX 11 application (test app recommended)
- Inject the DLL into the process
- The ImGui overlay should appear
- Press END to unload

Notes
- DX11 only
- Not compatible with DX12 or Vulkan
- Intended for offline or test applications only (Works with Grand Theft Auto V Legacy, not safe for Online.)
- Not meant for protected or online games

Inspiration
Based on public DX11 ImGui and MinHook examples. Implementation and structure adapted and modified.

Credits
- ImGui by ocornut
- MinHook by TsudaKageyu
- DX11Hook by CasualCoder91 (https://github.com/CasualCoder91/DX11Hook)

Disclaimer
For educational purposes only. The author is not responsible for misuse.
Note, i made this with the Help of AI. this is my first Real C++ Project. the Code Might be bad.
