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

        MRDesktopClient.setFrameCallback((data, width, height) -> {
            Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            bmp.copyPixelsFromBuffer(ByteBuffer.wrap(data));
            runOnUiThread(() -> imageFrame.setImageBitmap(bmp));
        });

        btnConnect.setOnClickListener(v -> {
            statusText.setText(getString(R.string.connecting));
            String ip = editIP.getText().toString();
            int port = 8080;
            try {
                port = Integer.parseInt(editPort.getText().toString());
            } catch (NumberFormatException ignored) {
            }
            new Thread(() -> {
                boolean ok = client.connect(ip, port);
                runOnUiThread(() -> statusText.setText(ok ? getString(R.string.connected) : getString(R.string.disconnected)));
            }).start();
        });

        btnDisconnect.setOnClickListener(v -> {
            client.disconnect();
            statusText.setText(getString(R.string.disconnected));
        });
    }
}
