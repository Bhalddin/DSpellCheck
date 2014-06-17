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
#include "AspellInterface.h"
#include "HunspellInterface.h"

#include "Aspell.h"
#include "DicOptions.h"
#include "DownloadDicsDlg.h"
#include "iconv.h"
#include "CommonFunctions.h"
#include "LanguageName.h"
#include "LangList.h"
#include "MainDef.h"
#include "PluginInterface.h"
#include "Plugin.h"
#include "RemoveDics.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "Scintilla.h"
#include "ScintillaStyleSwitches.h"
#include "SelectProxy.h"
#include "Suggestions.h"

SpellChecker::SpellChecker (const wchar_t *IniFilePathArg, SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
                            Suggestions *SuggestionsInstanceArg, LangList *LangListInstanceArg)
{
  CurrentPosition = 0;
  DelimUtf8 = 0;
  DelimExcUtf8 = 0;
  DelimUtf8Converted = 0;
  DelimExcUtf8Converted = 0;
  IniFilePath = 0;
  AspellLanguage = 0;
  AspellMultiLanguages = 0;
  HunspellLanguage = 0;
  HunspellMultiLanguages = 0;
  VisibleText = 0;
  DelimConverted = 0;
  DelimConvertedWchar = 0;
  DelimExcConverted = 0;
  DelimExcConvertedWchar = 0;
  SetString (IniFilePath, IniFilePathArg);
  SettingsDlgInstance = SettingsDlgInstanceArg;
  SuggestionsInstance = SuggestionsInstanceArg;
  NppDataInstance = NppDataInstanceArg;
  LangListInstance = LangListInstanceArg;
  AutoCheckText = 0;
  MultiLangMode = 0;
  AspellPath = 0;
  HunspellPath = 0;
  FileTypes = 0;
  CheckThose = 0;
  SBTrans = 0;
  SBSize = 0;
  CurWordList = 0;
  SelectedWord = 0;
  SuggestionsMode = 1;
  WUCLength = 0;
  WUCPosition = 0;
  WUCisRight = TRUE;
  CurrentScintilla = GetScintillaWindow (NppDataInstance);
  SuggestionMenuItems = 0;
  AspellSpeller = new AspellInterface (NppDataInstance->_nppHandle);
  HunspellSpeller = new HunspellInterface (NppDataInstance->_nppHandle);
  CurrentSpeller = AspellSpeller;
  LastSuggestions = 0;
  PrepareStringForConversion ();
  memset (ServerNames, 0, sizeof (ServerNames));
  memset (DefaultServers, 0, sizeof (DefaultServers));
  AddressIsSet = 0;
  SetString (DefaultServers[0], L"ftp://ftp.snt.utwente.nl/pub/software/openoffice/contrib/dictionaries/");
  SetString (DefaultServers[1], L"ftp://sunsite.informatik.rwth-aachen.de/pub/mirror/OpenOffice/contrib/dictionaries/");
  SetString (DefaultServers[2], L"ftp://gd.tuwien.ac.at/office/openoffice/contrib/dictionaries/");
  CurrentLangs = 0;
  DecodeNames = FALSE;
  ResetHotSpotCache ();
  ProxyUserName = 0;
  ProxyHostName = 0;
  ProxyPassword = 0;
  ProxyAnonymous = TRUE;
  ProxyType = 0;
  ProxyPort = 0;
  LastCurPos = 0;
  SettingsLoaded = FALSE;
  UseProxy = FALSE;
  WordNearCursorProtection = FALSE;
  MultiLangList = 0;
  SettingsToSave = new std::map<wchar_t *, DWORD, bool ( *)(wchar_t *, wchar_t *)> (SortCompare);
  if (SendMsgToNpp (NppDataInstance, NPPM_ALLOCATESUPPORTED, 0, 0))
  {
    SetUseAllocatedIds (TRUE);
    int Id;
    SendMsgToNpp (NppDataInstance, NPPM_ALLOCATECMDID, MENU_IDS_TOTAL, (LPARAM) &Id);
    SetContextMenuIdStart (Id);
    SetLangsMenuIdStart (Id + CONTEXT_MENU_IDS_TOTAL + 1);
  }
}

static const char Yo[] = "\xd0\x81";
static const char Ye[] = "\xd0\x95";
static const char yo[] = "\xd1\x91";
static const char ye[] = "\xd0\xb5";
static const char PunctuationApostrophe[] = "\xe2\x80\x99";

static const wchar_t YoWCHAR[] = L"\u0401";
static const wchar_t YeWCHAR[] = L"\u0415";
static const wchar_t yoWCHAR[] = L"\u0451";
static const wchar_t yeWCHAR[] = L"\u0435";
static const wchar_t PunctuationApostropheWCHAR[] = L"\u2019";


void SpellChecker::PrepareStringForConversion ()
{
  iconv_t Conv = iconv_open ("CHAR", "UTF-8");
  const char *InString [] = {Yo, yo, Ye, ye, PunctuationApostrophe};
  char **OutString [] = {&YoANSI, &yoANSI, &YeANSI, &yeANSI, &PunctuationApostropheANSI};
  char *Buf = 0;
  char *OutBuf = 0;
  const char *InBuf = 0;
  size_t InSize = 0;
  size_t OutSize = 0;
  size_t Res = 0;

  for (int i = 0; i < countof (InString); i++)
  {
    InSize = strlen (InString[i]) + 1;
    Buf = 0;
    SetString (Buf, InString[i]);
    InBuf = Buf;
    OutSize = Utf8Length (InString[i]) + 1;
    OutBuf = new char[OutSize];
    *OutString[i] = OutBuf;
    Res = iconv (Conv, &InBuf, &InSize, &OutBuf, &OutSize);
    CLEAN_AND_ZERO_ARR (Buf);
    if (Res == (size_t) - 1)
    {
      CLEAN_AND_ZERO_ARR (*OutString[i]);
    }
  }
  iconv_close (Conv);
}

SpellChecker::~SpellChecker ()
{
  CLEAN_AND_ZERO_STRING_VECTOR (LastSuggestions);
  CLEAN_AND_ZERO (AspellSpeller);
  CLEAN_AND_ZERO (HunspellSpeller);
  CLEAN_AND_ZERO_ARR (SelectedWord);
  CLEAN_AND_ZERO_ARR (DelimConverted);
  CLEAN_AND_ZERO_ARR (DelimConvertedWchar);
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DelimUtf8);
  CLEAN_AND_ZERO_ARR (DelimExcConverted);
  CLEAN_AND_ZERO_ARR (DelimExcConvertedWchar);
  CLEAN_AND_ZERO_ARR (DelimExcUtf8Converted);
  CLEAN_AND_ZERO_ARR (DelimExcUtf8);
  CLEAN_AND_ZERO_ARR (AspellLanguage);
  CLEAN_AND_ZERO_ARR (AspellMultiLanguages);
  CLEAN_AND_ZERO_ARR (HunspellLanguage);
  CLEAN_AND_ZERO_ARR (HunspellMultiLanguages);
  CLEAN_AND_ZERO_ARR (IniFilePath);
  CLEAN_AND_ZERO_ARR (AspellPath);
  CLEAN_AND_ZERO_ARR (HunspellPath);
  CLEAN_AND_ZERO_ARR (VisibleText);
  CLEAN_AND_ZERO_ARR (FileTypes);
  CLEAN_AND_ZERO_ARR (AdditionalHunspellPath);

  CLEAN_AND_ZERO_ARR (YoANSI);
  CLEAN_AND_ZERO_ARR (yoANSI);
  CLEAN_AND_ZERO_ARR (YeANSI);
  CLEAN_AND_ZERO_ARR (yeANSI);
  CLEAN_AND_ZERO_ARR (PunctuationApostropheANSI);
  CLEAN_AND_ZERO_ARR (ProxyHostName);
  CLEAN_AND_ZERO_ARR (ProxyUserName);
  CLEAN_AND_ZERO_ARR (ProxyPassword);
  CLEAN_AND_ZERO_STRING_VECTOR (MultiLangList);
  for (int i = 0; i < countof (ServerNames); i++)
    CLEAN_AND_ZERO_ARR (ServerNames[i]);
  for (int i = 0; i < countof (DefaultServers); i++)
    CLEAN_AND_ZERO_ARR (DefaultServers[i]);

  std::map<wchar_t *, DWORD, bool ( *)(wchar_t *, wchar_t *)>::iterator it = SettingsToSave->begin ();
  for (; it != SettingsToSave->end (); ++it)
  {
    delete ((*it).first);
  }
  CLEAN_AND_ZERO (SettingsToSave);

  CLEAN_AND_ZERO (CurrentLangs);
}

void InsertSuggMenuItem (HMENU Menu, wchar_t *Text, BYTE Id, int InsertPos, HMENU &HMenuTempStore, MenuFlagEnum::e Flag)
{
  MENUITEMINFO mi;
  memset(&mi, 0, sizeof(mi));
  mi.cbSize = sizeof(MENUITEMINFO);
  if (Flag == MenuFlagEnum::MENU_CHILD)
    Menu = HMenuTempStore;
  if (Flag == MenuFlagEnum::SEPARATOR)
  {
    mi.fType = MFT_SEPARATOR;
  }
  else
  {
    mi.fType = MFT_STRING;
    mi.fMask = MIIM_ID | MIIM_TYPE | ((Flag == MenuFlagEnum::MENU_PARENT) ? MIIM_SUBMENU : 0);
    if (!GetUseAllocatedIds ())
      mi.wID = MAKEWORD (Id, DSPELLCHECK_MENU_ID);
    else
      mi.wID = GetContextMenuIdStart () + Id;

    mi.dwTypeData = Text;
    if (Flag == MenuFlagEnum::MENU_PARENT)
    {
      HMenuTempStore = CreatePopupMenu ();
      mi.hSubMenu = HMenuTempStore;
    }
    mi.cch = wcslen (Text) + 1;
  }

  if (InsertPos == -1)
    InsertMenuItem (Menu, GetMenuItemCount (Menu), TRUE, &mi);
  else
    InsertMenuItem (Menu, InsertPos, TRUE, &mi);

  /*
  MENUITEMINFO Mif;
  Mif.fMask = MIIM_ID;
  Mif.cbSize = sizeof (MENUITEMINFO);
  HMenuTempStore = (HMENU) Mif.wID;
  GetMenuItemInfo (Menu, InsertPos == -1 ? GetMenuItemCount (Menu) : InsertPos, TRUE, &Mif);
  */
}

BOOL WINAPI SpellChecker::NotifyMessage (UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch (Msg)
  {
  case TM_MENU_RESULT:
    {
      ProcessMenuResult (wParam);
      return TRUE;
    }
    break;
  case TM_PRECALCULATE_MENU:
    {
      if (CheckTextNeeded () && SuggestionsMode == SUGGESTIONS_CONTEXT_MENU)
      {
        long Pos, Length;
        WUCisRight = GetWordUnderCursorIsRight (Pos, Length, TRUE);
        if (!WUCisRight)
        {
          WUCPosition = Pos;
          WUCLength = Length;
          FillSuggestionsMenu (0);
        }
      }

      PostMessage (NppDataInstance->_nppHandle, GetCustomGUIMessageId (CustomGUIMessage::SHOW_CALCULATED_MENU), 0, (LPARAM)SuggestionMenuItems);
      SuggestionMenuItems = 0;
    }
    break;
  case TM_WRITE_SETTING:
    {
      WriteSetting (lParam);
    }
    break;
  case TM_ADD_USER_SERVER:
    {
      wchar_t *Name = (wchar_t *) wParam;
      wchar_t *TrimmedName = 0;
      SetString (TrimmedName, Name);
      FtpTrim (TrimmedName);
      wchar_t *Buf = 0;
      for (int i = 0; i < countof (DefaultServers); i++)
      {
        SetString (Buf, DefaultServers[i]);
        FtpTrim (Buf);
        if (wcscmp (Buf, TrimmedName) == 0)
          goto add_user_server_cleanup; // Nothing is done in this case
      }
      for (int i = 0; i < countof (ServerNames); i++)
      {
        SetString (Buf, ServerNames[i]);
        FtpTrim (Buf);
        if (wcscmp (Buf, TrimmedName) == 0)
          goto add_user_server_cleanup; // Nothing is done in this case
      }
      // Then we're adding finally
      CLEAN_AND_ZERO_ARR (ServerNames[countof (ServerNames) - 1]);
      for (int i = countof (ServerNames) - 1; i > 0; i--)
      {
        ServerNames[i] = ServerNames[i - 1];
      }
      ServerNames[0] = 0;
      SetString (ServerNames[0], Name);
add_user_server_cleanup:
      CLEAN_AND_ZERO_ARR (Buf);
      CLEAN_AND_ZERO_ARR (Name);
      CLEAN_AND_ZERO_ARR (TrimmedName);
      ResetDownloadCombobox ();
      SaveSettings ();
    }
    break;
  default:
    break;
  }
  return TRUE;
}

void SpellChecker::SetSuggType (int SuggType)
{
  SuggestionsMode = SuggType;
  HideSuggestionBox ();
}

void SpellChecker::SetShowOnlyKnow (BOOL Value)
{
  ShowOnlyKnown = Value;
}

void SpellChecker::SetInstallSystem (BOOL Value)
{
  InstallSystem = Value;
}

BOOL SpellChecker::GetShowOnlyKnown ()
{
  return ShowOnlyKnown;
}

BOOL SpellChecker::GetInstallSystem ()
{
  return InstallSystem;
}

BOOL SpellChecker::GetDecodeNames ()
{
  return DecodeNames;
}

wchar_t *SpellChecker::GetLangByIndex (int i)
{
  return CurrentLangs->at(i).OrigName;
}

void SpellChecker::ReinitLanguageLists ()
{
  int SpellerId = LibMode;
  BOOL CurrentLangExists = FALSE;
  wchar_t *CurrentLang;

  AbstractSpellerInterface *SpellerToUse = (SpellerId == 1 ?
                                            (AbstractSpellerInterface *)HunspellSpeller :
                                            (AbstractSpellerInterface *)AspellSpeller);

  if (SpellerId == 0)
  {
    GetDownloadDics ()->display (false);
    GetRemoveDics ()->display (false);
  }

  if (SpellerId == 1)
    CurrentLang = HunspellLanguage;
  else
    CurrentLang = AspellLanguage;
  CLEAN_AND_ZERO (CurrentLangs);

  if (SpellerToUse->IsWorking ())
  {
    SettingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (FALSE);
    std::vector <wchar_t *> *LangsFromSpeller =  SpellerToUse->GetLanguageList ();
    CurrentLangs = new std::vector<LanguageName> ();

    if (!LangsFromSpeller || LangsFromSpeller->size () == 0)
    {
      SettingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (TRUE);
      return;
    }
    for (unsigned int i = 0; i < LangsFromSpeller->size (); i++)
    {
      LanguageName Lang (LangsFromSpeller->at (i), (SpellerId == 1 && DecodeNames)); // Using them only for Hunspell
      CurrentLangs->push_back (Lang);                               // TODO: Add option - use or not use aliases.
      if (wcscmp (Lang.OrigName, CurrentLang) == 0)
        CurrentLangExists = TRUE;
    }
    if (wcscmp (CurrentLang, L"<MULTIPLE>") == 0)
      CurrentLangExists = TRUE;

    CLEAN_AND_ZERO_STRING_VECTOR (LangsFromSpeller);
    std::sort (CurrentLangs->begin (), CurrentLangs->end (), DecodeNames ? CompareAliases : CompareOriginal);
    if (!CurrentLangExists && CurrentLangs->size () > 0)
    {
      if (SpellerId == 1)
        SetHunspellLanguage (CurrentLangs->at (0).OrigName);
      else
        SetAspellLanguage (CurrentLangs->at (0).OrigName);
      RecheckVisibleBothViews ();
    }
    SettingsDlgInstance->GetSimpleDlg ()->AddAvailableLanguages (CurrentLangs,
                                                                 SpellerId == 1 ? HunspellLanguage : AspellLanguage,
                                                                 SpellerId == 1 ? HunspellMultiLanguages : AspellMultiLanguages,
                                                                 SpellerId == 1 ? HunspellSpeller : 0);
  }
  else
  {
    SettingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (TRUE);
  }
}

int SpellChecker::GetLibMode ()
{
  return LibMode;
}

void SpellChecker::FillDialogs (BOOL NoDisplayCall)
{
  ReinitLanguageLists ();
  SettingsDlgInstance->GetSimpleDlg ()->SetLibMode (LibMode);
  SettingsDlgInstance->GetSimpleDlg ()->FillLibInfo (AspellSpeller->IsWorking () ? 2 - (!CurrentLangs || CurrentLangs->size () == 0) : 0, AspellPath, HunspellPath, AdditionalHunspellPath);
  SettingsDlgInstance->GetSimpleDlg ()->FillSugestionsNum (SuggestionsNum);
  SettingsDlgInstance->GetSimpleDlg ()->SetFileTypes (CheckThose, FileTypes);
  SettingsDlgInstance->GetSimpleDlg ()->SetCheckComments (CheckComments);
  SettingsDlgInstance->GetSimpleDlg ()->SetDecodeNames (DecodeNames);
  SettingsDlgInstance->GetSimpleDlg ()->SetSuggType (SuggestionsMode);
  SettingsDlgInstance->GetAdvancedDlg ()->SetDelimiterMode (DelimiterMode);
  SettingsDlgInstance->GetAdvancedDlg ()->FillDelimiters (this, FALSE);
  SettingsDlgInstance->GetAdvancedDlg ()->SetConversionOpts (IgnoreYo, ConvertSingleQuotes, RemoveBoundaryApostrophes);
  SettingsDlgInstance->GetAdvancedDlg ()->SetUnderlineSettings (UnderlineColor, UnderlineStyle);
  SettingsDlgInstance->GetAdvancedDlg ()->SetIgnore (IgnoreNumbers, IgnoreCStart, IgnoreCHave, IgnoreCAll, Ignore_, IgnoreSEApostrophe, IgnoreOneLetter);
  SettingsDlgInstance->GetAdvancedDlg ()->SetSuggBoxSettings (SBSize, SBTrans);
  SettingsDlgInstance->GetAdvancedDlg ()->SetBufferSize (BufferSize / 1024);
  SettingsDlgInstance->GetAdvancedDlg ()->SetRecheckPreventionType (RecheckPreventionType);
  if (!NoDisplayCall)
    SettingsDlgInstance->display ();
}

void SpellChecker::RecheckVisibleBothViews ()
{
  int OldLexer = Lexer;
  EncodingType OldEncoding = CurrentEncoding;
  Lexer = SendMsgToEditor (NppDataInstance->_scintillaMainHandle, NppDataInstance, SCI_GETLEXER);
  CurrentScintilla = NppDataInstance->_scintillaMainHandle;
  RecheckVisible ();

  CurrentScintilla = NppDataInstance->_scintillaSecondHandle;
  Lexer = SendMsgToEditor (NppDataInstance->_scintillaSecondHandle, NppDataInstance, SCI_GETLEXER);
  RecheckVisible ();
  Lexer = OldLexer;
  CurrentEncoding = OldEncoding;
  AspellSpeller->SetEncoding (CurrentEncoding);
  HunspellSpeller->SetEncoding (CurrentEncoding);
}

BOOL WINAPI SpellChecker::NotifyNetworkEvent (DWORD Event)
{
  if (!CurrentScintilla)
    return FALSE; // If scintilla is dead there's nothing else to do
  switch (Event)
  {
  case EID_DOWNLOAD_SELECTED:
    GetDownloadDics ()->DownloadSelected ();
    break;
  case EID_FILL_FILE_LIST:
    GetDownloadDics ()->FillFileList ();
    break;
  case EID_CANCEL_DOWNLOAD:
    // Do nothing, just unflag event
    break;
  case EID_KILLNETWORKTHREAD:
    return FALSE;
  }
  return TRUE;
}

BOOL WINAPI SpellChecker::NotifyEvent (DWORD Event)
{
  CurrentScintilla = GetScintillaWindow (NppDataInstance); // All operations should be done with current scintilla anyway
  if (!CurrentScintilla)
    return FALSE; // If scintilla is dead there's nothing else to do
  switch (Event)
  {
  case  EID_FILL_DIALOGS:
    FillDialogs ();
    break;
  case EID_APPLY_SETTINGS:
    SettingsDlgInstance->GetSimpleDlg ()->ApplySettings (this);
    SettingsDlgInstance->GetAdvancedDlg ()->ApplySettings (this);
    FillDialogs (TRUE);
    SaveSettings ();
    CheckFileName (); // Cause filters may change
    RefreshUnderlineStyle ();
    RecheckVisibleBothViews ();
    break;
  case EID_APPLY_MULTI_LANG_SETTINGS:
    LangListInstance->ApplyChoice (this);
    SaveSettings ();
    break;

  case EID_APPLY_PROXY_SETTINGS:
    GetSelectProxy ()->ApplyChoice (this);
    SaveSettings ();
    break;

  case EID_HIDE_DIALOG:
    SettingsDlgInstance->display (false);
    break;
  case EID_LOAD_SETTINGS:
    LoadSettings ();
    break;
  case EID_RECHECK_VISIBLE:
    RecheckVisible ();
    break;
  case EID_RECHECK_VISIBLE_BOTH_VIEWS:
    {
      RecheckVisibleBothViews ();
    }
    break;
  case EID_RECHECK_INTERSECTION:
    RecheckVisible (TRUE);
    break;
  case EID_SWITCH_AUTOCHECK:
    if (!SettingsLoaded)
      return FALSE;

    SwitchAutoCheck ();
    break;
  case EID_KILLTHREAD:
    return FALSE;
  case EID_INIT_SUGGESTIONS_BOX:
    if (SuggestionsMode == SUGGESTIONS_BOX)
      InitSuggestionsBox ();
    else
    {
      /* placeholder */
    }
    break;
  case EID_SHOW_SUGGESTION_MENU:
    FillSuggestionsMenu (SuggestionsInstance->GetPopupMenu ());
    SendMessage (SuggestionsInstance->getHSelf (), WM_SHOWANDRECREATEMENU, 0, 0);
    break;
  case EID_HIDE_SUGGESTIONS_BOX:
    HideSuggestionBox ();
    break;
  case EID_SET_SUGGESTIONS_BOX_TRANSPARENCY:
    SetSuggestionsBoxTransparency ();
    break;
  case EID_DEFAULT_DELIMITERS:
    SetDefaultDelimiters ();
    break;
  case EID_FIND_NEXT_MISTAKE:
    FindNextMistake ();
    break;
  case EID_FIND_PREV_MISTAKE:
    FindPrevMistake ();
    break;
  case EID_RECHECK_MODIFIED_ZONE:
    GetLimitsAndRecheckModified ();
    break;
  case EID_CHECK_FILE_NAME:
    CheckFileName ();
    break;
  case EID_FILL_DOWNLOAD_DICS_DIALOG:
    FillDownloadDics ();
    SendNetworkEvent (EID_FILL_FILE_LIST);
    break;
  case EID_INIT_DOWNLOAD_COMBOBOX:
    ResetDownloadCombobox ();
    break;
  case EID_REMOVE_SELECTED_DICS:
    GetRemoveDics ()->RemoveSelected (this);
    break;
  case EID_UPDATE_SELECT_PROXY:
    GetSelectProxy ()->SetOptions (UseProxy, ProxyHostName, ProxyUserName, ProxyPassword, ProxyPort, ProxyAnonymous, ProxyType);
    break;
  case EID_UPDATE_LANG_LISTS:
    ReinitLanguageLists ();
    break;
  case EID_UPDATE_LANGS_MENU:
    DoPluginMenuInclusion ();
    break;

  case EID_UPDATE_FROM_REMOVE_DICS_OPTIONS:
    GetRemoveDics ()->UpdateOptions (this);
    SaveSettings ();
    break;

  case EID_UPDATE_REMOVE_DICS_OPTIONS:
    GetRemoveDics ()->SetCheckBoxes (RemoveUserDics, RemoveSystem);
    break;

  case EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS:
    GetDownloadDics ()->UpdateOptions (this);
    GetDownloadDics ()->UpdateListBox ();
    SaveSettings ();
    break;

  case EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS_NO_UPDATE:
    GetDownloadDics ()->UpdateOptions (this);
    SaveSettings ();
    break;

  case EID_LIB_CHANGE:
    SettingsDlgInstance->GetSimpleDlg ()->ApplyLibChange (this);
    SettingsDlgInstance->GetSimpleDlg ()->FillLibInfo (AspellSpeller->IsWorking () ? 2 - (!CurrentLangs || CurrentLangs->size () == 0) : 0, AspellPath, HunspellPath, AdditionalHunspellPath);
    RecheckVisibleBothViews ();
    SaveSettings ();
    break;

  case EID_LANG_CHANGE:
    Lexer = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLEXER);
    RecheckVisible ();
    break;

  case EID_HIDE_DOWNLOAD_DICS: // TODO: Make it async, though it's really hard
    GetDownloadDics ()->display (false);
    break;

  case EID_SHOW_SELECT_PROXY:
    GetSelectProxy ()->display ();
    break;

  case EID_DELIM_MODE_CHANGE:
    SettingsDlgInstance->GetAdvancedDlg ()->FillDelimiterStyle (this);
    SettingsDlgInstance->GetAdvancedDlg ()->FillDelimiters (this, FALSE);
    SaveSettings ();
    break;

  case EID_EDITING_DONE:
    if (RecheckPreventionType == RecheckPreventionTypes::FIREFOX_LIKE)
    {
      BOOL RecheckNeeded = FALSE;
      if (!WordNearCursorProtection)
        RecheckNeeded = TRUE;

      WordNearCursorProtection = TRUE;
      if (RecheckNeeded)
        RecheckVisible ();
    }
    else
      WordNearCursorProtection = FALSE;

    LastCurPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETCURRENTPOS);
    // break;

  case EID_FILL_DIC_OPTIONS:
    GetDicOptions ()->Fill (this);

  case EID_COLLECT_DIC_OPTIONS:
    GetDicOptions ()->Collect (this);

  case EID_SELECTION_CHANGED:
    if (RecheckPreventionType == RecheckPreventionTypes::FIREFOX_LIKE)
    {
      BOOL RecheckNeeded = FALSE;
      long NewCurPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETCURRENTPOS);
      int Line = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINEFROMPOSITION, NewCurPos);
      long LineLength = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINELENGTH, Line);
      char *Buf = new char[LineLength + 1];
      SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLINE, Line, (LPARAM) Buf);
      Buf [LineLength] = 0;
      long Offset = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POSITIONFROMLINE, Line);
      char *WordFound = GetWordAt (NewCurPos, Buf, Offset);
      if (!WordFound || !*WordFound)
      {
        if (WordNearCursorProtection)
          RecheckNeeded = TRUE;
        LastCurPos = NewCurPos;
        WordNearCursorProtection = FALSE;
        if (RecheckNeeded)
          RecheckVisible ();
        CLEAN_AND_ZERO_ARR (Buf);
        break;
      }
      long Pos = WordFound - Buf + Offset;
      long PosEnd = Pos + strlen (WordFound);
      if (LastCurPos < Pos || LastCurPos > PosEnd)
      {
        if (WordNearCursorProtection)
          RecheckNeeded = TRUE;
        WordNearCursorProtection = FALSE;
      }

      LastCurPos = NewCurPos;
      if (RecheckNeeded)
        RecheckVisible ();
      CLEAN_AND_ZERO_ARR (Buf);
    }
    else
      WordNearCursorProtection = FALSE;
    break;
    /*
    case EID_APPLYMENUACTION:
    ApplyMenuActions ();
    break;
    */
  }
  return TRUE;
}

void SpellChecker::SetRemoveUserDics (BOOL Value)
{
  RemoveUserDics = Value;
}

void SpellChecker::SetRemoveSystem (BOOL Value)
{
  RemoveSystem = Value;
}

BOOL SpellChecker::GetRemoveUserDics ()
{
  return RemoveUserDics;
}

BOOL SpellChecker::GetRemoveSystem ()
{
  return RemoveSystem;
}

void SpellChecker::DoPluginMenuInclusion (BOOL Invalidate)
{
  BOOL Res;
  MENUITEMINFO Mif;
  HMENU DSpellCheckMenu = GetDSpellCheckMenu ();
  HMENU LangsSubMenu = GetLangsSubMenu (DSpellCheckMenu);
  if (LangsSubMenu)
    DestroyMenu (LangsSubMenu);
  wchar_t *CurLang = (LibMode == 1) ? HunspellLanguage : AspellLanguage;
  HMENU NewMenu = CreatePopupMenu ();
  if (!Invalidate)
  {
    if (CurrentLangs)
    {
      if (CurrentLangs->size () > 0)
      {
        for (unsigned int i = 0; i < CurrentLangs->size (); i++)
        {
          int Checked = (wcscmp (CurLang, CurrentLangs->at(i).OrigName) == 0) ? (MFT_RADIOCHECK | MF_CHECKED) : MF_UNCHECKED;
          Res = AppendMenu (NewMenu, MF_STRING | Checked, GetUseAllocatedIds () ? i + GetLangsMenuIdStart () : MAKEWORD (i, LANGUAGE_MENU_ID), DecodeNames ? CurrentLangs->at(i).AliasName : CurrentLangs->at(i).OrigName);
          if (!Res)
            return;
        }
        int Checked = (wcscmp (CurLang, L"<MULTIPLE>") == 0) ? (MFT_RADIOCHECK | MF_CHECKED)  : MF_UNCHECKED;
        Res = AppendMenu (NewMenu, MF_STRING | Checked, GetUseAllocatedIds () ? MULTIPLE_LANGS + GetLangsMenuIdStart () : MAKEWORD (MULTIPLE_LANGS, LANGUAGE_MENU_ID), L"Multiple Languages");
        Res = AppendMenu (NewMenu, MF_SEPARATOR, -1, 0);
        Res = AppendMenu (NewMenu, MF_STRING, GetUseAllocatedIds () ? CUSTOMIZE_MULTIPLE_DICS + GetLangsMenuIdStart () : MAKEWORD (CUSTOMIZE_MULTIPLE_DICS, LANGUAGE_MENU_ID), L"Set Multiple Languages...");
        if (LibMode == 1) // Only Hunspell supported
        {
          Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? DOWNLOAD_DICS + GetLangsMenuIdStart () : MAKEWORD (DOWNLOAD_DICS, LANGUAGE_MENU_ID), L"Download More Languages...");
          Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? REMOVE_DICS + GetLangsMenuIdStart () : MAKEWORD (REMOVE_DICS, LANGUAGE_MENU_ID), L"Remove Unneeded Languages...");
        }
      }
      else if (LibMode == 1)
        Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? DOWNLOAD_DICS + GetLangsMenuIdStart () : MAKEWORD (DOWNLOAD_DICS, LANGUAGE_MENU_ID), L"Download Languages...");
    }
  }
  else
    Res = AppendMenu (NewMenu, MF_STRING | MF_DISABLED, -1, L"Loading...");

  Mif.fMask = MIIM_SUBMENU | MIIM_STATE;
  Mif.cbSize = sizeof (MENUITEMINFO);
  Mif.hSubMenu = (CurrentLangs ? NewMenu : 0);
  Mif.fState = (!CurrentLangs ? MFS_GRAYED : MFS_ENABLED);

  SetMenuItemInfo (DSpellCheckMenu, QUICK_LANG_CHANGE_ITEM, TRUE, &Mif);
}

void SpellChecker::FillDownloadDics ()
{
  GetDownloadDics ()->SetOptions (ShowOnlyKnown, InstallSystem);
}

void SpellChecker::ResetDownloadCombobox ()
{
  HWND TargetCombobox = GetDlgItem (GetDownloadDics ()->getHSelf (), IDC_ADDRESS);
  wchar_t Buf[DEFAULT_BUF_SIZE];
  ComboBox_GetText (TargetCombobox, Buf, DEFAULT_BUF_SIZE);
  if (AddressIsSet)
  {
    PreserveCurrentAddressIndex ();
  }
  ComboBox_ResetContent (TargetCombobox);
  for (int i = 0; i < countof (DefaultServers); i++)
  {
    ComboBox_AddString (TargetCombobox, DefaultServers[i]);
  }
  for (int i = 0; i < countof (ServerNames); i++)
  {
    if (*ServerNames[i])
      ComboBox_AddString (TargetCombobox, ServerNames[i]);
  }
  if (LastUsedAddress < USER_SERVER_CONST)
    ComboBox_SetCurSel (TargetCombobox, LastUsedAddress);
  else
    ComboBox_SetCurSel (TargetCombobox, LastUsedAddress - USER_SERVER_CONST + countof (DefaultServers));
  AddressIsSet = 1;
}

void SpellChecker::PreserveCurrentAddressIndex ()
{
  HWND TargetCombobox = GetDlgItem (GetDownloadDics ()->getHSelf (), IDC_ADDRESS);
  if (!TargetCombobox)
    return;
  wchar_t CurText [DEFAULT_BUF_SIZE];
  wchar_t *Buf = 0;
  ComboBox_GetText (TargetCombobox, CurText, DEFAULT_BUF_SIZE);
  FtpTrim (CurText);
  for (int i = 0; i < countof (ServerNames); i++)
  {
    SetString (Buf, DefaultServers[i]);
    FtpTrim (Buf);
    if (wcscmp (Buf, CurText) == 0)
    {
      LastUsedAddress = i;
      goto cleanup;
    }
  };
  for (int i = 0; i < countof (ServerNames); i++)
  {
    SetString (Buf, ServerNames[i]);
    FtpTrim (Buf);
    if (wcscmp (Buf, CurText) == 0)
    {
      LastUsedAddress = USER_SERVER_CONST + i;
      goto cleanup;
    }
  }
  LastUsedAddress = 0;
cleanup:
  CLEAN_AND_ZERO_ARR (Buf);
}

/*
void SpellChecker::ApplyMenuActions ()
{
}
*/

// For now just int option, later maybe choose option type in wParam
void SpellChecker::WriteSetting (LPARAM lParam)
{
  std::pair<wchar_t *, DWORD> *x = (std::pair<wchar_t *, DWORD> *) lParam;
  if (SettingsToSave->find (x->first) == SettingsToSave->end ())
    (*SettingsToSave)[x->first] = x->second;
  else
  {
    CLEAN_AND_ZERO_ARR (x->first);
  }
  CLEAN_AND_ZERO (x);
}

void SpellChecker::SetCheckComments (BOOL Value)
{
  CheckComments = Value;
}

void SpellChecker::CheckFileName ()
{
  wchar_t *Context = 0;
  wchar_t *Token = 0;
  wchar_t *FileTypesCopy = 0;
  wchar_t FullPath[MAX_PATH];
  SetString (FileTypesCopy, FileTypes);
  Token = wcstok_s (FileTypesCopy, L";", &Context);
  CheckTextEnabled = !CheckThose;
  ::SendMessage(NppDataInstance->_nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM) FullPath);

  while (Token)
  {
    if (CheckThose)
    {
      CheckTextEnabled = CheckTextEnabled || PathMatchSpec (FullPath, Token);
      if (CheckTextEnabled)
        break;
    }
    else
    {
      CheckTextEnabled &= CheckTextEnabled && (!PathMatchSpec (FullPath, Token));
      if (!CheckTextEnabled)
        break;
    }
    Token = wcstok_s (NULL, L";", &Context);
  }
  CLEAN_AND_ZERO_ARR (FileTypesCopy);
  Lexer = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLEXER);
}

int SpellChecker::GetStyle (int Pos)
{
  return SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETSTYLEAT, Pos);
}

BOOL SpellChecker::CheckTextNeeded ()
{
  return CheckTextEnabled && AutoCheckText;
}

void SpellChecker::GetLimitsAndRecheckModified ()
{
  MSG Msg;
  GetMessage (&Msg, 0, 0, 0);
  std::pair<long, long> *Pair = reinterpret_cast <std::pair<long, long> *> (Msg.lParam);
  ModifiedStart = Pair->first;
  ModifiedEnd = Pair->second;
  CLEAN_AND_ZERO (Pair);
  RecheckModified ();
}

void SpellChecker::HideSuggestionBox ()
{
  SuggestionsInstance->display (false);
}
void SpellChecker::FindNextMistake ()
{
  CurrentPosition = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETCURRENTPOS);
  int CurLine = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINEFROMPOSITION, CurrentPosition);
  int LineStartPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POSITIONFROMLINE, CurLine);
  long DocLength = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLENGTH);
  int IteratorPos = LineStartPos;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
  {
    Range.chrg.cpMin = IteratorPos;
    Range.chrg.cpMax = IteratorPos + BufferSize;
    int IgnoreOffsetting = 0;
    if (Range.chrg.cpMax > DocLength)
    {
      IgnoreOffsetting = 1;
      Range.chrg.cpMax = DocLength;
    }
    Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
    SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
    char *IteratingStart = Range.lpstrText + Range.chrg.cpMax - Range.chrg.cpMin - 1;
    char *IteratingChar = IteratingStart;
    if (!IgnoreOffsetting)
    {
      switch (CurrentEncoding)
      {
      case ENCODING_UTF8:
        {
          int CharsFound = 0;
          while (Utf8IsCont (*IteratingChar) && Range.lpstrText < IteratingChar)
          {
            IteratingChar--;
            CharsFound++;
          }

          if (CharsFound < Utf8GetCharSize (*IteratingChar) - 1) // If tail of leading symbol isn't full then we're skipping it
          {
            IteratingChar--;
            while (Utf8IsCont (*IteratingChar) && Range.lpstrText < IteratingChar)
              IteratingChar--;
          }

          while ((!Utf8chr ( DelimUtf8Converted, IteratingChar)) && Range.lpstrText < IteratingChar)
          {
            IteratingChar = (char *) Utf8Dec (Range.lpstrText, IteratingChar);
          }
        }
      case ENCODING_ANSI:
        {
          while (!strchr (DelimConverted, *IteratingChar) && Range.lpstrText < IteratingChar)
            IteratingChar--;
        }
        break;
      case ENCODING_WCHAR:
        {
          size_t *Indexation = 0;
          wchar_t *ConvertedRange = 0;
          int CharsFound = 0;
          while (Utf8IsCont (*IteratingChar) && Range.lpstrText < IteratingChar)
          {
            IteratingChar--;
            CharsFound++;
          }
          if (CharsFound < Utf8GetCharSize (*IteratingChar) - 1)
            *(IteratingChar) = 0;

          SetStringSUtf8Safe (ConvertedRange, Range.lpstrText, Indexation);
          int i = wcslen (ConvertedRange) - 1;
          IteratingChar = Range.lpstrText + Indexation[i];
          while (!wcschr (DelimConvertedWchar, ConvertedRange[i]) && i > 0)
          {
            i--;
            IteratingChar = Range.lpstrText + Indexation[i];
          }
          CLEAN_AND_ZERO_ARR (Indexation);
          CLEAN_AND_ZERO_ARR (ConvertedRange);
        }
        break;
      }

      *IteratingChar = '\0';
    }
    SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_COLOURISE, Range.chrg.cpMin, Range.chrg.cpMax);
    SCNotification scn;
    scn.nmhdr.code = SCN_SCROLLED;
    SendMsgToNpp (NppDataInstance, WM_NOTIFY, 0, (LPARAM )&scn); // To fix bug with hotspots being removed
    Result = CheckText (Range.lpstrText, IteratorPos, FIND_FIRST);
    CLEAN_AND_ZERO_ARR (Range.lpstrText);
    if (Result)
      break;

    IteratorPos += (BufferSize + IteratingChar - IteratingStart);

    if (IteratorPos > DocLength)
    {
      if (!FullCheck)
      {
        CurrentPosition = 0;
        IteratorPos = 0;
        FullCheck = TRUE;
      }
      else
        break;

      if (FullCheck && IteratorPos > CurrentPosition)
        break; // So nothing was found TODO: Message probably
    }
  }
}

void SpellChecker::FindPrevMistake ()
{
  CurrentPosition = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETCURRENTPOS);
  int CurLine = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINEFROMPOSITION, CurrentPosition);
  int LineCount = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLINECOUNT);
  long LineEndPos = 0;
  long DocLength = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLENGTH);
  LineEndPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLINEENDPOSITION, CurLine);

  int IteratorPos = LineEndPos;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
  {
    Range.chrg.cpMin = IteratorPos - BufferSize;
    Range.chrg.cpMax = IteratorPos;
    int IgnoreOffsetting = 0;
    if (Range.chrg.cpMin < 0)
    {
      Range.chrg.cpMin = 0;
      IgnoreOffsetting = 1;
    }
    Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
    SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
    wchar_t *ConvertedRange = 0;
    size_t *Indexation = 0;
    char *IteratingStart = Range.lpstrText;
    char *IteratingChar = IteratingStart;
    if (!IgnoreOffsetting)
    {
      switch (CurrentEncoding)
      {
      case ENCODING_UTF8:
        {
          while (Utf8IsCont (*IteratingChar) && *IteratingChar)
            IteratingChar++;

          while ((!Utf8chr ( DelimUtf8Converted, IteratingChar)) && *IteratingChar)
          {
            IteratingChar = (char *) Utf8Inc (IteratingChar);
          }
        }
        break;
      case ENCODING_ANSI:
        {
          while (!strchr (DelimConverted, *IteratingChar) && IteratingChar)
            IteratingChar++;
        }
        break;
      case ENCODING_WCHAR:
        {
          while (Utf8IsCont (*IteratingChar) && *IteratingChar)
            IteratingChar++;
          char *IteratingCharAtStart = IteratingChar;
          char *OneMoreIteratingChar = Range.lpstrText + strlen (Range.lpstrText) - 1;
          int CharsFound = 0;
          while (Utf8IsCont (*OneMoreIteratingChar) && Range.lpstrText < OneMoreIteratingChar)
          {
            OneMoreIteratingChar--;
            CharsFound++;
          }
          if (CharsFound < Utf8GetCharSize (*IteratingChar) - 1)
            *(OneMoreIteratingChar) = 0;

          SetStringSUtf8Safe (ConvertedRange, IteratingChar, Indexation);
          int i = 0;
          IteratingChar = IteratingCharAtStart + Indexation [i];
          while (!wcschr (DelimConvertedWchar, ConvertedRange[i]) && ConvertedRange[i])
          {
            i++;
            IteratingChar = IteratingCharAtStart + Indexation [i];
          }
        }
      }
    }
    int offset = IteratingChar - IteratingStart;
    SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_COLOURISE, Range.chrg.cpMin + offset, Range.chrg.cpMax);
    SCNotification scn;
    scn.nmhdr.code = SCN_SCROLLED;
    SendMsgToNpp (NppDataInstance, WM_NOTIFY, 0, (LPARAM) &scn);
    Result = CheckText (Range.lpstrText + offset, Range.chrg.cpMin + offset, FIND_LAST); // Possibly there should be done optimization to avoid double conversion
    CLEAN_AND_ZERO_ARR (Range.lpstrText);
    CLEAN_AND_ZERO_ARR (ConvertedRange);
    CLEAN_AND_ZERO_ARR (Indexation);
    if (Result)
      break;

    IteratorPos -= (BufferSize - offset);

    if (IteratorPos < 0)
    {
      if (!FullCheck)
      {
        CurrentPosition = DocLength + 1;
        IteratorPos = DocLength;
        FullCheck = TRUE;
      }
      else
        break;

      if (FullCheck && IteratorPos < CurrentPosition - 1)
        break; // So nothing was found TODO: Message probably
    }
  }
}

void SpellChecker::SetDefaultDelimiters ()
{
  SettingsDlgInstance->GetAdvancedDlg ()->FillDelimiters (this, TRUE);
}

HWND SpellChecker::GetCurrentScintilla ()
{
  return CurrentScintilla;
}

BOOL SpellChecker::GetWordUnderCursorIsRight (long &Pos, long &Length, BOOL UseTextCursor)
{
  BOOL Ret = TRUE;
  POINT p;
  int initCharPos = -1;
  int SelectionStart = 0;
  int SelectionEnd = 0;

  if (!UseTextCursor)
  {
    if (GetCursorPos(&p) == 0)
      return TRUE;

    ScreenToClient(GetScintillaWindow (NppDataInstance), &p);

    initCharPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);
  }
  else
  {
    SelectionStart = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETSELECTIONSTART);
    SelectionEnd = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETSELECTIONEND);
    initCharPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETCURRENTPOS);
  }

  if (initCharPos != -1)
  {
    int Line = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINEFROMPOSITION, initCharPos);
    long LineLength = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINELENGTH, Line);
    char *Buf = new char[LineLength + 1];
    SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLINE, Line, (LPARAM) Buf);
    Buf [LineLength] = 0;
    long Offset = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POSITIONFROMLINE, Line);
    char *WordFound = GetWordAt (initCharPos, Buf, Offset);


    if (!WordFound || !*WordFound)
    {
      Ret = TRUE;
    }
    else
    {
      char *Word = 0;
      switch (CurrentEncoding)
      {
      case ENCODING_ANSI:
        SetString (Word, WordFound);
        break;
      case ENCODING_UTF8:
        SetString (Word, WordFound);
        break;
      case ENCODING_WCHAR:
        {
          wchar_t *TempWord = 0;
          SetStringSUtf8 (TempWord, WordFound);
          Word = (char *) TempWord;
          break;
        }
      }
      Pos = WordFound - Buf + Offset;
      long PosEnd = Pos + strlen (WordFound);
      CheckSpecialDelimeters (Word, Pos, PosEnd);
      long WordLen = PosEnd - Pos;
      if (SelectionStart != SelectionEnd && (SelectionStart != Pos || SelectionEnd != Pos + WordLen))
        {
          CLEAN_AND_ZERO_ARR (Buf);
          return TRUE;
        }
      if (CheckWord (Word, Pos, Pos + WordLen - 1, TRUE))
      {
        Ret = TRUE;
      }
      else
      {
        Ret = FALSE;
        Length = WordLen;
      }
      CLEAN_AND_ZERO_ARR (Word);
    }
    CLEAN_AND_ZERO_ARR (Buf);
  }
  return Ret;
}

char *SpellChecker::GetWordAt (long CharPos, char *Text, long Offset)
{
  char *UsedText = 0;
  if (!DelimUtf8 || !*Text)
    return 0;

  wchar_t *ConvertedText = 0;
  size_t *Indexation = 0;
  int index = 0; // Which wchar_t we're iterating currently

  char *Iterator = Text + CharPos - Offset;

  switch (CurrentEncoding)
  {
  case ENCODING_UTF8:
    if ((!*Iterator || Utf8chr ( DelimUtf8Converted, Iterator)) && Iterator > Text)
      Iterator = (char *) Utf8Dec (Text, Iterator);

    while ((!Utf8chr ( DelimUtf8Converted, Iterator)) && Text < Iterator)
      Iterator = (char *) Utf8Dec (Text, Iterator);
    break;
  case ENCODING_ANSI:
    if ((!*Iterator || strchr (DelimConverted, *Iterator)) && Iterator > Text)
      Iterator--;

    while (!strchr (DelimConverted, *Iterator) && Text < Iterator)
      Iterator--;
    break;
  case ENCODING_WCHAR:
    SetStringSUtf8Safe (ConvertedText, Text, Indexation);
    for (unsigned int i = 0; i <= wcslen (ConvertedText); i++)
    {
      if (Indexation[i] >= (unsigned int) (Iterator - Text))
      {
        index = i;
        break;
      }
    }

    if ((!ConvertedText[index] || wcschr (DelimConvertedWchar, ConvertedText[index])) && index > 0)
    {
      index--;
    }

    while (!wcschr (DelimConvertedWchar, ConvertedText[index]) && index > 0)
    {
      index--;
    }
    break;
  }
  UsedText = Iterator;

  switch (CurrentEncoding)
  {
  case ENCODING_UTF8:
    if (Utf8chr (DelimUtf8Converted, UsedText))
      UsedText = Utf8Inc (UsedText); // Then find first token after this zero
    break;
  case ENCODING_ANSI:
    if (strchr (DelimConverted, *UsedText))
      UsedText++;
    break;
  case ENCODING_WCHAR:
    if (wcschr (DelimConvertedWchar, ConvertedText[index]))
    {
      index++;
    }
    break;
  }

  char *Context = 0;
  // We're just taking the first token (basically repeating the same code as an in CheckVisible

  char *Res = 0;
  switch (CurrentEncoding)
  {
  case ENCODING_UTF8:
  case ENCODING_ANSI:
    Res = DoCommonStringTokenization (UsedText, &Context);
    break;
  case ENCODING_WCHAR:
    {
      wchar_t *WcharRes = (wchar_t *) DoCommonStringTokenization ((char *) (ConvertedText + index), &Context);

      if (!WcharRes)
      {
        CLEAN_AND_ZERO_ARR (Indexation);
        CLEAN_AND_ZERO_ARR (ConvertedText);
        return 0;
      }

      Res = Text + Indexation [WcharRes - ConvertedText];
      Text[Indexation [WcharRes + wcslen (WcharRes) - ConvertedText]] = '\0';
    }
    CLEAN_AND_ZERO_ARR (ConvertedText);
    CLEAN_AND_ZERO_ARR (Indexation);
    break;
  }

  if (Res - Text + Offset > CharPos)
    return 0;
  else
    return Res;
}

void SpellChecker::SetSuggestionsBoxTransparency ()
{
  // Set WS_EX_LAYERED on this window
  SetWindowLong (SuggestionsInstance->getHSelf (), GWL_EXSTYLE,
                 GetWindowLong(SuggestionsInstance->getHSelf (), GWL_EXSTYLE) | WS_EX_LAYERED);
  SetLayeredWindowAttributes(SuggestionsInstance->getHSelf (), 0, (255 * SBTrans) / 100, LWA_ALPHA);
  SuggestionsInstance->display (true);
  SuggestionsInstance->display (false);
}

void SpellChecker::InitSuggestionsBox ()
{
  if (!CurrentSpeller->IsWorking ())
    return;
  POINT p;
  if (!CheckTextNeeded ()) // If there's no red underline let's do nothing
  {
    SuggestionsInstance->display (false);
    return;
  }

  GetCursorPos (&p);
  if (WindowFromPoint (p) != GetScintillaWindow (NppDataInstance))
  {
    return;
  }

  long Pos, Length;
  if (GetWordUnderCursorIsRight (Pos, Length))
  {
    return;
  }
  WUCLength = Length;
  WUCPosition = Pos;
  int Line = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINEFROMPOSITION, WUCPosition);
  int TextHeight = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_TEXTHEIGHT, Line);
  int XPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POINTXFROMPOSITION, 0, WUCPosition);
  int YPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POINTYFROMPOSITION, 0, WUCPosition);

  p.x = XPos; p.y = YPos;
  RECT R;
  GetWindowRect (GetCurrentScintilla (), &R);
  ClientToScreen (GetScintillaWindow (NppDataInstance), &p);
  if (R.top > p.y + TextHeight - 3 || R.left > p.x || R.bottom < p.y + TextHeight - 3 + SBSize ||  R.right < p.x + SBSize)
    return;
  MoveWindow (SuggestionsInstance->getHSelf (), p.x, p.y + TextHeight - 3, SBSize, SBSize, TRUE);
  SuggestionsInstance->display ();
}

void SpellChecker::ProcessMenuResult (UINT MenuId)
{
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
  case  DSPELLCHECK_MENU_ID:
    {
      char *AnsiBuf = 0;
      int Result = 0;
      if (!GetUseAllocatedIds ())
        Result = LOBYTE (MenuId);
      else
        Result = MenuId - GetContextMenuIdStart ();
      AspellStringEnumeration *els = 0;

      if (Result != 0)
      {
        if (Result == MID_IGNOREALL)
        {
          ApplyConversions (SelectedWord);
          CurrentSpeller->IgnoreAll (SelectedWord);
          SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL, WUCPosition + WUCLength , WUCPosition + WUCLength );
          RecheckVisibleBothViews ();
        }
        else if (Result == MID_ADDTODICTIONARY)
        {
          ApplyConversions (SelectedWord);
          int DicNum = -1;
          if (LibMode == 1 && !MultiLangMode && (HunspellDicMode == HunspellDictionaryMode::SeparateForSingle || HunspellDicMode == HunspellDictionaryMode::SeparateForAll))
            DicNum = 0;

          CurrentSpeller->AddToDictionary (SelectedWord, DicNum);
          SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL, WUCPosition + WUCLength , WUCPosition + WUCLength );
          RecheckVisibleBothViews ();
        }
        else if (Result >= MID_ADD_TO_DICS_START)
        {
          ApplyConversions (SelectedWord);
          CurrentSpeller->AddToDictionary (SelectedWord, Result - MID_ADD_TO_DICS_START);
          SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL, WUCPosition + WUCLength , WUCPosition + WUCLength );
          RecheckVisibleBothViews ();
        }
        else if ((unsigned int)Result <= LastSuggestions->size ())
        {
          if (CurrentEncoding == ENCODING_ANSI)
            SetString (AnsiBuf, LastSuggestions->at (Result - 1));
          else
            SetStringDUtf8 (AnsiBuf, LastSuggestions->at (Result - 1));
          /*
          if (SuggestionsMode == SUGGESTIONS_CONTEXT_MENU)
          {
          SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETTARGETSTART, WUCPosition );
          SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETTARGETEND, WUCPosition  + WUCLength );
          SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_REPLACETARGET, -1, (LPARAM)AnsiBuf);
          SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL, WUCPosition  + strlen (AnsiBuf), WUCPosition  + strlen (AnsiBuf));
          }
          else
          */
          SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_REPLACESEL, 0, (LPARAM) AnsiBuf);

          CLEAN_AND_ZERO_ARR (AnsiBuf);
        }
      }
    }
    break;
  case LANGUAGE_MENU_ID:
    {
      int Result = 0;
      if (!GetUseAllocatedIds ())
        Result = LOBYTE (MenuId);
      else
        Result = MenuId - GetLangsMenuIdStart ();

      wchar_t *LangString = 0;
      if (Result == MULTIPLE_LANGS)
      {
        LangString = L"<MULTIPLE>";
      }
      else if (Result == CUSTOMIZE_MULTIPLE_DICS ||
               Result == DOWNLOAD_DICS ||
               Result == REMOVE_DICS)
      {
        // All actions are done in GUI thread in that case
        return;
      }
      else
        LangString = CurrentLangs->at (Result).OrigName;
      DoPluginMenuInclusion (TRUE);

      if (LibMode == 0)
        SetAspellLanguage (LangString);
      else
        SetHunspellLanguage (LangString);

      ReinitLanguageLists ();
      UpdateLangsMenu ();
      RecheckVisibleBothViews ();
      SaveSettings ();
      break;
    }
  }
}

void SpellChecker::FillSuggestionsMenu (HMENU Menu)
{
  if (!CurrentSpeller->IsWorking ())
    return; // Word is already off-screen

  int Pos = WUCPosition;
  Sci_TextRange Range;
  wchar_t *Buf = 0;
  Range.chrg.cpMin = WUCPosition;
  Range.chrg.cpMax = WUCPosition + WUCLength;
  Range.lpstrText = new char [WUCLength + 1];
  PostMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL, Pos, Pos + WUCLength);
  SuggestionMenuItems = new std::vector <SuggestionsMenuItem *>;

  SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);

  switch (CurrentEncoding)
  {
  case ENCODING_ANSI:
  case ENCODING_UTF8:
    SetString (SelectedWord, Range.lpstrText);
    break;

  case ENCODING_WCHAR:
    wchar_t *TempStr = 0;
    CLEAN_AND_ZERO_ARR (SelectedWord);
    SetStringSUtf8 (TempStr, Range.lpstrText);
    SelectedWord = (char *) TempStr;
    break;
  }

  ApplyConversions (SelectedWord);

  CLEAN_AND_ZERO_STRING_VECTOR (LastSuggestions);
  LastSuggestions = CurrentSpeller->GetSuggestions (SelectedWord);
  if (!LastSuggestions)
    return;

  for (unsigned int i = 0; i < LastSuggestions->size (); i++)
  {
    if (i >= (unsigned int) SuggestionsNum)
      break;
    // TODO: ANSI Version wouldn't work here
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (LastSuggestions->at (i), i + 1));
  }

  if (LastSuggestions->size () > 0)
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (L"", 0, MenuFlagEnum::SEPARATOR));

  wchar_t *MenuString = new wchar_t [WUCLength + 50 + 1]; // Add "" to dictionary
  char *BufUtf8 = 0;
  switch (CurrentEncoding)
  {
  case ENCODING_UTF8:
  case ENCODING_WCHAR: // In that case it has nothing to deal with delimiters so we're just doing the same stuff as for UTF-8
    SetString (BufUtf8, Range.lpstrText);
    break;
  case ENCODING_ANSI:
    SetStringDUtf8 (BufUtf8, Range.lpstrText);
    break;
  }
  ApplyConversions (BufUtf8);
  SetStringSUtf8 (Buf, BufUtf8);
  // TODO: make custom ignore list for aspell
  _swprintf (MenuString, L"Ignore \"%s\" for Current Session", Buf);
  SuggestionMenuItems->push_back (new SuggestionsMenuItem (MenuString, MID_IGNOREALL));
  _swprintf (MenuString, L"Add \"%s\" to Dictionary", Buf);
  MenuFlagEnum::e flag = MenuFlagEnum::SIMPLE_ITEM;
  if (
    MultiLangMode
    &&
    (   (LibMode == 0)
        || (LibMode == 1 && HunspellDicMode == HunspellDictionaryMode::SeparateForAll)
    )
  )
  {
    flag = MenuFlagEnum::MENU_PARENT;
  }

  SuggestionMenuItems->push_back (new SuggestionsMenuItem (MenuString, MID_ADDTODICTIONARY, flag));

  if (flag == MenuFlagEnum::MENU_PARENT)
  {
    for (unsigned int i = 0; i < MultiLangList->size (); i++)
    {
      SuggestionMenuItems->push_back (new SuggestionsMenuItem (MultiLangList->at (i), MID_ADD_TO_DICS_START + i, MenuFlagEnum::MENU_CHILD));
    }
  }

  if (SuggestionsMode == SUGGESTIONS_CONTEXT_MENU)
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (L"", 0, MenuFlagEnum::SEPARATOR));

  HMENU menu = 0;
  if (SuggestionsMode == SUGGESTIONS_BOX)
  {
    for (unsigned int i = 0; i < SuggestionMenuItems->size (); i++)
    {
      InsertSuggMenuItem (Menu, SuggestionMenuItems->at (i)->Text, SuggestionMenuItems->at (i)->Id, i, menu, SuggestionMenuItems->at (i)->Flag);
      CLEAN_AND_ZERO (SuggestionMenuItems->at (i));
    }
    CLEAN_AND_ZERO (SuggestionMenuItems);
  }

  CLEAN_AND_ZERO_ARR (Range.lpstrText);
  CLEAN_AND_ZERO_ARR (Buf);
  CLEAN_AND_ZERO_ARR (BufUtf8);

  CLEAN_AND_ZERO_ARR (MenuString);
}

void SpellChecker::UpdateAutocheckStatus (int SaveSetting)
{
  if (SaveSetting)
    SaveSettings ();

  SendMsgToNpp (NppDataInstance, NPPM_SETMENUITEMCHECK, get_funcItem ()[0]._cmdID, AutoCheckText);
}

void SpellChecker::SetCheckThose (int CheckThoseArg)
{
  CheckThose = CheckThoseArg;
}

void SpellChecker::SetFileTypes (wchar_t *FileTypesArg)
{
  SetString (FileTypes, FileTypesArg);
}

void SpellChecker::SetHunspellMultipleLanguages (const char *MultiLanguagesArg)
{
  SetString (HunspellMultiLanguages, MultiLanguagesArg);
}

void SpellChecker::SetAspellMultipleLanguages (const char *MultiLanguagesArg)
{
  SetString (AspellMultiLanguages, MultiLanguagesArg);
}

void SpellChecker::RefreshUnderlineStyle ()
{
  SendMsgToBothEditors(NppDataInstance, SCI_INDICSETSTYLE, SCE_ERROR_UNDERLINE, UnderlineStyle);
  SendMsgToBothEditors(NppDataInstance, SCI_INDICSETFORE, SCE_ERROR_UNDERLINE, UnderlineColor);
}

void SpellChecker::SetUnderlineColor (int Value)
{
  UnderlineColor = Value;
}

void SpellChecker::SetUnderlineStyle (int Value)
{
  UnderlineStyle = Value;
}

void SpellChecker::SetProxyUserName (wchar_t *Str)
{
  SetString (ProxyUserName, Str);
}

void SpellChecker::SetProxyHostName (wchar_t *Str)
{
  SetString (ProxyHostName, Str);
}

void SpellChecker::SetProxyPassword (wchar_t *Str)
{
  SetString (ProxyPassword, Str);
}

void SpellChecker::SetProxyPort (int Value)
{
  ProxyPort = Value;
}

void SpellChecker::SetUseProxy (BOOL Value)
{
  UseProxy = Value;
}

void SpellChecker::SetProxyAnonymous (BOOL Value)
{
  ProxyAnonymous = Value;
}

void SpellChecker::SetProxyType (int Value)
{
  ProxyType = Value;
}

wchar_t *SpellChecker::GetProxyUserName ()
{
  return ProxyUserName;
}

wchar_t *SpellChecker::GetProxyHostName ()
{
  return ProxyHostName;
}

wchar_t *SpellChecker::GetProxyPassword ()
{
  return ProxyPassword;
}

int SpellChecker::GetProxyPort ()
{
  return ProxyPort;
}

BOOL SpellChecker::GetUseProxy ()
{
  return UseProxy;
}

BOOL SpellChecker::GetProxyAnonymous ()
{
  return ProxyAnonymous;
}

int SpellChecker::GetProxyType ()
{
  return ProxyType;
}

void SpellChecker::SetIgnore (BOOL IgnoreNumbersArg, BOOL IgnoreCStartArg, BOOL IgnoreCHaveArg,
                              BOOL IgnoreCAllArg, BOOL Ignore_Arg, BOOL IgnoreSEApostropheArg, BOOL IgnoreOneLetterArg)
{
  IgnoreNumbers = IgnoreNumbersArg;
  IgnoreCStart = IgnoreCStartArg;
  IgnoreCHave = IgnoreCHaveArg;
  IgnoreCAll = IgnoreCAllArg;
  Ignore_ = Ignore_Arg;
  IgnoreSEApostrophe = IgnoreSEApostropheArg;
  IgnoreOneLetter = IgnoreOneLetterArg;
}

void SpellChecker::GetDefaultHunspellPath (wchar_t *&Path)
{
  Path = new wchar_t[MAX_PATH];
  wcscpy (Path, IniFilePath);
  wchar_t *Pointer = wcschr (Path, L'\\');
  *Pointer = 0;
  wcscat (Path, L"\\Hunspell");
}

void SpellChecker::SaveSettings ()
{
  FILE *Fp;
  _wfopen_s (&Fp, IniFilePath, L"w"); // Cleaning settings file (or creating it)
  fclose (Fp);
  wchar_t *TBuf = 0;
  if (!SettingsLoaded)
    return;
  SaveToIni (L"Autocheck", AutoCheckText, 1);
  SaveToIni (L"Hunspell_Multiple_Languages", HunspellMultiLanguages, L"");
  SaveToIni (L"Aspell_Multiple_Languages", AspellMultiLanguages, L"");
  SaveToIni (L"Hunspell_Language", HunspellLanguage, L"en_GB");
  SaveToIni (L"Aspell_Language", AspellLanguage, L"en");
  SaveToIni (L"Remove_User_Dics_On_Dic_Remove", RemoveUserDics, 0);
  SaveToIni (L"Remove_Dics_For_All_Users", RemoveSystem, 0);
  SaveToIni (L"Show_Only_Known", ShowOnlyKnown, TRUE);
  SaveToIni (L"Install_Dictionaries_For_All_Users", InstallSystem, FALSE);
  wchar_t Buf[DEFAULT_BUF_SIZE];
  for (int i = 0; i < countof (ServerNames); i++)
  {
    if (!*ServerNames[i])
      continue;
    _swprintf (Buf, L"Server_Address[%d]", i);
    SaveToIni (Buf, ServerNames[i], L"");
  }
  SaveToIni (L"Suggestions_Control", SuggestionsMode, 1);
  SaveToIni (L"Ignore_Yo", IgnoreYo, 0);
  SaveToIni (L"Convert_Single_Quotes_To_Apostrophe", ConvertSingleQuotes, 1);
  SaveToIni (L"Remove_Ending_And_Beginning_Apostrophe", RemoveBoundaryApostrophes, 1);
  SaveToIni (L"Check_Only_Comments_And_Strings", CheckComments, 1);
  SaveToIni (L"Check_Those_\\_Not_Those", CheckThose, 1);
  SaveToIni (L"File_Types", FileTypes, L"*.*");
  SaveToIni (L"Ignore_Having_Number", IgnoreNumbers, 1);
  SaveToIni (L"Ignore_Start_Capital", IgnoreCStart, 0);
  SaveToIni (L"Ignore_Have_Capital", IgnoreCHave, 1);
  SaveToIni (L"Ignore_All_Capital", IgnoreCAll, 1);
  SaveToIni (L"Ignore_With_", Ignore_, 1);
  SaveToIni (L"Ignore_That_Start_or_End_with_'", IgnoreSEApostrophe, 0);
  SaveToIni (L"Ignore_One_Letter", IgnoreOneLetter, 0);
  SaveToIni (L"Underline_Color", UnderlineColor, 0x0000ff);
  SaveToIni (L"Underline_Style", UnderlineStyle, INDIC_SQUIGGLE);
  wchar_t *Path = 0;
  GetDefaultAspellPath (Path);
  SaveToIni (L"Aspell_Path", AspellPath, Path);
  CLEAN_AND_ZERO_ARR (Path);
  GetDefaultHunspellPath (Path);
  SaveToIni (L"User_Hunspell_Path", HunspellPath, Path);
  SaveToIni (L"System_Hunspell_Path", AdditionalHunspellPath, L".\\plugins\\config\\Hunspell");
  CLEAN_AND_ZERO_ARR (Path);
  SaveToIni (L"Suggestions_Number", SuggestionsNum, 5);
  char *DefaultDelimUtf8 = 0;
  SetStringDUtf8 (DefaultDelimUtf8, DEFAULT_DELIMITERS);
  SaveToIni (L"Delimiter_Mode", DelimiterMode, 1);
  SaveToIniUtf8 (L"Delimiters", DelimUtf8, DefaultDelimUtf8, TRUE);
  SaveToIniUtf8 (L"Delimiter_Exceptions", DelimExcUtf8, DefaultDelimUtf8, TRUE);
  CLEAN_AND_ZERO_ARR (DefaultDelimUtf8);
  SaveToIni (L"Find_Next_Buffer_Size", BufferSize / 1024, 4);
  SaveToIni (L"Suggestions_Button_Size", SBSize, 15);
  SaveToIni (L"Suggestions_Button_Opacity", SBTrans, 70);
  SaveToIni (L"Library", LibMode, 1);
  PreserveCurrentAddressIndex ();
  SaveToIni (L"Last_Used_Address_Index", LastUsedAddress, 0);
  SaveToIni (L"Decode_Language_Names", DecodeNames, TRUE);
  SaveToIni (L"Hunspell_User_Dictionary_Mode", (int) HunspellDicMode, 1);

  SaveToIni (L"Use_Proxy", UseProxy, FALSE);
  SaveToIni (L"Proxy_User_Name", ProxyUserName, L"anonymous");
  SaveToIni (L"Proxy_Host_Name", ProxyHostName, L"");
  SaveToIni (L"Proxy_Password", ProxyPassword, L"");
  SaveToIni (L"Proxy_Port", ProxyPort, 808);
  SaveToIni (L"Proxy_Is_Anonymous", ProxyAnonymous, TRUE);
  SaveToIni (L"Proxy_Type", ProxyType, 0);
  SaveToIni (L"Recheck_Prevention_Type", RecheckPreventionType, 0);
  std::map<wchar_t *, DWORD, bool ( *)(wchar_t *, wchar_t *)>::iterator it = SettingsToSave->begin ();
  for (; it != SettingsToSave->end (); ++it)
  {
    SaveToIni ((*it).first, LOWORD ((*it).second), HIWORD ((*it).second));
  }
}

void SpellChecker::SetDecodeNames (BOOL Value)
{
  DecodeNames = Value;
}

HunspellDictionaryMode::e SpellChecker::GetHunspellDicMode ()
{
  return HunspellDicMode;
}

void SpellChecker::SetHunspellDicMode (HunspellDictionaryMode::e mode)
{
  HunspellDicMode = mode;
}

void SpellChecker::SetDelimiterMode (int mode)
{
  DelimiterMode = mode;
}
void SpellChecker::SetLibMode (int i)
{
  LibMode = i;
  if (i == 0)
  {
    AspellReinitSettings ();
    CurrentSpeller = AspellSpeller;
  }
  else
  {
    CurrentSpeller = HunspellSpeller;
    HunspellReinitSettings (0);
  }
}

void SpellChecker::LoadSettings ()
{
  char *BufUtf8 = 0;
  wchar_t *Path = 0;
  wchar_t *TBuf = 0;
  SettingsLoaded = TRUE;
  GetDefaultAspellPath (Path);
  LoadFromIni (AspellPath, L"Aspell_Path", Path);
  CLEAN_AND_ZERO_ARR (Path);
  GetDefaultHunspellPath (Path);
  LoadFromIni (HunspellPath, L"User_Hunspell_Path", Path);
  CLEAN_AND_ZERO_ARR (Path);

  AdditionalHunspellPath = 0;
  LoadFromIni (AdditionalHunspellPath, L"System_Hunspell_Path", L".\\plugins\\config\\Hunspell");

  LoadFromIni (SuggestionsMode, L"Suggestions_Control", 1);
  LoadFromIni (AutoCheckText, L"Autocheck", 1);
  UpdateAutocheckStatus (0);
  LoadFromIni (AspellMultiLanguages, L"Aspell_Multiple_Languages", L"");
  LoadFromIni (HunspellMultiLanguages, L"Hunspell_Multiple_Languages", L"");
  LoadFromIni (TBuf, L"Aspell_Language", L"en");
  SetAspellLanguage (TBuf);
  CLEAN_AND_ZERO_ARR (TBuf);
  LoadFromIni (TBuf, L"Hunspell_Language", L"en_GB");
  SetHunspellLanguage (TBuf);
  CLEAN_AND_ZERO_ARR (TBuf);

  LoadFromIni (DelimiterMode, L"Delimiter_Mode", 1);

  SetStringDUtf8 (BufUtf8, DEFAULT_DELIMITERS);
  LoadFromIniUtf8 (BufUtf8, L"Delimiters", BufUtf8, TRUE);
  SetDelimiters (BufUtf8);
  SetStringDUtf8 (BufUtf8, DEFAULT_DELIMITER_EXCEPTION);
  LoadFromIniUtf8 (BufUtf8, L"Delimiter_Exceptions", BufUtf8, TRUE);
  SetDelimiterException (BufUtf8);
  LoadFromIni (SuggestionsNum, L"Suggestions_Number", 5);
  LoadFromIni (IgnoreYo, L"Ignore_Yo", 0);
  LoadFromIni (ConvertSingleQuotes, L"Convert_Single_Quotes_To_Apostrophe", 1);
  LoadFromIni (RemoveBoundaryApostrophes, L"Remove_Ending_And_Beginning_Apostrophe", 1);
  LoadFromIni (CheckThose , L"Check_Those_\\_Not_Those", 1);
  LoadFromIni (FileTypes, L"File_Types", L"*.*");
  LoadFromIni (CheckComments, L"Check_Only_Comments_And_Strings", 1);
  LoadFromIni (UnderlineColor, L"Underline_Color", 0x0000ff);
  LoadFromIni (UnderlineStyle, L"Underline_Style", INDIC_SQUIGGLE);
  LoadFromIni (IgnoreNumbers, L"Ignore_Having_Number", 1);
  LoadFromIni (IgnoreCStart, L"Ignore_Start_Capital", 0);
  LoadFromIni (IgnoreCHave, L"Ignore_Have_Capital", 1);
  LoadFromIni (IgnoreCAll, L"Ignore_All_Capital", 1);
  LoadFromIni (IgnoreOneLetter, L"Ignore_One_Letter", 0);
  LoadFromIni (Ignore_, L"Ignore_With_", 1);
  int Value;
  LoadFromIni (Value, L"Hunspell_User_Dictionary_Mode", 1);
  HunspellDicMode = (HunspellDictionaryMode::e) Value;
  LoadFromIni (IgnoreSEApostrophe, L"Ignore_That_Start_or_End_with_'", 0);
  int i;

  HunspellSpeller->SetDirectory (HunspellPath);
  HunspellSpeller->SetAdditionalDirectory (AdditionalHunspellPath);
  AspellSpeller->Init (AspellPath);
  LoadFromIni (i, L"Library", 1);
  SetLibMode (i);
  int Size, Trans;
  LoadFromIni (Size, L"Suggestions_Button_Size", 15);
  LoadFromIni (Trans, L"Suggestions_Button_Opacity", 70);
  SetSuggBoxSettings (Size, Trans, 0);
  LoadFromIni (Size, L"Find_Next_Buffer_Size", 4);
  SetBufferSize (Size, 0);
  RefreshUnderlineStyle ();
  CLEAN_AND_ZERO_ARR (BufUtf8);
  LoadFromIni (ShowOnlyKnown, L"Show_Only_Known", TRUE);
  LoadFromIni (InstallSystem, L"Install_Dictionaries_For_All_Users", FALSE);
  wchar_t Buf[DEFAULT_BUF_SIZE];
  for (int i = 0; i < countof (ServerNames); i++)
  {
    _swprintf (Buf, L"Server_Address[%d]", i);
    LoadFromIni (ServerNames[i], Buf, L"");
  }
  LoadFromIni (LastUsedAddress, L"Last_Used_Address_Index", 0);
  LoadFromIni (RemoveUserDics, L"Remove_User_Dics_On_Dic_Remove", 0);
  LoadFromIni (RemoveSystem, L"Remove_Dics_For_All_Users", 0);
  LoadFromIni (DecodeNames, L"Decode_Language_Names", TRUE);

  LoadFromIni (UseProxy, L"Use_Proxy", FALSE);
  LoadFromIni (ProxyUserName, L"Proxy_User_Name", L"anonymous");
  LoadFromIni (ProxyHostName, L"Proxy_Host_Name", L"");
  LoadFromIni (ProxyPassword, L"Proxy_Password", L"");
  LoadFromIni (ProxyPort, L"Proxy_Port", 808);
  LoadFromIni (ProxyAnonymous, L"Proxy_Is_Anonymous", TRUE);
  LoadFromIni (ProxyType, L"Proxy_Type", 0);
  LoadFromIni (Value, L"Recheck_Prevention_Type", 0);
  RecheckPreventionType = (RecheckPreventionTypes::e) Value;
}

void SpellChecker::CreateWordUnderline (HWND ScintillaWindow, int start, int end)
{
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_INDICATORFILLRANGE, start, (end - start + 1));
}

void SpellChecker::RemoveUnderline (HWND ScintillaWindow, int start, int end)
{
  if (end < start)
    return;
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_INDICATORCLEARRANGE, start, (end - start + 1));
}

// Warning - temporary buffer will be created
char *SpellChecker::GetDocumentText ()
{
  int lengthDoc = (SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLENGTH) + 1);
  char *buf = new char [lengthDoc];
  SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETTEXT, lengthDoc, (LPARAM)buf);
  return buf;
}

void SpellChecker::GetVisibleLimits(long &Start, long &Finish)
{
  long top = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETFIRSTVISIBLELINE);
  long bottom = top + SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINESONSCREEN);
  top = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_DOCLINEFROMVISIBLE, top);
  bottom = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_DOCLINEFROMVISIBLE, bottom);
  long LineCount = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLINECOUNT);
  Start = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POSITIONFROMLINE, top);
  // Not using end of line position cause utf-8 symbols could be more than one char
  // So we use next line start as the end of our visible text
  if (bottom + 1 < LineCount)
    Finish = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POSITIONFROMLINE, bottom + 1);
  else
    Finish = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETTEXTLENGTH);
}

char *SpellChecker::GetVisibleText(long *offset, BOOL NotIntersectionOnly)
{
  Sci_TextRange range;
  GetVisibleLimits (range.chrg.cpMin, range.chrg.cpMax);

  if (range.chrg.cpMax < 0 || range.chrg.cpMin > range.chrg.cpMax)
    return 0;

  PreviousA = range.chrg.cpMin;
  PreviousB = range.chrg.cpMax;

  if (NotIntersectionOnly)
  {
    if (range.chrg.cpMin < PreviousA && range.chrg.cpMax >= PreviousA)
      range.chrg.cpMax = PreviousA - 1;
    else if (range.chrg.cpMax > PreviousB && range.chrg.cpMin <= PreviousB)
      range.chrg.cpMin = PreviousB + 1;
  }

  char *Buf = new char [range.chrg.cpMax - range.chrg.cpMin + 1]; // + one byte for terminating zero
  range.lpstrText = Buf;
  SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETTEXTRANGE, NULL , (LPARAM)&range);
  *offset = range.chrg.cpMin;
  Buf[range.chrg.cpMax - range.chrg.cpMin] = 0;
  return Buf;
}

void SpellChecker::ClearAllUnderlines ()
{
  int length = SendMsgToEditor(NppDataInstance, SCI_GETLENGTH);
  if (length > 0)
  {
    PostMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
    PostMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_INDICATORCLEARRANGE, 0, length);
  }
}

void SpellChecker::Cleanup()
{
  CLEAN_AND_ZERO_ARR (AspellLanguage);
  CLEAN_AND_ZERO_ARR (HunspellLanguage);
  CLEAN_AND_ZERO_ARR (AspellMultiLanguages);
  CLEAN_AND_ZERO_ARR (HunspellMultiLanguages);
  CLEAN_AND_ZERO_ARR (DelimUtf8);
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DelimConverted);
  CLEAN_AND_ZERO_ARR (AspellPath);
}

void SpellChecker::SetAspellPath (const wchar_t *Path)
{
  SetString (AspellPath, Path);
  AspellReinitSettings ();
}

void SpellChecker::SetHunspellPath (const wchar_t *Path)
{
  if (!Path || !*Path)
    return;
  SetString (HunspellPath, Path);
  HunspellReinitSettings (1);
}

void SpellChecker::SetHunspellAdditionalPath (const wchar_t *Path)
{
  if (!Path || !*Path)
    return;
  SetString (AdditionalHunspellPath, Path);
  HunspellReinitSettings (1);
}

void SpellChecker::SaveToIni (const wchar_t *Name, const wchar_t *Value, const wchar_t *DefaultValue, BOOL InQuotes)
{
  if (!Name || !Value)
    return;

  if (DefaultValue && wcscmp (Value, DefaultValue) == 0)
    return;

  if (InQuotes)
  {
    int Len = 1 + wcslen (Value) + 1 + 1;
    wchar_t *Buf = new wchar_t[Len];
    _swprintf (Buf, L"\"%s\"", Value, Len - 3);
    WritePrivateProfileString (L"SpellCheck", Name, Buf, IniFilePath);
    CLEAN_AND_ZERO_ARR (Buf);
  }
  else
  {
    WritePrivateProfileString (L"SpellCheck", Name, Value, IniFilePath);
  }
}

void SpellChecker::SaveToIni (const wchar_t *Name, int Value, int DefaultValue)
{
  if (!Name)
    return;

  if (Value == DefaultValue)
    return;

  wchar_t Buf[DEFAULT_BUF_SIZE];
  _itow_s (Value, Buf, 10);
  SaveToIni (Name, Buf, 0);
}

void SpellChecker::SaveToIniUtf8 (const wchar_t *Name, const char *Value,  const char *DefaultValue, BOOL InQuotes)
{
  if (!Name || !Value)
    return;

  if (DefaultValue && strcmp (Value, DefaultValue) == 0)
    return;

  wchar_t *Buf = 0;
  SetStringSUtf8 (Buf, Value);
  SaveToIni (Name, Buf, 0, InQuotes);
  CLEAN_AND_ZERO_ARR (Buf);
}

void SpellChecker::LoadFromIni (wchar_t *&Value, const wchar_t *Name, const wchar_t *DefaultValue, BOOL InQuotes)
{
  if (!Name || !DefaultValue)
    return;

  CLEAN_AND_ZERO_ARR (Value);
  Value = new wchar_t [DEFAULT_BUF_SIZE];

  GetPrivateProfileString (L"SpellCheck", Name, DefaultValue, Value, DEFAULT_BUF_SIZE, IniFilePath);

  if (InQuotes)
  {
    int Len = wcslen (Value);
    // Proof check for quotes
    if (Value[0] != '\"' || Value[Len] != '\"')
    {
      wcscpy_s (Value, DEFAULT_BUF_SIZE, DefaultValue);
      return;
    }

    for (int i = 0; i < Len; i++)
      Value[i] = Value[i + 1];

    Value[Len - 1] = 0;
  }
}

void SpellChecker::LoadFromIni (int &Value, const wchar_t *Name, int DefaultValue)
{
  if (!Name)
    return;

  wchar_t BufDefault[DEFAULT_BUF_SIZE];
  wchar_t *Buf = 0;
  _itow_s (DefaultValue, BufDefault, 10);
  LoadFromIni (Buf, Name, BufDefault);
  Value = _wtoi (Buf);
  CLEAN_AND_ZERO_ARR (Buf);
}

void SpellChecker::LoadFromIniUtf8 (char *&Value, const wchar_t *Name, const char *DefaultValue, BOOL InQuotes)
{
  if (!Name || !DefaultValue)
    return;

  wchar_t *BufDefault = 0;
  wchar_t *Buf = 0;
  SetStringSUtf8 (BufDefault, DefaultValue);
  LoadFromIni (Buf, Name, BufDefault);
  SetStringDUtf8 (Value, Buf);
  CLEAN_AND_ZERO_ARR (Buf);
  CLEAN_AND_ZERO_ARR (BufDefault);
}

// Here parameter is in ANSI (may as well be utf-8 cause only English I guess)
void SpellChecker::SetAspellLanguage (const wchar_t *Str)
{
  SetString (AspellLanguage, Str);

  if (wcscmp (Str, L"<MULTIPLE>") == 0)
  {
    SetMultipleLanguages (AspellMultiLanguages, AspellSpeller);
    AspellSpeller->SetMode (1);
    MultiLangMode = 1;
  }
  else
  {
    wchar_t *TBuf = 0;
    MultiLangMode = 0;
    SetString (TBuf, Str);
    AspellSpeller->SetLanguage (TBuf);
    CLEAN_AND_ZERO_ARR (TBuf);
    CurrentSpeller->SetMode (0);
  }
}

void SpellChecker::SetHunspellLanguage (const wchar_t *Str)
{
  SetString (HunspellLanguage, Str);

  if (wcscmp (Str, L"<MULTIPLE>") == 0)
  {
    SetMultipleLanguages (HunspellMultiLanguages, HunspellSpeller);
    MultiLangMode = 1;
    HunspellSpeller->SetMode (1);
  }
  else
  {
    MultiLangMode = 0;
    HunspellSpeller->SetLanguage (HunspellLanguage);
    HunspellSpeller->SetMode (0);
  }
}

const char *SpellChecker::GetDelimiters ()
{
  return DelimUtf8;
}

const char *SpellChecker::GetDelimiterException ()
{
  return DelimExcUtf8;
}

void SpellChecker::SetSuggestionsNum (int Num)
{
  SuggestionsNum = Num;
}

// Here parameter is in UTF-8
void SpellChecker::SetDelimiters (const char *Str)
{
  wchar_t *DestBuf = 0;
  wchar_t *SrcBuf = 0;
  SetString (DelimUtf8, Str);
  SetStringSUtf8 (SrcBuf, DelimUtf8);
  SetParsedString (DestBuf, SrcBuf);
  int TargetBufLength = wcslen (DestBuf) + 5 + 1;
  wchar_t *TargetBuf = new wchar_t [wcslen (DestBuf) + 5 + 1];
  wcscpy (TargetBuf, DestBuf);
  wcscat_s (TargetBuf, TargetBufLength, L" \n\r\t\v");
  SetStringDUtf8 (DelimUtf8Converted, TargetBuf);
  SetStringSUtf8 (DelimConverted, DelimUtf8Converted);
  SetStringSUtf8 (DelimConvertedWchar, DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DestBuf);
  CLEAN_AND_ZERO_ARR (SrcBuf);
  CLEAN_AND_ZERO_ARR (TargetBuf);
}

void SpellChecker::SetRecheckPreventionType (RecheckPreventionTypes::e Value)
{
  RecheckPreventionType = Value;
}

void SpellChecker::SetDelimiterException (const char *Str)
{
  wchar_t *DestBuf = 0;
  wchar_t *SrcBuf = 0;
  SetString (DelimExcUtf8, Str);
  SetStringSUtf8 (SrcBuf, DelimExcUtf8);
  SetParsedString (DestBuf, SrcBuf);
  SetString (DelimExcUtf8Converted, DestBuf);
  SetStringSUtf8 (DelimExcConverted, DelimExcUtf8Converted);
  SetStringSUtf8 (DelimExcConvertedWchar, DelimExcUtf8Converted);
  CLEAN_AND_ZERO_ARR (DestBuf);
  CLEAN_AND_ZERO_ARR (SrcBuf);
}

void SpellChecker::SetMultipleLanguages (const wchar_t *MultiString, AbstractSpellerInterface *Speller)
{
  wchar_t *Context = 0;
  CLEAN_AND_ZERO_STRING_VECTOR (MultiLangList);
  MultiLangList = new std::vector<wchar_t *>;
  wchar_t *MultiStringCopy = 0;
  wchar_t *Token = 0;
  wchar_t *TBuf = 0;

  SetString (MultiStringCopy, MultiString);
  Token = wcstok_s (MultiStringCopy, L"|", &Context);
  while (Token)
  {
    TBuf = 0;
    SetString (TBuf, Token);
    MultiLangList->push_back (TBuf);
    Token = wcstok_s (NULL, L"|", &Context);
  }

  Speller->SetMultipleLanguages (MultiLangList);
  CLEAN_AND_ZERO_ARR (MultiStringCopy);
}

BOOL SpellChecker::HunspellReinitSettings (BOOL ResetDirectory)
{
  wchar_t *TBuf = 0;
  if (ResetDirectory)
  {
    HunspellSpeller->SetDirectory (HunspellPath);
    HunspellSpeller->SetAdditionalDirectory (AdditionalHunspellPath);
  }
  char *MultiLanguagesCopy = 0;
  if (wcscmp (HunspellLanguage, L"<MULTIPLE>") != 0)
    HunspellSpeller->SetLanguage (HunspellLanguage);
  else
    SetMultipleLanguages (HunspellMultiLanguages, HunspellSpeller);

  CLEAN_AND_ZERO_ARR (MultiLanguagesCopy);
  return TRUE;
}

BOOL SpellChecker::AspellReinitSettings ()
{
  wchar_t *TBuf = 0;
  AspellSpeller->Init (AspellPath);

  if (wcscmp (AspellLanguage, L"<MULTIPLE>") != 0)
  {
    AspellSpeller->SetLanguage (AspellLanguage);
  }
  else
    SetMultipleLanguages (AspellMultiLanguages, AspellSpeller);
  return TRUE;
}

void SpellChecker::SetBufferSize (int Size, BOOL SaveIni)
{
  if (Size < 1)
    Size = 1;
  if (Size > 10 * 1024)
    Size = 10 * 1024;
  BufferSize = Size * 1024;
}

void SpellChecker::SetSuggBoxSettings (int Size, int Transparency, int SaveIni)
{
  if (SBSize != Size)
  {
    SBSize = Size;
    if (SaveIni)
      HideSuggestionBox ();
  }

  if (Transparency != SBTrans)
  {
    SBTrans = Transparency;
    // Don't sure why but this helps to fix a bug with notepad++ window resizing
    // TODO: Fix it normal way
    SetLayeredWindowAttributes(SuggestionsInstance->getHSelf (), 0, (255 * SBTrans) / 100, LWA_ALPHA);
  }
}

void SpellChecker::ApplyConversions (char *Word) // In Utf-8, Maybe shortened during conversion
{
  const char *ConvertFrom [3];
  const char *ConvertTo [3];
  int Apply[3] = {IgnoreYo, IgnoreYo, ConvertSingleQuotes};

  switch (CurrentEncoding)
  {
  case ENCODING_ANSI:
    {
      ConvertFrom[0] = YoANSI;
      ConvertFrom[1] = yoANSI;
      ConvertFrom[2] = PunctuationApostropheANSI;
      ConvertTo[0] = YeANSI;
      ConvertTo[1] = yeANSI;
      ConvertTo[2] = "\'";
      break;
    }
  case ENCODING_UTF8:
    {
      ConvertFrom[0] = Yo;
      ConvertFrom[1] = yo;
      ConvertFrom[2] = PunctuationApostrophe;
      ConvertTo[0] = Ye;
      ConvertTo[1] = ye;
      ConvertTo[2] = "\'";
      break;
    }
  case ENCODING_WCHAR:
    ConvertFrom[0] = (char *) YoWCHAR;
    ConvertFrom[1] = (char *) yoWCHAR;
    ConvertFrom[2] = (char *) PunctuationApostropheWCHAR;
    ConvertTo[0] = (char *) YeWCHAR;
    ConvertTo[1] = (char *) yeWCHAR;
    ConvertTo[2] = (char *) L"\'";
    break;
  }

  // FOR NOW It works only if destination string is shorter than source string.

  switch (CurrentEncoding)
  {
  case ENCODING_ANSI:
  case ENCODING_UTF8:
    {
      for (int i = 0; i < countof (ConvertFrom); i++)
      {
        if (!Apply[i] || ConvertFrom [i] == 0 || ConvertTo[i] == 0 || *ConvertFrom [i] == 0 || *ConvertTo [i] == 0)
          continue;

        char *Iter = Word;
        char *NestedIter = 0;
        int Diff = strlen (ConvertFrom[i]) - strlen (ConvertTo[i]);
        if (Diff < 0)
          continue; // For now this case isn't needed.
        while (Iter = strstr (Iter, ConvertFrom[i]))
        {
          for (size_t j = 0; j < strlen (ConvertTo[i]); j++)
          {
            *Iter = ConvertTo[i][j];
            Iter++;
          }
          NestedIter = Iter;
          while (*(NestedIter + Diff))
          {
            *NestedIter =  *(NestedIter + Diff);
            NestedIter++;
          }
          for (int j = 0; j < Diff; j++)
            *(NestedIter + j) = 0;
        }
      }
      break;
    }
  case ENCODING_WCHAR:
    {
      for (int i = 0; i < countof (ConvertFrom); i++)
      {
        if (!Apply[i] || ConvertFrom [i] == 0 || ConvertTo[i] == 0 || *((wchar_t *) ConvertFrom [i]) == 0 || *((wchar_t *) ConvertTo [i]) == 0)
          continue;

        wchar_t *Iter = (wchar_t *) Word;
        wchar_t *NestedIter = 0;
        int Diff = wcslen ((wchar_t *) ConvertFrom[i]) - wcslen ((wchar_t *) ConvertTo[i]);
        if (Diff < 0)
          continue; // For now this case isn't needed.
        while (Iter = wcsstr (Iter, (wchar_t *) ConvertFrom[i]))
        {
          for (size_t j = 0; j < wcslen ((wchar_t *) ConvertTo[i]); j++)
          {
            *Iter = ((wchar_t *)ConvertTo[i])[j];
            Iter++;
          }
          NestedIter = Iter;
          while (*(NestedIter + Diff))
          {
            *NestedIter =  *(NestedIter + Diff);
            NestedIter++;
          }
          for (int j = 0; j < Diff; j++)
            *(NestedIter + j) = 0;
        }
      }
      break;
    }
  }
  unsigned int LastChar = strlen (Word) - 1; // Apostrophe is ASCII char so it's leading anyway
}

void SpellChecker::ResetHotSpotCache ()
{
  memset (HotSpotCache, -1, sizeof (HotSpotCache));
}

BOOL SpellChecker::CheckWord (char *Word, long Start, long End, BOOL ignoreProtection)
{
  BOOL res = FALSE;
  if (!CurrentSpeller->IsWorking () || !Word || !*Word)
    return TRUE;

  if (Start <= LastCurPos && LastCurPos <= End && WordNearCursorProtection && RecheckPreventionType == RecheckPreventionTypes::FIREFOX_LIKE && !ignoreProtection)
    return TRUE;
  // Well Numbers have same codes for ANSI and Unicode I guess, so
  // If word contains number then it's probably just a number or some crazy name
  int Style = GetStyle (Start);
  if (CheckComments && !CheckWordInCommentOrString (Lexer, Style))
    return TRUE;

  if (HotSpotCache[Style] == -1)
    HotSpotCache[Style] = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_STYLEGETHOTSPOT, Style);

  if (HotSpotCache[Style] == 1)
    return TRUE;

  ApplyConversions (Word);

  wchar_t *Ts = 0;
  long SymbolsNum = 0;
  switch (CurrentEncoding)
  {
  case ENCODING_UTF8:
    SymbolsNum = Utf8Length (Word);
    break;
  case ENCODING_ANSI:
    SymbolsNum = strlen (Word);
    break;
  case ENCODING_WCHAR:
    SymbolsNum = wcslen ((wchar_t *) Word);
    break;
  }

  if (SymbolsNum == 0)
  {
    res = TRUE;
    goto CleanUp;
  }

  if (IgnoreOneLetter && SymbolsNum == 1)
  {
    res = TRUE;
    goto CleanUp;
  }

  bool ContainsNumbers = false;
  if (IgnoreNumbers)
  {
    switch (CurrentEncoding)
    {
    case ENCODING_UTF8:
      ContainsNumbers = (Utf8pbrk (Word, "0123456789") != 0);
      break;
    case ENCODING_ANSI:
      ContainsNumbers = (strpbrk (Word, "0123456789") != 0);
      break;
    case ENCODING_WCHAR:
      ContainsNumbers = (wcspbrk ((wchar_t *) Word, L"0123456789") != 0);
      break;
    }
  }

  if (IgnoreNumbers && ContainsNumbers)
  {
    res = TRUE;
    goto CleanUp;
  }

  if (IgnoreCStart || IgnoreCHave || IgnoreCAll)
  {

    switch (CurrentEncoding)
    {
    case ENCODING_UTF8:
      SetStringSUtf8 (Ts, Word);
      break;
    case ENCODING_ANSI:
      SetString (Ts, Word);
      break;
    case ENCODING_WCHAR:
      SetString (Ts, (wchar_t *) Word);
      break;
    }
    if (IgnoreCStart && IsCharUpper (Ts[0]))
    {
      res = TRUE;
      goto CleanUp;
    }
    if (IgnoreCHave || IgnoreCAll)
    {
      BOOL AllUpper = IsCharUpper (Ts[0]);
      for (unsigned int i = 1; i < wcslen (Ts); i++)
      {
        if (IsCharUpper (Ts[i]))
        {
          if (IgnoreCHave)
          {
            res = TRUE;
            goto CleanUp;
          }
        }
        else
          AllUpper = FALSE;
      }

      if (AllUpper && IgnoreCAll)
      {
        res = TRUE;
        goto CleanUp;
      }
    }
  }

  switch (CurrentEncoding)
  {
  case ENCODING_UTF8:
  case ENCODING_ANSI:
    if (Ignore_ && strchr (Word, '_') != 0) // I guess the same for UTF-8 and ANSI
    {
      res = TRUE;
      goto CleanUp;
    }
    break;
  case ENCODING_WCHAR:
    if (Ignore_ && wcschr ((wchar_t *) Word, L'_') != 0) // I guess the same for UTF-8 and ANSI
    {
      res = TRUE;
      goto CleanUp;
    }
    break;
  }

  unsigned int Len = 0;;

  if (IgnoreSEApostrophe)
  {
    switch (CurrentEncoding)
    {
    case ENCODING_UTF8:
    case ENCODING_ANSI:
      Len = strlen (Word);
      if (Word[0] == '\'' || Word[Len - 1] == '\'')
      {
        res = TRUE;
        goto CleanUp;
      }
      break;
    case ENCODING_WCHAR:
      if (((wchar_t *)Word)[0] == L'\'' || ((wchar_t *)Word)[Len - 1] == L'\'')
      {
        res = TRUE;
        goto CleanUp;
      }
      break;
    }
  }

  res = CurrentSpeller->CheckWord (Word);

CleanUp:
  CLEAN_AND_ZERO_ARR (Ts);

  return res;
}

void SpellChecker::CheckSpecialDelimeters (char *&Word, long &WordStart, long &WordEnd)
{
  switch (CurrentEncoding)
  {
  case ENCODING_ANSI:
  case ENCODING_UTF8:
    if (RemoveBoundaryApostrophes)
    {
      while (*Word == '\'' && *Word != '\0')
      {
        *Word = '\0';
        Word++;
        WordStart++;
      }

      char *it = Word + strlen (Word) - 1;
      while (*it == '\'' && *it != '\0' && it > Word)
      {
        *it = '\0';
        WordEnd--;
        it--;
      }
    }
    break;
  case ENCODING_WCHAR:
    wchar_t *WordWchar = (wchar_t *) Word;
    if (RemoveBoundaryApostrophes)
    {
      while (*WordWchar == L'\'' && *WordWchar != L'\0')
      {
        *WordWchar = '\0';
        WordWchar++;
        WordStart++;
      }

      wchar_t *it = WordWchar + wcslen (WordWchar) - 1;
      while (*it == L'\'' && *it != L'\0' && it > (wchar_t *) Word)
      {
        *it = L'\0';
        WordEnd--;
        it--;
      }
    }
    break;
  }
}

// Delims are taken internally
char *SpellChecker::DoCommonStringTokenization (char *Str, char **Context)
{
  char *Res = 0;
  switch (DelimiterMode)
  {
  case DelimiterModes::SPECIFIED:

    switch (CurrentEncoding)
    {
    case ENCODING_UTF8:
      return Utf8strtok (Str, DelimUtf8Converted, Context);
      break;
    case ENCODING_ANSI:
      return strtok_s (Str, DelimConverted, Context);
      break;
    case ENCODING_WCHAR:
      return (char *) wcstok_s ((wchar_t *) Str, DelimConvertedWchar, (wchar_t **) Context);
      break;
    }
    break;
  case DelimiterModes::ALLEXCEPT:

    switch (CurrentEncoding)
    {
    case ENCODING_UTF8:
      return 0;
      // UNSUPPORTED
      break;
    case ENCODING_ANSI:
      return strtok_s_nonalnum (Str, DelimExcConverted, Context);
      break;
    case ENCODING_WCHAR:
      return (char *) wcstok_s_nonalnum ((wchar_t *) Str, DelimExcConvertedWchar, (wchar_t **) Context);
      break;
    }
    break;
  }
  return 0;
}

BOOL SpellChecker::CheckText (char *InputText, long Offset, CheckTextMode Mode)
{
  if (!InputText || !*InputText)
    return FALSE;

  char *TextToCheck = 0;
  size_t *Indexation = 0;
  if (CurrentEncoding == ENCODING_WCHAR)
  {
    wchar_t *WcharString =  0;
    SetStringSUtf8Safe (WcharString, InputText, Indexation);
    TextToCheck = (char *) WcharString;
  }
  else
    SetString (TextToCheck, InputText);

  HWND ScintillaWindow = GetCurrentScintilla ();
  int oldid = SendMsgToEditor (ScintillaWindow, NppDataInstance, SCI_GETINDICATORCURRENT);
  char *Context = 0; // Temporary variable for strtok_s usage
  char *token;
  BOOL stop = FALSE;
  long ResultingWordEnd = -1, ResultingWordStart = -1;
  long TextLen = strlen (InputText);
  std::vector<long> UnderlineBuffer;
  long WordStart = 0;
  long WordEnd = 0;

  if (!DelimUtf8)
    return FALSE;

  token = DoCommonStringTokenization (TextToCheck, &Context);

  while (token)
  {
    if (token)
    {
      switch (CurrentEncoding)
      {
      case ENCODING_UTF8:
      case ENCODING_ANSI:
        WordStart = Offset + token - TextToCheck;
        WordEnd = Offset + token - TextToCheck + strlen (token);
        break;
      case ENCODING_WCHAR:
        WordStart = Offset + Indexation [(token - TextToCheck) >> 1];
        WordEnd = Offset + Indexation [((token - TextToCheck) >> 1) + wcslen ((wchar_t *) token)];
        break;
      }
      CheckSpecialDelimeters (token, WordStart, WordEnd);
      if (WordEnd < WordStart)
        goto newtoken;


      if (!CheckWord (token, WordStart, WordEnd, FALSE))
      {
        switch (Mode)
        {
        case UNDERLINE_ERRORS:
          UnderlineBuffer.push_back (WordStart);
          UnderlineBuffer.push_back (WordEnd);
          break;
        case FIND_FIRST:
          if (WordEnd > CurrentPosition)
          {
            SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL, WordStart, WordEnd);
            stop = TRUE;
          }
          break;
        case FIND_LAST:
          {
            if (WordEnd >= CurrentPosition)
            {
              stop = TRUE;
              break;
            }
            ResultingWordStart = WordStart;
            ResultingWordEnd = WordEnd;
          }
          break;
        }
        if (stop)
          break;
      }
      else
      {
      }
    }

newtoken:
    token = DoCommonStringTokenization (NULL, &Context);
  }

  if (Mode == UNDERLINE_ERRORS)
  {
    long PrevPos = Offset;
    for (long i = 0; i < (long) UnderlineBuffer.size () - 1; i += 2)
    {
      RemoveUnderline (ScintillaWindow, PrevPos, UnderlineBuffer[i] - 1);
      CreateWordUnderline (ScintillaWindow, UnderlineBuffer[i], UnderlineBuffer[i + 1] - 1);
      PrevPos = UnderlineBuffer[i + 1];
    }
    RemoveUnderline (ScintillaWindow, PrevPos, Offset + TextLen);
  }

  // PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, oldid);
  switch (CurrentEncoding)
  {
  case ENCODING_UTF8:
    break;
  case ENCODING_ANSI:
    break;
  case ENCODING_WCHAR:
    CLEAN_AND_ZERO_ARR (TextToCheck);
    CLEAN_AND_ZERO_ARR (Indexation);
    break;
  }


  switch (Mode)
  {
  case UNDERLINE_ERRORS:
    return TRUE;
  case FIND_FIRST:
    return stop;
  case FIND_LAST:
    if (ResultingWordStart == -1)
      return FALSE;
    else
    {
      SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL, ResultingWordStart, ResultingWordEnd);
      return TRUE;
    }
  };
  return FALSE;
}

void SpellChecker::ClearVisibleUnderlines ()
{
  int length = SendMsgToEditor(NppDataInstance, SCI_GETLENGTH);
  if (length > 0)
  {
    PostMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
    PostMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_INDICATORCLEARRANGE, 0, length);
  }
}

void SpellChecker::CheckVisible (BOOL NotIntersectionOnly)
{
  CLEAN_AND_ZERO_ARR (VisibleText);
  VisibleText = GetVisibleText (&VisibleTextOffset, NotIntersectionOnly);
  if (!VisibleText)
    return;

  CheckText (VisibleText, VisibleTextOffset, UNDERLINE_ERRORS);
}

void SpellChecker::setEncodingById (int EncId)
{
  /*
  int CCH;
  char szCodePage[10];
  char *FinalString;
  */
  switch (EncId)
  {
  case SC_CP_UTF8:
    CurrentEncoding = ENCODING_WCHAR; // Conversion should be done manually
    // CurrentEncoding = ENCODING_UTF8;
    // SetEncoding ("utf-8");
    break;
  default:
    {
      CurrentEncoding = ENCODING_ANSI;
      /*
      CCH = GetLocaleInfoA (GetSystemDefaultLCID(),
      LOCALE_IDEFAULTANSICODEPAGE,
      szCodePage,
      countof (szCodePage));
      FinalString = new char [2 + strlen (szCodePage) + 1];
      strcpy (FinalString, "cp"); // Actually this encoding may as well be ISO-XXXX, that's why this code sucks
      strcat (FinalString, szCodePage);
      SetEncoding (FinalString);
      break;
      */
    }
  }
  HunspellSpeller->SetEncoding (CurrentEncoding);
  AspellSpeller->SetEncoding (CurrentEncoding);
}

void SpellChecker::SwitchAutoCheck ()
{
  AutoCheckText = !AutoCheckText;
  UpdateAutocheckStatus ();
  RecheckVisibleBothViews ();
}

void SpellChecker::RecheckModified ()
{
  if (!CurrentSpeller->IsWorking ())
  {
    ClearAllUnderlines ();
    return;
  }

  long FirstModifiedLine = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINEFROMPOSITION, ModifiedStart);
  long LastModifiedLine = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_LINEFROMPOSITION, ModifiedEnd);
  long LineCount = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLINECOUNT);
  long FirstPossiblyModifiedPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POSITIONFROMLINE, FirstModifiedLine);
  long LastPossiblyModifiedPos = 0;
  if (LastModifiedLine + 1 < LineCount)
    LastPossiblyModifiedPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_POSITIONFROMLINE, LastModifiedLine + 1);
  else
    LastPossiblyModifiedPos = SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETLENGTH);

  Sci_TextRange Range;
  Range.chrg.cpMin = FirstPossiblyModifiedPos;
  Range.chrg.cpMax = LastPossiblyModifiedPos;
  Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
  SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);

  CheckText (Range.lpstrText, FirstPossiblyModifiedPos, UNDERLINE_ERRORS);
  CLEAN_AND_ZERO_ARR (Range.lpstrText);
}

void SpellChecker::SetConversionOptions (BOOL ConvertYo, BOOL ConvertSingleQuotesArg, BOOL RemoveBoundaryApostrophesArg)
{
  IgnoreYo = ConvertYo;
  ConvertSingleQuotes = ConvertSingleQuotesArg;
  RemoveBoundaryApostrophes = RemoveBoundaryApostrophesArg;
}

void SpellChecker::RecheckVisible (BOOL NotIntersectionOnly)
{
  int CodepageId = 0;
  if (!CurrentSpeller->IsWorking ())
  {
    ClearAllUnderlines ();
    return;
  }
  CodepageId = (int) SendMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_GETCODEPAGE, 0, 0);
  setEncodingById (CodepageId); // For now it just changes should we convert it to utf-8 or no
  if (CheckTextNeeded ())
    CheckVisible (NotIntersectionOnly);
  else
    ClearAllUnderlines ();
}

void SpellChecker::ErrorMsgBox (const wchar_t *message)
{
  wchar_t buf [DEFAULT_BUF_SIZE];
  swprintf_s (buf, L"%s", "DSpellCheck Error:", message, wcslen (message));
  MessageBox (NppDataInstance->_nppHandle, message, L"Error Happened!", MB_OK | MB_ICONSTOP);
}