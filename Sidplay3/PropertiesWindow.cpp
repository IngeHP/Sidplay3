#pragma once

#include "PropertiesWindow.h"
#include <Windowsx.h>
#include <direct.h>
#include "IniFile.h"
#include "resource.h"

extern HWND hMain;
extern HINSTANCE hInst;
extern char szIni[MAX_PATH];

PropertiesWindow::PropertiesWindow()
{
	initialize();
}

PropertiesWindow::~PropertiesWindow()
{
	if (hwndProperties)
	{

		SendMessage(hwndProperties, WM_CLOSE, 0L, 0L);
	}
}

void PropertiesWindow::initialize()
{
	hwndProperties = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SIDPROPERTIES), hMain, (DLGPROC)PropertiesWindowProc);

	if (hwndProperties == NULL)
	{
		feil = GetLastError();
	}
	else
	{
		ShowWindow(hwndProperties, SW_HIDE);
	}
}

void PropertiesWindow::update(const wchar_t *szInfo)
{
	wcscpy_s(szTextbuffer, 16382, szInfo);

	SendDlgItemMessage(hwndProperties, IDC_INFO, WM_SETTEXT, 0, (LPARAM)szTextbuffer);
}

void PropertiesWindow::Show(bool show)
{
	if (show)
		ShowWindow(hwndProperties, SW_RESTORE);
	else
		ShowWindow(hwndProperties, SW_HIDE);
}

INT_PTR CALLBACK PropertiesWindow::PropertiesWindowProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	COLORREF crBk = RGB(193, 193, 255);
	HANDLE font;
	
	switch (message)
	{

	case WM_INITDIALOG:
	{
		int x = GetMyProfileInt("WINDOWS", "PropertiesX", 0, szIni);
		int y = GetMyProfileInt("WINDOWS", "PropertiesY", 0, szIni);
		
		WINDOWPLACEMENT wpl;
		wpl.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hwnd, &wpl);
		int sx = wpl.rcNormalPosition.right - wpl.rcNormalPosition.left;
		int sy = wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top;

		MoveWindow(hwnd, x, y, sx, sy, true);

		return (INT_PTR)TRUE;
		break;
	}
	case WM_CTLCOLORSTATIC:
	{
		HDC hdc = (HDC)wParam;
		HWND hClr = (HWND)lParam;
		if (GetDlgCtrlID(hClr) == IDC_INFO)
		{
			//SetBkColor(hdc, crBk);
			//SetDCBrushColor(hdc, crBk);
			return (INT_PTR)GetStockObject(DC_BRUSH);
		}
		else
		{
			return DefWindowProc(hClr, message, wParam, lParam);
		}
		break;
	}
	case WM_SETFONT:
		SendDlgItemMessage(hwnd, IDC_INFO, WM_SETFONT, wParam, (LPARAM)true);
		break;

	case WM_CLOSE:
		CheckMenuItem(GetMenu(hMain), IDM_VIEW_PROPERTIES, MF_BYCOMMAND | MF_UNCHECKED);
		EndDialog(hwnd, TRUE);
		break;

	case WM_DESTROY:
	{
		WINDOWPLACEMENT wpl;
		wpl.length = sizeof(WINDOWPLACEMENT);
		if (GetWindowPlacement(hwnd, &wpl))
		{
			WriteMyProfileInt("WINDOWS", "PropertiesX", wpl.rcNormalPosition.left, szIni);
			WriteMyProfileInt("WINDOWS", "PropertiesY", wpl.rcNormalPosition.top, szIni);
		}
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			break;
		}

		break;
	}
	default:
		break;
	}
	return (INT_PTR) FALSE;
}
