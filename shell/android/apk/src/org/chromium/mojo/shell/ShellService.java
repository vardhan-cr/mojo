// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.AssetManager;
import android.os.Binder;
import android.os.IBinder;
import android.util.JsonReader;
import android.util.Log;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.Pair;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.mojo.ServiceProvider;
import org.chromium.mojom.mojo.Shell;

import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A placeholder class to call native functions.
 **/
@JNINamespace("shell")
public class ShellService extends Service {
    private static final String TAG = "ShellService";

    // Directory where applications bundled with the shell will be extracted.
    private static final String LOCAL_APP_DIRECTORY = "local_apps";
    // Individual applications bundled with the shell as assets.
    private static final String NETWORK_LIBRARY_APP = "network_service.mojo";
    // Directory where the child executable will be extracted.
    private static final String CHILD_DIRECTORY = "child";
    // Directory to set TMPDIR to.
    private static final String TMP_DIRECTORY = "tmp";
    // Directory to set HOME to.
    private static final String HOME_DIRECTORY = "home";
    // Name of the child executable.
    private static final String MOJO_SHELL_CHILD_EXECUTABLE = "mojo_shell_child";
    // Path to the default origin of mojo: apps.
    private static final String DEFAULT_ORIGIN = "https://core.mojoapps.io/";
    // Name of the default window manager.
    private static final String DEFAULT_WM = "mojo:kiosk_wm";
    // Binder to this service.
    private final ShellBinder mBinder = new ShellBinder();
    // A guard flag for calling nativeInit() only once.
    private boolean mInitialized = false;
    // A static reference to the service.
    private static ShellService sShellService;

    private static class ShellImpl implements Shell {
        private static final ShellImpl INSTANCE = new ShellImpl();

        /**
         * @see Shell#close()
         */
        @Override
        public void close() {}

        /**
         * @see Shell#onConnectionError(org.chromium.mojo.system.MojoException)
         */
        @Override
        public void onConnectionError(MojoException e) {}

        /**
         * @see Shell#connectToApplication(String, InterfaceRequest, ServiceProvider)
         */
        @Override
        public void connectToApplication(String applicationUrl,
                InterfaceRequest<ServiceProvider> services, ServiceProvider exposedServices) {
            int exposedServicesHandle = CoreImpl.INVALID_HANDLE;
            if (exposedServices != null) {
                if (exposedServices instanceof ServiceProvider.Proxy) {
                    ServiceProvider.Proxy proxy = (ServiceProvider.Proxy) exposedServices;
                    exposedServicesHandle =
                            proxy.getProxyHandler().passHandle().releaseNativeHandle();
                } else {
                    Pair<MessagePipeHandle, MessagePipeHandle> pipes =
                            CoreImpl.getInstance().createMessagePipe(null);
                    ServiceProvider.MANAGER.bind(exposedServices, pipes.first);
                    exposedServicesHandle = pipes.second.releaseNativeHandle();
                }
            }
            nativeConnectToApplication(applicationUrl,
                    services == null ? CoreImpl.INVALID_HANDLE
                                     : services.passHandle().releaseNativeHandle(),
                    exposedServicesHandle);
        }
    }

    /**
     * Binder for the Shell service. This object is passed to the calling activities.
     **/
    private class ShellBinder extends Binder {
        ShellService getService() {
            return ShellService.this;
        }
    }

    /**
     * Interface implemented by activities wanting to bind with the ShellService service.
     **/
    static interface IShellBindingActivity {
        // Called when the ShellService service is connected.
        void onShellBound(ShellService shellService);

        // Called when the ShellService service is disconnected.
        void onShellUnbound();
    }

    /**
     * ServiceConnection for the ShellService service.
     **/
    static class ShellServiceConnection implements ServiceConnection {
        private final IShellBindingActivity mActivity;

        ShellServiceConnection(IShellBindingActivity activity) {
            this.mActivity = activity;
        }

        @Override
        public void onServiceConnected(ComponentName className, IBinder binder) {
            ShellService.ShellBinder shellBinder = (ShellService.ShellBinder) binder;
            this.mActivity.onShellBound(shellBinder.getService());
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            this.mActivity.onShellUnbound();
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        sShellService = this;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        sShellService = null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // A client is starting this service; make sure the shell is initialized.
        // Note that ensureInitialized is gated by the mInitialized boolean flag. This means that
        // only the first set of parameters will ever be taken into account.
        // TODO(eseidel): ShellService can fail, but we're ignoring the return.
        ensureStarted(getApplicationContext(), getParametersFromIntent(intent));
        return Service.START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        if (!mInitialized) {
            throw new IllegalStateException("Start the service first");
        }
        return mBinder;
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        if (!rootIntent.getComponent().getClassName().equals(ViewportActivity.class.getName())) {
            return;
        }
        String viewportId = rootIntent.getStringExtra("ViewportId");
        NativeViewportSupportApplicationDelegate.viewportClosed(viewportId);
    }

    /**
     * Initializes the native system and starts the shell.
     **/
    private void ensureStarted(Context applicationContext, String[] parameters) {
        if (mInitialized) return;
        try {
            FileHelper.extractFromAssets(applicationContext, NETWORK_LIBRARY_APP,
                    getLocalAppsDir(applicationContext), false);
            FileHelper.extractFromAssets(applicationContext, MOJO_SHELL_CHILD_EXECUTABLE,
                    getChildDir(applicationContext), false);
            File mojoShellChild =
                    new File(getChildDir(applicationContext), MOJO_SHELL_CHILD_EXECUTABLE);
            // The shell child executable needs to be ... executable.
            mojoShellChild.setExecutable(true, true);

            List<String> parametersList = new ArrayList<String>();

            parametersList.add("--args-for=mojo:notifications " + R.mipmap.ic_launcher);

            // Program name.
            if (parameters != null) {
                parametersList.addAll(Arrays.asList(parameters));
            } else {
                // Apply default parameters.
                parametersList.add("--origin=" + DEFAULT_ORIGIN);
                parametersList.add("--url-mappings=mojo:window_manager=" + DEFAULT_WM);
            }

            nativeStart(applicationContext, applicationContext.getAssets(),
                    mojoShellChild.getAbsolutePath(),
                    parametersList.toArray(new String[parametersList.size()]),
                    getLocalAppsDir(applicationContext).getAbsolutePath(),
                    getTmpDir(applicationContext).getAbsolutePath(),
                    getHomeDir(applicationContext).getAbsolutePath());
            mInitialized = true;
        } catch (Exception e) {
            Log.e(TAG, "ShellService initialization failed.", e);
            throw new RuntimeException(e);
        }
    }

    private static String[] getParametersFromIntent(Intent intent) {
        if (intent == null) {
            return null;
        }
        String[] parameters = intent.getStringArrayExtra("parameters");
        if (parameters != null) {
            return parameters;
        }
        String encodedParameters = intent.getStringExtra("encodedParameters");
        if (encodedParameters != null) {
            JsonReader reader = new JsonReader(new StringReader(encodedParameters));
            List<String> parametersList = new ArrayList<String>();
            try {
                reader.beginArray();
                while (reader.hasNext()) {
                    parametersList.add(reader.nextString());
                }
                reader.endArray();
                reader.close();
                return parametersList.toArray(new String[parametersList.size()]);
            } catch (IOException e) {
                Log.w(TAG, e.getMessage(), e);
            }
        }
        return null;
    }

    /**
     * Adds the given URL to the set of mojo applications to run on start. This must be called
     * before {@link ShellService#ensureStarted(Context, String[])}
     */
    void addApplicationURL(String url) {
        nativeAddApplicationURL(url);
    }

    /**
     * Starts this application in an already-initialized shell.
     */
    void startApplicationURL(String url) {
        nativeStartApplicationURL(url);
    }

    /**
     * Returns an instance of the shell interface that allows to interact with mojo applications.
     */
    Shell getShell() {
        return ShellImpl.INSTANCE;
    }

    private static File getLocalAppsDir(Context context) {
        return context.getDir(LOCAL_APP_DIRECTORY, Context.MODE_PRIVATE);
    }

    private static File getChildDir(Context context) {
        return context.getDir(CHILD_DIRECTORY, Context.MODE_PRIVATE);
    }

    private static File getTmpDir(Context context) {
        return new File(context.getCacheDir(), TMP_DIRECTORY);
    }

    private static File getHomeDir(Context context) {
        return context.getDir(HOME_DIRECTORY, Context.MODE_PRIVATE);
    }

    @CalledByNative
    private static void finishActivities() {
        for (WeakReference<Activity> activityRef : ApplicationStatus.getRunningActivities()) {
            Activity activity = activityRef.get();
            if (activity != null) {
                activity.finishAndRemoveTask();
            }
        }
        if (sShellService != null) {
            sShellService.stopSelf();
        }
    }

    /**
     * Initializes the native system. This API should be called only once per process.
     **/
    private static native void nativeStart(Context context, AssetManager assetManager,
            String mojoShellChildPath, String[] parameters, String bundledAppsDirectory,
            String tmpDir, String homeDir);

    private static native void nativeAddApplicationURL(String url);

    private static native void nativeStartApplicationURL(String url);

    private static native void nativeConnectToApplication(
            String applicationURL, int servicesHandle, int exposedServicesHandle);
}
