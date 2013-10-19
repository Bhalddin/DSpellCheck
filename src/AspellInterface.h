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
#ifndef ASPELLINTERFACE_H
#define ASPELLINTERFACE_H

struct AspellCanHaveError;
struct AspellSpeller;

#include "AbstractSpellerInterface.h"

class AspellInterface : public AbstractSpellerInterface
{
public:
  AspellInterface (HWND NppWindowArg); // Window for error reporting
  ~AspellInterface ();
  __override virtual std::vector<TCHAR*> *GetLanguageList ();
  __override virtual void SetLanguage (TCHAR *Lang);
  __override virtual void SetMultipleLanguages (std::vector<TCHAR *> *List); // Languages are from LangList
  __override virtual BOOL SpellerCheckWord (char *Word); // Word in Utf-8 or ANSI (For now only Utf-8)
  __override virtual BOOL IsWorking ();
  __override virtual std::vector<wchar_t *> *GetSuggestions (char *Word);
  __override virtual void AddToDictionary (char *Word, int DictionaryNum = -1);

  BOOL Init (TCHAR *PathArg);
private:
  void SendAspellErorr (AspellCanHaveError *Error);
public:
private:
  AspellSpeller *SingularSpeller;
  std::vector<AspellSpeller *> *Spellers;
  BOOL AspellLoaded;
  HWND NppWindow; // For message boxes
};
#endif // ASPELLINTERFACE_H