// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// To run: mojo/devtools/common/mojo_run --sky \
//             examples/dart/camera_roll/camera_roll.dart --android
// This example makes use of mojo:camera_roll which is available only when
// running on Android.

import 'dart:async';
import 'dart:sky';

import 'package:mojom/mojo/camera_roll.mojom.dart';
import 'package:sky/mojo/embedder.dart';

final CameraRollServiceProxy cameraRoll = new CameraRollServiceProxy.unbound();
int photoIndex = 0;

Picture draw(Image image) {
  PictureRecorder canvas = new PictureRecorder(view.width, view.height);
  Paint paint = new Paint()..color = const Color.fromARGB(255, 0, 255, 0);
  canvas.scale(view.width / image.width, view.height / image.height);
  canvas.drawImage(image, 0.0, 0.0, paint);
  return canvas.endRecording();
}

void drawNextPhoto() {
  var future = cameraRoll.ptr.getPhoto(photoIndex);
  future.then((response) {
    if (response.photo == null) {
      cameraRoll.ptr.update();
      photoIndex = 0;
      drawNextPhoto();
      return;
    }

    new ImageDecoder(response.photo.content.handle.h, (image) {
      if (image != null) {
        view.picture = draw(image);
        view.scheduleFrame();
      }
    });
  });
}

bool handleEvent(Event event) {
  if (event.type == "pointerdown") {
    return true;
  }

  if (event.type == "pointerup") {
    photoIndex++;
    drawNextPhoto();
    return true;
  }

  return false;
}

void main() {
  embedder.connectToService("mojo:camera_roll", cameraRoll);
  view.setEventCallback(handleEvent);
  drawNextPhoto();
}
