package org.mattvchandler.a2050;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

// TODO: crashes on rotate

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
    public native void setsurface(Surface surface);

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surface_view);
        surfaceView.getHolder().addCallback(this);

        // TODO: touch events
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
        setsurface(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        setsurface(null);
    }
}
