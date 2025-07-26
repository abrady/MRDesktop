package com.mrdesktop;

public class SimpleClient {
    static {
        System.loadLibrary("MRDesktopSimpleClient");
    }
    
    // Native method declarations
    public native boolean nativeConnect(String serverIP, int port);
    public native void nativeDisconnect();
    public native boolean nativeIsConnected();
    public native boolean nativeSendTestMessage(String message);
    
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
    
    public boolean sendTestMessage(String message) {
        return nativeSendTestMessage(message);
    }
    
    // Test method for emulator
    public void runConnectionTest(String serverIP, int port) {
        System.out.println("MRDesktop: Starting simple connection test to " + serverIP + ":" + port);
        
        if (connect(serverIP, port)) {
            System.out.println("MRDesktop: Connected successfully!");
            
            // Send a test message
            System.out.println("MRDesktop: Sending test message...");
            sendTestMessage("Hello from Android client!");
            
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            
            disconnect();
            System.out.println("MRDesktop: Simple connection test completed");
        } else {
            System.out.println("MRDesktop: Failed to connect!");
        }
    }
}