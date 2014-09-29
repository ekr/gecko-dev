/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _JSEPTRANSPORT_H_
#define _JSEPTRANSPORT_H_

#include <mozilla/RefPtr.h>

#include "signaling/src/sdp/SdpAttribute.h"

namespace mozilla {
namespace jsep {

class JsepDtlsTransport {
 public:
  virtual ~JsepDtlsTransport() {}

  enum Role {
    kJsepDtlsClient,
    kJsepDtlsServer,
    kJsepDtlsInvalidRole
  };

  virtual const SdpFingerprintAttributeList& fingerprints() const = 0;
  virtual Role role() const = 0;
};

class JsepIceTransport {
 public:
  virtual ~JsepIceTransport() {}

  virtual const std::string& ufrag() const = 0;
  virtual const std::string& password() const = 0;
  virtual const std::vector<std::string>& candidates() const = 0;

   //  virtual const SdpIceOptionsAttribute& options() const = 0;
 };

class JsepTransport {
  public:
   JsepTransport(const std::string& id, size_t components) :
       mTransportId(id),
       mState(kJsepTransportOffered),
       mComponents(components) {}


   enum State {
     kJsepTransportOffered,
     kJsepTransportAccepted,
     kJsepTransportClosed
   };

   // Unique identifier for this transport within this call. Group?
   std::string mTransportId;

   // The m-line index.
   size_t mLevel;
   size_t mBundleLevel;

   // State.
   State mState;

   // ICE stuff.
   UniquePtr<JsepIceTransport> mIce;
   UniquePtr<JsepDtlsTransport> mDtls;

   // Number of required components.
   size_t mComponents;

   NS_INLINE_DECL_THREADSAFE_REFCOUNTING(JsepTransport);

  protected:
  ~JsepTransport() {}
};

}  // namespace jsep
}  // namespace mozilla

#endif
