package com.zym.mediaplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    private SurfaceView mSurfaceView;
    private Button mBtn;
    private ZMediaPlayer mZMediaPlayer = new ZMediaPlayer();
    private float mSpeed = 1.0f;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mSurfaceView = findViewById(R.id.surfaceview);
        mBtn = findViewById(R.id.btn);
        mSurfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                mZMediaPlayer.setSurface(holder.getSurface());
                String path = MainActivity.this.getExternalFilesDir("video").getAbsolutePath() + "/video_test.mp4";
                Log.i("ZMediaPlayer", "path:" + path) ;
                mZMediaPlayer.setDataResource(path);
//                mZMediaPlayer.setDataResource(Environment.getExternalStorageDirectory().getAbsolutePath() + "/video_test.mp4");
                mZMediaPlayer.start();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });

        mBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
//                if(mSpeed == 1.0f) {
//                    mSpeed = 1.2f;
//                } else if(mSpeed == 1.2f) {
//                    mSpeed = 0.8f;
//                } else {
//                    mSpeed = 1.0f;
//                }
//                mZMediaPlayer.setPlaybackSpeed(mSpeed);
//                boolean play = mZMediaPlayer.isPlaying();
//                Log.i("ZMediaPlayer", "isPlaying:" + play) ;
//                if(!play) {
//                    mZMediaPlayer.start();
//                } else {
//                    mZMediaPlayer.stop();
//                }
//                String path = MainActivity.this.getExternalFilesDir("video").getAbsolutePath() + "/1111.jpg";
//                Bitmap bitmap = BitmapFactory.decodeFile(path);
//                mZMediaPlayer.setWatermark(bitmap, 0, 0);
                mZMediaPlayer.relase();
                String path = MainActivity.this.getExternalFilesDir("video").getAbsolutePath() + "/video_test.mp4";
                Log.i("ZMediaPlayer", "path:" + path) ;
                mZMediaPlayer.setDataResource(path);
                mZMediaPlayer.start();
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

    }
}
