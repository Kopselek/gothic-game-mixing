#include <Windows.h>
#include <ZenGin/zGothicAPI.h>

BOOL WINAPI DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.

        switch (GetGameVersion())
        {
#ifdef __G1
            case Engine_G1:
                break;
#endif

#ifdef __G1A
            case Engine_G1A:
                break;
#endif

#ifdef __G2
            case Engine_G2:
                break;
#endif

#ifdef __G2A
            case Engine_G2A:
                break;
#endif

            default:
                return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:
        if (lpvReserved != nullptr)
            break; // do not do cleanup if process termination scenario

        // Perform any necessary cleanup.
        break;

    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;
    }

    return TRUE;
}