package com.zym.mediaplayer;

import android.graphics.Bitmap;
import android.util.Log;
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

    /**
     * set path of video file from disk
     * */
    public void setDataResource(String path) {
        _setDataResource(path);
    }

    /**
     * set surface used by player to display
     * */
    public void setSurface(Surface surface) {
        _setSurface(surface);
    }

    /**
     * start playback video
     * */
    public void start() {
        _start();
    }

    /**
     * pause playback video
     * */
    public void pause() {
        _pause();
    }

    /**
     * stop playback video
     * */
    public void stop() {
        _stop();
    }

    /**
     * relase source of video
     * */
    public void release() {
        _release();
    }

    /**
     * obtain playstate
     * @return  true:Playing false:other
     * */
    public boolean isPlaying() {
        return _isPlaying() == 1;
    }

    /**
     * set playback is looping
     * @param loop true:playback is looping
     *             false:playback is not looping
     * */
    public void setLooping(boolean loop) {
        _setLooping(loop ? 1 : 0);
    }

    /**
     * set speed of playback
     * @param speed 1.0f is normal speed
     * */
    public void setPlaybackSpeed(float speed) {
        _setPlaybackSpeed(speed);
    }

    /**
     * set watermark and location of the watermark
     *
     * @param bitmap data of watermark
     * @param left distance that left of the screen
     * @param top distance that top of the screen
     * */
    public void setWatermark(Bitmap bitmap, int left, int top) {
        _setWatermark(bitmap, left, top);
    }

    /**
     * set index of frame to pause
     *
     * @param frames the end index of to show in playback
     * */
    public void setBreakPointFrameIndex(int[] frames) {
        if(frames == null || frames.length < 1) {
            return;
        }
        _setBreakPointFrameIndex(frames);
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
    private native void _release();
    private native int _isPlaying();
    private native void _setLooping(int loop);
    private native void _setPlaybackSpeed(float speed);
    private native void _setWatermark(Bitmap bitmap, int left, int top);
    private native void _setBreakPointFrameIndex(int[] frames);
}
