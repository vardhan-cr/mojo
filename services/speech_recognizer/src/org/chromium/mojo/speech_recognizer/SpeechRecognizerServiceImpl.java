// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.speech_recognizer;

import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.util.Log;

import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;

/**
 * Implementation of org.chromium.mojom.mojo.SpeechRecognizerService.
 */
final class SpeechRecognizerServiceImpl implements SpeechRecognizerService {
    private static final String TAG = "SpeechRecognizerServiceImpl";

    private final Context mContext;
    private final Handler mMainHandler;

    private SpeechRecognizer mSpeechRecognizer;

    private AudioManager mAudioManager;

    private SpeechRecognizerService.ListenResponse mCallback;

    SpeechRecognizerServiceImpl(Context context) {
        mContext = context;
        mMainHandler = new Handler(mContext.getMainLooper());

        mCallback = null;

        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                mSpeechRecognizer = SpeechRecognizer.createSpeechRecognizer(mContext);
                mSpeechRecognizer.setRecognitionListener(new SpeechListener());
            }
        });

        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
    }

    /**
     * Returns a list of utterances from the user's speech input.
     *
     * @see
     *org.chromium.mojo.speech_recognizer.SpeechRecognizerService#listen(
     *        SpeechRecognizerService.ListenResponse)
     */
    @Override
    public void listen(SpeechRecognizerService.ListenResponse callback) {
        if (mCallback != null) {
            ResultOrError result_or_error = new ResultOrError();
            result_or_error.setErrorCode(Error.RECOGNIZER_BUSY);
            callback.call(result_or_error);
        }

        mCallback = callback;

        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                // Ignore the result of requestAudioFocus() since a failure only indicates that
                // we'll be listening in a slightly noise environment.
                mAudioManager.requestAudioFocus(null, AudioManager.USE_DEFAULT_STREAM_TYPE,
                        AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE);

                Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
                intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL,
                        RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);

                mSpeechRecognizer.startListening(intent);
            }
        });
    }

    /**
     * @see org.chromium.mojo.speech_recognizer.SpeechRecognizerService#stopListening()
     */
    @Override
    public void stopListening() {
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                mSpeechRecognizer.stopListening();
            }
        });
    }

    /**
     * When connection is closed, we stop receiving location updates.
     *
     * @see org.chromium.mojo.bindings.Interface#close()
     */
    @Override
    public void close() {}

    /**
     * @see org.chromium.mojo.bindings.Interface#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {
        throw e;
    }

    class SpeechListener implements RecognitionListener {
        @Override
        public void onReadyForSpeech(Bundle params) {}
        @Override
        public void onBeginningOfSpeech() {}
        @Override
        public void onRmsChanged(float rmsdB) {}
        @Override
        public void onBufferReceived(byte[] buffer) {}
        @Override
        public void onEndOfSpeech() {}
        @Override
        public void onError(int error) {
            mAudioManager.abandonAudioFocus(null);
            Log.d(TAG, "onError " + error);

            ResultOrError result_or_error = new ResultOrError();
            // The enum in the mojom for SpeechRecognizerService matches the
            // errors that come from Android's RecognizerService.
            result_or_error.setErrorCode(error);
            mCallback.call(result_or_error);

            mCallback = null;
        }
        @Override
        public void onResults(Bundle results) {
            mAudioManager.abandonAudioFocus(null);

            ArrayList<String> utterances =
                    results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
            float[] confidences = results.getFloatArray(SpeechRecognizer.CONFIDENCE_SCORES);

            ArrayList<UtteranceCandidate> candidates = new ArrayList<UtteranceCandidate>();
            for (int i = 0; i < utterances.size(); i++) {
                UtteranceCandidate candidate = new UtteranceCandidate();
                candidate.text = utterances.get(i);
                candidate.confidenceScore = confidences[i];

                candidates.add(candidate);
            }

            ResultOrError result_or_error = new ResultOrError();
            result_or_error.setResults(
                    candidates.toArray(new UtteranceCandidate[candidates.size()]));

            mCallback.call(result_or_error);
            mCallback = null;
        }
        @Override
        public void onPartialResults(Bundle partialResults) {}
        @Override
        public void onEvent(int eventType, Bundle params) {}
    }
}
