# MRDesktop

## Setup

### Android

* `setup_android.bat` for a one-time android client build setup
* if this fails because you don't have android studio installed (and you don't want to) run:
  * `tools\download_android_tools.bat`
  * `tools\build_apk_standalone.bat`
* for an OpenXR build targeting Quest 3 use the `android-openxr-arm64-debug` preset (requires `vcpkg` and the `openxr-loader` package)

[README_ANDROID.md](README_ANDROID.md)

## Testing

To test the desktop video streaming:

  Step 1: Build the applications
  configure.bat
  build.bat

  Step 2: Start the server
  Open a command prompt and run:
  run.bat server
  You should see:
  MRDesktop Server - Desktop Duplication Service
  Network and COM initialized successfully
  Primary display: 1920x1080
  Desktop Duplication initialized successfully!
  Server listening on port 8080...
  Waiting for client connection...

  Step 3: Start the client
  Open another command prompt and run:
  run.bat client
  You should see:
  MRDesktop Client - Remote Desktop Viewer
  Connected to MRDesktop Server!
  Receiving desktop stream...
  Saved frame as first_frame.bmp

  What to expect:

* Server shows: "Client connected! Starting desktop streaming..."
* Both show FPS stats every 30 frames
* A first_frame.bmp file is created showing your desktop screenshot
* Console displays streaming statistics

  To stop: Press Ctrl+C in either window.

  Troubleshooting:

* If server fails with "Desktop duplication is not available", close any remote desktop connections
* Make sure no other screen capture software is running
* Check Windows firewall isn't blocking port 8080

 The first_frame.bmp file will prove the desktop capture is working

