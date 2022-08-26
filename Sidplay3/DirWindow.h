#ifndef DIRWINDOW_H
#define DIRWINDOW_H

#define OWM_PLAYFILE			(WM_USER+0)
#define OWM_REFRESH				(WM_USER+10)

#include <Windows.h>
#include <string>
#include "misc.h"

using namespace std;

class DirWind
{
public:
	// Constructors
	DirWind();

	// Destructor
	~DirWind();

	// Methods
	void SetDir(const char* curdir);

	// Public variables
	HWND hwndDir;

private:
	void initialize();
	static LRESULT CALLBACK DirWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SelectFilterDlgProc(HWND hDlg, UINT message, UINT wParam, LONG);
	//INT_PTR CALLBACK ConversionDlgProc(HWND hDlg, UINT message, UINT wParam, LONG lParam);

	// Private variables
	DWORD feil;
	HFONT thisFont;
	enum { CONV_TO_PSID = 0, CONV_TO_SID, CONV_TO_DAT, CONV_TO_SID_DAT };

};

#endif