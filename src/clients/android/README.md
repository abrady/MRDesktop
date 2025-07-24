# MRDesktop Android Client

Android client app for connecting to MRDesktop server and controlling Windows/Mac desktop remotely.

## Prerequisites

1. **Android Studio** with Android SDK
2. **Android NDK** (for native C++ compilation)
3. **Gradle** (included with Android Studio)
4. **ADB** (Android Debug Bridge) for device installation

## Building the APK

### Method 1: Using Android Studio
1. Open the `android` folder in Android Studio
2. Let Gradle sync complete
3. Click Build → Build Bundle(s)/APK(s) → Build APK(s)
4. APK will be in `app/build/outputs/apk/debug/`

### Method 2: Command Line
1. Open terminal in the `android` directory
2. Run: `gradlew assembleDebug` (Linux/Mac) or `gradlew.bat assembleDebug` (Windows)
3. Or use the provided batch file: `build_apk.bat`

## Installation

1. Enable "Developer Options" and "USB Debugging" on your Android device
2. Connect device via USB
3. Install APK: `adb install app/build/outputs/apk/debug/app-debug.apk`

## Usage

1. **Start MRDesktop Server** on your PC/Mac
2. **Connect device to same network** as the server
3. **Launch the app** on Android
4. **Enter server IP address** (find it with `ipconfig` on Windows or `ifconfig` on Mac)
5. **Tap Connect** to start desktop streaming
6. **Touch screen to control mouse**:
   - Tap: Left click
   - Long press: Right click  
   - Drag: Move mouse cursor

## Touch Controls

- **Single Tap**: Left mouse click
- **Long Press**: Right mouse click
- **Drag**: Move mouse cursor
- **Back Button**: Disconnect from server

## Network Requirements

- Android device and desktop server must be on the same network
- Default port: 8080
- Make sure Windows Firewall allows the connection

## Troubleshooting

1. **Connection fails**: Check IP address and network connectivity
2. **Video not showing**: Ensure desktop server is running and capturing screen
3. **Touch not working**: Make sure you're connected and server is receiving input
4. **Build errors**: Check NDK installation and Gradle sync

## Architecture

- **Java UI**: Connection management and touch input handling
- **C++ Native**: Network communication and frame rendering
- **Shared Protocol**: Same message format as Windows desktop client
- **RGBA Video**: Real-time desktop frame streaming over TCP