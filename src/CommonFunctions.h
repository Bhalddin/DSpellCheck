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

#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include "MainDef.h"

struct NppData;

void SetString (char *&Target, const char *Str);

void SetString (wchar_t *&Target, const wchar_t *Str);

void SetString (char *&Target, const wchar_t *Str);

void SetString (wchar_t *&Target, const char *Str);

// In case source is in UTF-8
void SetStringSUtf8 (char *&Target, const char *Str);
void SetStringSUtf8 (wchar_t *&Target, const char *Str);
void SetStringSUtf8Safe (wchar_t *&Target, const char *Str, size_t *&Indexation, const wchar_t c = ' ');

// In case destination is in UTF-8
void SetStringDUtf8 (char *&Target, const char *Str);
void SetStringDUtf8 (char *&Target, const wchar_t *Str);

BOOL SetStringWithAliasApplied (TCHAR *&Target, TCHAR *OrigName);

void SetParsedString (TCHAR *&Dest, TCHAR *Source);
HWND GetScintillaWindow(const NppData *NppDataArg);
LRESULT SendMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT SendMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
void SendMsgToBothEditors (const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

LRESULT SendMsgToNpp (const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

char *Utf8Dec(const char *string, const char *current);
char *Utf8chr (const char *s, const char *sfc);
int Utf8GetCharSize (char c);
char * Utf8strtok (char *s1, const char *Delimit, char **Context);
char *Utf8Inc (const char *string);
char *Utf8pbrk(const char *s, const char *set);
size_t Utf8Length (const char *String);
BOOL Utf8IsLead (char c);
BOOL Utf8IsCont (char c);

bool SortCompare (TCHAR *a, TCHAR *b);
bool EquivTCHARStrings (TCHAR *a, TCHAR *b);
size_t HashTCHARString (TCHAR *a);
bool EquivCharStrings (char *a, char *b);
size_t HashCharString (char *a);
bool SortCompareChars (char *a, char *b);

BOOL CheckForDirectoryExistence (TCHAR *Path, BOOL Silent = TRUE, HWND NppWindow = 0);
TCHAR *GetLastSlashPosition (TCHAR *Path);
void SortStringVectorByLevenshteinDistanceUtf8 (std::vector <char *> *Vector, char *Word);
void SortStringVectorByDamerauLevenshteinDistanceUtf8 (std::vector <char *> *Vector, char *Word);
void SortStringVectorByDamerauLevenshteinDistanceWchar (std::vector <wchar_t *> *Vector, wchar_t *Word);
void StripEqualElements (std::vector <wchar_t *> *&Vector); // Warning - changes pointer, vector order is being spoiled in process
char *strtok_s_nonalnum (char *s1, const char *delimit, char **context);
wchar_t *wcstok_s_nonalnum (wchar_t *s1, const wchar_t *delimit, wchar_t **context);

void CleanAndZeroWordList (WordSet *&WordListInstance);

#endif // COMMON_FUNCTIONS_H;
