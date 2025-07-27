package com.mrdesktop;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity {
    private final MRDesktopClient client = new MRDesktopClient();

    private ImageView imageFrame;
    private EditText editIP;
    private EditText editPort;
    private TextView statusText;
    private Button btnConnect;
    private Button btnDisconnect;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        imageFrame = findViewById(R.id.imageFrame);
        editIP = findViewById(R.id.editServerIP);
        editPort = findViewById(R.id.editServerPort);
        statusText = findViewById(R.id.textStatus);
        btnConnect = findViewById(R.id.btnFullConnect);
        btnDisconnect = findViewById(R.id.btnFullDisconnect);

        // Set default values
        editIP.setText("10.0.2.2");
        editPort.setText("8080");

        MRDesktopClient.setFrameCallback((data, width, height) -> {
            try {
                // Check if we have enough data for ARGB_8888 format
                int expectedSize = width * height * 4; // 4 bytes per pixel for ARGB_8888
                if (data.length >= expectedSize) {
                    Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                    bmp.copyPixelsFromBuffer(ByteBuffer.wrap(data));
                    runOnUiThread(() -> imageFrame.setImageBitmap(bmp));
                } else {
                    android.util.Log.e("MRDesktop", "Buffer too small: got " + data.length + " bytes, need " + expectedSize);
                }
            } catch (Exception e) {
                android.util.Log.e("MRDesktop", "Error processing frame: " + e.getMessage());
            }
        });

        btnConnect.setOnClickListener(v -> {
            statusText.setText(getString(R.string.connecting));
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
                boolean ok = client.connect(finalIp, finalPort);
                runOnUiThread(() -> statusText.setText(ok ? getString(R.string.connected) : getString(R.string.disconnected)));
            }).start();
        });

        btnDisconnect.setOnClickListener(v -> {
            client.disconnect();
            statusText.setText(getString(R.string.disconnected));
        });
    }
}
