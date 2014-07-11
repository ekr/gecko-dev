/*!
 * \copy
 *     Copyright (c)  2009-2014, Cisco Systems
 *     Copyright (c)  2014, Mozilla
 *
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *************************************************************************************
 */

#include <stdint.h>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <memory>
#include <assert.h>
#include <limits.h>

#include "gmp-platform.h"
#include "gmp-video-host.h"
#include "gmp-video-encode.h"
#include "gmp-video-decode.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"


#if defined(_MSC_VER)
#define PUBLIC_FUNC __declspec(dllexport)
#else
#define PUBLIC_FUNC
#endif

static int g_log_level = 0;

#define GMPLOG(l, x) do { \
        if (l <= g_log_level) { \
        const char *log_string = "unknown"; \
        if ((l >= 0) && (l <= 3)) {               \
        log_string = kLogStrings[l];            \
        } \
        std::cerr << log_string << ": " << x << std::endl; \
        } \
    } while(0)

#define GL_CRIT 0
#define GL_ERROR 1
#define GL_INFO  2
#define GL_DEBUG 3

const char* kLogStrings[] = {
  "Critical",
  "Error",
  "Info",
  "Debug"
};

static GMPPlatformAPI* g_platform_api = nullptr;

class FakeVideoEncoder;
class FakeVideoDecoder;

struct EncodedFrame {
  uint32_t width_;
  uint32_t height_;
  uint8_t value_;
  uint32_t timestamp_;
};

template <typename T> class SelfDestruct {
 public:
  SelfDestruct (T* t) : t_ (t) {}
  ~SelfDestruct() {
    if (t_) {
      t_->Destroy();
    }
  }

  T* forget() {
    T* t = t_;
    t_ = nullptr;

    return t;
  }

 private:
  T* t_;
};


class FakeVideoEncoder;

class FakeEncoderTask : public GMPTask {
 public:
  FakeEncoderTask(FakeVideoEncoder* encoder,
                  GMPVideoi420Frame* frame,
                  GMPVideoFrameType type)
      : encoder_(encoder), frame_(frame), type_(type) {}

  virtual void Run();
  virtual void Destroy() {}

  FakeVideoEncoder* encoder_;
  GMPVideoi420Frame* frame_;
  GMPVideoFrameType type_;
};

class FakeVideoEncoder : public GMPVideoEncoder {
 public:
  FakeVideoEncoder (GMPVideoHost* hostAPI) :
    host_ (hostAPI),
    worker_thread_ (nullptr),
    callback_ (nullptr) {}

  virtual ~FakeVideoEncoder() {
    worker_thread_->Join();
  }

  virtual GMPErr InitEncode (const GMPVideoCodec& codecSettings,
                             const uint8_t* aCodecSpecific,
                             uint32_t aCodecSpecificLength,
                             GMPVideoEncoderCallback* callback,
                             int32_t numberOfCores,
                             uint32_t maxPayloadSize) {
    GMPErr err = g_platform_api->createthread (&worker_thread_);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't create new thread");
      return GMPGenericErr;
    }

    GMPLOG (GL_INFO, "Initialized encoder");

    return GMPNoErr;
  }

  virtual GMPErr Encode (GMPVideoi420Frame* inputImage,
                         const uint8_t* codecSpecificInfo,
                         uint32_t aCodecSpecificInfoLength,
                         const GMPVideoFrameType* aFrameTypes,
                         uint32_t aFrameTypesLength) {
    GMPLOG (GL_DEBUG,
            __FUNCTION__
            << " size="
            << inputImage->Width() << "x" << inputImage->Height());

    assert (aFrameTypesLength != 0);
    if (!aFrameTypesLength) {
      GMPLOG (GL_ERROR, "No frame types provided");
      inputImage->Destroy();
      return GMPGenericErr;
    }

    worker_thread_->Post (new FakeEncoderTask(this,
                                              inputImage,
                                              aFrameTypes[0]));

    return GMPGenericErr;
  }

  void Encode_m (GMPVideoi420Frame* inputImage,
                 GMPVideoFrameType frame_type) {
    if (frame_type  == kGMPKeyFrame) {
      if (!inputImage)
        return;
    }
    if (!inputImage) {
      GMPLOG (GL_ERROR, "no input image");
      return;
    }

    // Now return the encoded data back to the parent.
    GMPVideoFrame* ftmp;
    GMPErr err = host_->CreateFrame (kGMPEncodedVideoFrame, &ftmp);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Error creating encoded frame");
      inputImage->Destroy();
      return;
    }

    GMPVideoEncodedFrame* f = static_cast<GMPVideoEncodedFrame*> (ftmp);

    // Copy the data. This really should convert this to network byte order.
    EncodedFrame eframe;
    eframe.width_ = inputImage->Width();
    eframe.height_ = inputImage->Height();
    eframe.value_ = inputImage->Buffer(kGMPYPlane)[0];
    eframe.timestamp_ = inputImage->Timestamp();

    err = f->CreateEmptyFrame (sizeof(eframe));
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Error allocating frame data");
      f->Destroy();
      inputImage->Destroy();
      return;
    }
    memcpy(f->Buffer(), &eframe, sizeof(eframe));

    f->SetEncodedWidth (inputImage->Width());
    f->SetEncodedHeight (inputImage->Height());
    f->SetTimeStamp (inputImage->Timestamp());
    f->SetFrameType (frame_type);
    f->SetCompleteFrame (true);

    GMPLOG (GL_DEBUG, "Encoding complete. type= "
            << f->FrameType()
            << " length="
            << f->Size()
            << " timestamp="
            << f->TimeStamp());

    // Destroy the frame.
    inputImage->Destroy();

    // Return the encoded frame.
    GMPCodecSpecificInfo info;
    memset (&info, 0, sizeof (info));
    info.mCodecType = kGMPVideoCodecH264;
    info.mBufferType = GMP_BufferLength32;
    info.mCodecSpecific.mH264.mSimulcastIdx = 0;
    callback_->Encoded (f, reinterpret_cast<uint8_t*> (&info), sizeof(info));
  }

  // These frames must be destroyed on the main thread.
  void DestroyInputFrame_m (GMPVideoi420Frame* frame) {
    frame->Destroy();
  }

  virtual GMPErr SetChannelParameters (uint32_t aPacketLoss, uint32_t aRTT) {
    return GMPNoErr;
  }

  virtual GMPErr SetRates (uint32_t aNewBitRate, uint32_t aFrameRate) {
    return GMPNoErr;
  }

  virtual GMPErr SetPeriodicKeyFrames (bool aEnable) {
    return GMPNoErr;
  }

  virtual void EncodingComplete() {
    delete this;
  }

 private:
  GMPVideoHost* host_;
  GMPThread* worker_thread_;
  GMPVideoEncoderCallback* callback_;
};

void FakeEncoderTask::Run() {
  encoder_->Encode_m(frame_, type_);
}




class FakeDecoderTask : public GMPTask {
 public:
  FakeDecoderTask(FakeVideoDecoder* decoder,
                  GMPVideoEncodedFrame* frame,
                  int64_t time)
      : decoder_(decoder), frame_(frame), time_(time) {}

  virtual void Run();
  virtual void Destroy() {}

  FakeVideoDecoder* decoder_;
  GMPVideoEncodedFrame* frame_;
  int64_t time_;
};

class FakeVideoDecoder : public GMPVideoDecoder {
 public:
  FakeVideoDecoder (GMPVideoHost* hostAPI) :
    host_ (hostAPI),
    worker_thread_ (nullptr),
    callback_ (nullptr) {}

  virtual ~FakeVideoDecoder() {
  }

  virtual GMPErr InitDecode (const GMPVideoCodec& codecSettings,
                             const uint8_t* aCodecSpecific,
                             uint32_t aCodecSpecificLength,
                             GMPVideoDecoderCallback* callback,
                             int32_t aCoreCount) {
    GMPLOG (GL_INFO, "InitDecode");

    GMPErr err = g_platform_api->createthread (&worker_thread_);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't create new thread");
      return GMPGenericErr;
    }

    callback_ = callback;
    return GMPNoErr;
  }

  virtual GMPErr Decode (GMPVideoEncodedFrame* inputFrame,
                         bool missingFrames,
                         const uint8_t* aCodecSpecificInfo,
                         uint32_t aCodecSpecificInfoLength,
                         int64_t renderTimeMs = -1) {
    GMPLOG (GL_DEBUG, __FUNCTION__
            << "Decoding frame size=" << inputFrame->Size()
            << " timestamp=" << inputFrame->TimeStamp());

    worker_thread_->Post (new FakeDecoderTask(this, inputFrame, renderTimeMs));

    return GMPNoErr;
  }

  // Return the decoded data back to the parent.
  void Decode_m (GMPVideoEncodedFrame* inputFrame,
                 int64_t renderTimeMs) {
    EncodedFrame *eframe;
    if (inputFrame->Size() != sizeof(*eframe)) {
      GMPLOG (GL_ERROR, "Couldn't decode frame");
      return;
    }
    eframe = reinterpret_cast<EncodedFrame*>(inputFrame->Buffer());

    // TODO(ekr@rtfm.com): Set these up.
    int width = eframe->width_;
    int height = eframe->height_;
    int ystride = eframe->width_;
    int uvstride = eframe->width_/2;

    GMPLOG (GL_DEBUG, "Video frame ready for display "
            << width
            << "x"
            << height
            << " timestamp="
            << inputFrame->TimeStamp());

    GMPVideoFrame* ftmp = nullptr;

    // Translate the image.
    GMPErr err = host_->CreateFrame (kGMPI420VideoFrame, &ftmp);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't allocate empty I420 frame");
      return;
    }


    GMPVideoi420Frame* frame = static_cast<GMPVideoi420Frame*> (ftmp);

    // TODO(ekr@rtfm.com): set this up.
    err = frame->CreateEmptyFrame (
        width, height,
        ystride, uvstride, uvstride);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't make decoded frame");
      return;
    }
    memset(frame->Buffer(kGMPYPlane),
           frame->AllocatedSize(kGMPYPlane),
           eframe->value_);
    memset(frame->Buffer(kGMPUPlane),
           frame->AllocatedSize(kGMPUPlane),
           eframe->value_);
    memset(frame->Buffer(kGMPVPlane),
           frame->AllocatedSize(kGMPVPlane),
           eframe->value_);

    GMPLOG (GL_DEBUG, "Allocated size = "
            << frame->AllocatedSize (kGMPYPlane));
    frame->SetTimestamp (inputFrame->TimeStamp());
    frame->SetDuration (inputFrame->Duration());
    callback_->Decoded (frame);
  }

  virtual GMPErr Reset() {
    return GMPNoErr;
  }

  virtual GMPErr Drain() {
    return GMPNoErr;
  }

  virtual void DecodingComplete() {
    delete this;
  }

 private:
  GMPVideoHost* host_;
  GMPThread* worker_thread_;
  GMPVideoDecoderCallback* callback_;
};

void FakeDecoderTask::Run() {
  decoder_->Decode_m(frame_, time_);
}


extern "C" {

  PUBLIC_FUNC GMPErr
  GMPInit (GMPPlatformAPI* aPlatformAPI) {
    g_platform_api = aPlatformAPI;
    return GMPNoErr;
  }

  PUBLIC_FUNC GMPErr
  GMPGetAPI (const char* aApiName, void* aHostAPI, void** aPluginApi) {
    if (!strcmp (aApiName, "decode-video")) {
      *aPluginApi = new FakeVideoDecoder (static_cast<GMPVideoHost*> (aHostAPI));
      return GMPNoErr;
    } else if (!strcmp (aApiName, "encode-video")) {
      *aPluginApi = new FakeVideoEncoder (static_cast<GMPVideoHost*> (aHostAPI));
      return GMPNoErr;
    }
    return GMPGenericErr;
  }

  PUBLIC_FUNC void
  GMPShutdown (void) {
    g_platform_api = nullptr;
  }

} // extern "C"



