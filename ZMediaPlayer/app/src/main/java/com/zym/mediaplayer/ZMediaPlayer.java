package com.zym.mediaplayer;

import android.graphics.Bitmap;
import android.view.Surface;

/**
 * @author ZYM
 * @since 2019-11-22
 *
 * playback Video without audio
 * */
public class ZMediaPlayer {
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native");
    }

    public ZMediaPlayer() {
        _init();
    }


    public void setDataResource(String path) {
        _setDataResource(path);
    }

    public void setSurface(Surface surface) {
        _setSurface(surface);
    }

    public void start() {
        _start();
    }

    public void pause() {
        _pause();
    }

    public void stop() {
        _stop();
    }

    public void relase() {
        _relase();
    }

    public void setLooping(int loop) {
        _setLooping(loop);
    }

    public void setPlaybackSpeed(float speed) {
        _setPlaybackSpeed(speed);
    }

    public void setWatermark(Bitmap bitmap, int left, int top) {
        _setWatermark(bitmap, left, top);
    }

    /**
     * A native method that is implemented by the 'native' native library,
     * which is packaged with this application.
     */
    private native void _init();
    private native void _setDataResource(String path);
    private native void _setSurface(Surface surface);
    private native void _start();
    private native void _pause();
    private native void _stop();
    private native void _relase();
    private native void _setLooping(int loop);
    private native void _setPlaybackSpeed(float speed);
    private native void _setWatermark(Bitmap bitmap, int left, int top);
}
