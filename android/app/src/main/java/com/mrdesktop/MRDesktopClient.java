package com.mrdesktop;

public class MRDesktopClient {
    static {
        System.loadLibrary("MRDesktopClient");
    }

    /** Callback interface for delivering decoded frames to Java. */
    public interface FrameCallback {
        void onFrame(byte[] data, int width, int height);
    }

    private static FrameCallback frameCallback;

    /** Called from native code whenever a frame is received. */
    private static void onFrameReceived(byte[] data, int width, int height) {
        if (frameCallback != null) {
            frameCallback.onFrame(data, width, height);
        }
    }

    /** Register a callback to receive frames from native code. */
    public static void setFrameCallback(FrameCallback callback) {
        frameCallback = callback;
        nativeSetFrameCallback(MRDesktopClient.class);
    }
    
    // Native method declarations
    public native boolean nativeConnect(String serverIP, int port);
    public native void nativeDisconnect();
    public native boolean nativeIsConnected();
    public native boolean nativeSendMouseMove(int deltaX, int deltaY);
    public native boolean nativeSendMouseMoveAbsolute(int x, int y);
    public native boolean nativeSendMouseClick(int button, boolean pressed);
    public static native void nativeSetFrameCallback(Class<?> clazz);
    
    // Java wrapper methods
    public boolean connect(String serverIP, int port) {
        return nativeConnect(serverIP, port);
    }
    
    public void disconnect() {
        nativeDisconnect();
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
    
    // Frame validation test method similar to console client --test mode
    public void runFrameValidationTest(String serverIP, int port) {
        System.out.println("MRDesktop: Starting frame validation test to " + serverIP + ":" + port);
        
        final int[] frameCount = {0};
        final boolean[] testPassed = {true};
        final boolean[] testCompleted = {false};
        
        // Set up frame callback to validate frames
        setFrameCallback(new FrameCallback() {
            @Override
            public void onFrame(byte[] data, int width, int height) {
                frameCount[0]++;
                System.out.println("TEST: Received frame " + frameCount[0] + " - " + width + "x" + height + " (" + data.length + " bytes)");
                
                // Validate expected dimensions (same as console client test)
                if (width != 640 || height != 480) {
                    System.err.println("TEST FAILED: Expected 640x480, got " + width + "x" + height);
                    testPassed[0] = false;
                }
                
                // Validate data size 
                int expectedSize = 640 * 480 * 4; // ARGB format
                if (data.length != expectedSize) {
                    System.err.println("TEST FAILED: Expected " + expectedSize + " bytes, got " + data.length);
                    testPassed[0] = false;
                }
                
                // Validate test pattern - check a few key pixels
                if (data.length >= expectedSize) {
                    // Check top-left corner pixel (ARGB format)
                    int alpha = data[3] & 0xFF;
                    int red = data[2] & 0xFF;
                    int green = data[1] & 0xFF;
                    int blue = data[0] & 0xFF;
                    
                    if (alpha != 255) {
                        System.err.println("TEST FAILED: Alpha channel not 255 at (0,0)");
                        testPassed[0] = false;
                    }
                    
                    System.out.println("TEST: Frame " + frameCount[0] + " pixel (0,0) = R:" + red + " G:" + green + " B:" + blue + " A:" + alpha);
                }
                
                // Complete test after 3 frames
                if (frameCount[0] >= 3) {
                    testCompleted[0] = true;
                    System.out.println("TEST: Received all 3 frames, test completed");
                }
            }
        });
        
        if (connect(serverIP, port)) {
            System.out.println("MRDesktop: Connected successfully!");
            
            // Wait for frames to be received (up to 10 seconds)
            int maxWaitTime = 10000; // 10 seconds
            int waitTime = 0;
            while (!testCompleted[0] && waitTime < maxWaitTime) {
                try {
                    Thread.sleep(100);
                    waitTime += 100;
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
            
            disconnect();
            
            // Print test results
            System.out.println("\n=== ANDROID FRAME VALIDATION TEST RESULTS ===");
            System.out.println("Total frames received: " + frameCount[0]);
            
            if (testPassed[0] && frameCount[0] >= 3) {
                System.out.println("TEST PASSED: Frame validation successful - " + frameCount[0] + " frames processed correctly!");
            } else {
                System.out.println("TEST FAILED: " + (testPassed[0] ? "Insufficient frames received" : "Frame validation failed"));
            }
            
        } else {
            System.out.println("MRDesktop: Failed to connect!");
        }
    }
}