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

#include "mozIGeckoMediaPluginService.h"
#include "nsContentCID.h"
#include "nsServiceManagerUtils.h"

#include "gmp-video-host.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"

#include "WebrtcGmpVideoCodec.h"

namespace mozilla {

// Encoder.
WebrtcGmpVideoEncoder::WebrtcGmpVideoEncoder() {
}

int32_t WebrtcGmpVideoEncoder::InitEncode(
    const webrtc::VideoCodec* codecSettings,
    int32_t numberOfCores,
    uint32_t maxPayloadSize) {

  nsCOMPtr<mozIGeckoMediaPluginService> mps =
      do_GetService("@mozilla.org/gecko-media-plugin-service;1");

  if (!mps) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  GMPVideoHost* host = nullptr;
  GMPVideoEncoder* gmp = nullptr;

  nsresult rv = mps->GetGMPVideoEncoderVP8(&host, &gmp);
  if (NS_FAILED(rv))
    return WEBRTC_VIDEO_CODEC_ERROR;

  gmp_ = gmp;
  host_ = host;

  if (!gmp)
    return WEBRTC_VIDEO_CODEC_ERROR;
  if (!host)
    return WEBRTC_VIDEO_CODEC_ERROR;

  // TODO(ekr@rtfm.com): transfer settings from codecSettings to codec.
  GMPVideoCodec codec;
  memset(&codec, 0, sizeof(codec));
  
  codec.mWidth = codecSettings->width;
  codec.mHeight = codecSettings->height;
  codec.mStartBitrate = codecSettings->startBitrate;
  codec.mMaxFramerate = codecSettings->maxFramerate;
    
  GMPVideoErr err = gmp_->InitEncode(codec, this, 1, 1);
  if (err != GMPVideoNoErr) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  return WEBRTC_VIDEO_CODEC_OK;
}


int32_t WebrtcGmpVideoEncoder::Encode(
    const webrtc::I420VideoFrame& inputImage,
    const webrtc::CodecSpecificInfo* codecSpecificInfo,
    const std::vector<webrtc::VideoFrameType>* frame_types) {
  GMPVideoFrame* ftmp = nullptr;

  // Translate the image.
  GMPVideoErr err = host_->CreateFrame(kGMPI420VideoFrame, &ftmp);
  if (err != GMPVideoNoErr) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  GMPVideoi420Frame* frame = static_cast<GMPVideoi420Frame*>(ftmp);

  err = frame->CreateFrame(inputImage.allocated_size(webrtc::kYPlane),
                           inputImage.buffer(webrtc::kYPlane),
                           inputImage.allocated_size(webrtc::kUPlane),
                           inputImage.buffer(webrtc::kUPlane),
                           inputImage.allocated_size(webrtc::kVPlane),
                           inputImage.buffer(webrtc::kVPlane),
                           inputImage.width_, inputImage.height,
                           inputImage.stride(webrtc::kYPlane),
                           inputImage.stride(webrtc::kUPlane),
                           inputImage.stride(webrtc::kVPlane));
  if (err != GMPVideoNoErr) {
    return err;
  }
  frame->SetTimestamp(inputImage.timestamp_);
  frame->SetRenderTime_ms(inputImage.render_time_ms_);

  // Translate the codecSpecificInfo.
  
  err = gmp_->Encode(*frame, 
  
                           

    
  

    
  host_->
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
