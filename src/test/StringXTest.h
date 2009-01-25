/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// MyStringTest.h: Use the test class
#include <sstream>

#include "test.h"
#include "corelib/StringX.h"
#include "corelib/StringXStream.h"
#include "os/typedefs.h"

class StringXTest : public Test
{

public:
  StringXTest()
  {
  }

  void run()
  {
    // The tests to run:
    testConstructors();
    testOperators();
    testAppend();
    testAssign();
  }

  void testConstructors()
  {
    StringX s0;
    // StringX()
    _test(s0.length() == 0);
    _test(s0.empty());
    
    // StringX( const char* str )
    wchar_t v1[] = L"abcd";
    StringX s1(v1);
    _test(s1.length() == 4);
    for (int i = 0; i < 4; i++)
      _test(s1[i] == v1[i]);
    
    // StringX( const StringX& s )
    StringX s2(s1);
    _test(s1 == s2);

    // StringX( size_type length, const char& ch )
    StringX s3(5, L'X');
    StringX s4(L"XXXXX");
    _test(s3 == s4);
    // StringX( const char* str, size_type length )
    StringX s5(v1, 2);
    _test(s5.length() == 2);
    for (int i = 0; i < 2; i++)
      _test(s5[i] == v1[i]);

    // StringX( const string& str, size_type index, size_type length )
    StringX s6(s1, 1, 2);
    _test(s6.length() == 2);
    _test(s6[0] == s1[1] && s6[1] == s1[2]);
    
    // StringX( input_iterator start, input_iterator end )
    StringX s7(s1.begin()+1, s1.end()-1);
    _test(s6 == s7);
  }

  void testOperators()
  {
    /* bool operator==(const StringX& c1, const StringX& c2) */
    StringX s1(L"yada"), s2(L"yada"), s3;
    _test(s1 == s2);
    _test(!(s2 == s3));

/* bool operator!=(const StringX& c1, const StringX& c2) */
    _test(s2 != s3);
    _test(!(s1 != s2));

/* bool operator<(const StringX& c1, const StringX& c2) */
    StringX s4(L"one1"), s5(L"one2");
    _test(s4 < s5);

/* bool operator>(const StringX& c1, const StringX& c2) */
    _test(s5 > s4);
    
/* bool operator<=(const StringX& c1, const StringX& c2) */
    _test(s4 <= s5);
    _test(s4 <= s4);
/* bool operator>=(const StringX& c1, const StringX& c2) */
    _test(s5 >= s4);
    _test(s5 >= s5);
    
/* StringX operator+(const StringX& s1, const StringX& s2 ) */
    StringX s6(L"one1one2");
    _test(s6 == s4 + s5);

/* StringX operator+(const char* s, const StringX& s2 ) */
    _test(s6 == L"one1" + s5);

/* StringX operator+( char c, const StringX& s2 ) */
    StringX s7 = L'q' + s6;
    StringX s8(L"qone1one2");
    _test(s7 == s8);

/* StringX operator+( const StringX& s1, const char* s ) */
    wchar_t v2[] = L"hell";
    StringX s9(L"bloody"), s10(L"bloodyhell");
    _test(s10 == s9 + v2);

/* StringX operator+( const StringX& s1, char c ) */
    StringX s11 = s6 + L'q';
    StringX s12(L"one1one2q");
    _test(s11 == s12);

/* ostream& operator<<( ostream& os, const StringX& s ) */
    woStringXStream os;
    os << s12;
    _test(os.str() == L"one1one2q");
    
/* istream& operator>>( istream& is, StringX& s ) */
    wiStringXStream is(L"15");
    int x;
    is >> x;
    _test(x == 15);
/* StringX& operator=( const StringX& s ) */
    StringX s14(L"mumble");
    StringX s15;
    s15 = s14;
    _test(s15 == s14);

/* StringX& operator=( const char* s ) */
    s15 = L"oklahoma";
    _test(s15 == L"oklahoma");

/* StringX& operator=( char ch ) */
    s15 = L'W';
    _test(s15.length() == 1 && s15[0] == L'W');

/* char& operator[]( size_type index ) */
    s14[0] = L'j';
    _test(s14 == L"jumble");
  }

  void testAppend()
  {
    /* StringX& append( const StringX& str ); */
    StringX s1(L"blue"), s2(L"eyes"), s3;
    s3 = s1.append(s2);
    _test(s3 == L"blueeyes");
    
    /* StringX& append( const char* str ); */
    s1 = L"blue";
    s3 = s1.append(L"nose");
    _test(s3 == L"bluenose");
    
    /* StringX& append( const StringX& str, size_type index, size_type len ); */
    StringX s4(L"green");
    s3 = s4.append(s3, 4, 4);
    _test(s3 == L"greennose");
    
    /* StringX& append( const char* str, size_type num ); */
    s3 = s3.append(L"redyellow", 3);
    _test(s3 == L"greennosered");
    
    /* StringX& append( size_type num, char ch ); */
    s3 = s3.append(3, L'!');
    _test(s3 == L"greennosered!!!");
    
    /* StringX& append( input_iterator start, input_iterator end ); */
    s3 = L"Yeehaw";
    s3 = s3.append(s3.begin()+3, s3.end());
    _test(s3 == L"Yeehawhaw");
  }
  void testAssign()
  {
    StringX s1(L"Flew");
    s1.assign(2, L'B');
    _test(s1 == L"BB");
    StringX s2;
    s2.assign(s1);
    _test(s1 == s2);
  }
};


