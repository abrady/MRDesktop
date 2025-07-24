package com.mrdesktop.client;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity implements SurfaceHolder.Callback {
    
    // UI Components
    private LinearLayout connectionPanel;
    private LinearLayout controlOverlay;
    private EditText editServerIp;
    private EditText editServerPort;
    private Button btnConnect;
    private Button btnDisconnect;
    private TextView tvStatus;
    private TextView tvFrameInfo;
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    
    // State
    private boolean isConnected = false;
    private Handler mainHandler;
    private float lastTouchX, lastTouchY;
    private boolean isDragging = false;
    
    // Native methods
    public native boolean nativeConnect(String serverIp, int port);
    public native void nativeDisconnect();
    public native void nativeSetSurface(Object surface);
    public native void nativeSendMouseMove(int deltaX, int deltaY);
    public native void nativeSendMouseClick(int button, boolean pressed);
    public native void nativeSendMouseScroll(int deltaX, int deltaY);
    
    // Load native library
    static {
        System.loadLibrary("mrdesktop-android-client");
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        mainHandler = new Handler(Looper.getMainLooper());
        
        initializeViews();
        setupListeners();
    }
    
    private void initializeViews() {
        connectionPanel = findViewById(R.id.connectionPanel);
        controlOverlay = findViewById(R.id.controlOverlay);
        editServerIp = findViewById(R.id.editServerIp);
        editServerPort = findViewById(R.id.editServerPort);
        btnConnect = findViewById(R.id.btnConnect);
        btnDisconnect = findViewById(R.id.btnDisconnect);
        tvStatus = findViewById(R.id.tvStatus);
        tvFrameInfo = findViewById(R.id.tvFrameInfo);
        surfaceView = findViewById(R.id.surfaceView);
        
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
    }
    
    private void setupListeners() {
        btnConnect.setOnClickListener(v -> connectToServer());
        btnDisconnect.setOnClickListener(v -> disconnectFromServer());
        
        // Touch input handling for mouse control
        surfaceView.setOnTouchListener((v, event) -> {
            if (!isConnected) return false;
            
            float x = event.getX();
            float y = event.getY();
            
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    lastTouchX = x;
                    lastTouchY = y;
                    isDragging = false;
                    
                    // Send mouse press
                    nativeSendMouseClick(1, true); // LEFT_BUTTON = 1
                    return true;
                    
                case MotionEvent.ACTION_MOVE:
                    if (!isDragging) {
                        // Check if this is a drag (moved more than threshold)
                        float deltaX = Math.abs(x - lastTouchX);
                        float deltaY = Math.abs(y - lastTouchY);
                        if (deltaX > 10 || deltaY > 10) {
                            isDragging = true;
                            // Release mouse button for drag
                            nativeSendMouseClick(1, false);
                        }
                    }
                    
                    if (isDragging) {
                        // Send mouse movement
                        int dx = (int)(x - lastTouchX);
                        int dy = (int)(y - lastTouchY);
                        nativeSendMouseMove(dx, dy);
                        
                        lastTouchX = x;
                        lastTouchY = y;
                    }
                    return true;
                    
                case MotionEvent.ACTION_UP:
                    if (!isDragging) {
                        // This was a tap, release mouse button
                        nativeSendMouseClick(1, false);
                    }
                    isDragging = false;
                    return true;
            }
            
            return false;
        });
        
        // Long press for right click
        surfaceView.setOnLongClickListener(v -> {
            if (isConnected) {
                nativeSendMouseClick(2, true);  // RIGHT_BUTTON = 2
                mainHandler.postDelayed(() -> nativeSendMouseClick(2, false), 100);
                return true;
            }
            return false;
        });
    }
    
    private void connectToServer() {
        String serverIp = editServerIp.getText().toString().trim();
        String portStr = editServerPort.getText().toString().trim();
        
        if (serverIp.isEmpty() || portStr.isEmpty()) {
            Toast.makeText(this, "Please enter server IP and port", Toast.LENGTH_SHORT).show();
            return;
        }
        
        int port;
        try {
            port = Integer.parseInt(portStr);
        } catch (NumberFormatException e) {
            Toast.makeText(this, "Invalid port number", Toast.LENGTH_SHORT).show();
            return;
        }
        
        tvStatus.setText(R.string.status_connecting);
        btnConnect.setEnabled(false);
        
        // Connect in background thread
        new Thread(() -> {
            boolean connected = nativeConnect(serverIp, port);
            
            mainHandler.post(() -> {
                if (connected) {
                    onConnected();
                } else {
                    onConnectionFailed();
                }
            });
        }).start();
    }
    
    private void disconnectFromServer() {
        new Thread(() -> {
            nativeDisconnect();
            mainHandler.post(this::onDisconnected);
        }).start();
    }
    
    private void onConnected() {
        isConnected = true;
        tvStatus.setText(R.string.status_connected);
        
        // Hide connection panel, show video surface and controls
        connectionPanel.setVisibility(View.GONE);
        surfaceView.setVisibility(View.VISIBLE);
        controlOverlay.setVisibility(View.VISIBLE);
        
        btnConnect.setEnabled(true);
    }
    
    private void onConnectionFailed() {
        isConnected = false;
        tvStatus.setText(R.string.status_error);
        btnConnect.setEnabled(true);
        Toast.makeText(this, "Failed to connect to server", Toast.LENGTH_LONG).show();
    }
    
    private void onDisconnected() {
        isConnected = false;
        tvStatus.setText(R.string.status_disconnected);
        
        // Show connection panel, hide video surface and controls
        connectionPanel.setVisibility(View.VISIBLE);
        surfaceView.setVisibility(View.GONE);
        controlOverlay.setVisibility(View.GONE);
        
        btnConnect.setEnabled(true);
    }
    
    // Called from native code to update frame statistics
    public void updateFrameInfo(int frameCount, float fps, int width, int height) {
        mainHandler.post(() -> {
            String info = String.format("%.1f FPS\\n%dx%d\\n%d frames", fps, width, height, frameCount);
            tvFrameInfo.setText(info);
        });
    }
    
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        // Surface is created, set it in native code
        nativeSetSurface(holder.getSurface());
    }
    
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // Surface changed, update native window
        nativeSetSurface(holder.getSurface());
    }
    
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        // Surface destroyed, clear native window
        nativeSetSurface(null);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isConnected) {
            nativeDisconnect();
        }
    }
    
    @Override
    public void onBackPressed() {
        if (isConnected) {
            disconnectFromServer();
        } else {
            super.onBackPressed();
        }
    }
}