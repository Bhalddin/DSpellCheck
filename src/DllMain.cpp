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

#include <shlwapi.h>

#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "LangList.h"
#include "Plugin.h"
#include "RemoveDics.h"
#include "MainDef.h"
#include "SpellChecker.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;
extern bool doCloseTag;
HANDLE HModule;
int RecheckDelay;
std::vector<std::pair <long, long> > CheckQueue;
HANDLE Timer = 0;
WNDPROC wndProcNotepad = NULL;
BOOL RestylingCausedRecheckWasDone = FALSE; // Hack to avoid eternal cycle in case of scintilla bug

int GetRecheckDelay ()
{
  return RecheckDelay;
}

void SetRecheckDelay (int Value, int WriteToIni)
{
  if (Value < 0)
    Value = 0;

  if (Value > 20000)
    Value = 20000;

  if (WriteToIni)
  {
    std::pair <wchar_t *, DWORD> *x = new std::pair <wchar_t *, DWORD>;
    x->first = 0;
    SetString (x->first, _T ("Recheck_Delay"));
    x->second = MAKELPARAM (Value, 500);
    PostMessageToMainThread (TM_WRITE_SETTING, 0, (LPARAM) x);
  }
  RecheckDelay = Value;
}

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  reasonForCall,
                       LPVOID lpReserved )
{
  HModule = hModule;

  switch (reasonForCall)
  {
  case DLL_PROCESS_ATTACH:
    pluginInit(hModule);
    break;

  case DLL_PROCESS_DETACH:
    pluginCleanUp();
    _CrtDumpMemoryLeaks();
    if (wndProcNotepad != NULL)
      ::SetWindowLongPtr(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad); // Removing subclassing
    break;

  case DLL_THREAD_ATTACH:
    break;

  case DLL_THREAD_DETACH:
    break;
  }

  return TRUE;
}
WPARAM LastHwnd  = NULL;
LPARAM LastCoords = 0;
std::vector <SuggestionsMenuItem *> *MenuList = NULL;
// Ok, trying to use window subclassing to handle messages
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  int ret;
  switch (Message)
  {
  case WM_INITMENUPOPUP:
    {
      // Checking that it isn't system menu and nor any main menu except 0th
      if (MenuList && LOWORD (lParam) == 0 && HIWORD (lParam) == 0)
      {
        // Special check for 0th main menu item
        MENUBARINFO info;
        info.cbSize = sizeof (MENUBARINFO);
        GetMenuBarInfo (nppData._nppHandle, OBJID_MENU, 0, &info);
        HMENU MainMenu = info.hMenu;
        MENUITEMINFO FileMenuItem;
        FileMenuItem.cbSize = sizeof (MENUITEMINFO);
        FileMenuItem.fMask = MIIM_SUBMENU;
        GetMenuItemInfo (MainMenu, 0, TRUE, &FileMenuItem);

        HMENU Menu = (HMENU) wParam;
        if (FileMenuItem.hSubMenu != Menu)
        {
          HMENU TempMenu = 0;
          int j = 0;
          for (unsigned int i = 0; i < MenuList->size (); i++)
          {
            InsertSuggMenuItem (Menu, (*MenuList)[i]->Text, (*MenuList)[i]->Id, ((*MenuList)[i]->Flag != MenuFlagEnum::MENU_CHILD) ? j : -1, TempMenu, (*MenuList)[i]->Flag);
            if ((*MenuList)[i]->Flag != MenuFlagEnum::MENU_CHILD)
              j++;
          }
        }
      }
      CLEAN_AND_ZERO_POINTER_VECTOR (MenuList);
    }
    break;

  case WM_COMMAND:
    if (HIWORD (wParam) == 0)
    {
      if (!GetUseAllocatedIds ())
        PostMessageToMainThread (TM_MENU_RESULT, wParam, 0);
    }
    break;
  case WM_NOTIFY:
    // Removing possibility of adding items to tab bar menu.
    if (((LPNMHDR)lParam)->code == NM_RCLICK)
    {
      CLEAN_AND_ZERO_POINTER_VECTOR (MenuList);
    }
    break;
  case WM_CONTEXTMENU:
    LastHwnd = wParam;
    LastCoords = lParam;
    PostMessageToMainThread (TM_PRECALCULATE_MENU, wParam, lParam);
    return TRUE;
  case WM_DISPLAYCHANGE:
    {
      SendEvent (EID_HIDE_SUGGESTIONS_BOX);
    }
    break;
  }

  if (Message != 0)
  {
    if (Message == GetCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX))
    {
      MessageBoxInfo *MsgBox = (MessageBoxInfo *) wParam;
      DWORD Result = MessageBox (MsgBox->hWnd, MsgBox->Message, MsgBox->Title, MsgBox->Flags);
      return Result;
    }
    else if (Message == GetCustomGUIMessageId (CustomGUIMessage::SHOW_CALCULATED_MENU))
    {
      MenuList = (std::vector <SuggestionsMenuItem *> *) lParam;
      return ::CallWindowProc(wndProcNotepad, hWnd, WM_CONTEXTMENU, LastHwnd, LastCoords);
    }
  }

  /*
  wchar_t Buf[DEFAULT_BUF_SIZE];
  _itot (Message, Buf, 16);
  _tcscat (Buf, _T ("\n"));
  OutputDebugString (Buf);
  */
  ret = ::CallWindowProc(wndProcNotepad, hWnd, Message, wParam, lParam);
  return ret;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
  nppData = notpadPlusData;
  commandMenuInit();
  wndProcNotepad = (WNDPROC)::SetWindowLongPtr(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);
}

extern "C" __declspec(dllexport) const wchar_t *getName()
{
  return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF)
{
  *nbF = nbFunc;
  return funcItem;
}

static void AddToQueue (long Start, long End)
{
  std::pair <long, long> *Pair = new std::pair <long, long>;
  Pair->first = Start;
  Pair->second = End;
  CheckQueue.push_back (*Pair);
}

// For now doesn't look like there is such a need in check modified, but code stays until thorough testing
VOID CALLBACK ExecuteQueue (
  PVOID lpParameter,
  BOOLEAN TimerOrWaitFired
)
{
  /*
  std::vector<std::pair <long, long>>::iterator Iterator;
  for (Iterator = CheckQueue.begin (); Iterator != CheckQueue.end (); ++Iterator)
  {
  std::pair <long, long> *Pair = new std::pair <long, long> (*Iterator);
  SendEvent (EID_RECHECK_MODIFIED_ZONE);
  PostMessageToMainThread (TM_MODIFIED_ZONE_INFO, 0, (LPARAM) Pair);
  }
  CheckQueue.clear ();
  */
  if (Timer)
    DeleteTimerQueueTimer (0, Timer, 0);

  Timer = 0;
  SendEvent (EID_RECHECK_VISIBLE);
  RestylingCausedRecheckWasDone = TRUE;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
  /*
  // DEBUG_CODE:
  long CurPos = SendMsgToEditor(&nppData, SCI_GETCURRENTPOS);
  int Style = SendMsgToEditor(&nppData, SCI_GETSTYLEAT, CurPos);
  */
  switch (notifyCode->nmhdr.code)
  {
  case NPPN_SHUTDOWN:
    {
      SendEvent (EID_KILLTHREAD);
      SendNetworkEvent (EID_KILLNETWORKTHREAD);
      commandMenuCleanUp();
      DeleteTimerQueueTimer (0, Timer, 0);
    }
    break;

  case NPPN_READY:
    {
      RegisterCustomMessages ();
      InitClasses ();
      CheckQueue.clear ();
      CreateThreadResources ();
      LoadSettings ();
      SendEvent (EID_CHECK_FILE_NAME);
      CreateHooks ();
      CreateTimerQueueTimer (&Timer, 0, ExecuteQueue, NULL, INFINITE, INFINITE , 0);
      SendEvent (EID_RECHECK_VISIBLE_BOTH_VIEWS);
      RestylingCausedRecheckWasDone = FALSE;
      SendEvent (EID_SET_SUGGESTIONS_BOX_TRANSPARENCY);
      SendEvent (EID_UPDATE_LANG_LISTS); // To update quick lang change menu
      UpdateLangsMenu ();
    }
    break;

  case NPPN_BUFFERACTIVATED:
    {
      SendEvent (EID_CHECK_FILE_NAME);
      //SendEvent (EID_HIDE_SUGGESTIONS_BOX);
      RecheckVisible ();
      RestylingCausedRecheckWasDone = FALSE;
    }
    break;

  case SCN_UPDATEUI:
    if (notifyCode->updated & SC_UPDATE_SELECTION)
      SendEvent (EID_SELECTION_CHANGED);

    if (notifyCode->updated & (SC_UPDATE_CONTENT)  && !Timer && !RestylingCausedRecheckWasDone) // If restyling wasn't caused by user input...
    {
      SendEvent (EID_RECHECK_VISIBLE);
      RestylingCausedRecheckWasDone = TRUE;
    }
    else if (notifyCode->updated & (SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL) && !Timer) // If scroll wasn't caused by user input...
    {
      SendEvent (EID_RECHECK_INTERSECTION);
      RestylingCausedRecheckWasDone = FALSE;
    }
    SendEvent (EID_HIDE_SUGGESTIONS_BOX);
    break;

  case SCN_MODIFIED:
    if (notifyCode->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
    {
      // SendEvent (EID_HIDE_SUGGESTIONS_BOX);
      long Start = 0, End = 0;
      if (notifyCode->modificationType & SC_MOD_DELETETEXT)
      {
        Start = notifyCode->position;
        End = notifyCode->position;
      }
      else
      {
        Start = notifyCode->position;
        End = notifyCode->position + notifyCode->length - 1;
      }

      // AddToQueue (Start, End);
      if (Timer)
        ChangeTimerQueueTimer (0, Timer, RecheckDelay, 0);
      else
        CreateTimerQueueTimer (&Timer, 0, ExecuteQueue, NULL, RecheckDelay, 0 , 0);
      SendEvent (EID_EDITING_DONE);
    }
    break;

  case NPPN_LANGCHANGED:
    {
      SendEvent (EID_LANG_CHANGE);
    }
    break;

  case NPPN_TBMODIFICATION:
    {
      AddIcons ();
    }

  default:
    return;
  }
}

// Here you can process the Npp Messages
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :

void InitNeededDialogs (int wParam)
{
  // A little bit of code duplication here :(
  int MenuId = wParam;
  if ((!GetUseAllocatedIds () && HIBYTE (MenuId) != DSPELLCHECK_MENU_ID &&
       HIBYTE (MenuId) != LANGUAGE_MENU_ID)
      || (GetUseAllocatedIds () && ((int) MenuId < GetContextMenuIdStart () || (int) MenuId > GetContextMenuIdStart () + MENU_IDS_TOTAL)))
    return;
  int UsedMenuId = 0;
  if (GetUseAllocatedIds ())
  {
    UsedMenuId = ((int) MenuId < GetLangsMenuIdStart () ? DSPELLCHECK_MENU_ID : LANGUAGE_MENU_ID);
  }
  else
  {
    UsedMenuId = HIBYTE (MenuId);
  }
  switch (UsedMenuId)
  {
  case  LANGUAGE_MENU_ID:
    int Result = 0;
    if (!GetUseAllocatedIds ())
      Result = LOBYTE (MenuId);
    else
      Result = MenuId - GetLangsMenuIdStart ();
    if (Result == DOWNLOAD_DICS)
      GetDownloadDics ()->DoDialog ();
    else if (Result == CUSTOMIZE_MULTIPLE_DICS)
      GetLangList ()->DoDialog ();
    else if (Result == REMOVE_DICS)
      GetRemoveDics ()->DoDialog ();
    break;
  }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
  switch (Message)
  {
  case WM_MOVE:
    SendEvent (EID_HIDE_SUGGESTIONS_BOX);
    return FALSE;
  case WM_COMMAND:
    {
      if (HIWORD (wParam) == 0 && GetUseAllocatedIds ())
      {
        InitNeededDialogs (wParam);
        PostMessageToMainThread (TM_MENU_RESULT, wParam, 0);
      }
    }
    break;
  }

  return FALSE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
  return TRUE;
}
#endif //UNICODE