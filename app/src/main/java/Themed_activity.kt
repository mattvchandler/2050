// Copyright 2022 Matthew Chandler

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

import android.os.Bundle
import androidx.preference.PreferenceManager
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate

abstract class Themed_activity: AppCompatActivity()
{
    override fun onCreate(savedInstanceState: Bundle?)
    {
        val night_mode_str = PreferenceManager.getDefaultSharedPreferences(this).getString("theme", resources.getString(R.string.theme_values_default))

        var night_mode = AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM

        when(night_mode_str)
        {
            resources.getString(R.string.theme_values_system) -> night_mode = AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM
            resources.getString(R.string.theme_values_day)    -> night_mode = AppCompatDelegate.MODE_NIGHT_NO
            resources.getString(R.string.theme_values_night)  -> night_mode = AppCompatDelegate.MODE_NIGHT_YES
            resources.getString(R.string.theme_values_auto)   -> night_mode = AppCompatDelegate.MODE_NIGHT_AUTO // I *know* It's deprecated, but this is usually the only option for automatically changing themes on all but the most recent versions of android
            resources.getString(R.string.theme_values_auto_batt)   -> night_mode = AppCompatDelegate.MODE_NIGHT_AUTO_BATTERY
        }
        AppCompatDelegate.setDefaultNightMode(night_mode)

        super.onCreate(savedInstanceState)
    }
}
