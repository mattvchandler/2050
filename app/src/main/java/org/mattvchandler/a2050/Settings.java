package org.mattvchandler.a2050;

import android.content.SharedPreferences;
import android.databinding.DataBindingUtil;
import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.view.MenuItem;

import org.mattvchandler.a2050.databinding.ActivitySettingsBinding;

public class Settings extends AppCompatActivity
{
    public static class Settings_frag extends PreferenceFragment
            implements SharedPreferences.OnSharedPreferenceChangeListener // TODO: Probably won't need
    {
        @Override
        public void onCreate(Bundle savedInstanceState)
        {
            // TODO: check to see if we have an accelerometer, disable if we don't
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.settings);
            PreferenceManager.setDefaultValues(getActivity(), R.xml.settings, false);
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
            if(key.equals("gravity"))
            {
                // TODO
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
