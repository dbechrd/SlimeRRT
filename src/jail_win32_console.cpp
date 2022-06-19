#include "jail_win32_console.h"
#include "jail_windows_h.h"
#undef NOUSER // need WinUser.h for SetWindowPos
#include <Windows.h>
#include <cstdio>

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType);

void InitConsole(void)
{
    // TODO: Handle console hard exits properly to ensure server flushes logs and sends disconnect packet when possible
    //SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void InitConsole(int x, int y, int w, int h)
{
    InitConsole();
    SetConsolePosition(x, y, w, h);
}

void ShowConsole(void)
{
    AllocConsole();
}

void HideConsole(void)
{
    FreeConsole();
}

void SetConsolePosition(int x, int y, int w, int h)
{
    HWND con = GetConsoleWindow();
    SetWindowPos(con, HWND_TOP, x, y, w, h, SWP_NOACTIVATE);
}

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType)
{
    BOOL retVal = FALSE;
    switch (fdwCtrlType) {
        // Handle the CTRL-C signal.
        case CTRL_C_EVENT:
            printf("Ctrl-C event\n\n");
            Beep(640, 100);
            retVal = TRUE;
            break;
        // CTRL-CLOSE: confirm that the user wants to exit.
        case CTRL_CLOSE_EVENT:
            Beep(640, 100);
            Beep(640, 100);
            printf("Ctrl-Close event\n\n");
            retVal = TRUE;
            break;
        // Pass other signals to the next handler.
        case CTRL_BREAK_EVENT:
            Beep(640, 100);
            Beep(640, 100);
            Beep(640, 100);
            printf("Ctrl-Break event\n\n");
            retVal = FALSE;
            break;
        case CTRL_LOGOFF_EVENT:
            Beep(640, 100);
            Beep(640, 100);
            Beep(640, 100);
            Beep(640, 100);
            printf("Ctrl-Logoff event\n\n");
            retVal = FALSE;
            break;
        case CTRL_SHUTDOWN_EVENT:
            Beep(640, 100);
            Beep(640, 100);
            Beep(640, 100);
            Beep(640, 100);
            Beep(640, 100);
            printf("Ctrl-Shutdown event\n\n");
            retVal = FALSE;
            break;
    }
    return retVal;
}