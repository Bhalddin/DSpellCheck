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

#include <CommCtrl.h>

#include "DicOptions.h"

#include "Controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "HunspellInterface.h"
#include "CommonFunctions.h"
#include "Plugin.h"
#include "SpellChecker.h"

#include "resource.h"

void DicOptions::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_HUNSPELL_DIC_OPTIONS);
  }
  else
  {
    goToCenter ();
    display ();
  }
}

void DicOptions::init (HINSTANCE hInst, HWND Parent)
{
  return Window::init (hInst, Parent);
}

void DicOptions::Fill (SpellChecker *SpellCheckerInstance)
{
  switch (SpellCheckerInstance->GetHunspellDicMode ())
    {
  case HunspellDictionaryMode::SeparateForAll:
    Button_SetCheck (HSeparateForAll, BST_CHECKED);
    break;
      case HunspellDictionaryMode::SeparateForSingle:
        Button_SetCheck (HSeparateForSingle, BST_CHECKED);
    break;
        case HunspellDictionaryMode::UnifiedForAll:
          Button_SetCheck (HUnifiedForAll, BST_CHECKED);
    break;
    }
}

void DicOptions::Collect (SpellChecker *SpellCheckerInstance)
{
  if (Button_GetCheck (HSeparateForAll) == BST_CHECKED)
    SpellCheckerInstance->SetHunspellDicMode (HunspellDictionaryMode::SeparateForAll);
  else if (Button_GetCheck (HSeparateForSingle) == BST_CHECKED)
    SpellCheckerInstance->SetHunspellDicMode (HunspellDictionaryMode::SeparateForSingle);
  else if (Button_GetCheck (HUnifiedForAll) == BST_CHECKED)
    SpellCheckerInstance->SetHunspellDicMode (HunspellDictionaryMode::UnifiedForAll);
}

BOOL CALLBACK DicOptions::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      HSeparateForAll = ::GetDlgItem (_hSelf, IDC_SEPARATE_FOR_ALL);
      HSeparateForSingle = ::GetDlgItem (_hSelf, IDC_SEPARATE_FOR_SINGLE);
      HUnifiedForAll = ::GetDlgItem (_hSelf, IDC_UNIFIED_FOR_ALL);
      SendEvent (EID_FILL_DIC_OPTIONS);
      return TRUE;
    }
    break;
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDOK:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          SendEvent (EID_COLLECT_DIC_OPTIONS);
          display (false);
        }
        break;
      case IDCANCEL:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          display (false);
        }

        break;
      }
    }
    break;
  };
  return FALSE;
}