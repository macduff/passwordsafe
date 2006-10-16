#pragma once

// sha256.h
// SHA256 for PasswordSafe, based on LibTomCrypt by
// Tom St Denis, tomstdenis@iahu.ca, http://libtomcrypt.org
//-----------------------------------------------------------------------------

#include "typedefs.h"
class SHA256
{
public:
  enum {HASHLEN = 32};
  SHA256();
  ~SHA256();
  void Update(const unsigned char *in, unsigned long inlen);
  void Final(unsigned char digest[HASHLEN]);
private:
  ulong64 length;
  ulong32 state[8], curlen;
  unsigned char buf[64];
};


