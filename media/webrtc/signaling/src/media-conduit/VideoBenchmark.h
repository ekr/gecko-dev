/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsISupports.h"
#include "nsICommandLineHandler.h"

namespace mozilla {

class VideoBenchmark : public nsICommandLineHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOMMANDLINEHANDLER

  VideoBenchmark();

private:
  ~VideoBenchmark();

protected:
  /* additional members */
};

}  // End of namespace
