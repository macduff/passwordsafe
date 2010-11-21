/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file ItemData.cpp
//-----------------------------------------------------------------------------

#include "ItemData.h"
#include "BlowFish.h"
#include "TwoFish.h"
#include "PWSrand.h"
#include "UTF8Conv.h"
#include "PWSprefs.h"
#include "VerifyFormat.h"
#include "PWHistory.h"
#include "Util.h"
#include "StringXStream.h"
#include "corelib.h"

#include "os/typedefs.h"
#include "os/pws_tchar.h"
#include "os/mem.h"

#include <time.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

bool CItemData::IsSessionKeySet = false;
unsigned char CItemData::SessionKey[64];

void CItemData::SetSessionKey()
{
  // must be called once per session, no more, no less
  ASSERT(!IsSessionKeySet);
  pws_os::mlock(SessionKey, sizeof(SessionKey));
  PWSrand::GetInstance()->GetRandomData( SessionKey, sizeof( SessionKey ) );
  IsSessionKeySet = true;
}

//-----------------------------------------------------------------------------
// Constructors

CItemData::CItemData()
  : m_Name(NAME), m_Title(TITLE), m_User(USER), m_Password(PASSWORD),
    m_Notes(NOTES), m_UUID(UUID), m_Group(GROUP),
    m_URL(URL), m_AutoType(AUTOTYPE),
    m_tttATime(ATIME), m_tttCTime(CTIME), m_tttXTime(XTIME),
    m_tttPMTime(PMTIME), m_tttRMTime(RMTIME), m_PWHistory(PWHIST),
    m_PWPolicy(POLICY), m_XTimeInterval(XTIME_INT), m_RunCommand(RUNCMD),
    m_DCA(DCA), m_email(EMAIL), m_entrytype(ET_NORMAL),
    m_entrystatus(ES_CLEAN), m_display_info(NULL)
{
  PWSrand::GetInstance()->GetRandomData( m_salt, SaltLength );
}

CItemData::CItemData(const CItemData &that) :
  m_Name(that.m_Name), m_Title(that.m_Title), m_User(that.m_User),
  m_Password(that.m_Password), m_Notes(that.m_Notes), m_UUID(that.m_UUID),
  m_Group(that.m_Group), m_URL(that.m_URL), m_AutoType(that.m_AutoType),
  m_tttATime(that.m_tttATime), m_tttCTime(that.m_tttCTime),
  m_tttXTime(that.m_tttXTime), m_tttPMTime(that.m_tttPMTime),
  m_tttRMTime(that.m_tttRMTime), m_PWHistory(that.m_PWHistory),
  m_PWPolicy(that.m_PWPolicy), m_XTimeInterval(that.m_XTimeInterval),
  m_RunCommand(that.m_RunCommand), m_DCA(that.m_DCA), m_email(that.m_email),
  m_entrytype(that.m_entrytype), m_entrystatus(that.m_entrystatus),
  m_display_info(that.m_display_info == NULL ?
                 NULL : that.m_display_info->clone())
{
  memcpy(reinterpret_cast<char*>(m_salt), reinterpret_cast<const char*>(that.m_salt), SaltLength);
  if (!that.m_URFL.empty())
    m_URFL = that.m_URFL;
  else
    m_URFL.clear();
}

CItemData::~CItemData()
{
  delete m_display_info;
}

CItemData& CItemData::operator=(const CItemData &that)
{
  // Check for self-assignment
  if (this != &that) {
    m_UUID = that.m_UUID;
    m_Name = that.m_Name;
    m_Title = that.m_Title;
    m_User = that.m_User;
    m_Password = that.m_Password;
    m_Notes = that.m_Notes;
    m_Group = that.m_Group;
    m_URL = that.m_URL;
    m_AutoType = that.m_AutoType;
    m_RunCommand = that.m_RunCommand;
    m_DCA = that.m_DCA;
    m_email = that.m_email;
    m_tttCTime = that.m_tttCTime;
    m_tttPMTime = that.m_tttPMTime;
    m_tttATime = that.m_tttATime;
    m_tttXTime = that.m_tttXTime;
    m_tttRMTime = that.m_tttRMTime;
    m_PWHistory = that.m_PWHistory;
    m_PWPolicy = that.m_PWPolicy;
    m_XTimeInterval = that.m_XTimeInterval;

    delete m_display_info;
    m_display_info = that.m_display_info == NULL ?
      NULL : that.m_display_info->clone();

    if (!that.m_URFL.empty())
      m_URFL = that.m_URFL;
    else
      m_URFL.clear();

    m_entrytype = that.m_entrytype;
    m_entrystatus = that.m_entrystatus;
    memcpy((char*)m_salt, (char*)that.m_salt, SaltLength);
  }

  return *this;
}

void CItemData::Clear()
{
  m_Group.Empty();
  m_Title.Empty();
  m_User.Empty();
  m_Password.Empty();
  m_Notes.Empty();
  m_URL.Empty();
  m_AutoType.Empty();
  m_RunCommand.Empty();
  m_DCA.Empty();
  m_email.Empty();
  m_tttCTime.Empty();
  m_tttPMTime.Empty();
  m_tttATime.Empty();
  m_tttXTime.Empty();
  m_tttRMTime.Empty();
  m_PWHistory.Empty();
  m_PWPolicy.Empty();
  m_XTimeInterval.Empty();
  m_URFL.clear();
  m_entrytype = ET_NORMAL;
  m_entrystatus = ES_CLEAN;
}

//-----------------------------------------------------------------------------
// Accessors

StringX CItemData::GetField(const CItemField &field) const
{
  StringX retval;
  BlowFish *bf = MakeBlowFish();
  field.Get(retval, bf);
  delete bf;
  return retval;
}

void CItemData::GetField(const CItemField &field, unsigned char *value, unsigned int &length) const
{
  BlowFish *bf = MakeBlowFish();
  field.Get(value, length, bf);
  delete bf;
}

StringX CItemData::GetFieldValue(const FieldType &ft) const
{
  StringX str(_T(""));
  int xint(0);

  switch (ft) {
    case GROUPTITLE: /* 00 */
      str = GetGroup() + TCHAR('.') + GetTitle();
      break;
    case UUID:       /* 01 */
    {
      uuid_array_t uuid_array = {0};
      GetUUID(uuid_array);
      const CUUIDGen cuuid(uuid_array, true);
      str = cuuid.GetHexStr();
      break;
    }
    case GROUP:      /* 02 */
      return GetGroup();
    case TITLE:      /* 03 */
      return GetTitle();
    case USER:       /* 04 */
      return GetUser();
    case NOTES:      /* 05 */
      return GetNotes();
    case PASSWORD:   /* 06 */
      return GetPassword();
    case CTIME:      /* 07 */
      return GetCTimeL();
    case PMTIME:     /* 08 */
      return GetPMTimeL();
    case ATIME:      /* 09 */
      return GetATimeL();
    case XTIME:      /* 0a */
      str = GetXTimeL();
      GetXTimeInt(xint);
      if (xint != 0)
        str += _T(" *");
      return str;
    case RMTIME:     /* 0c */
      return GetRMTimeL();
    case URL:        /* 0d */
      return GetURL();
    case AUTOTYPE:   /* 0e */
      return GetAutoType();
    case PWHIST:     /* 0f */
      return GetPWHistory();
    case POLICY:     /* 10 */
    {
      PWPolicy pwp;
      GetPWPolicy(pwp);
      if (pwp.flags != 0) {
        stringT st_pwp(_T("")), st_text;
        if (pwp.flags & PWSprefs::PWPolicyUseLowercase) {
          st_pwp += _T("L");
          if (pwp.lowerminlength > 1) {
            Format(st_text, _T("(%d)"), pwp.lowerminlength);
            st_pwp += st_text;
          }
        }
        if (pwp.flags & PWSprefs::PWPolicyUseUppercase) {
          st_pwp += _T("U");
          if (pwp.upperminlength > 1) {
            Format(st_text, _T("(%d)"), pwp.upperminlength);
            st_pwp += st_text;
          }
        }
        if (pwp.flags & PWSprefs::PWPolicyUseDigits) {
          st_pwp += _T("D");
          if (pwp.digitminlength > 1) {
            Format(st_text, _T("(%d)"), pwp.digitminlength);
            st_pwp += st_text;
          }
        }
        if (pwp.flags & PWSprefs::PWPolicyUseSymbols) {
          st_pwp += _T("S");
            if (pwp.symbolminlength > 1) {
            Format(st_text, _T("(%d)"), pwp.symbolminlength);
              st_pwp += st_text;
          }
        }
        if (pwp.flags & PWSprefs::PWPolicyUseHexDigits)
          st_pwp += _T("H");
        if (pwp.flags & PWSprefs::PWPolicyUseEasyVision)
          st_pwp += _T("E");
        if (pwp.flags & PWSprefs::PWPolicyMakePronounceable)
          st_pwp += _T("P");
        oStringXStream osx;
        osx << st_pwp << _T(":") << pwp.length;
        return osx.str().c_str();
      }
      break;
    }
    case XTIME_INT:  /* 11 */
     return GetXTimeInt();
    case RUNCMD:     /* 12 */
      return GetRunCommand();
    case DCA:        /* 13 */
      return GetDCA();
    case EMAIL:      /* 14 */
      return GetEmail();
    case RESERVED:
    default:
      ASSERT(0);
  }
  return str;
}

size_t CItemData::GetSize()
{
  size_t length(0);
  length += m_Name.GetLength();
  length += m_Title.GetLength();
  length += m_User.GetLength();
  length += m_Password.GetLength();
  length += m_Notes.GetLength();
  length += m_UUID.GetLength();
  length += m_Group.GetLength();
  length += m_URL.GetLength();
  length += m_AutoType.GetLength();
  length += m_tttATime.GetLength();
  length += m_tttCTime.GetLength();
  length += m_tttXTime.GetLength();
  length += m_tttPMTime.GetLength();
  length += m_tttRMTime.GetLength();
  length += m_PWHistory.GetLength();
  length += m_PWPolicy.GetLength();
  length += m_XTimeInterval.GetLength();
  length += m_RunCommand.GetLength();
  length += m_DCA.GetLength();
  length += m_email.GetLength();

  for (unsigned int i = 0; i != m_URFL.size(); i++) {
    CItemField &item = m_URFL.at(i);
    length += item.GetLength();
  }

  return length;
}

void CItemData::GetSize(int &isize) const
{
  isize  = m_Name.GetLength();
  isize += m_Title.GetLength();
  isize += m_User.GetLength();
  isize += m_Password.GetLength();
  isize += m_Notes.GetLength();
  isize += m_UUID.GetLength();
  isize += m_Group.GetLength();
  isize += m_URL.GetLength();
  isize += m_AutoType.GetLength();
  isize += m_tttATime.GetLength();
  isize += m_tttCTime.GetLength();
  isize += m_tttXTime.GetLength();
  isize += m_tttPMTime.GetLength();
  isize += m_tttRMTime.GetLength();
  isize += m_PWHistory.GetLength();
  isize += m_PWPolicy.GetLength();
  isize += m_XTimeInterval.GetLength();
  isize += m_RunCommand.GetLength();
  isize += m_DCA.GetLength();
  isize += m_email.GetLength();

  for (unsigned int i = 0; i != m_URFL.size(); i++) {
    isize += m_URFL.at(i).GetLength();
  }
}

StringX CItemData::GetName() const
{
  return GetField(m_Name);
}

StringX CItemData::GetTitle() const
{
  return GetField(m_Title);
}

StringX CItemData::GetUser() const
{
  return GetField(m_User);
}

StringX CItemData::GetPassword() const
{
  return GetField(m_Password);
}

static void CleanNotes(StringX &s, TCHAR delimiter)
{
  if (delimiter != 0) {
    StringX r2;
    for (StringX::iterator iter = s.begin(); iter != s.end(); iter++)
      switch (*iter) {
      case TCHAR('\r'): continue;
      case TCHAR('\n'): r2 += delimiter; continue;
      default: r2 += *iter;
      }
    s = r2;
  }
}

StringX CItemData::GetNotes(TCHAR delimiter) const
{
  StringX ret = GetField(m_Notes);
  CleanNotes(ret, delimiter);
  return ret;
}

StringX CItemData::GetGroup() const
{
  return GetField(m_Group);
}

StringX CItemData::GetURL() const
{
  return GetField(m_URL);
}

StringX CItemData::GetAutoType() const
{
  return GetField(m_AutoType);
}

StringX CItemData::GetTime(int whichtime, int result_format) const
{
  time_t t;

  GetTime(whichtime, t);
  return PWSUtil::ConvertToDateTimeString(t, result_format);
}

void CItemData::GetTime(int whichtime, time_t &t) const
{
  unsigned char in[TwoFish::BLOCKSIZE]; // required by GetField
  unsigned int tlen = sizeof(in); // ditto

  switch (whichtime) {
    case ATIME:
      GetField(m_tttATime, in, tlen);
      break;
    case CTIME:
      GetField(m_tttCTime, in, tlen);
      break;
    case XTIME:
      GetField(m_tttXTime, in, tlen);
      break;
    case PMTIME:
      GetField(m_tttPMTime, in, tlen);
      break;
    case RMTIME:
      GetField(m_tttRMTime, in, tlen);
      break;
    default:
      ASSERT(0);
  }

  if (tlen != 0) {
    int t32;
    ASSERT(tlen == sizeof(t32));
    memcpy(&t32, in, sizeof(t32));
    t = t32;
  } else {
    t = 0;
  }
}

void CItemData::GetUUID(uuid_array_t &uuid_array) const
{
  unsigned int length = sizeof(uuid_array);
  GetField(m_UUID, static_cast<unsigned char *>(uuid_array), length);
}

static void String2PWPolicy(const stringT &cs_pwp, PWPolicy &pwp)
{
  // should really be a c'tor of PWPolicy - later...

  // We need flags(4), length(3), lower_len(3), upper_len(3)
  //   digit_len(3), symbol_len(3) = 4 + 5 * 3 = 19 
  // All fields are hexadecimal digits representing flags or lengths

  // Note: order of fields set by PWSprefs enum that can have minimum lengths.
  // Later releases must support these as a minimum.  Any fields added
  // by these releases will be lost if the user changes these field.
  ASSERT(cs_pwp.length() == 19);

  // Get fields
  istringstreamT is_flags(stringT(cs_pwp, 0, 4));
  istringstreamT is_length(stringT(cs_pwp, 4, 3));
  istringstreamT is_digitminlength(stringT(cs_pwp, 7, 3));
  istringstreamT is_lowreminlength(stringT(cs_pwp, 10, 3));
  istringstreamT is_symbolminlength(stringT(cs_pwp, 13, 3));
  istringstreamT is_upperminlength(stringT(cs_pwp, 16, 3));

  // Put them into PWPolicy structure
  unsigned int f; // dain bramaged istringstream requires this runaround
  is_flags >> hex >> f;
  pwp.flags = static_cast<unsigned short>(f);
  is_length >> hex >> pwp.length;
  is_digitminlength >> hex >> pwp.digitminlength;
  is_lowreminlength >> hex >> pwp.lowerminlength;
  is_symbolminlength >> hex >> pwp.symbolminlength;
  is_upperminlength >> hex >> pwp.upperminlength;
}

void CItemData::GetPWPolicy(PWPolicy &pwp) const
{
  StringX cs_pwp(GetField(m_PWPolicy));

  pwp.flags = 0;
  if (cs_pwp.length() == 19)
    String2PWPolicy(cs_pwp.c_str(), pwp);
}

StringX CItemData::GetPWPolicy() const
{
  return GetField(m_PWPolicy);
}

void CItemData::GetXTimeInt(int &xint) const
{
  unsigned char in[TwoFish::BLOCKSIZE]; // required by GetField
  unsigned int tlen = sizeof(in); // ditto

  GetField(m_XTimeInterval, in, tlen);

  if (tlen != 0) {
    ASSERT(tlen == sizeof(int));
    memcpy(&xint, in, sizeof(int));
  } else {
    xint = 0;
  }
}

StringX CItemData::GetXTimeInt() const
{
  int xint;
  GetXTimeInt(xint);
  if (xint == 0)
    return _T("");

  oStringXStream os;
  os << xint;
  return os.str();
}

void CItemData::GetDCA(short &iDCA) const
{
  unsigned char in[TwoFish::BLOCKSIZE]; // required by GetField
  unsigned int tlen = sizeof(in); // ditto

  GetField(m_DCA, in, tlen);

  if (tlen != 0) {
    ASSERT(tlen == sizeof(short));
    memcpy(&iDCA, in, sizeof(short));
  } else {
    iDCA = -1;
  }
}

StringX CItemData::GetDCA() const
{
  short dca;
  GetDCA(dca);
  oStringXStream os;
  os << dca;
  return os.str();
}

StringX CItemData::GetRunCommand() const
{
  return GetField(m_RunCommand);
}

StringX CItemData::GetEmail() const
{
  return GetField(m_email);
}

void CItemData::GetUnknownField(unsigned char &type, unsigned int &length,
                                unsigned char * &pdata,
                                const CItemField &item) const
{
  ASSERT(pdata == NULL && length == 0);

  const unsigned int BLOCKSIZE = 8;

  type = item.GetType();
  unsigned int flength = item.GetLength();
  length = flength;
  flength += BLOCKSIZE; // ensure that we've enough for at least one block
  pdata = new unsigned char[flength];
  GetField(item, pdata, flength);
}

void CItemData::GetUnknownField(unsigned char &type, unsigned int &length,
                                unsigned char * &pdata,
                                const unsigned int &num) const
{
  const CItemField &unkrfe = m_URFL.at(num);
  GetUnknownField(type, length, pdata, unkrfe);
}

void CItemData::GetUnknownField(unsigned char &type, unsigned int &length,
                                unsigned char * &pdata,
                                const UnknownFieldsConstIter &iter) const
{
  const CItemField &unkrfe = *iter;
  GetUnknownField(type, length, pdata, unkrfe);
}

StringX CItemData::GetPWHistory() const
{
  StringX ret = GetField(m_PWHistory);
  if (ret == _T("0") || ret == _T("00000"))
    ret = _T("");
  return ret;
}

StringX CItemData::GetPlaintext(const TCHAR &separator,
                                const FieldBits &bsFields,
                                const TCHAR &delimiter,
                                const CItemData *pcibase) const
{
  StringX ret(_T(""));

  StringX grouptitle;
  const StringX title(GetTitle());
  const StringX group(GetGroup());
  const StringX user(GetUser());
  const StringX url(GetURL());
  const StringX notes(GetNotes(delimiter));

  // a '.' in title gets Import confused re: Groups
  grouptitle = title;
  if (grouptitle.find(TCHAR('.')) != StringX::npos) {
    if (delimiter != 0) {
      StringX s;
      for (StringX::iterator iter = grouptitle.begin();
           iter != grouptitle.end(); iter++)
        s += (*iter == TCHAR('.')) ? delimiter : *iter;
      grouptitle = s;
    } else {
      grouptitle = TCHAR('\"') + title + TCHAR('\"');
    }
  }

  if (!group.empty())
    grouptitle = group + TCHAR('.') + grouptitle;

  StringX history(_T(""));
  if (bsFields.test(CItemData::PWHIST)) {
    // History exported as "00000" if empty, to make parsing easier
    BOOL pwh_status;
    size_t pwh_max, num_err;
    PWHistList pwhistlist;

    pwh_status = CreatePWHistoryList(GetPWHistory(), pwh_max, num_err,
                                     pwhistlist, TMC_EXPORT_IMPORT);

    //  Build export string
    history = MakePWHistoryHeader(pwh_status, pwh_max, pwhistlist.size());
    PWHistList::iterator iter;
    for (iter = pwhistlist.begin(); iter != pwhistlist.end(); iter++) {
      const PWHistEntry &pwshe = *iter;
      history += _T(' ');
      history += pwshe.changedate;
      ostringstreamT os1;
      os1 << hex << charT(' ') << setfill(charT('0')) << setw(4)
          << pwshe.password.length() << charT(' ');
      history += os1.str().c_str();
      history += pwshe.password;
    }
  }

  StringX csPassword;
  if (m_entrytype == ET_ALIAS) {
    ASSERT(pcibase != NULL);
    csPassword = _T("[[") + 
                 pcibase->GetGroup() + _T(":") + 
                 pcibase->GetTitle() + _T(":") + 
                 pcibase->GetUser() + _T("]]") ;
  } else if (m_entrytype == ET_SHORTCUT) {
    ASSERT(pcibase != NULL);
    csPassword = _T("[~") + 
                 pcibase->GetGroup() + _T(":") + 
                 pcibase->GetTitle() + _T(":") + 
                 pcibase->GetUser() + _T("~]") ;
  } else
    csPassword = GetPassword();

  // Notes field must be last, for ease of parsing import
  if (bsFields.count() == bsFields.size()) {
    // Everything - note can't actually set all bits via dialog!
    // Must be in same order as full header
    ret = (grouptitle + separator + 
           user + separator +
           csPassword + separator + 
           url + separator + 
           GetAutoType() + separator +
           GetCTimeExp() + separator +
           GetPMTimeExp() + separator +
           GetATimeExp() + separator +
           GetXTimeExp() + separator +
           GetXTimeInt() + separator +
           GetRMTimeExp() + separator +
           GetPWPolicy() + separator +
           history + separator +
           GetRunCommand() + separator +
           GetDCA() + separator +
           GetEmail() + separator +
           _T("\"") + notes + _T("\""));
  } else {
    // Not everything
    // Must be in same order as custom header
    if (bsFields.test(CItemData::GROUP) && bsFields.test(CItemData::TITLE))
      ret += grouptitle + separator;
    else if (bsFields.test(CItemData::GROUP))
      ret += group + separator;
    else if (bsFields.test(CItemData::TITLE))
      ret += title + separator;
    if (bsFields.test(CItemData::USER))
      ret += user + separator;
    if (bsFields.test(CItemData::PASSWORD))
      ret += csPassword + separator;
    if (bsFields.test(CItemData::URL))
      ret += url + separator;
    if (bsFields.test(CItemData::AUTOTYPE))
      ret += GetAutoType() + separator;
    if (bsFields.test(CItemData::CTIME))
      ret += GetCTimeExp() + separator;
    if (bsFields.test(CItemData::PMTIME))
      ret += GetPMTimeExp() + separator;
    if (bsFields.test(CItemData::ATIME))
      ret += GetATimeExp() + separator;
    if (bsFields.test(CItemData::XTIME))
      ret += GetXTimeExp() + separator;
    if (bsFields.test(CItemData::XTIME_INT))
      ret += GetXTimeInt() + separator;
    if (bsFields.test(CItemData::RMTIME))
      ret += GetRMTimeExp() + separator;
    if (bsFields.test(CItemData::POLICY))
      ret += GetPWPolicy() + separator;
    if (bsFields.test(CItemData::PWHIST))
      ret += history + separator;
    if (bsFields.test(CItemData::RUNCMD))
      ret += GetRunCommand() + separator;
    if (bsFields.test(CItemData::DCA))
      ret += GetDCA() + separator;
    if (bsFields.test(CItemData::EMAIL))
      ret += GetEmail() + separator;
    if (bsFields.test(CItemData::NOTES))
      ret += _T("\"") + notes + _T("\"");
    // remove trailing separator
    if (ret[ret.length()-1] == separator) {
      int rl = ret.length();
      ret = ret.substr(0, rl - 1);
    }
  }

  return ret;
}

string CItemData::GetXML(unsigned id, const FieldBits &bsExport,
                         TCHAR delimiter, const CItemData *pcibase,
                         bool bforce_normal_entry) const
{
  ostringstream oss; // ALWAYS a string of chars, never wchar_t!
  oss << "\t<entry id=\"" << dec << id << "\"";
  if (bforce_normal_entry)
    oss << " normal=\"" << "true" << "\"";

  oss << ">" << endl;

  StringX tmp;
  CUTF8Conv utf8conv;

  tmp = GetGroup();
  if (bsExport.test(CItemData::GROUP) && !tmp.empty())
    PWSUtil::WriteXMLField(oss, "group", tmp, utf8conv);

  // Title mandatory (see pwsafe.xsd)
  PWSUtil::WriteXMLField(oss, "title", GetTitle(), utf8conv);

  tmp = GetUser();
  if (bsExport.test(CItemData::USER) && !tmp.empty())
    PWSUtil::WriteXMLField(oss, "username", tmp, utf8conv);

  // Password mandatory (see pwsafe.xsd)
  if (m_entrytype == ET_ALIAS) {
    ASSERT(pcibase != NULL);
    tmp = _T("[[") + 
          pcibase->GetGroup() + _T(":") + 
          pcibase->GetTitle() + _T(":") + 
          pcibase->GetUser() + _T("]]") ;
  } else
  if (m_entrytype == ET_SHORTCUT) {
    ASSERT(pcibase != NULL);
    tmp = _T("[~") + 
          pcibase->GetGroup() + _T(":") + 
          pcibase->GetTitle() + _T(":") + 
          pcibase->GetUser() + _T("~]") ;
  } else
    tmp = GetPassword();
  PWSUtil::WriteXMLField(oss, "password", tmp, utf8conv);

  tmp = GetURL();
  if (bsExport.test(CItemData::URL) && !tmp.empty())
    PWSUtil::WriteXMLField(oss, "url", tmp, utf8conv);

  tmp = GetAutoType();
  if (bsExport.test(CItemData::AUTOTYPE) && !tmp.empty())
    PWSUtil::WriteXMLField(oss, "autotype", tmp, utf8conv);

  tmp = GetNotes();
  if (bsExport.test(CItemData::NOTES) && !tmp.empty()) {
    CleanNotes(tmp, delimiter);
    PWSUtil::WriteXMLField(oss, "notes", tmp, utf8conv);
  }

  uuid_array_t uuid_array;
  GetUUID(uuid_array);
  const CUUIDGen uuid(uuid_array);
  oss << "\t\t<uuid><![CDATA[" << uuid << "]]></uuid>" << endl;

  time_t t;
  int i32;
  short i16;

  GetCTime(t);
  if (bsExport.test(CItemData::CTIME) && t)
    oss << PWSUtil::GetXMLTime(2, "ctime", t, utf8conv);

  GetATime(t);
  if (bsExport.test(CItemData::ATIME) && t)
    oss << PWSUtil::GetXMLTime(2, "atime", t, utf8conv);

  GetXTime(t);
  if (bsExport.test(CItemData::XTIME) && t)
    oss << PWSUtil::GetXMLTime(2, "xtime", t, utf8conv);

  GetXTimeInt(i32);
  if (bsExport.test(CItemData::XTIME_INT) && i32 > 0 && i32 <= 3650)
    oss << "\t\t<xtime_interval>" << dec << i32 << "</xtime_interval>" << endl;

  GetPMTime(t);
  if (bsExport.test(CItemData::PMTIME) && t)
    oss << PWSUtil::GetXMLTime(2, "pmtime", t, utf8conv);

  GetRMTime(t);
  if (bsExport.test(CItemData::RMTIME) && t)
    oss << PWSUtil::GetXMLTime(2, "rmtime", t, utf8conv);

  PWPolicy pwp;
  GetPWPolicy(pwp);
  if (bsExport.test(CItemData::POLICY) && pwp.flags != 0) {
    oss << "\t\t<PasswordPolicy>" << endl;
    oss << dec;
    oss << "\t\t\t<PWLength>" << pwp.length << "</PWLength>" << endl;
    if (pwp.flags & PWSprefs::PWPolicyUseLowercase)
      oss << "\t\t\t<PWUseLowercase>1</PWUseLowercase>" << endl;
    if (pwp.flags & PWSprefs::PWPolicyUseUppercase)
      oss << "\t\t\t<PWUseUppercase>1</PWUseUppercase>" << endl;
    if (pwp.flags & PWSprefs::PWPolicyUseDigits)
      oss << "\t\t\t<PWUseDigits>1</PWUseDigits>" << endl;
    if (pwp.flags & PWSprefs::PWPolicyUseSymbols)
      oss << "\t\t\t<PWUseSymbols>1</PWUseSymbols>" << endl;
    if (pwp.flags & PWSprefs::PWPolicyUseHexDigits)
      oss << "\t\t\t<PWUseHexDigits>1</PWUseHexDigits>" << endl;
    if (pwp.flags & PWSprefs::PWPolicyUseEasyVision)
      oss << "\t\t\t<PWUseEasyVision>1</PWUseEasyVision>" << endl;
    if (pwp.flags & PWSprefs::PWPolicyMakePronounceable)
      oss << "\t\t\t<PWMakePronounceable>1</PWMakePronounceable>" << endl;

    if (pwp.lowerminlength > 0) {
      oss << "\t\t\t<PWLowercaseMinLength>" << pwp.lowerminlength << "</PWLowercaseMinLength>" << endl;
    }
    if (pwp.upperminlength > 0) {
      oss << "\t\t\t<PWUppercaseMinLength>" << pwp.upperminlength << "</PWUppercaseMinLength>" << endl;
    }
    if (pwp.digitminlength > 0) {
      oss << "\t\t\t<PWDigitMinLength>" << pwp.digitminlength << "</PWDigitMinLength>" << endl;
    }
    if (pwp.symbolminlength > 0) {
      oss << "\t\t\t<PWSymbolMinLength>" << pwp.symbolminlength << "</PWSymbolMinLength>" << endl;
    }
    oss << "\t\t</PasswordPolicy>" << endl;
  }

  if (bsExport.test(CItemData::PWHIST)) {
    size_t pwh_max, num_err;
    PWHistList pwhistlist;
    bool pwh_status = CreatePWHistoryList(GetPWHistory(), pwh_max, num_err,
                                          pwhistlist, TMC_XML);
    oss << dec;
    if (pwh_status || pwh_max > 0 || !pwhistlist.empty()) {
      oss << "\t\t<pwhistory>" << endl;
      oss << "\t\t\t<status>" << pwh_status << "</status>" << endl;
      oss << "\t\t\t<max>" << pwh_max << "</max>" << endl;
      oss << "\t\t\t<num>" << pwhistlist.size() << "</num>" << endl;
      if (!pwhistlist.empty()) {
        oss << "\t\t\t<history_entries>" << endl;
        int num = 1;
        PWHistList::iterator hiter;
        for (hiter = pwhistlist.begin(); hiter != pwhistlist.end();
             hiter++) {
          const unsigned char * utf8 = NULL;
          int utf8Len = 0;

          oss << "\t\t\t\t<history_entry num=\"" << num << "\">" << endl;
          const PWHistEntry &pwshe = *hiter;
          oss << "\t\t\t\t\t<changed>" << endl;
          oss << "\t\t\t\t\t\t<date>";
          if (utf8conv.ToUTF8(pwshe.changedate.substr(0, 10), utf8, utf8Len))
            oss.write(reinterpret_cast<const char *>(utf8), utf8Len);
          oss << "</date>" << endl;
          oss << "\t\t\t\t\t\t<time>";
          if (utf8conv.ToUTF8(pwshe.changedate.substr(pwshe.changedate.length()-8),
                              utf8, utf8Len))
            oss.write(reinterpret_cast<const char *>(utf8), utf8Len);
          oss << "</time>" << endl;
          oss << "\t\t\t\t\t</changed>" << endl;
          PWSUtil::WriteXMLField(oss, "oldpassword", pwshe.password,
                        utf8conv, "\t\t\t\t\t");
          oss << "\t\t\t\t</history_entry>" << endl;

          num++;
        } // for
        oss << "\t\t\t</history_entries>" << endl;
      } // if !empty
      oss << "\t\t</pwhistory>" << endl;
    }
  }

  tmp = GetRunCommand();
  if (bsExport.test(CItemData::RUNCMD) && !tmp.empty())
    PWSUtil::WriteXMLField(oss, "runcommand", tmp, utf8conv);

  GetDCA(i16);
  if (bsExport.test(CItemData::DCA) && 
      i16 >= PWSprefs::minDCA && i16 <= PWSprefs::maxDCA)
    oss << "\t\t<dca>" << i16 << "</dca>" << endl;

  tmp = GetEmail();
  if (bsExport.test(CItemData::EMAIL) && !tmp.empty())
    PWSUtil::WriteXMLField(oss, "email", tmp, utf8conv);

  if (NumberUnknownFields() > 0) {
    oss << "\t\t<unknownrecordfields>" << endl;
    for (unsigned int i = 0; i != NumberUnknownFields(); i++) {
      unsigned int length = 0;
      unsigned char type;
      unsigned char *pdata(NULL);
      GetUnknownField(type, length, pdata, i);
      if (length == 0)
        continue;
      // UNK_HEX_REP will represent unknown values
      // as hexadecimal, rather than base64 encoding.
      // Easier to debug.
#ifndef UNK_HEX_REP
      tmp = PWSUtil::Base64Encode(pdata, length).c_str();
#else
      tmp.clear();
      String X sx_tmp;
      unsigned char * pdata2(pdata);
      unsigned char c;
      for (int j = 0; j < (int)length; j++) {
        c = *pdata2++;
        Format(sx_tmp, _T("%02x"), c);
        tmp += sx_tmp;
      }
#endif
      oss << "\t\t\t<field ftype=\"" << int(type) << "\">"
          << tmp.c_str() << "</field>" << endl;
      trashMemory(pdata, length);
      delete[] pdata;
    } // iteration over unknown fields
    oss << "\t\t</unknownrecordfields>" << endl;  
  } // if there are unknown fields

  oss << "\t</entry>" << endl << endl;
  return oss.str();
}

void CItemData::SplitName(const StringX &name,
                          StringX &title, StringX &username)
{
  StringX::size_type pos = name.find(SPLTCHR);
  if (pos == StringX::npos) {//Not a split name
    StringX::size_type pos2 = name.find(DEFUSERCHR);
    if (pos2 == StringX::npos)  {//Make certain that you remove the DEFUSERCHR
      title = name;
    } else {
      title = (name.substr(0, pos2));
    }
  } else {
    /*
    * There should never ever be both a SPLITCHR and a DEFUSERCHR in
    * the same string
    */
    StringX temp;
    temp = name.substr(0, pos);
    TrimRight(temp);
    title = temp;
    temp = name.substr(pos+1); // Zero-index string
    TrimLeft(temp);
    username = temp;
  }
}

//-----------------------------------------------------------------------------
// Setters

void CItemData::SetField(CItemField &field, const StringX &value)
{
  BlowFish *bf = MakeBlowFish();
  field.Set(value, bf);
  delete bf;
}

void CItemData::SetField(CItemField &field,
                         const unsigned char *value, unsigned int length)
{
  BlowFish *bf = MakeBlowFish();
  field.Set(value, length, bf);
  delete bf;
}

void CItemData::CreateUUID()
{
  CUUIDGen uuid;
  uuid_array_t uuid_array;
  uuid.GetUUID(uuid_array);
  SetUUID(uuid_array);
}

void CItemData::SetName(const StringX &name, const StringX &defaultUsername)
{
  // the m_name is from pre-2.0 versions, and may contain the title and user
  // separated by SPLTCHR. Also, DEFUSERCHR signified that the default username is to be used.
  // Here we fill the title and user fields so that
  // the application can ignore this difference after an ItemData record
  // has been created
  StringX title, user;
  StringX::size_type pos = name.find(DEFUSERCHR);
  if (pos != StringX::npos) {
    title = name.substr(0, pos);
    user = defaultUsername;
  } else
    SplitName(name, title, user);
  // In order to avoid unecessary BlowFish construction/deletion,
  // we forego SetField here...
  BlowFish *bf = MakeBlowFish();
  m_Name.Set(name, bf);
  m_Title.Set(title, bf);
  m_User.Set(user, bf);
  delete bf;
}

void CItemData::SetTitle(const StringX &title, TCHAR delimiter)
{
  if (delimiter == 0)
    SetField(m_Title, title);
  else {
    StringX new_title(_T(""));
    StringX newstringT, tmpstringT;
    StringX::size_type pos = 0;

    newstringT = title;
    do {
      pos = newstringT.find(delimiter);
      if ( pos != StringX::npos ) {
        new_title += newstringT.substr(0, pos) + _T(".");

        tmpstringT = newstringT.substr(pos + 1);
        newstringT = tmpstringT;
      }
    } while ( pos != StringX::npos );

    if (!newstringT.empty())
      new_title += newstringT;

    SetField(m_Title, new_title);
  }
}

void CItemData::SetUser(const StringX &user)
{
  SetField(m_User, user);
}

void CItemData::UpdatePassword(const StringX &password)
{
  // use when password changed - manages history, modification times
  UpdatePasswordHistory();
  SetPassword(password);

  time_t t;
  time(&t);
  SetPMTime(t);

  int xint;
  GetXTimeInt(xint);
  if (xint != 0) {
    // convert days to seconds for time_t
    t += (xint * 86400);
    SetXTime(t);
  }
}

void CItemData::UpdatePasswordHistory()
{
  PWHistList pwhistlist;
  size_t pwh_max;
  bool saving;
  const StringX pwh_str = GetPWHistory();
  if (pwh_str.empty()) {
    // If GetPWHistory() is empty, use preference values!
    const PWSprefs *prefs = PWSprefs::GetInstance();
    saving = prefs->GetPref(PWSprefs::SavePasswordHistory);
    pwh_max = prefs->GetPref(PWSprefs::NumPWHistoryDefault);
  } else {
    size_t num_err;
    saving = CreatePWHistoryList(pwh_str, pwh_max, num_err,
                                 pwhistlist, TMC_EXPORT_IMPORT);
  }
  if (!saving)
    return;

  size_t num = pwhistlist.size();

  time_t t;
  GetPMTime(t); // get mod time of last password

  if (!t) // if never set - try creation date
    GetCTime(t);

  PWHistEntry pwh_ent;
  pwh_ent.password = GetPassword();
  pwh_ent.changetttdate = t;
  pwh_ent.changedate =
    PWSUtil::ConvertToDateTimeString(t, TMC_EXPORT_IMPORT);

  if (pwh_ent.changedate.empty()) {
    StringX unk;
    LoadAString(unk, IDSC_UNKNOWN);
    pwh_ent.changedate = unk;
  }

  // Now add the latest
  pwhistlist.push_back(pwh_ent);

  // Increment count
  num++;

  // Too many? remove the excess
  if (num > pwh_max) {
    PWHistList hl(pwhistlist.begin() + (num - pwh_max),
                  pwhistlist.end());
    ASSERT(hl.size() == pwh_max);
    pwhistlist = hl;
    num = pwh_max;
  }

  // Now create string version!
  StringX new_PWHistory, buffer;

  Format(new_PWHistory, _T("1%02x%02x"), pwh_max, num);

  PWHistList::iterator iter;
  for (iter = pwhistlist.begin(); iter != pwhistlist.end(); iter++) {
    Format(buffer, _T("%08x%04x%s"),
           static_cast<long>(iter->changetttdate), iter->password.length(),
           iter->password.c_str());
    new_PWHistory += buffer;
    buffer = _T("");
  }
  SetPWHistory(new_PWHistory);
}


void CItemData::SetPassword(const StringX &password)
{
  SetField(m_Password, password);
}

void CItemData::SetNotes(const StringX &notes, TCHAR delimiter)
{
  if (delimiter == 0)
    SetField(m_Notes, notes);
  else {
    const StringX CRLF = _T("\r\n");
    StringX multiline_notes(_T(""));

    StringX newstringT;
    StringX tmpstringT;

    StringX::size_type pos = 0;

    newstringT = notes;
    do {
      pos = newstringT.find(delimiter);
      if ( pos != StringX::npos ) {
        multiline_notes += newstringT.substr(0, pos) + CRLF;

        tmpstringT = newstringT.substr(pos + 1);
        newstringT = tmpstringT;
      }
    } while ( pos != StringX::npos );

    if (!newstringT.empty())
      multiline_notes += newstringT;

    SetField(m_Notes, multiline_notes);
  }
}

void CItemData::SetGroup(const StringX &group)
{
  SetField(m_Group, group);
}

void CItemData::SetUUID(const uuid_array_t &uuid)
{
  SetField(m_UUID, static_cast<const unsigned char *>(uuid), sizeof(uuid));
}

void CItemData::SetURL(const StringX &url)
{
  SetField(m_URL, url);
}

void CItemData::SetAutoType(const StringX &autotype)
{
  SetField(m_AutoType, autotype);
}

void CItemData::SetTime(int whichtime)
{
  time_t t;
  time(&t);
  SetTime(whichtime, t);
}

void CItemData::SetTime(int whichtime, time_t t)
{
  int t32 = static_cast<int>(t);
  switch (whichtime) {
    case ATIME:
      SetField(m_tttATime, reinterpret_cast<const unsigned char *>(&t32), sizeof(t32));
      break;
    case CTIME:
      SetField(m_tttCTime, reinterpret_cast<const unsigned char *>(&t32), sizeof(t32));
      break;
    case XTIME:
      SetField(m_tttXTime, reinterpret_cast<const unsigned char *>(&t32), sizeof(t32));
      break;
    case PMTIME:
      SetField(m_tttPMTime, reinterpret_cast<const unsigned char *>(&t32), sizeof(t32));
      break;
    case RMTIME:
      SetField(m_tttRMTime, reinterpret_cast<const unsigned char *>(&t32), sizeof(t32));
      break;
    default:
      ASSERT(0);
  }
}

bool CItemData::SetTime(int whichtime, const stringT &time_str)
{
  time_t t(0);

  if (time_str.empty()) {
    SetTime(whichtime, t);
    return true;
  } else
    if (time_str == _T("now")) {
      time(&t);
      SetTime(whichtime, t);
      return true;
    } else
      if ((VerifyImportDateTimeString(time_str, t) ||
           VerifyXMLDateTimeString(time_str, t) ||
           VerifyASCDateTimeString(time_str, t)) &&
          (t != time_t(-1))  // checkerror despite all our verification!
          ) {
        SetTime(whichtime, t);
        return true;
      }
  return false;
}

void CItemData::SetXTimeInt(int &xint)
{
   SetField(m_XTimeInterval, reinterpret_cast<const unsigned char *>(&xint), sizeof(int));
}

bool CItemData::SetXTimeInt(const stringT &xint_str)
{
  int xint(0);

  if (xint_str.empty()) {
    SetXTimeInt(xint);
    return true;
  }

  if (xint_str.find_first_not_of(_T("0123456789")) == stringT::npos) {
    istringstreamT is(xint_str);
    is >> xint;
    if (is.fail())
      return false;
    if (xint >= 0 && xint <= 3650) {
      SetXTimeInt(xint);
      return true;
    }
  }
  return false;
}

void CItemData::SetUnknownField(const unsigned char &type,
                                const unsigned int &length,
                                const unsigned char * &ufield)
{
  CItemField unkrfe(type);
  SetField(unkrfe, ufield, length);
  m_URFL.push_back(unkrfe);
}

void CItemData::SetPWHistory(const StringX &PWHistory)
{
  StringX pwh = PWHistory;
  if (pwh == _T("0") || pwh == _T("00000"))
    pwh = _T("");
  SetField(m_PWHistory, pwh);
}

void CItemData::SetPWPolicy(const PWPolicy &pwp)
{
  // Must be some flags; however hex incompatible with other flags
  bool bhex_flag = (pwp.flags & PWSprefs::PWPolicyUseHexDigits) != 0;
  bool bother_flags = (pwp.flags & (~PWSprefs::PWPolicyUseHexDigits)) != 0;

  StringX cs_pwp;

  if (pwp.flags == 0 || (bhex_flag && bother_flags)) {
    cs_pwp = _T("");
  } else {
    ostringstreamT os;
    unsigned int f; // dain bramaged istringstream requires this runaround
    f = static_cast<unsigned int>(pwp.flags);
    os.fill(charT('0'));
    os << hex << setw(4) << f
       << setw(3) << pwp.length
       << setw(3) << pwp.digitminlength
       << setw(3) << pwp.lowerminlength
       << setw(3) << pwp.symbolminlength
       << setw(3) << pwp.upperminlength;
    cs_pwp = os.str().c_str();
  }
  SetField(m_PWPolicy, cs_pwp);
}

bool CItemData::SetPWPolicy(const stringT &cs_pwp)
{
  // Basic sanity checks
  if (cs_pwp.empty()) {
    SetField(m_PWPolicy, cs_pwp.c_str());
    return true;
  }
  if (cs_pwp.length() < 19)
    return false;

  // Parse policy string, more sanity checks
  // See String2PWPolicy for valid format
  PWPolicy pwp;
  String2PWPolicy(stringT(cs_pwp), pwp);
  StringX cs_pwpolicy(cs_pwp.c_str());

  // Must be some flags; however hex incompatible with other flags
  bool bhex_flag = (pwp.flags & PWSprefs::PWPolicyUseHexDigits) != 0;
  bool bother_flags = (pwp.flags & (~PWSprefs::PWPolicyUseHexDigits)) != 0;
  const int total_sublength = pwp.digitminlength + pwp.lowerminlength +
    pwp.symbolminlength + pwp.upperminlength;

  if (pwp.flags == 0 || (bhex_flag && bother_flags) ||
      pwp.length > 1024 || total_sublength > pwp.length ||
      pwp.digitminlength > 1024 || pwp.lowerminlength > 1024 ||
      pwp.symbolminlength > 1024 || pwp.upperminlength > 1024) {
    cs_pwpolicy.clear();
  }

  SetField(m_PWPolicy, cs_pwpolicy);
  return true;
}

void CItemData::SetRunCommand(const StringX &cs_RunCommand)
{
  SetField(m_RunCommand, cs_RunCommand);
}

void CItemData::SetEmail(const StringX &cs_email)
{
  SetField(m_email, cs_email);
}

void CItemData::SetDCA(const short &iDCA)
{
   
   SetField(m_DCA, reinterpret_cast<const unsigned char *>(&iDCA), sizeof(short));
}

bool CItemData::SetDCA(const stringT &cs_DCA)
{
  short iDCA(-1);

  if (cs_DCA.empty()) {
    SetDCA(iDCA);
    return true;
  }

  if (cs_DCA.find_first_not_of(_T("0123456789")) == stringT::npos) {
    istringstreamT is(cs_DCA);
    is >> iDCA;
    if (is.fail())
      return false;
    if (iDCA == -1 || (iDCA >= PWSprefs::minDCA && iDCA <= PWSprefs::maxDCA)) {
      SetDCA(iDCA);
      return true;
    }
  }
  return false;
}

void CItemData::SetFieldValue(const FieldType &ft, const StringX &value)
{
  switch (ft) {
    case GROUP:      /* 02 */
      SetGroup(value);
      break;
    case TITLE:      /* 03 */
      SetTitle(value);
      break;
    case USER:       /* 04 */
      SetUser(value);
      break;
    case NOTES:      /* 05 */
      SetNotes(value);
      break;
    case PASSWORD:   /* 06 */
      SetPassword(value);
      break;
    case CTIME:      /* 07 */
      SetCTime(value.c_str());
      break;
    case PMTIME:     /* 08 */
      SetPMTime(value.c_str());
      break;
    case ATIME:      /* 09 */
      SetATime(value.c_str());
      break;
    case XTIME:      /* 0a */
      SetXTime(value.c_str());
      break;
    case RMTIME:     /* 0c */
      SetRMTime(value.c_str());
      break;
    case URL:        /* 0d */
      SetURL(value);
      break;
    case AUTOTYPE:   /* 0e */
      SetAutoType(value);
      break;
    case PWHIST:     /* 0f */
      SetPWHistory(value);
      break;
    case POLICY:     /* 10 */
      SetPWPolicy(value.c_str());
      break;
    case XTIME_INT:  /* 11 */
      SetXTimeInt(value.c_str());
      break;
    case RUNCMD:     /* 12 */
      SetRunCommand(value);
      break;
    case DCA:        /* 13 */
      SetDCA(value.c_str());
      break;
    case EMAIL:      /* 14 */
      SetEmail(value);
      break;
    case GROUPTITLE: /* 00 */
    case UUID:       /* 01 */
    case RESERVED:   /* 0b */
    default:
      ASSERT(0);     /* Not supported */
  }
}

BlowFish *CItemData::MakeBlowFish() const
{
  ASSERT(IsSessionKeySet);
  return BlowFish::MakeBlowFish(SessionKey, sizeof(SessionKey),
    m_salt, SaltLength);
}

int CItemData::ValidateUUID(const unsigned short &nMajor, const unsigned short &nMinor,
                            uuid_array_t &uuid_array)
{
  // currently only ensure that item has a uuid, creating one if missing.

  /* NOTE the unusual position of the default statement.

  This is by design as it assumes that if we don't know the version, the
  database probably has all the problems and should be fixed.

  To date, we know that databases of format 0x0200 and 0x0300 have a UUID
  problem if records were duplicated.  Databases of format 0x0100 did not
  have the duplicate function and it has been fixed in databases in format
  0x0301.

  As more problems are detected and need to be fixed, this code will expand
  and may have to change.
  */
  int iResult(0);
  switch ((nMajor << 8) + nMinor) {
    default:
    case 0x0200:  // V2 format
    case 0x0300:  // V3 format prior to PWS V3.03
      CreateUUID();
      GetUUID(uuid_array);
      iResult = 1;
    case 0x0100:  // V1 format
    case 0x0301:  // V3 format PWS V3.03 and later
      break;
  }
  return iResult;
}

bool CItemData::ValidatePWHistory()
{
  if (m_PWHistory.IsEmpty())
    return true;

  const StringX pwh = GetPWHistory();
  if (pwh.length() < 5) { // not empty, but too short.
    SetPWHistory(_T(""));
    return false;
  }

  size_t pwh_max, num_err;
  PWHistList pwhistlist;
  bool pwh_status = CreatePWHistoryList(pwh, pwh_max, num_err,
                                        pwhistlist, TMC_EXPORT_IMPORT);
  if (num_err == 0)
    return true;

  size_t listnum = pwhistlist.size();

  if (pwh_max == 0 && listnum == 0) {
    SetPWHistory(_T(""));
    return false;
  }

  if (listnum > pwh_max)
    pwh_max = listnum;

  if (pwh_max < listnum)
    pwh_max = listnum;

  // Rebuild PWHistory from the data we have
  StringX history = MakePWHistoryHeader(pwh_status, pwh_max, listnum);

  PWHistList::iterator iter;
  for (iter = pwhistlist.begin(); iter != pwhistlist.end(); iter++) {
    const PWHistEntry &pwshe = *iter;
    history += pwshe.changedate;
    oStringXStream os1;
    os1 << hex << setfill(charT('0')) << setw(4)
        << pwshe.password.length();
    history += os1.str();
    history += pwshe.password;
  }
  SetPWHistory(history);

  return false;
}

bool CItemData::Matches(const stringT &string, int iObject,
                        int iFunction) const
{
  ASSERT(iFunction != 0); // must be positive or negative!

  StringX csObject;
  switch(iObject) {
    case GROUP:
      csObject = GetGroup();
      break;
    case TITLE:
      csObject = GetTitle();
      break;
    case USER:
      csObject = GetUser();
      break;
    case GROUPTITLE:
      csObject = GetGroup() + TCHAR('.') + GetTitle();
      break;
    case URL:
      csObject = GetURL();
      break;
    case NOTES:
      csObject = GetNotes();
      break;
    case PASSWORD:
      csObject = GetPassword();
      break;
    case RUNCMD:
      csObject = GetRunCommand();
      break;
    case EMAIL:
      csObject = GetEmail();
      break;
    case AUTOTYPE:
      csObject = GetAutoType();
      break;
    default:
      ASSERT(0);
  }

  const bool bValue = !csObject.empty();
  if (iFunction == PWSMatch::MR_PRESENT || iFunction == PWSMatch::MR_NOTPRESENT) {
    return PWSMatch::Match(bValue, iFunction);
  }

  if (!bValue) // String empty - always return false for other comparisons
    return false;
  else
    return PWSMatch::Match(string.c_str(), csObject, iFunction);
}

bool CItemData::Matches(int num1, int num2, int iObject,
                        int iFunction) const
{
  //   Check integer values are selected
  int iValue;

  switch (iObject) {
    case XTIME_INT:
      GetXTimeInt(iValue);
      break;
    case ENTRYSIZE:
      GetSize(iValue);
      break;
    default:
      ASSERT(0);
      return false;
  }

  const bool bValue = (iValue != 0);
  if (iFunction == PWSMatch::MR_PRESENT || iFunction == PWSMatch::MR_NOTPRESENT)
    return PWSMatch::Match(bValue, iFunction);

  if (!bValue) // integer empty - always return false for other comparisons
    return false;
  else
    return PWSMatch::Match(num1, num2, iValue, iFunction);
}

bool CItemData::Matches(short dca, int iFunction) const
{
  short iDCA;
  GetDCA(iDCA);
  switch (iFunction) {
    case PWSMatch::MR_IS:
      return iDCA == dca;
    case PWSMatch::MR_ISNOT:
      return iDCA != dca;
    default:
      ASSERT(0);
  }
  return false;
}

bool CItemData::Matches(time_t time1, time_t time2, int iObject,
                        int iFunction) const
{
  //   Check time values are selected
  time_t tValue;

  switch (iObject) {
    case CTIME:
    case PMTIME:
    case ATIME:
    case XTIME:
    case RMTIME:
      GetTime(iObject, tValue);
      break;
    default:
      ASSERT(0);
      return false;
  }

  const bool bValue = (tValue != time_t(0));
  if (iFunction == PWSMatch::MR_PRESENT || iFunction == PWSMatch::MR_NOTPRESENT) {
    return PWSMatch::Match(bValue, iFunction);
  }

  if (!bValue)  // date empty - always return false for other comparisons
    return false;
  else {
    time_t testtime;
    if (tValue) {
      struct tm st;
#if (_MSC_VER >= 1400)
      errno_t err;
      err = localtime_s(&st, &tValue);  // secure version
      ASSERT(err == 0);
#else
      st = *localtime(&tValue);
#endif
      st.tm_hour = 0;
      st.tm_min = 0;
      st.tm_sec = 0;
      testtime = mktime(&st);
    } else
      testtime = time_t(0);
    return PWSMatch::Match(time1, time2, testtime, iFunction);
  }
}
  
bool CItemData::Matches(EntryType etype, int iFunction) const
{
  switch (iFunction) {
    case PWSMatch::MR_IS:
      return GetEntryType() == etype;
    case PWSMatch::MR_ISNOT:
      return GetEntryType() != etype;
    default:
      ASSERT(0);
  }
  return false;
}

bool CItemData::Matches(EntryStatus estatus, int iFunction) const
{
  switch (iFunction) {
    case PWSMatch::MR_IS:
      return GetStatus() == estatus;
    case PWSMatch::MR_ISNOT:
      return GetStatus() != estatus;
    default:
      ASSERT(0);
  }
  return false;
}

bool CItemData::IsExpired() const
{
  time_t now, XTime;
  time(&now);

  GetXTime(XTime);
  return (XTime && (XTime < now));
}

bool CItemData::WillExpire(const int numdays) const
{
  time_t now, exptime, XTime;
  time(&now);

  GetXTime(XTime);
  // Check if there is an expiry date?
  if (!XTime)
    return false;

  // Ignore if already expired
  if (XTime <= now)
    return false;

  struct tm st;
#if (_MSC_VER >= 1400)
  errno_t err;
  err = localtime_s(&st, &now);  // secure version
  ASSERT(err == 0);
#else
  st = *localtime(&now);
#endif
  st.tm_mday += numdays;
  exptime = mktime(&st);
  if (exptime == time_t(-1))
    exptime = now;

  // Will it expire in numdays?
  return (XTime < exptime);
}

bool pull_string(StringX &str, const unsigned char *data, int len)
{
  CUTF8Conv utf8conv;
  vector<unsigned char> v(data, (data + len));
  v.push_back(0); // null terminate for FromUTF8.
  bool utf8status = utf8conv.FromUTF8(&v[0], len, str);
  if (!utf8status) {
    pws_os::Trace(_T("CItemData::DeserializePlainText(): FromUTF8 failed!\n"));
  }
  trashMemory(&v[0], len);
  return utf8status;
}

bool pull_time(time_t &t, const unsigned char *data, size_t len)
{
  if (len == sizeof(__time32_t)) {
    t = *reinterpret_cast<const __time32_t *>(data);
  } else if (len == sizeof(__time64_t)) {
    // convert from 64 bit time to currently supported 32 bit
    struct tm ts;
    const __time64_t *t64 = reinterpret_cast<const __time64_t *>(data);
    if (_gmtime64_s(&ts, t64) != 0) {
      ASSERT(0); return false;
    }
    t = _mkgmtime32(&ts);
    if (t == time_t(-1)) { // time is past 2038!
      t = 0; return false;
    }
  } else {
    ASSERT(0); return false;
  }
  return true;
}

bool pull_uint(unsigned int &uint, const unsigned char *data, size_t len)
{
  if (len == sizeof(unsigned long)) {
    uint = *reinterpret_cast<const unsigned long *>(data);
  } else {
    ASSERT(0);
    return false;
  }
  return true;
}

bool pull_int(int &i, const unsigned char *data, size_t len)
{
  if (len == sizeof(int)) {
    i = *reinterpret_cast<const int *>(data);
  } else {
    ASSERT(0);
    return false;
  }
  return true;
}

bool pull_int16(short &i16, const unsigned char *data, size_t len)
{
  if (len == sizeof(short)) {
    i16 = *reinterpret_cast<const short *>(data);
  } else {
    ASSERT(0);
    return false;
  }
  return true;
}

bool CItemData::DeserializePlainText(const std::vector<char> &v)
{
  vector<char>::const_iterator iter = v.begin();
  int emergencyExit = 255;

  while (iter != v.end()) {
    int type = (*iter++ & 0xff); // required since enum is an int
    if (size_t(v.end() - iter) < sizeof(size_t)) {
      ASSERT(0); // type must ALWAYS be followed by length
      return false;
    }

    if (type == END)
      return true; // happy end

    unsigned int len = *(reinterpret_cast<const unsigned int *>(&(*iter)));
    ASSERT(len < v.size()); // sanity check
    iter += sizeof(unsigned int);

    if (--emergencyExit == 0) {
      ASSERT(0);
      return false;
    }
    if (!SetField(type, reinterpret_cast<const unsigned char *>(&(*iter)), len))
      return false;
    iter += len;
  }
  return false; // END tag not found!
}

bool CItemData::SetField(int type, const unsigned char *data, int len)
{
  StringX str;
  time_t t;
  int i32;
  short i16;

  switch (type) {
    case NAME:
      ASSERT(0); // not serialized, or in v3 format
      return false;
    case UUID:
    {
      uuid_array_t uuid_array;
      ASSERT(len == sizeof(uuid_array));
      for (unsigned i = 0; i < sizeof(uuid_array); i++)
        uuid_array[i] = data[i];
      SetUUID(uuid_array);
      break;
    }
    case GROUP:
      if (!pull_string(str, data, len)) return false;
      SetGroup(str);
      break;
    case TITLE:
      if (!pull_string(str, data, len)) return false;
      SetTitle(str);
      break;
    case USER:
      if (!pull_string(str, data, len)) return false;
      SetUser(str);
      break;
    case NOTES:
      if (!pull_string(str, data, len)) return false;
      SetNotes(str);
      break;
    case PASSWORD:
      if (!pull_string(str, data, len)) return false;
      SetPassword(str);
      break;
    case CTIME:
      if (!pull_time(t, data, len)) return false;
      SetCTime(t);
      break;
    case  PMTIME:
      if (!pull_time(t, data, len)) return false;
      SetPMTime(t);
      break;
    case ATIME:
      if (!pull_time(t, data, len)) return false;
      SetATime(t);
      break;
    case XTIME:
      if (!pull_time(t, data, len)) return false;
      SetXTime(t);
      break;
    case XTIME_INT:
      if (!pull_int(i32, data, len)) return false;
      SetXTimeInt(i32);
      break;
    case POLICY:
      if (!pull_string(str, data, len)) return false;
      SetPWPolicy(str.c_str());
      break;
    case RMTIME:
      if (!pull_time(t, data, len)) return false;
      SetRMTime(t);
      break;
    case URL:
      if (!pull_string(str, data, len)) return false;
      SetURL(str);
      break;
    case AUTOTYPE:
      if (!pull_string(str, data, len)) return false;
      SetAutoType(str);
      break;
    case PWHIST:
      if (!pull_string(str, data, len)) return false;
      SetPWHistory(str);
      break;
    case RUNCMD:
      if (!pull_string(str, data, len)) return false;
      SetRunCommand(str);
      break;
    case DCA:
      if (!pull_int16(i16, data, len)) return false;
      SetDCA(i16);
      break;
    case EMAIL:
      if (!pull_string(str, data, len)) return false;
      SetEmail(str);
      break;
    case END:
      break;
    default:
      // unknowns!
      SetUnknownField(char(type), len, data);
      break;
  }
  return true;
}

static void push_length(vector<char> &v, unsigned int s)
{
  v.insert(v.end(),
    reinterpret_cast<char *>(&s), reinterpret_cast<char *>(&s) + sizeof(s));
}

static void push_string(vector<char> &v, char type,
                        const StringX &str)
{
  if (!str.empty()) {
    CUTF8Conv utf8conv;
    bool status;
    const unsigned char *utf8;
    int utf8Len;
    status = utf8conv.ToUTF8(str, utf8, utf8Len);
    if (status) {
      v.push_back(type);
      push_length(v, utf8Len);
      v.insert(v.end(), reinterpret_cast<const char *>(utf8), reinterpret_cast<const char *>(utf8) + utf8Len);
    } else
      pws_os::Trace(_T("ItemData::SerializePlainText:ToUTF8(%s) failed\n"),
            str.c_str());
  }
}

static void push_time(vector<char> &v, char type, time_t t)
{
  if (t != 0) {
    __time32_t t32 = static_cast<__time32_t>(t);
    v.push_back(type);
    push_length(v, sizeof(t32));
    v.insert(v.end(),
      reinterpret_cast<char *>(&t32), reinterpret_cast<char *>(&t32) + sizeof(t32));
  }
}

static void push_int(vector<char> &v, char type, int i)
{
  if (i != 0) {
    v.push_back(type);
    push_length(v, sizeof(int));
    v.insert(v.end(),
      reinterpret_cast<char *>(&i), reinterpret_cast<char *>(&i) + sizeof(int));
  }
}

static void push_int16(vector<char> &v, char type, short i)
{
  if (i != 0) {
    v.push_back(type);
    push_length(v, sizeof(short));
    v.insert(v.end(),
      reinterpret_cast<char *>(&i), reinterpret_cast<char *>(&i) + sizeof(short));
  }
}

void CItemData::SerializePlainText(vector<char> &v,
                                   const CItemData *pcibase)  const
{
  StringX tmp;
  uuid_array_t uuid_array;
  time_t t = 0;
  int i32 = 0;
  short i16 = 0;

  v.clear();
  GetUUID(uuid_array);
  v.push_back(UUID);
  push_length(v, sizeof(uuid_array));
  v.insert(v.end(), uuid_array, (uuid_array + sizeof(uuid_array)));
  push_string(v, GROUP, GetGroup());
  push_string(v, TITLE, GetTitle());
  push_string(v, USER, GetUser());

  if (m_entrytype == ET_ALIAS) {
    // I am an alias entry
    ASSERT(pcibase != NULL);
    tmp = _T("[[") + pcibase->GetGroup() + _T(":") + pcibase->GetTitle() + _T(":") + pcibase->GetUser() + _T("]]");
  } else
  if (m_entrytype == ET_SHORTCUT) {
    // I am a shortcut entry
    ASSERT(pcibase != NULL);
    tmp = _T("[~") + pcibase->GetGroup() + _T(":") + pcibase->GetTitle() + _T(":") + pcibase->GetUser() + _T("~]");
  } else
    tmp = GetPassword();

  push_string(v, PASSWORD, tmp);
  push_string(v, NOTES, GetNotes());
  push_string(v, URL, GetURL());
  push_string(v, AUTOTYPE, GetAutoType());

  GetCTime(t);   push_time(v, CTIME, t);
  GetPMTime(t);  push_time(v, PMTIME, t);
  GetATime(t);   push_time(v, ATIME, t);
  GetXTime(t);   push_time(v, XTIME, t);
  GetRMTime(t);  push_time(v, RMTIME, t);

  GetXTimeInt(i32); push_int(v, XTIME_INT, i32);

  push_string(v, POLICY, GetPWPolicy());
  push_string(v, PWHIST, GetPWHistory());

  push_string(v, RUNCMD, GetRunCommand());
  GetDCA(i16);   push_int16(v, DCA, i16);
  push_string(v, EMAIL, GetEmail());

  UnknownFieldsConstIter vi_IterURFE;
  for (vi_IterURFE = GetURFIterBegin();
       vi_IterURFE != GetURFIterEnd();
       vi_IterURFE++) {
    unsigned char type;
    unsigned int length = 0;
    unsigned char *pdata = NULL;
    GetUnknownField(type, length, pdata, vi_IterURFE);
    if (length != 0) {
      v.push_back(static_cast<char>(type));
      push_length(v, length);
      v.insert(v.end(), reinterpret_cast<char *>(pdata), reinterpret_cast<char *>(pdata) + length);
      trashMemory(pdata, length);
    }
    delete[] pdata;
  }

  int end = END; // just to keep the compiler happy...
  v.push_back(static_cast<const char>(end));
  push_length(v, 0);
}
