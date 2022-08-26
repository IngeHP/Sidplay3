#include "misc.h"
#include <stdlib.h>		// for ltoa and atol
#include <stdio.h>		// for sprintf
#include <string.h>		// stricmp
#include <string>
#include <Shlwapi.h>

/**********************************************************************
 * cmdlinetok - a strtok replacement for Windows 95 commandline parsing
 * where filenames with spaces are quoted. Cmdlinetok functions like
 * strtok with two exceptions
 * 1. It always uses the space character to separate tokens
 * 2. When a '"' character is the first character of an argument,
 *    all that comes until the next '"' character is considered as
 *    one argument
 *
 * Created 1996-07-07 by Adam Lorentzon
 **********************************************************************/
int cmdlinetok( const wchar_t *cmdline, char *dst )
{
	static char *curp = NULL;
	char *cp;
	static bool bReachedEnd = true;
	size_t size = (wcslen(cmdline) + 1) * 2;
	char *src = new char[size];
	// Convert from wchar_t to char
	wtoc(cmdline, src);

	if( src != NULL )
	{
		curp = src;
		bReachedEnd = false;
	}
	// Uninitialized use!
	if (bReachedEnd || curp == NULL)
	{
		delete[] src;
		return -1;
	}

	// Skip spaces on cmdline
	while( *curp == ' ' )
		++curp;

	// Windows 95 quotes filenames with spaces.
	if( *curp == '"' )
	{
		++curp;
		cp = curp;	// cp is beginning of filename with spaces in
		while( *curp != '"' )
		{
			if( *curp == '\0' )
			{
				bReachedEnd = true;
				break;
			}
			++curp;
		}
	}
	else
	{
		cp = curp;	// cp is beginning of argument
		while( *curp != ' ' )
		{
			if( *curp == '\0' )
			{
				bReachedEnd = true;
				break;
			}
			++curp;
		}
	}
	*curp = '\0';
	++curp;
	if( *curp == '\0' )
		bReachedEnd = true;
	strcpy_s(dst, size, cp);
	delete[] src;
	return 0;
}


/*
 * maketextdisplayable - converts the string into buffer buf, making
 * the ampersand character doubled because otherwise Windows takes it
 * as a control character (the next character should be underlined).
 * The buffer buf should be at least twice the size of the length of szText
 */
char *maketextdisplayable( char *buf, const char *szText )
{
	int i = 0;
	while( *szText != '\0' )
	{
		if( *szText == '&' )
		{
			buf[i++] = '&';
			buf[i++] = '&';
		}
		else
			buf[i++] = *szText;
		++szText;
	}
	buf[i] = '\0';
	return buf;
}


/*
 * GetFreeDiskSpace (drive)
 * returns the free disk space on the specified drive, or -1 if the drive
 * doesn't exist or something went wrong.
 * drive is a number specifying the drive. a: is 0, b: is 1, and so on
 */
unsigned long
GetFreeDiskSpace( int drive )
{
	DWORD dwSectorsPerCluster, dwBytesPerSector, dwFreeClusters, dwClusters;
	char buf[4];
	sprintf_s( buf, "%c:\\", 'a' + drive );
	GetDiskFreeSpaceA( buf, &dwSectorsPerCluster, &dwBytesPerSector,
					  &dwFreeClusters, &dwClusters );
	return (unsigned long) dwFreeClusters * dwSectorsPerCluster * dwBytesPerSector;
}

bool
OverwriteIfExists( char *szFileName, HWND hwnd )
{
	OFSTRUCT of;
	if( OpenFile(szFileName, &of, OF_EXIST) != HFILE_ERROR )
	{
		return( IDYES == MessageBoxA( hwnd, "File exists. Do you want to overwrite it?",
									 szFileName,
									 MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) );
	}
	return true;
}

/*
 * EndStringWithChar - makes sure a string ends with a certain character.
 * the string supplied must be able to store an additional character, if
 * necessary.
 */
void
EndStringWithChar( char *szString, char c )
{
	if( szString == NULL )
		return;

	char cLast = '\0';

	// Find string terminator
	while( (cLast = *szString) != '\0' )
		szString++;

	// If last char is no c, add it.
	if( cLast != c )
	{
		*szString++ = c;
		*szString = '\0';
	}
}

char *
FileNameInPath (char *szFullname)
{
	char *cp;
	if ((cp = strrchr (szFullname, '\\')) != NULL)
		return cp + 1;
	return szFullname;
}

/*
 * RunsNewShell - returns true if we're running Windows 95 or Win NT 4.0
 *
bool
RunsNewShell (void)
{
	OSVERSIONINFO os;
	memset( &os, 0, sizeof(OSVERSIONINFO) );
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &os );
	return ( os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ||
			 (os.dwPlatformId == VER_PLATFORM_WIN32_NT && os.dwMajorVersion >= 4) );
}
*/

void copyListboxContents( HWND hlbDest, HWND hlbSrc )
{
	int nEntries = SendMessage( hlbSrc, LB_GETCOUNT, 0, 0L );
	char szTmp[MAX_PATH];
	for( int i = 0; i < nEntries; ++i )
	{
		if( SendMessage(hlbSrc, LB_GETTEXTLEN, i, 0L) < sizeof(szTmp)-1 )
		{
			SendMessage( hlbSrc, LB_GETTEXT, i, (LPARAM)szTmp );
			SendMessage( hlbDest, LB_ADDSTRING, i, (LPARAM)szTmp );
		}
	}
}

void upcaseString (wchar_t *s)
{
	if (s != NULL)
		CharUpperBuff(s, wcslen(s));
/*
		while (*s != '\0')
	  {
		*s = toupper(*s);
		 s++;
	  }
*/
}

void downcaseString (wchar_t *s)
{
	if (s != NULL)
		CharLowerBuff(s, wcslen(s));
/*		while (*s != '\0')
	  {
		*s = tolower(*s);
		 s++;
	  }
*/
}


extern HINSTANCE hInst;

/*
	DirFillListLong
	Fills the listbox hList with a listing of the directory szDir, including
	drives and directories.
	szFilter is for which files are to be selected, like "*.dat", or if
	multiple extensions are wanted, "*.dat;*.mus". NULL or "" equals "*.*".
*/
void DirFillListLong( HWND hList, const char *szDirectory, const char *szFilter, DWORD settings )
{
	HANDLE hDir;
	WIN32_FIND_DATA info;
	wchar_t szTmp[512];
	HWND hlbDirs, hlbDrives, hlbFiles;
	DWORD dwStyle = GetWindowLong( hList, GWL_STYLE );
	int maxPixels = 50;
	HDC hdcList = 0;
	if( dwStyle & LBS_MULTICOLUMN )
		hdcList = GetDC( hList );
	if( settings == (DWORD)-1 )
	{
		extern DWORD dwDirSettings;
		settings = dwDirSettings;
	}

	// Create hidden listboxes for individual sorting of files, dirs and drives
	hlbDirs = CreateWindow(L"listbox", NULL,
		LBS_SORT | LBS_HASSTRINGS,
		0, 0, 80, 80,
		0, 0,
		hInst, NULL);

	hlbDrives = CreateWindow (L"listbox", NULL,
						LBS_SORT | LBS_HASSTRINGS,
						0,0,80,80,
						0, 0,
						hInst, NULL);

	hlbFiles = CreateWindow(L"listbox", NULL,
						LBS_SORT | LBS_HASSTRINGS,
						0,0,80,80,
						0, 0,
						hInst, NULL);

	wchar_t *szFiles = new wchar_t[FILENAME_MAX];
	wchar_t	*szDir = new wchar_t[_MAX_DIR];
	if( szFilter == NULL || *szFilter == '\0' )
		szFiles = StrDup( L"*.*" );
	else
		ctow(szFilter,szFiles);
	int dirlen = strlen( szDirectory );
	int sSize = dirlen + max(3, (int)wcslen(szFiles));
	// Allocate space. Dir + '\\' + filter (min *.*) + '\0'
	szDir = new wchar_t[_MAX_DIR];
	ctow(szDirectory, szDir);

	if (szDir[dirlen - 1] != '\\')
	{
		wcscpy_s (szDir+dirlen, sSize + 2, L"\\");
		++dirlen;
	}
	wcscat_s(szDir, sSize + 2 ,L"*.*\0");

	// Get all drives into a listbox
	SendMessage( hlbDrives, LB_DIR, DDL_DRIVES | DDL_EXCLUSIVE, (LPARAM)"" );

	// All directories into another
	if ((hDir = FindFirstFile (szDir, &info)) != INVALID_HANDLE_VALUE)
	{
		BOOL bNextOK = 1;
		while( bNextOK != 0)
		{
			if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Add all directories but the current (".")
				if (wcscmp(info.cFileName, L"."))
				{
					// Get long or short directory name
					if (!(settings & DIR_SHORTNAMES) || *info.cAlternateFileName == '\0')
						swprintf_s (szTmp, L"[%.*s]", (sizeof szTmp) - 3, info.cFileName);
					else
						swprintf_s (szTmp, L"[%s]", info.cAlternateFileName);
					// Change case, if required
					if (settings & DIR_LOWERCASE)
						downcaseString(szTmp);
					else if (settings & DIR_UPPERCASE)
						upcaseString(szTmp);
					SendMessage(hlbDirs, LB_ADDSTRING, 0, (LPARAM)szTmp);

					if (hdcList)
					{
						SIZE strSize;	// SIZE is identical to POINT
						GetTextExtentPoint32(hdcList, szTmp, wcslen(szTmp), &strSize);
						maxPixels = max (maxPixels, (int)strSize.cx);
					}
				}
			}

			bNextOK = FindNextFile( hDir, &info );
		}		// while (bNextOK)
		FindClose (hDir);
	}

	// And selected files into a third.
	wchar_t *nextToken;
	wchar_t *type = wcstok_s( szFiles, L";", &nextToken );
	while( type != NULL )
	{
		wcscpy_s(szDir + dirlen, sSize + 2, type);
		if( (hDir = FindFirstFile (szDir, &info)) != INVALID_HANDLE_VALUE )
		{
			BOOL bNextOK = 1;
			while( bNextOK )
			{
				// Add all regular files
				if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					// Get long or short file name
					if (!(settings & DIR_SHORTNAMES) || *info.cAlternateFileName == '\0')
						wcscpy_s(szTmp, 511,info.cFileName);
					else
						wcscpy_s(szTmp, 511, info.cAlternateFileName);
					// Change case, if required
					if (settings & DIR_LOWERCASE)
						downcaseString(szTmp);
					else if (settings & DIR_UPPERCASE)
						upcaseString(szTmp);
					SendMessage(hlbFiles, LB_ADDSTRING, 0, (LPARAM)szTmp);

					if (hdcList)
					{
						SIZE strSize;
						GetTextExtentPoint32(hdcList, szTmp, wcslen(szTmp), &strSize);
						maxPixels = max (maxPixels, (int)strSize.cx);
					}
				}
				bNextOK =  FindNextFile( hDir, &info );
			}
			FindClose (hDir);
		}
		type = wcstok_s (NULL, L";",&nextToken);
	}		// while (type != NULL)

	// Clear list box
	SendMessage(hList, LB_RESETCONTENT, 0, 0L);
	// Finally, move contents of the three list boxes into hList
	DWORD order = settings & 127;
	for (int i = 0; i < 3; ++i)
	{
		switch (order % 4)
		{
		case 1:
			copyListboxContents (hList, hlbFiles);
			break;
		case 2:
			copyListboxContents (hList, hlbDirs);
			break;
		case 3:
			copyListboxContents (hList, hlbDrives);
			break;
		default:
			break;	// Skip it, apparently
		}
		order /= 4;
	}

	// Adjust the column width
	if( hdcList )
		SendMessage( hList, LB_SETCOLUMNWIDTH, (WPARAM)maxPixels, 0 );

	// Clean up
	DestroyWindow(hlbDirs);
	DestroyWindow(hlbDrives);
	DestroyWindow(hlbFiles);
	delete[] szDir;
	delete[] szFiles;

	// Make caret visible
	if( dwStyle & ~LBS_MULTIPLESEL )
		SendMessage( hList, LB_SETCURSEL, 0, 0 );
}

bool
WriteFileString (HFILE hf, const char *str)
{
	UINT size=strlen(str);
	UINT retvalue = _lwrite (hf, str, strlen(str));
	return (retvalue != HFILE_ERROR && retvalue == size);
}

/*
 * MyDeleteFile - deletes the file with the path given in szFileName
 * Returns TRUE if the file was deleted, FALSE otherwise
 */
bool
MyDeleteFile( const char *szFileName )
{
   OFSTRUCT of;
   of.cBytes = sizeof(OFSTRUCT);
   return( OpenFile(szFileName, &of, OF_DELETE) != HFILE_ERROR );
}



/* Reads a timefield, like 5:20.1, 1.45:67, etc. and returns
   the number of tenths of seconds it corresponds to. */
int
GetDlgItemTime( HWND hDlg, int nIDDlgItem )
{
	wchar_t szBuf[16];
	szBuf[0] = '\0';
	int retval = 0;
	GetDlgItemText( hDlg, nIDDlgItem, szBuf, sizeof(szBuf)-1 );
	wchar_t *numstart = szBuf;
	wchar_t *cp = szBuf;
	for( ; *cp != '\0'; ++cp )
	{
		if( *cp >= '0' && *cp <= '9' )
			continue;
		*cp = '\0';
		retval = retval * 60 + _wtoi(numstart);
		numstart = cp+1;
	}
	// If the last number is not the only one, and only one digit, it's a tenth
	if( numstart > szBuf && wcslen(numstart) < 2 )
		return 10 * retval + _wtoi(numstart);	// tenth of seconds
	return 10 * (retval * 60 + _wtoi(numstart));		// seconds
}


/* Sets a timefield, ss or mm:ss */
void
SetDlgItemTime( HWND hDlg, int nIDDlgItem, int seconds )
{
	char szBuf[16];
	int ss = seconds % 60;
	int mm = (seconds / 60) % 60;
	sprintf_s( szBuf, "%d:%.02d", mm, ss );
	SetDlgItemTextA( hDlg, nIDDlgItem, szBuf );
}

/********************************************************************
 * parseKeyValue  -  Finds a key and value pair in a buffer
 *
 * In -  pKey:		pointer to int receiving index to key in buffer
 *       pValue:	pointer to int receiving index to value in buffer
 *       buf:		pointer to buffer storing line with key/value
 * Out:  Returns true and sets pKey and pValue if pair was found,
 *       also puts terminating zeroes for key and value in the buffer,
 *       otherwise returns false
 *
 * Created by Adam Lorentzon 1998-08-31
 *******************************************************************/
bool
parseKeyValue( int *pKey, int *pValue, char *buf )
{
	int i = 0;

	if( buf == NULL )
		return false;

	while( buf[i] != '\0' )
	{
		if( buf[i] == '=' )
		{
			*pKey = 0;
			*pValue = i+1;
			buf[i] = '\0';
			// todo? trim leading and trailing key/value whitespace
			return true;
		}
	}
	return false;
}


/********************************************************************
 * getLine  -  Reads a line from a file into a buffer
 *
 * In -  fp:		file to read from
 *       buf:		buffer to read line into
 *       bufsize:	size of buffer
 * Out:  Returns true if a line was successfully read.
 *               false if an error occurred during reading (eof, ...)
 * NOTE: If buffer is too small to hold the line, the function will
 *       read as much as possible into the buffer and zero-terminate
 *       it. It will then read (and discard) the remainder of the line
 *       so that the next call will get the next line. The return
 *       value in this case will be true.
* TODO: Return int value to be able to warn about long lines
 *
 * Created by Adam Lorentzon 1998-09-04
 *******************************************************************/
bool
getLine( FILE *fp, char *buf, int bufsize )
{
	int i = 0, ch;

	if( buf == NULL )
		return false;

	if( bufsize < 1 )
		return false;

	while( i < bufsize-1 )
	{
		ch = fgetc( fp );

		// End of file?
		if( ch == EOF && i == 0 )
			return false;

		if( ch == EOF || ch == '\n' )
			break;
		else
			buf[i++] = (char) ch;
	}
	buf[i] = '\0';

	return true;
}


/*
 * wtoc - function for converting from wchar_t to char
 *
 * Based on code-example from Microsoft
 * http://msdn.microsoft.com/en-us/library/ms235631.aspx
 *
 */

size_t wtoc(const wchar_t *src, char *dst)
{
	errno_t err;
	size_t srcSize = wcslen(src) + 1;
	size_t convertedChars = 0;
	size_t newsize = srcSize * 2;
	char *szTmp = new char[newsize];

	// convert the string
	err = wcstombs_s(&convertedChars, szTmp, newsize, src, _TRUNCATE);
	if (err != 0)
		return -1;
	strcpy_s(dst, newsize, szTmp);
	delete[] szTmp;
	return convertedChars;
}

/*
* ctow - function for converting from char to wchar_t
*
* Based on code-example from Microsoft
* http://msdn.microsoft.com/en-us/library/ms235631.aspx
*
*/

size_t ctow(const char *src, wchar_t *dst)
{
	errno_t err;
	size_t convertedChars = 0;
	size_t size = strlen(src) + 1;
	err = mbstowcs_s(&convertedChars, dst, size, src, _TRUNCATE);
	if (err != 0)
		return -1;
	return convertedChars;
}

/*
* getAppPath - returns the path of the current exe-file
*
*
*/
int getAppPath(wchar_t *dst)
{
	HMODULE hModule = GetModuleHandle(NULL);
	wchar_t *path = new wchar_t[MAX_PATH + 1];
	int size = GetModuleFileName(hModule, path, MAX_PATH);
	
	int i = size - 1;
	while (path[i] != L'\\')
		i--;
	path[i + 1] = L'\0';
	size = wcscpy_s(dst, MAX_PATH, path);
	delete[] path;
	return size;
}

/*
* SetupIni - Checks if an ini-file exists and creates one if it doesn't
*
* Also copies an old one if possible
*/
void SetupIni()
{
	// Get Appdata path
}