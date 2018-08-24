// Copyright 2018 Matthew Chandler

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

import android.content.Context
import android.content.SharedPreferences
import android.databinding.DataBindingUtil
import android.hardware.Sensor
import android.hardware.SensorManager
import android.os.Bundle
import android.preference.PreferenceManager
import android.support.v7.preference.PreferenceFragmentCompat
import android.view.MenuItem
import org.mattvchandler.a2050.databinding.ActivitySettingsBinding
import java.util.*

class Settings: Themed_activity()
{
    class Settings_frag: PreferenceFragmentCompat(), SharedPreferences.OnSharedPreferenceChangeListener
    {
        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?)
        {
            setPreferencesFromResource(R.xml.settings, rootKey)
            PreferenceManager.setDefaultValues(activity, R.xml.settings, false)

            if(!has_accelerometer)
            {
                val grav_pref = preferenceScreen.findPreference("gravity")
                grav_pref.isEnabled = false
                grav_pref.setSummary(R.string.no_accel)
            }
        }

        // register / unregister listener
        override fun onResume()
        {
            super.onResume()
            preferenceScreen.sharedPreferences.registerOnSharedPreferenceChangeListener(this)

        }
        override fun onPause()
        {
            super.onPause()
            preferenceScreen.sharedPreferences.unregisterOnSharedPreferenceChangeListener(this)

        }

        override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences, key: String)
        {
            if(key == "theme")
            {
                // recreate this activity to apply the new theme
                activity?.recreate()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?)
    {
        super.onCreate(savedInstanceState)

        val binding = DataBindingUtil.setContentView<ActivitySettingsBinding>(this, R.layout.activity_settings)
        setSupportActionBar(binding.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)

        // put settings content into frame layout
        supportFragmentManager.beginTransaction().replace(R.id.preferences, Settings_frag()).commit()

        val sensorManager = getSystemService(Context.SENSOR_SERVICE) as SensorManager
        if(Objects.requireNonNull(sensorManager).getDefaultSensor(Sensor.TYPE_ACCELEROMETER) == null)
        {
            has_accelerometer = false
            PreferenceManager.getDefaultSharedPreferences(this).edit().putBoolean("gravity", false).apply()
        }
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean
    {
        // make home button go back
        when(item.itemId)
        {
            android.R.id.home -> { finish(); return true }
        }
        return false
    }

    companion object
    {
        private var has_accelerometer = true
    }
}
