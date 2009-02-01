/*
 * Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/**
 * \file wStringXStream.h
 *
 * STL-based implementation of secure string streams.
 * typedefs of secure versions of istringstream, ostringstream
 * and stringbuf.
 * Secure in the sense that memory is scrubbed before
 * being returned to system.
 */

#ifndef _STRINGXSTREAM_H_
#define _STRINGXSTREAM_H_

#include "StringX.h"
#include <sstream>

// stringstream typedefs for StringX 
typedef std::basic_stringbuf<wchar_t, std::char_traits<wchar_t>,
                             S_Alloc::SecureAlloc<wchar_t> > wStringXBuf;

typedef std::basic_istringstream<wchar_t, std::char_traits<wchar_t>,
                                 S_Alloc::SecureAlloc<wchar_t> > wiStringXStream;

typedef std::basic_ostringstream<wchar_t, std::char_traits<wchar_t>,
                                 S_Alloc::SecureAlloc<wchar_t> > woStringXStream;

typedef std::basic_stringstream<wchar_t, std::char_traits<wchar_t>,
                                S_Alloc::SecureAlloc<wchar_t> > wStringXStream;

#endif /* _STRINGXSTREAM_H_ */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
