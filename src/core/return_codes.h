/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/*
  Return codes used by all routines within core
*/

#ifndef __RETURN_CODES_H
#define __RETURN_CODES_H

namespace PWSRC {
  enum {
    SUCCESS = 0,                              // 0x0000 =    0
    FAILURE,                                  // 0x0001 =    1
    USER_CANCEL,                              // 0x0002 =    2
    USER_EXIT,                                // 0x0003 =    3
    LIMIT_REACHED,                            // 0x0004 =    4
    USER_DECLINED_SAVE,                       // 0x0005 =    5
    CANT_GET_LOCK,                            // 0x0006 =    6
    DB_HAS_CHANGED,                           // 0x0007 =    7
    OK_WITH_VALIDATION_ERRORS,                // 0x0008 =    8
 
    // Files
    CANT_OPEN_FILE = 0x0020,                  // 0x0020 =   32
    NOT_PWS3_FILE,                            // 0x0021 =   33
    UNSUPPORTED_VERSION,                      // 0x0022 =   34
    WRONG_VERSION,                            // 0x0023 =   35
    UNKNOWN_VERSION,                          // 0x0024 =   36
    INVALID_FORMAT,                           // 0x0025 =   37
    END_OF_FILE,                              // 0x0026 =   38
    TRUNCATED_FILE,                           // 0x0027 =   39 (missing EOF marker)
    READ_FAIL,                                // 0x0028 =   40
    WRITE_FAIL,                               // 0x0029 =   41
    ALREADY_OPEN,                             // 0x002A =   42
    WRONG_PASSWORD,                           // 0x002B =   43
    BAD_DIGEST,                               // 0x002C =   44

    // XML import/export
    XML_FAILED_VALIDATION = 0x0040,           // 0x0040 =   64
    XML_FAILED_IMPORT,                        // 0x0041 =   65
    NO_ENTRIES_EXPORTED,                      // 0x0042 =   66
    DB_HAS_DUPLICATES,                        // 0x0043 =   67
    OK_WITH_ERRORS,                           // 0x0044 =   68

    // Preferences
    XML_LOAD_FAILED = 0x0060,                 // 0x0060 =   96
    XML_NODE_NOT_FOUND,                       // 0x0061 =   97
    XML_PUT_TEXT_FAILED,                      // 0x0062 =   98
    XML_SAVE_FAILED,                          // 0x0063 =   99

    // Attachments
    HEADERS_INVALID = 0x0100,                 // 0x0100 =  256
    BAD_ATTACHMENT,                           // 0x0101 =  257
    END_OF_DATA,                              // 0x0102 =  258
    BADTARGETDEVICE,                          // 0x0103 =  259
    CANTCREATEFILE,                           // 0x0104 =  260
    CANTFINDATTACHMENT,                       // 0x0105 =  261
    BADDATA,                                  // 0x0106 =  262
    BADATTACHMENTWRITE,                       // 0x0107 =  263
    BADCRCDIGEST,                             // 0x0108 =  264
    BADLENGTH,                                // 0x0109 =  265
    BADINFLATE,                               // 0x010A =  266

    UNIMPLEMENTED = 0x0FFF,                   // 0x0FFF = 4095
  };
};

#endif /* __RETURN_CODES_H */
