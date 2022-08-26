#ifndef PROPERTIESWINDOW_H
#define PROPERTIESWINDOW_H

#include <Windows.h>
#include <string>
#include "misc.h"

using namespace std;

class PropertiesWindow
{
public:
	// Constructors
	PropertiesWindow();

	// Destructor
	~PropertiesWindow();

	// Toggle window
	void Show(bool show);

	// Update window contents
	void update(const wchar_t *szInfo);
	HWND hwndProperties;

private:
	void initialize();
	static INT_PTR CALLBACK PropertiesWindowProc(HWND hwnd, UINT message, UINT wParam, LONG lParam);


	// Private variables
	DWORD feil;
	wchar_t szTextbuffer[16384];
	HANDLE font;
};

#endif