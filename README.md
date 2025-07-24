# MRDesktop

## Setup

* `setup_android.bat` for a one-time android client build setup
* if this fails because you don't have android studio installed (and you don't want to) run:
  * `tools\download_android_tools.bat`
  * `tools\build_apk_standalone.bat`
* for an OpenXR build targeting Quest 3 use the `android-openxr-arm64-debug` preset (requires `vcpkg` and the `openxr-loader` package)

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

## Building a VR/MR Virtual Desktop for Meta Quest 3

Overview: VR Desktop in Passthrough
A VR/MR virtual desktop lets you see and control your PC or Mac screen inside your Quest 3 headset, even while viewing your real environment via passthrough. On the Quest 3, passthrough mode uses the headset’s cameras to show your surroundings, and a virtual screen can be overlaid in that space. This is similar to commercial solutions like Meta’s Remote Desktop (for Horizon Workrooms) or the Virtual Desktop app, which stream your computer’s display into VR
reddit.com
. The goal is to achieve full interactivity – meaning you can view your desktop and interact with it (move the cursor, type, click, etc.) in real-time through the VR headset. Creating this involves replicating the desktop content to the headset and sending input back to the PC. At a high level, the system will capture the desktop’s video feed, compress and stream it to the Quest, which will display it on a virtual screen in an MR setting. Meanwhile, the Quest sends back your controller or hand inputs as mouse/keyboard events to the host computer. Below, we’ll walk through the technical steps and components needed to build a prototype of this, and highlight open-source tools that can help.
Desktop Capture and Streaming (PC/Mac Side)
The first challenge is capturing the desktop’s video frames efficiently, then streaming them to the headset with minimal latency. Here’s how this process generally works
medium.com
:
Capture the desktop frames – On Windows, the most efficient way is using the Desktop Duplication API (DXGI) introduced in Windows 8.1. This API provides direct access to the GPU’s framebuffer for the desktop, enabling a fast GPU-to-GPU copy of the screen image
medium.com
. It essentially lets you grab the current frame of the desktop as a texture. (In fact, open-source projects like Sunshine use DXGI for capture on Windows
stackoverflow.com
.) This is very fast – one source notes it’s “extremely fast, making it suitable for any activity, including fast-paced gaming”
blevok.com
. If for some reason DXGI Desktop Duplication isn’t available (e.g. on older OS or certain hardware), you could fall back to other methods like GDI capture, but those are much slower (on the order of ~15–20 FPS and higher latency)
blevok.com
. On macOS, you would use a similar screen capture API – modern macOS versions provide ScreenCaptureKit (or AVFoundation/Quartz APIs) for high-performance screen capture
developer.apple.com
. These let you grab the Mac’s display frames efficiently, though the implementation details differ. The key is to get raw frames of the screen as fast as possible.
Compress/encode the frames – Raw uncompressed frames (especially at high resolution and 60+ FPS) are too large to send over Wi-Fi in real-time. For example, a 1080p 60 FPS raw stream would be gigabits per second. Video compression is essential
medium.com
. The typical approach is to encode each frame (or a sequence of frames) with a video codec like H.264 or H.265. H.264 (AVC) is very common and widely supported by hardware; H.265 (HEVC) can offer better compression for the same quality but not all devices support it fully yet. Both the Quest 3 and modern GPUs support H.264 and H.265 decoding/encoding in hardware. For an MVP, H.264 is a safe choice due to broad support
medium.com
. It’s highly recommended to use hardware-accelerated encoding on the PC/Mac for low latency. GPUs have dedicated video encode engines (e.g. NVIDIA’s NVENC, AMD’s AMF/VCE, Intel’s QuickSync, or Apple’s VideoToolbox on macOS). Using these reduces latency and CPU load significantly
medium.com
medium.com
. As the Parsec team noted, relying on GPU video encoders can get encode+decode latency down to ~10 ms, whereas CPU encoding would be much slower
medium.com
. Many open-source projects leverage this: ALVR, an open wireless VR streaming project, requires an NVIDIA, AMD, or Intel GPU with hardware encoding support on the host
github.com
. In summary, your PC-side app should take the captured frames and feed them to a hardware encoder to produce a video stream (ideally in real-time at 60 FPS or more).
Send frames over the network – Once encoded, the video frames need to be transmitted to the Quest headset, typically over Wi-Fi. Most solutions use a low-latency streaming protocol over UDP. For example, Virtual Desktop and ALVR use a custom UDP-based streaming protocol to minimize delay (with their own packet handling, maybe forward error correction, etc.). You could also use WebRTC, which is an open standard for real-time video streaming that handles a lot of networking details (UDP, packet loss recovery, etc.) for you; it introduces a bit of overhead but is a convenient option. Another simple approach is to send the stream as RTP/RTSP or even just raw UDP packets and have the client decode them. (Some projects stream via a generic protocol: Sunshine, for instance, implements NVIDIA’s GameStream protocol which is essentially a form of RTP with video/audio, and clients like Moonlight use that.) The network layer should also carry control/input data (we’ll cover input later), or you can open a separate channel/socket for inputs. Make sure to keep the streaming on the same local network (Quest and host on the same Wi-Fi or the Quest connected via USB) for minimal latency. On a good 5 GHz Wi-Fi, you can expect decent throughput and latency around a few milliseconds for the transport
blevok.com
. Keep in mind to tune the streaming parameters (bitrate, etc.) to avoid Wi-Fi congestion. For an MVP, a straightforward way is to use an existing library (for example, FFmpeg or GStreamer) to send an H.264 stream to the client, or even set up a simple WebRTC connection with a data channel for inputs. The exact method can vary, but the goal is fast, reliable transport of the video frames.
Client receives and decodes – The Quest headset (client side) will receive the video stream. Your Quest app needs to decode the H.264/H.265 frames back into images. This should also use hardware-accelerated decoding on the Quest. The Quest 3 is essentially an Android-based device (running on a Snapdragon XR2 or similar chip), which has dedicated video decode hardware. You can use Android’s MediaCodec API or a native codec library to decode the stream. Some open-source VR streaming clients (like ALVR’s Android client) use NDK/C++ code to decode frames using the codec hardware and get an OpenGL texture. Alternatively, Unity might allow some form of hardware decoding via plugins. If not using a game engine, you could integrate something like FFmpeg on Android, but using FFmpeg’s software decode would likely be too slow for high res + 60 FPS. It’s best to tap into the Quest’s video decoder. The output of decoding each frame will be a pixel buffer or texture that you can then display. This decoding step, if using hardware, can also be very low-latency (just a few milliseconds)
medium.com
.
Render in VR/MR – Finally, the decoded frame is shown in the VR app as a virtual screen. In a Unity or Unreal application (or native OpenXR app), you would create a texture and update it each frame with the decoded image. For example, in Unity you might use a RenderTexture or similar, updated from the video frames. This texture is then mapped onto a floating screen object – typically a flat rectangle or curved surface placed in front of the user. You can position this “virtual monitor” at a comfortable distance (e.g. 1–2 meters away) and scale it to appear as big as desired (like a giant theater screen or a typical monitor size). Because we want passthrough MR, you would enable the Quest’s Passthrough layer as the background. Meta’s Passthrough API (available in the Oculus SDK/Unity Integration or OpenXR extensions) allows you to render the real world from the headset cameras behind your virtual objects
developers.meta.com
. In Unity, for instance, you’d use the OVR Passthrough Layer component to enable the passthrough view behind your scene. The result is that the user sees their real room, and in it a virtual desktop screen floating in space. At this point, the user can see their desktop in real-time in VR. The next crucial piece is making it interactive.
Interactivity and Input Handling
Full interactivity means the user should be able to control the remote desktop as if they were at their PC. In VR, they won’t be using a physical mouse/monitor; instead they’ll use VR controllers (or hand tracking) and potentially a keyboard. Implementing this involves two parts: capturing the user’s intent in VR, and injecting that as input events on the PC/Mac. 1. VR Controls as Mouse/Keyboard: Your VR app needs to provide a way to move a cursor and send clicks/keystrokes. A common approach is to use a laser pointer or ray-casting from the VR controller. For example, you can cast a ray from the Quest controller and find where it intersects the plane of the virtual screen. That gives an (x,y) position on the screen which corresponds to a pixel on the remote desktop. You can render a pointer on the screen at that position for feedback. When the user presses the trigger on the controller, register a mouse click at that location. Essentially, the VR controllers take the place of the mouse
virtualrealitytimes.com
. This concept has been demonstrated in projects like Collabora’s xrdesktop for Linux, where VR controllers allow you to point at windows and even type using VR inputs
virtualrealitytimes.com
. Typing is trickier; you might allow a VR laser pointer to focus a text field and then use an on-screen keyboard or voice dictation. However, since Quest 3 supports Bluetooth keyboards, a simpler route is to pair a physical keyboard to the Quest (so you can physically type, seeing your real keyboard through passthrough). In fact, the official Quest Remote Desktop app lets you use a Bluetooth mouse/keyboard connected to the headset to control the PC
reddit.com
reddit.com
. For an MVP, focusing on pointer/click and basic key input might suffice. 2. Transmitting Input to the Host: Once the VR app captures an input (e.g., “user clicked at (500,300)” or “user pressed the ‘A’ key”), it needs to send that to the PC or Mac, where a companion component will inject it into the OS. You’ll establish a bidirectional communication channel (it could be part of the same streaming socket or a separate TCP/UDP connection, or even piggyback on a WebRTC data channel). The VR client would send messages like “MOVE_MOUSE x,y” or “LEFT_CLICK DOWN/UP” or “KEY_PRESS code”. On the PC side, a small server program will listen for these events. Injecting the input on the PC/Mac can often be done with OS-native APIs:
On Windows: You can use the Win32 SendInput function or the newer Windows Input Simulation APIs to synthesize mouse and keyboard events. For example, to move the mouse you might call SetCursorPos or inject relative movement, and to click you send a mouse down and up event. There are libraries that can do this as well. Another method is to create a virtual HID device or use a driver, but that’s more complex – for an MVP, SendInput (which doesn’t require special privileges if your app is in user space) is adequate.
On macOS: macOS has the CGEvent API (in Quartz) which allows posting keyboard and mouse events to the system. You might need to enable the “Accessibility” permission for your app to allow it to control the computer. Apple’s documentation on generating events or using IOKit for keyboard events could be referenced.
Cross-platform option: If you chose to use a standard protocol like VNC or RDP, the server part would handle injecting input for you. For instance, a VNC server on the PC will accept pointer and key events from the client. But if we’re building custom, we handle it as above.
Thus, the PC host program receives the input command from the Quest, and then calls the appropriate OS functions to simulate that input. The effect is that the remote PC thinks you directly clicked or typed. Combining this with the video stream, you get real-time remote control. In practice, the round-trip latency (capture -> stream -> display -> click -> send back -> inject) needs to be low for it to “feel” interactive. With optimizations (GPU encoding/decoding, good network), this can be on the order of ~20–50 ms total, which is quite usable even for typing or some gaming. (For comparison, typical remote desktop apps over LAN can achieve <50 ms; specialized ones like Parsec or Virtual Desktop aim for ~20 ms or less, which feels almost local
medium.com
medium.com
.) One thing to be careful of is synchronization: you want the input events to align with the correct frame. Usually this isn’t a big problem (a click at a location is processed at whatever the current PC state is), but for fast-moving content (like a game) more advanced systems sometimes predict motion. For a desktop usage scenario, it’s fine to keep it simple.
Choosing Windows or Mac for the MVP
You asked which platform (Windows or macOS) is easier for a minimum viable product. Windows is generally the path of least resistance for a few reasons:
The Quest ecosystem and existing VR desktop streaming tools are heavily Windows-focused. Windows has the Desktop Duplication API for efficient capture
medium.com
, and most VR streaming solutions (Virtual Desktop, ALVR, etc.) target Windows. There’s a lot of sample code and even Microsoft’s Windows Mixed Reality has had features to show desktop windows in VR.
Windows allows relatively straightforward input injection (SendInput) and has widely-available hardware encoder support (NVIDIA/AMD drivers, etc.).
Many open-source projects we’ve mentioned (ALVR, Sunshine, etc.) support Windows as a primary platform. For example, Sunshine explicitly supports Windows (and Linux) but not macOS
github.com
.
macOS is also doable, especially now that Apple has high-performance screen capture APIs (ScreenCaptureKit) and strong GPUs in Apple Silicon. In fact, Meta’s official Remote Desktop has a Mac version, which suggests it installs a driver or uses the macOS screen capture + an RDP-like protocol. The steps on Mac would be analogous: use ScreenCaptureKit or AVCaptureScreenInput to grab frames, use Apple’s VideoToolbox to encode H.264, and send it out. One challenge on Mac is that not all hardware supports modern encoding (older Macs with Intel GPUs might not have H.264 encoders accessible), but any Apple Silicon Mac or recent Intel Mac with a discrete GPU should. Input injection on Mac is possible via CGEvent as mentioned, but you must deal with the security prompts for screen recording and input control (the user would have to grant your app permission). So, it’s a bit more involved to set up. If your goal is fastest prototyping, start with Windows. You can always add Mac support later by abstracting the capture/encode and input parts per OS. If you design the system with a modular approach (separate capture/encode module and a network protocol that’s OS-agnostic), you can make the PC/Mac server cross-platform. For instance, using a library like FFmpeg or GStreamer can give you a unified way to capture and encode on each OS (with the right plugins). But that adds complexity. Many developers choose Windows first because it’s been tried and tested in VR streaming scenarios.
Leveraging Open-Source Projects
To reduce development effort, you can study or even reuse components from existing open-source tools that tackle similar problems:
ALVR (Air Light VR) – This is an open-source project for streaming PC VR games to standalone headsets like the Quest
github.com
. While its primary use-case is gaming (streaming a VR game’s view rather than the desktop UI), the underlying tech is identical: it captures the GPU output (via APIs similar to Desktop Duplication), encodes with NVENC/AMF, streams over Wi-Fi, and the Quest client (built with Android NDK) decodes and displays it in a VR app. ALVR’s code (on GitHub) can provide insight into setting up the video stream and handling tracking data. Notably, ALVR optimizes for low latency by using techniques like asynchronous timewarp on the client. For your desktop viewer, timewarp (reprojection) isn’t critical but it’s interesting to note how they keep motion smooth. ALVR demonstrates how to use the Quest’s hardware decoder and even how to integrate with the Oculus/OpenXR runtime on Quest.
Sunshine + Moonlight – Sunshine is an open-source self-hosted GameStream server, and Moonlight is an open-source client. Sunshine was created to replace NVIDIA’s GameStream, and it can stream your full desktop, not just games. Sunshine’s maintainer has explained that it uses DXGI Desktop Duplication on Windows for capture and also a newer WindowsGraphicsCapture API as an option
stackoverflow.com
. It supports hardware encoding on NVIDIA, AMD, and Intel GPUs
github.com
. Essentially, Sunshine handles the capture, encoding, and sending parts. Moonlight (which runs on many platforms, including Android) handles receiving, decoding, and input forwarding. If you want a quick solution, you could try running Sunshine on your PC and porting or running Moonlight on the Quest. The Moonlight Android client isn’t VR-aware by default (it’s a 2D app), but you could potentially integrate the Moonlight protocol in a custom Quest app to get frames in a texture. Even without VR integration, Sunshine/Moonlight can prove the concept: it’s known to achieve low-latency desktop streaming (comparable to Parsec or NVIDIA’s solution). They are open-source, so you can dig into Sunshine’s code to see how it captures frames and how input messages are structured.
VR Remote Desktop Clients (VNC/RDP) – There are open-source Android clients for traditional remote desktop protocols, which have even been adapted for Quest. For example, the GitHub project wasdwasd0105/remote-desktop-clients-quest contains Android/Quest versions of bVNC (VNC client) and aRDP (RDP client)
github.com
github.com
. The developer packaged these so they can run on the Quest (in 2D mode) and connect to a PC’s VNC or Windows Remote Desktop service. This shows that using an existing protocol is possible. If you went the VNC route: you’d run a VNC server on your PC (like TightVNC, which is free
tightvnc.com
), then modify a VNC client to render on a texture in VR. VNC is not as optimized as a custom video stream (it sends regions of the screen using encodings like JPEG or PNG), but for a simple implementation it works and is cross-platform. RDP (Remote Desktop Protocol) is another option – Windows’ built-in RDP can be very efficient (it can even transmit a video stream via h264 when configured), but implementing an RDP client is complex. The Quest RDP client on SideQuest basically wraps an open-source Android RDP client. The developer noted that RDP is “not a good option for low-latency gaming” but is fine for productivity
reddit.com
. So VNC/RDP might introduce more latency or minor compression artifacts for video, but they benefit from existing stable implementations. As an MVP, you could even sideload a standard Android VNC client on the Quest to test viewing your desktop, though it wouldn’t be in passthrough MR. To get MR, you’d integrate it into a Unity app with passthrough.
xrdesktop (Linux) – While this targets Linux, it’s worth mentioning as an inspiration. xrdesktop (by Collabora) makes the Linux desktop environment available in VR. It modifies the window manager to project windows into a 3D VR space and lets you interact with them using VR controllers
virtualrealitytimes.com
. It’s sponsored by Valve and is fully open-source. If you were interested in a window-level integration (e.g., manipulating individual application windows instead of just a big screen), xrdesktop’s approach is informative. On Windows, we don’t have an exact equivalent open-source project, but conceptually OpenXR could be used to overlay desktop windows in VR. In any case, xrdesktop shows how traditional 2D UI can be combined with VR input in an open-source way. It’s more complex than needed for an MVP (involves custom compositors), but it reinforces the idea that VR controllers = mouse/keyboard and that you can operate a regular desktop in VR
virtualrealitytimes.com
.
Parsec/RustDesk/Others – Parsec (mentioned earlier) is a proprietary low-latency remote desktop geared for gaming. It’s not open-source, but their blog has a great breakdown of the technology (which we cited here)
medium.com
medium.com
. Open-source alternatives like RustDesk or NiceDCV exist for remote desktops. While these aren’t VR-specific, you might glean ideas for network protocols or image capture. For example, RustDesk is an open remote desktop that uses a video codec for efficiency. If you prefer not to start from scratch on the streaming protocol, you could potentially embed an existing remote desktop solution’s core and just change the rendering/input to VR.
Meta’s Remote Desktop (Horizon Workrooms) – This is not open-source, but since it’s free, you can test it to see the user experience they provide. It consists of a PC/Mac companion that streams the desktop to the Quest Workrooms app. Notably, it supports multiple monitors and tracks a physical keyboard. They likely use a combination of a driver for capturing the screen and sending it, and they definitely use a secure channel for input. While you can’t see its code, using it could inform your design (e.g., how they position the screen in passthrough, etc.). In Workrooms, your desktop appears as a floating window on a virtual desk, and you see your real room around it – this is exactly the MR scenario you’re aiming for.
Putting It All Together
Building a VR/MR desktop streamer is an integration of all the above components. In summary, the steps to create a prototype are:
Develop or obtain a PC/Mac capture & streaming app: This program runs on your computer, capturing the screen and sending it out. Start with Desktop Duplication (Windows) or ScreenCaptureKit (Mac) for capturing at 60 FPS. Pipe that into a hardware H.264 encoder. Then transmit the video stream over a network socket (UDP) or framework (WebRTC). This app should also listen for incoming input commands (e.g., on a UDP port or WebRTC data channel) and inject those events into the OS using the appropriate API. You could write this in C++ (possibly using libraries like FFmpeg for encoding), or even in a high-level language if performance-critical parts are in native libraries. Ensure this runs with minimal overhead to achieve low latency. Tools like Sunshine could be repurposed here – e.g., Sunshine already does capture->encode->send; you might modify it to accept input from your Quest app.
Develop the Quest VR app: Using Unity or Unreal Engine is probably the fastest route. Unity, for instance, has the Oculus Integration SDK which provides access to passthrough and VR rendering. In Unity, you’d create a scene with a flat Quad object for the screen, and attach an OVR Passthrough Layer to enable the background passthrough. Write scripts to handle network communication with the PC app. For decoding video on Quest, you might need a native plugin – one approach is to use Unity’s native plugin interface to call Android MediaCodec. Alternatively, there are Unity assets or open-source projects for real-time video decoding. (One hack: You could stream as an MJPEG sequence and use Unity’s VideoPlayer, but that’s not efficient for realtime; stick to real video streams). Once you can get frames into a Texture2D or RenderTexture each update, apply that to the Quad. Then handle input: use OVRInput (for Oculus) or OpenXR Input to get controller poses and button presses. Cast a ray to the Quad to get hit position, draw a cursor. When trigger pressed, send a message to PC app like “click at (normX,normY)”. Also handle other buttons or gestures (perhaps one controller could emulate right-click or bring up a virtual keyboard). If using a physical keyboard via Bluetooth, the Quest might be able to send those keystrokes directly if the PC sees it as a normal input (in Workrooms, the keyboard pairing is used to show your hands, but the actual keystrokes still go from the Quest over the network to the PC). You might for now just implement a basic on-screen keyboard for testing text input, or use the Quest’s voice-to-text as input to send to PC.
Testing and iteration: Try the pipeline on the same network. Tune the encoder bitrate and resolution – Quest 3 has a higher-resolution display than Quest 2, so you might run the virtual desktop at 1080p or 1440p to start (Quest 3 can handle 1600p+ streaming as Virtual Desktop supports up to 2560×1440 or even 4K in some cases
github.com
). Check the latency by moving the mouse or a window and seeing the response in headset. If latency is high, ensure you’re using hardware encode/decode and that your network isn’t the bottleneck. Also ensure the Quest app is running at a full 72 or 90 Hz and not dropping frames when updating the texture (failing to do this can cause lag in display). You may need to implement some buffering or frame pacing – one or two frame buffer queue on the client to smooth network jitter, etc. But too much buffering increases latency, so find a balance.
Throughout, keep in mind that existing open-source code can save you a lot of time. For instance, instead of writing a custom encoder pipeline, you could incorporate FFmpeg: use avcodec with NVENC, pack the H.264 into RTP, and send it. On the client, use FFmpeg’s decoder. Or use GStreamer, which has elements for DXGI capture (dxgiscreencapsrc), h264 encoding (nvh264enc), RTP/RTSP streaming, etc., and similarly on Android for decoding. If you’re comfortable with such frameworks, you could get a basic stream up with minimal coding (just pipeline configuration). The trade-off is less control than writing your own socket protocol. For a prototype though, it might be fine. Finally, ensure you respect security: streaming your desktop should ideally be on a closed network or with encryption if over the internet. If you plan to open-source your implementation, note that some platforms (like NVENC) have license requirements (NVIDIA’s SDK is free to use but check their license). H.264 itself has patent licensing, but for personal or open-source non-commercial projects it’s usually not an issue (many open-source projects use it freely).
Conclusion
In summary, building a VR/MR desktop for the Quest 3 involves capturing your computer’s display, encoding it as a video stream, and rendering it on a virtual screen in the headset, while sending user inputs back to fully control the remote system. The technical steps include using OS-level screen capture APIs (like Desktop Duplication on Windows)
medium.com
, leveraging hardware video encoding/decoding for low latency
medium.com
, streaming over a fast network protocol, and implementing input forwarding using controller rays as mouse pointers
virtualrealitytimes.com
 and OS event injection. There are several open-source projects and tools available that demonstrate pieces of this puzzle – from ALVR and Sunshine for streaming, to modified VNC/RDP clients for Quest – which you can reference or even repurpose in your own prototype. By starting with the simplest working pipeline (even if it’s a modest framerate or uses an existing protocol), you can incrementally improve performance and interactivity. Soon you’ll have a functional virtual desktop in passthrough: imagine wearing the Quest 3, seeing your room around you, and in front of you a huge virtual monitor mirroring your PC – and you can point and click as if it were a physical setup. It’s a challenging but achievable project that combines graphics, networking, and XR development. Good luck, and happy coding! Sources:
Parsec team’s overview of low-latency streaming (desktop capture, compression, etc.)
medium.com
medium.com
Home Theater VR streamer notes on Desktop Duplication vs. other capture methods
blevok.com
blevok.com
Sunshine (open-source Quest streaming host) using DXGI capture on Windows
stackoverflow.com
Collabora xrdesktop project (VR controllers used as mouse/KB for desktop in VR)
virtualrealitytimes.com
Reddit discussion confirming Meta Remote Desktop and Virtual Desktop usage on Quest 3
reddit.com
Citations

is it possible to stream my desktop to my quest 3 in a pass through compatible window, similar to youtubeVR or a system menu? : r/Quest3

<https://www.reddit.com/r/Quest3/comments/1bgtlgw/is_it_possible_to_stream_my_desktop_to_my_quest_3/>

The Technology Behind A Low Latency Cloud Gaming Service | by Parsec | Blog | Game, Work, and Play Together From Anywhere | Parsec | Medium

<https://medium.com/parsec/description-of-parsec-technology-b2738dcc3842>

The Technology Behind A Low Latency Cloud Gaming Service | by Parsec | Blog | Game, Work, and Play Together From Anywhere | Parsec | Medium

<https://medium.com/parsec/description-of-parsec-technology-b2738dcc3842>

c++ - How does sunshine capture and stream the desktop - Stack Overflow

<https://stackoverflow.com/questions/68418755/how-does-sunshine-capture-and-stream-the-desktop>
blevok.com

<https://blevok.com/htvr_monitor_mirroring>
blevok.com

<https://blevok.com/htvr_monitor_mirroring>

ScreenCaptureKit | Apple Developer Documentation

<https://developer.apple.com/documentation/screencapturekit/>

The Technology Behind A Low Latency Cloud Gaming Service | by Parsec | Blog | Game, Work, and Play Together From Anywhere | Parsec | Medium

<https://medium.com/parsec/description-of-parsec-technology-b2738dcc3842>

The Technology Behind A Low Latency Cloud Gaming Service | by Parsec | Blog | Game, Work, and Play Together From Anywhere | Parsec | Medium

<https://medium.com/parsec/description-of-parsec-technology-b2738dcc3842>

GitHub - alvr-org/ALVR: Stream VR games from your PC to your headset via Wi-Fi

<https://github.com/alvr-org/ALVR>
blevok.com

<https://blevok.com/htvr_monitor_mirroring>

Passthrough API Overview | Meta Horizon OS Developers

<https://developers.meta.com/horizon/documentation/unity/unity-passthrough/>

New Open-Source Project Brings VR Linux Desktop – Virtual Reality Times – Metaverse & VR

<https://virtualrealitytimes.com/2019/08/01/new-open-source-project-brings-vr-linux-desktop/>

The first RDP client for Meta Quest : r/OculusQuest

<https://www.reddit.com/r/OculusQuest/comments/1drmpxn/the_first_rdp_client_for_meta_quest/>

The first RDP client for Meta Quest : r/OculusQuest

<https://www.reddit.com/r/OculusQuest/comments/1drmpxn/the_first_rdp_client_for_meta_quest/>

The Technology Behind A Low Latency Cloud Gaming Service | by Parsec | Blog | Game, Work, and Play Together From Anywhere | Parsec | Medium

<https://medium.com/parsec/description-of-parsec-technology-b2738dcc3842>

GitHub - alvr-org/ALVR: Stream VR games from your PC to your headset via Wi-Fi

<https://github.com/alvr-org/ALVR>

GitHub - alvr-org/ALVR: Stream VR games from your PC to your headset via Wi-Fi

<https://github.com/alvr-org/ALVR>

LizardByte/Sunshine: Self-hosted game stream host for Moonlight.

<https://github.com/LizardByte/Sunshine>

GitHub - wasdwasd0105/remote-desktop-clients-quest: VNC, RDP, SPICE, and oVirt/RHEV/Proxmox Clients for meta quest

<https://github.com/wasdwasd0105/remote-desktop-clients-quest>

GitHub - wasdwasd0105/remote-desktop-clients-quest: VNC, RDP, SPICE, and oVirt/RHEV/Proxmox Clients for meta quest

<https://github.com/wasdwasd0105/remote-desktop-clients-quest>

TightVNC: VNC-Compatible Free Remote Desktop Software

<https://www.tightvnc.com/>

The first RDP client for Meta Quest : r/OculusQuest

<https://www.reddit.com/r/OculusQuest/comments/1drmpxn/the_first_rdp_client_for_meta_quest/>

New Open-Source Project Brings VR Linux Desktop – Virtual Reality Times – Metaverse & VR

<https://virtualrealitytimes.com/2019/08/01/new-open-source-project-brings-vr-linux-desktop/>

Releases · guygodin/VirtualDesktop - GitHub

<https://github.com/guygodin/VirtualDesktop/releases>
blevok.com

<https://blevok.com/htvr_monitor_mirroring>
