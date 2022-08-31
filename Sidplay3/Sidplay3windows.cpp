// Sidplay3windows.cpp : Defines the entry point for the application.
//

#include "Sidplay3windows.h"
#include "stdafx.h"
#include <process.h>
#include <vector>
#include <shellapi.h>
#include <ShlObj.h>
#include <direct.h>
#include <math.h>
#include <Commctrl.h>

// Other includes
#include "ofile.h"
#include "misc.h"
#include "stilview.h"
#include "IniFile.h"
#include "DirWindow.h"
#include "PropertiesWindow.h"
#include "sidid.h"
#include "Songlength.h"
#include "AudioSettings.h"
#include "Progress.h"
#include "PlayList.h"
#include "CpuDebug.h"

// SidPlayFP includes
#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidConfig.h>

// Audio output
#include "Audio/AudioDrv.h"
#include "Audio/wav/WavFile.h"

// builders
//#include <builders/resid-builder/resid.h>
#include <builders/residfp-builder/residfp.h>

// Some boring defines
constexpr auto MAX_LOADSTRING = 100;
#define PROGRAMNAME "Sidplay3"
//const char *szHVSCDir = "d:\\Inge\\C64\\C64Music\\C64Music\0";

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szAppPath[MAX_PATH];
TCHAR inFile[MAX_PATH + 1];
const DWORD dwMaxWavVol = 0xffff;

char				infilename[MAX_PATH+ 1] = "";
char				szIni[MAX_PATH];
char				hvscDir[MAX_PATH];
char				szSidId[MAX_PATH];
unsigned int		selectedSong = 0;
int					secondsFadeout;
unsigned long		uLastPlayed, uSecondsToPlay;
unsigned long		lUsageSession, lUsageEver;
bool				isMusPlayer = false;
SidTune				*m_tune;
SidConfig			m_engCfg;
enum player_state_t	m_state;
thread_state_t		m_threadstate;
sidplayfp			m_engine;
HANDLE				hPlayThread = NULL;
DWORD				dwThreadId;
STILView			*mySTILView;
DWORD				dwDirSettings;
audio_section		as;
sidplay2_section	ss;
emulation_section	es;
DirWind				*m_DirWind;
PropertiesWindow	*myProperties;
SIDID				*m_SidId;
int					m_SidIdFound = 0;
Songlength			*mySLDB;
uint_least32_t		tuneLength;
int					rsecs = 0;
bool				eot;
HFONT				mainFont = NULL;
HFONT				fixFont = NULL;
ReSIDfpBuilder		*rs;
AudioSettings		audiosettings;
PlayList*			myPlayList;
HHOOK				keyboardHook;
CpuDebug			cpuDebug;

// ROM resourcers
const uint8_t KERNAL[8192] = {
#include "kernal.bin"
};

const uint8_t BASIC[8192] = {
#include "basic.bin"
};

const uint8_t CHARACTERS[4096] = {
#include "char.bin"
};

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Player(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	FilterDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	MixerDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	HvscDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	AudioOutDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	EmuDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	SettingsDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DirsettingsDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CpuDebugDlg(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	KeyboardHook(INT, WPARAM, LPARAM);

// Helper functions
bool				LoadSid(FileExtensions &extension);
bool				InitTune(unsigned int song);

void				InfoBox(HWND hwnd, const char *title, const char* szFormat...);
void				RefreshProperties();
bool				CreateSidEmu(SIDEMUS emu);
bool				CreateOutput(OUTPUTS driver);
void				Start();
void				Stop();
bool				Play();
unsigned int __stdcall		PlayThread(LPVOID lpParam);
void				CleanUp();
void				SetSpeed(int nTimesNormal);
void				ChangeMainDialog();
void				CreateMusInfo(HDC dc);
//void				PrintC64Char(HDC dc, uint8_t ch, int x, int y);
bool				updateLiveFilter(m_filter_t testFilter);
void				updateMixer(HWND hDLg, uint16_t solo);
void				updateVoices();
//DWORD WINAPI		DisplayTimer(LPVOID lpParam);
void				MakeWavFile(HWND hwnd, AudioSettings aus, SaveWavFile* saveFile);

// Global window variables
HWND hMain, hPlayer, hMusplayer, hDirWnd, hProperties, hFilterDlg, hMixerDlg, hAudioOutDlg, hAbout;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	MSG msg;
	HACCEL hAccelTable;
	bool bOpenNewInstance = false;

	// Parse command line
	char *arg = new char[(wcslen(lpCmdLine) + 1) * 2];
	cmdlinetok(lpCmdLine, arg);

	while (arg[0] != '\0')
	{
		if (arg[0] == '-')
		{
			switch (arg[1])
			{
			case 'h':
				// Print helptext
				break;
			case 'i':
				bOpenNewInstance = true;
				break;
			case 'o':
				selectedSong = atoi(arg + 2);
				break;
			default:
				// Print helptext
				break;
			}
		}
		else
		{
			if (*infilename == '\0')
				strncpy_s(infilename, arg, MAX_PATH);
			//else
				// Print helptext
		}
		arg[0] = '\0';
	}
	delete[] arg;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SIDPLAY3WINDOWS, szWindowClass, MAX_LOADSTRING);
	//strcpy_s(szIni, MAX_PATH, GetIniFile());

	if (!bOpenNewInstance)
	{
		// Switch to active instance of SIDPLAY: (Code from MSJ feb 1994 p. 18)
		HWND hWnd, hWndLast;

		// Find top-level window of previous instance
		if ((hWnd = FindWindow(szWindowClass, NULL)) != NULL)
		{
			// Find any open dialogs or msg boxes
			hWndLast = GetLastActivePopup(hWnd);
			if (hWndLast == NULL)
				hWndLast = hWnd;

			// Make it visible
			BringWindowToTop(hWndLast);

			// Restore previous instance if necessary
			if (IsIconic(hWndLast))
				ShowWindow(hWndLast, SW_RESTORE);

			// Play the tune given in the command line, if any
			if (*infilename)
			{
				COPYDATASTRUCT MyCDS;

				MyCDS.dwData = 448;
				MyCDS.cbData = strlen(infilename);
				MyCDS.lpData = infilename;

				// Notify the already running Sidplay3w so it can play the selected file
				SendMessage(hWnd, WM_COPYDATA, (WPARAM)(HWND)NULL, (LPARAM)(LPVOID)&MyCDS);
			}
			return false;
		}
	}
	MyRegisterClass(hInstance);

	//wcscpy_s(szAppPath,MAX_PATH, getAppPath());
	getAppPath(szAppPath);
	wtoc(szAppPath, szIni);
	//strcpy_s(szIni, MAX_PATH, wtoc(szAppPath));
	strcat_s(szIni, MAX_PATH, "Sidplay3.ini");

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIDPLAY3WINDOWS));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SIDPLAY3WINDOWS));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SIDPLAY3WINDOWS);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX,
		//CW_DEFAULT,
		//CW_DEFAULT,
		GetMyProfileInt("WINDOWS", "MainPosX", CW_USEDEFAULT, szIni),
		GetMyProfileInt("WINDOWS", "MainPosY", CW_USEDEFAULT, szIni),
		CW_DEFAULT,
		CW_DEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	typedef struct _MYITEM
	{
		HFONT hfont;
		LPSTR psz;
	} MYITEM;             // structure for item font and string  

	MYITEM *pmyitem;      // pointer to item's font and string        
	static MYITEM myitem[16];   // array of MYITEMS               
	static HMENU hmenu;             // handle to main menu     

	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HRSRC res;
	RECT rectAll, rectClient, rectChild;
	int x = 0, y = 0, cx, cy;
	static OpenSidFile ofile(hWnd);
	static FileExtensions extensions;
	static HWND hPlayListView;

	switch (message)
	{
	case WM_CREATE:
	{
		// Setup main window
		hMain = hWnd;

		// Setup global keyboard hook
		keyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardHook, hInst, GetCurrentThreadId());

		// Init custom fonts
		int noRegFont = AddFontResourceEx(L"Roboto-Regular.ttf", FR_PRIVATE, 0);

		if (noRegFont > 0)
		{
			mainFont = CreateFont(17, 0, 0, 0, FW_REGULAR, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_SWISS, L"Roboto");
		}
		else
		{
			mainFont = CreateFont(8, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Tahoma");
		}

		int fixfont = AddFontResourceEx(L"RobotoMono-Regular.ttf", FR_PRIVATE, 0);

		if (fixfont > 0)
		{
			fixFont = CreateFont(17, 0, 0, 0, FW_REGULAR, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Roboto Mono");
		}
		else
		{
			fixFont = CreateFont(8, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Consolas");
		}

		hPlayer = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MAIN_PLAYER), hWnd, Player, (LPARAM)mainFont);

		// Get some info for Screen size so that no internal windows are placed outside the desktop size
		int maxPrimaryX = GetSystemMetrics(SM_CXMAXIMIZED);
		int maxPrimaryY = GetSystemMetrics(SM_CYMAXIMIZED);
		int maxDesktopX = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int maxDesktopY = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		RECT tempRect;

		// Create Directory Window
		m_DirWind = new DirWind();
		hDirWnd = m_DirWind->hwndDir;
		SendMessage(hDirWnd, WM_SETFONT, (WPARAM)mainFont, (LPARAM)true);
		GetWindowRect(hDirWnd, &tempRect);

		if ((tempRect.left < 0) || (tempRect.top < 0) || (tempRect.bottom > maxDesktopY) || (tempRect.right > maxDesktopX))
		{
			int w = tempRect.right - tempRect.left;
			int h = tempRect.bottom - tempRect.top;

			MoveWindow(hDirWnd, 48, 96, w, h, true);
		}

		if (GetMyProfileBool("WINDOWS", "DirOpen", false, szIni))
			SendMessage(hWnd, WM_COMMAND, IDM_VIEWDIRUI, 0);

		// Create Properties Window
		myProperties = new PropertiesWindow();
		if (GetMyProfileBool("WINDOWS", "Properties", false, szIni))
			SendMessage(hWnd, WM_COMMAND, IDM_VIEW_PROPERTIES, 0);
		SendMessage(myProperties->hwndProperties, WM_SETFONT, (WPARAM)mainFont, (LPARAM)true);
		hProperties = myProperties->hwndProperties;

		// Clear pointer to Filter dialog
		hFilterDlg = NULL;

		// Create Mixer Dialog
		hMixerDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MIXER), hWnd, MixerDlg, (LPARAM)mainFont);
		if (GetMyProfileBool("WINDOWS", "Mixer", false, szIni))
			SendMessage(hWnd, WM_COMMAND, ID_VIEW_MIXER, 0);

		// Create SidID structure
		m_SidId = new SIDID();

		// Read HVSC dir from ini-file
		GetPrivateProfileStringA("HVSC", "Directory", "", hvscDir, MAX_PATH, szIni);

		// Create Songlength class
		mySLDB = new Songlength(hvscDir);

		// Show milliseconds i player?
		showMillis = GetMyProfileBool("HVSC", "ShowMilliSeconds", false, szIni);

		// Create Playlist class
		myPlayList = new PlayList();
		EnableDlgItem(hPlayer, IDB_PREVSID, myPlayList->ExistsPrev());
		EnableDlgItem(hPlayer, IDB_NEXTSID, myPlayList->ExistsNext());
		myPlayList->AddDependent(hWnd);

		// Init our random number generator
		srand(time(NULL));

		// Read settings from ini-file
		ReadAudioSettings(&as, szIni);
		m_engCfg.frequency = as.frequency;
		m_engCfg.playback = as.playback;
		m_precision = as.precision;
	

		ReadEmulationSettings(&es, szIni);
		m_engCfg.forceC64Model = es.c64modelForced;
		m_engCfg.defaultC64Model = es.c64modelDefault;
		m_engCfg.defaultSidModel = es.sidModel;
		m_engCfg.forceSidModel = es.sidModelForced;
		m_engCfg.digiBoost = es.digiBoost;
		m_engCfg.samplingMethod = es.samplingMethod;
		m_filter.enabled = es.filter;
		m_filter.bias = 0.0;
		m_filter.filterCurve6581 = es.filterCurve6581;
		m_filter.filterCurve8580 = es.filterCurve8580;
		m_driver.sid = EMU_RESIDFP;

		// Usage
		lUsageEver = GetMyProfileLong("STATS", "Usage", 0, szIni);
		lUsageSession = 0;

		// Setup audio device
		m_driver.device = NULL;
		CreateOutput(OUT_NULL);
		CreateSidEmu(EMU_NONE);
		m_driver.output = OUT_SOUNDCARD;

		m_engine.setRoms(KERNAL, BASIC, CHARACTERS);

		// Create STIL window
		mySTILView = new STILView();
		mySTILView->setHVSCdir(hvscDir);
	

		// Cleare mute variables
		v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;

		// Setup
		CheckRadioButton(hPlayer, IDB_PLAYSPEED_X1, IDB_PLAYSPEED_X4, IDB_PLAYSPEED_X1);
		uLastPlayed = 0;

		if (*infilename != '\0')
			PostMessage(hWnd, OWM_PLAYFILE, selectedSong, (LPARAM)infilename);

		break;
	}
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
		{
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About, (LPARAM)mainFont);
			break;
		}
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_FILE_OPEN:
			GetPrivateProfileStringA("SIDTUNES", "Directory", "", ofile.GetDirectory(), 256, szIni);
			if (ofile.Choose(extensions, "Open SID file"))
			{
				WritePrivateProfileStringA("SIDTUNES", "Directory", ofile.GetDirectory(), szIni);
				PostMessage(hWnd, OWM_PLAYFILE, 0, (LPARAM)ofile.GetFileName());
			}
			break;

		case ID_FILE_EXPORT:
		{
			// Export WAV
			AudioSettings wavcfg;
			SaveWavFile savewavfile(hWnd);
			int nOldStatus = m_state;

			if (!m_tune)
			{
				InfoBox(hWnd, "Error", "No file is open!");
				break;
			}
			wavcfg.setFrequency(44100);
			wavcfg.setChannels(2);
			wavcfg.setBitsPerSample(16);
			if (tuneLength > 0)
				wavcfg.setTimeToPlay(tuneLength);
			else
				wavcfg.setTimeToPlay(180);
			
			savewavfile.PresetFileTitle(infilename);
			if (savewavfile.Choose("Save as WAV file"))
			{
				Stop();
				MakeWavFile(hWnd, wavcfg, &savewavfile);
				if (nOldStatus == playerRunning)
				{
					Start();
				}
			}
			break;
		}

		case ID_FILE_CPUDEBUG:
		{
			cpudebug_param cpParam;
			cpParam.font = mainFont;
			cpParam.tune = m_tune;

			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CPUDEBUG), hWnd, CpuDebugDlg, (LPARAM)&cpParam);



			break;
		}
			// Windowhandling
		case IDM_VIEWDIRUI:
		{
			BOOL bIsOpen = GetMenuState(GetMenu(hMain), IDM_VIEWDIRUI, MF_BYCOMMAND) & MF_CHECKED;

			if (bIsOpen)
				ShowWindow(hDirWnd, SW_HIDE);
			else
				ShowWindow(hDirWnd, SW_RESTORE);

			CheckMenuItem(GetMenu(hMain), IDM_VIEWDIRUI, (MF_BYCOMMAND | bIsOpen) ? MF_UNCHECKED : MF_CHECKED);
			break;
		}

		case IDM_VIEW_PROPERTIES:
		{
			BOOL bIsOpen = GetMenuState(GetMenu(hMain), IDM_VIEW_PROPERTIES, MF_BYCOMMAND) & MF_CHECKED;

			if (bIsOpen)
				myProperties->Show(false);
			else
				myProperties->Show(true);

			CheckMenuItem(GetMenu(hMain), IDM_VIEW_PROPERTIES, (MF_BYCOMMAND | bIsOpen) ? MF_UNCHECKED : MF_CHECKED);
			break;
		}

		case ID_VIEW_MIXER:
		{
			BOOL bIsOpen = GetMenuState(GetMenu(hMain), ID_VIEW_MIXER, MF_BYCOMMAND) & MF_CHECKED;

			if (bIsOpen)
				ShowWindow(hMixerDlg, SW_HIDE);
			else
				ShowWindow(hMixerDlg, SW_RESTORE);

			CheckMenuItem(GetMenu(hMain), ID_VIEW_MIXER, (MF_BYCOMMAND | bIsOpen) ? MF_UNCHECKED : MF_CHECKED);
			break;
		}

		case IDM_FILTERSETTINGS:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_FILTERSETTINGS), hWnd, FilterDlg, (LPARAM)mainFont);
			break;

		case IDM_SETTINGS_HVSC:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_HVSCCFG), hWnd, HvscDlg, (LPARAM)mainFont);
			break;

		case IDM_SETTINGS_EMULATION:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EMULATION), hWnd, EmuDlg, (LPARAM)mainFont);
			break;

		case IDM_SETTINGS_SETTINGS:
		{
			audio_section oldas = as;
			bool bChanged = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, SettingsDlg, (LPARAM)mainFont) || ((oldas.playback != as.playback) || (oldas.frequency != as.frequency) || (oldas.precision != as.precision));

			if (bChanged)
			{
				m_engCfg.frequency = as.frequency;
				m_engCfg.playback = as.playback;
				m_precision = 16;
				PostMessage(hMain, OWM_UPDATE_ES, 0, 0);
			}
			break;
		}

		case IDM_SETTINGS_DIRECTORYUI:
		{
			bool changed = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIRSETTINGS), hWnd, DirsettingsDlg, (LPARAM)mainFont);

			if (changed)
			{
				DWORD dirSettings = GetMyProfileLong("DIR LIST", "Settings", DIR_DIFIDR, szIni);
				PostMessage(hDirWnd, OWM_REFRESH, (WPARAM)dirSettings, 0);
			}
			break;
		}


		case IDM_FILESAVEAS:
			break;

		case ID_VIEW_PLAYLIST:
			if (hPlayListView)
			{
				DestroyWindow(hPlayListView);
				hPlayListView = NULL;
			}
			else
			{
				hPlayListView = myPlayList->CreateListViewWindow(hMain);
				SendMessage(hPlayListView, OWM_SETFILEEXTENSIONS, 0, (LPARAM)&extensions);
				SendMessage(hPlayListView, WM_SETFONT, (WPARAM)mainFont, (LPARAM)true);
				ShowWindow(hPlayListView, SW_SHOW);
			}
			CheckMenuItem(GetMenu(hWnd), ID_VIEW_PLAYLIST, MF_BYCOMMAND | (hPlayListView != NULL ? MF_CHECKED : MF_UNCHECKED));
			break;

		case IDM_COPYFILENAME:
		{
			const SidTuneInfo* info = m_tune->getInfo();
			if (info->dataFileName() != NULL && OpenClipboard(hWnd) != FALSE && EmptyClipboard() != FALSE)
			{
				char* pStr;
				int pLength = strlen(info->dataFileName());
				if (info->path())
					pLength += strlen(info->path());

				HGLOBAL hStr = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, pLength + 1);
				if ((pStr = (char*)GlobalLock(hStr)) != NULL)
				{
					if (info->path() == NULL)
						pStr[0] = '\0';
					else
					{
						strcpy_s(pStr, pLength + 1, info->path());
					}
				}
				strcat_s(pStr, pLength + 1, info->dataFileName());
				GlobalUnlock(pStr);
				SetClipboardData(CF_TEXT, hStr);
				CloseClipboard();
			}
			break;
		}

			// Accelerators
		case IDA_SPEEDX1:
			SetSpeed(1);
			break;

		case IDA_SPEEDX2:
			SetSpeed(2);
			break;

		case IDA_SPEEDX4:
			SetSpeed(4);
			break;

		case IDA_SPEEDX8:
			SetSpeed(8);
			break;

		case IDA_PREVSONG:
		
			if (m_tune && (selectedSong - 1 >= 1))
			{
				SendMessage(hWnd, OWM_PLAYFILE, selectedSong - 1, 0);
			}
			break;

		case IDA_NEXTSONG:
		{
			if (m_tune)
			{
				const SidTuneInfo *info = m_tune->getInfo();
				if (selectedSong + 1 <= info->songs())
				{
					SendMessage(hWnd, OWM_PLAYFILE, selectedSong + 1, 0);
				}
			}
			break;
		}

		case IDA_NEXTSID:
			SendMessage(hPlayer, WM_COMMAND, MAKEWPARAM(IDB_NEXTSID, 0), 0);
			break;

		case IDA_PREVSID:
			SendMessage(hPlayer, WM_COMMAND, MAKEWPARAM(IDB_PREVSID, 0), 0);
			break;

		case IDA_PLAYPAUSE:
			SendMessage(hPlayer, WM_COMMAND, MAKEWPARAM(IDB_PLAYPAUSE, 0), 0);
			break;

		case IDA_STOP:
			if (m_state == playerRunning)
				SendMessage(hPlayer, WM_COMMAND, MAKEWPARAM(IDB_STOP, 0), 0);
			else if (m_state == playerStopped)
				SendMessage(hPlayer, WM_COMMAND, MAKEWPARAM(IDB_PLAYPAUSE, 0), 0);	
			break;
		
		default:

			break;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		WINDOWPLACEMENT wpl;
		wpl.length = sizeof(WINDOWPLACEMENT);

		if (GetWindowPlacement(hWnd, &wpl))
		{
			WriteMyProfileInt("WINDOWS", "MainPosX", wpl.rcNormalPosition.left, szIni);
			WriteMyProfileInt("WINDOWS", "MainPosY", wpl.rcNormalPosition.top, szIni);
		}

		WriteMyProfileBool("WINDOWS", "DirOpen", (GetMenuState(GetMenu(hWnd), IDM_VIEWDIRUI, MF_BYCOMMAND) & MF_CHECKED) != 0, szIni);
		WriteMyProfileBool("WINDOWS", "Properties", (GetMenuState(GetMenu(hWnd), IDM_VIEW_PROPERTIES, MF_BYCOMMAND) & MF_CHECKED) != 0, szIni);
		WriteMyProfileLong("STATS", "Usage", lUsageEver, szIni);
		WriteMyProfileBool("HVSC", "ShowMilliSeconds", showMillis, szIni);

		RemoveFontResourceEx(L"Roboto-Regular.ttf", FR_PRIVATE, 0);
		RemoveFontResourceEx(L"RobotoMono-Regular.ttf", FR_PRIVATE, 0);

		if (keyboardHook)
		{
			UnhookWindowsHookEx(keyboardHook);
		}

		CleanUp();
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)        // Iconized
			break;
		GetWindowRect(hPlayer, &rectChild);
		GetWindowRect(hMain, &rectAll);
		GetClientRect(hMain, &rectClient);
		MoveWindow(hMain, rectAll.left, rectAll.top,
			rectChild.right - rectChild.left + rectAll.right - rectAll.left - rectClient.right,
			rectChild.bottom - rectChild.top + rectAll.bottom - rectAll.top - rectClient.bottom,
			TRUE);
		break;

		// OWN stuff here
	case OWM_PLAYFILE:
		selectedSong = wParam;
		secondsFadeout = 0;

		if (lParam)
		{
			// Are we dealing with a playlist?
			if (extensions.isCorrectExtension((LPSTR)lParam, TYPE_LIST))
			{
				// Yes, wer are
				PlayList* tmplist = new PlayList((LPSTR)lParam);
				if (tmplist->Status())
				{
					delete myPlayList;
					myPlayList = tmplist;
					if (myPlayList->Next())
					{
						SendMessage(hWnd, OWM_PLAYENTRY, 0, 0);
					}
					else
					{
						MessageBoxA(hWnd, "Error reading playlist file", (LPSTR)lParam, MB_OK | MB_ICONEXCLAMATION);
					}
				}
			}
			else
			{
				bool bInitSuccess;
				strcpy_s(infilename, (LPSTR)lParam);

				Stop();

				bInitSuccess = LoadSid(extensions);
				mySTILView->showEntry(infilename, selectedSong);
				if (bInitSuccess)
				{
					Start();
				}
			}
		}
		else
		{
			bool bInitSuccess;

			Stop();

			bInitSuccess = InitTune(selectedSong);
			mySTILView->showEntry(infilename, selectedSong);
			if (bInitSuccess)
			{
				Start();
				SetDlgItemText(hPlayer, IDB_PLAYPAUSE, L"Pause");
			}
		}
		break;

	case OWM_PLAYATOMFILE:
		GlobalGetAtomNameA((ATOM)lParam, szIni, sizeof(szIni));
		GlobalDeleteAtom((ATOM)lParam);
		SendMessageA(hMain, OWM_PLAYFILE, wParam, (LPARAM)szIni);
		break;

	case OWM_PLAYENTRY:
	{
		PlayListEntry* tmp = NULL;
		if (myPlayList)
			tmp = myPlayList->Cur();
		if (tmp)
		{
			bool bInitSuccess;
			Stop();
			selectedSong = tmp->SubSong();
			//uSecondsToPlay = myPlayList->PlayTime(tmp);
			secondsFadeout = myPlayList->FadeoutTime(tmp);
			strcpy_s(infilename, tmp->Filename().c_str());

			bInitSuccess = LoadSid(extensions);
			mySTILView->showEntry(infilename, selectedSong);
			if (bInitSuccess)
			{
				Start();
			}
		}
		break;
	}

	case OWM_LISTCHANGED:
		EnableDlgItem(hPlayer, IDB_PREVSID, myPlayList->ExistsPrev());
		EnableDlgItem(hPlayer, IDB_NEXTSID, myPlayList->ExistsNext());
		break;

	case OWM_UPDATE_ES:
	{
		bool bInitSuccess;
		Stop();
		m_engCfg.defaultC64Model = es.c64modelDefault;
		m_engCfg.forceC64Model = es.c64modelForced;
		m_engCfg.defaultSidModel = es.sidModel;
		m_engCfg.forceSidModel = es.sidModelForced;
		m_engCfg.digiBoost = es.digiBoost;
		m_engCfg.samplingMethod = es.samplingMethod;
		bInitSuccess = LoadSid(extensions);
		mySTILView->showEntry(infilename, selectedSong);
		if (bInitSuccess)
		{
			Start();
		}
	}
		break;


	case WM_COPYDATA:
	{
		PCOPYDATASTRUCT pMyCDS;
		pMyCDS = (PCOPYDATASTRUCT)lParam;
		if (pMyCDS->dwData == 448)
		{
			char *szTmp = (char *)pMyCDS->lpData;
			size_t sz = pMyCDS->cbData;
			strcpy_s(infilename, MAX_PATH + 1, szTmp);

			// Check if we got the filepath null-terminated
			if (infilename[sz] != '\0')
				infilename[sz] = '\0';
			PostMessage(hWnd, OWM_PLAYFILE, 0, (LPARAM)infilename);
		}
		break;
	}

	case OWM_SETTIMEPLAYED:
	{
		WCHAR szTmp[32];
		uint_least32_t lNow = m_engine.time();

		// Update mileage
		lUsageSession += lNow - uLastPlayed;
		lUsageEver += lNow - uLastPlayed;
		uLastPlayed = lNow;


		int mm = m_engine.timeMs() % 1000;
		if (showMillis)
		{
			swprintf_s(szTmp, L"%02lu:%02lu:%02lu.%03lu", uLastPlayed / 3600, (uLastPlayed / 60) % 60, uLastPlayed % 60, mm);
		}
		else
		{
			swprintf_s(szTmp, L"%02lu:%02lu:%02lu      ", uLastPlayed / 3600, (uLastPlayed / 60) % 60, uLastPlayed % 60);
		}
		SetDlgItemText(hPlayer, IDC_TIME, szTmp);

		break;
	}

	case OWM_UPDATESL:
	{
		char md5[SidTune::MD5_LENGTH + 1];
		if (m_tune)
		{
			m_tune->createMD5New(md5);
			tuneLength = mySLDB->Length(md5, selectedSong);
		}
		else
		{
			if (myPlayList)
			{
				PlayListEntry* curEntry = myPlayList->Cur();
				if (curEntry)
				{
					tuneLength = curEntry->PlayTime();
				}
				else
				{
					tuneLength = 0;
				}
			}
			else
			{
				tuneLength = 0;
			}
			uSecondsToPlay = tuneLength;
		}
		if (tuneLength > 0)
		{
			int h, m, s, t, mill;
			char text[14];
			mill = tuneLength >> 16;
			int tuneLengthLower = tuneLength & 0xffff;
			uSecondsToPlay = tuneLengthLower;
			h = int(tuneLengthLower / 3600);
			t = tuneLengthLower % 3600;
			m = int(t / 60);
			s = t % 60;
			if (showMillis)
			{
				sprintf_s(text, "%02d:%02d:%02d.%03d", h, m, s, mill);
			}
			else
			{
				sprintf_s(text, "%02d:%02d:%02d     ", h, m, s);
			}

			SetDlgItemTextA(hPlayer, IDC_LENGTH, text);
		}
		else
			SetDlgItemText(hPlayer, IDC_LENGTH, L"N/A");
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


//---------------------------------------------------------------------------------------------------
// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HFONT myFont;
	static string infoText;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		myFont = (HFONT)lParam;
		SendDlgItemMessage(hDlg, IDC_STATIC1, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_INFO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		char tmptext[14];
		string convertedCredits = "";
		int lDaysSession = lUsageSession / (3600 * 24);
		int lHoursSession = (lUsageSession / 3600) % 24;
		int lMinsSession = (lUsageSession / 60) % 60;
		int lSecsSession = lUsageSession % 60;

		int lDaysEver = lUsageEver / (3600 * 24);
		int lHoursEver = (lUsageEver / 3600) % 24;
		int lMinsEver = (lUsageEver / 60) % 60;
		int lSecsEver = lUsageEver % 60;

		string mileageNow, mileageEver, infoText;

		if (lDaysSession < 1)
		{
			sprintf_s(tmptext, "%2lu:%02lu:%02lu", lHoursSession, lMinsSession, lSecsSession);
			mileageNow = string(tmptext);
		}
		else if (lDaysSession < 2)
		{
			sprintf_s(tmptext, "1 day, %02lu:%02lu:%02lu", lHoursSession, lMinsSession, lSecsSession);
			mileageNow = string(tmptext);
		}
		else
		{
			sprintf_s(tmptext, "%lu days, %02lu:%02lu:%02lu", lDaysSession, lHoursSession, lMinsSession, lSecsSession);
			mileageNow = string(tmptext);
		}

		if (lDaysEver < 1)
		{
			sprintf_s(tmptext, "%2lu:%02lu:%02lu", lHoursEver, lMinsEver, lSecsEver);
			mileageEver = string(tmptext);
		}
		else if (lDaysEver < 2)
		{
			sprintf_s(tmptext, "1 day, %02lu:%02lu:%02lu", lHoursEver, lMinsEver, lSecsEver);
			mileageEver = string(tmptext);
		}
		else
		{
			sprintf_s(tmptext, "%lu days, %02lu:%02lu:%02lu", lDaysEver, lHoursEver, lMinsEver, lSecsEver);
			mileageEver = string(tmptext);
		}

		infoText = "Sidplay Windows, version 3.0\n\n";
		infoText += "GUI code by Adam Lorenzon -2006\n";
		infoText += "Updated and rewritten by Inge H. Pedersen 2007-2022\n\n";
		
		// Get info from LibSidPlayFP
		const SidInfo& info = m_engine.info();
		const int credits = info.numberOfCredits();
		for (int i = 0; i < credits; i++)
		{
			infoText += info.credits(i);
			infoText += "\n";
		}

		ReSIDfpBuilder *rsf = new ReSIDfpBuilder("residfp");

		infoText += rsf->credits();
		infoText += "\n";
		infoText += "STILView v2.20\n(C) 1998, 2002 by LaLa (LaLa@C64.org).\n(C) 2019 by Inge H. Pedersen (ingehp@outlook.com)\n\n";

		infoText += "Roboto Regular and Roboto Mono Regular\n";
		infoText += "Font design by Christian Robertson / Google Fonts\n\n";

		infoText += "Mileage:\n";
		infoText += "This session:\t" + mileageNow + "\n";
		infoText += "Total usage:\t" + mileageEver + "\n\n";

		infoText += "Thanks to Shine, Yodelking and the HVSC crew\n";
		infoText += "for testing and suggestions.\n\n";


		for (int i = 0; i < infoText.length(); i++)
		{
			char c = infoText[i];
			switch (c)
			{
			case '\r':
				break;
			case '\n':
				convertedCredits.push_back('\r');
				convertedCredits.push_back('\n');
				break;
			default:
				convertedCredits.push_back(c);
			}
		}
		SetDlgItemTextA(hDlg, IDC_INFO, convertedCredits.c_str());

		delete rsf;

		return (INT_PTR)TRUE;

	}
	
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
} 

// Message handler for the player window
INT_PTR CALLBACK Player(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT rect;
	PAINTSTRUCT ps;
	static HFONT myFont;
	static long painted = 0;
	int i = 0;

	switch (message)
	{
	case WM_INITDIALOG:
		myFont = (HFONT) lParam;
		SendDlgItemMessage(hDlg, IDC_TEXT1, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_TEXT2, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_TEXT3, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_TEXT4, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_TEXT5, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		
		SendDlgItemMessage(hDlg, IDC_NAME, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_AUTHOR, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_RELEASED, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_CURRENTSONG, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_TIME, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_LENGTH, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);

		SendDlgItemMessage(hDlg, IDC_GROUPBOX4, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDB_PLAYSPEED_X1, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDB_PLAYSPEED_X2, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDB_PLAYSPEED_X4, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDB_STOP, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDB_PLAYPAUSE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDB_PREVSID, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDB_NEXTSID, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);



		return (INT_PTR)TRUE;
		break;

	case WM_PAINT:
		if (isMusPlayer && (painted > 1))
		{
			hdc = BeginPaint(hDlg, &ps);
			rect.top = 2;
			rect.bottom = 98;
			rect.left = 8;
			rect.right = 280;
			FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

			CreateMusInfo(hdc);
			EndPaint(hDlg, &ps);
		}
		painted++;
		break;

	case WM_HSCROLL:
	{
		WORD song = selectedSong;
		const SidTuneInfo *ti = m_tune->getInfo();
		const SidInfo &info = m_engine.info();

		switch (LOWORD(wParam))
		{
		case SB_BOTTOM:
			song = ti->songs();
			break;
		case SB_TOP:
			song = 1;
			break;
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			song += 1;
			break;
		case SB_LINEUP:
		case SB_PAGEUP:
			song -= 1;
			break;
		case SB_THUMBPOSITION:
		{
			song = HIWORD(wParam);
			if (song == selectedSong)
			{
				char szTmp[128];
				sprintf_s(szTmp, "Song %d/%d playing at %s", selectedSong, ti->songs(), info.speedString());
				SetDlgItemTextA(hPlayer, IDC_CURRENTSONG, szTmp);
			}
			break;
		}

		case SB_THUMBTRACK:
		{
			WORD tracksong = HIWORD(wParam);
			WCHAR szTmp[128];
			swprintf_s(szTmp, L"Song %d/%d", tracksong, ti->songs());
			break;
		}

		} // Switch

		song = song < 1 ? 1 : (song > ti->songs() ? ti->songs() : song);
		if (song != selectedSong)
			PostMessage(hMain, OWM_PLAYFILE, song, 0);
		break;
	}

	case WM_DROPFILES:
	{
		UINT f = DragQueryFile((HDROP)wParam, -1, NULL, 0);
		// if f > 1, make a playlist maybe?
		if (f > 0)
		{
			DragQueryFile((HDROP)wParam, 0, inFile, _countof(inFile));
			wtoc(inFile, infilename);
			PostMessage(hMain, OWM_PLAYFILE, 0, (LPARAM)infilename);
		}
		DragFinish((HDROP)wParam);
		break;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDB_STOP:
			Stop();
			m_state = playerStopped;
			SetDlgItemText(hDlg, IDB_PLAYPAUSE, L"Play");
			uLastPlayed = 0;
			break;
		case IDB_PLAYPAUSE:
			if (!m_tune)
				break;
			switch (m_state)
			{
			case playerStopped:
				if (InitTune(selectedSong))
				{
					Start();
					SetDlgItemText(hDlg, IDB_PLAYPAUSE, L"Pause");
				}
				break;
			case playerRunning:
				m_state = playerPaused;
				m_threadstate.threadPause = true;

				m_driver.selected->pause();
				SetDlgItemText(hDlg, IDB_PLAYPAUSE, L"Play");
				break;
			case playerPaused:
				m_state = playerRunning;
				m_threadstate.threadPause = false;
				m_driver.selected->pause();
				SetDlgItemText(hDlg, IDB_PLAYPAUSE, L"Pause");
				break;
			}
			break;
		case IDB_PREVSID:
			if (myPlayList && myPlayList->Prev())
			{
				SendMessage(hMain, OWM_PLAYENTRY, 0, 0);
			}
			break;
		case IDB_NEXTSID:
			if (myPlayList && myPlayList->Next())
			{
				SendMessage(hMain, OWM_PLAYENTRY, 0, 0);
			}
			else
			{
				SendMessage(hMain, WM_COMMAND, MAKEWPARAM(IDA_STOP, 0), 0);
			}
			myPlayList->isPending = false;
			break;
		case IDB_PLAYSPEED_X1:
			SetSpeed(1);
			break;
		case IDB_PLAYSPEED_X2:
			SetSpeed(2);
			break;
		case IDB_PLAYSPEED_X4:
			SetSpeed(4);
			break;
		case IDOK:
			DestroyWindow(hMain);
			return true;
		default:
			break;
		}

		break;

	default:
		break;
	}
	return (INT_PTR)FALSE;
}

// -----------------------------------------------------------------------------------------------

bool LoadSid(FileExtensions &extension)
{
	bool bInitSuccess;
	static bool bPrevMusPlayer = false;

	// Enough for a loong filepath
	char szTemp[280];

	if (!*infilename)
		return false;

	if (m_tune)
	{
		bPrevMusPlayer = isMusPlayer;
		delete m_tune;
		m_tune = NULL;
	}

	const char **extArray = extension.MakeExtArray();
	m_tune = new SidTune(infilename, extArray);
	delete[] extArray;

	if ((m_tune) && (!m_tune->getStatus()))    // Error while loading
	{
		InfoBox(NULL, "Error", "%s\n", m_tune->statusString());
		delete m_tune;
		m_tune = NULL;
		SetDlgItemText(hPlayer, IDC_NAME, L"No file loaded");
		SetDlgItemText(hPlayer, IDC_AUTHOR, L"-");
		SetDlgItemText(hPlayer, IDC_RELEASED, L"-");
		SetDlgItemText(hPlayer, IDB_PLAYPAUSE, L"Play");
		SetDlgItemText(hPlayer, IDC_TIME, L"00:00:00");
		SetDlgItemText(hPlayer, IDC_CURRENTSONG, L"");
		SetDlgItemText(hPlayer, IDC_LENGTH, L"");
		return FALSE;
	}
	m_SidIdFound = m_SidId->identifyfile(infilename);

	const SidTuneInfo *tuneInfo = m_tune->getInfo();
	const unsigned int nInf = tuneInfo->numberOfInfoStrings();
	const unsigned int nCom = tuneInfo->numberOfCommentStrings();

	if (nInf == 0)
	{
		isMusPlayer = true;
	}
	else
	{
		isMusPlayer = false;
	}

	if (!isMusPlayer)
	{
		// Window title is Author: Title, but if Title is uknown <?>, use filename instead
		sprintf_s(szTemp, "%s: %s", tuneInfo->infoString(1), tuneInfo->infoString(0));
	}
	else
	{
		sprintf_s(szTemp, "%s", tuneInfo->dataFileName());
	}
	SetWindowTextA(hMain, szTemp);

	// Change dialog if switching between normal SID and Sidplayer MUS
	if (isMusPlayer != bPrevMusPlayer)
	{
		ChangeMainDialog();
	}

	if (selectedSong == 0)
	{
		if (tuneInfo->currentSong() == 0)
		{
			selectedSong = tuneInfo->startSong();
		}
		else
		{
			selectedSong = tuneInfo->currentSong();
		}
	}
	else
	{
		if (selectedSong > tuneInfo->songs())
			selectedSong = tuneInfo->startSong();
	}

	bInitSuccess = InitTune(selectedSong);

	const SidInfo &info = m_engine.info();
	if (isMusPlayer)
	{
		RECT rect;
		rect.top = 0;
		rect.bottom = 150;
		rect.left = 5;
		rect.right = 325;
		InvalidateRect(hPlayer, &rect, true);
	}
	else
	{
		// Extra steps to secure correct UNICODE chars
		wchar_t textbuffer[40];
		ctow(tuneInfo->infoString(0), textbuffer);
		SetDlgItemText(hPlayer, IDC_NAME, textbuffer);
		ctow(tuneInfo->infoString(1), textbuffer);
		SetDlgItemText(hPlayer, IDC_AUTHOR, textbuffer);
		ctow(tuneInfo->infoString(2), textbuffer);
		SetDlgItemText(hPlayer, IDC_RELEASED, textbuffer);
	}

	SetDlgItemText(hPlayer, IDC_TIME, L"00:00:00");
	if (!isMusPlayer)
		sprintf_s(szTemp, "Song %d/%d playing at %s", selectedSong, tuneInfo->songs(), info.speedString());
	SetDlgItemTextA(hPlayer, IDC_CURRENTSONG, szTemp);

	EnableDlgItem(hPlayer, IDB_PLAYSPEED_X1, true);
	EnableDlgItem(hPlayer, IDB_PLAYSPEED_X2, true);
	EnableDlgItem(hPlayer, IDB_PLAYSPEED_X4, true);
	EnableDlgItem(hPlayer, IDB_STOP, true);
	EnableDlgItem(hPlayer, IDB_PLAYPAUSE, true);

	if (!bInitSuccess)
		return false;

	RefreshProperties();

	SetDlgItemText(hPlayer, IDB_PLAYPAUSE, L"Pause");

	return true;
}

bool InitTune(unsigned int song)
{
	if (!m_tune)
		return false;

	m_tune->selectSong(song);

	if (!m_tune->getStatus())
	{
		InfoBox(NULL, "Error", m_tune->statusString());
		return false;
	}

	if (!m_engine.load(m_tune))
	{
		InfoBox(NULL, "Error", m_engine.error());
		return false;
	}

	const SidTuneInfo *ti = m_tune->getInfo();
	const SidInfo &info = m_engine.info();

	if (!CreateOutput(m_driver.output))
		return false;
	if (!CreateSidEmu(m_driver.sid))
		return false;

	// Configure engine with settings
	if (!m_engine.config(m_engCfg))
	{
		// Config failed
		InfoBox(NULL, "Error", m_engine.error());
		return false;
	}

	m_driver.selected = &m_driver.null;
	m_speed.current = m_speed.max;
	m_engine.fastForward(100 * m_speed.current);

	m_engine.mute(0, 0, v1mute);
	m_engine.mute(0, 1, v2mute);
	m_engine.mute(0, 2, v3mute);
	m_engine.mute(1, 0, v4mute);
	m_engine.mute(1, 1, v5mute);
	m_engine.mute(1, 2, v6mute);
	m_engine.mute(2, 0, v7mute);
	m_engine.mute(2, 1, v8mute);
	m_engine.mute(2, 2, v9mute);

	// Implement a Master Volume ?
	DWORD vol = dwMaxWavVol + (dwMaxWavVol << 16);
	m_driver.device->setVolume(vol);

	EnableDlgItem(hPlayer, IDC_SCRSUBSONG, ti->songs() > 1);
	if (ti->songs() > 1)
	{
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 1;
		si.nMax = ti->songs();
		si.nPage = 1;
		si.nPos = song;
		SetScrollInfo(GetDlgItem(hPlayer, IDC_SCRSUBSONG), SB_CTL, &si, true);
	}

	char szTmp[128];
	sprintf_s(szTmp, "Song %d/%d playing at %s", selectedSong, ti->songs(), info.speedString());
	SetDlgItemTextA(hPlayer, IDC_CURRENTSONG, szTmp);

	PostMessage(hMain, OWM_UPDATESL, 0, 0);

	updateMixer(hMixerDlg, 0);

	return true;
}

// -----------------------------------------------------------------------------------------------
//
// Utilities
//
//

void InfoBox(HWND hwnd, const char *title, const char* szFormat...)
{
	char szMessBuffer[4096];
	char *pArguments;

	pArguments = (char *)&szFormat + sizeof(szFormat);
	vsprintf_s(szMessBuffer, szFormat, pArguments);
	MessageBoxA(hwnd, szMessBuffer, (title == NULL) ? "Info" : title,
		MB_OK | MB_ICONINFORMATION);
}

void RefreshProperties()
{
	static const char *sidModels[] = {
		"Unknown",
		"6581",
		"8580",
		"6581 or 8580",
	};
	static const char *clockSpeeds[] = {
		"Unknown",
		"PAL",
		"NTSC",
		"PAL or NSTC",
	};
	static const char *environments[] = {
		"PlaySID",
		"Transparent ROM",
		"Full Bank-switching",
		"Real C64"
	};

	const SidTuneInfo *ti = m_tune->getInfo();
	const SidInfo &info = m_engine.info();

	char *szTmp = new char[16384];
	int pr;

	pr = sprintf_s(szTmp, 16383,
		"Filename:   \t%s%s\r\n"
		"Data size:  \t%ld bytes\r\n"
		"File type:  \t%s\r\n"
		"File status:\t%s\r\n",
		ti->path(), ti->dataFileName(), ti->c64dataLen(), ti->formatString(), m_tune->statusString());

	if (m_SidIdFound > 0)
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr,
			"Player name(s):\t%s\r\n\r\n", m_SidId->playersfound);
	}
	else if (isMusPlayer)
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr,
			"Player name(s):\tCOMPUTE!'s Sidplayer (internal)\r\n\r\n");
	}
	else
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr,
			"Player name(s):\t<Not recognized>\r\n\r\n");
	}
	if (!isMusPlayer)
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr,
			"Name:       \t%s\r\n"
			"Author:     \t\t%s\r\n"
			"Released:\t%s\r\n",
			ti->infoString(0), ti->infoString(1), ti->infoString(2));
	}

	pr += sprintf_s(szTmp + pr, 16383 - pr,
		"Load range:\t$%04X-$%04X\r\n", ti->loadAddr(), ti->loadAddr() + ti->c64dataLen() - 1);

	pr += sprintf_s(szTmp + pr, 16383 - pr,
		"Init address:\t$%04X\r\n", ti->initAddr());

	pr += sprintf_s(szTmp + pr, 16383 - pr,
		"Play address:\t$%04X\r\n", ti->playAddr());

	pr += sprintf_s(szTmp + pr, 16383 - pr,
		"No. of songs:\t%d\r\n", ti->songs());

	pr += sprintf_s(szTmp + pr, 16383 - pr,
		"Default song:\t%d\r\n", ti->startSong());

	pr += sprintf_s(szTmp + pr, 16383 - pr,
		"Clock speed:\t%s\r\n", clockSpeeds[ti->clockSpeed()]);

	pr += sprintf_s(szTmp + pr, 16383 - pr,
		"SID Model:\t%s\r\n", sidModels[ti->sidModel(0)]);

	int ch = 2;
	while (ti->sidChips() >= ch)
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr,
			"SID%d model:\t%s\r\n"
			"SID%d address:\t$%04X\r\n",
			ch, sidModels[ti->sidModel(ch - 1)], ch, ti->sidChipBase(ch - 1));
		ch++;
	}

	if (ti->relocStartPage() == 0)
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr, "Reloc region:\tAuto");
	}
	else if ((ti->relocStartPage() == 0xff) || (ti->relocPages() == 0))
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr, "Reloc region:\tNone Available");
	}
	else
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr,
			"Reloc region:\t$%04X-$%04X",
			ti->relocStartPage() << 8,
			((ti->relocStartPage() + ti->relocPages() - 1) << 8) + 255);
	}

	if (!isMusPlayer)
	{
		char md5[SidTune::MD5_LENGTH + 1];
		md5[SidTune::MD5_LENGTH] = '\0';
	
		//pr += sprintf_s(szTmp + pr, 16383 - pr,
		//	"\r\nMD5:\t\t%s\r\n", md5);
		m_tune->createMD5New(md5);
		pr += sprintf_s(szTmp + pr, 16383 - pr,
			"\r\nMD5:\t\t%s\r\n", md5);
	}
	// Runtime properties

	pr += sprintf_s(szTmp + pr, 16383 - pr,
		"\r\n\r\n"
		"Emulation\r\n"
		"=========\r\n"
		"SID model:\t%s\r\n"
		"Clock speed:\t%s\r\n",
		sidModels[ti->sidModel(0)], m_engine.info().speedString());

	if (info.driverLength() != 0)
	{
		pr += sprintf_s(szTmp + pr, 16383 - pr,
			"Driver region:\t$%04X-$%04X\r\n",
			//"Power-on delay:\t%u\r\n",
			m_engine.info().driverAddr(), m_engine.info().driverAddr() + (m_engine.info().driverLength() - 1));
	}

	wchar_t *szInfo = new wchar_t[pr + 2];
	ctow(szTmp, szInfo);
	myProperties->update(szInfo);
	delete[] szInfo;
	delete[] szTmp;
}

// Setup SID Emulation
bool CreateSidEmu(SIDEMUS emu)
{
	// Remove old driver and emulation
	if (m_engCfg.sidEmulation)
	{
		sidbuilder *builder = m_engCfg.sidEmulation;
		m_engCfg.sidEmulation = NULL;
		m_engine.config(m_engCfg);
		delete builder;
	}

	// Now setup the sid emulation
	switch (emu)
	{
	case EMU_RESIDFP:
	{
		try
		{
			rs = new ReSIDfpBuilder("residfp");
			m_engCfg.sidEmulation = rs;
			if (!rs->getStatus()) goto createSidEmu_error;
			rs->create((m_engine.info()).maxsids());
			if (!rs->getStatus()) goto createSidEmu_error;

			if (m_filter.filterCurve6581)
				rs->filter6581Curve(m_filter.filterCurve6581);
			if (m_filter.filterCurve8580)
				rs->filter8580Curve((double)m_filter.filterCurve8580);
		}
		catch (std::bad_alloc const &ba) {}
		break;
	}

	/*
case EMU_RESID:
{
	try
	{
		ReSIDBuilder *rs = new ReSIDBuilder("resid");
		m_engCfg.sidEmulation = rs;
		if (!rs->getStatus()) goto createSidEmu_error;
		rs->create((m_engine.info()).maxsids());
		if (!rs->getStatus()) goto createSidEmu_error;

		rs->bias(m_filter.bias);
	}
	catch (std::bad_alloc const &ba) {}
	break;
} */

	default:
		// Some kind of emulation not supported.
		break;
	}

	if (!m_engCfg.sidEmulation)
	{
		if (emu > EMU_DEFAULT)
		{
			// Display error
			return false;
		}
	}

	if (m_engCfg.sidEmulation)
	{
		m_engCfg.sidEmulation->filter(m_filter.enabled);
	}

	return true;

createSidEmu_error:
	InfoBox(NULL, "Error", m_engCfg.sidEmulation->error());
	delete m_engCfg.sidEmulation;
	m_engCfg.sidEmulation = NULL;
	return false;
}

bool CreateOutput(OUTPUTS driver)
{
	// Remove old audio driver
	m_driver.null.close();
	m_driver.selected = &m_driver.null;
	if (m_driver.device != NULL)
	{
		if (m_driver.device != &m_driver.null)
			delete m_driver.device;
		m_driver.device = NULL;
	}

	// Create Audio driver
	switch (driver)
	{
	case OUT_NULL:
		m_driver.device = &m_driver.null;
		break;

	case OUT_SOUNDCARD:
		try
		{
			m_driver.device = new audioDrv();
		}
		catch (std::bad_alloc const &ba)
		{
			m_driver.device = 0;
		}
		break;

	case OUT_WAV:
	default:
		break;
	}

	// Audio driver failed
	if (!m_driver.device)
	{
		m_driver.device = &m_driver.null;
		InfoBox(NULL, "Error", "Couldn't create output");
		return false;
	}

	// Configure user settings
	m_driver.cfg.frequency = m_engCfg.frequency;
	m_driver.cfg.precision = 16;	// 8 bits not supported anymore
	m_driver.cfg.channels = 1;
	m_driver.cfg.bufSize = 0;	// Recalculate

	if (m_engCfg.playback == SidConfig::STEREO)
		m_driver.cfg.channels = 2;

	{
		// Open the Hardware
		bool err = false;
		if (!m_driver.device->open(m_driver.cfg))
			err = true;

		// Can't open the same driver twice
		if (driver != OUT_NULL)
		{
			if (!m_driver.null.open(m_driver.cfg))
				err = true;
		}

		if (err)
		{
			InfoBox(NULL, "Error", m_driver.device->getErrorString());
			return false;
		}
	}

	// See what we got from the audio-system
	m_engCfg.frequency = m_driver.cfg.frequency;
	m_precision = m_driver.cfg.precision;

	switch (m_driver.cfg.channels)
	{
	case 1:
		if (m_engCfg.playback == SidConfig::STEREO)
			m_engCfg.playback = SidConfig::MONO;
		break;
	case 2:
		if (m_engCfg.playback == SidConfig::MONO)
			m_engCfg.playback = SidConfig::STEREO;
		break;
	default:
		InfoBox(NULL, "Error", "%d audio channels not supported.", m_driver.cfg.channels);
		return false;
	}
	return true;
}

void Start()
{
	m_state = playerRunning;
	m_threadstate.threadPlaying = true;
	m_threadstate.threadPause = false;

	m_driver.selected = m_driver.device;
	dwThreadId = 0;
	eot = false;
	rsecs = 0;

	// hPlayThread = CreateThread(NULL, 0, PlayThread, NULL, 0, &dwThreadId);
	hPlayThread = (HANDLE)_beginthreadex(0, 0, PlayThread , (LPVOID) &m_threadstate, 0,0);
	

	if (hPlayThread == NULL)
		return;
	
	SetThreadPriority(hPlayThread, THREAD_PRIORITY_HIGHEST);
}

void Stop()
{
	if ((m_state == playerRunning) || (m_state == playerPaused) || (m_state == playerExit))
	{
		bool paused = (m_state == playerPaused) ? true : false;
		m_state = playerStopped;

		// Check if Thread is running
		if ((m_threadstate.threadPlaying))
		{
			// Wait for Thread to stop
			m_threadstate.threadPlaying = false;
			m_threadstate.threadPause = false;

			if (paused)
				WaitForSingleObject(hPlayThread, 100);
			else
				WaitForSingleObject(hPlayThread, INFINITE);
		}

		// For debugging
		//MessageBox(hPlayer, L"Stop - before CloseHandle", L"DEBUG", 0);
		CloseHandle(hPlayThread);
		hPlayThread = NULL;
		m_driver.selected->reset();
		m_engine.stop();

	}
	if (showMillis)
	{
		SetDlgItemText(hPlayer, IDC_TIME, L"00:00:00.000");
	}
	else
	{
		SetDlgItemText(hPlayer, IDC_TIME, L"00:00:00");
	}

	SetDlgItemText(hPlayer, IDB_PLAYPAUSE, L"Play");
}

bool Play()
{
	if (m_state == playerRunning)
	{
		// Fill buffer
		short *buffer = m_driver.selected->buffer();
		uint_least32_t length = m_driver.cfg.bufSize;
		//uint_least32_t length = 11024;
		/*
		if (rsecs == (tuneLength & 0xffff) && rsecs > 0)
		{
			if (mm == 0 || millis >= mm) // add a flag here
			{
				millis = mm;
				PostMessage(hMain, OWM_SETTIMEPLAYED, 0, 0);
				m_state = playerExit;
				return false;
			}
			if ((millis) + mil > mm)
			{
				while (mm > mil)
					mm -= mil;
				length = (int) 2 * mm * 44.1;
			}
		}*/
		uint_least32_t ret = 0;
		if (length > 0)
			ret = m_engine.play(buffer, length);

		PostMessage(hMain, OWM_SETTIMEPLAYED, 0, 0);

		if (ret < length)
		{
			if (m_engine.isPlaying())
			{
				m_state = playerError;
				return false;
			}
			return false;
		}
	}

	if (m_state == playerExit)
		return false;

	return true;
}

unsigned int __stdcall PlayThread(LPVOID data)
{
	thread_state_t *state = (thread_state_t *)data;
	bool ret = false;
	static int fadeCountDown = -1;
	static int fadeSub = 0;
	static int fadeVol = 0;
	static int fadeTime = 0;

	while (state->threadPlaying)
	{
		// Don't play if player is stopped
		if ((state->threadPlaying) && (!(state->threadPause)))
		{
			ret = Play();
		}

		if ((state->threadPlaying) && (!(state->threadPause)) && (ret))
			m_driver.selected->write();
		uint_least32_t iTimer = m_engine.time();

		if (myPlayList)
		{
			if (myPlayList->NumEntries() > 0 && fadeCountDown == -1)
			{
				if (tuneLength > 0 && iTimer >= (tuneLength & 0xffff) && (!myPlayList->isPending))
				{
					long mm = m_engine.timeMs() % 1000;
					if (mm >= (tuneLength >> 16))
					{
						PlayListEntry* tmp = myPlayList->Cur();
						fadeCountDown = myPlayList->FadeoutTime(tmp);

						if (fadeCountDown > 0)
						{
							fadeSub = (int)(dwMaxWavVol / fadeCountDown / (1000 / 187));
							fadeVol = dwMaxWavVol;
							fadeTime = (int)iTimer;
						}
					}

				}
			}
			else if (myPlayList->NumEntries() > 0 && fadeCountDown > 0)
			{
				unsigned long tmpVol = fadeVol + (fadeVol << 16);
				m_driver.device->setVolume(tmpVol);
				fadeVol -= fadeSub;
				if (fadeVol <= 187 || fadeCountDown == 0)
				{
					if (!myPlayList->isPending)
					{
						PostMessage(hPlayer, WM_COMMAND, MAKELPARAM(IDB_NEXTSID, 0), 0);
						myPlayList->isPending = true;
					}
					fadeCountDown = -1;
				}
				else
				{
					if (fadeTime != (int)iTimer)
					{
						fadeTime = (int)iTimer;
						fadeCountDown--;
					}
				}
			}
		}
		if (!ret)
		{
			PostMessage(hMain, OWM_SETTIMEPLAYED, 0, 0);
			state->threadPlaying = false;
			Stop();
		}
	}

	return 0;
}

void CleanUp()
{
	es.filter = m_filter.enabled;
	es.bias = m_filter.bias;
	es.filterCurve6581 = m_filter.filterCurve6581;
	es.filterCurve8580 = m_filter.filterCurve8580;

	WriteAudioSettings(&as, szIni);
	WriteEmulationSettings(&es, szIni);

	// Emulation-related
	Stop();
	m_driver.selected->close();
	CreateOutput(OUT_NULL);
	CreateSidEmu(EMU_NONE);
	m_engine.load(NULL);
	m_engine.config(m_engCfg);

	// GUI-related
	delete mySTILView;

}

void SetSpeed(int nTimesNormal)
{
	switch (nTimesNormal)
	{
	case 1:
		CheckRadioButton(hPlayer, IDB_PLAYSPEED_X1, IDB_PLAYSPEED_X4, IDB_PLAYSPEED_X1);
		break;
	case 2:
		CheckRadioButton(hPlayer, IDB_PLAYSPEED_X1, IDB_PLAYSPEED_X4, IDB_PLAYSPEED_X2);
		break;
	case 4:
		CheckRadioButton(hPlayer, IDB_PLAYSPEED_X1, IDB_PLAYSPEED_X4, IDB_PLAYSPEED_X4);
		break;
	case 8:
		// TODO, can I check none?
		CheckRadioButton(hPlayer, IDB_PLAYSPEED_X1, IDB_PLAYSPEED_X4, IDB_PLAYSPEED_X4);
		break;
	default:
		return;
	}

	m_engine.fastForward(100 * nTimesNormal);
}

void ChangeMainDialog()
{
	RECT rectChild;
	HWND hwndFocus = GetFocus();
	DestroyWindow(hPlayer);
	hPlayer = CreateDialogParam(hInst,
		MAKEINTRESOURCE(isMusPlayer ? IDD_SIDPLAYER : IDD_MAIN_PLAYER),
		hMain, Player, (LPARAM)mainFont);
	GetWindowRect(hPlayer, &rectChild);
	MoveWindow(hPlayer, 0, 0, rectChild.right - rectChild.left,
		rectChild.bottom - rectChild.top, TRUE);
	SendMessage(hMain, WM_SIZE, SIZE_RESTORED,
		MAKELONG(rectChild.right - rectChild.left, rectChild.bottom - rectChild.top));
	ShowWindow(hPlayer, SW_RESTORE);
	if (hwndFocus)
		SetFocus(hwndFocus);        // Let the DIR UI keep it's focus
	bool rb = CheckRadioButton(hPlayer, IDB_PLAYSPEED_X1, IDB_PLAYSPEED_X4, IDB_PLAYSPEED_X1);
}

void CreateMusInfo(HDC dc)
{
	// Colours by Colodore
	int x, y, basex, basey, dx, dy;
	int r[16] = { 0x00, 0xff, 0x81, 0x75, 0x8e, 0x56, 0x2e, 0xed, 0x8e, 0x55, 0xc4, 0x4a, 0x7b, 0xa9, 0x70, 0xb2 };
	int g[16] = { 0x00, 0xff, 0x33, 0xce, 0x3c, 0xa4, 0x2c, 0xf1, 0x50, 0x38, 0x6c, 0x4a, 0x7b, 0xff, 0x6d, 0xb2 };
	int b[16] = { 0x00, 0xff, 0x38, 0xc8, 0x97, 0x4d, 0x9b, 0x71, 0x29, 0x00, 0x71, 0x4a, 0x7b, 0x9f, 0xeb, 0xb2 };
	int bits[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
	int colour = 15, i;
	bool p, reverse;
	uint8_t c;

	const SidTuneInfo *tuneInfo = m_tune->getInfo();
	int musCommentLength = strlen(tuneInfo->musString()) + 1;
	char* musComment = new char[musCommentLength + 1];
	strcpy_s(musComment, musCommentLength, tuneInfo->musString());
	basex = 16;
	basey = 10;
	i = 0;
	x = basex;
	y = basey;
	reverse = false;

	while (musComment[i])
	{
		p = false;
		c = musComment[i];

		switch (c)
		{
		case 0x90:
			colour = 0;
			break;
		case 0x05:
			colour = 1;
			break;
		case 0x1c:
			colour = 2;
			break;
		case 0x9f:
			colour = 3;
			break;
		case 0x9c:
			colour = 4;
			break;
		case 0x1e:
			colour = 5;
			break;
		case 0x1f:
			colour = 6;
			break;
		case 0x9e:
			colour = 7;
			break;
		case 0x81:
			colour = 8;
			break;
		case 0x95:
			colour = 9;
			break;
		case 0x96:
			colour = 10;
			break;
		case 0x97:
			colour = 11;
			break;
		case 0x98:
			colour = 12;
			break;
		case 0x99:
			colour = 13;
			break;
		case 0x9a:
			colour = 14;
			break;
		case 0x9b:
			colour = 15;
			break;
		case 0x0d:
			if (y < 90)
				basey = y + 16;
			x = basex;
			y = basey;
			reverse = false;
			break;
		case 0x12:
			reverse = true;
			break;
		case 0x92:
			reverse = false;
			break;
		case 0x11:
			if (y < 90)
				y += 16;
			break;
		case 0x91:
			y -= 16;
			break;
		case 0x1a:
			break;
		case 0x1d:
			x += 8;
			break;
		case 0x9d:
			x -= 8;
			break;
		case 0x14:
			x -= 8;
			c = 32;
			{
				int offset = c * 8;
				dy = y;

				for (int k = 0; k < 8; k++)
				{
					uint8_t t = CHARACTERS[offset + k];

					for (int l = 0; l < 8; l++)
					{
						if ((t & bits[l]) > 0)
						{
							SetPixel(dc, x + l, dy, RGB(r[colour], g[colour], b[colour]));
							SetPixel(dc, x + l, dy + 1, RGB(r[colour], g[colour], b[colour]));
						}
						else
						{
							SetPixel(dc, x + l, dy, RGB(r[0], g[0], b[0]));
							SetPixel(dc, x + l, dy + 1, RGB(r[0], g[0], b[0]));
						}
					}
					dy += 2;
				}
			}
			break;

		default:
			p = true;
			break;
		}
		i++;

		if (p)
		{
			if (c > 63 && c < 96)
				c = c & 63;
			if (c > 95 && c < 128)
				c = c & 223;
			if (c > 128)
			{
				c = c & 127;
				c = c | 64;
			}
			if (reverse)
				c = c | 128;

			int offset = c * 8;
			dy = y;

			for (int k = 0; k < 8; k++)
			{
				uint8_t t = CHARACTERS[offset + k];

				for (int l = 0; l < 8; l++)
				{
					if ((t & bits[l]) > 0)
					{
						SetPixel(dc, x + l, dy, RGB(r[colour], g[colour], b[colour]));
						SetPixel(dc, x + l, dy + 1, RGB(r[colour], g[colour], b[colour]));
					}
					else
					{
						SetPixel(dc, x + l, dy, RGB(r[0], g[0], b[0]));
						SetPixel(dc, x + l, dy + 1, RGB(r[0], g[0], b[0]));
					}
				}
				dy += 2;
			}
			x += 8;
			if (x >= 270)
			{
				x = 270;
			}
		}
	}
}

void MakeWavFile(HWND hwnd, AudioSettings aus, SaveWavFile* saveFile)
{
	const int wavbufsize = 16384;
	short* buf;
	short int sbuf[wavbufsize];
	//short int *sbuf = (short int*)malloc(wavbufsize);
	long lBytesToWrite, lTotalBytes;
	UINT uBytesToCalc, ret;
	HFILE hWavFile;

	const char* errormsg = "Error writing file. Disk full?";
	int percentdone = 0;

	if (!m_tune)
	{
		InfoBox(hwnd, "Error", "No file loaded");
		return;
	}

	buf = new short[wavbufsize];

	unsigned long lSpace = GetFreeDiskSpace(tolower(*saveFile->GetFileName()) - 'a');
	if (lSpace < (aus.timetoplay() * aus.avgbytespersec() + sizeof(WAVFILEHEADER)))
	{
		InfoBox(hwnd, "Error", "Not enough free disk space");
		return;
	}

	//InitTune(selectedSong);

	hWavFile = saveFile->OpenChosen();
	if (hWavFile == NULL)
	{
		InfoBox(hwnd, "Error", "Couldn't open %s.", saveFile->GetFileName());
		return;
	}

	lTotalBytes = aus.timetoplay() * aus.avgbytespersec();
	lBytesToWrite = lTotalBytes;

	// Write WAV header
	WAVFILEHEADER wavFileHdr;
	wavFileHdr.main_chunk[0] = 'R';
	wavFileHdr.main_chunk[1] = 'I';
	wavFileHdr.main_chunk[2] = 'F';
	wavFileHdr.main_chunk[3] = 'F';
	wavFileHdr.length = lBytesToWrite + sizeof(WAVFILEHEADER);
	wavFileHdr.chunk_type[0] = 'W';
	wavFileHdr.chunk_type[1] = 'A';
	wavFileHdr.chunk_type[2] = 'V';
	wavFileHdr.chunk_type[3] = 'E';
	wavFileHdr.sub_chunk[0] = 'f';
	wavFileHdr.sub_chunk[1] = 'm';
	wavFileHdr.sub_chunk[2] = 't';
	wavFileHdr.sub_chunk[3] = ' ';
	wavFileHdr.clength = sizeof(PCMWAVEFORMAT);
	wavFileHdr.fmt = aus.MakePcmWaveFormat();
	wavFileHdr.data_chunk[0] = 'd';
	wavFileHdr.data_chunk[1] = 'a';
	wavFileHdr.data_chunk[2] = 't';
	wavFileHdr.data_chunk[3] = 'a';
	wavFileHdr.data_length = lBytesToWrite;

	// Creates Progress dialog
	Progress* progress = new Progress();
	   
	if (_lwrite(hWavFile, (const char*)& wavFileHdr, sizeof(WAVFILEHEADER)) != sizeof(WAVFILEHEADER))
	{
		InfoBox(hwnd, "Error", errormsg);
		goto wav_end;
	}
	progress->SetPercentage(percentdone);

	// The calculation loop
	while (lBytesToWrite > 0)
	{
		uBytesToCalc = (wavbufsize < lBytesToWrite) ? wavbufsize : lBytesToWrite;
		ret = m_engine.play(buf, uBytesToCalc / 2);
		for (unsigned int i = 0; i < uBytesToCalc; i++)
		{
			sbuf[i] = static_cast<short int> (buf[i]);
		}

		if (_lwrite(hWavFile, (const char*)sbuf, uBytesToCalc) != uBytesToCalc)
		{
			InfoBox(hwnd, "Error", errormsg);
			goto wav_end;
		}
		lBytesToWrite -= uBytesToCalc;
		int diff = lTotalBytes - lBytesToWrite;
		double per = (double)(diff / lTotalBytes) * 100.0;
		

		percentdone = (int)(((double)lTotalBytes - lBytesToWrite) / (double)lTotalBytes * 100.0);
		progress->SetPercentage(percentdone);
		progress->PumpMessages();
		if (progress->GetStatus() != Progress::PROGRESS_OK)
			break;
	}
wav_end:
	if (progress->GetStatus() == Progress::PROGRESS_TRUNCATE)
	{
		_llseek(hWavFile, 0L, 0);
		wavFileHdr.length = (lTotalBytes - lBytesToWrite) + sizeof(WAVFILEHEADER);
		_lwrite(hWavFile, (const char*)& wavFileHdr, sizeof(WAVFILEHEADER));
	}
	
	_lclose(hWavFile);
	if (progress->GetStatus() == Progress::PROGRESS_DELETE)
		MyDeleteFile(saveFile->GetFileName());

	//free (sbuf);
	delete[] buf;
	delete progress;
	InitTune(selectedSong);
}

// ****************************************************************************
//
// Helper dialogs
//
// ****************************************************************************
INT_PTR CALLBACK FilterDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	static m_filter_t tempfilter;
	static int filterSelectValue6581, filterSelectValue8580;
	static HFONT myFont;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		tempfilter = m_filter;
		myFont = (HFONT)lParam;

		SendDlgItemMessage(hDlg, IDOK, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDCANCEL, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC1, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC2, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC3, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC4, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_EDIT6581, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_EDIT8580, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);

		filterSelectValue6581 = static_cast<int> (round(tempfilter.filterCurve6581 * 100));
		filterSelectValue8580 = static_cast<int> (round(tempfilter.filterCurve8580 * 100));
		SendDlgItemMessage(hDlg, IDC_6581Slider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
		SendDlgItemMessage(hDlg, IDC_6581Slider, TBM_SETPOS, TRUE, (LPARAM)filterSelectValue6581);
		SendDlgItemMessage(hDlg, IDC_6581Slider, TBM_SETTICFREQ, 10, 0);
		SetDlgItemInt(hDlg, IDC_EDIT6581, filterSelectValue6581, FALSE);
		SendDlgItemMessage(hDlg, IDC_EDIT6581, EM_SETLIMITTEXT, 3, 0);
		SendDlgItemMessage(hDlg, IDC_8580Slider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
		SendDlgItemMessage(hDlg, IDC_8580Slider, TBM_SETPOS, TRUE, (LPARAM)filterSelectValue8580);
		SendDlgItemMessage(hDlg, IDC_8580Slider, TBM_SETTICFREQ, 1000, 0);
		SetDlgItemInt(hDlg, IDC_EDIT8580, filterSelectValue8580, FALSE);
		SendDlgItemMessage(hDlg, IDC_EDIT8580, EM_SETLIMITTEXT, 3, 0);
		return (INT_PTR)TRUE;
	}
	case WM_CLOSE:
		EndDialog(hDlg, TRUE);
		break;

	case WM_HSCROLL:
	{
		filterSelectValue6581 = SendDlgItemMessage(hDlg, IDC_6581Slider, TBM_GETPOS, 0, 0);
		SetDlgItemInt(hDlg, IDC_EDIT6581, filterSelectValue6581, FALSE);
		
		filterSelectValue8580 = SendDlgItemMessage(hDlg, IDC_8580Slider, TBM_GETPOS, 0, 0);
		SetDlgItemInt(hDlg, IDC_EDIT8580, filterSelectValue8580, FALSE);
		tempfilter.filterCurve8580 = static_cast<double> (filterSelectValue8580 / 100.00);
		tempfilter.filterCurve6581 = static_cast<double> (filterSelectValue6581 / 100.00);
		
		if (rs)
		{
			rs->filter6581Curve(tempfilter.filterCurve6581);
			rs->filter8580Curve(tempfilter.filterCurve8580);
		}
		
		break;
	}

	case WM_COMMAND:
	{
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDCANCEL:
			if (rs)
			{
				rs->filter6581Curve(m_filter.filterCurve6581);
				rs->filter8580Curve(m_filter.filterCurve8580);
			}
			EndDialog(hDlg, FALSE);
			break;

		case IDOK:
		{
			HWND hwndTest = GetFocus();
			if (hwndTest == GetDlgItem(hDlg, IDC_EDIT6581))
			{
				int val = GetDlgItemInt(hDlg, IDC_EDIT6581, FALSE, false);
				if (val > 100)
				{
					val = 100;
					SetDlgItemInt(hDlg, IDC_EDIT6581, val, FALSE);
				}
				if (val < 0)
				{
					val = 0;
					SetDlgItemInt(hDlg, IDC_EDIT6581, val, FALSE);
				}
				filterSelectValue6581 = val;
				SendDlgItemMessage(hDlg, IDC_6581Slider, TBM_SETPOS, TRUE, (LPARAM)filterSelectValue6581);
				tempfilter.filterCurve6581 = static_cast<double> (filterSelectValue6581 / 100.00);
				if (rs)
					rs->filter6581Curve(tempfilter.filterCurve6581);
				break;
			}
			else if (hwndTest == GetDlgItem(hDlg, IDC_EDIT8580))
			{
				int val = GetDlgItemInt(hDlg, IDC_EDIT8580, FALSE, false);
				if (val > 100)
				{
					val = 100;
					SetDlgItemInt(hDlg, IDC_EDIT8580, val, FALSE);
				}

				if (val < 0)
				{
					val = 0;
					SetDlgItemInt(hDlg, IDC_EDIT8580, val, FALSE);
				}
				filterSelectValue8580 = val;
				SendDlgItemMessage(hDlg, IDC_8580Slider, TBM_SETPOS, TRUE, (LPARAM)filterSelectValue8580);
				tempfilter.filterCurve8580 = static_cast<double> (filterSelectValue8580 / 100.00);
				if (rs)
					rs->filter8580Curve(tempfilter.filterCurve8580);
				break;
			}
			else
			{
				m_filter = tempfilter;
				EndDialog(hDlg, TRUE);
				break;
			}
		}
		default:
			break;
		}
	}

	}
	return 0;
}

bool updateLiveFilter(m_filter_t tempfilter)
{
	if (m_driver.sid == EMU_RESIDFP)
	{
	}

	return true;
}

INT_PTR CALLBACK MixerDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	static uint16_t solo;
	static HFONT myFont;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		myFont = (HFONT)lParam;

		SendDlgItemMessage(hDlg, IDOK, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_SID1, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC1, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V1ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V1MUTE , WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V1SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC2, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V2ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V2MUTE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V2SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC3, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V3ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V3MUTE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V3SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_SID2, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC4, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V4ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V4MUTE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V4SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC5, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V5ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V5MUTE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V5SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC6, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V6ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V6MUTE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V6SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_SID3, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC7, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V7ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V7MUTE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V7SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC8, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V8ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V8MUTE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V8SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC9, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V9ACTIVE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V9MUTE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_V9SOLO, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);

		int x = GetMyProfileInt("WINDOWS", "MixerX", 0, szIni);
		int y = GetMyProfileInt("WINDOWS", "MixerY", 0, szIni);

		WINDOWPLACEMENT wpl;
		wpl.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hDlg, &wpl);
		int sx = wpl.rcNormalPosition.right - wpl.rcNormalPosition.left;
		int sy = wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top;

		MoveWindow(hDlg, x, y, sx, sy, true);

		solo = 0;

		updateMixer(hDlg, solo);
		return (INT_PTR)TRUE;
	}
	case WM_DESTROY:
		WINDOWPLACEMENT wpl;
		wpl.length = sizeof(WINDOWPLACEMENT);
		if (GetWindowPlacement(hDlg, &wpl))
		{
			WriteMyProfileInt("WINDOWS", "MixerX", wpl.rcNormalPosition.left, szIni);
			WriteMyProfileInt("WINDOWS", "MixerY", wpl.rcNormalPosition.top, szIni);
		}
		break;

	case WM_CLOSE:
		CheckMenuItem(GetMenu(hMain), ID_VIEW_MIXER, MF_BYCOMMAND | MF_UNCHECKED);
		EndDialog(hDlg, TRUE);
		break;

	case OWM_MIXERUPDATE:
	{
		updateMixer(hDlg, solo);
		break;
	}

	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDOK:
			CheckMenuItem(GetMenu(hMain), ID_VIEW_MIXER, MF_BYCOMMAND | MF_UNCHECKED);
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		case IDC_V1MUTE:
			v1mute = true;
			solo = 0;
			m_engine.mute(0, 0, v1mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V1ACTIVE:
			v1mute = false;
			solo = 0;
			m_engine.mute(0, 0, v1mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V2MUTE:
			v2mute = true;
			solo = 0;
			m_engine.mute(0, 1, v2mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V2ACTIVE:
			v2mute = false;
			solo = 0;
			m_engine.mute(0, 1, v2mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V3MUTE:
			v3mute = true;
			solo = 0;
			m_engine.mute(0, 2, v3mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V3ACTIVE:
			v3mute = false;
			solo = 0;
			m_engine.mute(0, 2, v3mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V4MUTE:
			v4mute = true;
			solo = 0;
			m_engine.mute(1, 0, v4mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V4ACTIVE:
			v4mute = false;
			solo = 0;
			m_engine.mute(1, 0, v4mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V5MUTE:
			v5mute = true;
			solo = 0;
			m_engine.mute(1, 1, v5mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V5ACTIVE:
			v5mute = false;
			solo = 0;
			m_engine.mute(1, 1, v5mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V6MUTE:
			v6mute = true;
			solo = 0;
			m_engine.mute(1, 2, v6mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V6ACTIVE:
			v6mute = false;
			solo = 0;
			m_engine.mute(1, 2, v6mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V7MUTE:
			v7mute = true;
			solo = 0;
			m_engine.mute(2, 0, v7mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V7ACTIVE:
			v7mute = false;
			solo = 0;
			m_engine.mute(2, 0, v7mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V8MUTE:
			v8mute = true;
			solo = 0;
			m_engine.mute(2, 1, v8mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V8ACTIVE:
			v8mute = false;
			solo = 0;
			m_engine.mute(2, 1, v8mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V9MUTE:
			v9mute = true;
			solo = 0;
			m_engine.mute(2, 2, v9mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V9ACTIVE:
			v9mute = false;
			solo = 0;
			m_engine.mute(2, 2, v9mute);
			updateMixer(hDlg, solo);
			break;

		case IDC_V1SOLO:
			if (!(solo & 0x01))
			{
				v1mute = false;
				v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = true;
				solo = 0x01;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		case IDC_V2SOLO:
			if (!(solo & 0x02))
			{
				v2mute = false;
				v1mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = true;
				solo = 0x02;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		case IDC_V3SOLO:
			if (!(solo & 0x04))
			{
				v3mute = false;
				v1mute = v2mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = true;
				solo = 0x04;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		case IDC_V4SOLO:
			if (!(solo & 0x08))
			{
				v4mute = false;
				v1mute = v2mute = v3mute = v5mute = v6mute = v7mute = v8mute = v9mute = true;
				solo = 0x08;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		case IDC_V5SOLO:
			if (!(solo & 0x10))
			{
				v5mute = false;
				v1mute = v2mute = v3mute = v4mute = v6mute = v7mute = v8mute = v9mute = true;
				solo = 0x10;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		case IDC_V6SOLO:
			if (!(solo & 0x20))
			{
				v6mute = false;
				v1mute = v2mute = v3mute = v4mute = v5mute = v7mute = v8mute = v9mute = true;
				solo = 0x20;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		case IDC_V7SOLO:
			if (!(solo & 0x40))
			{
				v7mute = false;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v8mute = v9mute = true;
				solo = 0x40;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		case IDC_V8SOLO:
			if (!(solo & 0x80))
			{
				v8mute = false;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v9mute = true;
				solo = 0x80;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		case IDC_V9SOLO:
			if (!(solo & 0x100))
			{
				v9mute = false;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = true;
				solo = 0x100;
			}
			else
			{
				solo = 0;
				v1mute = v2mute = v3mute = v4mute = v5mute = v6mute = v7mute = v8mute = v9mute = false;
			}
			updateVoices();
			updateMixer(hDlg, solo);
			break;

		default:
			break;
		}

	default:
		break;
	}
	return 0;
}

void updateMixer(HWND hDlg, uint16_t solo)
{
	SendDlgItemMessage(hDlg, IDC_V1ACTIVE, BM_SETCHECK, (v1mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V1MUTE, BM_SETCHECK, (v1mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V2ACTIVE, BM_SETCHECK, (v2mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V2MUTE, BM_SETCHECK, (v2mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V3ACTIVE, BM_SETCHECK, (v3mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V3MUTE, BM_SETCHECK, (v3mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V4ACTIVE, BM_SETCHECK, (v4mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V4MUTE, BM_SETCHECK, (v4mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V5ACTIVE, BM_SETCHECK, (v5mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V5MUTE, BM_SETCHECK, (v5mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V6ACTIVE, BM_SETCHECK, (v6mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V6MUTE, BM_SETCHECK, (v6mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V7ACTIVE, BM_SETCHECK, (v7mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V7MUTE, BM_SETCHECK, (v7mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V8ACTIVE, BM_SETCHECK, (v8mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V8MUTE, BM_SETCHECK, (v8mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V9ACTIVE, BM_SETCHECK, (v9mute) ? BST_UNCHECKED : BST_CHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V9MUTE, BM_SETCHECK, (v9mute) ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(hDlg, IDC_V1SOLO, BM_SETCHECK, (solo & 0x01) ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V2SOLO, BM_SETCHECK, (solo & 0x02) ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V3SOLO, BM_SETCHECK, (solo & 0x04) ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V4SOLO, BM_SETCHECK, (solo & 0x08) ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V5SOLO, BM_SETCHECK, (solo & 0x10) ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V6SOLO, BM_SETCHECK, (solo & 0x20) ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V7SOLO, BM_SETCHECK, (solo & 0x40) ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V8SOLO, BM_SETCHECK, (solo & 0x80) ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hDlg, IDC_V9SOLO, BM_SETCHECK, (solo & 0x100) ? BST_CHECKED : BST_UNCHECKED, 0);
}

void updateVoices()
{
	m_engine.mute(0, 0, v1mute);
	m_engine.mute(0, 1, v2mute);
	m_engine.mute(0, 2, v3mute);
	m_engine.mute(1, 0, v4mute);
	m_engine.mute(1, 1, v5mute);
	m_engine.mute(1, 2, v6mute);
	m_engine.mute(2, 0, v7mute);
	m_engine.mute(2, 1, v8mute);
	m_engine.mute(2, 2, v9mute);
}

INT_PTR CALLBACK HvscDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char szTemp[MAX_PATH];
	static HFONT myFont;

	switch (message)
	{
	case WM_INITDIALOG:
		myFont = (HFONT)lParam;

		SendDlgItemMessage(hDlg, IDOK, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDCANCEL, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC1, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_HVSCPATH, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_BROWSE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STILGLOBAL, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STILENTRIES, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STILBUGGED, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_CURSUBS, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_ALLSUBS, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC2, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STILAUTOSIZE, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC3, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_STATIC4, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);

		SetDlgItemTextA(hDlg, IDC_HVSCPATH, mySTILView->getHVSCdir());
		CheckDlgButton(hDlg, IDC_STILENTRIES, mySTILView->toshowEntry());
		CheckDlgButton(hDlg, IDC_STILGLOBAL, mySTILView->toshowGlobal());
		CheckDlgButton(hDlg, IDC_STILBUGGED, mySTILView->toshowBug());
		CheckRadioButton(hDlg, IDC_CURSUBS, IDC_ALLSUBS, mySTILView->bAllSubtunes ? IDC_ALLSUBS : IDC_CURSUBS);
		SetDlgItemInt(hDlg, IDC_STILMAXHEIGHT, mySTILView->getMaxAutoHeight(), TRUE);
		CheckDlgButton(hDlg, IDC_STILAUTOSIZE, mySTILView->bAutoSize);
		EnableDlgItem(hDlg, IDC_STILMAXHEIGHT, IsDlgButtonChecked(hDlg, IDC_STILAUTOSIZE) == BST_CHECKED);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BROWSE:
			BROWSEINFOA brInfo;
			LPITEMIDLIST pidl;
			brInfo.hwndOwner = hDlg;
			brInfo.pidlRoot = NULL;
			brInfo.pszDisplayName = szTemp;
			brInfo.lpszTitle = "Select HVSC base directory";
			brInfo.ulFlags = BIF_RETURNONLYFSDIRS;
			brInfo.lpfn = NULL;
			pidl = SHBrowseForFolderA(&brInfo);
			if (pidl != NULL)
			{
				SHGetPathFromIDListA(pidl, szTemp);
				CoTaskMemFree(pidl);
				SetDlgItemTextA(hDlg, IDC_HVSCPATH, szTemp);
			}
			return TRUE;

		case IDOK:
			GetDlgItemTextA(hDlg, IDC_HVSCPATH, szTemp, sizeof(szTemp) - 1);
			mySTILView->setHVSCdir(szTemp);
			mySTILView->setVisibility(
				IsDlgButtonChecked(hDlg, IDC_STILENTRIES) == BST_CHECKED,
				IsDlgButtonChecked(hDlg, IDC_STILBUGGED) == BST_CHECKED,
				IsDlgButtonChecked(hDlg, IDC_STILGLOBAL) == BST_CHECKED
			);
			mySTILView->bAllSubtunes = IsDlgButtonChecked(hDlg, IDC_ALLSUBS) == BST_CHECKED;
			mySTILView->setMaxAutoHeight(GetDlgItemInt(hDlg, IDC_STILMAXHEIGHT, NULL, FALSE));
			mySTILView->bAutoSize = IsDlgButtonChecked(hDlg, IDC_STILAUTOSIZE) == BST_CHECKED;

			EndDialog(hDlg, TRUE);

			// Update STILView to reflect the changes (if we have got the correct path to HVSC)
			mySTILView->showEntry(infilename, selectedSong);

			// Update Song length path accordingly
			mySLDB->SetPath(szTemp);

			// Update song length of current/playing song
			PostMessage(hMain, OWM_UPDATESL, 0, 0);

			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK AudioOutDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int dx = 0;
	static int height = 128;
	static short *buffer;
	static HANDLE hDisplayTimer;
	static DWORD dwThreadId2;

	switch (message)
	{
	case WM_INITDIALOG:
		dx = 0;
		break;

	case WM_PAINT:
		if (buffer)
		{
			int l = m_driver.cfg.bufSize;


		}


	}
		
	return FALSE;
}


INT_PTR CALLBACK EmuDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HFONT myFont;
	static bool changes;
	static emulation_section es_temp;

	switch (message)
	{
	case WM_INITDIALOG:
		myFont = (HFONT)lParam;

		SendDlgItemMessage(hDlg, IDOK, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDCANCEL, WM_SETFONT, (WPARAM)myFont, (LPARAM)true);
		SendDlgItemMessage(hDlg, IDC_GRPCLOCK, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_PAL, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_NTSC, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_OLD_NTSC, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_DREAN, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_PAL_M, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_CLOCK_FORCE, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_SIDGRP, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_STATIC, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_SID1_6581, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_SID1_8580, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_FORCE_SID, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_DIGIBOOST, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_SAMPLING_METHOD, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_INTERPOLATE, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_RESAMPLE_INTERPOLATE, WM_SETFONT, (WPARAM)myFont, true);

		CheckRadioButton(hDlg, IDC_PAL, IDC_PAL_M, es.c64modelDefault + IDC_PAL);
		//CheckDlgButton(hDlg, IDC_PAL, es.c64modelDefault == SidConfig::c64_model_t::PAL ? BST_CHECKED : BST_UNCHECKED);
		//CheckDlgButton(hDlg, IDC_NTSC, es.c64modelDefault == SidConfig::c64_model_t::NTSC ? BST_CHECKED : BST_UNCHECKED);
		//CheckDlgButton(hDlg, IDC_OLD_NTSC, es.c64modelDefault == SidConfig::c64_model_t::OLD_NTSC ? BST_CHECKED : BST_UNCHECKED);
		//CheckDlgButton(hDlg, IDC_DREAN, es.c64modelDefault == SidConfig::c64_model_t::DREAN ? BST_CHECKED : BST_UNCHECKED);
		//CheckDlgButton(hDlg, IDC_PAL_M, es.c64modelDefault == SidConfig::c64_model_t::PAL_M ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_CLOCK_FORCE, es.c64modelForced);
		CheckRadioButton(hDlg, IDC_SID1_6581, IDC_SID1_8580, es.sidModel == SidConfig::sid_model_t::MOS6581 ? IDC_SID1_6581 : IDC_SID1_8580);
		//CheckDlgButton(hDlg, IDC_SID1_6581, es.sidModel == SidConfig::sid_model_t::MOS6581 ? BST_CHECKED : BST_UNCHECKED);
		//CheckDlgButton(hDlg, IDC_SID1_8580, es.sidModel == SidConfig::sid_model_t::MOS8580 ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_FORCE_SID, es.sidModelForced);
		CheckDlgButton(hDlg, IDC_DIGIBOOST, es.digiBoost);
		
		CheckRadioButton(hDlg, IDC_INTERPOLATE, IDC_RESAMPLE_INTERPOLATE, es.samplingMethod == SidConfig::sampling_method_t::INTERPOLATE ? IDC_INTERPOLATE : IDC_RESAMPLE_INTERPOLATE);
		//CheckDlgButton(hDlg, IDC_INTERPOLATE, es.samplingMethod == SidConfig::sampling_method_t::INTERPOLATE ? BST_CHECKED : BST_UNCHECKED);
		//CheckDlgButton(hDlg, IDC_RESAMPLE_INTERPOLATE, es.samplingMethod == SidConfig::sampling_method_t::RESAMPLE_INTERPOLATE ? BST_CHECKED : BST_UNCHECKED);
		changes = false;
		es_temp = es;
		return TRUE;
		
	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;

		case IDOK:
			if (IsDlgButtonChecked(hDlg, IDC_PAL))
				es.c64modelDefault = SidConfig::c64_model_t::PAL;
			if (IsDlgButtonChecked(hDlg, IDC_NTSC))
				es.c64modelDefault = SidConfig::c64_model_t::NTSC;
			if (IsDlgButtonChecked(hDlg, IDC_OLD_NTSC))
				es.c64modelDefault = SidConfig::c64_model_t::OLD_NTSC;
			if (IsDlgButtonChecked(hDlg, IDC_DREAN))
				es.c64modelDefault = SidConfig::c64_model_t::DREAN;
			if (IsDlgButtonChecked(hDlg, IDC_PAL_M))
				es.c64modelDefault = SidConfig::c64_model_t::PAL_M;
			es.c64modelForced = IsDlgButtonChecked(hDlg, IDC_CLOCK_FORCE) == BST_CHECKED;
			if (IsDlgButtonChecked(hDlg, IDC_SID1_6581))
				es.sidModel = SidConfig::sid_model_t::MOS6581;
			if (IsDlgButtonChecked(hDlg, IDC_SID1_8580))
				es.sidModel = SidConfig::sid_model_t::MOS8580;
			es.sidModelForced = IsDlgButtonChecked(hDlg, IDC_FORCE_SID) == BST_CHECKED;
			es.digiBoost = IsDlgButtonChecked(hDlg, IDC_DIGIBOOST) == BST_CHECKED;
			if (IsDlgButtonChecked(hDlg, IDC_INTERPOLATE))
				es.samplingMethod = SidConfig::sampling_method_t::INTERPOLATE;
			if (IsDlgButtonChecked(hDlg, IDC_RESAMPLE_INTERPOLATE))
				es.samplingMethod = SidConfig::sampling_method_t::RESAMPLE_INTERPOLATE;

			if (es_temp.c64modelDefault != es.c64modelDefault || es_temp.c64modelForced != es.c64modelForced)
				changes = true;
			if (es_temp.sidModel != es.sidModel || es_temp.sidModelForced != es.sidModelForced || es_temp.digiBoost != es.digiBoost)
				changes = true;
			if (es_temp.samplingMethod != es.samplingMethod)
				changes = true;

			if (changes)
			{
				PostMessage(hMain, OWM_UPDATE_ES, 0, 0);
			}
			EndDialog(hDlg, TRUE);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK SettingsDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HFONT myFont;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		myFont = (HFONT)lParam;

		SendDlgItemMessage(hDlg, IDOK, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDCANCEL, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_GROUPBOX1, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_GROUPBOX2, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_GROUPBOX3, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_MONO, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_STEREO, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_F11025, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_F22050, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_F44100, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_F48000, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_8BITS, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_16BITS, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_NBUFCOMBO, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_CBMILLIS, WM_SETFONT, (WPARAM)myFont, true);
		//SendDlgItemMessage(hDlg, IDC_BUFTXT, WM_SETFONT, (WPARAM)myFont, true);

		CheckRadioButton(hDlg, IDC_MONO, IDC_STEREO, as.playback == 1 ? IDC_MONO : IDC_STEREO);

		CheckDlgButton(hDlg, IDC_CBMILLIS, showMillis ? BST_CHECKED : BST_UNCHECKED);

		switch (as.frequency)
		{
		case 11025:
			CheckRadioButton(hDlg, IDC_F11025, IDC_F48000, IDC_F11025);
			break;
		case 22050:
			CheckRadioButton(hDlg, IDC_F11025, IDC_F48000, IDC_F22050);
			break;
		case 44100:
			CheckRadioButton(hDlg, IDC_F11025, IDC_F48000, IDC_F44100);
			break;
		case 48000:
			CheckRadioButton(hDlg, IDC_F11025, IDC_F48000, IDC_F48000);
			break;
		default:
			CheckRadioButton(hDlg, IDC_F11025, IDC_F48000, IDC_F44100);
			break;
		}

		return TRUE;
	}
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;

		case IDOK:
		{
			bool newMillis;
			as.playback = IsDlgButtonChecked(hDlg, IDC_MONO) ? SidConfig::playback_t::MONO : SidConfig::playback_t::STEREO;
			as.precision = 16;

			if (IsDlgButtonChecked(hDlg, IDC_F11025) == BST_CHECKED)
				as.frequency = 11025;
			if (IsDlgButtonChecked(hDlg, IDC_F22050) == BST_CHECKED)
				as.frequency = 22050;
			if (IsDlgButtonChecked(hDlg, IDC_F44100) == BST_CHECKED)
				as.frequency = 44100;
			if (IsDlgButtonChecked(hDlg, IDC_F48000) == BST_CHECKED)
				as.frequency = 48000;

			if (IsDlgButtonChecked(hDlg, IDC_CBMILLIS) == BST_CHECKED)
				newMillis = true;
			else
				newMillis = false;

			if (showMillis != newMillis)
			{
				showMillis = newMillis;
				PostMessage(hMain, OWM_UPDATESL, 0, 0);
			}
			EndDialog(hDlg, FALSE);

			return TRUE;
		}
		}
		break;
	}
	
	return FALSE;
}

INT_PTR CALLBACK DirsettingsDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static DWORD dirSettings;
	static HFONT myFont;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		int id;
		myFont = (HFONT)lParam;
		dirSettings = GetMyProfileLong("DIR LIST", "Settings", DIR_DIFIDR, szIni);

		CheckRadioButton(hDlg, IDRB_DIRSHORT, IDRB_DIRLONG, (dirSettings & DIR_SHORTNAMES) ? IDRB_DIRSHORT : IDRB_DIRLONG);
		CheckRadioButton(hDlg, IDRB_LOWERCASE, IDRB_MIXEDCASE, (dirSettings & DIR_LOWERCASE) ? IDRB_LOWERCASE : ((dirSettings & DIR_UPPERCASE) ? IDRB_UPPERCASE : IDRB_MIXEDCASE));

		id = IDRB_FIDIDR;

		if ((dirSettings & 127) == DIR_FIDRDI)
			id = IDRB_FIDRDI;
		else if ((dirSettings & 127) == DIR_DIFIDR)
			id = IDRB_DIFIDR;
		else if ((dirSettings & 127) == DIR_DIDRFI)
			id = IDRB_DIDRFI;
		else if ((dirSettings & 127) == DIR_DRFIDI)
			id = IDRB_DRFIDI;
		else if ((dirSettings & 127) == DIR_DRDIFI)
			id = IDRB_DRDIFI;

		CheckRadioButton(hDlg, IDRB_FIDIDR, IDRB_DRDIFI, id);

		CheckRadioButton(hDlg, IDRB_SINGLECOL, IDRB_MULTICOL, (dirSettings & DIR_SINGLECOL) ? IDRB_SINGLECOL : IDRB_MULTICOL);

		SendDlgItemMessage(hDlg, IDC_GROUPBOX8, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_DIRSHORT, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_DIRLONG, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_GROUPBOX9, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_LOWERCASE, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_UPPERCASE, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_MIXEDCASE, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_GROUPBOX10, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_FIDIDR, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_FIDRDI, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_DIFIDR, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_DIDRFI, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_DRFIDI, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_DRDIFI, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_GROUPBOX11, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_SINGLECOL, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDRB_MULTICOL, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDOK, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDCANCEL, WM_SETFONT, (WPARAM)myFont, true);
		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			DWORD newSettings = 0;

			if (IsDlgButtonChecked(hDlg, IDRB_DIRSHORT) == BST_CHECKED)
				newSettings |= DIR_SHORTNAMES;

			if (IsDlgButtonChecked(hDlg, IDRB_LOWERCASE) == BST_CHECKED)
				newSettings |= DIR_LOWERCASE;
			else if (IsDlgButtonChecked(hDlg, IDRB_UPPERCASE) == BST_CHECKED)
				newSettings |= DIR_UPPERCASE;

			if (IsDlgButtonChecked(hDlg, IDRB_FIDIDR) == BST_CHECKED)
				newSettings |= DIR_FIDIDR;
			else if (IsDlgButtonChecked(hDlg, IDRB_FIDRDI) == BST_CHECKED)
				newSettings |= DIR_FIDRDI;
			else if (IsDlgButtonChecked(hDlg, IDRB_DIFIDR) == BST_CHECKED)
				newSettings |= DIR_DIFIDR;
			else if (IsDlgButtonChecked(hDlg, IDRB_DIDRFI) == BST_CHECKED)
				newSettings |= DIR_DIDRFI;
			else if (IsDlgButtonChecked(hDlg, IDRB_DRFIDI) == BST_CHECKED)
				newSettings |= DIR_DRFIDI;
			else if (IsDlgButtonChecked(hDlg, IDRB_DRDIFI) == BST_CHECKED)
				newSettings |= DIR_DRDIFI;

			if (IsDlgButtonChecked(hDlg, IDRB_SINGLECOL) == BST_CHECKED)
				newSettings |= DIR_SINGLECOL;

			if (newSettings != dirSettings)
			{
				WriteMyProfileLong("DIR LIST", "Settings", newSettings, szIni);
				EndDialog(hDlg, TRUE);
			}
			else
				EndDialog(hDlg, FALSE);

			return TRUE;
		}
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK CpuDebugDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HFONT myFont;

	char szTmp[256];
	static int iDur = 0;
	static int startMs = 0;
	static int powerOnDelaySticky = 0;
	static unsigned int powerOnDelay = 0;
	static unsigned int enginePowerOnDelay = 0;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		cpudebug_param* cp = (cpudebug_param*)lParam;

		myFont = cp->font;
		SidTune* sidTune = cp->tune;

		SendDlgItemMessage(hDlg, -1, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_DEBUG_DELAY, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_DEBUG_DURATION, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_DEBUG_FILE, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_DEBUG_START_TIME, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_DEBUG_SUBTUNE, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDC_DELAY_KEEP, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDOK, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDCANCEL, WM_SETFONT, (WPARAM)myFont, true);
		SendDlgItemMessage(hDlg, IDD_CPUDEBUG, WM_SETFONT, (WPARAM)myFont, true);

		// Fill start time with 0:00
		sprintf_s(szTmp, sizeof(szTmp), "%d:%.02d.%.03d", startMs / 60000, startMs / 1000, startMs % 1000);
		SetDlgItemTextA(hDlg, IDC_DEBUG_START_TIME, szTmp);

		// FIll duration combo box
		int durCount = size(defaultStartTimeStrings);
		for (int i = 0; i < durCount; i++)
			SendDlgItemMessageA(hDlg, IDC_DEBUG_DURATION, CB_ADDSTRING, 0, (LPARAM)defaultStartTimeStrings[i].c_str());

		// Select the first line
		SendDlgItemMessage(hDlg, IDC_DEBUG_DURATION, CB_SETCURSEL, iDur, 0);

		// Fill power-on delay
		enginePowerOnDelay = m_engine.config().powerOnDelay;
		if (!powerOnDelaySticky)
			powerOnDelay = enginePowerOnDelay;
		SetDlgItemInt(hDlg, IDC_DEBUG_DELAY, powerOnDelay, TRUE);
		CheckDlgButton(hDlg, IDC_DELAY_KEEP, powerOnDelaySticky ? BST_CHECKED : BST_UNCHECKED);

		// Fill file and sidtune
		if (sidTune != NULL)
		{
			const SidTuneInfo* tuneInfo = sidTune->getInfo();
			sprintf_s(szTmp, 256, "%s%s", tuneInfo->path(), tuneInfo->dataFileName());
			SetDlgItemTextA(hDlg, IDC_DEBUG_FILE, szTmp);
			SetDlgItemInt(hDlg, IDC_DEBUG_SUBTUNE, tuneInfo->currentSong(), FALSE);
		}

		// Enable OK if we have a file loaded
		EnableWindow(GetDlgItem(hDlg, IDOK), sidTune != NULL);

		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			char debugFile[MAX_PATH];
			OwnSaveFile save(hDlg);

			// Get starting time and duration (in milliseconds)
			startMs = GetDlgItemTime(hDlg, IDC_DEBUG_START_TIME);
			iDur = SendDlgItemMessage(hDlg, IDC_DEBUG_DURATION, CB_GETCURSEL, 0, 0);
			if (iDur == CB_ERR)
			{
				iDur = 0;
				return FALSE;
			}
			int durMillis = defaultStartTimesMillis[iDur];
			BOOL DELAYOK;
			unsigned int newPowerOnDelay = GetDlgItemInt(hDlg, IDC_DEBUG_DELAY, &DELAYOK, TRUE);
			if (DELAYOK == FALSE)
				powerOnDelay = enginePowerOnDelay;
			else
				powerOnDelay = newPowerOnDelay;
			powerOnDelaySticky = (IsDlgButtonChecked(hDlg, IDC_DELAY_KEEP) == BST_CHECKED);
			GetDlgItemTextA(hDlg, IDC_DEBUG_FILE, szTmp, sizeof(szTmp));
			int subtune = GetDlgItemInt(hDlg, IDC_DEBUG_SUBTUNE, NULL, FALSE);

			GetPrivateProfileStringA("CPUDEBUG", "Output", "C:\\cpudebug.txt", debugFile, sizeof(debugFile), szIni);
			save.SetFileName(debugFile);

			if (save.Choose())
			{
				cpuDebug.Set(string(szTmp), string(save.GetFileName()), subtune, startMs, durMillis, powerOnDelay);
				cpuDebug.Run();
			}
			EndDialog(hDlg, TRUE);
			return TRUE;
		}

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		break;
	}
	return FALSE;
}
LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		POINT pt;
		
		GetCursorPos(&pt);
		HWND hwnd = WindowFromPoint(pt);
		int retrycount = 0;

		if (LOWORD(wParam) == VK_ESCAPE)
		{
			while (retrycount++ < 2)
			{
				if (hwnd == hProperties)
				{
					SendMessage(hMain, WM_COMMAND, MAKEWPARAM(IDM_VIEW_PROPERTIES,0), 0L);
					return 0;
				}
				if (hwnd == hDirWnd)
				{
					SendMessage(hMain, WM_COMMAND, MAKEWPARAM(IDM_VIEWDIRUI, 0), 0L);
					return 0;
				}
				if (hwnd == hMixerDlg)
				{
					SendMessage(hMain, WM_COMMAND, MAKEWPARAM(ID_VIEW_MIXER, 0), 0L);
					return 0;
				}

				hwnd = GetParent(hwnd);
			}
		}
	}


	return CallNextHookEx(NULL, nCode, wParam, lParam);
}