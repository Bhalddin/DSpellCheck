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
#include <windows.h>

#include <map>
#include <set>
#include <hash_set>
#include <string.h>
#include <algorithm>
#include <stack>
#include <cstring>
#include <time.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <wchar.h>
#include <vector>
#include <Shlobj.h>
#include <string>
#include <iosfwd>
#include <sstream>

using std::wstring;
using std::vector;
using std::wstringstream;
using std::map;

#if (_MSC_VER >= 1700)
#define _CRTDBG_MAP_ALLOC
#ifdef _DEBUG
#ifndef DEBUG_NEW
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#endif
#include <stdlib.h>
#include <crtdbg.h>
#endif //_MSC_VER >= 1700