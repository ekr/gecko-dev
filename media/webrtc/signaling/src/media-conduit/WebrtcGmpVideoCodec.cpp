/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nspr.h"

#include <iostream>

#include <mozilla/Scoped.h>
#include "VideoConduit.h"
#include "AudioConduit.h"

#include "video_engine/include/vie_external_codec.h"

#include "runnable_utils.h"
#include "WebrtcGmpVideoCodec.h"

namespace mozilla {

// Encoder.
WebrtcGmpVideoEncoder::WebrtcGmpVideoEncoder() {
}

int32_t WebrtcGmpVideoEncoder::InitEncode(
    const webrtc::VideoCodec* codecSettings,
    int32_t numberOfCores,
    uint32_t maxPayloadSize) {
  return WEBRTC_VIDEO_CODEC_OK;
}


int32_t WebrtcGmpVideoEncoder::Encode(
    const webrtc::I420VideoFrame& inputImage,
    const webrtc::CodecSpecificInfo* codecSpecificInfo,
    const std::vector<webrtc::VideoFrameType>* frame_types) {
  return WEBRTC_VIDEO_CODEC_OK;
}


int32_t WebrtcGmpVideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* callback) {
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoEncoder::Release() {
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoEncoder::SetChannelParameters(uint32_t packetLoss,
						     int rtt) {
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoEncoder::SetRates(uint32_t newBitRate,
					 uint32_t frameRate) {
  return WEBRTC_VIDEO_CODEC_OK;
}



// Decoder.
WebrtcGmpVideoDecoder::WebrtcGmpVideoDecoder() {
}

int32_t WebrtcGmpVideoDecoder::InitDecode(
    const webrtc::VideoCodec* codecSettings,
    int32_t numberOfCores) {
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoDecoder::Decode(
    const webrtc::EncodedImage& inputImage,
    bool missingFrames,
    const webrtc::RTPFragmentationHeader* fragmentation,
    const webrtc::CodecSpecificInfo*
    codecSpecificInfo,
    int64_t renderTimeMs) {
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* callback) {
  return WEBRTC_VIDEO_CODEC_OK;
}


int32_t WebrtcGmpVideoDecoder::Release() {
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoDecoder::Reset() {
  return WEBRTC_VIDEO_CODEC_OK;
}

}
