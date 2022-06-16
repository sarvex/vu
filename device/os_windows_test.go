// Copyright © 2013-2022 Galvanized Logic Inc.
// Use is governed by a BSD-style license found in the LICENSE file.

package device

import (
	"testing"
)

// Tests the go device layer similar to the ".c" version.
// Not a real test since this blocks while the test window
// is up.
func TestDevice(t *testing.T) {
	Display.Init()
	Display.SetTitle("Test Window")
	Display.SetSize(600, 200, 600, 400)
	for {
		// process any user input since last loop and check
		// if the display was closed.
		if !Display.ProcessInput() {
			break
		}
		pressed := Display.Down()
		for k, v := range pressed.Down {
			println("key:val", k, v)
		}
	}
	Display.Dispose()
}
