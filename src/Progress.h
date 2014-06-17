/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef PROGRESS_H
#define PROGRESS_H

#include "StaticDialog\StaticDialog.h"

class Progress : public StaticDialog
{
public:
  Progress(void);
  ~Progress(void);

  BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
  void init (HINSTANCE hInst, HWND Parent);
  void DoDialog ();
  void SetProgress ();
  void SetProgress (int value);
  void SetBottomMessage (wchar_t *Message);
  void SetTopMessage (wchar_t *Message);
private:
  HWND HDescBottom;
  HWND HDescTop;
  HWND HProgressBar;
};
#endif //PROGRESS_H
