# MRDesktop Build Tools

This directory contains build tools and scripts for MRDesktop development.

## 🔧 Android Development Tools

### Quick Start
```batch
# Download Android tools (one-time setup)
download_android_tools.bat

# Build complete APK  
build_apk_standalone.bat
```

### What Gets Downloaded
- **Android NDK r25c** (~1GB) - For native C++ compilation
- **Android SDK Command Line Tools** (~100MB) - For APK building  
- **Gradle Wrapper** (~100MB) - Build system

### Directory Structure (After Download)
```
tools/
├── android-tools/              # ⚠️ .gitignored (downloaded tools)
│   ├── android-ndk-r25c/      # NDK toolchain
│   ├── cmdline-tools/          # SDK tools
│   └── setup_env.bat           # Environment setup
├── download_android_tools.bat  # ✅ Tracked (download script)
└── build_apk_standalone.bat    # ✅ Tracked (build script)
```

## 🚫 What's Not in Git

The `android-tools/` directory is excluded from Git because:
- **Size**: ~1.2GB would bloat the repository
- **Platform-specific**: Different binaries for Windows/Mac/Linux
- **Licensing**: Google's distribution restrictions
- **Reproducible**: Download script ensures consistent versions

## 🔄 CI/CD Usage

```yaml
# GitHub Actions example
- name: Setup Android Tools
  run: tools/download_android_tools.bat
  
- name: Build APK
  run: tools/build_apk_standalone.bat
```

## 🛠 Manual Tool Management

If you prefer to use existing Android SDK/NDK installations:

```batch
# Set environment variables instead of downloading
set ANDROID_HOME=C:\Android\Sdk
set ANDROID_NDK_HOME=C:\Android\Sdk\ndk\25.2.9519653

# Then build as normal
build_android.bat debug
```

## 🧹 Cleanup

To remove downloaded tools:
```batch
rmdir /s /q tools\android-tools
```

The tools will be re-downloaded automatically when needed.