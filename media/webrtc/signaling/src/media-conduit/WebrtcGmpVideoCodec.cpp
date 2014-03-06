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

static int WebrtcFrameTypeToGmpFrameType(webrtc::VideoFrameType in,
                                         GMPVideoFrameType *out) {
  switch(in) {
    case webrtc::kKeyFrame:
      *out = kGMPKeyFrame;
      break;
    case webrtc::kDeltaFrame:
      *out = kGMPDeltaFrame;
      break;
    case webrtc::kGoldenFrame:
      *out = kGMPGoldenFrame;
      break;
    case webrtc::kAltRefFrame:
      *out = kGMPAltRefFrame;
      break;
    case webrtc::kSkipFrame:
      *out = kGMPSkipFrame;
      break;
    default:
      MOZ_CRASH();
      return WEBRTC_VIDEO_CODEC_ERROR;
  }

  return WEBRTC_VIDEO_CODEC_OK;
}

static int GmpFrameTypeToWebrtcFrameType(GMPVideoFrameType in,
                                         webrtc::VideoFrameType *out) {
  switch(in) {
    case kGMPKeyFrame:
      *out = webrtc::kKeyFrame;
      break;
      *out = webrtc::kDeltaFrame;
    case kGMPDeltaFrame:
      break;
    case kGMPGoldenFrame:
      *out = webrtc::kGoldenFrame;
      break;
    case kGMPAltRefFrame:
      *out = webrtc::kAltRefFrame;

      break;
    case kGMPSkipFrame:
      *out = webrtc::kSkipFrame;
      break;
    default:
      MOZ_CRASH();
      return WEBRTC_VIDEO_CODEC_ERROR;
  }

  return WEBRTC_VIDEO_CODEC_OK;
}


int32_t WebrtcGmpVideoEncoder::InitEncode(
    const webrtc::VideoCodec* codecSettings,
    int32_t numberOfCores,
    uint32_t maxPayloadSize) {
  nsresult rv = NS_GetMainThread(&main_thread_);
  if (NS_FAILED(rv))
    return WEBRTC_VIDEO_CODEC_ERROR;

  int32_t ret;
  RUN_ON_THREAD(main_thread_,
                WrapRunnableRet(this,
                                &WebrtcGmpVideoEncoder::InitEncode_m,
                                codecSettings,
                                numberOfCores,
                                maxPayloadSize,
                                &ret),
                NS_DISPATCH_SYNC);

  return ret;
}

int32_t WebrtcGmpVideoEncoder::InitEncode_m(
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
  int32_t ret;
  RUN_ON_THREAD(main_thread_,
                WrapRunnableRet(this,
                                &WebrtcGmpVideoEncoder::Encode_m,
                                &inputImage,
                                codecSpecificInfo,
                                frame_types,
                                &ret),
                NS_DISPATCH_SYNC);

  return ret;
}


 int32_t WebrtcGmpVideoEncoder::Encode_m(const webrtc::I420VideoFrame* inputImage,
                                         const webrtc::CodecSpecificInfo* codecSpecificInfo,
                                         const std::vector<webrtc::VideoFrameType>* frame_types) {
  GMPVideoFrame* ftmp = nullptr;

  // Translate the image.
  GMPVideoErr err = host_->CreateFrame(kGMPI420VideoFrame, &ftmp);
  if (err != GMPVideoNoErr) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  GMPVideoi420Frame* frame = static_cast<GMPVideoi420Frame*>(ftmp);

  err = frame->CreateFrame(inputImage->allocated_size(webrtc::kYPlane),
                           inputImage->buffer(webrtc::kYPlane),
                           inputImage->allocated_size(webrtc::kUPlane),
                           inputImage->buffer(webrtc::kUPlane),
                           inputImage->allocated_size(webrtc::kVPlane),
                           inputImage->buffer(webrtc::kVPlane),
                           inputImage->width(), inputImage->height(),
                           inputImage->stride(webrtc::kYPlane),
                           inputImage->stride(webrtc::kUPlane),
                           inputImage->stride(webrtc::kVPlane));
  if (err != GMPVideoNoErr) {
    return err;
  }
  frame->SetTimestamp(inputImage->timestamp());
  frame->SetRenderTime_ms(inputImage->render_time_ms());

  // TODO(ekr@rtfm.com): actually translate this.
  GMPCodecSpecificInfo info;
  memset(&info, 0, sizeof(info));

  std::vector<GMPVideoFrameType> gmp_frame_types;
  for (auto it = frame_types->begin(); it != frame_types->end(); ++it) {
    GMPVideoFrameType ft;

    int32_t ret = WebrtcFrameTypeToGmpFrameType(*it, &ft);
    if (ret != WEBRTC_VIDEO_CODEC_OK)
      return ret;

    gmp_frame_types.push_back(ft);
  }

  err = gmp_->Encode(*frame, info, &gmp_frame_types);
  if (err != GMPVideoNoErr) {
    return err;
  }

  frame->Destroy();

  return WEBRTC_VIDEO_CODEC_OK;
}



int32_t WebrtcGmpVideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* callback) {
  callback_ = callback;

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

// GMPEncoderCallback virtual functions.
void WebrtcGmpVideoEncoder::Encoded(GMPVideoEncodedFrame& aEncodedFrame,
                                    const GMPCodecSpecificInfo& aCodecSpecificInfo) {
  webrtc::EncodedImage image(aEncodedFrame.Buffer(), aEncodedFrame.AllocatedSize(),
                             aEncodedFrame.Size());

  image._encodedWidth = aEncodedFrame.EncodedWidth();
  image._encodedHeight = aEncodedFrame.EncodedHeight();
  image._timeStamp = aEncodedFrame.TimeStamp();

  webrtc::VideoFrameType ft;
  int32_t ret = GmpFrameTypeToWebrtcFrameType(aEncodedFrame.FrameType(),
                                              &ft);
  if (ret != WEBRTC_VIDEO_CODEC_OK)
    return;
  image._frameType = ft;
  image._completeFrame = true;

  callback_->Encoded(image, nullptr, nullptr);
}


// Decoder.
WebrtcGmpVideoDecoder::WebrtcGmpVideoDecoder() {
}

int32_t WebrtcGmpVideoDecoder::InitDecode(
    const webrtc::VideoCodec* codecSettings,
    int32_t numberOfCores) {
  nsresult rv = NS_GetMainThread(&main_thread_);
  if (NS_FAILED(rv))
    return WEBRTC_VIDEO_CODEC_ERROR;

  int32_t ret;
  RUN_ON_THREAD(main_thread_,
                WrapRunnableRet(this,
                                &WebrtcGmpVideoDecoder::InitDecode_m,
                                codecSettings,
                                numberOfCores,
                                &ret),
                NS_DISPATCH_SYNC);

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoDecoder::InitDecode_m(
    const webrtc::VideoCodec* codecSettings,
    int32_t numberOfCores) {
  std::cerr << __FUNCTION__ << std::endl;
  nsCOMPtr<mozIGeckoMediaPluginService> mps =
      do_GetService("@mozilla.org/gecko-media-plugin-service;1");

  if (!mps) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  GMPVideoHost* host = nullptr;
  GMPVideoDecoder* gmp = nullptr;

  nsresult rv = mps->GetGMPVideoDecoderVP8(&host, &gmp);
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

  GMPVideoErr err = gmp_->InitDecode(codec, this, 1);
  if (err != GMPVideoNoErr) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  std::cerr << "OK: " << __FUNCTION__ << std::endl;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoDecoder::Decode(
    const webrtc::EncodedImage& inputImage,
    bool missingFrames,
    const webrtc::RTPFragmentationHeader* fragmentation,
    const webrtc::CodecSpecificInfo*
    codecSpecificInfo,
    int64_t renderTimeMs) {
  int32_t ret;

  RUN_ON_THREAD(main_thread_,
                WrapRunnableRet(this,
                                &WebrtcGmpVideoDecoder::Decode_m,
                                inputImage,
                                missingFrames,
                                fragmentation,
                                codecSpecificInfo,
                                renderTimeMs,
                                &ret),
                NS_DISPATCH_SYNC);

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoDecoder::Decode_m(
    const webrtc::EncodedImage& inputImage,
    bool missingFrames,
    const webrtc::RTPFragmentationHeader* fragmentation,
    const webrtc::CodecSpecificInfo*
    codecSpecificInfo,
    int64_t renderTimeMs) {
  GMPVideoEncodedFrame* frame = nullptr;

  GMPVideoErr err = host_->CreateEncodedFrame(&frame);
  if (err != GMPVideoNoErr) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  err = frame->CreateEmptyFrame(inputImage._size);
  if (err != GMPVideoNoErr) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  memcpy(frame->Buffer(), inputImage._buffer, frame->Size());

  frame->SetEncodedWidth(inputImage._encodedWidth);
  frame->SetEncodedHeight(inputImage._encodedHeight);
  frame->SetTimeStamp(inputImage._timeStamp);
  frame->SetCompleteFrame(inputImage._completeFrame);

  GMPVideoFrameType ft;
  int32_t ret = WebrtcFrameTypeToGmpFrameType(inputImage._frameType, &ft);
  if (ret != WEBRTC_VIDEO_CODEC_OK)
    return ret;

  // TODO(ekr@rtfm.com): Fill in
  GMPCodecSpecificInfo info;
  memset(&info, 0, sizeof(info));

  err = gmp_->Decode(*frame, missingFrames, info, renderTimeMs);
  if (err != GMPVideoNoErr)
    return WEBRTC_VIDEO_CODEC_ERROR;

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* callback) {
  callback_ = callback;

  return WEBRTC_VIDEO_CODEC_OK;
}


int32_t WebrtcGmpVideoDecoder::Release() {
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGmpVideoDecoder::Reset() {
  return WEBRTC_VIDEO_CODEC_OK;
}

}
