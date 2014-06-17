#/*
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
#ifndef HUNSPELLINTERFACE_H
#define HUNSPELLINTERFACE_H

#include "AbstractSpellerInterface.h"

#ifndef USING_STATIC_LIBICONV
#define USING_STATIC_LIBICONV
#endif // !USING_STATIC_LIBICONV

#include "iconv.h"
#include "CommonFunctions.h"
#include "MainDef.h"

class Hunspell;

struct DicInfo
{
  Hunspell *Speller;
  iconv_t ConverterUTF8;
  iconv_t BackConverterUTF8;
  iconv_t ConverterANSI;
  iconv_t BackConverterANSI;
  iconv_t ConverterWCHAR;
  iconv_t BackConverterWCHAR;
  wchar_t *LocalDicPath;
  WordSet *LocalDic; // Stored in Dictionary encoding
};

struct AvailableLangInfo
{
  wchar_t *Name;
  int Type; // Type = 1 - System Dir Dictionary, 0 - Nomal Dictionary

  bool operator < (const AvailableLangInfo &rhs ) const
  {
    return (wcscmp (Name, rhs.Name) < 0);
  }
};

class HunspellInterface : public AbstractSpellerInterface
{
public:
  HunspellInterface (HWND NppWindowArg);
  ~HunspellInterface ();
  __override virtual std::vector<wchar_t*> *GetLanguageList ();
  __override virtual void SetLanguage (wchar_t *Lang);
  __override virtual void SetMultipleLanguages (std::vector<wchar_t *> *List); // Languages are from LangList
  __override virtual BOOL SpellerCheckWord (char *Word); // Word in Utf-8 or ANSI
  __override virtual BOOL IsWorking ();
  __override virtual std::vector<wchar_t *> *GetSuggestions (char *Word);
  __override virtual void AddToDictionary (char *Word, int DictionaryNum = -1);

  void SetDirectory (wchar_t *Dir);
  void SetAdditionalDirectory (wchar_t *Dir);
  void WriteUserDic (WordSet *Target, wchar_t *Path);
  void ReadUserDic (WordSet *Target, wchar_t *Path);
  void UpdateOnDicRemoval (wchar_t *Path, BOOL &NeedSingleLangReset, BOOL &NeedMultiLangReset);
  BOOL GetLangOnlySystem (wchar_t *Lang);
private:
  DicInfo CreateHunspell (wchar_t *Name, int Type);
  BOOL ConvertAndCheckWord (DicInfo Dic, char *Word, EncodingType Encoding);
  void MessageBoxWordCannotBeAdded ();
  void FilleVectorFromHunspellWordList (const char ***WordList, int num, std::vector<wchar_t *> *TargetVector);
public:
private:
  BOOL IsHunspellWorking;
  BOOL UseOneDic;
  wchar_t *DicDir;
  wchar_t *SysDicDir;
  std::set <AvailableLangInfo> *DicList;
  std::map <wchar_t *, DicInfo, bool (*)(wchar_t *, wchar_t *)> *AllHunspells;
  char *GetConvertedWord (const char *Source, iconv_t Converter);
  char *GetConvertedWord (const wchar_t *Source, iconv_t Converter);
  DicInfo SingularSpeller;
  DicInfo Empty;
  std::vector <DicInfo> *Spellers;
  WordSet *Memorized;
  BOOL InitialReadingBeenDone;
  char *TemporaryBuffer;
  wchar_t *UserDicPath; // For now only default one.
  wchar_t *SystemWrongDicPath; // Only for reading and then removing
  HWND NppWindow;
};
#endif // HUNSPELLINTERFACE_H