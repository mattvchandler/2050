package org.mattvchandler.a2050;

import android.content.res.Resources;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.app.AppCompatDelegate;

public abstract class Themed_activity extends AppCompatActivity
{
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        Resources resources = getResources();
        String night_mode_str = PreferenceManager.getDefaultSharedPreferences(this).getString("theme", resources.getString(R.string.theme_values_default));

        int night_mode = AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM;

        if(night_mode_str.equals(resources.getString(R.string.theme_values_system)))
            night_mode = AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM;
        else if(night_mode_str.equals(resources.getString(R.string.theme_values_day)))
            night_mode = AppCompatDelegate.MODE_NIGHT_NO;
        else if(night_mode_str.equals(resources.getString(R.string.theme_values_night)))
            night_mode = AppCompatDelegate.MODE_NIGHT_YES;
        else if(night_mode_str.equals(resources.getString(R.string.theme_values_auto)))
            night_mode = AppCompatDelegate.MODE_NIGHT_AUTO;

        AppCompatDelegate.setDefaultNightMode(night_mode);

        super.onCreate(savedInstanceState);
    }
}
