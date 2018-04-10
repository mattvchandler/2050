package org.mattvchandler.a2050;

import android.content.Context;
import android.content.DialogInterface;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.databinding.DataBindingUtil;
import android.databinding.ObservableFloat;
import android.databinding.ObservableInt;
import android.os.Build;
import android.os.Handler;
import android.os.PowerManager;
import android.support.v4.view.GestureDetectorCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.app.AppCompatDelegate;
import android.util.Log;
import android.util.TypedValue;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.View;

import org.mattvchandler.a2050.databinding.ActivityMainBinding;

import java.io.IOException;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback
{
    static
    {
        System.loadLibrary("2050");
        //AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM);
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);
    }

    private native void create(AssetManager assetManager, String path, Resources resources);
    private native void start();
    private native void resume();
    private native void pause(boolean screen_on);
    private native void stop();
    private native void destroy();
    private native void focus(boolean has_focus);
    private native void surfaceChanged(Surface surface);
    private native void fling(float x, float y);
    private native void newGame();
    private native void pauseGame();
    private native void unpause();
    private native void getUIData(DispData data);

    private ActivityMainBinding binding;
    private DispData data = new DispData();
    private Handler update_data = new Handler();

    private GestureDetectorCompat gestureDetector;

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

        binding = DataBindingUtil.setContentView(this, R.layout.activity_main);
        Log.d("MainActivity", "onCreate");

        binding.surfaceView.getHolder().addCallback(this);
        binding.setData(data);

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
        // TypedValue color = new TypedValue();
        // getTheme().resolveAttribute(android.R.attr.windowBackground, color, true);
        create(getResources().getAssets(), path, getResources());

        int delay = 100; // ms
        update_data.postDelayed(new Runnable()
        {
            @Override
            public void run()
            {
                getUIData(data);
                update_data.postDelayed(this, delay);
            }
        }, delay);
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
        PowerManager powerManager = (PowerManager)getSystemService(Context.POWER_SERVICE);
        boolean screen_on = true;
        if(powerManager != null)
        {
            if(Build.VERSION.SDK_INT >= 20)
                screen_on = powerManager.isInteractive();
            else
                screen_on = powerManager.isScreenOn();
        }
        Log.d("MainActivity", "onPause");
        pause(screen_on);
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
        binding.surfaceView.getHolder().removeCallback(this);

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
    public void surfaceCreated(SurfaceHolder holder) {}

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        Log.d("MainActivity", "surfaceDestroyed");
        surfaceChanged(null);
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
        if(event.getRepeatCount() > 0)
            return super.onKeyDown(keyCode, event);

        switch(keyCode)
        {
            case KeyEvent.KEYCODE_DPAD_UP:
                fling(0.0f, -1.0f);
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                fling(0.0f, 1.0f);
                return true;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                fling(-1.0f, 0.0f);
                return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                fling(1.0f, 0.0f);
                return true;
            case KeyEvent.KEYCODE_DPAD_UP_LEFT:
                fling(-1.0f, -1.0f);
                return true;
            case KeyEvent.KEYCODE_DPAD_UP_RIGHT:
                fling(1.0f, -1.0f);
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN_LEFT:
                fling(-1.0f, 1.0f);
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN_RIGHT:
                fling(1.0f, 1.0f);
                return true;
            default:
                return super.onKeyDown(keyCode, event);
        }
    }

    public void game_win(final int score, final boolean new_high_score)
    {
        Log.d("MainActivity::game_win", "score: " + String.valueOf(score));
        runOnUiThread(() ->
        {
            new AlertDialog.Builder(MainActivity.this).setTitle(R.string.win)
                .setMessage(getResources().getString(R.string.final_score, score)
                        + (new_high_score ?  "\n" + getResources().getString(R.string.new_high_score) : ""))
                .setNegativeButton(R.string.new_game, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        newGame();
                    }
                })
                .setPositiveButton(R.string.continue_playing, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        unpause();
                    }
                })
                .setCancelable(false)
                .show();
        });
    }
    public void game_over(final int score, final boolean new_high_score)
    {
        Log.d("MainActivity::game_over", "score: " + String.valueOf(score));
        runOnUiThread(() ->
        {
            new AlertDialog.Builder(MainActivity.this).setTitle(R.string.game_over)
                .setMessage(getResources().getString(R.string.final_score, score)
                        + (new_high_score ? "\n" + getResources().getString(R.string.new_high_score) : ""))
                .setPositiveButton(R.string.new_game, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        newGame();
                    }
                })
                .setCancelable(false)
                .show();
        });
    }
    public void game_pause()
    {
        Log.d("MainActivity", "game_pause");
        runOnUiThread(() ->
        {
            new AlertDialog.Builder(MainActivity.this).setTitle(R.string.paused)
                .setPositiveButton(R.string.cont, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        unpause();
                    }
                })
                .setOnCancelListener(new DialogInterface.OnCancelListener()
                {
                    @Override
                    public void onCancel(DialogInterface dialog)
                    {
                        unpause();
                    }
                })
                .show();
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        super.onCreateOptionsMenu(menu);
        getMenuInflater().inflate(R.menu.activity_main_action_bar, menu);

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        switch(item.getItemId())
        {
            case R.id.pause:
                pauseGame();
                return true;

            case R.id.new_game:
                newGame();
                return true;
        }
        return false;
    }

    public class DispData
    {
        public final ObservableInt   score      = new ObservableInt();
        public final ObservableInt   high_score = new ObservableInt();
        public final ObservableFloat grav_angle = new ObservableFloat();
    }
}
