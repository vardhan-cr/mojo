// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.common;

import android.os.Handler;
import android.util.Log;

import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.DataPipe;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.MojoResult;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.concurrent.Executor;


/**
 * Helper class for copyToFile.
 */
class CopyToFileJob implements Runnable {
    private static final String TAG = "CopyToFileJob";

    private final DataPipe.ConsumerHandle mSource;
    private final File mDest;
    private final Runnable mComplete;
    private final Handler mCaller;
    private final Core mCore;

    public CopyToFileJob(Core core, DataPipe.ConsumerHandle handle,
            File file, Runnable complete, Handler caller) {
        mCore = core;
        mSource = handle;
        mDest = file;
        mComplete = complete;
        mCaller = caller;
    }

    private void readLoop(FileChannel dest) {
        do {
            try {
                ByteBuffer buffer = mSource.beginReadData(0,
                        DataPipe.ReadFlags.NONE);
                if (buffer.capacity() == 0)
                    break;
                dest.write(buffer);
                mSource.endReadData(buffer.capacity());
            } catch (MojoException e) {
                // No one read the pipe, they just closed it.
                if (e.getMojoResult() == MojoResult.FAILED_PRECONDITION) {
                    break;
                } else if (e.getMojoResult() == MojoResult.SHOULD_WAIT) {
                    mCore.wait(mSource, Core.HandleSignals.READABLE, -1);
                } else {
                    throw e;
                }
            } catch (IOException e) {
                Log.e(TAG, "mDest.write failed", e);
                break;
            }
        } while (true);
    }

    @Override
    public void run() {
        FileChannel dest = null;
        try {
            dest = new FileOutputStream(mDest, false).getChannel();
            readLoop(dest);
        } catch (java.io.FileNotFoundException e) {
            Log.e(TAG, "destination file does not exist ", e);
        }
        mSource.close();
        try {
            dest.close();
        } catch (IOException e) {
            Log.e(TAG, "failed to close file ", e);
        }

        mCaller.post(mComplete);
    }
}


/**
 * Java helpers for dealing with DataPipes.
 */
public class DataPipeUtils {
    public static void copyToFile(Core core, DataPipe.ConsumerHandle source,
              File dest, Executor executor, Runnable complete) {
        // The default constructor for Handler uses the Looper for this thread.
        executor.execute(
                new CopyToFileJob(core, source, dest, complete, new Handler()));
    }
}
