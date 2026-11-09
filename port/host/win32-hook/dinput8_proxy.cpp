// dinput8.dll proxy: the game imports only DirectInput8Create from dinput8.dll, so a DLL with that one
// export placed next to SPEED2.EXE gets loaded at startup. It installs the hooks, then forwards the call
// to the real system dinput8.dll.
#include "hooks.h"
#include <windows.h>

using DirectInput8Create_t = HRESULT(WINAPI *)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);

extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE inst, DWORD version, REFIID riid,
                                                                    LPVOID *out, LPUNKNOWN outer)
{
    static DirectInput8Create_t real;
    if (!real) {
        char path[MAX_PATH];
        UINT n = GetSystemDirectoryA(path, MAX_PATH);
        lstrcpynA(path + n, "\\dinput8.dll", MAX_PATH - n);
        HMODULE m = LoadLibraryA(path);
        if (m) real = reinterpret_cast<DirectInput8Create_t>(GetProcAddress(m, "DirectInput8Create"));
        hook::log("DirectInput8Create -> %s (%s)", path, real ? "ok" : "NOT FOUND");
        if (!real) return E_FAIL;
    }
    return real(inst, version, riid, out, outer);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        hook::log("nfsu2port: loaded into SPEED2.EXE");
        hook::install_all();
    }
    return TRUE;
}
