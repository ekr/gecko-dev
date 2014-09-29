/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _MEDIAPIPELINEFACTORY_H_
#define _MEDIAPIPELINEFACTORY_H_

#include "MediaConduitInterface.h"
#include "PeerconnectionMedia.h"
#include "TransportFlow.h"

#include "signaling/src/jsep/JsepSession.h"
#include "signaling/src/jsep/JsepTrack.h"


namespace mozilla {

#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"

class MediaPipelineFactory {
 public:
  MediaPipelineFactory(sipcc::PeerConnectionMedia* pcmedia) :
      mPCMedia(pcmedia),
      mPC(pcmedia->GetPC()) {}

  nsresult CreateMediaPipeline(
      const mozilla::UniquePtr<mozilla::jsep::JsepSession>& session,
      const mozilla::jsep::JsepTrackPair& trackPair,
      const mozilla::UniquePtr<mozilla::jsep::JsepTrack>& track);

 private:
  nsresult CreateMediaPipelineReceiving(
      RefPtr<TransportFlow> rtp_flow,
      RefPtr<TransportFlow> rtcp_flow,
      RefPtr<TransportFlow> bundle_rtp_flow,
      RefPtr<TransportFlow> bundle_rtcp_flow,
      const mozilla::UniquePtr<mozilla::jsep::JsepSession>& session,
      const mozilla::jsep::JsepTrackPair& trackPair,
      const mozilla::UniquePtr<mozilla::jsep::JsepTrack>& track,
      const mozilla::RefPtr<mozilla::MediaSessionConduit>& conduit);

  nsresult CreateMediaPipelineSending(
      RefPtr<TransportFlow> rtp_flow,
      RefPtr<TransportFlow> rtcp_flow,
      RefPtr<TransportFlow> bundle_rtp_flow,
      RefPtr<TransportFlow> bundle_rtcp_flow,
      const mozilla::UniquePtr<mozilla::jsep::JsepSession>& session,
      const mozilla::jsep::JsepTrackPair& trackPair,
      const mozilla::UniquePtr<mozilla::jsep::JsepTrack>& track,
      const mozilla::RefPtr<mozilla::MediaSessionConduit>& conduit);

  nsresult CreateAudioConduit(
      const mozilla::UniquePtr<mozilla::jsep::JsepSession>& session,
      const mozilla::jsep::JsepTrackPair& trackPair,
      const mozilla::UniquePtr<mozilla::jsep::JsepTrack>& track,
      mozilla::RefPtr<mozilla::MediaSessionConduit>* conduitp);
  nsresult CreateVideoConduit(
      const mozilla::UniquePtr<mozilla::jsep::JsepSession>& session,
      const mozilla::jsep::JsepTrackPair& trackPair,
      const mozilla::UniquePtr<mozilla::jsep::JsepTrack>& track,
      mozilla::RefPtr<mozilla::MediaSessionConduit>* conduitp);
  static void FinalizeTransportFlow(
      RefPtr<sipcc::PeerConnectionMedia> media,
      RefPtr<TransportFlow> flow,
      size_t level, bool rtcp,
      nsAutoPtr<std::queue<TransportLayer *> > layers);

  int EnsureExternalCodec(
      const mozilla::RefPtr<mozilla::VideoSessionConduit>& conduit,
      mozilla::VideoCodecConfig* config,
      bool send);

  nsresult CreateOrGetTransportFlow(
    size_t level,bool rtcp,
    const mozilla::RefPtr<mozilla::jsep::JsepTransport>& transport,
    mozilla::RefPtr<mozilla::TransportFlow>* out);

 private:
  sipcc::PeerConnectionMedia* mPCMedia;
  sipcc::PeerConnectionImpl* mPC;
};


}  // namespace mozilla

#endif



