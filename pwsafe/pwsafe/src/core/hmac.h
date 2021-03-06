/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file hmac.h
// HMAC for PasswordSafe

//-----------------------------------------------------------------------------
#ifndef __HMAC_H
#define __HMAC_H

// Currently implemented only for sha256, as required by version 3
// of the database format.
// HMAC algorithm as per RFC2104
// Generalizing this to other hashes is left as an exercise to the reader...

#include "sha256.h" 

class HMAC_SHA256
{
public:
  enum {HASHLEN = 32};
  HMAC_SHA256(const unsigned char *key, unsigned long keylen); // Calls Init
  HMAC_SHA256(); // Init needs to be called separately
  ~HMAC_SHA256();
  void Init(const unsigned char *key, unsigned long keylen);
  void Update(const unsigned char *in, unsigned long inlen);
  void Final(unsigned char digest[HASHLEN]);

private:
  SHA256 H;
  /* for SHA256 hashsize(L) = 32, blocksize(B) = 64 */
  enum {L = 32, B = 64};
  unsigned char K[B];
};

#endif /* __HMAC_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:

