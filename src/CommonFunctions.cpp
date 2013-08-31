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
  _tcscpy (Target, Str);
}

void SetString (char *&Target, const wchar_t *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  size_t OutSize = _tcslen (Str) + 1;
  size_t PrevOutSize = 0;
  Target = new char[OutSize];
  char *OutBuf = Target;
  size_t res = (size_t) -1;

  iconv_t Converter = iconv_open ("CHAR//IGNORE", "UCS-2LE");
  size_t InSize = (_tcslen (Str) + 1) * sizeof (wchar_t);

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
  size_t InSize = (_tcslen (Str) + 1) * sizeof (wchar_t);
  size_t OutSize = 6 * _tcslen (Str) + 1; // Maximum Possible UTF-8 size
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
BOOL MatchSpecialChar (TCHAR *Dest, TCHAR *&Source)
{
  int len, i;
  TCHAR c, n;
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
#ifndef UNICODE
    len = 2;
#endif
    for (i = 0; i < len; i++)
    {
      char buf[2] = { '\0', '\0' };

      c = *(Source++);
      if (c > UCHAR_MAX
        || !(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')))
        return FALSE;

      buf[0] = (unsigned char) c;
      n = n << 4;
      n += (TCHAR) strtol (buf, NULL, 16);
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

void SetParsedString (TCHAR *&Dest, TCHAR *Source)
{
  Dest = new TCHAR [_tcslen (Source) + 1];
  TCHAR *LastPos = 0;
  TCHAR *ResString = Dest;
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
    else
      s1 += Utf8spn (s1, Delimit);
  } else {
    s1 += Utf8spn (s1, Delimit);
  }

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

bool SortCompare(TCHAR *a, TCHAR *b)
{
  return _tcscmp (a, b) < 0;
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

bool EquivTCHARStrings (TCHAR *a, TCHAR *b)
{
  return (_tcscmp (a, b) == 0);
}

size_t HashTCHARString (TCHAR *a)
{
  size_t Hash = 7;
  for(unsigned int i = 0; i < _tcslen (a); i++)
  {
    Hash = Hash * 31 + a[i];
  }
  return Hash;
}

bool SortCompareChars (char *a, char *b)
{
  return strcmp (a, b) < 0;
}

const TCHAR *const AliasesFrom[] = {_T ("af_Za"), _T ("ak_GH"), _T ("bg_BG"), _T ("ca_ANY"), _T ("ca_ES"), _T ("cop_EG"), _T ("cs_CZ"), _T ("cy_GB"), _T ("da_DK"), _T ("de_AT"), _T ("de_CH"), _T ("de_DE"),
  _T ("de_DE_comb"), _T ("de_DE_frami"), _T ("de_DE_neu"), _T ("el_GR"), _T ("en_AU"), _T ("en_CA"), _T ("en_GB"), _T ("en_GB-oed"), _T ("en_NZ"), _T ("en_US"), _T ("en_ZA"), _T ("eo_EO"), _T ("es_AR"),
  _T ("es_BO"), _T ("es_CL"), _T ("es_CO"), _T ("es_CR"), _T ("es_CU"), _T ("es_DO"), _T ("es_EC"), _T ("es_ES"), _T ("es_GT"), _T ("es_HN"), _T ("es_MX"), _T ("es_NEW"), _T ("es_NI"), _T ("es_PA"), _T ("es_PE"),
  _T ("es_PR"), _T ("es_PY"), _T ("es_SV"), _T ("es_UY"), _T ("es_VE"), _T ("et_EE"),  _T ("fo_FO"), _T ("fr_FR"), _T ("fr_FR-1990"), _T ("fr_FR-1990_1-3-2"), _T ("fr_FR-classique"), _T ("fr_FR-classique_1-3-2"), _T ("fr_FR_1-3-2"), _T ("fy_NL"),
  _T ("ga_IE"), _T ("gd_GB"), _T ("gl_ES"), _T ("gu_IN"), _T ("he_IL"), _T ("hi_IN"), _T ("hil_PH"), _T ("hr_HR"), _T ("hu_HU"), _T ("ia"), _T ("id_ID"), _T ("is_IS"), _T ("it_IT"), _T ("ku_TR"),
  _T ("la"), _T ("lt_LT"), _T ("lv_LV"), _T ("mg_MG"), _T ("mi_NZ"), _T ("mk_MK"), _T ("mos_BF"), _T ("mr_IN"), _T ("ms_MY"), _T ("nb_NO"), _T ("ne_NP"), _T ("nl_NL"), _T ("nn_NO"), _T ("nr_ZA"),
  _T ("ns_ZA"), _T ("ny_MW"), _T ("oc_FR"), _T ("pl_PL"), _T ("pt_BR"), _T ("pt_PT"), _T ("ro_RO"), _T ("ru_RU"), _T ("ru_RU_ie"), _T ("ru_RU_ye"), _T ("ru_RU_yo"), _T ("rw_RW"), _T ("si_SI"), _T ("sk_SK"), _T ("sq_AL"),
  _T ("ss_ZA"), _T ("st_ZA"), _T ("sv_SE"), _T ("sw_KE"),  _T ("tet_ID"), _T ("th_TH"), _T ("tl_PH"), _T ("tn_ZA"), _T ("ts_ZA"), _T ("uk_UA"), _T ("ur_PK"), _T ("ve_ZA"), _T ("vi-VN"), _T ("xh_ZA"), _T ("zu_ZA")
};
const TCHAR *const AliasesTo[] = {_T ("Afrikaans"), _T ("Akan"), _T ("Bulgarian"), _T ("Catalan (Any)"), _T ("Catalan (Spain)"), _T ("Coptic (Bohairic dialect)"), _T ("Czech"), _T ("Welsh"),
  _T ("Danish"), _T ("German (Austria)"), _T ("German (Switzerland)"), _T ("German (Germany)"), _T ("German (Old and New Spelling)"), _T ("German (Additional)"), _T ("German (New Spelling)"),
  _T ("Greek"), _T ("English (Australia)"), _T ("English (Canada)"), _T ("English (Great Britain)"), _T ("English (Great Britain) [OED]"), _T ("English (New Zealand)"),
  _T ("English (United States)"), _T ("English (South Africa)"), _T ("Esperanto"), _T ("Spanish (Argentina)"), _T ("Spanish (Bolivia)"), _T ("Spanish (Chile)"), _T ("Spanish (Colombia)"), _T ("Spanish (Costa Rica)"),
  _T ("Spanish (Cuba)"), _T ("Spanish (Dominican Republic)"), _T ("Spanish (Ecuador)"), _T ("Spanish (Spain)"), _T ("Spanish (Guatemala)"), _T ("Spanish (Honduras)"), _T ("Spanish (Mexico)"),
  _T ("Spanish (New)"), _T ("Spanish (Nicaragua)"), _T ("Spanish (Panama)"), _T ("Spanish (Peru)"), _T ("Spanish (Puerto Rico)"), _T ("Spanish (Paraguay)"), _T ("Spanish (El Salvador)"), _T ("Spanish (Uruguay)"),
  _T ("Spanish (Bolivarian Republic of Venezuela)"), _T ("Estonian"), _T ("Faroese"), _T ("French"), _T ("French (1990)"), _T ("French (1990)"), _T ("French (Classique)"), _T ("French (Classique)"), _T ("French"), _T ("Frisian"), _T ("Irish"), _T ("Scottish Gaelic"),
  _T ("Galician"), _T ("Gujarati"), _T ("Hebrew"), _T ("Hindi"), _T ("Filipino"), _T ("Croatian"), _T ("Hungarian"), _T ("Interlingua"), _T ("Indonesian"), _T ("Icelandic"), _T ("Italian"), _T ("Kurdish"),
  _T ("Latin"), _T ("Lithuanian"), _T ("Latvian"), _T ("Malagasy"), _T ("Maori"), _T ("Macedonian (FYROM)"), _T ("Mossi"), _T ("Marathi"), _T ("Malay"), _T ("Norwegian (Bokmal)"), _T ("Nepali"), _T ("Dutch"),
  _T ("Norwegian (Nynorsk)"), _T ("Ndebele"), _T ("Northern Sotho"), _T ("Chichewa"), _T ("Occitan"), _T ("Polish"), _T ("Portuguese (Brazil)"), _T ("Portuguese (Portugal)"), _T ("Romanian"), _T ("Russian"),
  _T ("Russian (without io)"), _T ("Russian (without io)"), _T ("Russian (with io)"), _T ("Kinyarwanda"), _T ("Slovenian"), _T ("Slovak"), _T ("Alban"), _T ("Swazi"), _T ("Northern Sotho"), _T ("Swedish"),
  _T ("Kiswahili"), _T ("Tetum"), _T ("Thai"), _T ("Tagalog"), _T ("Setswana"), _T ("Tsonga"), _T ("Ukrainian"), _T ("Urdu"), _T ("Venda"), _T ("Vietnamese"), _T ("isiXhosa"), _T ("isiZulu")
};

// Language Aliases
BOOL SetStringWithAliasApplied (TCHAR *&Target, TCHAR *OrigName)
{
  int Left, Right;
  Left = 0;
#ifdef _DEBUG
  for (int i = 0; i < countof (AliasesFrom) - 1; i++)
  {
    if (_tcscmp (AliasesFrom[i], AliasesFrom[i + 1]) >= 0)
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
    int CompareResult = _tcscmp (AliasesFrom[CentralElement], OrigName);
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

static BOOL TryToCreateDir (TCHAR *Path, BOOL Silent, HWND NppWindow)
{
  if (!CreateDirectory (Path, NULL))
  {
    if (!Silent)
    {
      if (!NppWindow)
        return FALSE;

      TCHAR Message[DEFAULT_BUF_SIZE];
      _stprintf (Message, _T ("Can't create directory %s"), Path);
      MessageBoxInfo MsgBox (NppWindow, Message, _T ("Error in directory creation"), MB_OK | MB_ICONERROR);
      SendMessage (NppWindow, GetCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
    }
    return FALSE;
  }
  return TRUE;
}

BOOL CheckForDirectoryExistence (TCHAR *Path, BOOL Silent, HWND NppWindow)
{
  for (unsigned int i = 0; i < _tcslen (Path); i++)
  {
    if (Path[i] == _T ('\\'))
    {
      Path[i] = _T ('\0');
      if (!PathFileExists (Path))
      {
        if (!TryToCreateDir (Path, Silent, NppWindow))
          return FALSE;
      }
      Path[i] = _T ('\\');
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
        Score[(i + 1) * H + (j + 1)] = std::min<int> (Score[i * H + j], std::min<int> (Score[(i + 1) * H + j], Score[i * H + (j + 1)])) + 1;
      }

      Score[(i + 1) * H + (j + 1)] = std::min<int> (Score[(i + 1) * H + (j + 1)], Score[i1 * H + j1] + (i - i1 - 1) + 1 + (j - j1 - 1));
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

void SortStringVectorByLevenshteinDistanceUtf8 (std::vector <char *> *Vector, char *Word)
{
  std::sort (Vector->begin (), Vector->end (), LevenshteinSortComparatorUtf8 (Word));
}

void SortStringVectorByDamerauLevenshteinDistanceUtf8 (std::vector <char *> *Vector, char *Word)
{
  std::sort (Vector->begin (), Vector->end (), DamerauLevenshteinSortComparatorUtf8 (Word));
}

void StripEqualElements (std::vector <char *> *&Vector)
{
  std::sort (Vector->begin (), Vector->end (), SortCompareChars);
  std::vector <char *> *NewVector = new std::vector <char *>;
  for (size_t i = 0; i < Vector->size (); i++)
  {
    if (i == 0 || strcmp (Vector->at (i), Vector->at (i - 1)) != 0)
      NewVector->push_back (Vector->at (i));
  }

  for (size_t i = 1; i < Vector->size (); i++)
  {
    if (strcmp (Vector->at (i), Vector->at (i - 1)) == 0)
      delete [] Vector->at (i);
  }
  CLEAN_AND_ZERO (Vector);
  Vector = NewVector;
}

TCHAR *GetLastSlashPosition (TCHAR *Path)
{
  return _tcsrchr (Path, _T ('\\'));
}