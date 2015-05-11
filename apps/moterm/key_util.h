// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_MOTERM_KEY_UTIL_H_
#define APPS_MOTERM_KEY_UTIL_H_

#include <string>

namespace mojo {
class Event;
}

// Gets an appropriate sequence of characters to generate for the given key
// pressed event (|key_event| must have |action| value
// |EVENT_TYPE_KEY_PRESSED|). (Here, "appropriate" means VT100/xterm-ish.)
std::string GetInputSequenceForKeyPressedEvent(const mojo::Event& key_event);

#endif  // APPS_MOTERM_KEY_UTIL_H_
