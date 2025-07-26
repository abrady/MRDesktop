package com.mrdesktop;

public class TestApp {
    public static void main(String[] args) {
        System.out.println("MRDesktop Android Test App Starting...");
        
        // Default to localhost if no args provided
        String serverIP = "10.0.2.2"; // Default emulator host IP
        int port = 8080;
        
        if (args.length >= 1) {
            serverIP = args[0];
        }
        if (args.length >= 2) {
            try {
                port = Integer.parseInt(args[1]);
            } catch (NumberFormatException e) {
                System.err.println("Invalid port number: " + args[1]);
                return;
            }
        }
        
        MRDesktopClient client = new MRDesktopClient();
        client.runConnectionTest(serverIP, port);
        
        System.out.println("MRDesktop Android Test App Finished.");
    }
}