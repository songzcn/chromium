// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_MOCK_SPEECH_SYNTHESIZER_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_MOCK_SPEECH_SYNTHESIZER_CLIENT_H_

#include <string>

#include "chrome/browser/chromeos/dbus/speech_synthesizer_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class MockSpeechSynthesizerClient : public SpeechSynthesizerClient {
 public:
  MockSpeechSynthesizerClient();
  virtual ~MockSpeechSynthesizerClient();

  MOCK_METHOD1(Speak, void(const std::string&));
  MOCK_METHOD1(SetSpeakProperties, void(const std::string&));
  MOCK_METHOD0(StopSpeaking, void());
  MOCK_METHOD1(IsSpeaking, void(IsSpeakingCallback));
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_MOCK_SPEECH_SYNTHESIZER_CLIENT_H_
