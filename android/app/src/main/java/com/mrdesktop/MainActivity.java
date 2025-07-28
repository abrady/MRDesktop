package com.mrdesktop;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.View;
import android.view.MotionEvent;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.appcompat.app.AppCompatActivity;

import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity {
    private final MRDesktopClient client = new MRDesktopClient();

    private ImageView imageFrame;
    private EditText editIP;
    private EditText editPort;
    private LinearLayout serverConfigPanel;
    private Button btnConnection;
    private Button btnTest;
    private boolean isConnected = false;
    private int remoteWidth = 0;
    private int remoteHeight = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        imageFrame = findViewById(R.id.imageFrame);
        editIP = findViewById(R.id.editServerIP);
        editPort = findViewById(R.id.editServerPort);
        serverConfigPanel = findViewById(R.id.serverConfigPanel);
        btnConnection = findViewById(R.id.btnConnection);
        btnTest = findViewById(R.id.btnTest);

        // Set default values
        editIP.setText("192.168.1.170");
        editPort.setText("8080");

        // Set up frame callback before any connection attempts
        MRDesktopClient.setFrameCallback((data, width, height) -> {
            remoteWidth = width;
            remoteHeight = height;
            android.util.Log.d("MRDesktop", "Frame callback received: " + data.length + " bytes, " + width + "x" + height);
            try {
                // Check if we have enough data for ARGB_8888 format
                int expectedSize = width * height * 4; // 4 bytes per pixel for ARGB_8888
                if (data.length >= expectedSize) {
                    Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                    bmp.copyPixelsFromBuffer(ByteBuffer.wrap(data));
                    runOnUiThread(() -> {
                        android.util.Log.d("MRDesktop", "Setting bitmap to imageFrame");
                        imageFrame.setImageBitmap(bmp);
                    });
                } else {
                    android.util.Log.e("MRDesktop", "Buffer too small: got " + data.length + " bytes, need " + expectedSize);
                }
            } catch (Exception e) {
                android.util.Log.e("MRDesktop", "Error processing frame: " + e.getMessage());
            }
        });

        imageFrame.setOnTouchListener((v, event) -> {
            if (!isConnected) {
                return true;
            }

            int action = event.getAction();
            if (action == MotionEvent.ACTION_DOWN ||
                action == MotionEvent.ACTION_MOVE ||
                action == MotionEvent.ACTION_UP) {

                int viewW = v.getWidth();
                int viewH = v.getHeight();
                if (viewW == 0 || viewH == 0 || remoteWidth == 0 || remoteHeight == 0) {
                    return true;
                }

                float scale = Math.min((float) viewW / remoteWidth, (float) viewH / remoteHeight);
                float imgW = remoteWidth * scale;
                float imgH = remoteHeight * scale;
                float offsetX = (viewW - imgW) / 2f;
                float offsetY = (viewH - imgH) / 2f;

                float remoteXf = (event.getX() - offsetX) / scale;
                float remoteYf = (event.getY() - offsetY) / scale;
                int remoteX = Math.round(remoteXf);
                int remoteY = Math.round(remoteYf);

                if (remoteX < 0 || remoteX >= remoteWidth || remoteY < 0 || remoteY >= remoteHeight) {
                    return true;
                }

                client.sendMouseMoveAbsolute(remoteX, remoteY);

                if (action == MotionEvent.ACTION_DOWN) {
                    client.sendMouseClick(0, true);
                } else if (action == MotionEvent.ACTION_UP) {
                    client.sendMouseClick(0, false);
                }
            }
            return true;
        });

        btnConnection.setOnClickListener(v -> {
            android.util.Log.d("MRDesktop", "Connection button clicked, isConnected: " + isConnected);
            if (!isConnected) {
                // Connect
                btnConnection.setText("Connecting...");
                btnConnection.setEnabled(false);
                String ip = editIP.getText().toString();
                int port;
                try {
                    port = Integer.parseInt(editPort.getText().toString());
                } catch (NumberFormatException ignored) {
                    port = 8080;
                }
                final String finalIp = ip;
                final int finalPort = port;
                android.util.Log.d("MRDesktop", "Attempting to connect to " + finalIp + ":" + finalPort);
                new Thread(() -> {
                    boolean ok = client.connect(finalIp, finalPort);
                    android.util.Log.d("MRDesktop", "Connection result: " + ok);
                    runOnUiThread(() -> {
                        if (ok) {
                            isConnected = true;
                            btnConnection.setText("Disconnect");
                            btnConnection.setEnabled(true);
                            // Hide server configuration when connected
                            serverConfigPanel.setVisibility(View.GONE);
                            android.util.Log.d("MRDesktop", "Connected successfully, UI updated");
                        } else {
                            btnConnection.setText("Connect");
                            btnConnection.setEnabled(true);
                            android.util.Log.e("MRDesktop", "Connection failed");
                        }
                    });
                }).start();
            } else {
                // Disconnect
                android.util.Log.d("MRDesktop", "Disconnecting...");
                client.disconnect();
                isConnected = false;
                btnConnection.setText("Connect");
                // Show server configuration when disconnected
                serverConfigPanel.setVisibility(View.VISIBLE);
            }
        });

        btnTest.setOnClickListener(v -> {
            android.util.Log.d("MRDesktop", "Test button clicked");
            btnTest.setText("Testing...");
            btnTest.setEnabled(false);
            
            String ip = editIP.getText().toString();
            int port;
            try {
                port = Integer.parseInt(editPort.getText().toString());
            } catch (NumberFormatException ignored) {
                port = 8080;
            }
            
            final String finalIp = ip;
            final int finalPort = port;
            
            new Thread(() -> {
                // Create a new client instance for testing to avoid interfering with main connection
                MRDesktopClient testClient = new MRDesktopClient();
                android.util.Log.d("MRDesktop", "Starting frame validation test...");
                testClient.runFrameValidationTest(finalIp, finalPort);
                
                runOnUiThread(() -> {
                    btnTest.setText("Test");
                    btnTest.setEnabled(true);
                    android.util.Log.d("MRDesktop", "Frame validation test completed");
                });
            }).start();
        });
    }
}
