#include "stilview.h"
#include <stdlib.h>
#include "resource.h"
#include "inifile.h"
#include "misc.h"
#include <Shlwapi.h>

#define OWM_SETTEXT 			(WM_USER+0)
#define IDC_STILLIST			1	// Only child window in STIL View
#define MAX_STIL_AUTO_HEIGHT	200	// Maximum height the STIL View auto-sizes to
#define SCROLLBAR_WIDTH			30	// Approximation of scrollbar width

extern HINSTANCE hInst;
extern HWND hMain;
extern HFONT fixFont;
extern char szIni[MAX_PATH];
extern STILView *mySTILView;
COLORREF background = RGB(255, 255, 204);

STILView::STILView()
{
	char szTemp[MAX_PATH];
	entry = "";
	STILWindow = NULL;
	HVSCdir[0] = '\0';

	x = GetMyProfileInt( "Windows", "STILPosX", 0, szIni );
	y = GetMyProfileInt( "Windows", "STILPosY", 0, szIni );
	width  = GetMyProfileInt( "Windows", "STILExtX", 0, szIni );
	height = GetMyProfileInt( "Windows", "STILExtY", 0, szIni );
	bAutoSize = GetMyProfileBool( "HVSC", "STILAutoSize", true, szIni );
	// TODO: Borde se till att (x,y) är en punkt pEskärmen!
	setVisibility (
		GetMyProfileBool( "HVSC", "ShowSTILEntry", true, szIni ),
		GetMyProfileBool( "HVSC", "ShowBug", true, szIni ),
		GetMyProfileBool( "HVSC", "ShowSTILglobal", true, szIni )
		);
	bAllSubtunes = GetMyProfileBool( "HVSC", "STILAllSubtunes", false, szIni );
	GetPrivateProfileStringA( "HVSC", "Directory", "", szTemp, sizeof(szTemp), szIni );
	setHVSCdir( szTemp );
	setMaxAutoHeight( GetMyProfileInt( "Windows", "STILMaxAutoHeight",
											MAX_STIL_AUTO_HEIGHT, szIni ));
}


STILView::~STILView()
{
	closeSTILWindow();
	WriteMyProfileInt( "Windows", "STILPosX", x, szIni );
	WriteMyProfileInt( "Windows", "STILPosY", y, szIni );
	WriteMyProfileInt( "Windows", "STILExtX", width, szIni );
	WriteMyProfileInt( "Windows", "STILExtY", height, szIni );
	WriteMyProfileInt( "Windows", "STILMaxAutoHeight", maxAutoHeight, szIni );
	WritePrivateProfileStringA( "HVSC", "Directory", HVSCdir, szIni );
	WriteMyProfileBool( "HVSC", "ShowSTILEntry", bShowEntry, szIni );
	WriteMyProfileBool( "HVSC", "ShowBug", bShowBug, szIni );
	WriteMyProfileBool( "HVSC", "ShowSTILglobal", bShowGlobal, szIni );
	WriteMyProfileBool( "HVSC", "STILAllSubtunes", bAllSubtunes, szIni );
	WriteMyProfileBool( "HVSC", "STILAutoSize", bAutoSize, szIni );
}


void STILView::setVisibility( bool showEntry, bool showBug, bool showGlobal )
{
	bShowEntry = showEntry;
	bShowBug = showBug;
	bShowGlobal = showGlobal;
	showSTIL = (showEntry || showBug || showGlobal) && *HVSCdir;
	if( showSTIL && !STILWindow)
		STILWindow = createSTILWindow( hMain );
	else if( !showSTIL && STILWindow )
		closeSTILWindow();
}


void STILView::setHVSCdir( const char *dir )
{
	// Avoid delay if nothing changes.
	if( dir == NULL || !_stricmp(HVSCdir, dir) )
		return;

	strncpy_s(HVSCdir, MAX_PATH, dir, strlen(dir));
	HVSCdir[MAX_PATH-1] = '\0';

	// Empty path -> disable STIL
	if( !*HVSCdir )
	{
		// There is no way of invalidating the currently loaded STIL
		showSTIL = false;
	}
	else if( !theSTIL.setBaseDir(HVSCdir) )
	{
		MessageBoxA( hMain, theSTIL.getErrorStr().c_str(), "STIL View error",
					MB_OK | MB_ICONWARNING );
		showSTIL = false;
		HVSCdir[0] = '\0';
	}
	setVisibility( bShowEntry, bShowBug, bShowGlobal );
}


const char * STILView::getHVSCdir()
{
	return (const char *)HVSCdir;
}

void STILView::setFont(HFONT font)
{
	myFont = font;
	SendMessage(STILWindow, WM_SETFONT, (WPARAM)font, (LPARAM)true);
}

void STILView::textToListbox( HWND hList, const char *text )
{
	wchar_t *mytext;
	wchar_t *buffer;

	mytext = (wchar_t*) malloc(STIL_MAX_ENTRY_SIZE * 2);
	if (mytext != NULL)
	{
		ctow(text, mytext);

		SendMessage( hList, LB_RESETCONTENT, 0, 0 );

		if (mytext != NULL)
		{
			wchar_t* cp = wcstok_s(mytext, L"\x0a\x0d", &buffer);
			while (cp)
			{
				SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)cp);
				cp = wcstok_s(NULL, L"\x0a\x0d", &buffer);
			}
		}
		free(mytext);
	}

}


void STILView::removeTrailingNewline()
{
	if( *comment )
	{
		// At least one character in string
		int i = 0;
		while( comment[i] )
			i++;
		i--;
		while(i > 0 && (comment[i] == '\x0a' || comment[i] == '\x0d') )
			comment[i--] = '\0';
	}
}


void STILView::showEntry( char *fullPath, int tuneNo )
{
	if( !showSTIL )
		return;

	comment[0] = '\0';
	if( bAllSubtunes )
		tuneNo = 0;
	if( bShowGlobal )
	{
		// Retrieve the section-global comment
		entry = theSTIL.getAbsGlobalComment( fullPath );
		if (entry != "")
		{
			//strcat(comment, "[G]");
			strcat_s(comment, entry.c_str());
		}
	}
	if( bShowEntry )
	{
		// Retrieve the tune-global comment
		// If info for all sub-tunes is shown, this is not needed
		if( !bAllSubtunes )
		{
			entry = theSTIL.getAbsEntry( fullPath, 0, STIL::comment );
			if (entry != "")
			{
				//strcat(comment, "[C]");
				strcat_s(comment, entry.c_str());
			}
		}
		// Retrieve all of the STIL entry for the tune
		entry = theSTIL.getAbsEntry( fullPath, tuneNo, STIL::all );
		if (entry != "")
		{
			//strcat(comment, "[S]");
			strcat_s(comment, entry.c_str());
		}
	}
	if( bShowBug )
	{
		entry = theSTIL.getAbsBug( fullPath, tuneNo );
		if (entry != "")
		{
			strcat_s(comment, entry.c_str());
		}
	}
	
	if( *comment )
	{
		removeTrailingNewline();
		if( STILWindow )
		{
			SendMessage( STILWindow, OWM_SETTEXT, (WPARAM)comment, NULL );
			ShowWindow( STILWindow, SW_SHOWNA );
		}
	}
	else if( STILWindow )
		ShowWindow( STILWindow, SW_HIDE );
}


void STILView::closeSTILWindow()
{
	if( STILWindow )
	{
		RECT rect;
		GetWindowRect( STILWindow, &rect );
		x = rect.left;
		y = rect.top;
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		DestroyWindow( STILWindow );
		STILWindow = NULL;
	}
}


LRESULT CALLBACK STILView::STILWindowProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	static char *text;
	static HWND hInfo;
	TCHAR achbuffer[100];
	HDC hdc;
	PDRAWITEMSTRUCT pdis;
	TEXTMETRIC tm;
	int ypos;
	RECT rItem;
	static int ymax;
	static HFONT thisFont;

	switch( message )
	{
	case WM_CREATE:
	{
		hInfo = CreateWindow( L"listbox",           	// window class name
                       NULL,						// window caption
					   LBS_NOINTEGRALHEIGHT | LBS_NOSEL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS |
					   WS_CHILD | WS_VISIBLE | WS_VSCROLL & ~WS_SIZEBOX,
					   0,0,0,0,
                       hwnd,						// parent window handle
                       (HMENU) IDC_STILLIST,		// child window identifier
                       hInst, 						// program instance handle
                       NULL );                 		// creation parameters

		// Setup font
		if (fixFont != NULL)
		{
			thisFont = fixFont;
			SendMessage(hInfo, WM_SETFONT, (WPARAM)thisFont, (LPARAM)true);
		}

		// Do nothing, perhaps get a pointer to STILView object?
		return FALSE;
	}

	case WM_SIZE:
		MoveWindow( hInfo, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );
		return 0;

	case WM_SETFOCUS:
		SetFocus( hInfo );
		return 0;

	case WM_SETFONT:
		thisFont = (HFONT)wParam;
		SendMessage(hInfo, WM_SETFONT, wParam, lParam);
		break;

	case WM_DRAWITEM:
		pdis = (PDRAWITEMSTRUCT)lParam;

		if (pdis->itemID == -1)
			break;

		switch (pdis->itemAction)
		{
		case ODA_DRAWENTIRE:
		case ODA_SELECT:

			rItem = pdis->rcItem;
			//FillRect(pdis->hDC, &rItem, CreateSolidBrush(background));
			SendMessage(pdis->hwndItem, LB_GETTEXT, pdis->itemID, (LPARAM)achbuffer);

			GetTextMetrics(pdis->hDC, &tm);
			ypos = (pdis->rcItem.bottom + pdis->rcItem.top - tm.tmHeight) / 2;
			size_t cch;

			cch = wcslen(achbuffer);
			//SetBkColor(pdis->hDC,background);
			TextOut(pdis->hDC, 6, ypos, achbuffer, cch);

			return 0;

		default:
			break;
		}
		break;

	case OWM_SETTEXT:
	{
		text = (char *)wParam;
		HANDLE monoFont = 0;

		if( mySTILView->bAutoSize )
		{
			RECT rectAll, rectClient;

			int width, height;
			HDC cHdc;
			wchar_t temptxt[STIL_MAX_ENTRY_SIZE * 2];
			ctow(text, temptxt);

			// For "drawing" in:
			RECT rect = {0, 0, 0, 0};

			cHdc = CreateCompatibleDC(NULL);

			SelectObject(cHdc, thisFont);
			DrawText( cHdc, temptxt, -1, &rect, DT_LEFT|DT_CALCRECT );

			DeleteDC(cHdc);
			width = rect.right + 50;
			
			if( rect.bottom < mySTILView->maxAutoHeight )
				height = rect.bottom;
			else
			{
				height = mySTILView->maxAutoHeight;
				width += SCROLLBAR_WIDTH;	// Adjust for the scrollbar
			}
			ymax = height;
			GetWindowRect( hwnd, &rectAll );
			GetClientRect( hwnd, &rectClient );
			MoveWindow( hwnd, rectAll.left, rectAll.top,
					(rectAll.right -rectAll.left) - (rectClient.right -rectClient.left) + width,
					(rectAll.bottom-rectAll.top)  - (rectClient.bottom-rectClient.top)  + height, TRUE );
		}

		// Put the new text into the window
		mySTILView->textToListbox( hInfo, text );

		if (monoFont != 0)
		{
			RemoveFontMemResourceEx(monoFont);
		}

		return 0;
	}

	case WM_COMMAND:
		if( HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == IDC_STILLIST )
			SendMessage( hwnd, WM_CLOSE, 0, 0 );
		return 0;

	case WM_CLOSE:
		ShowWindow( hwnd, SW_HIDE );
		return 0;	// Avoid destruction, just hide

	case WM_DESTROY:
	{
		RECT rectAll;
		GetWindowRect( hwnd, &rectAll );
		mySTILView->x = rectAll.left;
		mySTILView->y = rectAll.top;
		mySTILView->STILWindow = NULL;

		break;
	}

	default:
		return DefWindowProc( hwnd, message, wParam, lParam );
	}	// switch(message)

	return FALSE;
}


HWND STILView::createSTILWindow( HWND hParent )
{
	const TCHAR *szClassName = L"STILView";

	WNDCLASSEX wndclass = { 0 };
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = STILWindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInst;
	wndclass.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(IDI_SIDPLAY3WINDOWS) );
	wndclass.hCursor = LoadCursor( NULL, IDC_ARROW );
	wndclass.hbrBackground = CreateSolidBrush(background);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szClassName;
	wndclass.hIconSm = LoadIcon( hInst, MAKEINTRESOURCE(IDI_SMALL) );

	if (!RegisterClassEx(&wndclass))
	{
		DWORD feil = GetLastError();
	}

	HWND window = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
		szClassName,            	// window class name
		L"STIL View",				// window caption
		WS_OVERLAPPED | WS_SYSMENU | WS_SIZEBOX,
		x, y, CW_USEDEFAULT, CW_USEDEFAULT,
		hParent,					// parent window handle
		NULL,                  		// window menu handle
		hInst, 						// program instance handle
		(LPVOID)fixFont );                 	// creation parameters

	if (window == NULL)
	{
		DWORD err = GetLastError();
	}

	return window;
}

