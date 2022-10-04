// Copyright © 2013-2022 Galvanized Logic Inc.
// Use is governed by a BSD-style license found in the LICENSE file.

// Design Notes:
// Big thanks to GLFW (http://www.glfw.org) from which the minimalist API
// philosophy was borrowed along with which OS specific API's mattered.
// Also thank you to https://github.com/golang/mobile.
// FUTURE: Linux support  : Wayland only, ignore X support.
//                          Use Ubuntu as this is what is expected by Steam.
// FUTURE: Android support: need access to android hardware.
// FUTURE: Apple support  : macos, ios, etc. currently control the game timing
//                          loop by calling back the app when the display needs
//                          a refresh. Wait until a game loop with full timing
//                          control before attempting apple support.

// Package device provides minimal platform/os access to a render display and
// user input. Access to user keyboard and mouse or touch input is provided
// through the Input structure. The application is responsible for providing
// the windowing constructs like buttons, dialogs, sub-panels, text-boxes, etc.
//
// Package device is provided as part of the vu (virtual universe) 3D engine.
package device

// Device wraps OS specific functionality. The expected usage is for
// the application to create the device as follows:
//    device.Display.Init()
// and then loop collecting and processing user input:
//    input, active := device.Display.GetInput()
// Commonly applications are closed when the user closes the device window
// or app. This results in GetInput returning a nil input and active == false. 
type Device interface {
	Init()    // Initialize device and allocate OS resources.
	Dispose() // Stop device and release OS specific resources.

	// Fetches the latest user input. Expected to be called often.
	// Returns true is the display is still active.
	GetInput() (*Input, bool) // user input since last call.

	// Returns the size and position of the screen. Mobile devices
	// are always full screen where x=y=0.
	Size() (x, y, w, h int) // in pixels where 0,0 is bottom left.

	// Windowed (PC) computer API's.
	// The x,y (0,0) coordinates are at the bottom left.
	// The w,h are width, height dimensions in pixels.
	SetSize(x, y, w, h int) // Used to restore from preferences.
	SetTitle(t string)      // Expected to be called once on startup.
	IsFullScreen() bool     // Returns true if window is full screen.
	ToggleFullScreen()      // Flips between full screen and windowed.
	ShowCursor(show bool)   // Displays or hides the cursor.
	SetCursorAt(x, y int)   // Places the cursor at the given window location.
}

// Input is used to communicate user input. Input is mostly the keys that
// are currently pressed and how long they have been
// pressed (measured in update ticks).
type Input struct {
	Mx, My  int  // Current mouse location.
	Scroll  int32  // The amount of scrolling, if any.
	Focus   bool // True if window has focus.
	Resized bool // True if window was resized or moved.

	// Pressed keys keyCodes and pressed duration in ticks.
	// A postitive duration means the key is still being held down.
	// A negative duration means that the key has been released since
	// the last poll. The total pressed duration prior to release can
	// be determined using the difference with KEY_RELEASED.
	Down map[int]int // Pressed keys and pressed duration.
}

// KeyReleased is used to indicate a key up event has occurred.
// The total duration of a key press can be calculated by the difference
// of Input.Down duration with KEY_RELEASED. A user would have to hold
// a key down for 24 hours before the released duration became positive
// (assuming a reasonable update time of 0.02 seconds).
const KeyReleased = -1000000000
