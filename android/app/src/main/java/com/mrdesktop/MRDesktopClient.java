package com.mrdesktop;

public class MRDesktopClient {
    static {
        System.loadLibrary("MRDesktopAndroidClient");
    }
    
    // Native method declarations
    public native boolean nativeConnect(String serverIP, int port);
    public native void nativeDisconnect();
    public native boolean nativeIsConnected();
    public native boolean nativeSendMouseMove(int deltaX, int deltaY);
    public native boolean nativeSendMouseClick(int button, boolean pressed);
    
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
}