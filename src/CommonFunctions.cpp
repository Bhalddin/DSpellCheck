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
#include "CommonFunctions.h"
#include "iconv.h"
#include "MainDef.h"
#include "Plugin.h"
#include "PluginInterface.h"

#include <hash_map>

void SetString (char *&Target, const char *Str)
{
  if (Target == Str)
    return;
  CLEAN_AND_ZERO_ARR (Target);
  Target = new char[strlen (Str) + 1];
  strcpy (Target, Str);
}

void SetString (wchar_t *&Target, const wchar_t *Str)
{
  if (Target == Str)
    return;
  CLEAN_AND_ZERO_ARR (Target);
  Target = new wchar_t[wcslen (Str) + 1];
  wcscpy (Target, Str);
}

void SetString (char *&Target, const wchar_t *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  size_t OutSize = wcslen (Str) + 1;
  size_t PrevOutSize = 0;
  Target = new char[OutSize];
  char *OutBuf = Target;
  size_t res = (size_t) -1;

  iconv_t Converter = iconv_open ("CHAR//IGNORE", "UCS-2LE");
  size_t InSize = (wcslen (Str) + 1) * sizeof (wchar_t);

  while (*Str)
  {
    InSize = 2;
    if (!InSize)
      break;
    PrevOutSize = OutSize;
    res = iconv (Converter, (const char **) &Str, &InSize, &OutBuf, &OutSize);
    if (PrevOutSize - OutSize > 1)
    {
      // If char is multichar then we count it as not converted
      OutBuf -= (PrevOutSize - OutSize);
      *(OutBuf) = 0;
      OutSize = PrevOutSize;
    }
  }

  *(OutBuf) = 0;
  iconv_close (Converter);
  if (res == (size_t)(-1))
  {
    *Target = '\0';
  }
}

void SetString (wchar_t *&Target, const char *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  size_t OutSize = sizeof (wchar_t) * (strlen (Str) + 1);
  Target = new wchar_t [OutSize];
  char *OutBuf = (char *) Target;

  iconv_t Converter = iconv_open ("UCS-2LE//IGNORE", "CHAR");

  size_t InSize = strlen (Str) + 1;
  size_t res = iconv (Converter, (const char **) &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);

  if (res == (size_t)(-1))
  {
    *Target = '\0';
  }
}

void SetStringSUtf8 (wchar_t *&Target, const char *Str)
{
  iconv_t Converter = iconv_open ("UCS-2LE//IGNORE", "UTF-8");
  CLEAN_AND_ZERO_ARR (Target);
  size_t InSize = strlen (Str) + 1;
  size_t OutSize = (Utf8Length (Str) + 1) * sizeof (wchar_t);
  Target = new wchar_t[OutSize];
  char *OutBuf = (char *) Target;
  size_t res = iconv (Converter, &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);

  if (res == (size_t)(-1))
  {
    *Target = '\0';
  }
}

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const wchar_t *Str)
{
  iconv_t Converter = iconv_open ("UTF-8//IGNORE", "UCS-2LE");
  CLEAN_AND_ZERO_ARR (Target);
  size_t InSize = (wcslen (Str) + 1) * sizeof (wchar_t);
  size_t OutSize = 6 * wcslen (Str) + 1; // Maximum Possible UTF-8 size
  char *TempBuf = new char[OutSize];
  char *OutBuf = (char *) TempBuf;
  size_t res = iconv (Converter, (const char **) &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);
  if (res == (size_t)(-1))
  {
    Target = new char[1];
    *Target = '\0';
    CLEAN_AND_ZERO_ARR (TempBuf)
      return;
  }
  SetString (Target, TempBuf); // Cutting off unnecessary symbols.
  CLEAN_AND_ZERO_ARR (TempBuf);
}

void SetStringSUtf8Safe (wchar_t *&Target, const char *Str, size_t *&Indexation, const wchar_t c)
{
  iconv_t Converter = iconv_open ("UCS-2LE", "UTF-8");
  CLEAN_AND_ZERO_ARR (Target);
  CLEAN_AND_ZERO_ARR (Indexation);
  size_t InSize = 0;
  size_t OutLength = Utf8Length (Str) + 1;
  Target = new wchar_t[OutLength];
  Indexation = new size_t[OutLength];
  size_t OutSize = OutLength * 2;
  int CurrentIndex = 0;
  Indexation[0] = 0;
  size_t res = (size_t) -1;
  const char *OriginalStr = Str;
  char *OutBuf = (char *) Target;
  while (*Str)
  {
    InSize = Utf8GetCharSize (*Str);
    if (!InSize)
      break;
    res = iconv (Converter, &Str, &InSize, &OutBuf, &OutSize);
    if (res == (size_t) -1)
    {
      // If char was not convertible then it will be substituted with char c
      *(wchar_t *)(OutBuf) = c;
      OutBuf += 2;
    }
    CurrentIndex++;
    Indexation[CurrentIndex] = Str - OriginalStr;
  }
  *(wchar_t *) OutBuf = L'\0';
  iconv_close (Converter);
}

// In case source is in utf-8
void SetStringSUtf8 (char *&Target, const char *Str)
{
  if (Target == Str)
    return;
  iconv_t Converter = iconv_open ("CHAR//IGNORE", "UTF-8");
  CLEAN_AND_ZERO_ARR (Target);
  size_t InSize = 0;
  size_t OutSize = Utf8Length (Str) + 1;
  size_t PrevOutSize = 0;
  Target = new char[OutSize];
  size_t res = (size_t) -1;
  char *OutBuf = (char *) Target;
  while (*Str)
  {
    InSize = Utf8GetCharSize (*Str);
    if (!InSize)
      break;
    PrevOutSize = OutSize;
    res = iconv (Converter, &Str, &InSize, &OutBuf, &OutSize);

    if (PrevOutSize - OutSize > 1)
    {
      // If char is multichar then we count it as not converted
      OutBuf -= (PrevOutSize - OutSize);
      *(OutBuf) = 0;
      OutSize = PrevOutSize;
    }
  }
  *(OutBuf) = 0;
  iconv_close (Converter);
}

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const char *Str)
{
  if (Target == Str)
    return;
  iconv_t Converter = iconv_open ("UTF-8//IGNORE", "CHAR");
  CLEAN_AND_ZERO_ARR (Target);
  size_t InSize = strlen (Str) + 1;
  size_t OutSize = 6 * strlen (Str) + 1; // Maximum Possible UTF-8 size
  char *TempBuf = new char[OutSize];
  char *OutBuf = (char *) TempBuf;
  size_t res = iconv (Converter, (const char **) &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);
  if (res == (size_t)(-1))
  {
    Target = new char[1];
    *Target = '\0';
    CLEAN_AND_ZERO_ARR (TempBuf)
      return;
  }
  SetString (Target, TempBuf); // Cutting off unnecessary symbols.
  CLEAN_AND_ZERO_ARR (TempBuf);
}

void SetStringToFirstUtf8Char (char *&Dest, const char *Source)
{
  if (Dest == Source)
    return;
  CLEAN_AND_ZERO_ARR (Dest);
  if (!*Source)
    return;
  size_t Len = Utf8GetCharSize (*Source);
  Dest = new char[Len + 1];
  strncpy (Dest, Source, Len);
  Dest[Len] = '\0';
}

// This function is more or less transferred from gcc source
BOOL MatchSpecialChar (wchar_t *Dest, wchar_t *&Source)
{
  int len, i;
  wchar_t c, n;
  BOOL m;

  m = TRUE;

  switch (c = *(Source++))
  {
  case 'a':
    *Dest = '\a';
    break;
  case 'b':
    *Dest = '\b';
    break;
  case 't':
    *Dest = '\t';
    break;
  case 'f':
    *Dest = '\f';
    break;
  case 'n':
    *Dest = '\n';
    break;
  case 'r':
    *Dest = '\r';
    break;
  case 'v':
    *Dest = '\v';
    break;
  case '\\':
    *Dest = '\\';
    break;
  case '0':
    *Dest = '\0';
    break;

  case 'x':
  case 'u':
  case 'U':
    /* Hexadecimal form of wide characters.  */
    len = (c == 'x' ? 2 : (c == 'u' ? 4 : 8));
    n = 0;
    for (i = 0; i < len; i++)
    {
      char buf[2] = { '\0', '\0' };

      c = *(Source++);
      if (c > UCHAR_MAX
        || !(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')))
        return FALSE;

      buf[0] = (unsigned char) c;
      n = n << 4;
      n += (wchar_t) strtol (buf, NULL, 16);
    }
    *Dest = n;
    break;

  default:
    /* Unknown backslash codes are simply not expanded.  */
    m = FALSE;
    break;
  }
  return m;
}

void SetParsedString (wchar_t *&Dest, wchar_t *Source)
{
  Dest = new wchar_t [wcslen (Source) + 1];
  wchar_t *LastPos = 0;
  wchar_t *ResString = Dest;
  while (*Source)
  {
    LastPos = Source;
    if (*Source == '\\')
    {
      Source++;
      if (!MatchSpecialChar (Dest, Source))
      {
        Source = LastPos;
        *Dest = *(Source++);
        Dest++;
      }
      else
      {
        Dest++;
      }
    }
    else
    {
      *Dest = *(Source++);
      Dest++;
    }
  }
  *Dest = 0;
  Dest = ResString;
}

HWND GetScintillaWindow(const NppData *NppDataArg)
{
  int which = -1;
  SendMessage(NppDataArg->_nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
  if (which == -1)
    return NULL;
  if (which == 1)
    return NppDataArg->_scintillaSecondHandle;
  return (which == 0) ? NppDataArg->_scintillaMainHandle : NppDataArg->_scintillaSecondHandle;
}

void SendMsgToBothEditors (const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  SendMessage (NppDataArg->_scintillaMainHandle, Msg, wParam, lParam);
  SendMessage (NppDataArg->_scintillaSecondHandle, Msg, wParam, lParam);
}

LRESULT SendMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  HWND wEditor = GetScintillaWindow(NppDataArg);
  return SendMessage (wEditor, Msg, wParam, lParam);
}

LRESULT SendMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return SendMessage (ScintillaWindow, Msg, wParam, lParam);
}

LRESULT SendMsgToNpp (const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return SendMessage (NppDataArg->_nppHandle, Msg, wParam, lParam);
}

// Remember: it's better to use PostMsg wherever possible, to avoid gui update on each message send etc etc
// Also it's better to avoid get current scintilla window too much times, since it's obviously uses 1 SendMsg call
LRESULT PostMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  HWND wEditor = GetScintillaWindow(NppDataArg);
  return PostMessage(wEditor, Msg, wParam, lParam);
}

LRESULT PostMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return PostMessage(ScintillaWindow, Msg, wParam, lParam);
}

// These functions are mostly taken from http://research.microsoft.com

BOOL Utf8IsLead (char c)
{
  return (((c & 0x80) == 0)                       // 0xxxxxxx
    || ((c & 0xC0) == 0xC0 && (c & 0x20) == 0)    // 110xxxxx
    || ((c & 0xE0) == 0xE0 && (c & 0x10) == 0)    // 1110xxxx
    || ((c & 0xF0) == 0xF0 && (c & 0x08) == 0)    // 11110xxx
    || ((c & 0xF8) == 0xF8 && (c & 0x04) == 0)   // 111110xx
    || ((c & 0xFC) == 0xFC && (c & 0x02) == 0));
}

BOOL Utf8IsCont (char c)
{
  return ((c & 0x80) == 0x80 && (c & 0x40) == 0);  // 10xxxxx
}

char *Utf8Dec(const char *string, const char *current)
{
  const char *temp;
  if (string > current)
    return 0;

  temp = current - 1;
  while ( string <= temp && (!Utf8IsLead (*temp) ))
    temp--;

  return (char *) temp;
}

char *Utf8Inc (const char *string)
{
  const char *temp;
  temp = string + 1;
  while (*temp && !Utf8IsLead (*temp))
    temp++;

  return (char *) temp;
}

int Utf8GetCharSize (char c)
{
  if ((c & 0x80) == 0)
    return 1;
  else if ((c & 0xC0) > 0 && (c & 0x20) == 0)
    return 2;
  else if ((c & 0xE0) > 0 && (c & 0x10) == 0)
    return 3;
  else if ((c & 0xF0) > 0 && (c & 0x08) == 0)
    return 4;
  else if ((c & 0xF8) > 0 && (c & 0x04) == 0)
    return 5;
  else if ((c & 0xFC) > 0 && (c & 0x02) == 0)
    return 6;
  return 0;
}

BOOL Utf8FirstCharsAreEqual (const char *Str1, const char *Str2)
{
  int FirstCharSize1 = Utf8GetCharSize (*Str1);
  int FirstCharSize2 = Utf8GetCharSize (*Str2);
  if (FirstCharSize1 != FirstCharSize2)
    return FALSE;
  return (strncmp (Str1, Str2, FirstCharSize1) == 0);
}

char *Utf8pbrk(const char *s, const char *set)
{
  const char *x;
  for (; *s; s = Utf8Inc (s))
    for (x = set; *x; x = Utf8Inc (x))
      if (Utf8FirstCharsAreEqual (s, x))
        return (char *) s;
  return NULL;
}

long Utf8spn (const char *s, const char *set)
{
  const char *x;
  const char *it = 0;
  it = s;

  for (; *it; it = Utf8Inc (it)) {
    for (x = set; *x; x = Utf8Inc (x))
    {
      if (Utf8FirstCharsAreEqual (it, x))
        goto continue_outer;
    }
    break;
continue_outer:;
  }
  return it - s;
}

char *Utf8chr (const char *s, const char *sfc) // Char is first from the string sfc (string with first char)
{
  while (*s)
  {
    if (s && Utf8FirstCharsAreEqual (s, sfc))
      return (char *) s;
    s = Utf8Inc (s);
  }
  return 0;
}

char * Utf8strtok (char *s1, const char *Delimit, char **Context)
{
  char *tmp;

  /* Skip leading delimiters if new string. */
  if ( s1 == NULL ) {
    s1 = *Context;
    if (s1 == NULL)         /* End of story? */
      return NULL;
  }
  s1 += Utf8spn (s1, Delimit);

  /* Find end of segment */
  tmp = Utf8pbrk (s1, Delimit);
  if (tmp) {
    /* Found another delimiter, split string and save state. */
    *tmp = '\0';
    tmp++;
    while (!Utf8IsLead (*(tmp)))
    {
      *tmp = '\0';
      tmp++;
    }

    *Context = tmp;
  } else {
    /* Last segment, remember that. */
    *Context = NULL;
  }

  return s1;
}

size_t Utf8Length (const char *String)
{
  char *It = const_cast <char *> (String);
  size_t Size = 0;
  while (*It)
  {
    Size++;
    It = Utf8Inc (It);
  }
  return Size;
}

bool SortCompare(wchar_t *a, wchar_t *b)
{
  return wcscmp (a, b) < 0;
}

bool EquivCharStrings (char *a, char *b)
{
  return (strcmp (a, b) == 0);
}

size_t HashCharString (char *a)
{
  size_t Hash = 7;
  for(unsigned int i = 0; i < strlen (a); i++)
  {
    Hash = Hash * 31 + a[i];
  }
  return Hash;
}

bool Equivwchar_tStrings (wchar_t *a, wchar_t *b)
{
  return (wcscmp (a, b) == 0);
}

size_t Hashwchar_tString (wchar_t *a)
{
  size_t Hash = 7;
  for(unsigned int i = 0; i < wcslen (a); i++)
  {
    Hash = Hash * 31 + a[i];
  }
  return Hash;
}

bool SortCompareChars (char *a, char *b)
{
  return strcmp (a, b) < 0;
}

bool SortCompareWChars (wchar_t *a, wchar_t *b)
{
  return wcscmp (a, b) < 0;
}

const wchar_t *const AliasesFrom[] = {L"af_Za", L"ak_GH", L"bg_BG", L"ca_ANY", L"ca_ES", L"cop_EG", L"cs_CZ", L"cy_GB", L"da_DK", L"de_AT", L"de_CH", L"de_DE",
  L"de_DE_comb", L"de_DE_frami", L"de_DE_neu", L"el_GR", L"en_AU", L"en_CA", L"en_GB", L"en_GB-oed", L"en_NZ", L"en_US", L"en_ZA", L"eo_EO", L"es_AR",
  L"es_BO", L"es_CL", L"es_CO", L"es_CR", L"es_CU", L"es_DO", L"es_EC", L"es_ES", L"es_GT", L"es_HN", L"es_MX", L"es_NEW", L"es_NI", L"es_PA", L"es_PE",
  L"es_PR", L"es_PY", L"es_SV", L"es_UY", L"es_VE", L"et_EE",  L"fo_FO", L"fr_FR", L"fr_FR-1990", L"fr_FR-1990_1-3-2", L"fr_FR-classique", L"fr_FR-classique_1-3-2", L"fr_FR_1-3-2", L"fy_NL",
  L"ga_IE", L"gd_GB", L"gl_ES", L"gu_IN", L"he_IL", L"hi_IN", L"hil_PH", L"hr_HR", L"hu_HU", L"ia", L"id_ID", L"is_IS", L"it_IT", L"ku_TR",
  L"la", L"lt_LT", L"lv_LV", L"mg_MG", L"mi_NZ", L"mk_MK", L"mos_BF", L"mr_IN", L"ms_MY", L"nb_NO", L"ne_NP", L"nl_NL", L"nn_NO", L"nr_ZA",
  L"ns_ZA", L"ny_MW", L"oc_FR", L"pl_PL", L"pt_BR", L"pt_PT", L"ro_RO", L"ru_RU", L"ru_RU_ie", L"ru_RU_ye", L"ru_RU_yo", L"rw_RW", L"si_SI", L"sk_SK", L"sq_AL",
  L"ss_ZA", L"st_ZA", L"sv_SE", L"sw_KE",  L"tet_ID", L"th_TH", L"tl_PH", L"tn_ZA", L"ts_ZA", L"uk_UA", L"ur_PK", L"ve_ZA", L"vi-VN", L"xh_ZA", L"zu_ZA"
};
const wchar_t *const AliasesTo[] = {L"Afrikaans", L"Akan", L"Bulgarian", L"Catalan (Any)", L"Catalan (Spain)", L"Coptic (Bohairic dialect)", L"Czech", L"Welsh",
  L"Danish", L"German (Austria)", L"German (Switzerland)", L"German (Germany)", L"German (Old and New Spelling)", L"German (Additional)", L"German (New Spelling)",
  L"Greek", L"English (Australia)", L"English (Canada)", L"English (Great Britain)", L"English (Great Britain [OED])", L"English (New Zealand)",
  L"English (United States)", L"English (South Africa)", L"Esperanto", L"Spanish (Argentina)", L"Spanish (Bolivia)", L"Spanish (Chile)", L"Spanish (Colombia)", L"Spanish (Costa Rica",
  L"Spanish (Cuba)", L"Spanish (Dominican Republic)", L"Spanish (Ecuador)", L"Spanish (Spain)", L"Spanish (Guatemala)", L"Spanish (Honduras)", L"Spanish (Mexico)",
  L"Spanish (New)", L"Spanish (Nicaragua)", L"Spanish (Panama)", L"Spanish (Peru)", L"Spanish (Puerto Rico)", L"Spanish (Paraguay)", L"Spanish (El Salvador)", L"Spanish (Uruguay)",
  L"Spanish (Bolivarian Republic of Venezuela)", L"Estonian", L"Faroese", L"French", L"French (1990)", L"French (1990)", L"French (Classique)", L"French (Classique)", L"French", L"Frisian", L"Irish", L"Scottish Gaelic",
  L"Galician", L"Gujarati", L"Hebrew", L"Hindi", L"Filipino", L"Croatian", L"Hungarian", L"Interlingua", L"Indonesian", L"Icelandic", L"Italian", L"Kurdish",
  L"Latin", L"Lithuanian", L"Latvian", L"Malagasy", L"Maori", L"Macedonian (FYROM)", L"Mossi", L"Marathi", L"Malay", L"Norwegian (Bokmal)", L"Nepali", L"Dutch",
  L"Norwegian (Nynorsk)", L"Ndebele", L"Northern Sotho", L"Chichewa", L"Occitan", L"Polish", L"Portuguese (Brazil)", L"Portuguese (Portugal)", L"Romanian", L"Russian",
  L"Russian (without io)", L"Russian (without io)", L"Russian (with io)", L"Kinyarwanda", L"Slovenian", L"Slovak", L"Alban", L"Swazi", L"Northern Sotho", L"Swedish",
  L"Kiswahili", L"Tetum", L"Thai", L"Tagalog", L"Setswana", L"Tsonga", L"Ukrainian", L"Urdu", L"Venda", L"Vietnamese", L"isiXhosa", L"isiZulu"
};

// Language Aliases
BOOL SetStringWithAliasApplied (wchar_t *&Target, wchar_t *OrigName)
{
  int Left, Right;
  Left = 0;
#ifdef _DEBUG
  for (int i = 0; i < countof (AliasesFrom) - 1; i++)
  {
    if (wcscmp (AliasesFrom[i], AliasesFrom[i + 1]) >= 0)
    {
      DebugBreak ();
      // String are not sorted correctly in AliasesFrom
    }
  }
#endif
  Right = countof (AliasesFrom);
  while (1)
  {
    int CentralElement = (Left + Right) / 2;
    int CompareResult = wcscmp (AliasesFrom[CentralElement], OrigName);
    if (CompareResult == 0)
    {
      SetString (Target, AliasesTo[CentralElement]);
      return TRUE;
    }
    else if (Right - Left <= 1)
      break;
    else if (CompareResult < 0)
      Left = CentralElement;
    else
      Right = CentralElement;
  }
  SetString (Target, OrigName);
  return FALSE;
}

static BOOL TryToCreateDir (wchar_t *Path, BOOL Silent, HWND NppWindow)
{
  if (!CreateDirectory (Path, NULL))
  {
    if (!Silent)
    {
      if (!NppWindow)
        return FALSE;

      wchar_t Message[DEFAULT_BUF_SIZE];
      _swprintf (Message, L"Can't create directory %s", Path);
      MessageBoxInfo MsgBox (NppWindow, Message, L"Error in directory creation", MB_OK | MB_ICONERROR);
      SendMessage (NppWindow, GetCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
    }
    return FALSE;
  }
  return TRUE;
}

BOOL CheckForDirectoryExistence (wchar_t *Path, BOOL Silent, HWND NppWindow)
{
  for (unsigned int i = 0; i < wcslen (Path); i++)
  {
    if (Path[i] == L'\\')
    {
      Path[i] = L'\0';
      if (!PathFileExists (Path))
      {
        if (!TryToCreateDir (Path, Silent, NppWindow))
          return FALSE;
      }
      Path[i] = L'\\';
    }
  }
  if (!PathFileExists (Path))
  {
    if (!TryToCreateDir (Path, Silent, NppWindow))
      return FALSE;
  }
  return TRUE;
}

// Copied from corresponding Wikipedia page and modified
static int LevenshteinDistanceUtf8 (char *S, char *T)
{
  char *SIterator;
  char *TIterator;
  // degenerate cases
  if (strcmp (S,T) == 0) return 0;
  size_t SLength = Utf8Length (S);
  size_t TLength = Utf8Length (T);
  if (SLength == 0) return TLength;
  if (TLength == 0) return SLength;

  // create two work vectors of integer distances
  int *V0 = new int[TLength + 1];
  int *V1 = new int[TLength + 1];

  // initialize v0 (the previous row of distances)
  // this row is A[0][i]: edit distance for an empty s
  // the distance is just the number of characters to delete from t

  for (size_t i = 0; i < TLength + 1; i++)
    V0[i] = i;

  SIterator = S;
  for (size_t i = 0; i < SLength; i++, SIterator = Utf8Inc (SIterator))
  {
    // calculate v1 (current row distances) from the previous row v0

    // first element of v1 is A[i+1][0]
    // edit distance is delete (i+1) chars from s to match empty t
    V1[0] = i + 1;

    // use formula to fill in the rest of the row
    TIterator = T;
    for (size_t j = 0; j < TLength; j++, TIterator = Utf8Inc (TIterator))
    {
      int cost = Utf8FirstCharsAreEqual (SIterator, TIterator) ? 0 : 1;
      V1[j + 1] = min (V1[j] + 1, min (V0[j + 1] + 1, V0[j] + cost));
    }

    // copy v1 (current row) to v0 (previous row) for next iteration
    for (size_t j = 0; j < TLength + 1; j++)
      V0[j] = V1[j];
  }

  int Result = V1[TLength];
  CLEAN_AND_ZERO_ARR (V0);
  CLEAN_AND_ZERO_ARR (V1);
  return Result;
}

static BOOL _min (int a, int b)
{
  return a < b ? a : b;
}

static BOOL _max (int a, int b)
{
  return a > b ? a : b;
}

static int SubstitionCost = 2;
static int TranspositionCost = 0;
static int DeletionCost = 3;
static int InsertionCost = 1;

static int DamerauLevenshteinDistanceUtf8 (char *Source, char *Target)
{
  size_t SourceLength = Utf8Length (Source);
  size_t TargetLength = Utf8Length (Target);
  if (Source == 0 || *Source == 0)
  {
    if (Target == 0 || *Target == 0)
    {
      return 0;
    }
    else
    {
      return TargetLength;
    }
  }
  else if (Target == 0 || *Target == 0)
  {
    return SourceLength;
  }

  int W = (SourceLength + 2);
  int H = (TargetLength + 2);
  int *Score = new int[(SourceLength + 2) * (TargetLength + 2)];

  int Inf = SourceLength + TargetLength;
  Score[0] = Inf;
  for (size_t i = 0; i <= SourceLength; i++) { Score[(i + 1) * H + 1] = i; Score [(i + 1) * H + 0] = Inf; }
  for (size_t j = 0; j <= TargetLength; j++) { Score[1 * H + (j + 1)] = j; Score [0 * H + (j + 1)] = Inf; }

  stdext::hash_map <char *, int, hash_compare_strings> Sd;

  char *Iterators[2] = {Source, Target};
  for (int i = 0; i < countof (Iterators); i++)
  {
    while (TRUE)
    {
      char *TempString = 0;
      if (!*Iterators[i])
        break;
      SetStringToFirstUtf8Char (TempString, Iterators[i]);
      if (Sd.find (TempString) == Sd.end ())
        Sd[TempString] = 0;
      else
        CLEAN_AND_ZERO_ARR (TempString);
      Iterators[i] = Utf8Inc (Iterators[i]);
    }
  }

  char *SourceIterator = Source;
  for (size_t i = 1; i <= SourceLength; i++, SourceIterator = Utf8Inc (SourceIterator))
  {
    int DB = 0;
    char *TargetIterator = Target;
    for (size_t j = 1; j <= TargetLength; j++, TargetIterator = Utf8Inc (TargetIterator))
    {
      char *TempString = 0;
      SetStringToFirstUtf8Char (TempString, TargetIterator);

      int i1 = Sd[TempString];
      int j1 = DB;
      CLEAN_AND_ZERO_ARR (TempString);

      if (Source[i - 1] == Target[j - 1])
      {
        Score[(i + 1) * H + (j + 1)] = Score[i * H + j];
        DB = j;
      }
      else
      {
        Score[(i + 1) * H + (j + 1)] = std::min<int> (Score[i * H + j] + SubstitionCost, std::min<int> (Score[(i + 1) * H + j] + InsertionCost, Score[i * H + (j + 1)] + DeletionCost));
      }

      Score[(i + 1) * H + (j + 1)] = std::min<int> (Score[(i + 1) * H + (j + 1)], Score[i1 * H + j1] + (i - i1 - 1) + TranspositionCost + (j - j1 - 1));
    }
    char *TempString = 0;
    SetStringToFirstUtf8Char (TempString, SourceIterator);
    Sd[TempString] = i;
    CLEAN_AND_ZERO_ARR (TempString);
  }

  int Result = Score[(SourceLength + 1) * H  + (TargetLength + 1)];
  CLEAN_AND_ZERO_ARR (Score);

  stdext::hash_map <char *, int, hash_compare_strings>::iterator It;
  for (It = Sd.begin (); It != Sd.end (); ++It)
    delete [](*It).first;

  return Result;
}

static int DamerauLevenshteinDistanceWchar (wchar_t *Source, wchar_t *Target)
{
  // Hopefully only one memory allocation (which is good)
  size_t SourceLength = wcslen (Source);
  size_t TargetLength = wcslen (Target);
  if (Source == 0 || *Source == 0)
  {
    if (Target == 0 || *Target == 0)
    {
      return 0;
    }
    else
    {
      return TargetLength;
    }
  }
  else if (Target == 0 || *Target == 0)
  {
    return SourceLength;
  }

  int W = (SourceLength + 2);
  int H = (TargetLength + 2);
  int *Score = new int[(SourceLength + 2) * (TargetLength + 2)];

  int Inf = SourceLength + TargetLength;
  Score[0] = Inf;
  for (size_t i = 0; i <= SourceLength; i++) { Score[(i + 1) * H + 1] = i; Score [(i + 1) * H + 0] = Inf; }
  for (size_t j = 0; j <= TargetLength; j++) { Score[1 * H + (j + 1)] = j; Score [0 * H + (j + 1)] = Inf; }

  stdext::hash_map <wchar_t, int, hash_compare_wchars_icase> Sd;

  wchar_t *Iterators[2] = {Source, Target};
  for (int i = 0; i < countof (Iterators); i++)
  {
    while (TRUE)
    {
      wchar_t Char; // TODO:Rename
      if (!*Iterators[i])
        break;
      Char = *Iterators[i];
      if (Sd.find (Char) == Sd.end ())
        Sd[Char] = 0;
      Iterators[i]++;
    }
  }

  wchar_t *SourceIterator = Source;
  for (size_t i = 1; i <= SourceLength; i++, SourceIterator++)
  {
    int DB = 0;
    wchar_t *TargetIterator = Target;
    for (size_t j = 1; j <= TargetLength; j++, TargetIterator++)
    {
      int i1 = Sd[*TargetIterator];
      int j1 = DB;

      if (towupper (Source[i - 1]) == towupper (Target[j - 1]))
      {
        Score[(i + 1) * H + (j + 1)] = Score[i * H + j];
        DB = j;
      }
      else
      {
        Score[(i + 1) * H + (j + 1)] = std::min<int> (Score[i * H + j] + SubstitionCost, std::min<int> (Score[(i + 1) * H + j] + InsertionCost, Score[i * H + (j + 1)]) + DeletionCost) ;
      }

      Score[(i + 1) * H + (j + 1)] = std::min<int> (Score[(i + 1) * H + (j + 1)], Score[i1 * H + j1] + (i - i1 - 1) + TranspositionCost + (j - j1 - 1));
    }
    Sd[*SourceIterator] = i;
  }

  int Result = Score[(SourceLength + 1) * H  + (TargetLength + 1)];
  CLEAN_AND_ZERO_ARR (Score);

  return Result;
}

// Second version took from repository of git itself (https://github.com/git/git/blob/master/levenshtein.c)
int DamerauLevenshteinDistanceWchar2 (const wchar_t *string1, const wchar_t *string2, int w, int s, int a, int d)
{
  int len1 = wcslen (string1), len2 = wcslen (string2);
  int *row0 = new int[len2 + 1];
  int *row1 = new int[len2 + 1];
  int *row2 = new int[len2 + 1];
  int i, j;

  for (j = 0; j <= len2; j++)
    row1[j] = j * a;
  for (i = 0; i < len1; i++) {
    int *dummy;

    row2[0] = (i + 1) * d;
    for (j = 0; j < len2; j++) {
      /* substitution */
      row2[j + 1] = row1[j] + s * (towupper (string1[i]) != towupper (string2[j]));
      /* swap */
      if (i > 0 && j > 0 && towupper (string1[i - 1]) == towupper (string2[j]) &&
        towupper (string1[i]) == towupper (string2[j - 1]) &&
        row2[j + 1] > row0[j - 1] + w)
        row2[j + 1] = row0[j - 1] + w;
      /* deletion */
      if (row2[j + 1] > row1[j + 1] + d)
        row2[j + 1] = row1[j + 1] + d;
      /* insertion */
      if (row2[j + 1] > row2[j] + a)
        row2[j + 1] = row2[j] + a;
    }

    dummy = row0;
    row0 = row1;
    row1 = row2;
    row2 = dummy;
  }

  i = row1[len2];
  CLEAN_AND_ZERO_ARR (row0);
  CLEAN_AND_ZERO_ARR (row1);
  CLEAN_AND_ZERO_ARR (row2);

  return i;
}

class LevenshteinSortComparatorUtf8
{
public:
  char * T;
  LevenshteinSortComparatorUtf8 (char *TArg) { T = TArg; }
  bool operator()(char *S1, char *S2)
  {
    return (LevenshteinDistanceUtf8 (S1, T) < LevenshteinDistanceUtf8 (S2, T));
  }
};

class DamerauLevenshteinSortComparatorUtf8
{
public:
  char * T;
  DamerauLevenshteinSortComparatorUtf8 (char *TArg) { T = TArg; }
  bool operator()(char *S1, char *S2)
  {
    return (DamerauLevenshteinDistanceUtf8 (S1, T) < DamerauLevenshteinDistanceUtf8 (S2, T));
  }
};

class DamerauLevenshteinSortComparatorWchar
{
public:
  wchar_t * T;
  DamerauLevenshteinSortComparatorWchar (wchar_t *TArg) { T = TArg; }
  bool operator()(wchar_t *S1, wchar_t *S2)
  {
    // 0, 2, 1, 3
    return (DamerauLevenshteinDistanceWchar2 (S1, T, TranspositionCost, SubstitionCost, InsertionCost, DeletionCost) <
            DamerauLevenshteinDistanceWchar2 (S2, T, TranspositionCost, SubstitionCost, InsertionCost, DeletionCost));
  }
};

void SortStringVectorByLevenshteinDistanceUtf8 (std::vector <char *> *Vector, char *Word)
{
  std::sort (Vector->begin (), Vector->end (), LevenshteinSortComparatorUtf8 (Word));
}

void SortStringVectorByDamerauLevenshteinDistanceUtf8 (std::vector <char *> *Vector, char *Word)
{
  std::sort (Vector->begin (), Vector->end (), DamerauLevenshteinSortComparatorUtf8 (Word));
}

void SortStringVectorByDamerauLevenshteinDistanceWchar (std::vector <wchar_t *> *Vector, wchar_t *Word)
{
  std::sort (Vector->begin (), Vector->end (), DamerauLevenshteinSortComparatorWchar (Word));
}


void StripEqualElements (std::vector <wchar_t *> *&Vector)
{
  std::sort (Vector->begin (), Vector->end (), SortCompareWChars);
  std::vector <wchar_t *> *NewVector = new std::vector <wchar_t *>;
  for (size_t i = 0; i < Vector->size (); i++)
  {
    if (i == 0 || wcscmp (Vector->at (i), Vector->at (i - 1)) != 0)
      NewVector->push_back (Vector->at (i));
  }

  for (size_t i = 1; i < Vector->size (); i++)
  {
    if (wcscmp (Vector->at (i), Vector->at (i - 1)) == 0)
      delete [] Vector->at (i);
  }
  CLEAN_AND_ZERO (Vector);
  Vector = NewVector;
}

wchar_t *GetLastSlashPosition (wchar_t *Path)
{
  return wcschr (Path, L'\\');
}

// strtok_s, delimiters are all non-alphabetic and non-numeric characters except one in delim
char *strtok_s_nonalnum (char *s1, const char *delimit, char **context)
{
    char *tmp;

    /* Skip leading delimiters if new string. */
    if ( s1 == NULL ) {
        s1 = *context;
        if (s1 == NULL)         /* End of story? */
            return NULL;
    }
    // Well, we should skip all the freaking delimiters here
    // Our delimiters are non-alphabetic except the shit in delimit array, so...
    while (*s1 && (!isalnum ((unsigned char) *s1) && strchr (delimit, *s1)))
      s1++;

    /* Find end of segment */
    // Now we're skipping all the alnum shit and shit found in delimit
    tmp =  s1;
    while (*tmp && (isalnum ((unsigned char) *tmp) || strchr (delimit, *tmp)))
      tmp++;
    if (!*tmp)
      tmp = 0;
    if (tmp) {
        /* Found another delimiter, split string and save state. */
        *tmp = '\0';
        *context = tmp + 1;
    } else {
        /* Last segment, remember that. */
        *context = NULL;
    }

    return s1;
}

// wcstok_s, delimiters are all non-alphabetic and non-numeric characters except one in delim
wchar_t *wcstok_s_nonalnum (wchar_t *s1, const wchar_t *delimit, wchar_t **context)
{
    wchar_t *tmp;

    /* Skip leading delimiters if new string. */
    if ( s1 == NULL ) {
        s1 = *context;
        if (s1 == NULL)         /* End of story? */
            return NULL;
    }
    // Well, we should skip all the freaking delimiters here
    // Our delimiters are non-alphabetic except the shit in delimit array, so...
    while (*s1 && (!iswalnum (*s1) && !wcschr (delimit, *s1)))
      s1++;

    /* Find end of segment */
    // Now we're skipping all the alnum shit and shit found in delimit
    tmp =  s1;
    while (*tmp && (iswalnum (*tmp) || wcschr (delimit, *tmp)))
      tmp++;
    if (!*tmp)
      tmp = 0;
    if (tmp) {
        /* Found another delimiter, split string and save state. */
        *tmp = '\0';
        *context = tmp + 1;
    } else {
        /* Last segment, remember that. */
        *context = NULL;
    }

    return s1;
}

void CleanAndZeroWordList (WordSet *&WordListInstance)
{
  WordSet::iterator it = WordListInstance->begin ();
  for (; it != WordListInstance->end (); ++it)
  {
    delete [] (*it);
  }
  CLEAN_AND_ZERO (WordListInstance);
}

vector<wchar_t> GetPrivateProfileString_optimized  (const wchar_t *lpAppName, const wchar_t *lpKeyName, const wchar_t *lpDefault, const wchar_t *lpFileName)
{
  static const int initial_size = 64;
  vector<wchar_t> t (initial_size);
  int sizeReceived = 0;
  do
    {
      int sizeBound = sizeReceived * 3 / 2;
      if ((int) t.size () < sizeBound)
        t.resize (sizeBound);
      sizeReceived = GetPrivateProfileString (lpAppName, lpKeyName, lpDefault, t.data (), t.size (), lpFileName);
    }
  while ((int) t.size () <= sizeReceived);
  return t;
}

bool str2int (int &i, const wchar_t *s, int base = 10)
{
    long  l;
    wchar_t *end;
    l = wcstol (s, &end, base);

    if (*s == L'\0' || *end != L'\0')
        return false;

    i = static_cast<int> (l);
    return true;
}
