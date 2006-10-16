#pragma once

// PWSfileV1V2.h
// Abstract the gory details of reading and writing an encrypted database
//-----------------------------------------------------------------------------

#include "PWSfile.h"
#include "BlowFish.h"

class PWSfileV1V2 : public PWSfile {
 public:
  static int CheckPassword(const CMyString &filename,
                           const CMyString &passkey, FILE *a_fd = NULL);

  PWSfileV1V2(const CMyString &filename, RWmode mode, VERSION version);
  ~PWSfileV1V2();

  virtual int Open(const CMyString &passkey);
  virtual int Close();

  virtual int WriteRecord(const CItemData &item);
  virtual int ReadRecord(CItemData &item);

 private:
  // crypto stuff for reading/writing files:
  unsigned char m_salt[SaltLength];
  unsigned char m_ipthing[BlowFish::BLOCKSIZE]; // for CBC
  int WriteV2Header();
  int ReadV2Header();
};
