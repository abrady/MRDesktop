package com.mrdesktop;

import android.graphics.SurfaceTexture;
import android.view.Surface;

public class MRDesktopClient {
    static {
        System.loadLibrary("MRDesktopClient");
    }

    /** Callback interface for Surface-based frame rendering (GPU-optimized). */
    public interface SurfaceFrameCallback {
        void onFrameAvailable(int textureId, int width, int height);
    }

    private static SurfaceFrameCallback surfaceFrameCallback;
    private SurfaceTexture surfaceTexture;
    private Surface surface;
    private int textureId;

    /** Called from native code when a Surface frame is available. */
    private static void onSurfaceFrameAvailable(int textureId, int width, int height) {
        if (surfaceFrameCallback != null) {
            surfaceFrameCallback.onFrameAvailable(textureId, width, height);
        }
    }

    /** Register a callback for Surface-based frame rendering. */
    public void setSurfaceFrameCallback(SurfaceFrameCallback callback) {
        surfaceFrameCallback = callback;
        nativeSetSurfaceFrameCallback(MRDesktopClient.class);
    }

    /** Initialize Surface for MediaCodec decoding. */
    public boolean initializeSurface(int textureId) {
        this.textureId = textureId;
        
        // Create SurfaceTexture from the provided GL_TEXTURE_EXTERNAL_OES texture
        surfaceTexture = new SurfaceTexture(textureId);
        
        // Create Surface from SurfaceTexture
        surface = new Surface(surfaceTexture);
        
        // Pass the Surface to native code for MediaCodec configuration
        return nativeInitializeSurface(surface);
    }

    /** Update texture image for Surface decoding (call this in your render loop). */
    public void updateTexImage() {
        if (surfaceTexture != null) {
            surfaceTexture.updateTexImage();
        }
    }

    /** Get transform matrix for Surface texture. */
    public void getTransformMatrix(float[] matrix) {
        if (surfaceTexture != null && matrix.length >= 16) {
            surfaceTexture.getTransformMatrix(matrix);
        }
    }

    /** Clean up Surface resources. */
    public void cleanup() {
        if (surface != null) {
            surface.release();
            surface = null;
        }
        if (surfaceTexture != null) {
            surfaceTexture.release();
            surfaceTexture = null;
        }
        nativeCleanupSurface();
    }
    
    // Native method declarations
    public native boolean nativeConnect(String serverIP, int port);
    public native void nativeDisconnect();
    public native boolean nativeIsConnected();
    public native boolean nativeSendMouseMove(int deltaX, int deltaY);
    public native boolean nativeSendMouseMoveAbsolute(int x, int y);
    public native boolean nativeSendMouseClick(int button, boolean pressed);
    public static native void nativeSetSurfaceFrameCallback(Class<?> clazz);
    public native boolean nativeInitializeSurface(Surface surface);
    public native void nativeCleanupSurface();
    
    // Java wrapper methods
    public boolean connect(String serverIP, int port) {
        return nativeConnect(serverIP, port);
    }
    
    public void disconnect() {
        nativeDisconnect();
        // Clean up Surface resources on disconnect
        cleanup();
    }
    
    public boolean isConnected() {
        return nativeIsConnected();
    }
    
    public boolean sendMouseMove(int deltaX, int deltaY) {
        return nativeSendMouseMove(deltaX, deltaY);
    }

    public boolean sendMouseMoveAbsolute(int x, int y) {
        return nativeSendMouseMoveAbsolute(x, y);
    }
    
    public boolean sendMouseClick(int button, boolean pressed) {
        return nativeSendMouseClick(button, pressed);
    }
    
    // Test method for emulator
    public void runConnectionTest(String serverIP, int port) {
        System.out.println("MRDesktop: Starting connection test to " + serverIP + ":" + port);
        
        if (connect(serverIP, port)) {
            System.out.println("MRDesktop: Connected successfully!");
            
            // Wait a bit to receive frames
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            
            // Test sending some mouse movements
            System.out.println("MRDesktop: Testing mouse movements...");
            sendMouseMove(10, 10);
            sendMouseMove(-5, 5);
            
            // Test mouse click
            System.out.println("MRDesktop: Testing mouse click...");
            sendMouseClick(0, true);  // Left click down
            sendMouseClick(0, false); // Left click up
            
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            
            disconnect();
            System.out.println("MRDesktop: Connection test completed");
        } else {
            System.out.println("MRDesktop: Failed to connect!");
        }
    }
    
    /** 
     * Performance test using Surface decoding (GPU-optimized for VR/OpenGL applications).
     * This eliminates CPU-based YUV→RGB conversion for significantly better performance.
     */
    public void runSurfacePerformanceTest(String serverIP, int port, int glTextureId) {
        System.out.println("MRDesktop: Starting Surface-based performance test");
        
        final int[] frameCount = {0};
        final long[] totalTime = {0};
        final long startTime = System.currentTimeMillis();
        
        // Set up Surface frame callback
        setSurfaceFrameCallback(new SurfaceFrameCallback() {
            @Override
            public void onFrameAvailable(int textureId, int width, int height) {
                frameCount[0]++;
                long currentTime = System.currentTimeMillis();
                totalTime[0] = currentTime - startTime;
                
                if (frameCount[0] % 30 == 0) { // Log every 30 frames
                    double fps = (frameCount[0] * 1000.0) / totalTime[0];
                    System.out.println("SURFACE PERFORMANCE: " + frameCount[0] + " frames, " + 
                                     String.format("%.2f", fps) + " FPS, " +
                                     "Texture: " + textureId + ", " +
                                     "Resolution: " + width + "x" + height);
                }
                
                // Update texture for rendering (this is where your OpenGL code would use the texture)
                updateTexImage();
            }
        });
        
        if (initializeSurface(glTextureId) && connect(serverIP, port)) {
            System.out.println("MRDesktop: Connected with Surface rendering (GPU-accelerated)");
            
            // Run performance test for 10 seconds
            try {
                Thread.sleep(10000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            
            disconnect();
            
            // Final performance report
            double totalSeconds = totalTime[0] / 1000.0;
            double avgFPS = frameCount[0] / totalSeconds;
            
            System.out.println("\n=== SURFACE PERFORMANCE TEST RESULTS ===");
            System.out.println("Total frames: " + frameCount[0]);
            System.out.println("Total time: " + String.format("%.2f", totalSeconds) + " seconds");
            System.out.println("Average FPS: " + String.format("%.2f", avgFPS));
            System.out.println("Method: MediaCodec Surface decoding (GPU-based)");
            System.out.println("CPU YUV→RGB conversion: ELIMINATED");
            System.out.println("Ready for VR/OpenGL rendering!");
            
        } else {
            System.out.println("MRDesktop: Failed to initialize Surface or connect!");
        }
    }
}