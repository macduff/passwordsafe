$Id$

*******************************************************************
*** IMPORTANT: THIS IS NOT A MAINSTREAM VERSION OF PASSWORDSAFE ***
*******************************************************************

This is version 1.92-DK, an experimental version, released in order to
gather input from the user community regarding a feature being debated
by the developers.

This version supports up to three passwords per entry, each one may be
copied to the clipboard separately. The motivation for this feature is
a banking website used by the author which requires this.

The development team will support this version, in the sense of
responding to minor bugs. However, the multiple password feature will
either be incorporated into release 2.0, or silently dropped.

This version will read existing passwordsafe databases. However,
databases created (or saved) by this version CANNOT be read by other
versions of PasswordSafe.

So, if you decide to use this version, please drop the developers
(mailto:passwordsafe-devel@lists.sourceforge.net) a note on your
thought regarding this feature. Thanks.

The rest of this file is the standard README.

Introduction:
=============
Password Safe is a password database utility. Like many other such
products, commercial and otherwise, it stores your passwords in an
encrypted file, allowing you to remember only one password (the "safe
combination"), instead of all the username/password combinations that
you use.

What makes Password Safe different? Three things:
1. Simplicity: Password Safe is designed to do one thing, and to do it
well. Start the application, enter your "combination", double-click on
the right entry - presto - the password is now on your clipboard,ready
for pasting.
2. Security: The original version was designed and written by Bruce
Schneier - 'nuff said.
3. Open Source: The source code for the project is available for
inspection. For more information, see http://passwordsafe.sourceforge.net

Password Safe currently runs on Windows 95, 98, ME, NT4, 2000 and
XP. Support for additional platforms is planned for future releases.

Downloading:
============
The latest & greatest version of Password Safe may be downloaded from
https://sourceforge.net/project/showfiles.php?group_id=41019

Installation:
=============
Nothing special. No "Setup", no dependencies, no annoying wizard, no
need to sacrifice a goat and/or reboot your computer. Just extract the
files (using WinZip, for example) to any directory, double-click on
the PasswordSafe.exe icon,and that's it. "Advanced" users may want to
create a shortcut to their desktop and/or Start menu.

License:
========
Password Safe is available under the restrictions set forth in the
standard "Artistic License". For more details, see the LICENSE file
that comes with the package.

Helping Out:
============
Please send all bugs and feature requests to the PasswordSafe user's
mailing list. You can subscribe via:

http://lists.sourceforge.net/lists/listinfo/passwordsafe-users

If you wish to contribute to the project by writing code and/or
documentation, please drop a note to the developer's mailing list:

http://lists.sourceforge.net/lists/listinfo/passwordsafe-devel

(Just to round out the list, there's also an announcement mailing list
where new releases are announced:

http://lists.sourceforge.net/lists/listinfo/passwordsafe-announce)

Release Notes:
==============
For information on the latest features, bugfixes and known problems,
see the ReleaseNotes.txt file that comes with the package.

Credits:
========
- The multiple-password feature of this version of Password Safe was
written by David Kelvin.
- The original version of Password Safe was written by Mark Rosen,
and was freely downloadable from Conterpane Lab's website. Thanks to
Mark for writing a great little application! Following Mark, it was
maintained by "AYB", at Counterpane. Thanks to Counterpane for
deciding to open source the project!
- Jim Russell first brought the code to SourceForge, did some major
cleaning up of the code, set up a nice project and added some minor
features in release 1.9.0
- The current release has been brought to you by: Andrew Mullican,
Edward Quackenbush, Gregg Conklin, Graham Ullrich, and Rony
Shapiro. Karel (Karlo) Van der Gucht also contributed some of the
password policy code and some GUI improvements for 1.92.
- Finally, thanks to the folks at SourceForge for hosting this
project.

$Log$
Revision 1.2.2.1.2.2  2003/05/30 06:12:26  ronys
Update Credits

Revision 1.2.2.1.2.1  2003/05/29 15:35:42  ronys
First checkin of DK's multi-password changes

Revision 1.2.2.1  2003/05/13 13:40:13  ronys
1.92 release

Revision 1.2  2003/04/30 13:20:14  ronys
Listed supported platforms

Revision 1.1  2003/04/29 14:22:32  ronys
First versions of README and Release Notes under CVS for 1.91
