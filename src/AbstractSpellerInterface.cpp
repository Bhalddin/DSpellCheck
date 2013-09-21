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
#include "AbstractSpellerInterface.h"
#include "CommonFunctions.h"

AbstractSpellerInterface::AbstractSpellerInterface ()
{
  Ignored = new WordSet;
}

AbstractSpellerInterface::~AbstractSpellerInterface ()
{
  CleanAndZeroWordList (Ignored);
}

void AbstractSpellerInterface::IgnoreAll (char *Word)
{
  char *Buf = 0;
  switch (CurrentEncoding)
    {
  case ENCODING_UTF8:
    SetString (Buf, Word);
    break;
    case ENCODING_ANSI:
    SetStringDUtf8 (Buf, Word);
    break;
    case ENCODING_WCHAR:
    SetStringDUtf8 (Buf, (wchar_t *)Word);
    break;
    }

  Ignored->insert (Buf);
}

BOOL AbstractSpellerInterface::CheckWord (char *Word)
{
  char *Buf = 0;
  switch (CurrentEncoding)
    {
  case ENCODING_UTF8:
    SetString (Buf, Word);
    break;
    case ENCODING_ANSI:
    SetStringDUtf8 (Buf, Word);
    break;
    case ENCODING_WCHAR:
    SetStringDUtf8 (Buf, (wchar_t *)Word);
    break;
    }

  if (Ignored->find (Buf) != Ignored->end ())
    {
      CLEAN_AND_ZERO_ARR (Buf);
      return TRUE;
    }

  CLEAN_AND_ZERO_ARR (Buf);
  return SpellerCheckWord (Word);
}
