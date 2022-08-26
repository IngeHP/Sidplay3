/*
 * ofile.cpp - An interface simplifying opening and saving files
 * Customized for use with Sidplay
 * Original code by Adam Lorentzon
 * 
 * Rewritten to use ANSI types for use in newer Windows version
 * and to use the safe version of stringcopy-functions
 * Inge HP 2014
 */

#include "stdafx.h"
#include "ofile.h"
#include <string.h>
#include <stdio.h>

// ----------------------------------------------------------------------------
// OwnFile class

OwnFile::OwnFile (HWND hwnd)
{
	GetWindowsDirectoryA (szDirName, sizeof(szDirName));
	szFile[0] = '\0';
	szFileTitle[0] = '\0';
	szWindowTitle[0] = '\0';
	szFilter = NULL;
	szFileExtension = NULL;

	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = NULL;    // Ignored unless OFN_ENABLETEMPLATE* is set in Flags
	ofn.lpstrFilter = szFilter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile= szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;    // Is current dir if szDirName == NULL
	ofn.lpstrTitle = szWindowTitle;			// Is open if lpstr == NULL
	ofn.lpstrDefExt = szFileExtension;
	ofn.Flags = 0;

	ofn.nFileExtension = 0;
	ofn.nFileOffset = NULL;
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
}

void
OwnFile::SetFileName( const char *szFilename )
{
	strncpy_s(szFile, sizeof(szFile), szFilename, sizeof(szFile));
	SetDirAndFile();
}

void
OwnFile::SetDirAndFile ()
{
	char *cp;

	strcpy_s (szDirName, szFile);		// Assumes equal size
	if ((cp = strrchr (szDirName, '\\')) != NULL)
	{
		strcpy_s (szFileTitle, cp+1);
		if (*(cp-1) == ':')					// Keep backslash for C:\, D:\, etc.
			++cp;
		*cp = '\0';
	}
}

void
OwnFile::ShortenFilename ()
{
	char *cp;

	// If a full filename is set and its directory matches the szDirName one,
	// leave only the filetitle, because it looks better.
	if ((cp = strrchr (szFile, '\\')) != NULL)
	{
		if (!strncmp (szFile, szDirName, cp-szFile))
		{
//			strcpy (szFile, cp+1);	// Don't know if this is guaranteed to work.
			strcpy_s (szFileTitle, cp+1);
			strcpy_s (szFile, szFileTitle);
		}
	}
}

void
OwnFile::SetFilter(const char *szNewFilter )
{
	char *cp;

	delete[] szFilter;
	szFilter = _strdup( szNewFilter );
	nFilters = 0;
	for( cp = szFilter; *cp != '\0'; ++cp )
		if( *cp == '|' )
		{
			*cp = '\0';
			nFilters++;
		}

	nFilters /= 2;

	ofn.lpstrFilter = szFilter;
	SetFilterIndex( ofn.nFilterIndex );
}

// NOTE: The filter index starts at 1 !
void
OwnFile::SetFilterIndex (int nIndex)
{
	// the nFilters > 0 check is to be able to set the initial
	// selection (from last run) before the filter has been set
	if (nIndex < 1 || (nFilters > 0 && nIndex > nFilters))
		ofn.nFilterIndex = 0;
	else
		ofn.nFilterIndex = nIndex;
}

void
OwnFile::SetFileExtension(const char *szExt )
{
	delete[] szFileExtension;
	szFileExtension = _strdup( szExt );
	ofn.lpstrDefExt = szFileExtension;
}


OwnFile::~OwnFile ()
{
	delete[] szFilter;
	delete[] szFileExtension;
}


//-----------------------------------------------------------------------------
// OwnOpenFile Class

OwnOpenFile::OwnOpenFile (HWND hwnd)
	: OwnFile(hwnd)
{
	ofn.Flags = OFN_FILEMUSTEXIST;
}

HFILE
OwnOpenFile::Open (char *szTitle)
{
	HFILE hf;

	if (szTitle)
		strncpy_s (szWindowTitle, szTitle, sizeof(szWindowTitle));

	ShortenFilename ();

	if (GetOpenFileNameA(&ofn))
	{
		SetDirAndFile();
		hf = _lopen (szFile, OF_READ);
		return (hf == HFILE_ERROR) ? NULL : hf;
	}
	else
		return NULL;
}

char *
OwnOpenFile::Choose (const char *szTitle)
{
	if (szTitle)
		strncpy_s (szWindowTitle, szTitle, 100);

	ShortenFilename ();

	if (GetOpenFileNameA(&ofn))
	{
		SetDirAndFile();
		return szFile;
	}
	else
		return NULL;
}

HFILE
OwnOpenFile::OpenChosen ()
{
	HFILE hf;

	hf = _lopen(szFile, OF_READ);
	return (hf == HFILE_ERROR) ? NULL : hf;
}



//-----------------------------------------------------------------------------
// OwnSaveFile Class

OwnSaveFile::OwnSaveFile (HWND hwnd)
	: OwnFile(hwnd)
{
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
}

HFILE
OwnSaveFile::Open (char *szTitle)
{
	HFILE hf;

	if (szTitle)
		strncpy_s (szWindowTitle, szTitle, sizeof(szWindowTitle));

	ShortenFilename ();

	if (GetSaveFileNameA(&ofn))
	{
		SetDirAndFile();
		hf = _lcreat (szFile, 0);
		return (hf == HFILE_ERROR) ? NULL : hf;
	}
	else
		return NULL;
}

char *
OwnSaveFile::Choose (const char *szTitle)
{
	if (szTitle)
		strncpy_s (szWindowTitle, szTitle, 100);

	ShortenFilename ();

	if (GetSaveFileNameA(&ofn))
	{
		SetDirAndFile();
		return szFile;
	}
	else
		return NULL;
}

HFILE
OwnSaveFile::OpenChosen ()
{
	HFILE hf;

	hf = _lcreat (szFile, 0);
	return (hf == HFILE_ERROR) ? NULL : hf;
}

void
OwnSaveFile::SetDirAndFile ()
{
	OwnFile::SetDirAndFile ();

	if (strrchr (szFile, '.') == NULL)
	{
		// If no file extension was typed in - use default
		char *extp = szFilter;
		for (int i = 0; i < 2 * ((int)ofn.nFilterIndex-1) + 1 ; ++i)
		{
			if (!*extp)
				break;
			while (*extp)
				extp++;
			extp++;
		}
		// expt not points to the right filter e.g. *.sid, or NULL
		if (*extp && (extp = strrchr (extp, '.')) != NULL)
			strcat_s (szFile, extp);
	}
}


//-----------------------------------------------------------------------------
// SaveWavFile Class

SaveWavFile::SaveWavFile (HWND hwnd)
	: OwnSaveFile(hwnd)
{
	SetFilter ("Windows PCM (*.WAV)|*.wav|All files (*.*)|*.*|");
	SetFileExtension ("WAV");
}

void
SaveWavFile::PresetFileTitle (char *szSidFile)
{
	char *cp;

	if ((cp = strrchr (szSidFile, '\\')) != NULL)
		cp++;
	else
		cp = szSidFile;
	strcpy_s (szFile, sizeof(szFile), cp);

	if ((cp = strrchr(szFile, '.')) != NULL)
		strcpy_s(cp, 5, ".wav");
	else
		strcat_s (szFile, ".wav");
}

//-----------------------------------------------------------------------------
// SaveListFile Class

SaveListFile::SaveListFile (HWND hwnd)
	: OwnSaveFile(hwnd)
{
	SetFilter ("SIDPLAY PlayList (*.SPL)|*.spl|All files (*.*)|*.*|");
	SetFileExtension ("spl");
}

void
SaveListFile::PresetFileTitle (char *szSidFile)
{
	char *cp;

	if ((cp = strrchr (szSidFile, '\\')) != NULL)
		cp++;
	else
		cp = szSidFile;
	strcpy_s (szFile, cp);

	if ((cp = strrchr (szFile, '.')) != NULL)
		strcpy_s (cp, sizeof(cp), ".spl");
	else
		strcat_s (szFile, ".spl");
}


//-----------------------------------------------------------------------------
// OpenListFile Class

OpenListFile::OpenListFile (HWND hwnd)
: OwnOpenFile(hwnd)
{
}

char *
OpenListFile::Choose (const FileExtensions& extensions, const char *szTitle)
{
	char szNewFilter[100];
	sprintf_s (szNewFilter, "Playlists (*.%s)|*.%s|",
				extensions.GetExtension(TYPE_LIST),
				extensions.GetExtension(TYPE_LIST));
	SetFilter (szNewFilter);
	return OwnOpenFile::Choose (szTitle);
}


//-----------------------------------------------------------------------------
// SaveAsFile Class

SaveAsFile::SaveAsFile (HWND hwnd)
	: OwnSaveFile(hwnd)
{
	// We do our own checking after adding the correct extension.
	ofn.Flags &= ~OFN_OVERWRITEPROMPT;
}

void
SaveAsFile::PresetFileTitle (char *szSidFile)
{
	char *cp;

	if ((cp = strrchr (szSidFile, '\\')) != NULL)
		cp++;
	else
		cp = szSidFile;
	strcpy_s (szFile, cp);

	// Remove extension
	char *tmp;
	if ((tmp = strrchr (szFile, '.')) != NULL)
		*tmp = '\0';
}

int
SaveAsFile::GetFileType ()
{
	if (bMusFilter)
	{
		if (ofn.nFilterIndex == 1)
			return TYPE_MUS;
		else
			return TYPE_WAV;
	}
	else
		return TYPE_PSID + ofn.nFilterIndex - 1;
}

void
SaveAsFile::SetPSIDFilter (const FileExtensions& extensions)
{
	char szNewFilter[128];
	sprintf_s (szNewFilter, "PSID file (*.%s)|*.%s|SID info (*.%s)|*.%s|"
						 "C64 data (*.%s)|*.%s|Windows PCM (*.wav)|*.wav|",
		 extensions.GetExtension(TYPE_PSID),extensions.GetExtension(TYPE_PSID),
		 extensions.GetExtension(TYPE_INFO),extensions.GetExtension(TYPE_INFO),
		 extensions.GetExtension(TYPE_DATA),extensions.GetExtension(TYPE_DATA));
	SetFilter (szNewFilter);
	bMusFilter = FALSE;
}

void
SaveAsFile::SetMUSFilter (const FileExtensions& extensions)
{
	char szNewFilter[64];
	sprintf_s (szNewFilter, "Sidplayer file (*.%s)|*.%s|Windows PCM (*.wav)|*.wav|",
		 extensions.GetExtension(TYPE_MUS),extensions.GetExtension(TYPE_MUS));
	SetFilter (szNewFilter);
	bMusFilter = TRUE;
}

char *
SaveAsFile::Choose (const FileExtensions& extensions,
										char *szTitle, BOOL bSidplayer)
{
	if (bSidplayer)
		SetMUSFilter(extensions);
	else
		SetPSIDFilter(extensions);
	return OwnSaveFile::Choose (szTitle);
}

//-----------------------------------------------------------------------------
// OpenSidFile Class

OpenSidFile::OpenSidFile (HWND hwnd)
: OwnOpenFile(hwnd)
{
}

char *
OpenSidFile::Choose (const FileExtensions& extensions, const char *szTitle)
{
	char szNewFilter[512];
	sprintf_s (szNewFilter, "PSID files (*.%s)|*.%s|Sidplayer files (*.%s;*.str)|*.%s;*.str|"
		                  "C64 executables (*.c64;*.p00;*.prg)|*.c64;*.p00;*.prg|"
	                      "SIDPLAY playlists (*.%s)|*.%s|All files (*.*)|*.*|"
						  ,
		 extensions.GetExtension(TYPE_PSID),extensions.GetExtension(TYPE_PSID),
		 extensions.GetExtension(TYPE_MUS), extensions.GetExtension(TYPE_MUS),
		 extensions.GetExtension(TYPE_LIST),extensions.GetExtension(TYPE_LIST));
	SetFilter (szNewFilter);
	return OwnOpenFile::Choose (szTitle);
}



//-----------------------------------------------------------------------------
// FileExtensions Class

static const char *_defextensions[NUM_TYPES+1] = {"sid", "sid", "dat", "wav", "mus", "inf", "spl", ""};
static const char *_iniStrings[NUM_TYPES] = {"PlaySID", "SID_Info", "C64_Data", "Wave", "Sidplayer", "Amiga_WB_info", "Playlist"};
static const char *_backupextensions[] = {".sid", ".dat", ".inf", "", ".c64", ".prg", ".str", ".mus", 0};

FileExtensions::FileExtensions()
{
	for (int i = 0; i < NUM_TYPES; ++i)
	{
		_fullextensions[i] = new char[strlen(_defextensions[i])+2];
      *_fullextensions[i] = '.';
      _extensions[i] = _fullextensions[i] + 1;
		strcpy_s(_extensions[i], sizeof(_extensions[i]), _defextensions[i]);
	}
   // The last one is the empty string, for both full and ordinary
   // Point to constant string ""
	_fullextensions[NUM_TYPES] = (char*) "";
   _extensions[NUM_TYPES] = _fullextensions[NUM_TYPES];
}

FileExtensions::~FileExtensions()
{

}

char *
FileExtensions::GetExtension (int type) const
{
	if (type >= 0 && type < NUM_TYPES)
		return _extensions[type];
	else
		return _extensions[NUM_TYPES];
}

void
FileExtensions::SetExtension (int type, char *szExt)
{
	if (type >= 0 && type < NUM_TYPES)
	{
		delete[] _fullextensions[type];
		_fullextensions[type] = new char[strlen(szExt)+2];
      *_fullextensions[type] = '.';
      _extensions[type] = _fullextensions[type] + 1;
		strcpy_s (_extensions[type], sizeof(_extensions[type]), szExt);
	}
}

void
FileExtensions::AppendExtension (char *szFile, int type)
{
	char *cp;

	// Make szFile point out the filename without path
	if ((cp = strrchr (szFile, '\\')) != NULL)
		szFile = cp + 1;

	// If there's no dot in the filename, add one before extension is added!
	// unless the extension is empty TODO
	if ((cp = strrchr (szFile, '.')) == NULL)
	{
		strcat_s (szFile, sizeof(szFile), ".");
		cp = szFile + strlen(szFile) - 1;
	}
	++cp;

	// Add the correct extension, cp now points after the dot
	if (type < 0 && type >= NUM_TYPES)
		type = NUM_TYPES;
	strcpy_s (cp, sizeof(cp), _extensions[type]);
}

BOOL
FileExtensions::isCorrectExtension (char *szFile, int type)
{
	int extlen = strlen(_extensions[type]);
	int filelen = strlen(szFile);
	if (filelen > extlen)
		return !_stricmp (szFile + filelen - extlen, _extensions[type]);
	return FALSE;
}

void
FileExtensions::ReadExtensions (char *szIniFile, char *szIniSection)
{
	char buf[128];
	for (int i = 0; i < NUM_TYPES; ++i)
	{
		GetPrivateProfileStringA(szIniSection, _iniStrings[i], "",
								buf, sizeof(buf), szIniFile);
		if (*buf)
			SetExtension (i, buf);
	}
}

void
FileExtensions::WriteExtensions (char *szIniFile, char *szIniSection)
{
	for (int i = 0; i < NUM_TYPES; ++i)
		WritePrivateProfileStringA(szIniSection, _iniStrings[i],
								  GetExtension(i), szIniFile);
}

// This function could be a bit more efficient by not including
// the extensions from _backupextensions that are already selected.
const char **
FileExtensions::MakeExtArray()
{
	int i;
	const char **extArray = new const char*[4 + sizeof(_backupextensions)/sizeof(char *)];
	extArray[0] = _fullextensions[TYPE_DATA];
	extArray[1] = _fullextensions[TYPE_INFO];
	extArray[2] = _fullextensions[TYPE_AWBI];
	for (i = 0; _backupextensions[i] != NULL; ++i)
		extArray[i+3] = _backupextensions[i];
	extArray[i+3] = 0;
	return extArray;
}
