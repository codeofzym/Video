package com.zym.mediaplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    @Override
    protected void onResume() {
        super.onResume();
        ZMediaPlayer player = new ZMediaPlayer();
        player.start();
        player.pause();
        player.stop();
        player.relase();
        player.setDataResource("");
        player.setSurface(null);
        player.setLooping(1);
        player.setPlaybackSpeed(1.0f);
        player.setWatermark(null, 0, 0);
    }
}
