package org.mattvchandler.a2050;

import android.content.Context;
import android.content.SharedPreferences;
import android.databinding.DataBindingUtil;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.view.MenuItem;

import org.mattvchandler.a2050.databinding.ActivitySettingsBinding;

import java.util.Objects;

public class Settings extends Themed_activity
{
    public static boolean has_accelerometer = true;

    public static class Settings_frag extends PreferenceFragment
            implements SharedPreferences.OnSharedPreferenceChangeListener
    {
        @Override
        public void onCreate(Bundle savedInstanceState)
        {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.settings);
            PreferenceManager.setDefaultValues(getActivity(), R.xml.settings, false);

            if(!has_accelerometer)
            {
                Preference grav_pref = getPreferenceScreen().findPreference("gravity");
                grav_pref.setEnabled(false);
                grav_pref.setSummary(R.string.no_accel);
            }
        }

        // register / unregister listener
        @Override
        public void onResume()
        {
            super.onResume();
            getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);

        }
        @Override
        public void onPause()
        {
            super.onPause();
            getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);

        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
        {
            if(key.equals("theme"))
            {
                // recreate this activity to apply the new theme
                getActivity().recreate();
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        ActivitySettingsBinding binding = DataBindingUtil.setContentView(this, R.layout.activity_settings);
        setSupportActionBar(binding.toolbar);
        if(getSupportActionBar() != null)
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        // put settings content into frame layout
        getFragmentManager().beginTransaction().replace(R.id.preferences, new Settings_frag()).commit();

        SensorManager sensorManager = (SensorManager)getSystemService(Context.SENSOR_SERVICE);
        if(Objects.requireNonNull(sensorManager).getDefaultSensor(Sensor.TYPE_ACCELEROMETER) == null)
        {
            has_accelerometer = false;
            PreferenceManager.getDefaultSharedPreferences(this).edit().putBoolean("gravity", false).apply();
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        // make home button go back
        switch(item.getItemId())
        {
        case android.R.id.home:
            finish();
            return true;
        }
        return false;
    }
}
