// Copyright ©2022 Galvanized Logic Inc
// Use is governed by a BSD-style license found in the LICENSE file.
package main

import (
	"fmt"
	"os"

	"github.com/gazed/vu/device"
)

// sh is used to test and showcase the vu/device package. Just getting a window
// to appear demonstrates that the majority of the device functionality is working.
// The remainder of the example dumps keyboard and mouse events showing that
// user input is being processed. See vu/device package for more information.
func sh() {
	device.Display.Init()
	device.Display.SetTitle("Test Window")
	device.Display.SetSize(600, 200, 600, 400)
	focus := true
	for {
		// process any user input since last loop, stopping
		// if the display was closed.
		input, active := device.Display.GetInput()
		if !active || input == nil {
			break
		}

		// display is still active so dump the user input.delta
		if focus != input.Focus {
			focus = input.Focus
			fmt.Println("focus change", focus)
		}
		if input.Resized {
			x, y, w, h := device.Display.Size()
			fmt.Println("window moved or resized", x, y, w, h)
		}
		if input.Scroll != 0 {
			fmt.Println("scroll", input.Scroll)
		}
		for k, v := range input.Down {
			switch {
			case v == 1:
				fmt.Println("key pressed", k, v)
				switch {
				case k == device.KW:
					fmt.Println("toggled window full screen")
					device.Display.ToggleFullScreen()
				case k == device.KX:
					fmt.Println("close window")
					device.Display.Dispose()
					os.Exit(0)
				}
			case v < 0:
				fmt.Println("key released", k, v)
			}
		}
	}
	device.Display.Dispose()
}
