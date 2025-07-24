# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MRDesktop is a VR/MR Virtual Desktop project for Meta Quest 3 that enables viewing and controlling a PC or Mac screen inside a VR headset using passthrough mode. The system streams the desktop to the headset in real-time while allowing interactive control through VR controllers.

## Current Project State

This is a new repository containing only documentation (README.md) outlining the technical approach and architecture for building a VR/MR desktop streaming solution. No implementation code exists yet.

## Architecture Overview

The system consists of two main components:

### Desktop Host (PC/Mac Side)
- **Screen Capture**: Uses Desktop Duplication API (Windows) or ScreenCaptureKit (macOS) for efficient frame capture
- **Video Encoding**: Hardware-accelerated H.264/H.265 encoding using GPU encoders (NVENC, AMF, QuickSync, VideoToolbox)
- **Network Streaming**: Low-latency UDP-based streaming protocol or WebRTC for video transmission
- **Input Injection**: Receives VR input commands and injects them as OS-level mouse/keyboard events using SendInput (Windows) or CGEvent (macOS)

### VR Client (Quest 3 Side)
- **Video Decoding**: Hardware-accelerated decoding using Android MediaCodec API
- **VR Rendering**: Displays decoded frames on virtual screen overlay in passthrough MR environment
- **Input Capture**: Converts VR controller interactions (ray casting, triggers) to mouse/keyboard commands
- **Network Communication**: Bidirectional communication for video streaming and input forwarding

## Key Technical Requirements

- **Low Latency**: Target ~20-50ms total round-trip latency for responsive interaction
- **Hardware Acceleration**: Essential for both encoding (host) and decoding (client) to achieve real-time performance
- **Passthrough Integration**: Uses Quest 3's passthrough API to overlay virtual desktop in real environment
- **Cross-Platform Support**: Designed to work on both Windows and macOS hosts

## Referenced Open-Source Projects

The README mentions several relevant open-source projects for reference:
- **ALVR**: VR game streaming with similar capture/encode/stream architecture
- **Sunshine + Moonlight**: Desktop streaming solution using DXGI capture and hardware encoding
- **xrdesktop**: Linux VR desktop environment for UI interaction patterns
- **Remote Desktop Clients for Quest**: VNC/RDP implementations

## Platform Recommendations

- **Windows** recommended for initial development due to mature VR streaming ecosystem and Desktop Duplication API
- **macOS** feasible but requires handling security permissions for screen capture and input control

## Development Notes

- Consider Unity or Unreal Engine for Quest VR app development
- Use native plugins for hardware video decoding on Quest
- Implement modular architecture to support cross-platform host development
- Focus on network optimization and frame synchronization for smooth experience