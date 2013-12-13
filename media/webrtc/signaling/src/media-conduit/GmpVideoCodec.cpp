/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcGmpVideoCodec.h"
#include "GmpVideoCodec.h"

namespace mozilla {

VideoEncoder* GmpVideoCodec::CreateEncoder() {
  WebrtcGmpVideoEncoder *enc =
      new WebrtcGmpVideoEncoder();
  return enc;
}

VideoDecoder* GmpVideoCodec::CreateDecoder() {
  WebrtcGmpVideoDecoder *dec =
      new WebrtcGmpVideoDecoder();
  return dec;
}

}
