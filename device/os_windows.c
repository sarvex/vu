// Copyright © 2013-2022 Galvanized Logic Inc.
// Use is governed by a BSD-style license found in the LICENSE file.

// The microsoft (windows) native layer implementation.
// This wraps the microsoft windowing API's (where the real work is done).

#include <stdio.h>
#include "os_windows.h"

// App Icon related. 101 must be used in the resource file. For example:
//     101 ICON "application.ico"
// See windres for compiling resource files. Compiled resource files are
// included in a golang build using the .syso file type.
#define IDI_APPICON 101

// The golang callbacks that would normally be defined in _cgo_export.h
// are manually reproduced here and also implemented in the native test file
// os_windows_test.c.
extern void handleInput(long event, long data);

// Globals to track the windows window and context handles.
HWND  display;            // Window handle.
HDC   shell;              // Device context handle.

// Global for full screen toggle.
static displayInfo display_screen = {0, 0, 0, 0, {0, 0, 0, 0}};

// Windows callback procedure. Handle a few events often returning 0 to mark
// them as handled. This method is mostly microsoft magic as each event has
// its own behaviour and different return codes.
//
// Called as frequently as possible to process user input and window changes.
LRESULT CALLBACK gs_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch( msg )
    {
        case WM_ACTIVATE:
        {
            if (LOWORD(wParam) != WA_INACTIVE)
            {
                handleInput(devFocusIn, 0);
            }
            else
            {
                handleInput(devFocusOut, 0);
            }
            return 0;
        }
        case WM_SYSCOMMAND:
        {
            if ( (wParam & 0xfff0)  == SC_KEYMENU )
            {
                return 0;
            }
            break;
        }
        case WM_CLOSE:
        {
            PostQuitMessage( 0 );
            return 0;
        }
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN: // mod keys can mask regular keys.
        case WM_SYSKEYUP:   // track key releases like keyup for V in ALT-V.
        {
            long key = wParam;
            if (msg == WM_SYSKEYUP || msg == WM_KEYUP) {
                handleInput(devUp, key);
            }
            if (msg == WM_SYSKEYDOWN || msg == WM_KEYDOWN) {
                handleInput(devDown, key);
            }

            // send SYSKEY events to DefWindowProc so system stuff like
            // tabbing between windows still works.
            if (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            return 0;
        }
        case WM_LBUTTONDOWN:
        {
            SetCapture(display);
            handleInput(devDown, devMouseL);
            return 0;
        }
        case WM_LBUTTONUP:
        {
            handleInput(devUp, devMouseL);
            ReleaseCapture();
            return 0;
        }
        case WM_MBUTTONDOWN:
        {
            SetCapture(display);
            handleInput(devDown, devMouseM);
            return 0;
        }
        case WM_MBUTTONUP:
        {
            handleInput(devUp, devMouseM);
            ReleaseCapture();
            return 0;
        }
        case WM_RBUTTONDOWN:
        {
            SetCapture(display);
            handleInput(devDown, devMouseR);
            return 0;
        }
        case WM_RBUTTONUP:
        {
            handleInput(devUp, devMouseR);
            ReleaseCapture();
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            // flip scroll direction to match OSX.
            long scroll = -1 * (((int)wParam) >> 16) / WHEEL_DELTA;
            handleInput(devScroll, scroll);
            return 0;
        }
        case WM_SIZE:
        {
            if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
            {
                handleInput(devResize, 0);
            }
            return 0;
        }
        case WM_EXITSIZEMOVE:
        {
            handleInput(devResize, 0);
            return 0;
        }
    }

    // Pass all unhandled messages to DefWindowProc
    return DefWindowProc( hwnd, msg, wParam, lParam );
}

// Create the window.
HWND gs_create_window(HMODULE hInstance, LPSTR className)
{
    DWORD style = WS_TILEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    DWORD exStyle = WS_EX_APPWINDOW;

    // calculate the real window size from the desired size.
    RECT desktop;
    long xDefault = 600;
    long yDefault = 400;
    GetWindowRect(GetDesktopWindow(), &desktop);
    RECT rect = {0, 0, xDefault-1, yDefault-1};
    AdjustWindowRectEx( &rect, style, FALSE, exStyle );
    long wWidth = rect.right - rect.left + 1;
    long wHeight = rect.bottom - rect.top + 1;
    long yTop = desktop.bottom - yDefault - wHeight;

    // create the window
    HWND display = CreateWindowEx(
        exStyle,                // Optional styles
        className,              // Window class
        "WinTest",              // Window title
        style,                  // Window style
        600,                    // Size and position.
        yTop,                   // Size and position.
        wWidth,                 // Size and position.
        wHeight,                // Size and position.
        NULL,                   // Parent window
        NULL,                   // Menu
        hInstance,              // Module instance handle.
        NULL                    // Additional app data.
    );
    return display;
}

// =============================================================================
// Native layer wrappers.

// display_init creates a display window. 
// Sets the display and shell globals.
long display_init()
{
     // Get the application instance.
     HMODULE hInstance = GetModuleHandle(NULL);
     LPSTR gs_className = TEXT("GS_WIN");
 
     // Register the window class - once.
     WNDCLASSEX wc;
     wc.cbSize        = sizeof(WNDCLASSEX);
     wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
     wc.lpfnWndProc   = (WNDPROC) gs_wnd_proc;
     wc.cbClsExtra    = 0;
     wc.cbWndExtra    = 0;
     wc.hInstance     = hInstance;
     wc.hIcon         = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 0, 0, LR_SHARED);
     wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
     wc.hbrBackground = NULL;
     wc.lpszMenuName  = NULL;
     wc.lpszClassName = gs_className;
     wc.hIconSm       = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 0, 0, LR_SHARED);
     if(!RegisterClassEx(&wc))
     {
         printf("Failed RegisterClassEx %ld\n", GetLastError());
         return -1;
     }
 
     // Create the initial window.
     display = gs_create_window(hInstance, gs_className);
     shell = GetDC(display);
     if (shell == NULL)
     {
         printf("Failed GetDC %ld\n", GetLastError());
         return -1;
     }
    ShowWindow(display, SW_SHOW);
    SetForegroundWindow(display);
    return 1; 
}

// Destroy the application window using the display and
// shell globals.
void display_dispose()
{
    HDC shell = GetDC(display);
    ReleaseDC(display, shell);
    DestroyWindow(display);
}

// Checks for user input. Returns displayClosed if the user has
// closed the app. Otherwise returns displayActive.
long display_process_input()
{
    MSG msg;
    while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) )
    {
        // treat keys like buttons on a controller, ie: no character
        // input, so don't have to care about translating the virtual keys.
        DispatchMessage( &msg ); // calls gs_wnd_proc for processing.
    }
    if (msg.message == WM_QUIT)
    {
        return displayClosed;
    }
    return displayActive;
}

// Swaps rendering buffer. Called after rendering a frame.
void display_swap()
{
    SwapBuffers(shell);
}

// Used to check if the application is full screen mode.
// Return 1 if the application is full screen, 0 otherwise.
unsigned char display_fullscreen()
{
    return display_screen.full;
}

// Flip full screen mode. Expected to be called after starting
// processing of events. Based on:
// http://src.chromium.org/viewvc/chrome/trunk/src/ui/views/win/
//        fullscreen_handler.cc?revision=HEAD&view=markup
void display_toggle_fullscreen()
{
    if (!display_screen.full)
    {
        display_screen.maxed = IsZoomed(display);
        if (display_screen.maxed)
        {
            SendMessage(display, WM_SYSCOMMAND, SC_RESTORE, 0);
        }
        display_screen.style = GetWindowLong(display, GWL_STYLE);
        display_screen.ex_style = GetWindowLong(display, GWL_EXSTYLE);
        GetWindowRect(display, &display_screen.rect);
    }
    display_screen.full = !display_screen.full;
    if (display_screen.full)
    {
        SetWindowLong(display, GWL_STYLE,
                   display_screen.style & ~(WS_CAPTION | WS_THICKFRAME));
        SetWindowLong(display, GWL_EXSTYLE,
                   display_screen.ex_style & ~(WS_EX_DLGMODALFRAME |
                   WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
        MONITORINFO m_info;
        m_info.cbSize = sizeof(m_info);
        GetMonitorInfo(MonitorFromWindow(display, MONITOR_DEFAULTTONEAREST), &m_info);
        RECT m_rect = m_info.rcMonitor;
        SetWindowPos(display, NULL, m_rect.left, m_rect.top,
                     m_rect.right-m_rect.left, m_rect.bottom-m_rect.top,
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    else
    {
        SetWindowLong(display, GWL_STYLE, display_screen.style);
        SetWindowLong(display, GWL_EXSTYLE, display_screen.ex_style);
        RECT m_rect = display_screen.rect;
        SetWindowPos(display, NULL, m_rect.left, m_rect.top,
                     m_rect.right-m_rect.left, m_rect.bottom-m_rect.top,
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        if (display_screen.maxed)
        {
            SendMessage(display, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }
    }
    PostMessage(display, WM_EXITSIZEMOVE, 0, 0); // Trigger window resize.
}

// Show or hide cursor. Lock it if it is hidden.
void display_show_cursor(unsigned char show) {
    if (show)
    {
        ReleaseCapture();
    }
    else
    {
        SetCapture(display);
    }
    ShowCursor( show );
}

// Get the current mouse position relative to the bottom left corner
// of the application window.
void display_cursor(long *x, long *y)
{
    POINT point;
    GetCursorPos(&point);
    ScreenToClient(display, &point);
    RECT rect;
    GetClientRect(display, &rect);
    *x = point.x;
    *y = rect.bottom - point.y;
}

// Position the cursor at the given window location. The incoming coordinates
// are relative to the bottom left corner - switch that to be relative to the
// top left corner expected by windows.
void display_set_cursor_location(long x, long y)
{
    RECT rect;
    if (GetClientRect(display, &rect) != 0 )
    {
        POINT loc;
        loc.x = x;
        loc.y = rect.bottom - y;
        if (ClientToScreen(display, &loc) != 0 )
        {
            SetCursorPos(loc.x, loc.y);
        }
    }
}

// Sets the windows size and location.
// The y value is reversed because the incoming coordinates are relative
// to the bottom left corner. Windows expects it to be the top left.
void display_set_size(long x, long y, long w, long h)
{
    RECT desk;
    if (GetWindowRect(GetDesktopWindow(), &desk) != 0 )
    {
        RECT wind;
        if (GetWindowRect(display, &wind) != 0 ) {
            RECT disp;
            if (GetClientRect(display, &disp) != 0 )
            {
                int xExtra = wind.right - wind.left - disp.right;
                int yExtra = wind.bottom - wind.top - disp.bottom;
                y = desk.bottom - y - h;
                SetWindowPos(display, HWND_TOP, x, y, w+xExtra, h+yExtra, 0);
            }
        }
    }
}

// Get the current main window drawing area size.
// Reverse y so origin is bottom left.
void display_size(long *x, long *y, long *w, long *h)
{
    RECT rect;
    GetClientRect(display, &rect);
    *w = rect.right - rect.left;
    *h = rect.bottom - rect.top;
    RECT desktop, window;
    GetWindowRect(GetDesktopWindow(), &desktop);
    GetWindowRect(display, &window);
    *x = window.left;
    int yExtra = window.bottom - window.top - rect.bottom;
    *y = desktop.bottom - window.bottom + yExtra;
}

// Sets the windows title.
void display_set_title(char * label)
{
    SetWindowText(display, label);
}

