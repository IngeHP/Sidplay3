#ifndef _MISC_H_
#define _MISC_H_

#include <windows.h>
#include <Shlwapi.h>

// some nice macros concerning dialog items.
#define EnableDlgItem(d,x,e) EnableWindow(GetDlgItem((d),(x)),(e))
#define IsDlgItemEnabled(d,x) IsWindowEnabled(GetDlgItem((d),(x)))
#define SetDlgItemFocus(d,x) SetFocus(GetDlgItem((d),(x)))
#define GetDlgItemTextLen(d,x) GetWindowTextLength(GetDlgItem((d),(x)))


int cmdlinetok (const wchar_t *cmdline, char *dst);
char *maketextdisplayable (char *buf, const char *szText);
unsigned long GetFreeDiskSpace( int drive );
bool OverwriteIfExists (char *szFileName, HWND hwnd = NULL);
void EndStringWithChar (char *szString, char c);
char *FileNameInPath (char *szFullname);
//bool RunsNewShell (void);
#define DIR_FIDIDR		(3*16 + 2*4 + 1*1)
#define DIR_FIDRDI		(2*16 + 3*4 + 1*1)
#define DIR_DIFIDR		(3*16 + 1*4 + 2*1)
#define DIR_DIDRFI		(1*16 + 3*4 + 2*1)
#define DIR_DRFIDI		(2*16 + 1*4 + 3*1)
#define DIR_DRDIFI		(1*16 + 2*4 + 3*1)
// max value is 57, leave some space.
#define DIR_LOWERCASE	128
#define DIR_UPPERCASE	256
#define DIR_SHORTNAMES	512
#define DIR_SINGLECOL	1024
void DirFillListLong( HWND hList, const char *szDirectory,
					  const char *szFilter, DWORD settings = -1 );
bool	WriteFileString( HFILE hf, const char *str );
bool	MyDeleteFile( const char *szFileName );
int		GetDlgItemTime( HWND hDlg, int nIDDlgItem );
void	SetDlgItemTime( HWND hDlg, int nIDDlgItem, int seconds );
size_t	wtoc(const wchar_t *src, char *dst);
size_t ctow(const char *src, wchar_t *dst);
int getAppPath(wchar_t *dst);
void	SetupIni();
#endif // _MISC_H_

