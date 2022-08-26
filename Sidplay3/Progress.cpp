#include "Progress.h"


extern HWND hMain;
extern HINSTANCE hInst;

Progress::Progress(LPWSTR title)
{
	status = 0;
	hParent = hMain;
	hDialog = CreateDialogParam(hInst, MAKEINTRESOURCE(WAVPROGRESSION), hParent, (DLGPROC)ProgressDlgProc, (LPARAM)this);

	if (title != NULL)
		SetWindowText(hDialog, title);
}

Progress::~Progress()
{
	if (hDialog)
	{
		DestroyWindow(hDialog);
		hDialog = NULL;
	}

}

void Progress::SetPercentage(int percentage)
{
	if (hDialog)
		SendMessage(hDialog, OWM_SETPERCENTAGE, 0, (LPARAM)percentage);
}

void Progress::PumpMessages()
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (!IsDialogMessage(hDialog, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

INT_PTR Progress::ProgressDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char szPercentage[10];
	static Progress* prog;

	switch (message)
	{
	case WM_INITDIALOG:
		prog = (Progress*)lParam;
		return TRUE;

	case OWM_SETPERCENTAGE:
	{
		int percentage = (int)lParam;
		RECT rcDialog, rect;
		POINT p;
		HBRUSH hBrush;
		HDC hdc;
		int hspace, vspace;
		GetClientRect(hDlg, &rcDialog);
		p.y = 0;
		ClientToScreen(GetDlgItem(hDlg, IDCANCEL), &p);
		rcDialog.bottom = p.y;
		p.y = 0;
		ClientToScreen(hDlg, &p);
		rcDialog.bottom -= p.y;
		hspace = rcDialog.right / 8;
		vspace = rcDialog.bottom / 4;
		rect = rcDialog;
		rect.left += hspace;
		rect.right -= hspace;
		rect.top += vspace;
		rect.bottom -= vspace;
		hdc = GetDC(hDlg);
		hBrush = SelectBrush(hdc, CreateSolidBrush(RGB(255, 234, 0)));
		Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		rect.right -= (rcDialog.right - 2 * hspace) * (100 - percentage) / 100;
		DeleteBrush(SelectBrush(hdc, CreateSolidBrush(RGB(0, 255, 0))));
		Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		DeleteBrush(SelectBrush(hdc, hBrush));

		SetBkMode(hdc, TRANSPARENT);
		sprintf_s(szPercentage, "%d%%", percentage);
		DrawTextA(hdc, szPercentage, -1, &rcDialog, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		ReleaseDC(hDlg, hdc);
		break;
	}

	case WM_COMMAND:
		switch (LOWORD(lParam))
		{
		case IDCANCEL:
			prog->status = PROGRESS_DELETE;
		}
		break;
	} // Switch
	return FALSE;
}