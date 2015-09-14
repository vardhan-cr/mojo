// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/key_util.h"

#include "base/logging.h"
#include "mojo/services/input_events/public/interfaces/input_events.mojom.h"
#include "mojo/services/input_events/public/interfaces/input_key_codes.mojom.h"

// TODO(vtl): Handle more stuff and verify that we're consistent about the
// sequences we generate.
// TODO(vtl): In particular, our implementation of keypad_application_mode is
// incomplete.
std::string GetInputSequenceForKeyPressedEvent(const mojo::Event& key_event,
                                               bool keypad_application_mode) {
  DCHECK_EQ(key_event.action, mojo::EVENT_TYPE_KEY_PRESSED);
  CHECK(key_event.key_data);
  const mojo::KeyData& key_data = *key_event.key_data;

  DVLOG(2) << "Key pressed:"
           << "\n  is_char = " << key_data.is_char
           << "\n  character = " << key_data.character
           << "\n  windows_key_code = " << key_data.windows_key_code
           << "\n  text = " << key_data.text
           << "\n  unmodified_text = " << key_data.unmodified_text;

  // We'll only deal with character events (which we'll get even if |character|
  // isn't set).
  if (!key_data.is_char)
    return std::string();

  // Use |character| if that's set.
  // TODO(vtl): Maybe we should use |text| instead, but it seems to be the same
  // as |character|. (The comments claim that |text| will have something for
  // backspace while |character| won't, but this does not appear to be true
  // currently.)
  if (key_data.character) {
    if (key_data.character >= 128) {
      // TODO(vtl): Need to UTF-8 encode.
      NOTIMPLEMENTED();
      return std::string();
    }
    return std::string(1, static_cast<char>(key_data.character));
  }

  // TODO(vtl): For some of these, we may need to handle modifiers (from
  // |event.flags|).
  switch (key_data.windows_key_code) {
    // Produces input sequence:
    case mojo::KEYBOARD_CODE_BACK:
      // Have backspace send DEL instead of BS.
      return std::string("\x7f");
    case mojo::KEYBOARD_CODE_ESCAPE:
      return std::string("\x1b");
    case mojo::KEYBOARD_CODE_PRIOR:
      return std::string("\x1b[5~");
    case mojo::KEYBOARD_CODE_NEXT:
      return std::string("\x1b[6~");
    case mojo::KEYBOARD_CODE_END:
      return std::string(keypad_application_mode ? "\x1bOF" : "\x1b[F");
    case mojo::KEYBOARD_CODE_HOME:
      return std::string(keypad_application_mode ? "\x1bOH" : "\x1b[H");
    case mojo::KEYBOARD_CODE_LEFT:
      return std::string(keypad_application_mode ? "\x1bOD" : "\x1b[D");
    case mojo::KEYBOARD_CODE_UP:
      return std::string(keypad_application_mode ? "\x1bOA" : "\x1b[A");
    case mojo::KEYBOARD_CODE_RIGHT:
      return std::string(keypad_application_mode ? "\x1bOC" : "\x1b[C");
    case mojo::KEYBOARD_CODE_DOWN:
      return std::string(keypad_application_mode ? "\x1bOB" : "\x1b[B");
    case mojo::KEYBOARD_CODE_INSERT:
      return std::string("\x1b[2~");
    case mojo::KEYBOARD_CODE_DELETE:
      return std::string("\x1b[3~");

    // Should have |character| set:
    case mojo::KEYBOARD_CODE_TAB:
    case mojo::KEYBOARD_CODE_RETURN:
    case mojo::KEYBOARD_CODE_SPACE:
    case mojo::KEYBOARD_CODE_NUM_0:
    case mojo::KEYBOARD_CODE_NUM_1:
    case mojo::KEYBOARD_CODE_NUM_2:
    case mojo::KEYBOARD_CODE_NUM_3:
    case mojo::KEYBOARD_CODE_NUM_4:
    case mojo::KEYBOARD_CODE_NUM_5:
    case mojo::KEYBOARD_CODE_NUM_6:
    case mojo::KEYBOARD_CODE_NUM_7:
    case mojo::KEYBOARD_CODE_NUM_8:
    case mojo::KEYBOARD_CODE_NUM_9:
    case mojo::KEYBOARD_CODE_A:
    case mojo::KEYBOARD_CODE_B:
    case mojo::KEYBOARD_CODE_C:
    case mojo::KEYBOARD_CODE_D:
    case mojo::KEYBOARD_CODE_E:
    case mojo::KEYBOARD_CODE_F:
    case mojo::KEYBOARD_CODE_G:
    case mojo::KEYBOARD_CODE_H:
    case mojo::KEYBOARD_CODE_I:
    case mojo::KEYBOARD_CODE_J:
    case mojo::KEYBOARD_CODE_K:
    case mojo::KEYBOARD_CODE_L:
    case mojo::KEYBOARD_CODE_M:
    case mojo::KEYBOARD_CODE_N:
    case mojo::KEYBOARD_CODE_O:
    case mojo::KEYBOARD_CODE_P:
    case mojo::KEYBOARD_CODE_Q:
    case mojo::KEYBOARD_CODE_R:
    case mojo::KEYBOARD_CODE_S:
    case mojo::KEYBOARD_CODE_T:
    case mojo::KEYBOARD_CODE_U:
    case mojo::KEYBOARD_CODE_V:
    case mojo::KEYBOARD_CODE_W:
    case mojo::KEYBOARD_CODE_X:
    case mojo::KEYBOARD_CODE_Y:
    case mojo::KEYBOARD_CODE_Z:
      // TODO(vtl): Actually, we won't get characters for Ctrl+<number> (and
      // probably other odd combinations).
      DLOG(WARNING) << "Expected character for key code "
                    << key_data.windows_key_code;
      break;

    // Purposely produce no input sequence:
    case mojo::KEYBOARD_CODE_SHIFT:
    case mojo::KEYBOARD_CODE_CONTROL:
    case mojo::KEYBOARD_CODE_MENU:
    case mojo::KEYBOARD_CODE_LSHIFT:
    case mojo::KEYBOARD_CODE_RSHIFT:
    case mojo::KEYBOARD_CODE_LCONTROL:
    case mojo::KEYBOARD_CODE_RCONTROL:
    case mojo::KEYBOARD_CODE_LMENU:
    case mojo::KEYBOARD_CODE_RMENU:
      break;

    // TODO(vtl): Figure these out.
    case mojo::KEYBOARD_CODE_CLEAR:
    case mojo::KEYBOARD_CODE_PAUSE:
    case mojo::KEYBOARD_CODE_CAPITAL:
    case mojo::KEYBOARD_CODE_KANA:  // A.k.a. |KEYBOARD_CODE_HANGUL|.
    case mojo::KEYBOARD_CODE_JUNJA:
    case mojo::KEYBOARD_CODE_FINAL:
    case mojo::KEYBOARD_CODE_HANJA:  // A.k.a. |KEYBOARD_CODE_KANJI|.
    case mojo::KEYBOARD_CODE_CONVERT:
    case mojo::KEYBOARD_CODE_NONCONVERT:
    case mojo::KEYBOARD_CODE_ACCEPT:
    case mojo::KEYBOARD_CODE_MODECHANGE:
    case mojo::KEYBOARD_CODE_SELECT:
    case mojo::KEYBOARD_CODE_PRINT:
    case mojo::KEYBOARD_CODE_EXECUTE:
    case mojo::KEYBOARD_CODE_SNAPSHOT:
    case mojo::KEYBOARD_CODE_HELP:
    case mojo::KEYBOARD_CODE_LWIN:  // A.k.a. |KEYBOARD_CODE_COMMAND|.
    case mojo::KEYBOARD_CODE_RWIN:
    case mojo::KEYBOARD_CODE_APPS:
    case mojo::KEYBOARD_CODE_SLEEP:
    case mojo::KEYBOARD_CODE_NUMPAD0:
    case mojo::KEYBOARD_CODE_NUMPAD1:
    case mojo::KEYBOARD_CODE_NUMPAD2:
    case mojo::KEYBOARD_CODE_NUMPAD3:
    case mojo::KEYBOARD_CODE_NUMPAD4:
    case mojo::KEYBOARD_CODE_NUMPAD5:
    case mojo::KEYBOARD_CODE_NUMPAD6:
    case mojo::KEYBOARD_CODE_NUMPAD7:
    case mojo::KEYBOARD_CODE_NUMPAD8:
    case mojo::KEYBOARD_CODE_NUMPAD9:
    case mojo::KEYBOARD_CODE_MULTIPLY:
    case mojo::KEYBOARD_CODE_ADD:
    case mojo::KEYBOARD_CODE_SEPARATOR:
    case mojo::KEYBOARD_CODE_SUBTRACT:
    case mojo::KEYBOARD_CODE_DECIMAL:
    case mojo::KEYBOARD_CODE_DIVIDE:
    case mojo::KEYBOARD_CODE_F1:
    case mojo::KEYBOARD_CODE_F2:
    case mojo::KEYBOARD_CODE_F3:
    case mojo::KEYBOARD_CODE_F4:
    case mojo::KEYBOARD_CODE_F5:
    case mojo::KEYBOARD_CODE_F6:
    case mojo::KEYBOARD_CODE_F7:
    case mojo::KEYBOARD_CODE_F8:
    case mojo::KEYBOARD_CODE_F9:
    case mojo::KEYBOARD_CODE_F10:
    case mojo::KEYBOARD_CODE_F11:
    case mojo::KEYBOARD_CODE_F12:
    case mojo::KEYBOARD_CODE_F13:
    case mojo::KEYBOARD_CODE_F14:
    case mojo::KEYBOARD_CODE_F15:
    case mojo::KEYBOARD_CODE_F16:
    case mojo::KEYBOARD_CODE_F17:
    case mojo::KEYBOARD_CODE_F18:
    case mojo::KEYBOARD_CODE_F19:
    case mojo::KEYBOARD_CODE_F20:
    case mojo::KEYBOARD_CODE_F21:
    case mojo::KEYBOARD_CODE_F22:
    case mojo::KEYBOARD_CODE_F23:
    case mojo::KEYBOARD_CODE_F24:
    case mojo::KEYBOARD_CODE_NUMLOCK:
    case mojo::KEYBOARD_CODE_SCROLL:
    case mojo::KEYBOARD_CODE_BROWSER_BACK:
    case mojo::KEYBOARD_CODE_BROWSER_FORWARD:
    case mojo::KEYBOARD_CODE_BROWSER_REFRESH:
    case mojo::KEYBOARD_CODE_BROWSER_STOP:
    case mojo::KEYBOARD_CODE_BROWSER_SEARCH:
    case mojo::KEYBOARD_CODE_BROWSER_FAVORITES:
    case mojo::KEYBOARD_CODE_BROWSER_HOME:
    case mojo::KEYBOARD_CODE_VOLUME_MUTE:
    case mojo::KEYBOARD_CODE_VOLUME_DOWN:
    case mojo::KEYBOARD_CODE_VOLUME_UP:
    case mojo::KEYBOARD_CODE_MEDIA_NEXT_TRACK:
    case mojo::KEYBOARD_CODE_MEDIA_PREV_TRACK:
    case mojo::KEYBOARD_CODE_MEDIA_STOP:
    case mojo::KEYBOARD_CODE_MEDIA_PLAY_PAUSE:
    case mojo::KEYBOARD_CODE_MEDIA_LAUNCH_MAIL:
    case mojo::KEYBOARD_CODE_MEDIA_LAUNCH_MEDIA_SELECT:
    case mojo::KEYBOARD_CODE_MEDIA_LAUNCH_APP1:
    case mojo::KEYBOARD_CODE_MEDIA_LAUNCH_APP2:
    case mojo::KEYBOARD_CODE_OEM_1:
    case mojo::KEYBOARD_CODE_OEM_PLUS:
    case mojo::KEYBOARD_CODE_OEM_COMMA:
    case mojo::KEYBOARD_CODE_OEM_MINUS:
    case mojo::KEYBOARD_CODE_OEM_PERIOD:
    case mojo::KEYBOARD_CODE_OEM_2:
    case mojo::KEYBOARD_CODE_OEM_3:
    case mojo::KEYBOARD_CODE_OEM_4:
    case mojo::KEYBOARD_CODE_OEM_5:
    case mojo::KEYBOARD_CODE_OEM_6:
    case mojo::KEYBOARD_CODE_OEM_7:
    case mojo::KEYBOARD_CODE_OEM_8:
    case mojo::KEYBOARD_CODE_OEM_102:
    case mojo::KEYBOARD_CODE_PROCESSKEY:
    case mojo::KEYBOARD_CODE_PACKET:
    case mojo::KEYBOARD_CODE_DBE_SBCSCHAR:
    case mojo::KEYBOARD_CODE_DBE_DBCSCHAR:
    case mojo::KEYBOARD_CODE_ATTN:
    case mojo::KEYBOARD_CODE_CRSEL:
    case mojo::KEYBOARD_CODE_EXSEL:
    case mojo::KEYBOARD_CODE_EREOF:
    case mojo::KEYBOARD_CODE_PLAY:
    case mojo::KEYBOARD_CODE_ZOOM:
    case mojo::KEYBOARD_CODE_NONAME:
    case mojo::KEYBOARD_CODE_PA1:
    case mojo::KEYBOARD_CODE_OEM_CLEAR:
    case mojo::KEYBOARD_CODE_UNKNOWN:
    case mojo::KEYBOARD_CODE_ALTGR:
      NOTIMPLEMENTED() << "Key code " << key_data.windows_key_code;
      break;

    default:
      LOG(WARNING) << "Unknown key code " << key_data.windows_key_code;
      break;
  }
  return std::string();
}
