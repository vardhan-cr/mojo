// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shortcuts;

import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.JsonReader;
import android.util.JsonWriter;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.StringWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

/**
 * Main activity for the shortcuts application. It downloads a configuration file and allows the
 * user to select which mojo application to launch.
 */
public class ShortcutsActivity extends ListActivity {
    /**
     * ID for the run menu item.
     */
    private static final int MENU_RUN = 0;

    /**
     * ID for the create shortcut menu item.
     */
    private static final int MENU_CREATE_SHORTCUT = 1;

    /**
     * Adapter to fill the ListView.
     */
    private ArrayAdapter<Shortcut> mAdapter = null;

    private static class Shortcut {
        private final String mName;
        private final String mEncodedCommandLine;

        public Shortcut(String name, List<String> commandLine) throws IOException {
            mName = name;
            StringWriter sw = new StringWriter();
            JsonWriter json = new JsonWriter(sw);
            json.beginArray();
            for (String p : commandLine) {
                json.value(p);
            }
            json.endArray();
            json.close();
            mEncodedCommandLine = sw.toString();
        }

        /**
         * @see java.lang.Object#toString()
         */
        @Override
        public String toString() {
            return mName;
        }

        /**
         * Returns the intent corresponding to the current shortcut.
         */
        private Intent getIntent() {
            Intent intent = new Intent();
            intent.setComponent(new ComponentName(
                    "org.chromium.mojo.shell", "org.chromium.mojo.shell.MojoShellActivity"));
            intent.putExtra("encodedParameters", mEncodedCommandLine);
            return intent;
        }
    }

    private class DownloadConfigurationTask
            extends AsyncTask<Void, Void, List<Shortcut>> {
        /**
         * @see android.os.AsyncTask#doInBackground(java.lang.Object[])
         */
        @Override
        protected List<Shortcut> doInBackground(Void... arg) {
            try {
                ArrayList<Shortcut> items = new ArrayList<Shortcut>();
                URL url = new URL("http://domokit.github.io/shortcuts.json");
                HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
                urlConnection.setRequestMethod("GET");
                urlConnection.connect();
                // Gets the data, and parses it as a json array.
                JsonReader reader = new JsonReader(
                        new InputStreamReader(urlConnection.getInputStream()));
                reader.beginArray();
                while (reader.hasNext()) {
                    reader.beginObject();
                    String name = null;
                    List<String> commandLine = new ArrayList<String>();
                    while (reader.hasNext()) {
                        String tag = reader.nextName();

                        if (tag.equals("name")) {
                            name = reader.nextString();
                        } else if (tag.equals("args")) {
                            reader.beginArray();
                            while (reader.hasNext()) {
                                commandLine.add(reader.nextString());
                            }
                            reader.endArray();
                        } else {
                            reader.skipValue();
                        }
                    }
                    reader.endObject();
                    if (name != null) {
                        items.add(new Shortcut(name, commandLine));
                    }
                }
                reader.endArray();
                reader.close();
                return items;
            } catch (Exception e) {
                Log.e("chromium", e.getMessage(), e);
                return null;
            }
        }

        /**
         * @see android.os.AsyncTask#onPostExecute(java.lang.Object)
         */
        @Override
        protected void onPostExecute(List<Shortcut> result) {
            mAdapter = new ArrayAdapter<Shortcut>(
                    ShortcutsActivity.this, android.R.layout.simple_list_item_1, result);
            getListView().setAdapter(mAdapter);
            getListView().setOnItemClickListener(new OnItemClickListener() {

                    @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    Shortcut shortcut = mAdapter.getItem(position);
                    startActivity(shortcut.getIntent());
                }
            });
        }
    }

    /**
     * @see android.app.Activity#onCreate(android.os.Bundle)
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        registerForContextMenu(getListView());
        new DownloadConfigurationTask().execute();
    }

    /**
     * @see android.app.Activity#onCreateContextMenu
     */
    @Override
    public void onCreateContextMenu(
            ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
        if (v == getListView()) {
            menu.add(Menu.NONE, MENU_RUN, Menu.NONE, "Run");
            menu.add(Menu.NONE, MENU_CREATE_SHORTCUT, Menu.NONE, "Create shortcut");
        }
    }

    /**
     * @see android.app.Activity#onContextItemSelected
     */
    @Override
    public boolean onContextItemSelected(MenuItem item) {
        AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();
        final Shortcut shortcut = mAdapter.getItem(info.position);
        switch (item.getItemId()) {
            case MENU_RUN:
                startActivity(shortcut.getIntent());
                return true;
            case MENU_CREATE_SHORTCUT:
                Intent intent = new Intent();
                intent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcut.getIntent());
                intent.putExtra(Intent.EXTRA_SHORTCUT_NAME, shortcut.toString());
                intent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
                sendBroadcast(intent);
                return true;
            default:
                return super.onContextItemSelected(item);
        }
    }
}
