/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include <ctype.h>
#include <string.h>
#include <cstdarg>
#include "StringX.h"
#include "os/pws_tchar.h"
#ifdef _WIN32
  #include <afx.h>
#endif
// A few convenience functions for StringX & wstring

template<class T> int CompareNoCase(const T &s1, const T &s2)
{
  // case insensitive string comparison
  return _wcsicmp(s1.c_str(), s2.c_str());
}

template<class T> void ToLower(T &s)
{
  for (typename T::iterator iter = s.begin(); iter != s.end(); iter++)
    *iter = wchar_t(towlower(*iter));
}

template<class T> void ToUpper(T &s)
{
  for (typename T::iterator iter = s.begin(); iter != s.end(); iter++)
    *iter = wchar_t(towupper(*iter));
}

template<class T> T &Trim(T &s, const wchar_t *set)
{
  const wchar_t *ws = L" \t\r\n";
  const wchar_t *tset = (set == NULL) ? ws : set;

  typename T::size_type b = s.find_first_not_of(tset);
  if (b == T::npos) {
    s.clear();
  } else {
    typename T::size_type e = s.find_last_not_of(tset);
    T t(s.begin() + b, s.end() - (s.length() - e) + 1);
    s = t;
  }
  return s;
}

template<class T> T &TrimRight(T &s, const wchar_t *set)
{
  const wchar_t *ws = L" \t\r\n";
  const wchar_t *tset = (set == NULL) ? ws : set;

  typename T::size_type e = s.find_last_not_of(tset);
  if (e == T::npos) {
    s.clear();
  } else {
    T t(s.begin(), s.end() - (s.length() - e) + 1);
    s = t;
  }
  return s;
}

template<class T> T &TrimLeft(T &s, const wchar_t *set)
{
  const wchar_t *ws = L" \t\r\n";
  const wchar_t *tset = (set == NULL) ? ws : set;

  typename T::size_type b = s.find_first_not_of(tset);
  if (b == T::npos) {
    s.clear();
  } else {
    T t(s.begin() + b, s.end());
    s = t;
  }
  return s;
}

template<class T> void EmptyIfOnlyWhiteSpace(T &s)
{
  const wchar_t *ws = L" \t\r\n";
  typename T::size_type b = s.find_first_not_of(ws);
  if (b == T::npos)
    s.clear();
}

template<class T> int Replace(T &s, wchar_t from, wchar_t to)
{
  int retval = 0;
  T r;
  r.reserve(s.length());
  for (typename T::iterator iter = s.begin(); iter != s.end(); iter++)
    if (*iter == from) {
      r.append(1, to);
      retval++;
    } else
      r.append(1, *iter);
  s = r;
  return retval;
}

template<class T> int Replace(T &s, const T &from, const T &to)
{
  int retval = 0;
  T r;
  typename T::size_type i = 0;
  do {
   typename T::size_type j = s.find(from, i);
    r.append(s, i, j - i);
    if (j != StringX::npos) {
      r.append(to);
      retval++;
      i = j + from.length();
    } else
      i = j;
  } while (i != StringX::npos);
  s = r;
  return retval;
}

template<class T> int Remove(T &s, wchar_t c)
{
  int retval = 0;
  T t;
  for (typename T::iterator iter = s.begin(); iter != s.end(); iter++)
    if (*iter != c)
      t += *iter;
    else
      retval++;
  if (retval != 0)
    s = t;
  return retval;
}

template<class T> void LoadAString(T &s, int id)
{
#ifdef _WIN32
  CString cs;
  cs.LoadString(id);
  s = cs;
#endif
}

template<class T> void Format(T &s, const wchar_t *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  int len = _vsctprintf(fmt, args) + 1;
  wchar_t *buffer = new wchar_t[len];
  _vstprintf_s(buffer, len, fmt, args);
  s = buffer;
  delete[] buffer;
  va_end(args);
}

template<class T> void Format(T &s, int fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  T fmt_str;
  LoadAString(fmt_str, fmt);
  int len = _vsctprintf(fmt_str.c_str(), args) + 1;
  wchar_t *buffer = new wchar_t[len];
  _vstprintf_s(buffer, len, fmt_str.c_str(), args);
  s = buffer;
  delete[] buffer;
  va_end(args);
}


// instantiations for StringX & wstring
template int CompareNoCase(const StringX &s1, const StringX &s2);
template int CompareNoCase(const wstring &s1, const wstring &s2);
template void ToLower(StringX &s);
template void ToLower(wstring &s);
template void ToUpper(StringX &s);
template void ToUpper(wstring &s);
template StringX &Trim(StringX &s, const wchar_t *set);
template wstring &Trim(wstring &s, const wchar_t *set);
template StringX &TrimRight(StringX &s, const wchar_t *set);
template wstring &TrimRight(wstring &s, const wchar_t *set);
template StringX &TrimLeft(StringX &s, const wchar_t *set);
template wstring &TrimLeft(wstring &s, const wchar_t *set);
template void EmptyIfOnlyWhiteSpace(StringX &s);
template void EmptyIfOnlyWhiteSpace(wstring &s);
template int Replace(StringX &s, wchar_t from, wchar_t to);
template int Replace(wstring &s, wchar_t from, wchar_t to);
template int Replace(StringX &s, const StringX &from, const StringX &to);
template int Replace(wstring &s, const wstring &from, const wstring &to);
template int Remove(StringX &s, wchar_t c);
template int Remove(wstring &s, wchar_t c);
template void LoadAString(wstring &s, int id);
template void LoadAString(StringX &s, int id);
template void Format(wstring &s, int fmt, ...);
template void Format(StringX &s, int fmt, ...);
template void Format(wstring &s, const wchar_t *fmt, ...);
template void Format(StringX &s, const wchar_t *fmt, ...);


#ifdef TEST_TRIM
int main(int argc, char *argv[])
{
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " string-to-trim" << endl;
    exit(1);
  }

  StringX s(argv[1]), sl(argv[1]), sr(argv[1]);
  Trim(s); TrimLeft(sl); TrimRight(sr);
  cout << "Trim(\"" << argv[1] << "\") = \"" << s <<"\"" << endl;
  cout << "TrimLeft(\"" << argv[1] << "\") = \"" << sl <<"\"" << endl;
  cout << "TrimRight(\"" << argv[1] << "\") = \"" << sr <<"\"" << endl;
  return 0;
}
#endif
