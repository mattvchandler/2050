package org.mattvchandler.a2050;

import android.content.res.AssetManager;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

import java.io.IOException;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback
{
    static
    {
        System.loadLibrary("2050");
    }

    public native void create(AssetManager assetManager, String path);
    public native void start();
    public native void resume();
    public native void pause();
    public native void stop();
    public native void destroy();
    public native void focus(boolean has_focus);
    public native void surfaceCreated(Surface surface);
    public native void surfaceDestroyed();
    public native void surfaceChanged(Surface surface);
    public native void fling(float x, float y);

    private GestureDetectorCompat gestureDetector;
    private SurfaceView surfaceView;

    class GestureListener extends GestureDetector.SimpleOnGestureListener
    {
        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float x, float y)
        {
            fling(x, y);
            return true;
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        gestureDetector.onTouchEvent(event);
        return super.onTouchEvent(event);
    }

    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.d("MainActivity", "onCreate");

        surfaceView = new SurfaceView(this);
        setContentView(surfaceView);
        //setContentView(R.layout.activity_main);

        // SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surface_view);
        surfaceView.getHolder().addCallback(this);

        gestureDetector = new GestureDetectorCompat(this, new GestureListener());

        /*
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        */

        String path = "";
        try
        {
            path = getFilesDir().getCanonicalPath();
        }
        catch(IOException e)
        {
            Log.e("MainActivity", "Could not get data directory", e);
        }
        create(getResources().getAssets(), path);
    }

    @Override
    protected void onStart()
    {
        super.onStart();
        Log.d("MainActivity", "onStart");
        start();
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        Log.d("MainActivity", "onResume");
        resume();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        Log.d("MainActivity", "onPause");
        pause();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
        Log.d("MainActivity", "onStop");
        stop();
    }
    @Override
    protected void onDestroy()
    {
        Log.d("MainActivity", "onDestroy");
        destroy();

        // SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surface_view);
        surfaceView.getHolder().removeCallback(this);

        super.onDestroy();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        Log.d("MainActivity", "onWindowFocusChanged: " + String.valueOf(hasFocus));
        /*
        if(hasFocus)
        {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
        */
        focus(hasFocus);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        Log.d("MainActivity", "surfaceCreated");
        surfaceCreated(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        Log.d("MainActivity", "surfaceDestroyed");
        surfaceDestroyed();
    }
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        Log.d("MainActivity", "surfaceChanged");
        surfaceChanged(holder.getSurface());
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        return false;
    }

}
