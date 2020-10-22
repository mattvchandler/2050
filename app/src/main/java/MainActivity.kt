// Copyright 2019 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

package org.mattvchandler.a2050

import android.animation.ArgbEvaluator
import android.app.Dialog
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.res.AssetManager
import android.content.res.ColorStateList
import android.content.res.Resources
import android.graphics.BlendMode
import android.graphics.BlendModeColorFilter
import android.graphics.PorterDuff
import android.graphics.drawable.LayerDrawable
import android.hardware.display.DisplayManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.*
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.widget.Toolbar
import androidx.core.hardware.display.DisplayManagerCompat
import androidx.core.view.GestureDetectorCompat
import androidx.core.widget.ImageViewCompat
import androidx.databinding.DataBindingUtil
import androidx.databinding.ObservableFloat
import androidx.databinding.ObservableInt
import androidx.fragment.app.DialogFragment
import androidx.preference.PreferenceManager
import org.mattvchandler.a2050.databinding.ActivityMainBinding
import java.io.IOException

class MainActivity: Themed_activity(), SurfaceHolder.Callback
{
    private val SETTINGS_RESULT = 1

    private lateinit var binding: ActivityMainBinding
    private val data = DispData()
    private val update_data = Handler(Looper.getMainLooper())
    private var dialog: AlertDialog? = null

    private var gravity_mode = false

    private lateinit var gestureDetector: GestureDetectorCompat

    companion object
    {
        init
        {
            System.loadLibrary("2050")
        }

        @JvmStatic private external fun calcTextColor(color: Int): Int
        @JvmStatic private external fun ballColorIndex(size: Int, num_colors: Int): Int
    }

    private external fun create(assetManager: AssetManager, path: String, resources: Resources, gravity_mode: Boolean, rotation: Int)
    private external fun resume()
    private external fun pause()
    private external fun stop()
    private external fun destroy()
    private external fun focus(has_focus: Boolean)
    private external fun surfaceChanged(surface: Surface?)
    private external fun fling(x: Float, y: Float)
    private external fun newGame()
    private external fun pauseGame()
    private external fun getUIData(data: DispData)

    override fun onCreate(savedInstanceState: Bundle?)
    {
        super.onCreate(savedInstanceState)

        gravity_mode = PreferenceManager.getDefaultSharedPreferences(this).getBoolean("gravity", false)
        if(gravity_mode)
        {
            requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
            window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
        else
        {
            requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_USER
            window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }

        binding = DataBindingUtil.setContentView(this, R.layout.activity_main) ?: throw NullPointerException("Could not get binding")

        setSupportActionBar(binding.toolbar as Toolbar)

        binding.surfaceView.holder.addCallback(this)
        binding.data = data

        gestureDetector = GestureDetectorCompat(this, GestureListener())

        var path = ""
        try
        {
            path = filesDir.canonicalPath
        }
        catch(e: IOException)
        {
            Log.e("MainActivity", "Could not get data directory", e)
        }

        // TODO: replace with API 30 fullscreen methods once there is a compat version for all needed features
        window.decorView.setOnSystemUiVisibilityChangeListener { visibility ->
            if(visibility and (View.SYSTEM_UI_FLAG_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0)
                supportActionBar?.show()
            else
                supportActionBar?.hide()
        }

        val rotation = DisplayManagerCompat.getInstance(this).getDisplay(Display.DEFAULT_DISPLAY)?.rotation

        create(resources.assets, path, resources, gravity_mode, rotation!!)
    }

    override fun onResume()
    {
        super.onResume()

        if(!hasWindowFocus())
            pauseGame()

        resume()

        val delay: Long = 100 // ms
        update_data.postDelayed(object: Runnable
        {
            override fun run()
            {
                getUIData(data)

                val color_stops = resources.getIntArray(R.array.pressure_colors)
                if(color_stops.size <= 1)
                    throw AssertionError("Not enough color stops for pressure_colors")

                val percent = data.pressure.get().toFloat() / 100.0f

                var start_color_i = (percent * (color_stops.size.toFloat() - 1)).toInt()
                if(start_color_i >= color_stops.size - 1)
                    start_color_i = color_stops.size - 2

                val stop_percent = (color_stops.size - 1).toFloat() * percent - start_color_i.toFloat()

                val color = ArgbEvaluator().evaluate(stop_percent, color_stops[start_color_i], color_stops[start_color_i + 1]) as Int

                if(Build.VERSION.SDK_INT < 29)
                    (binding.pressure.progressDrawable as LayerDrawable).findDrawableByLayerId(android.R.id.progress).mutate().setColorFilter(color, PorterDuff.Mode.SRC_IN)
                else
                    (binding.pressure.progressDrawable as LayerDrawable).findDrawableByLayerId(android.R.id.progress).mutate().colorFilter = BlendModeColorFilter(color, BlendMode.SRC_IN)


                update_data.postDelayed(this, delay)
            }
        }, delay)
    }

    override fun onPause()
    {
        super.onPause()
        pause()

        update_data.removeCallbacksAndMessages(null)
    }

    override fun onStop()
    {
        super.onStop()
        stop()
    }

    override fun onDestroy()
    {
        destroy()

        dialog?.dismiss()

        binding.surfaceView.holder.removeCallback(this)

        super.onDestroy()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean)
    {
        super.onWindowFocusChanged(hasFocus)
        focus(hasFocus)
    }

    override fun surfaceCreated(holder: SurfaceHolder)
    {
    }

    override fun surfaceDestroyed(holder: SurfaceHolder)
    {
        surfaceChanged(null)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int)
    {
        surfaceChanged(holder.surface)
    }

    internal inner class GestureListener: GestureDetector.SimpleOnGestureListener()
    {
        override fun onFling(e1: MotionEvent, e2: MotionEvent, x: Float, y: Float): Boolean
        {
            fling(x, y)
            return true
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean
    {
        if(!gravity_mode)
            gestureDetector.onTouchEvent(event)
        return super.onTouchEvent(event)
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean
    {
        if(gravity_mode || event.repeatCount > 0)
            return super.onKeyDown(keyCode, event)

        return when(keyCode)
        {
            KeyEvent.KEYCODE_DPAD_UP ->         { fling( 0.0f, -1.0f); true }
            KeyEvent.KEYCODE_DPAD_DOWN ->       { fling( 0.0f,  1.0f); true }
            KeyEvent.KEYCODE_DPAD_LEFT ->       { fling(-1.0f,  0.0f); true }
            KeyEvent.KEYCODE_DPAD_RIGHT ->      { fling( 1.0f,  0.0f); true }
            KeyEvent.KEYCODE_DPAD_UP_LEFT ->    { fling(-1.0f, -1.0f); true }
            KeyEvent.KEYCODE_DPAD_UP_RIGHT ->   { fling( 1.0f, -1.0f); true }
            KeyEvent.KEYCODE_DPAD_DOWN_LEFT ->  { fling(-1.0f,  1.0f); true }
            KeyEvent.KEYCODE_DPAD_DOWN_RIGHT -> { fling( 1.0f,  1.0f); true }

            else -> super.onKeyDown(keyCode, event)
        }
    }

    fun game_win(score: Int, new_high_score: Boolean)
    {
        runOnUiThread {
            class Game_win_frag: DialogFragment()
            {
                override fun onCreateDialog(savedInstanceState: Bundle?) =
                    AlertDialog.Builder(activity as MainActivity)
                            .setTitle(R.string.win)
                            .setMessage(resources.getString(R.string.final_score, requireArguments().getInt("score")) + if(requireArguments().getBoolean("new_high_score")) {"\n" + resources.getString(R.string.new_high_score)} else "")
                            .setNegativeButton(R.string.new_game) { _, _ -> (activity as MainActivity).newGame() }
                            .setPositiveButton(R.string.continue_playing, null)
                            .create()
            }

            val bundle = Bundle()
            bundle.putInt("score", score)
            bundle.putBoolean("new_high_score", new_high_score)

            val frag = Game_win_frag()
            frag.arguments = bundle
            frag.isCancelable = false

            frag.show(supportFragmentManager, "game_win_dialog")
        }
    }

    fun game_over(score: Int, new_high_score: Boolean)
    {
        runOnUiThread {
            class Game_over_frag: DialogFragment()
            {
                override fun onCreateDialog(savedInstanceState: Bundle?) =
                    AlertDialog.Builder(activity as MainActivity).setTitle(R.string.game_over)
                            .setMessage(resources.getString(R.string.final_score, requireArguments().getInt("score")) + if(requireArguments().getBoolean("new_high_score")) "\n" + resources.getString(R.string.new_high_score) else "")
                            .setPositiveButton(R.string.new_game) { _, _ -> (activity as MainActivity).newGame() }
                            .create()
            }
            val bundle = Bundle()
            bundle.putInt("score", score)
            bundle.putBoolean("new_high_score", new_high_score)

            val frag = Game_over_frag()
            frag.arguments = bundle
            frag.isCancelable = false

            frag.show(supportFragmentManager, "game_win_dialog")
        }
    }

    fun achievement(size: Int)
    {
        // TODO: Custom toasts are deprecated now. Leave this in as long as it builds, but we're going to need to replace this with a normal, boring toast someday
        runOnUiThread {
            val layout = layoutInflater.inflate(R.layout.achievement_popup, findViewById(R.id.achievement_popup))

            (layout.findViewById<View>(R.id.ball_num) as TextView).text = (1 shl size).toString()

            val achieve_texts = resources.getStringArray(R.array.achieve_texts)
            (layout.findViewById<View>(R.id.achieve_text) as TextView).text =
                    if(size >= achieve_texts.size) achieve_texts[achieve_texts.size - 1] else achieve_texts[size]

            val ball_colors = resources.getIntArray(R.array.ball_colors)
            val ball_color = ball_colors[ballColorIndex(size, ball_colors.size)]

            ImageViewCompat.setImageTintList(layout.findViewById(R.id.ball), ColorStateList.valueOf(ball_color))
            (layout.findViewById<View>(R.id.ball_num) as TextView).setTextColor(calcTextColor(ball_color))

            val toast = Toast(applicationContext)
            toast.duration = Toast.LENGTH_SHORT
            toast.view = layout
            toast.show()
        }
    }

    private fun pause_dialog()
    {
        class Pause_frag: DialogFragment()
        {
            override fun onCreateDialog(savedInstanceState: Bundle?) =
                    AlertDialog.Builder(activity as MainActivity).setTitle(R.string.paused)
                            .setPositiveButton(R.string.cont, null)
                            .create()
        }

        Pause_frag().show(supportFragmentManager, "new_game_dialog")
    }

    private fun new_game_dialog()
    {
        class New_game_frag: DialogFragment()
        {
            override fun onCreateDialog(savedInstanceState: Bundle?): Dialog =
                AlertDialog.Builder(activity as MainActivity)
                    .setTitle(getString(R.string.confirm_new_game))
                    .setPositiveButton(R.string.yes) { _, _ -> (activity as MainActivity).newGame() }
                    .setNegativeButton(R.string.no, null)
                    .create()
        }

        New_game_frag().show(supportFragmentManager, "new_game_dialog")
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean
    {
        super.onCreateOptionsMenu(menu)
        menuInflater.inflate(R.menu.activity_main_action_bar, menu)

        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean
    {
        return when(item.itemId)
        {
            R.id.pause -> { pause_dialog(); true }
            R.id.new_game -> { new_game_dialog(); true }

            R.id.fullscreen ->
            {
                // TODO: replace with API 30 fullscreen methods once there is a compat version for all needed features
                window.decorView.systemUiVisibility = (
                       View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
                       View.SYSTEM_UI_FLAG_FULLSCREEN      or
                       View.SYSTEM_UI_FLAG_IMMERSIVE)

                val actionBar = supportActionBar
                actionBar?.hide()

                true
            }

            R.id.settings ->
            {
                startActivityForResult(Intent(this, Settings::class.java), SETTINGS_RESULT)
                true
            }

            R.id.help ->
            {
                val toast_delay = 4000L // ms
                Toast.makeText(this, resources.getString(R.string.help_general), Toast.LENGTH_LONG).show()
                // TODO: using a delayed handler as a workaround for bug in android: https://issuetracker.google.com/issues/79159357
                // remove when bug is resolved
                Handler(Looper.getMainLooper()).postDelayed({
                    if(gravity_mode)
                        Toast.makeText(this, resources.getString(R.string.help_accel), Toast.LENGTH_LONG).show()
                    else
                        Toast.makeText(this, resources.getString(R.string.help_touch), Toast.LENGTH_LONG).show()
                }, toast_delay)
                true
            }

            R.id.about -> { About_dialog().show(supportFragmentManager, "about_dialog"); false }

            else -> false
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?)
    {
        if(requestCode == SETTINGS_RESULT)
            Handler(Looper.getMainLooper()).postDelayed({recreate()}, 0)

        super.onActivityResult(requestCode, resultCode, data)
    }

    class DispData
    {
        val score = ObservableInt()
        val high_score = ObservableInt()
        val grav_angle = ObservableFloat()
        val pressure = ObservableInt()
    }
}
