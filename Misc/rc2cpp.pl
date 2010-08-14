#!/usr/bin/perl -w
#
# Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
# All rights reserved. Use of the code is allowed under the
# Artistic License 2.0 terms, as specified in the LICENSE file
# distributed with this code, or available from
# http://www.opensource.org/licenses/artistic-license-2.0.php
#
# A simple utility to create a C++ and header file given an rc2 file
# containing one or more stringtables.
#
# Usage: $0 rc2file
# Input: rc2file.rc2
# Output: rc2file_st.cpp, rc2file_st.h
#
# The .cpp file contains a std::map<int, TCHAR *> named rc2file_st
# The .h file contains a declaration of same.

use strict;
use warnings;
use locale;
use File::Basename;

sub usage {
    print "Usage: $0 rc2file\n";
    exit 1;
}

&usage unless ($#ARGV == 0);
my $PATHNAME = $ARGV[0];
my $RC2FILE;
my $CPPFILE;
my $HFILE;
my %MAP;

# accept both filename.rc2 and filename as input
if ($PATHNAME =~ m/(.+)\.rc2/) {
    $PATHNAME = $1;
}
$RC2FILE = "${PATHNAME}.rc2";
$CPPFILE = "${PATHNAME}_st.cpp";
$HFILE = "${PATHNAME}_st.h";

my $BASE;
my $dummy;
($BASE, $dummy, $dummy) = fileparse($RC2FILE, qr{\.rc2});
my $B = uc($BASE);
my $b = lc($BASE);

my $include;


&ReadRC2File;
&WriteHFile;
&WriteCPPFile;
exit 0;
#-----------------------------------------------------------------
sub ReadRC2File {
    my $inST = 0;
    open(RC2, "<$RC2FILE") || die "Couldn't open $RC2FILE\n";
    while (<RC2>) {
        if (m/^#include.*/) {
            $include = $_;
            $include =~ s,\\,/,g;
        } elsif (m/^\s*STRINGTABLE\s*$/) {
            $inST = 1;
        } elsif ($inST == 1 && m/^\s*BEGIN\s*$/) {
            $inST = 2;
        } elsif ($inST == 2 && m/^\s*END\s*$/) {
            $inST = 0;
        } elsif ($inST == 2 && m/^\s*(\w+)\s+(\".+\")[^"]*/) {
            my ($key, $val) = ($1, $2);
            # replace "" with \"
            $val =~ s/\"\"/\\\"/g;
            $MAP{$key} = $val;
        }
    }
    close(RC2);
}

sub WriteHFile {
    open(H, ">$HFILE") || die "Couldn't open $HFILE\n";
    print H <<"EOT";
#ifndef __${B}_ST_H
#define __${B}_ST_H
/**
 * Declaration of string table map
 * Derived from $RC2FILE
 * Generated by $0
 *
 * THIS FILE IS AUTOMATICALLY GENERATED. DO NOT EDIT.
 * Changes made to this file will be lost when the
 * file is regenerated.
 *
 * Note: The contents of this file is not needed for MFC builds but
 * it does not seem possible to conditionally exclude it from these.
 *
 */

#if !defined(_WIN32) || defined(__WX__)
#include <map>
#include "../os/typedefs.h" // for definition of TCHAR
extern std::map<int, const TCHAR *> ${b}_st;
#endif

#endif /* __${B}_ST_H */
EOT
    close(H);
}

sub WriteCPPFile {
    open(CPP, ">$CPPFILE") || die "Couldn't open $CPPFILE\n";
    print CPP <<"PREAMBLE";
/**
 * Definition of string table map
 * Derived from $RC2FILE
 * Generated by $0
 *
 * THIS FILE IS AUTOMATICALLY GENERATED. DO NOT EDIT.
 * Changes made to this file will be lost when the
 * file is regenerated.
 *
 * Note: The contents of this file is not needed for MFC builds but
 * it does not seem possible to conditionally exclude it from these.
 *
 */

#if !defined(_WIN32) || defined(__WX__)

#ifdef UNICODE
#define _(x) L ## x
#else
#define _(x) x
#endif

#include "./${BASE}_st.h"
#include <utility>
${include}

using namespace std;

namespace {
  pair<int, const TCHAR *> Pairs[] = {
PREAMBLE
# print %MAP, sorted by constant name, just to be consistent
    my $key;
    foreach $key (sort keys %MAP) {
        print CPP "    make_pair($key, _($MAP{$key})),\n";
}
    print CPP <<"POSTAMBLE";
  }; // Pairs array
} // anonymous namespace

map<int, const TCHAR *> ${b}_st(Pairs, Pairs + sizeof(Pairs)/sizeof(Pairs[0]));

#endif
POSTAMBLE
    close(CPP);
}


