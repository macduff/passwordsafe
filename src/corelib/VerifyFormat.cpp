/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "VerifyFormat.h"
#include "corelib.h"
#include "StringXStream.h"

bool verifyDTvalues(int yyyy, int mon, int dd,
                    int hh, int min, int ss)
{
  const int month_lengths[12] = {31, 28, 31, 30, 31, 30,
                                 31, 31, 30, 31, 30, 31};
  // Built-in obsolesence for pwsafe in 2038?
  if (yyyy < 1970 || yyyy > 2038)
    return false;

  if ((mon < 1 || mon > 12) || (dd < 1))
    return false;

  if (mon == 2 && (yyyy % 4) == 0) {
    // Feb and a leap year
    if (dd > 29)
      return false;
  } else {
    // Either (Not Feb) or (Is Feb but not a leap-year)
    if (dd > month_lengths[mon - 1])
      return false;
  }

  if ((hh < 0 || hh > 23) ||
      (min < 0 || min > 59) ||
      (ss < 0 || ss > 59))
    return false;

  return true;
}


bool VerifyImportDateTimeString(const wstring &time_str, time_t &t)
{
  //  String format must be "yyyy/mm/dd hh:mm:ss"
  //                        "0123456789012345678"

  const int ndigits = 14;
  const int idigits[ndigits] = {0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18};
  int yyyy, mon, dd, hh, min, ss;

  t = (time_t)-1;

  if (time_str.length() != 19)
    return false;

  // Validate time_str
  if (time_str[4] != L'/' ||
      time_str[7] != L'/' ||
      time_str[10] != L' ' ||
      time_str[13] != L':' ||
      time_str[16] != L':')
    return false;

  for (int i = 0;  i < ndigits; i++)
    if (!isdigit(time_str[idigits[i]]))
      return false;

  wistringstream is(time_str);
  wchar_t dummy;

  is >> yyyy >> dummy >> mon >> dummy >> dd
     >> hh >> dummy >> min >> dummy >> ss;

  if (!verifyDTvalues(yyyy, mon, dd, hh, min, ss))
    return false;

  // Accept 01/01/1970 as a special 'unset' value, otherwise there can be
  // issues with CTime constructor after apply daylight savings offset.
  if (yyyy == 1970 && mon == 1 && dd == 1) {
    t = (time_t)0;
    return true;
  }

  const CTime ct(yyyy, mon, dd, hh, min, ss, -1);

  t = (time_t)ct.GetTime();

  return true;
}

bool VerifyASCDateTimeString(const wstring &time_str, time_t &t)
{
  //  String format must be "ddd MMM dd hh:mm:ss yyyy"
  //                        "012345678901234567890123"
  // e.g.,                  "Wed Oct 06 21:02:38 2008"

  const wstring str_months = L"JanFebMarAprMayJunJulAugSepOctNovDec";
  const wstring str_days = L"SunMonTueWedThuFriSat";
  const int ndigits = 12;
  const int idigits[ndigits] = {8, 9, 11, 12, 14, 15, 17, 18, 20, 21, 22, 23};
  wstring::size_type iMON, iDOW;
  int yyyy, mon, dd, hh, min, ss;

  t = (time_t)-1;

  if (time_str.length() != 24)
    return false;

  // Validate time_str
  if (time_str[13] != L':' ||
      time_str[16] != L':')
    return false;

  for (int i = 0; i < ndigits; i++)
    if (!isdigit(time_str[idigits[i]]))
      return false;

  wistringstream is(time_str);
  wstring dow, mon_str;
  wchar_t dummy;
  is >> dow >> mon_str >> dd >> hh >> dummy >> min >> dummy >> ss >> yyyy;

  iMON = str_months.find(mon_str);
  if (iMON == wstring::npos)
    return false;

  mon = (iMON / 3) + 1;

  if (!verifyDTvalues(yyyy, mon, dd, hh, min, ss))
    return false;

  // Accept 01/01/1970 as a special 'unset' value, otherwise there can be
  // issues with CTime constructor after apply daylight savings offset.
  if (yyyy == 1970 && mon == 1 && dd == 1) {
    t = (time_t)0;
    return true;
  }

  const CTime ct(yyyy, mon, dd, hh, min, ss, -1);

  iDOW = str_days.find(dow);
  if (iDOW == wstring::npos)
    return false;

  iDOW = (iDOW / 3) + 1;
  if (iDOW != wstring::size_type(ct.GetDayOfWeek()))
    return false;

  t = (time_t)ct.GetTime();

  return true;
}

bool VerifyXMLDateTimeString(const wstring &time_str, time_t &t)
{
  //  String format must be "yyyy-mm-ddThh:mm:ss"
  //                        "0123456789012345678"
  // e.g.,                  "2008-10-06T21:20:56"

  wstring xtime_str;

  const int ndigits = 14;
  const int idigits[ndigits] = {0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18};
  int yyyy, mon, dd, hh, min, ss;

  t = (time_t)-1;

  if (time_str.length() != 19)
    return false;

  // Validate time_str
  if (time_str[4] != L'-' ||
      time_str[7] != L'-' ||
      time_str[10] != L'T' ||
      time_str[13] != L':' ||
      time_str[16] != L':')
    return false;

  for (int i = 0; i < ndigits; i++) {
    if (!isdigit(time_str[idigits[i]]))
      return false;
  }

  wistringstream is(time_str);
  wchar_t dummy;
  is >> yyyy >> dummy >> mon >> dummy >> dd >> dummy
      >> hh >> dummy >> min >> dummy >> ss;

  if (!verifyDTvalues(yyyy, mon, dd, hh, min, ss))
    return false;

  // Accept 01/01/1970 as a special 'unset' value, otherwise there can be
  // issues with CTime constructor after apply daylight savings offset.
  if (yyyy == 1970 && mon == 1 && dd == 1) {
    t = (time_t)0;
    return true;
  }

  const CTime ct(yyyy, mon, dd, hh, min, ss, -1);

  t = (time_t)ct.GetTime();

  return true;
}

bool VerifyXMLDateString(const wstring &time_str, time_t &t)
{
  //  String format must be "yyyy-mm-dd"
  //                        "0123456789"

  wstring xtime_str;
  const int ndigits = 8;
  const int idigits[ndigits] = {0, 1, 2, 3, 5, 6, 8, 9};
  int yyyy, mon, dd;

  t = (time_t)-1;

  if (time_str.length() != 10)
    return false;

  // Validate time_str
  if (time_str.substr(4,1) != L"-" ||
      time_str.substr(7,1) != L"-")
    return false;

  for (int i = 0; i < ndigits; i++) {
    if (!isdigit(time_str[idigits[i]]))
      return false;
  }

  wistringstream is(time_str);
  wchar_t dummy;

  is >> yyyy >> dummy >> mon >> dummy >> dd;

  if (!verifyDTvalues(yyyy, mon, dd, 1, 2, 3))
    return false;

  // Accept 01/01/1970 as a special 'unset' value, otherwise there can be
  // issues with CTime constructor after apply daylight savings offset.
  if (yyyy == 1970 && mon == 1 && dd == 1) {
    t = (time_t)0;
    return true;
  }

  const CTime ct(yyyy, mon, dd, 0, 0, 0, -1);

  t = (time_t)ct.GetTime();

  return true;
}

int VerifyImportPWHistoryString(const StringX &PWHistory,
                                StringX &newPWHistory, wstring &strErrors)
{
  // Format is (! == mandatory blank, unless at the end of the record):
  //    sxx00
  // or
  //    sxxnn!yyyy/mm/dd!hh:mm:ss!llll!pppp...pppp!yyyy/mm/dd!hh:mm:ss!llll!pppp...pppp!.........
  // Note:
  //    !yyyy/mm/dd!hh:mm:ss! may be !1970-01-01 00:00:00! meaning unknown

  wstring buffer;
  int ipwlen, pwleft = 0, s = -1, m = -1, n = -1;
  int rc = PWH_OK;
  time_t t;

  newPWHistory = L"";
  strErrors = L"";

  if (PWHistory.empty())
    return PWH_OK;

  StringX pwh(PWHistory);
  StringX tmp;
  const wchar_t *lpszPWHistory = NULL;
  int len = pwh.length();

  if (len < 5) {
    rc = PWH_INVALID_HDR;
    goto exit;
  }

  if (pwh[0] == L'0') s = 0;
  else if (pwh[0] == L'1') s = 1;
  else {
    rc = PWH_INVALID_STATUS;
    goto exit;
  }

  {
    StringX s1 (pwh.substr(1, 2));
    StringX s2 (pwh.substr(3, 4));
    wiStringXStream is1(s1), is2(s2);
    is1 >> hex >> m;
    is2 >> hex >> n;
  }

  if (n > m) {
    rc = PWH_INVALID_NUM;
    goto exit;
  }

  lpszPWHistory = pwh.c_str() + 5;
  pwleft = len - 5;

  if (pwleft == 0 && s == 0 && m == 0 && n == 0) {
    rc = PWH_IGNORE;
    goto exit;
  }

  Format(buffer, L"%01d%02x%02x", s, m, n);
  newPWHistory = buffer.c_str();

  for (int i = 0; i < n; i++) {
    if (pwleft < 26) {  //  blank + date(10) + blank + time(8) + blank + pw_length(4) + blank
      rc = PWH_TOO_SHORT;
      goto exit;
    }

    if (lpszPWHistory[0] != L' ') {
      rc = PWH_INVALID_CHARACTER;
      goto exit;
    }

    lpszPWHistory++;
    pwleft--;

    tmp = StringX(lpszPWHistory, 19);

    if (tmp.substr(0, 10) == L"1970-01-01")
      t = 0;
    else {
      if (!VerifyImportDateTimeString(tmp.c_str(), t)) {
        rc = PWH_INVALID_DATETIME;
        goto exit;
      }
    }

    lpszPWHistory += 19;
    pwleft -= 19;

    if (lpszPWHistory[0] != L' ') {
      rc = PWH_INVALID_CHARACTER;
      goto exit;
    }

    lpszPWHistory++;
    pwleft--;
    {
      StringX s3(lpszPWHistory, 4);
      wiStringXStream is3(s3);
      is3 >> hex >> ipwlen;
    }
    lpszPWHistory += 4;
    pwleft -= 4;

    if (lpszPWHistory[0] != L' ') {
      rc = PWH_INVALID_CHARACTER;
      goto exit;
    }

    lpszPWHistory += 1;
    pwleft -= 1;

    if (pwleft < ipwlen) {
      rc = PWH_INVALID_PSWD_LENGTH;
      goto exit;
    }

    tmp = StringX(lpszPWHistory, ipwlen);
    Format(buffer, L"%08x%04x%s", (long) t, ipwlen, tmp.c_str());
    newPWHistory += buffer.c_str();
    buffer.clear();
    lpszPWHistory += ipwlen;
    pwleft -= ipwlen;
  }

  if (pwleft > 0)
    rc = PWH_TOO_LONG;

 exit:
  Format(buffer, IDSC_PWHERROR, len - pwleft + 1);
  wstring temp;
  switch (rc) {
    case PWH_OK:
    case PWH_IGNORE:
      temp.clear();
      buffer.clear();
      break;
    case PWH_INVALID_HDR:
      Format(temp, IDSC_INVALIDHEADER, PWHistory.c_str());
      break;
    case PWH_INVALID_STATUS:
      Format(temp, IDSC_INVALIDPWHSTATUS, s);
      break;
    case PWH_INVALID_NUM:
      Format(temp, IDSC_INVALIDNUMOLDPW, n, m);
      break;
    case PWH_INVALID_DATETIME:
      LoadAString(temp, IDSC_INVALIDDATETIME);
      break;
    case PWH_INVALID_PSWD_LENGTH:
      LoadAString(temp, IDSC_INVALIDPWLENGTH);
      break;
    case PWH_TOO_SHORT:
      LoadAString(temp, IDSC_FIELDTOOSHORT);
      break;
    case PWH_TOO_LONG:
      LoadAString(temp, IDSC_FIELDTOOLONG);
      break;
    case PWH_INVALID_CHARACTER:
      LoadAString(temp, IDSC_INVALIDSEPARATER);
      break;
    default:
      ASSERT(0);
  }
  strErrors = buffer + temp;
  if (rc != PWH_OK)
    newPWHistory = L"";

  return rc;
}
