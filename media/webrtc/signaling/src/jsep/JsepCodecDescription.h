/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _JSEPCODECDESCRIPTION_H_
#define _JSEPCODECDESCRIPTION_H_

#include <iostream>
#include <string>
#include "signaling/src/sdp/SdpMediaSection.h"

namespace mozilla {
namespace jsep {

#define JSEP_CODEC_CLONE(T) \
  virtual JsepCodecDescription* clone() { return new T(*this); }

// A single entry in our list of known codecs.
struct JsepCodecDescription {
  JsepCodecDescription(mozilla::SdpMediaSection::MediaType type,
                       uint8_t default_pt,
                       const std::string& name,
                       uint32_t clock,
                       uint32_t channels,
                       bool enabled) :
      mType(type),
      mDefaultPt(default_pt),
      mName(name),
      mClock(clock),
      mChannels(channels),
      mEnabled(enabled) {}
  virtual ~JsepCodecDescription() {}

  virtual JsepCodecDescription* clone() = 0;

  mozilla::SdpMediaSection::MediaType mType;
  uint8_t mDefaultPt;
  std::string mName;
  uint32_t mClock;
  uint32_t mChannels;
  bool mEnabled;
};


struct JsepAudioCodecDescription : public JsepCodecDescription {
  JsepAudioCodecDescription(uint8_t default_pt,
                            const std::string& name,
                            uint32_t clock,
                            uint32_t channels,
                            uint32_t packet_size = 0, // TODO(ekr@rtfm.com): Remove when I have reasonable defaults.
                            uint32_t bit_rate = 0,
                            bool enabled = true) :
      JsepCodecDescription(mozilla::SdpMediaSection::kAudio,
                           default_pt, name, clock, channels, enabled),
      mPacketSize(packet_size),
      mBitrate(bit_rate) {}

  JSEP_CODEC_CLONE(JsepAudioCodecDescription)

  uint32_t mPacketSize;
  uint32_t mBitrate;
};


struct JsepVideoCodecDescription : public JsepCodecDescription {
  JsepVideoCodecDescription(uint8_t default_pt,
                            const std::string& name,
                            uint32_t clock,
                            bool enabled = true) :
      JsepCodecDescription(mozilla::SdpMediaSection::kVideo,
                           default_pt, name, clock, 0, enabled) {}

  JSEP_CODEC_CLONE(JsepVideoCodecDescription)

  uint32_t mFbTypes;
  uint32_t mMaxFs;
  uint32_t mMaxFr;  // TODO(ekr@rtfm.com): Update for H.264
};

}  // namespace jsep
}  // namespace mozilla

#endif
