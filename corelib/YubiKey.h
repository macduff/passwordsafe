/*
* Copyright (c) 2003-2008 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// \file YubiKey.h

#ifndef __YUBIKEY_H

/**
 * Simple interface for YubiKey authentication server API
 */
#include "os/typedefs.h"

class YubiKeyAuthenticator
{
 public:
  bool VerifyOTP(const stringT &otp);
  stringT GetError() const {return m_error;}

 private:
  stringT SignReq(const std::string &msg);
  bool VerifySig(const std::string &msg, const std::string &h);
  stringT m_error;
};

#define __YUBIKEY_H
#endif /* __YUBIKEY_H */
