/*
* Copyright (c) 2003-2008 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef VIEW_H
#define VIEW_H

//  Contains methods to display user interface, called by model

class View
{
public:
  static View * Instance() {};
  View() {};
  virtual ~View() {};

  virtual int PromptUserForCombination() = 0;
  virtual int ShowList()  = 0;
protected:
  static View * _instance;
};
#endif //  VIEW_H
