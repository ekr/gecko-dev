/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _JSEPTRANSPORTIMPL_H_
#define _JSEPTRANSPORTIMPL_H_

#include <mozilla/RefPtr.h>
#include <mozilla/UniquePtr.h>

#include "signaling/src/jsep/JsepTransport.h"
#include "signaling/src/sdp/SdpAttribute.h"

namespace mozilla {
namespace jsep {

class JsepDtlsTransportImpl : public JsepDtlsTransport {
 public:
  JsepDtlsTransportImpl()
      : mRole(kJsepDtlsInvalidRole) {}

  virtual ~JsepDtlsTransportImpl() {}

  virtual const SdpFingerprintAttributeList& fingerprints() const MOZ_OVERRIDE {
    return mFingerprints;
  }
  virtual Role role() const MOZ_OVERRIDE {
    return mRole;
  }

 private:
  friend JsepSessionImpl;

  SdpFingerprintAttributeList mFingerprints;
  Role mRole;
};

class JsepIceTransportImpl : public JsepIceTransport {
 public:
  JsepIceTransportImpl() {}

  virtual ~JsepIceTransportImpl() {}

  const std::string& ufrag() const MOZ_OVERRIDE { return mUfrag; }
  const std::string& password() const MOZ_OVERRIDE {
    return mPwd; }
  const std::vector<std::string>& candidates() const MOZ_OVERRIDE {
    return mCandidates; }
  //  const SdpIceOptionsAttribute& options() const MOZ_OVERRIDE {
  // return mOptions;
  // }

 private:
  friend JsepSessionImpl;

  std::string mUfrag;
  std::string mPwd;
  std::vector<std::string> mCandidates;
  // SdpIceOptionsAttribute mOptions;
};

}  // namespace jsep
}  // namespace mozilla

#endif
