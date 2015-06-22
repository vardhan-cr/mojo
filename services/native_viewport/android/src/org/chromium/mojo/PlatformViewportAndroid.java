// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo;

import android.app.Activity;
import android.content.Context;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.mojo.keyboard.KeyboardServiceImpl;

/**
 * Exposes SurfaceView to native code.
 */
@JNINamespace("native_viewport")
public class PlatformViewportAndroid extends SurfaceView {

    private long mNativeMojoViewport;
    private final SurfaceHolder.Callback mSurfaceCallback;

    @CalledByNative
    public static PlatformViewportAndroid createForActivity(
            Activity activity, long nativeViewport) {
        PlatformViewportAndroid rv = new PlatformViewportAndroid(activity, nativeViewport);
        KeyboardServiceImpl.setActiveView(rv);
        activity.setContentView(rv);
        return rv;
    }

    public PlatformViewportAndroid(Context context, long nativeViewport) {
        super(context);

        setFocusable(true);
        setFocusableInTouchMode(true);

        mNativeMojoViewport = nativeViewport;
        assert mNativeMojoViewport != 0;

        final float density = context.getResources().getDisplayMetrics().density;

        mSurfaceCallback = new SurfaceHolder.Callback() {
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                assert mNativeMojoViewport != 0;
                nativeSurfaceSetSize(mNativeMojoViewport, width, height, density);
            }

            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                assert mNativeMojoViewport != 0;
                nativeSurfaceCreated(mNativeMojoViewport, holder.getSurface());
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                assert mNativeMojoViewport != 0;
                nativeSurfaceDestroyed(mNativeMojoViewport);
            }
        };
        getHolder().addCallback(mSurfaceCallback);

    }

    @CalledByNative
    public void detach() {
        getHolder().removeCallback(mSurfaceCallback);
        mNativeMojoViewport = 0;
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        if (visibility == View.VISIBLE) {
            requestFocusFromTouch();
            requestFocus();
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int actionMasked = event.getActionMasked();
        if (actionMasked == MotionEvent.ACTION_POINTER_DOWN
                || actionMasked == MotionEvent.ACTION_POINTER_UP) {
            // Up/down events identify a single point.
            return notifyTouchEventAtIndex(event, event.getActionIndex());
        }
        assert event.getPointerCount() != 0;
        // All other types can have more than one point.
        boolean result = false;
        for (int i = 0, count = event.getPointerCount(); i < count; i++) {
            final boolean sub_result = notifyTouchEventAtIndex(event, i);
            result |= sub_result;
        }
        return result;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return KeyboardServiceImpl.createInputConnection(outAttrs);
    }

    private boolean notifyTouchEventAtIndex(MotionEvent event, int index) {
        return nativeTouchEvent(mNativeMojoViewport, event.getEventTime(), event.getActionMasked(),
                event.getPointerId(index), event.getX(index), event.getY(index),
                event.getPressure(index), event.getTouchMajor(index), event.getTouchMinor(index),
                event.getOrientation(index), event.getAxisValue(MotionEvent.AXIS_HSCROLL, index),
                event.getAxisValue(MotionEvent.AXIS_VSCROLL, index));
    }

    private static native void nativeDestroy(long nativePlatformViewportAndroid);

    private static native void nativeSurfaceCreated(
            long nativePlatformViewportAndroid, Surface surface);

    private static native void nativeSurfaceDestroyed(
            long nativePlatformViewportAndroid);

    private static native void nativeSurfaceSetSize(
            long nativePlatformViewportAndroid, int width, int height, float density);

    private static native boolean nativeTouchEvent(long nativePlatformViewportAndroid, long timeMs,
            int maskedAction, int pointerId, float x, float y, float pressure, float touchMajor,
            float touchMinor, float orientation, float hWheel, float vWheel);
}
