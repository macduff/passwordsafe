/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __PWSFILEV3_H
#define __PWSFILEV3_H

// PWSfileV3.h
// Abstract the gory details of reading and writing an encrypted database
//-----------------------------------------------------------------------------

#include "PWSfile.h"
#include "PWSFilters.h"
#include "TwoFish.h"
#include "sha256.h"
#include "hmac.h"
#include "UTF8Conv.h"

class PWSfileV3 : public PWSfile
{
public:

  enum {HDR_VERSION           = 0x00,
    HDR_UUID                  = 0x01,
    HDR_NDPREFS               = 0x02,
    HDR_DISPSTAT              = 0x03,
    HDR_LASTUPDATETIME        = 0x04,
    HDR_LASTUPDATEUSERHOST    = 0x05,     // DEPRECATED in format 0x0302
    HDR_LASTUPDATEAPPLICATION = 0x06,
    HDR_LASTUPDATEUSER        = 0x07,     // added in format 0x0302
    HDR_LASTUPDATEHOST        = 0x08,     // added in format 0x0302
    HDR_DBNAME                = 0x09,     // added in format 0x0302
    HDR_DBDESC                = 0x0a,     // added in format 0x0302
    HDR_FILTERS               = 0x0b,     // added in format 0x0305
    HDR_RESERVED1             = 0x0c,     // added in format 0x030?
    HDR_RESERVED2             = 0x0d,     // added in format 0x030?
    HDR_RESERVED3             = 0x0e,     // added in format 0x030?
    HDR_RUE                   = 0x0f,     // added in format 0x0307
    HDR_PSWDPOLICIES          = 0x10,     // added in format 0x030A
    HDR_EMPTYGROUP            = 0x11,     // added in format 0x030B
    HDR_RESERVED4             = 0x12,     // added in format 0x030C
    HDR_LAST,                             // Start of unknown fields!
    HDR_END                   = 0xff};    // header field types, per formatV{2,3}.txt

  static int CheckPasskey(const StringX &filename,
                          const StringX &passkey,
                          FILE *a_fd = NULL,
                          unsigned char *aPtag = NULL, int *nIter = NULL);
  static bool IsV3x(const StringX &filename, VERSION &v);

  PWSfileV3(const StringX &filename, RWmode mode, VERSION version);
  ~PWSfileV3();

  virtual int Open(const StringX &passkey);
  virtual int Close();

  virtual int WriteRecord(const CItemData &item);
  virtual int ReadRecord(CItemData &item);

  void SetFilters(const PWSFilters &MapFilters) {m_MapFilters = MapFilters;}
  const PWSFilters &GetFilters() const {return m_MapFilters;}

  void SetPasswordPolicies(const PSWDPolicyMap &MapPSWDPLC) {m_MapPSWDPLC = MapPSWDPLC;}
  const PSWDPolicyMap &GetPasswordPolicies() const {return m_MapPSWDPLC;}

  void SetEmptyGroups(const std::vector<StringX> &vEmptyGroups) {m_vEmptyGroups = vEmptyGroups;}
  const std::vector<StringX> &GetEmptyGroups() const {return m_vEmptyGroups;}

private:
  unsigned char m_ipthing[TwoFish::BLOCKSIZE]; // for CBC
  unsigned char m_key[32];
  HMAC_SHA256 m_hmac;
  CUTF8Conv m_utf8conv;
  virtual size_t WriteCBC(unsigned char type, const StringX &data);
  virtual size_t WriteCBC(unsigned char type, const unsigned char *data,
                          size_t length);

  virtual size_t ReadCBC(unsigned char &type, unsigned char* &data,
                         size_t &length);
  int WriteHeader();
  int ReadHeader();
  PWSFilters m_MapFilters;
  PSWDPolicyMap m_MapPSWDPLC;

  // EmptyGroups
  std::vector<StringX> m_vEmptyGroups;

  static int SanityCheck(FILE *stream); // Check for TAG and EOF marker
  static void StretchKey(const unsigned char *salt, unsigned long saltLen,
    const StringX &passkey,
    unsigned int N, unsigned char *Ptag);
};
#endif /* __PWSFILEV3_H */
