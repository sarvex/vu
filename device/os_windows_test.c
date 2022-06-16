// Copyright © 2013-2022 Galvanized Logic Inc.
// Use is governed by a BSD-style license found in the LICENSE file.

//go:build ignore
//
// Ignored because cgo attempts to compile it during normal builds.
// To build a native test application, compile this using:
//     gcc -o winapp os_windows.c os_windows_test.c -lgdi32 -Wall -m64

#include <stdio.h>
#include "os_windows.h"

// Needed to get console debug out from a windowed app.
// From http://dslweb.nwnexus.com/~ast/dload/guicon.htm
#include <fcntl.h>
#include <io.h>
void RedirectIOToConsole();

// Tests windows native library.
// Example C program that ensures the graphic shell works.
// This tests the native layer implmentation without golang.
int main()
{
    RedirectIOToConsole();
    display_init();
    display_set_title("Test Window");
    display_set_size(600, 200, 600, 400);
    long x, y, w, h;
    display_size(&x, &y, &w, &h);
    printf("windows size %ld %ld %ld %ld\n", x, y, w, h);
    while ( 1 )
    {
        // process any user input since last loop and check
        // if the display was closed.
        if ( display_process_input() == displayClosed )
        {
            break; // stop all processing immediately  
        }
        // a game would do updates and rendering here.
    }
    display_dispose();
    return 0;
}

// handleInput is called as user events occur.
// Dumps events to the screen to debug user input handling.
void handleInput(long event, long data)
{
    if (event == devDown) 
    {
        if (data == 0x54) // t key
        { 
            display_toggle_fullscreen();
        } 
        else if (data == devMouseL) 
        { 
            printf("left mouse click\n");
        } 
        else if (data == devMouseR) 
        { 
            printf("right mouse click\n");
        } 
        else if (data == devMouseM) 
        { 
            printf("middle mouse click\n");
        } 
        else 
        {
            printf("press %#lx\n", data);
        }
    } 
    else if (event == devUp) 
    {
        printf("release %#lx\n", data);
    } 
    else if (event == devScroll) 
    {
        printf("scroll %#lx\n", data);
    } 
    else if (event == devResize)  
    {
        printf("resize %#lx\n", event);
    }
    else if (event == devFocusIn)  
    {
        printf("focus in %#lx\n", event);
    }
    else if (event == devFocusOut)  
    {
        printf("focus out %#lx\n", event);
    }
    else 
    {
        printf("event %#lx\n", event);
    }
}

// From http://dslweb.nwnexus.com/~ast/dload/guicon.htm
static const WORD MAX_CONSOLE_LINES = 5000;

// From http://dslweb.nwnexus.com/~ast/dload/guicon.htm
// Associates a console with the GUI app and redirects stdout
// and stderr to the console.
void RedirectIOToConsole()
{
    int hConHandle;
    long lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE *fp;

    // allocate a console for this app
    AllocConsole();

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = MAX_CONSOLE_LINES;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    lStdHandle = (intptr_t)GetStdHandle(STD_OUTPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stdout = *fp;
    setvbuf( stdout, NULL, _IONBF, 0 );

    // redirect unbuffered STDERR to the console
    lStdHandle = (intptr_t)GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stderr = *fp;
    setvbuf( stderr, NULL, _IONBF, 0 );
 }
