#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"

#define OWM_SETPERCENTAGE		(WM_USER+2)

class Progress
{
public:
	Progress(LPWSTR title = NULL);
	~Progress();
	HWND Hwnd() { return hDialog; }
	void SetPercentage(int percentage);
	int GetStatus() { return status; }
	void PumpMessages();
	static INT_PTR ProgressDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	enum { PROGRESS_OK, PROGRESS_TRUNCATE, PROGRESS_DELETE };

private:
	HWND hDialog;
	HWND hParent;
	int status;

};

