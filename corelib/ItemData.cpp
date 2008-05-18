/*
* Copyright (c) 2003-2008 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file ItemData.cpp
//-----------------------------------------------------------------------------

#include "ItemData.h"
#include "os/typedefs.h"
#include "BlowFish.h"
#include "TwoFish.h"
#include "PWSrand.h"
#include "UTF8Conv.h"
#include "PWSprefs.h"

#include <time.h>
#include <sstream>
#include <iomanip>

// hide w_char/char differences where possible:
#ifdef UNICODE
typedef std::wistringstream istringstreamT;
typedef std::wostringstream ostringstreamT;
#else
typedef std::istringstream istringstreamT;
typedef std::ostringstream ostringstreamT;
#endif
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool CItemData::IsSessionKeySet = false;
unsigned char CItemData::SessionKey[64];

void CItemData::SetSessionKey()
{
  // must be called once per session, no more, no less
  ASSERT(!IsSessionKeySet);
  PWSrand::GetInstance()->GetRandomData( SessionKey, sizeof( SessionKey ) );
  IsSessionKeySet = true;
}

//-----------------------------------------------------------------------------
// Constructors

CItemData::CItemData()
  : m_Name(NAME), m_Title(TITLE), m_User(USER), m_Password(PASSWORD),
  m_Notes(NOTES), m_UUID(UUID), m_Group(GROUP),
  m_URL(URL), m_AutoType(AUTOTYPE),
  m_tttCTime(CTIME), m_tttPMTime(PMTIME), m_tttATime(ATIME),
  m_tttXTime(XTIME), m_tttRMTime(RMTIME), m_PWHistory(PWHIST),
  m_PWPolicy(POLICY), m_XTimeInterval(XTIME_INT),
  m_display_info(NULL), m_entrytype(ET_NORMAL)
{
  PWSrand::GetInstance()->GetRandomData( m_salt, SaltLength );
}

CItemData::CItemData(const CItemData &that) :
  m_Name(that.m_Name), m_Title(that.m_Title), m_User(that.m_User),
  m_Password(that.m_Password), m_Notes(that.m_Notes), m_UUID(that.m_UUID),
  m_Group(that.m_Group), m_URL(that.m_URL), m_AutoType(that.m_AutoType),
  m_tttCTime(that.m_tttCTime), m_tttPMTime(that.m_tttPMTime), m_tttATime(that.m_tttATime),
  m_tttXTime(that.m_tttXTime), m_tttRMTime(that.m_tttRMTime), m_PWHistory(that.m_PWHistory),
  m_PWPolicy(that.m_PWPolicy), m_XTimeInterval(that.m_XTimeInterval),
  m_display_info(that.m_display_info), m_entrytype(that.m_entrytype)
{
  memcpy((char*)m_salt, (char*)that.m_salt, SaltLength);
  if (!that.m_URFL.empty())
    m_URFL = that.m_URFL;
  else
    m_URFL.clear();
}

CItemData::~CItemData()
{
  if (!m_URFL.empty()) {
    m_URFL.clear();
  }
}

//-----------------------------------------------------------------------------
// Accessors

CMyString CItemData::GetField(const CItemField &field) const
{
  CMyString retval;
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

CMyString CItemData::GetName() const
{
  return GetField(m_Name);
}

CMyString CItemData::GetTitle() const
{
  return GetField(m_Title);
}

CMyString CItemData::GetUser() const
{
  return GetField(m_User);
}

CMyString CItemData::GetPassword() const
{
  return GetField(m_Password);
}

CMyString CItemData::GetNotes(TCHAR delimiter) const
{
  CMyString ret = GetField(m_Notes);
  if (delimiter != 0) {
    ret.Remove(TCHAR('\r'));
    ret.Replace(TCHAR('\n'), delimiter);
  }
  return ret;
}

CMyString CItemData::GetGroup() const
{
  return GetField(m_Group);
}

CMyString CItemData::GetURL() const
{
  return GetField(m_URL);
}

CMyString CItemData::GetAutoType() const
{
  return GetField(m_AutoType);
}

CMyString CItemData::GetTime(int whichtime, int result_format) const
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
      GetField(m_tttATime, (unsigned char *)in, tlen);
      break;
    case CTIME:
      GetField(m_tttCTime, (unsigned char *)in, tlen);
      break;
    case XTIME:
      GetField(m_tttXTime, (unsigned char *)in, tlen);
      break;
    case PMTIME:
      GetField(m_tttPMTime, (unsigned char *)in, tlen);
      break;
    case RMTIME:
      GetField(m_tttRMTime, (unsigned char *)in, tlen);
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
  GetField(m_UUID, (unsigned char *)uuid_array, length);
}

static void String2PWPolicy(const stringT &cs_pwp, PWPolicy &pwp)
{
  // should really be a c'tor of PWPolicy - later...

  // We need flags(4), length(3), lower_len(3), upper_len(3)
  //   digit_len(3), symbol_len(3) = 4 + 5 * 3 = 19 
  // Note: order of fields set by PWSprefs enum that can have minimum lengths.
  // Later releases must support these as a minimum.  Any fields added
  // by these releases will be lost if the user changes these field.
  ASSERT(cs_pwp.length() == 19);
  istringstreamT is_flags(stringT(cs_pwp, 0, 4));
  istringstreamT is_length(stringT(cs_pwp, 4, 3));
  istringstreamT is_digitminlength(stringT(cs_pwp, 7, 3));
  istringstreamT is_lowreminlength(stringT(cs_pwp, 10, 3));
  istringstreamT is_symbolminlength(stringT(cs_pwp, 13, 3));
  istringstreamT is_upperminlength(stringT(cs_pwp, 16, 3));
  unsigned int f; // dain bramaged istringstream requires this runaround
  is_flags >> hex >> f;
  pwp.flags = static_cast<WORD>(f);
  is_length >> hex >> pwp.length;
  is_digitminlength >> hex >> pwp.digitminlength;
  is_lowreminlength >> hex >> pwp.lowerminlength;
  is_symbolminlength >> hex >> pwp.symbolminlength;
  is_upperminlength >> hex >> pwp.upperminlength;
}

void CItemData::GetPWPolicy(PWPolicy &pwp) const
{
  stringT cs_pwp(GetField(m_PWPolicy));

  int len = cs_pwp.length();
  pwp.flags = 0;
  if (len == 19)
    String2PWPolicy(cs_pwp, pwp);
}

CMyString CItemData::GetPWPolicy() const
{
  return GetField(m_PWPolicy);
}

void CItemData::GetXTimeInt(int &xint) const
{
  unsigned char in[TwoFish::BLOCKSIZE]; // required by GetField
  unsigned int tlen = sizeof(in); // ditto

  GetField(m_XTimeInterval, (unsigned char *)in, tlen);

  if (tlen != 0) {
    ASSERT(tlen == sizeof(int));
    memcpy(&xint, in, sizeof(int));
  } else {
    xint = 0;
  }
}

CMyString CItemData::GetXTimeInt() const
{
  int xint;
  GetXTimeInt(xint);
  if (xint == 0)
    return CMyString(_T(""));

  ostringstreamT os;
  os << xint << ends;
  return CMyString(os.str().c_str());
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

void CItemData::SetUnknownField(const unsigned char type,
                                const unsigned int length,
                                const unsigned char * ufield)
{
  CItemField unkrfe(type);
  SetField(unkrfe, ufield, length);
  m_URFL.push_back(unkrfe);
}

/*
* Password History (PWH):
* Password history is represented in the entry record as a textual field
* with the following semantics:
*
* Password History Header: 
* %01x - status for saving PWH for this entry (0 = no; 1 = yes) 
* %02x - maximum number of entries in this entry 
* %02x - number of entries currently saved 
*
* Each Password History Entry: 
* %08x - time of this old password was set (time_t) 
* %04x - length of old password (in TCHAR)
* %s   - old password 
*
* No history being kept for a record can be represented either by the lack
* of the PWH field (preferred), or by a header of _T("00000"):
* status = 0, max = 00, num = 00 
*
* Note that 0aabb where bb <= aa is possible if password history was enabled in the past
* but has been disabled and the history hasn't been cleared.
*
*/

CMyString CItemData::GetPWHistory() const
{
  CMyString ret = GetField(m_PWHistory);
  if (ret == _T("0") || ret == _T("00000"))
    ret = _T("");
  return ret;
}

CMyString CItemData::GetPlaintext(const TCHAR &separator,
                                  const FieldBits &bsFields,
                                  const TCHAR &delimiter,
                                  const CItemData *cibase) const
{
  CMyString ret(_T(""));

  CMyString grouptitle;
  const CMyString title(GetTitle());
  const CMyString group(GetGroup());
  const CMyString user(GetUser());
  const CMyString url(GetURL());
  const CMyString notes(GetNotes(delimiter));

  // a '.' in title gets Import confused re: Groups
  grouptitle = title;
  if (grouptitle.Find(TCHAR('.')) != -1) {
    if (delimiter != 0) {
      grouptitle.Replace(TCHAR('.'), delimiter);
    } else {
      grouptitle = TCHAR('\"') + title + TCHAR('\"');
    }
  }

  if (!group.IsEmpty())
    grouptitle = group + TCHAR('.') + grouptitle;

  CMyString history(_T(""));
  if (bsFields.test(CItemData::PWHIST)) {
    // History exported as "00000" if empty, to make parsing easier
    BOOL pwh_status;
    size_t pwh_max, pwh_num;
    PWHistList PWHistList;

    CreatePWHistoryList(pwh_status, pwh_max, pwh_num,
                        &PWHistList, TMC_EXPORT_IMPORT);

    //  Build export string
    ostringstreamT os;
    os.fill(charT('0'));
    os << hex << setw(1) << pwh_status
       << setw(2) << pwh_max << setw(2) << pwh_num << ends;
    history = CMyString(os.str().c_str());
    PWHistList::iterator iter;
    for (iter = PWHistList.begin(); iter != PWHistList.end(); iter++) {
      const PWHistEntry pwshe = *iter;
      history += _T(' ');
      history += pwshe.changedate;
      ostringstreamT os1;
      os1 << hex << charT(' ') << setfill(charT('0')) << setw(4)
          << pwshe.password.GetLength() << charT(' ') << ends;
      history += CMyString(os1.str().c_str());
      history += pwshe.password;
    }
  }

  CMyString csPassword;
  if (m_entrytype == ET_ALIAS) {
    ASSERT(cibase != NULL);
    csPassword = _T("[[") + 
                 cibase->GetGroup() + _T(":") + 
                 cibase->GetTitle() + _T(":") + 
                 cibase->GetUser() + _T("]]") ;
  } else if (m_entrytype == ET_SHORTCUT) {
    ASSERT(cibase != NULL);
    csPassword = _T("[~") + 
                 cibase->GetGroup() + _T(":") + 
                 cibase->GetTitle() + _T(":") + 
                 cibase->GetUser() + _T("~]") ;
  } else
    csPassword = GetPassword();

  // Notes field must be last, for ease of parsing import
  if (bsFields.count() == bsFields.size()) {
    // Everything - note can't actually set all bits via dialog!
    ret = grouptitle + separator + 
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
          _T("\"") + notes + _T("\"");
  } else {
    // Not everything
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
    if (bsFields.test(CItemData::NOTES))
      ret += _T("\"") + notes + _T("\"");
    // remove trailing separator
    if ((CString)ret.Right(1) == separator) {
      int rl = ret.GetLength();
      ret.Left(rl - 1);
    }
  }

  return ret;
}

static string GetXMLTime(int indent, const char *name,
                         time_t t, CUTF8Conv &utf8conv)
{
  int i;
  const CMyString tmp = PWSUtil::ConvertToDateTimeString(t, TMC_XML);
  ostringstream oss;
  const unsigned char *utf8 = NULL;
  int utf8Len = 0;


  for (i = 0; i < indent; i++) oss << "\t";
  oss << "<" << name << ">" << endl;
  for (i = 0; i <= indent; i++) oss << "\t";
  utf8conv.ToUTF8(tmp.Left(10), utf8, utf8Len);
  oss << "<date>";
  oss.write(reinterpret_cast<const char *>(utf8), utf8Len);
  oss << "</date>" << endl;
  for (i = 0; i <= indent; i++) oss << "\t";
  utf8conv.ToUTF8(tmp.Right(8), utf8, utf8Len);
  oss << "<time>";
  oss.write(reinterpret_cast<const char *>(utf8), utf8Len);
  oss << "</time>" << endl;
  for (i = 0; i < indent; i++) oss << "\t";
  oss << "</" << name << ">" << endl;
  return oss.str();
}

static void WriteXMLField(ostream &os, const char *fname,
                          const CMyString &value, CUTF8Conv &utf8conv,
                          const char *tabs = "\t\t")
{
  const unsigned char * utf8 = NULL;
  int utf8Len = 0;
  int p = value.Find(_T("]]>")); // special handling required
  if (p == -1) {
    // common case
    os << tabs << "<" << fname << "><![CDATA[";
    if(utf8conv.ToUTF8(value, utf8, utf8Len))
      os.write(reinterpret_cast<const char *>(utf8), utf8Len);
    else
      os << "Internal error - unable to convert field to utf-8";
    os << "]]></" << fname << ">" << endl;
  } else {
    // value has "]]>" sequence(s) that need(s) to be escaped
    // Each "]]>" splits the field into two CDATA sections, one ending with
    // ']]', the other starting with '>'
    os << tabs << "<" << fname << ">";
    int from = 0, to = p + 2;
    do {
      CMyString slice = value.Mid(from, (to - from));
      os << "<![CDATA[";
      if(utf8conv.ToUTF8(slice, utf8, utf8Len))
        os.write(reinterpret_cast<const char *>(utf8), utf8Len);
      else
        os << "Internal error - unable to convert field to utf-8";
      os << "]]><![CDATA[";
      from = to;
      p = value.Find(_T("]]>"), from); // are there more?
      if (p == -1) {
        to = value.GetLength();
        slice = value.Mid(from, (to - from));
      } else {
        to = p + 2;
        slice = value.Mid(from, (to - from));
        from = to;
        to = value.GetLength();
      }
      if(utf8conv.ToUTF8(slice, utf8, utf8Len))
        os.write(reinterpret_cast<const char *>(utf8), utf8Len);
      else
        os << "Internal error - unable to convert field to utf-8";
      os << "]]>";
    } while (p != -1);
    os << "</" << fname << ">" << endl;
  } // special handling of "]]>" in value.
}

string CItemData::GetXML(unsigned id, const FieldBits &bsExport,
                         TCHAR delimiter, const CItemData *cibase) const
{
  ostringstream oss; // ALWAYS a string of chars, never wchar_t!
  oss << "\t<entry id=\"" << id << "\">" << endl;

  CMyString tmp;
  CUTF8Conv utf8conv;

  tmp = GetGroup();
  if (bsExport.test(CItemData::GROUP) && !tmp.IsEmpty())
    WriteXMLField(oss, "group", tmp, utf8conv);

  // Title mandatory (see pwsafe.xsd)
  WriteXMLField(oss, "title", GetTitle(), utf8conv);

  tmp = GetUser();
  if (bsExport.test(CItemData::USER) && !tmp.IsEmpty())
    WriteXMLField(oss, "username", tmp, utf8conv);

  // Password mandatory (see pwsafe.xsd)
  if (m_entrytype == ET_ALIAS) {
    ASSERT(cibase != NULL);
    tmp = _T("[[") + 
          cibase->GetGroup() + _T(":") + 
          cibase->GetTitle() + _T(":") + 
          cibase->GetUser() + _T("]]") ;
  } else
  if (m_entrytype == ET_SHORTCUT) {
    ASSERT(cibase != NULL);
    tmp = _T("[~") + 
          cibase->GetGroup() + _T(":") + 
          cibase->GetTitle() + _T(":") + 
          cibase->GetUser() + _T("~]") ;
  } else
    tmp = GetPassword();
  WriteXMLField(oss, "password", tmp, utf8conv);

  tmp = GetURL();
  if (bsExport.test(CItemData::URL) && !tmp.IsEmpty())
    WriteXMLField(oss, "url", tmp, utf8conv);

  tmp = GetAutoType();
  if (bsExport.test(CItemData::AUTOTYPE) && !tmp.IsEmpty())
    WriteXMLField(oss, "autotype", tmp, utf8conv);

  tmp = GetNotes();
  if (bsExport.test(CItemData::NOTES) && !tmp.IsEmpty()) {
    tmp.Remove(_T('\r'));
    tmp.Replace(_T('\n'), delimiter);
    WriteXMLField(oss, "notes", tmp, utf8conv);
  }

  uuid_array_t uuid_array;
  GetUUID(uuid_array);
  uuid_str_NH_t uuid_buffer;
  CUUIDGen::GetUUIDStr(uuid_array, uuid_buffer);
  oss << "\t\t<uuid><![CDATA[" << uuid_buffer << "]]></uuid>" << endl;

  time_t t;
  int xint;

  GetCTime(t);
  if (bsExport.test(CItemData::CTIME) && (long)t != 0L)
    oss << GetXMLTime(2, "ctime", t, utf8conv);

  GetATime(t);
  if (bsExport.test(CItemData::ATIME) && (long)t != 0L)
    oss << GetXMLTime(2, "atime", t, utf8conv);

  GetXTime(t);
  if (bsExport.test(CItemData::XTIME) && (long)t != 0L)
    oss << GetXMLTime(2, "xtime", t, utf8conv);

  GetXTimeInt(xint);
  if (bsExport.test(CItemData::XTIME_INT) && xint > 0 && xint <= 3650)
    oss << "\t\t<xtime_interval>" << xint << "</xtime_interval>" << endl;

  GetPMTime(t);
  if (bsExport.test(CItemData::PMTIME) && (long)t != 0L)
    oss << GetXMLTime(2, "pmtime", t, utf8conv);

  GetRMTime(t);
  if (bsExport.test(CItemData::RMTIME) && (long)t != 0L)
    oss << GetXMLTime(2, "rmtime", t, utf8conv);

  PWPolicy pwp;
  GetPWPolicy(pwp);
  if (bsExport.test(CItemData::POLICY) && pwp.flags != 0) {
    oss << "\t\t<PasswordPolicy>" << endl;
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
    BOOL pwh_status;
    size_t pwh_max, pwh_num;
    PWHistList PWHistList;
    CreatePWHistoryList(pwh_status, pwh_max, pwh_num,
      &PWHistList, TMC_XML);
    if (pwh_status == TRUE || pwh_max > 0 || pwh_num > 0) {
      oss << "\t\t<pwhistory>" << endl;
      oss << "\t\t\t<status>" << pwh_status << "</status>" << endl;
      oss << "\t\t\t<max>" << pwh_max << "</max>" << endl;
      oss << "\t\t\t<num>" << pwh_num << "</num>" << endl;
      if (!PWHistList.empty()) {
        oss << "\t\t\t<history_entries>" << endl;
        int num = 1;
        PWHistList::iterator hiter;
        for (hiter = PWHistList.begin(); hiter != PWHistList.end();
             hiter++) {
          const unsigned char * utf8 = NULL;
          int utf8Len = 0;

          oss << "\t\t\t\t<history_entry num=\"" << num << "\">" << endl;
          const PWHistEntry pwshe = *hiter;
          oss << "\t\t\t\t\t<changed>" << endl;
          oss << "\t\t\t\t\t\t<date>";
          if (utf8conv.ToUTF8(pwshe.changedate.Left(10), utf8, utf8Len))
            oss.write(reinterpret_cast<const char *>(utf8), utf8Len);
          oss << "</date>" << endl;
          oss << "\t\t\t\t\t\t<time>";
          if (utf8conv.ToUTF8(pwshe.changedate.Right(8), utf8, utf8Len))
            oss.write(reinterpret_cast<const char *>(utf8), utf8Len);
          oss << "</time>" << endl;
          oss << "\t\t\t\t\t</changed>" << endl;
          WriteXMLField(oss, "oldpassword", pwshe.password,
                        utf8conv, "\t\t\t\t\t");
          oss << "\t\t\t\t</history_entry>" << endl;

          num++;
        } // for
        oss << "\t\t\t</history_entries>" << endl;
      } // if !empty
      oss << "\t\t</pwhistory>" << endl;
    }
  }

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
      tmp = (CMyString)PWSUtil::Base64Encode(pdata, length);
#else
      tmp.Empty();
      unsigned char * pdata2(pdata);
      unsigned char c;
      for (int j = 0; j < (int)length; j++) {
        c = *pdata2++;
        cs_tmp.Format(_T("%02x"), c);
        tmp += CMyString(cs_tmp);
      }
#endif
      oss << "\t\t\t<field ftype=\"" << int(type) << "\">" <<  LPCTSTR(tmp) << "</field>" << endl;
      trashMemory(pdata, length);
      delete[] pdata;
    } // iteration over unknown fields
    oss << "\t\t</unknownrecordfields>" << endl;  
  } // if there are unknown fields

  oss << "\t</entry>" << endl << endl;
  return oss.str();
}

void CItemData::SplitName(const CMyString &name,
                          CMyString &title, CMyString &username)
{
  int pos = name.Find(SPLTCHR);
  if (pos==-1) {//Not a split name
    int pos2 = name.Find(DEFUSERCHR);
    if (pos2 == -1)  {//Make certain that you remove the DEFUSERCHR
      title = name;
    } else {
      title = CMyString(name.Left(pos2));
    }
  } else {
    /*
    * There should never ever be both a SPLITCHR and a DEFUSERCHR in
    * the same string
    */
    CMyString temp;
    temp = CMyString(name.Left(pos));
    temp.TrimRight();
    title = temp;
    temp = CMyString(name.Right(name.GetLength() - (pos+1))); // Zero-index string
    temp.TrimLeft();
    username = temp;
  }
}

//-----------------------------------------------------------------------------
// Setters

void CItemData::SetField(CItemField &field, const CMyString &value)
{
  BlowFish *bf = MakeBlowFish();
  field.Set(value, bf);
  delete bf;
}

void CItemData::SetField(CItemField &field, const unsigned char *value, unsigned int length)
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

void CItemData::SetName(const CMyString &name, const CMyString &defaultUsername)
{
  // the m_name is from pre-2.0 versions, and may contain the title and user
  // separated by SPLTCHR. Also, DEFUSERCHR signified that the default username is to be used.
  // Here we fill the title and user fields so that
  // the application can ignore this difference after an ItemData record
  // has been created
  CMyString title, user;
  int pos = name.Find(DEFUSERCHR);
  if (pos != -1) {
    title = CMyString(name.Left(pos));
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

void CItemData::SetTitle(const CMyString &title, TCHAR delimiter)
{
  if (delimiter == 0)
    SetField(m_Title, title);
  else {
    CMyString new_title(_T(""));
    CMyString newCString, tmpCString;
    int pos = 0;

    newCString = title;
    do {
      pos = newCString.Find(delimiter);
      if ( pos != -1 ) {
        new_title += CMyString(newCString.Left(pos)) + _T(".");

        tmpCString = CMyString(newCString.Mid(pos + 1));
        newCString = tmpCString;
      }
    } while ( pos != -1 );

    if (!newCString.IsEmpty())
      new_title += newCString;

    SetField(m_Title, new_title);
  }
}

void CItemData::SetUser(const CMyString &user)
{
  SetField(m_User, user);
}

void CItemData::SetPassword(const CMyString &password)
{
  SetField(m_Password, password);
}

void CItemData::SetNotes(const CMyString &notes, TCHAR delimiter)
{
  if (delimiter == 0)
    SetField(m_Notes, notes);
  else {
    const CMyString CRLF = _T("\r\n");
    CMyString multiline_notes(_T(""));

    CMyString newCString;
    CMyString tmpCString;

    int pos = 0;

    newCString = notes;
    do {
      pos = newCString.Find(delimiter);
      if ( pos != -1 ) {
        multiline_notes += CMyString(newCString.Left(pos)) + CRLF;

        tmpCString = CMyString(newCString.Mid(pos + 1));
        newCString = tmpCString;
      }
    } while ( pos != -1 );

    if (!newCString.IsEmpty())
      multiline_notes += newCString;

    SetField(m_Notes, multiline_notes);
  }
}

void CItemData::SetGroup(const CMyString &title)
{
  SetField(m_Group, title);
}

void CItemData::SetUUID(const uuid_array_t &UUID)
{
  SetField(m_UUID, (const unsigned char *)UUID, sizeof(UUID));
}

void CItemData::SetURL(const CMyString &URL)
{
  SetField(m_URL, URL);
}

void CItemData::SetAutoType(const CMyString &autotype)
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
  int t32 = (int)t;
  switch (whichtime) {
    case ATIME:
      SetField(m_tttATime, (const unsigned char *)&t32, sizeof(t32));
      break;
    case CTIME:
      SetField(m_tttCTime, (const unsigned char *)&t32, sizeof(t32));
      break;
    case XTIME:
      SetField(m_tttXTime, (const unsigned char *)&t32, sizeof(t32));
      break;
    case PMTIME:
      SetField(m_tttPMTime, (const unsigned char *)&t32, sizeof(t32));
      break;
    case RMTIME:
      SetField(m_tttRMTime, (const unsigned char *)&t32, sizeof(t32));
      break;
    default:
      ASSERT(0);
  }
}

bool CItemData::SetTime(int whichtime, const CString &time_str)
{
  time_t t(0);

  if (time_str.IsEmpty()) {
    SetTime(whichtime, t);
    return true;
  } else
  if (time_str == _T("now")) {
    time(&t);
    SetTime(whichtime, t);
    return true;
  } else
  if ((PWSUtil::VerifyImportDateTimeString(time_str, t) ||
    PWSUtil::VerifyXMLDateTimeString(time_str, t) ||
    PWSUtil::VerifyASCDateTimeString(time_str, t)) &&
    (t != (time_t)-1)  // checkerror despite all our verification!
    ) {
    SetTime(whichtime, t);
    return true;
  }
  return false;
}

void CItemData::SetXTimeInt(int &xint)
{
   SetField(m_XTimeInterval, (const unsigned char *)&xint, sizeof(int));
}

bool CItemData::SetXTimeInt(const CString &xint_str)
{
  int xint(0);

  if (xint_str.IsEmpty()) {
    SetXTimeInt(xint);
    return true;
  }

  if (xint_str.SpanIncluding(CString(_T("0123456789"))) == xint_str) {
    xint = _ttoi(xint_str);
    if (xint >= 0 && xint <= 3650) {
      SetXTimeInt(xint);
      return true;
    }
  }
  return false;
}

void CItemData::SetPWHistory(const CMyString &PWHistory)
{
  CMyString pwh = PWHistory;
  if (pwh == _T("0") || pwh == _T("00000"))
    pwh = _T("");
  SetField(m_PWHistory, pwh);
}

int CItemData::CreatePWHistoryList(BOOL &status,
                                   size_t &pwh_max, size_t &pwh_num,
                                   PWHistList* pPWHistList,
                                   const int time_format) const
{
  status = FALSE;
  pwh_max = pwh_num = 0;

  stringT pwh_s = GetPWHistory();
  int len = pwh_s.length();

  if (len < 5)
    return (len != 0 ? 1 : 0);

  BOOL s = pwh_s[0] == charT('0') ? FALSE : TRUE;

  int m, n;
  istringstreamT ism(stringT(pwh_s, 1, 2)); // max history 1 byte hex
  istringstreamT isn(stringT(pwh_s, 3, 2)); // cur # entries 1 byte hex
  ism >> hex >> m;
  if (!ism) return 1;
  isn >> hex >> n;
  if (!isn) return 1;

  int offset = 1 + 2 + 2; // where to extract the next token from pwh_s
  int i_error = 0;

  for (int i = 0; i < n; i++) {
    PWHistEntry pwh_ent;
    long t;
    istringstreamT ist(stringT(pwh_s, offset, 8)); // time in 4 byte hex
    ist >> hex >> t;
    if (!ist) {i_error++; continue;} // continue or break?
    offset += 8;

    pwh_ent.changetttdate = (time_t) t;
    pwh_ent.changedate =
      PWSUtil::ConvertToDateTimeString((time_t) t, time_format);
    if (pwh_ent.changedate.IsEmpty()) {
      //                       1234567890123456789
      pwh_ent.changedate = _T("1970-01-01 00:00:00");
    }
    istringstreamT ispwlen(stringT(pwh_s, offset, 4)); // pw length 2 byte hex
    int ipwlen;
    ispwlen >> hex >> ipwlen;
    if (!ispwlen) {i_error++; continue;} // continue or break?
    offset += 4;
    const stringT pw(pwh_s, offset, ipwlen);
    pwh_ent.password = pw.c_str();
    offset += ipwlen;
    pPWHistList->push_back(pwh_ent);
  }

  status = s;
  pwh_max = m;
  pwh_num = n;
  return i_error;
}

void CItemData::SetPWPolicy(const PWPolicy &pwp)
{
  // Must be some flags; however hex incompatible with other flags
  bool bhex_flag = (pwp.flags & PWSprefs::PWPolicyUseHexDigits) != 0;
  bool bother_flags = (pwp.flags & (~PWSprefs::PWPolicyUseHexDigits)) != 0;

  CMyString cs_pwp;
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
       << setw(3) << pwp.upperminlength << ends;
    cs_pwp = os.str().c_str();
  }
  SetField(m_PWPolicy, cs_pwp);
}

bool CItemData::SetPWPolicy(const CString &cs_pwp)
{
  // Basic sanity checks
  if (cs_pwp.IsEmpty()) {
    SetField(m_PWPolicy, cs_pwp);
    return true;
  }
  if (cs_pwp.GetLength() < 19)
    return false;

  // Parse policy string, more sanity checks
  // See String2PWPolicy for valid format
  PWPolicy pwp;
  String2PWPolicy(stringT(cs_pwp), pwp);
  CString cs_pwpolicy(cs_pwp);

  // Must be some flags; however hex incompatible with other flags
  bool bhex_flag = (pwp.flags & PWSprefs::PWPolicyUseHexDigits) != 0;
  bool bother_flags = (pwp.flags & (~PWSprefs::PWPolicyUseHexDigits)) != 0;
  const int total_sublength = pwp.digitminlength + pwp.lowerminlength +
    pwp.symbolminlength + pwp.upperminlength;

  if (pwp.flags == 0 || (bhex_flag && bother_flags) ||
      pwp.length > 1024 || total_sublength > pwp.length ||
      pwp.digitminlength > 1024 || pwp.lowerminlength > 1024 ||
      pwp.symbolminlength > 1024 || pwp.upperminlength > 1024) {
    cs_pwpolicy.Empty();
  }

  SetField(m_PWPolicy, cs_pwpolicy);
  return true;
}

BlowFish *CItemData::MakeBlowFish() const
{
  ASSERT(IsSessionKeySet);
  return BlowFish::MakeBlowFish(SessionKey, sizeof(SessionKey),
    m_salt, SaltLength);
}

CItemData& CItemData::operator=(const CItemData &that)
{
  //Check for self-assignment
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
    m_tttCTime = that.m_tttCTime;
    m_tttPMTime = that.m_tttPMTime;
    m_tttATime = that.m_tttATime;
    m_tttXTime = that.m_tttXTime;
    m_tttRMTime = that.m_tttRMTime;
    m_PWHistory = that.m_PWHistory;
    m_PWPolicy = that.m_PWPolicy;
    m_XTimeInterval = that.m_XTimeInterval;
    m_display_info = that.m_display_info;
    if (!that.m_URFL.empty())
      m_URFL = that.m_URFL;
    else
      m_URFL.clear();
    m_entrytype = that.m_entrytype;      
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

int CItemData::ValidatePWHistory()
{
  if (m_PWHistory.IsEmpty())
    return 0;

  CMyString pwh = GetPWHistory();
  if (pwh.GetLength() < 5) {
    SetPWHistory(_T(""));
    return 1;
  }

  BOOL pwh_status;
  size_t pwh_max, pwh_num;
  PWHistList PWHistList;
  int iResult = CreatePWHistoryList(pwh_status, pwh_max,
                                    pwh_num, &PWHistList, TMC_EXPORT_IMPORT);
  if (iResult == 0) {
    return 0;
  }

  size_t listnum = PWHistList.size();
  if (listnum > pwh_num)
    pwh_num = listnum;

  if (pwh_max == 0 && pwh_num == 0) {
    SetPWHistory(_T(""));
    return 1;
  }

  if (listnum > pwh_max)
    pwh_max = listnum;

  pwh_num = listnum;

  if (pwh_max < pwh_num)
    pwh_max = pwh_num;

  // Rebuild PWHistory from the data we have
  CMyString history;
  ostringstreamT os;
  os.fill(charT('0'));
  os << hex << setw(1) << pwh_status
     << setw(2) << pwh_max << setw(2) << pwh_num << ends;
  history = CMyString(os.str().c_str());

  PWHistList::iterator iter;
  for (iter = PWHistList.begin(); iter != PWHistList.end(); iter++) {
    PWHistEntry pwshe = *iter;
    history += _T(' ');
    history += pwshe.changedate;
    ostringstreamT os1;
    os1 << hex << charT(' ') << setfill(charT('0')) << setw(4)
        << pwshe.password.GetLength() << charT(' ') << ends;
    history += CMyString(os1.str().c_str());
    history += pwshe.password;
  }
  SetPWHistory(history);

  return 1;
}

bool CItemData::Matches(const CString &string1, const int &iObject,
                        const int &iFunction) const
{
  ASSERT(iFunction != 0); // must be positive or negative!

  CMyString csObject;
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
    case AUTOTYPE:
      csObject = GetAutoType();
      break;
    default:
      ASSERT(0);
  }

  if (iFunction == PWSUtil::MR_PRESENT || iFunction == PWSUtil::MR_NOTPRESENT) {
    const bool bValue = !csObject.IsEmpty();
    return PWSUtil::MatchesBool(bValue, iFunction);
  } else
    return PWSUtil::MatchesString(string1, csObject, iFunction);
}

bool CItemData::Matches(const int &num1, const int &num2, const int &iObject,
                        const int &iFunction) const
{
  //   Check integer values are selected
  int iValue;

  switch (iObject) {
    case XTIME_INT:
      GetXTimeInt(iValue);
      break;
    default:
      ASSERT(0);
      return false;
  }

  if (iFunction == PWSUtil::MR_PRESENT || iFunction == PWSUtil::MR_NOTPRESENT) {
    const bool bValue = (iValue == 0);
    return PWSUtil::MatchesBool(bValue, iFunction);
  } else
    return PWSUtil::MatchesInteger(num1, num2, iValue, iFunction);
}

bool CItemData::Matches(const time_t &time1, const time_t &time2, const int &iObject,
                        const int &iFunction) const
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

  if (iFunction == PWSUtil::MR_PRESENT || iFunction == PWSUtil::MR_NOTPRESENT) {
    const bool bValue = (tValue != (time_t)0);
    return PWSUtil::MatchesBool(bValue, iFunction);
  } else
    return PWSUtil::MatchesDateTime(time1, time2, tValue, iFunction);
}
  
bool CItemData::Matches(const EntryType &etype1,
                        const int &iFunction) const
{
  switch (iFunction) {
    case PWSUtil::MR_EQUALS:
      return GetEntryType() == etype1;
    case PWSUtil::MR_NOTEQUAL:
      return GetEntryType() != etype1;
    default:
      ASSERT(0);
  }
  return false;
}

static bool pull_string(CMyString &str, unsigned char *data, int len)
{
  CUTF8Conv utf8conv;
  vector<unsigned char> v(data, (data + len));
  v.push_back(0); // null terminate for FromUTF8.
  bool utf8status = utf8conv.FromUTF8((unsigned char *)&v[0],
    len, str);
  if (!utf8status) {
    TRACE(_T("CItemData::DeserializePlainText(): FromUTF8 failed!\n"));
  }
  trashMemory(&v[0], len);
  return utf8status;
}

static bool pull_time(time_t &t, unsigned char *data, size_t len)
{
  if (len == sizeof(__time32_t)) {
    t = *reinterpret_cast<__time32_t *>(data);
  } else if (len == sizeof(__time64_t)) {
    // convert from 64 bit time to currently supported 32 bit
    struct tm ts;
    __time64_t *t64 = reinterpret_cast<__time64_t *>(data);
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

static bool pull_int(int &i, unsigned char *data, size_t len)
{
  if (len == sizeof(int)) {
    i = *reinterpret_cast<int *>(data);
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
    if ((v.end() - iter) < sizeof(size_t)) {
      ASSERT(0); // type must ALWAYS be followed by length
      return false;
    }

    if (type == END)
      return true; // happy end

    unsigned int len = *((unsigned int *)&(*iter));
    ASSERT(len < v.size()); // sanity check
    iter += sizeof(unsigned int);

    if (--emergencyExit == 0) {
      ASSERT(0);
      return false;
    }
    if (!SetField(type, (unsigned char *)&(*iter), len))
      return false;
    iter += len;
  }
  return false; // END tag not found!
}

bool CItemData::SetField(int type, unsigned char *data, int len)
{
  CMyString str;
  time_t t;
  int xint;
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
      if (!pull_int(xint, data, len)) return false;
      SetXTimeInt(xint);
      break;
    case POLICY:
      if (!pull_string(str, data, len)) return false;
      SetPWPolicy(str);
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
    (char *)&s, (char *)&s + sizeof(s));
}

static void push_string(vector<char> &v, char type,
                        const CMyString &str)
{
  if (!str.IsEmpty()) {
    CUTF8Conv utf8conv;
    bool status;
    const unsigned char *utf8;
    int utf8Len;
    status = utf8conv.ToUTF8(str, utf8, utf8Len);
    if (status) {
      v.push_back(type);
      push_length(v, utf8Len);
      v.insert(v.end(), (char *)utf8, (char *)utf8 + utf8Len);
    } else
      TRACE(_T("ItemData::SerializePlainText:ToUTF8(%s) failed\n"), str);
  }
}

static void push_time(vector<char> &v, char type, time_t t)
{
  if (t != 0) {
    __time32_t t32 = (__time32_t)t;
    v.push_back(type);
    push_length(v, sizeof(t32));
    v.insert(v.end(),
      (char *)&t32, (char *)&t32 + sizeof(t32));
  }
}

void CItemData::SerializePlainText(vector<char> &v, CItemData *cibase)  const
{
  CMyString tmp;
  uuid_array_t uuid_array;
  time_t t = 0;

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
    ASSERT(cibase != NULL);
    tmp = _T("[[") + cibase->GetGroup() + _T(":") + cibase->GetTitle() + _T(":") + cibase->GetUser() + _T("]]");
  } else
  if (m_entrytype == ET_SHORTCUT) {
    // I am a shortcut entry
    ASSERT(cibase != NULL);
    tmp = _T("[~") + cibase->GetGroup() + _T(":") + cibase->GetTitle() + _T(":") + cibase->GetUser() + _T("~]");
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

  push_string(v, POLICY, GetPWPolicy());
  push_string(v, PWHIST, GetPWHistory());

  UnknownFieldsConstIter vi_IterURFE;
  for (vi_IterURFE = GetURFIterBegin();
       vi_IterURFE != GetURFIterEnd();
       vi_IterURFE++) {
    unsigned char type;
    unsigned int length = 0;
    unsigned char *pdata = NULL;
    GetUnknownField(type, length, pdata, vi_IterURFE);
    if (length != 0) {
      v.push_back((char)type);
      push_length(v, length);
      v.insert(v.end(), (char *)pdata, (char *)pdata + length);
      trashMemory(pdata, length);
    }
    delete[] pdata;
  }

  int end = END; // just to keep the compiler happy...
  v.push_back(static_cast<const char>(end));
  push_length(v, 0);
}
