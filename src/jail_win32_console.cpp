#include "jail_win32_console.h"
#include "jail_windows_h.h"
#undef NOUSER // need WinUser.h for SetWindowPos
#include <Windows.h>

void SetConsolePosition(int x, int y, int w, int h)
{
    HWND con = GetConsoleWindow();
    SetWindowPos(con, HWND_TOP, x, y, w, h, SWP_NOACTIVATE);
}