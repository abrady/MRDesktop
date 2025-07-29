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

        // Note: This MainActivity is a legacy test interface
        // For VR/OpenGL applications, use Surface-based rendering instead
        // See SurfaceTestActivity or runSurfacePerformanceTest() for modern approach

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
            // Launch Surface performance test activity
            android.content.Intent intent = new android.content.Intent(this, SurfaceTestActivity.class);
            startActivity(intent);
        });
    }
}
