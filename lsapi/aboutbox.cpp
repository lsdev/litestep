//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// This is a part of the Litestep Shell source code.
//
// Copyright (C) 1997-2007  Litestep Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "../utility/common.h"
#include "../litestep/resource.h"
#include <commctrl.h>
#include <WindowsX.h>
#include <math.h>
#include "../utility/core.hpp"
#include "../utility/shellhlp.h"

typedef void (*AboutFunction)(HWND);

void AboutBangs(HWND hListView);
void AboutDevTeam(HWND hListView);
void AboutLicense(HWND hEdit);
void AboutModules(HWND hListView);
void AboutRevIDs(HWND hListView);
void AboutSysInfo(HWND hListView);

//
// misc functions
//
INT_PTR CALLBACK AboutBoxProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

HFONT CreateSimpleFont(LPCSTR pszName, int nSizeInPoints, bool bBold);
int GetClientWidth(HWND hWnd);
void TrimLeft(char* pszToTrim);
void FormatBytes(size_t stBytes, LPSTR pszBuffer, size_t cchBuffer);

// Global handle to the running AboutBox instance (if any)
HWND g_hAboutbox = NULL;

/* Keep this enum synch'd with aboutOptions */
enum
{
	 ABOUT_BANGS = 0
	,ABOUT_DEVTEAM
	,ABOUT_LICENSE
	,ABOUT_MODULES
	,ABOUT_REVIDS
	,ABOUT_SYSINFO
};

struct
{
	const char *option;
	AboutFunction function;
} aboutOptions[] = {
	 {"Bang Commands",      AboutBangs}
	,{"Development Team",   AboutDevTeam}
	,{"License",            AboutLicense}
	,{"Loaded Modules",     AboutModules}
	,{"Revision IDs",       AboutRevIDs}
	,{"System Information", AboutSysInfo}
};

struct
{
	const char *nick;
	const char *realName;
} theDevTeam[] = {
	 {"Acidfire", "Alexander Vermaat"}
	,{"ilmcuts",  "Simon"}
	,{"jugg",     "Chris Rempel"}
	,{"Maduin",   "Kevin Schaffer"}
	,{"RabidCow", "Joshua Seagoe"}
	,{"Tobbe",    "Tobbe Lundberg"}
	,{"Xjill",    ""}
};

const unsigned int aboutOptionsCount = COUNTOF(aboutOptions);
const unsigned int theDevTeamCount = COUNTOF(theDevTeam);

struct CallbackInfo
{
	HWND hListView;
	int nItem;
};

/* LiteStep license notice */
const char * lsLicense = \
 "LiteStep is a replacement shell for the standard Windows� Explorer shell.\r\n"
 "\r\n"
 "Copyright (C) 1997-1998  Francis Gastellu\r\n"
 "Copyright (C) 1998-2009  LiteStep Development Team\r\n"
 "\r\n"
 "This program is free software; you can redistribute it and/or modify it under "
 "the terms of the GNU General Public License as published by the Free Software "
 "Foundation; either version 2 of the License, or (at your option) any later "
 "version.\r\n"
 "\r\n"
 "This program is distributed in the hope that it will be useful, but WITHOUT ANY "
 "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A "
 "PARTICULAR PURPOSE.  See the GNU General Public License for more details.\r\n"
 "\r\n"
 "You should have received a copy of the GNU General Public License along with "
 "this program; if not, write to the Free Software Foundation, Inc., 51 Franklin "
 "Street, Fifth Floor, Boston, MA  02110-1301, USA.\r\n"
 "\r\n"
 "http://www.lsdev.org/";

//
// AboutBox Dialog Procedure
//
INT_PTR CALLBACK AboutBoxProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_COMBOBOX &&
			    HIWORD(wParam) == CBN_SELCHANGE)
			{
                HWND hComboBox = (HWND)lParam;
                HWND hListView = GetDlgItem(hWnd, IDC_LISTVIEW);

				// Delete listview items
                ListView_DeleteAllItems(hListView);

				// Delete listview columns
				for (int nCol = 3; nCol >= 0; nCol--)
				{
                    ListView_DeleteColumn(hListView, nCol);
				}

				// get new selection
				int i = ComboBox_GetCurSel(hComboBox);

				switch (i)
				{
				default:
					// default to revision IDs
					i = ABOUT_REVIDS;
                    ComboBox_SetCurSel(hComboBox, ABOUT_REVIDS);
					/* FALL THROUGH */
				case ABOUT_REVIDS:
				case ABOUT_BANGS:
				case ABOUT_DEVTEAM:
				case ABOUT_MODULES:
				case ABOUT_SYSINFO:
					// set the current display to the list view
					aboutOptions[i].function(hListView);

					ShowWindow(hListView, SW_SHOWNA);
					ShowWindow(GetDlgItem(hWnd, IDC_EDIT), SW_HIDE);
					break;

				case ABOUT_LICENSE:
					// set the current display to the edit box
					aboutOptions[i].function(GetDlgItem(hWnd, IDC_EDIT));

					ShowWindow(GetDlgItem(hWnd, IDC_EDIT), SW_SHOWNA);
					ShowWindow(hListView, SW_HIDE);
					break;
				}
			}
			else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				HFONT hTitleFont = (HFONT)SendDlgItemMessage(hWnd, IDC_TITLE,
					WM_GETFONT, 0, 0);

				// close the dialog box
				EndDialog(hWnd, IDOK);

				// release title font
				DeleteObject(hTitleFont);

				return TRUE;
			}
			/* This isn't necessary as we have the edit control set to read only.
			 * It just ensures our text doesn't get changed somehow */
			else if (LOWORD(wParam) == IDC_EDIT && HIWORD(wParam) == EN_UPDATE)
			{
                HWND hEditCtl = GetDlgItem(hWnd, IDC_EDIT);

                DWORD dwStart = 0;
                SendMessage(hEditCtl, EM_GETSEL, (WPARAM)&dwStart, 0);

				Edit_SetText(hEditCtl, lsLicense);
                Edit_SetSel(hEditCtl, dwStart-1, dwStart-1);
			}

			return FALSE;
		}

		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			HBRUSH hbReturn = FALSE; // special return value to tell the system
			                         // to perform default message processing

			// the header and title need a white (COLOR_WINDOW) background
			int id = GetDlgCtrlID((HWND)lParam);

			if (id == IDC_TITLE || id == IDC_THEME_INFO || id == IDC_EDIT)
			{
				HDC hDC = (HDC)wParam;

				SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
				SetBkColor(hDC, GetSysColor(COLOR_WINDOW));

				hbReturn = GetSysColorBrush(COLOR_WINDOW);
			}
			else if (id == IDC_HEADER || id == IDC_LOGO)
			{
				hbReturn = GetSysColorBrush(COLOR_WINDOW);
			}

			return (INT_PTR)hbReturn;
		}

		case WM_INITDIALOG:
		{
			// save global handle
			g_hAboutbox = hWnd;

            // Add custom extended styles to ListView
            DWORD dwStyles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;

            HWND hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
            ListView_SetExtendedListViewStyleEx(hListView, dwStyles, dwStyles);

			// set title font
			HFONT hTitleFont = CreateSimpleFont("Verdana", 14, false);
			SendDlgItemMessage(hWnd, IDC_TITLE, WM_SETFONT,
				(WPARAM)hTitleFont, FALSE);

			// set title with LS version
			SetDlgItemText(hWnd, IDC_TITLE, "LiteStep 0.25.0 Alpha");

			// set Theme info
			char themeAuthor[16] = { 0 };
			char themeName[21] = { 0 };
			char themeOut[MAX_LINE_LENGTH] = { 0 };

			GetRCString("ThemeAuthor", themeAuthor, "(unknown)", sizeof(themeAuthor));

			if (GetRCString("ThemeName", themeName, NULL, sizeof(themeName)))
			{
				StringCchPrintf(themeOut, MAX_LINE_LENGTH,
					"Theme: %s by %s", themeName, themeAuthor);
			}
			else
			{
				StringCchPrintf(themeOut, MAX_LINE_LENGTH,
					"Theme by %s", themeAuthor);
			}

			SetDlgItemText(hWnd, IDC_THEME_INFO, themeOut);

			// set compile time
			char compileTime[42] = {0};
			LSGetVariableEx("CompileDate", compileTime, 42);
			SetDlgItemText(hWnd, IDC_COMPILETIME, compileTime);

			// set the License Notice text
			SetDlgItemText(hWnd, IDC_EDIT, lsLicense);

            //
            // Initialize ComboBox
            //
            HWND hComboBox = GetDlgItem(hWnd, IDC_COMBOBOX);

			// add options to combo box
			for (unsigned int i = 0; i < aboutOptionsCount; ++i)
			{
                ComboBox_AddString(hComboBox, aboutOptions[i].option);
			}

			// default to License Notice
            ComboBox_SetCurSel(hComboBox, ABOUT_LICENSE);

			// SetCurSel doesn't notify us via WM_COMMAND, so force it
            FORWARD_WM_COMMAND(
                hWnd, IDC_COMBOBOX, hComboBox, CBN_SELCHANGE, SendMessage);

			// center dialog on screen
			RECT rc;
			GetWindowRect(hWnd, &rc);

			SetWindowPos(hWnd, HWND_TOP,
				(GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2,
				0, 0, SWP_NOSIZE);

#ifdef __GNUC__
			typedef void (WINAPI *STTWTYPE)(HWND, BOOL);

			static STTWTYPE SwitchToThisWindow = (STTWTYPE)GetProcAddress(
				GetModuleHandle("USER32.DLL"), "SwitchToThisWindow");
#endif
			SwitchToThisWindow(hWnd, TRUE);
			SetFocus(GetDlgItem(hWnd, IDC_COMBOBOX));

			return FALSE;
		}

		default:
		break;
	}

	return FALSE;
}


// AboutBox Thread Procedure
//
ULONG WINAPI AboutBoxThread(void *)
{
	if (!g_hAboutbox)
	{
		DialogBox(GetModuleHandle(NULL),
		          MAKEINTRESOURCE(IDD_ABOUTBOX), NULL, AboutBoxProcedure);

		g_hAboutbox = NULL;
	}
	else
	{
		SetForegroundWindow(g_hAboutbox);
	}

	return 0;
}


// Fill listview with bang command information
//
BOOL CALLBACK BangCallback(HMODULE hModule, LPCSTR pszName, LPARAM lParam)
{
    CallbackInfo* pCi = (CallbackInfo*)lParam;

    LVITEM itemInfo;
    itemInfo.mask = LVIF_TEXT;
    itemInfo.iItem = pCi->nItem;
    itemInfo.pszText = (char*)pszName;
    itemInfo.iSubItem = 0;

    ListView_InsertItem(pCi->hListView, &itemInfo);

    CHAR szModule[MAX_PATH] = { 0 };

    if (LSGetModuleFileName(hModule, szModule, COUNTOF(szModule)))
    {
        PathStripPath(szModule);
        ListView_SetItemText(pCi->hListView, pCi->nItem, 1, szModule);
    }

    pCi->nItem++;
    return TRUE;
}

void AboutBangs(HWND hListView)
{
	LVCOLUMN columnInfo;

    int width = GetClientWidth(hListView) - GetSystemMetrics(SM_CXVSCROLL);

    columnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    columnInfo.fmt = LVCFMT_LEFT;
    columnInfo.cx = width / 2;
    columnInfo.pszText = "Bang Command";
    columnInfo.iSubItem = 0;

    ListView_InsertColumn(hListView, 0, &columnInfo);

    columnInfo.cx = width - columnInfo.cx;
    columnInfo.pszText = "Module";
    columnInfo.iSubItem = 1;

    ListView_InsertColumn(hListView, 1, &columnInfo);

	CallbackInfo ci = { 0 };
	ci.hListView = hListView;

	EnumLSData(ELD_BANGS_V2, (FARPROC)BangCallback, (LPARAM)&ci);
}


// Fill listview with development team information
//
void AboutDevTeam(HWND hListView)
{
	LVCOLUMN columnInfo;
	int width = GetClientWidth(hListView) - GetSystemMetrics(SM_CXVSCROLL);

	columnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	columnInfo.fmt = LVCFMT_LEFT;
	columnInfo.cx = width / 3;
	columnInfo.pszText = "Nick";
	columnInfo.iSubItem = 0;

	ListView_InsertColumn(hListView, 0, &columnInfo);

	columnInfo.cx = (2 * width) / 3;
	columnInfo.pszText = "Real Name";
	columnInfo.iSubItem = 1;

	ListView_InsertColumn(hListView, 1, &columnInfo);

	for (unsigned int i = 0; i < theDevTeamCount; i++)
	{
		LVITEM itemInfo;

		itemInfo.mask = LVIF_TEXT;
		itemInfo.iItem = i;
		itemInfo.pszText = (char *) theDevTeam[i].nick;
		itemInfo.iSubItem = 0;

		ListView_InsertItem(hListView, &itemInfo);
		ListView_SetItemText(hListView, i, 1, (char *) theDevTeam[i].realName);
	}
}


// Show License Notice... Nothing to do
//
void AboutLicense(HWND /* hEdit */)
{
	//SetDlgItemText(hWnd, IDC_EDIT, lsLicense);
}


// Fill listview with module information
//
BOOL CALLBACK ModulesCallback(LPCSTR pszPath, DWORD /* dwFlags */, LPARAM lParam)
{
	CallbackInfo* pCi = (CallbackInfo*)lParam;

	LVITEM itemInfo;
	itemInfo.mask = LVIF_TEXT;
	itemInfo.iItem = pCi->nItem++;
	itemInfo.pszText = (char*)pszPath;
	itemInfo.iSubItem = 0;

	ListView_InsertItem(pCi->hListView, &itemInfo);

	return TRUE;
}


void AboutModules(HWND hListView)
{
	LVCOLUMN columnInfo;

	columnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	columnInfo.fmt = LVCFMT_LEFT;
	columnInfo.cx = GetClientWidth(hListView) - GetSystemMetrics(SM_CXVSCROLL);
	columnInfo.pszText = "Module";
	columnInfo.iSubItem = 0;

	ListView_InsertColumn(hListView, 0, &columnInfo);

	CallbackInfo ci = { 0 };
	ci.hListView = hListView;

	EnumLSData(ELD_MODULES, (FARPROC)ModulesCallback, (LPARAM)&ci);
}


// Fill listview with revision ID (LM_GETREVID) information
//
BOOL CALLBACK RevIDCallback(LPCSTR pszRevID, LPARAM lParam)
{
	CallbackInfo* pCi = (CallbackInfo*)lParam;

	LVITEM itemInfo;
	itemInfo.mask = LVIF_TEXT;
	itemInfo.iItem = pCi->nItem++;
	itemInfo.pszText = (char*)pszRevID;
	itemInfo.iSubItem = 0;

	ListView_InsertItem(pCi->hListView, &itemInfo);

	return TRUE;
}


void AboutRevIDs(HWND hListView)
{
	LVCOLUMN columnInfo;

	columnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	columnInfo.fmt = LVCFMT_LEFT;
	columnInfo.cx = GetClientWidth(hListView) - GetSystemMetrics(SM_CXVSCROLL);
	columnInfo.pszText = "Revision ID";
	columnInfo.iSubItem = 0;

	ListView_InsertColumn(hListView, 0, &columnInfo);

	CallbackInfo ci = { 0 };
	ci.hListView = hListView;
	ci.nItem = 0;

	EnumLSData(ELD_REVIDS, (FARPROC)RevIDCallback, (LPARAM)&ci);
}


HRESULT GetWinVerString(LPTSTR pszVersion, DWORD cchVersion)
{
    ASSERT(pszVersion != NULL);

    UINT uVersion = GetWindowsVersion();

    LPCSTR pszTemp = NULL;

    switch (uVersion)
    {
    case WINVER_WIN95:   pszTemp = _T("Windows 95");            break;
    case WINVER_WIN98:   pszTemp = _T("Windows 98");            break;
    case WINVER_WINME:   pszTemp = _T("Windows ME");            break;

    case WINVER_WINNT4:  pszTemp = _T("Windows NT 4.0");        break;
    case WINVER_WIN2000: pszTemp = _T("Windows 2000");          break;
    case WINVER_WINXP:   pszTemp = _T("Windows XP");            break;
    case WINVER_VISTA:   pszTemp = _T("Windows Vista");         break;
    case WINVER_WIN7:    pszTemp = _T("Windows 7");             break;

    case WINVER_WIN2003:
        if (GetSystemMetrics(SM_SERVERR2))
        {
            pszTemp = _T("Windows Server 2003 R2");
        }
        else
        {
            pszTemp = _T("Windows Server 2003");
        }
        break;

    case WINVER_WHS:     pszTemp = _T("Windows Home Server");   break;
    case WINVER_WIN2008: pszTemp = _T("Windows Server 2008");   break;

    default:             pszTemp = _T("<Unknown Version>");     break;
    }

    HRESULT hr = StringCchCopy(pszVersion, cchVersion, pszTemp);

    if (SUCCEEDED(hr))
    {
#ifndef WIN64
        if (IsOS(OS_WOW6432))
#endif
        {
            StringCchCat(pszVersion, cchVersion, _T(" (64-Bit)"));
        }
    }

    return hr;
}


// Fill listview with system information
//
void AboutSysInfo(HWND hListView)
{
	LVCOLUMN columnInfo;
	LVITEM itemInfo;
	int i = 0;
	char buffer[MAX_PATH];

	int width = GetClientWidth(hListView) - GetSystemMetrics(SM_CXVSCROLL);

	columnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	columnInfo.fmt = LVCFMT_LEFT;
	columnInfo.cx = width / 3 + width / 8;
	columnInfo.pszText = "Name";
	columnInfo.iSubItem = 0;

	ListView_InsertColumn(hListView, 0, &columnInfo);

	/* Using this odd size, keeps the columns aligned with
	 * the other list views, and also gives the text a little
	 * more room to keep from being truncated. */
	columnInfo.cx = (2 * width) / 3 - width / 8;
	columnInfo.pszText = "Value";
	columnInfo.iSubItem = 1;

	ListView_InsertColumn(hListView, 1, &columnInfo);

	// operating system and version
	itemInfo.mask = LVIF_TEXT;
	itemInfo.iItem = i;
	itemInfo.pszText = "Operating System";
	itemInfo.iSubItem = 0;

	ListView_InsertItem(hListView, &itemInfo);

	GetWinVerString(buffer, COUNTOF(buffer));
	ListView_SetItemText(hListView, i++, 1, buffer);

	// memory information
	MEMORYSTATUS ms;
	ms.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&ms);

	itemInfo.iItem = i;
	itemInfo.pszText = "Memory Load";

	ListView_InsertItem(hListView, &itemInfo);

	StringCchPrintf(buffer, MAX_PATH, "%d%%", ms.dwMemoryLoad);
	ListView_SetItemText(hListView, i++, 1, buffer);

	itemInfo.iItem = i;
	itemInfo.pszText = "Physical Memory Total";

	ListView_InsertItem(hListView, &itemInfo);

	FormatBytes(ms.dwTotalPhys, buffer, 64);
	ListView_SetItemText(hListView, i++, 1, buffer);

	itemInfo.iItem = i;
	itemInfo.pszText = "Physical Memory Available";

	ListView_InsertItem(hListView, &itemInfo);

	FormatBytes(ms.dwAvailPhys, buffer, 64);
	ListView_SetItemText(hListView, i++, 1, buffer);

	itemInfo.iItem = i;
	itemInfo.pszText = "Swap Space Total";

	ListView_InsertItem(hListView, &itemInfo);

	FormatBytes(ms.dwTotalPageFile, buffer, 64);
	ListView_SetItemText(hListView, i++, 1, buffer);

	itemInfo.iItem = i;
	itemInfo.pszText = "Swap Space Available";

	ListView_InsertItem(hListView, &itemInfo);

	FormatBytes(ms.dwAvailPageFile, buffer, 64);
	ListView_SetItemText(hListView, i++, 1, buffer);

}


// Simplified version of CreateFont
//
HFONT CreateSimpleFont(LPCSTR pszName, int nSizeInPoints, bool bBold)
{
	ASSERT(NULL != pszName); ASSERT(nSizeInPoints > 0);

	// convert size from points to pixels
	HDC hDC = GetDC(NULL);
	int sizeInPixels = -MulDiv(nSizeInPoints,
		GetDeviceCaps(hDC, LOGPIXELSY), 72);

	ReleaseDC(NULL, hDC);

	// fill in LOGFONT structure
	LOGFONT lf = { 0 };
	lf.lfHeight = sizeInPixels;
	lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
	StringCchCopy(lf.lfFaceName, LF_FACESIZE, pszName);

	// create it
	return CreateFontIndirect(&lf);
}


// Return the width of the window's client area
//
int GetClientWidth(HWND hWnd)
{
	RECT r;
	GetClientRect(hWnd, &r);

	return r.right - r.left;
}


// Trims whitespace from the beginning of a string in-place
//
void TrimLeft(char *pszToTrim)
{
	char * trimmed = pszToTrim;

	// skip past spaces
	while (*pszToTrim && isspace(*pszToTrim))
		pszToTrim++;

	if(pszToTrim != trimmed)
	{
		// copy the rest of the string over
		while (*pszToTrim)
			*trimmed++ = *pszToTrim++;
	
		// null-terminate it
		*trimmed = 0;
	}
}


// Formats a byte count into a string suitable for display to the user
//
LPCSTR units[] = { "bytes", "KB", "MB", "GB", "TB", "PB" };

void FormatBytes(size_t stBytes, LPSTR pszBuffer, size_t cchBuffer)
{
	double dValue = (double)stBytes;
	unsigned int uUnit = 0;

	while ((dValue >= 1024) && (uUnit < COUNTOF(units) - 1))
	{
		dValue /= 1024;
		++uUnit;
	}

	StringCchPrintf(pszBuffer, cchBuffer,
		"%d %s", (int)floor(dValue + 0.5), units[uUnit]);
}
