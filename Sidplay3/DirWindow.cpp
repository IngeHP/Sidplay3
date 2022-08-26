#pragma once

#include "DirWindow.h"
#include <Windowsx.h>
#include <CommCtrl.h>
#include <direct.h>
#include "IniFile.h"
#include "resource.h"


extern HWND hMain;
extern HINSTANCE hInst;
extern char szIni[MAX_PATH];

DirWind::DirWind()
{
	initialize();
}

DirWind::~DirWind()
{
	if (hwndDir)
	{
		SendMessage(hwndDir, WM_CLOSE, 0L, 0L);
	}
}

void DirWind::initialize()
{
	// Register the Directory Window class
	WNDCLASSEX wndclass = { 0 };
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = DirWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInst;
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = L"DirList";
	if (!RegisterClassEx(&wndclass))
	{
		feil = GetLastError();
	}

	hwndDir = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		L"DirList",            // window class name
		L"File selection",    // window caption
		WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX,   // window style
		// WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX,
		GetMyProfileInt("WINDOWS", "DirPosX", CW_USEDEFAULT, szIni),
		GetMyProfileInt("WINDOWS", "DirPosY", CW_USEDEFAULT, szIni),
		GetMyProfileInt("WINDOWS", "DirExtX", CW_USEDEFAULT, szIni),
		GetMyProfileInt("WINDOWS", "DirExtY", CW_USEDEFAULT, szIni),
		// No real parent makes it possible for the dir win to be ontop of hWnd
		//                      HWND_DESKTOP,        // parent window handle
		// hwnd as parent makes all windows be one program in the alt+tab list
		hMain,                // parent window handle
		NULL,                // window menu handle
		hInst,                // program instance handle
		NULL);                // creation parameters

	if (hwndDir == NULL)
	{
		feil = GetLastError();
	}
}

LRESULT CALLBACK DirWind::DirWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TEXTMETRIC tm;
	static HWND hwndList, hwndText, hwndOK, hwndOptions;
	static char szCurrentDir[512 + 1];
	static char szFilter[80];
	const TCHAR *szButtText = L"Filter";
	static HFONT myFont;
	static DWORD dwDirSettings;
	static unsigned char textInput[32];
	static int textPos = 0;
	static LRESULT foundItem;
	static int columnHeight = 0;

	switch (message)
	{
	case WM_CREATE:
	{
		RECT rc;
		HDC hdc;

		GetPrivateProfileStringA("SIDTUNES", "Directory", "C:\\", szCurrentDir, sizeof(szCurrentDir), szIni);
		GetPrivateProfileStringA("SIDTUNES", "Filter", "*.sid;*.mus", szFilter, sizeof(szFilter), szIni);
		dwDirSettings = GetPrivateProfileIntA("DIR LIST", "Settings", DIR_DIFIDR, szIni);
		_chdrive(tolower(szCurrentDir[0]) - 'a' + 1);
		_chdir(szCurrentDir);

		// Make sure we have the correct working dir
		_getcwd(szCurrentDir, 512);

		hdc = GetDC(hwnd);
		HFONT myFont = CreateFont(14, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Tahoma");
		SelectFont(hdc, myFont);
		GetTextMetrics(hdc, &tm);
		ReleaseDC(hwnd, hdc);
		GetClientRect(hwnd, &rc);

		hwndList = CreateWindow(L"ListBox", NULL, WS_CHILDWINDOW | WS_VISIBLE | LBS_STANDARD & ~LBS_SORT |
			LBS_MULTICOLUMN | WS_HSCROLL | LBS_WANTKEYBOARDINPUT,
			0, 0, 80, 80,
			hwnd, (HMENU)IDC_DIRUILIST,
			hInst, NULL);

		hwndText = CreateWindowA("static", szCurrentDir, WS_CHILDWINDOW | WS_VISIBLE | SS_LEFT,
			tm.tmAveCharWidth, tm.tmHeight / 2,
			rc.right, tm.tmHeight + tm.tmDescent,
			hwnd, (HMENU)IDC_DIRUIDIR,
			hInst, NULL);

		hwndOK = CreateWindowA("button", "OK", WS_CHILDWINDOW | WS_VISIBLE | BS_DEFPUSHBUTTON,
			100, 100, 60, 20,
			hwnd, (HMENU)IDOK,
			hInst, NULL);

		hwndOptions = CreateWindowA("button", "Filter", WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON,
			100, 100, 60, 20,
			hwnd, (HMENU)IDC_DIRUIOPTIONS,
			hInst, NULL);

		SendMessage(hwndList, WM_SETFONT, (WPARAM)myFont, 0);
		SendMessage(hwndText, WM_SETFONT, (WPARAM)myFont, 0);
		SendMessage(hwndOK, WM_SETFONT, (WPARAM)myFont, 0);
		SendMessage(hwndOptions, WM_SETFONT, (WPARAM)myFont, 0);

		DirFillListLong(hwndList, szCurrentDir, szFilter, dwDirSettings);

		textInput[0] = '\0';
		textPos = 0;
		break;
	}

	case WM_SIZE:
	{
		int cxText, cyText, cxButton, cyButton;
		HDC hdc = GetDC(hwndOK);

		SIZE sizeText;
		GetTextExtentPoint(hdc, szButtText, wcslen(szButtText), &sizeText);
		cxText = sizeText.cx;
		cyText = sizeText.cy;
		ReleaseDC(hwndOK, hdc);

		cxButton = cxText + 2 * tm.tmAveCharWidth;
		cyButton = 7 * cyText / 4;
		
		MoveWindow(hwndOK, LOWORD(lParam) - (cxButton + 2 * tm.tmAveCharWidth), tm.tmHeight * 2, cxButton, cyButton, TRUE);
		MoveWindow(hwndOptions, LOWORD(lParam) - (cxButton + 2 * tm.tmAveCharWidth), tm.tmHeight * 3 + cyButton, cxButton, cyButton, TRUE);

		int listWidth = LOWORD(lParam) - (cxButton + 5 * tm.tmAveCharWidth);
		int listHeight = HIWORD(lParam) - (int)(tm.tmHeight * 2.2);
		DWORD oldStyle = GetWindowLong(hwndList, GWL_STYLE);
		DWORD newStyle;
		bool bChange = false;

		if (listWidth > listHeight)
		{
			if ((oldStyle & LBS_MULTICOLUMN) == 0)
			{
				bChange = true;
				newStyle = (oldStyle & ~WS_VSCROLL) | LBS_MULTICOLUMN | WS_HSCROLL;
			}
		}
		else
		{
			if ((oldStyle & LBS_MULTICOLUMN) != 0)
			{
				bChange = true;
				newStyle = (oldStyle & ~LBS_MULTICOLUMN & ~WS_HSCROLL) | WS_VSCROLL;
			}
		}

		if (bChange)
		{
			DestroyWindow(hwndList);
			hwndList = CreateWindowA("ListBox", NULL, newStyle,
				tm.tmAveCharWidth / 2, tm.tmHeight * 2,
				listWidth, listHeight,
				hwnd, (HMENU)IDC_DIRUILIST,
				hInst, NULL);
			SendMessage(hwndList, WM_SETFONT, (WPARAM)myFont, 0);
			DirFillListLong(hwndList, szCurrentDir, szFilter, dwDirSettings);
		}
		else
		{
			MoveWindow(hwndList, tm.tmAveCharWidth, tm.tmHeight * 2,
				listWidth, listHeight, TRUE);
		}

		columnHeight = GetListBoxInfo(hwndList);

		break;
	}

	case WM_SETFOCUS:
		SetFocus(hwndList);
		break;

	case WM_SETFONT:
	{
		myFont = (HFONT)wParam;
		RECT rc;
		HDC hdc = GetDC(hwnd);
		SelectFont(hdc, myFont);
		GetTextMetrics(hdc, &tm);
		ReleaseDC(hwnd, hdc);
		GetClientRect(hwnd, &rc);
		DestroyWindow(hwndText);

		hwndText = CreateWindowA("static", szCurrentDir, WS_CHILDWINDOW | WS_VISIBLE | SS_LEFT,
			tm.tmAveCharWidth, tm.tmHeight / 2,
			rc.right, tm.tmHeight + tm.tmDescent,
			hwnd, (HMENU)IDC_DIRUIDIR,
			hInst, NULL);

		SendMessage(hwndList, WM_SETFONT, (WPARAM)myFont, 0);
		SendMessage(hwndText, WM_SETFONT, (WPARAM)myFont, 0);
		SendMessage(hwndOK, WM_SETFONT, (WPARAM)myFont, 0);
		SendMessage(hwndOptions, WM_SETFONT, (WPARAM)myFont, 0);

		DirFillListLong(hwndList, szCurrentDir, szFilter, dwDirSettings);
		break;
	}


	case OWM_REFRESH:
		dwDirSettings = (DWORD)wParam;
		GetWindowTextA(hwndText, szCurrentDir, sizeof(szCurrentDir));
		DirFillListLong(hwndList, szCurrentDir, szFilter, dwDirSettings);
		textInput[0] = '\0';
		textPos = 0;
		break;

	case WM_VKEYTOITEM:
	{
		switch (LOWORD(wParam))
		{
		case VK_RETURN:
			SendMessage(hwnd, WM_COMMAND, IDOK, 0L);
			return -1;
			break;

		case VK_ESCAPE:
			SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0L);
			break;

		case VK_BACK:
		{
			if (SendMessage(hwndList, LB_SELECTSTRING, -1, (LPARAM)"..") != LB_ERR)
			{
				SendMessage(hwnd, WM_COMMAND, MAKELONG(IDC_DIRUILIST, LBN_DBLCLK), 0L);
				textInput[0] = '\0';
				textPos = 0;
			}
			break;

		}

		case VK_DELETE:
		{
			DWORD dwIndex, dwLength;

			dwIndex = SendMessage(hwndList, LB_GETCURSEL, 0, 0L);
			if (dwIndex == (DWORD)LB_ERR)
				return FALSE;

			dwLength = SendMessage(hwndList, LB_GETTEXTLEN, dwIndex, 0L);
			char *szSel = new char[dwLength + 1];
			SendMessage(hwndList, LB_GETTEXT, dwIndex, (LPARAM)szSel);

			if (szSel[0] != '[' || szSel[dwLength - 1] != ']')
			{
				char *szConfirm = new char[dwLength + 1 + 20];
				sprintf_s(szConfirm, dwLength + 1 + 20, "Delete file \"%s\"", szSel);
				if (MessageBoxA(hMain, "Are you sure?", szConfirm, MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					if (szCurrentDir[strlen(szCurrentDir) - 1] != '\\')
						strcat_s(szCurrentDir, "\\");
					strcat_s(szCurrentDir, szSel);

					if (!MyDeleteFile(szCurrentDir))
						MessageBoxA(hMain, "Couldn't delete file.", szSel, MB_OK | MB_ICONEXCLAMATION);
					GetWindowTextA(hwndText, szCurrentDir, sizeof(szCurrentDir));
					DirFillListLong(hwndList, szCurrentDir, szFilter, dwDirSettings);
				}
				delete[]szConfirm;
			}
			delete[] szSel;
			SetFocus(hwndList);
			break;
		}
		default:
			return -1;
			break;
		}
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_DIRUILIST:
			if (HIWORD(wParam) == LBN_DBLCLK)
				SendMessage(hwnd, WM_COMMAND, IDOK, 0L);
			return FALSE;

		case IDC_DIRUIOPTIONS:
			if (DialogBox(hInst, MAKEINTRESOURCE(FILEFILTER), hwnd, (DLGPROC)SelectFilterDlgProc))
			{
				GetPrivateProfileStringA("SIDTUNES", "Filter", "*.sid;*.mus", szFilter, sizeof(szFilter), szIni);
				DirFillListLong(hwndList, szCurrentDir, szFilter,dwDirSettings);
			}
			break;

		case IDOK:
		{
			wchar_t *szSel;
			DWORD dwIndex, dwLength;

			// Re-set the current dir (it might have changed)
			GetWindowTextA(hwndText, szCurrentDir, 512);
			_chdrive(tolower(szCurrentDir[0]) - 'a' + 1);
			_chdir(szCurrentDir);

			// Get the doubleclicked item
			dwIndex = SendMessage(hwndList, LB_GETCURSEL, 0, 0L);
			if (dwIndex == (DWORD)LB_ERR)
				return FALSE;
			dwLength = SendMessage(hwndList, LB_GETTEXTLEN, dwIndex, 0L);
			szSel = new wchar_t[dwLength + 1];
			SendMessage(hwndList, LB_GETTEXT, dwIndex, (LPARAM)szSel);

			if (szSel[0] == '[' && szSel[dwLength - 1] == ']')
			{
				if (szSel[1] == '-' && szSel[dwLength - 2] == '-')
				{
					// A drive is selected - change drive
					_chdrive(tolower(szSel[2]) - 'a' + 1);
					_getcwd(szCurrentDir, 512);
					SetWindowTextA(hwndText, szCurrentDir);
					DirFillListLong(hwndList, szCurrentDir, szFilter,dwDirSettings);
				}
				else
				{
					// A directory is selected
					char *cp, *olddir = NULL;
					BOOL bRememberDir = FALSE;
					
					if (!wcscmp(szSel, L"[..]") && szCurrentDir[strlen(szCurrentDir) - 1] != '\\')
					{
						// Remember which directory we are in
						if ((cp = strrchr(szCurrentDir, '\\')) != NULL)
						{
							cp++;
							bRememberDir = TRUE;
							olddir = new char[strlen(cp) + 3];
							sprintf_s(olddir, strlen(cp) + 3, "[%s]", cp);
						}
					}

					// Change directory
					szSel[dwLength - 1] = '\0';
					_wchdir(szSel + 1);
					_getcwd(szCurrentDir, 512);
					SetWindowTextA(hwndText, szCurrentDir);
					DirFillListLong(hwndList, szCurrentDir, szFilter,dwDirSettings);

					if (bRememberDir)
					{
						DWORD idx = SendMessageA(hwndList, LB_FINDSTRING, -1, (LPARAM)olddir);
						SendMessage(hwndList, LB_SETSEL, (WPARAM)TRUE, idx);
						delete[] olddir;
					}
				}
			}
			else
			{
				WritePrivateProfileStringA("SIDTUNES", "Directory", szCurrentDir, szIni);
				if (szCurrentDir[strlen(szCurrentDir) - 1] != '\\')
					strcat_s(szCurrentDir, "\\");
				char *szTmp2 = new char[(wcslen(szSel) + 1) * 2];
				wtoc(szSel, szTmp2);
				strcat_s(szCurrentDir, szTmp2);
				SendMessage(hMain, OWM_PLAYFILE, 0, (LPARAM)szCurrentDir);

				GetWindowTextA(hwndText, szCurrentDir, sizeof(szCurrentDir));
				delete[] szTmp2;
			}
			delete[] szSel;
			return TRUE;
		}

		case IDCANCEL:
			SendMessage(hMain, WM_COMMAND, IDM_VIEWDIRUI, 0L);
			SetFocus(hMain);
			break;
		}
		break;

	case WM_CLOSE:
		SendMessage(hMain, WM_COMMAND, IDM_VIEWDIRUI, 0L);
		SetFocus(hMain);
		break;

	case WM_DESTROY:
	{
		WINDOWPLACEMENT wpl;
		wpl.length = sizeof(WINDOWPLACEMENT);
		if (GetWindowPlacement(hwnd, &wpl))
		{
			WriteMyProfileInt("WINDOWS", "DirPosX", wpl.rcNormalPosition.left, szIni);
			WriteMyProfileInt("WINDOWS", "DirPosY", wpl.rcNormalPosition.top, szIni);
			WriteMyProfileInt("WINDOWS", "DirExtX", wpl.rcNormalPosition.right - wpl.rcNormalPosition.left, szIni);
			WriteMyProfileInt("WINDOWS", "DirExtY", wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top, szIni);
		}
		break;
	}

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return FALSE;
}

INT_PTR CALLBACK DirWind::SelectFilterDlgProc(HWND hDlg, UINT message, UINT wParam, LONG)
{
	char szBuf[100];
	static HFONT myFont;

	myFont = GetStockFont(DEFAULT_GUI_FONT);

	switch (message)
	{
	case WM_INITDIALOG:
		GetPrivateProfileStringA("SIDTUNES", "Filter", "*.sid;*.mus", szBuf, sizeof(szBuf), szIni);
		if (myFont)
		{
			SendDlgItemMessage(hDlg, IDOK, WM_SETFONT, (WPARAM)myFont, (LPARAM)TRUE);
			SendDlgItemMessage(hDlg, IDCANCEL, WM_SETFONT, (WPARAM)myFont, (LPARAM)TRUE);
			SendDlgItemMessage(hDlg, IDC_EDITFILTER, WM_SETFONT, (WPARAM)myFont, (LPARAM)TRUE);
			SendDlgItemMessage(hDlg, IDC_FILTERTEXT1, WM_SETFONT, (WPARAM)myFont, (LPARAM)TRUE);
		}
		SetDlgItemTextA(hDlg, IDC_EDITFILTER, szBuf);
		SetFocus(GetDlgItem(hDlg, IDC_EDITFILTER));
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemTextA(hDlg, IDC_EDITFILTER, szBuf, sizeof(szBuf) - 1);
			WritePrivateProfileStringA("SIDTUNES", "Filter", szBuf, szIni);
			EndDialog(hDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;

		}
		break;
	}
	return FALSE;
}