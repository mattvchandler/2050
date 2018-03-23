package org.mattvchandler.a2050;

import android.content.res.AssetManager;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback
{
    static
    {
        System.loadLibrary("native-lib");
    }

    public native void start();
    public native void resume();
    public native void pause();
    public native void stop();
    public native void setSurface(Surface surface);
    public native void setAsset(AssetManager asset_manager);
    public native void changeGravity(float x, float y);

    private GestureDetectorCompat gestureDetector;
    private AssetManager assetManager;

    class GestureListener extends GestureDetector.SimpleOnGestureListener
    {
        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float x, float y)
        {
            changeGravity(x, y);
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
        setContentView(R.layout.activity_main);

        SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surface_view);
        surfaceView.getHolder().addCallback(this);

        gestureDetector = new GestureDetectorCompat(this, new GestureListener());

        assetManager = getResources().getAssets();
        setAsset(assetManager);
    }

    @Override
    protected void onStart()
    {
        super.onStart();
        start();
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        resume();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        pause();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
        stop();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {}

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        // TODO: do we need size; what is format?
        setSurface(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        setSurface(null);
    }
}
