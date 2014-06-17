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

#ifndef ABSTRACTSPELLERINTERFACE_H
#define ABSTRACTSPELLERINTERFACE_H
#include <MainDef.h>
class AbstractSpellerInterface
{
private:
protected:
public:
  AbstractSpellerInterface ();
  ~AbstractSpellerInterface ();

  virtual std::vector<wchar_t*> *GetLanguageList () = 0;
  virtual void SetLanguage (wchar_t *Lang) = 0;
  virtual void SetMultipleLanguages (std::vector<wchar_t *> *List) = 0; // Languages are from LangList
  void SetMode (int Multi) { MultiMode = Multi; }                         // Multi - 1, Single - 0
  void SetEncoding (EncodingType Encoding) {CurrentEncoding = Encoding; }
  BOOL CheckWord (char *Word);                         // Word in Utf-8 or ANSI
  virtual BOOL SpellerCheckWord (char *Word) = 0;      // Word in Utf-8 or ANSI
  virtual std::vector<wchar_t *> *GetSuggestions (char *Word) = 0;
  virtual void AddToDictionary (char *Word, int DictionaryNum = -1) = 0;
  void IgnoreAll (char *Word);
  virtual BOOL IsWorking () = 0;

private:
protected:
  int MultiMode;
  EncodingType CurrentEncoding;
  WordSet *Ignored;
public:
};
#endif // ABSTRACTSPELLERINTERFACE_H