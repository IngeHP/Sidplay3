/*
 * ofile.h - An interface simplifying opening files
 * Customized for use with Sidplay
 */
#include <windows.h>
#include <commdlg.h>


#ifndef _OFILE_H_
#define _OFILE_H_


class FileExtensions;

class OwnFile
{
	public:
		OwnFile(HWND);
		~OwnFile();
		void SetFilter (const char *szNewFilter);
		void SetFilterIndex (int nIndex);
		void SetFileExtension (const char *szExt);
		void SetFileName (const char *szFilename);
		inline char *GetFileTitle () { return szFileTitle; };
		inline char *GetFileName () { return szFile; };
		inline char *GetDirectory () { return szDirName; };
		inline int	GetFilterIndex () { return ofn.nFilterIndex; }

	protected:
		OPENFILENAMEA ofn;
		char  szDirName[256];
		char  szFile[256];
		char  szFileTitle[256];
		char  szWindowTitle[100];
		char *szFilter;
		char *szFileExtension;
		int		nFilters;
		// Fills the FileTitle and DirName fields from the File string
		void	SetDirAndFile();
		void	ShortenFilename();
};

class OwnOpenFile : public OwnFile
{
	public:
		OwnOpenFile (HWND);
		HFILE Open (char *szTitle=NULL);
		char *Choose (const char *szTitle=NULL);
		HFILE OpenChosen ();
};

class OwnSaveFile : public OwnFile
{
	public:
		OwnSaveFile (HWND);
		HFILE Open (char *szTitle=NULL);
		char *Choose (const char *szTitle=NULL);
		HFILE OpenChosen ();
		void SetDirAndFile ();
};

class SaveWavFile : public OwnSaveFile
{
	public:
		SaveWavFile (HWND hwnd);
		void PresetFileTitle (char *szSidFile);
};

class SaveListFile : public OwnSaveFile
{
	public:
		SaveListFile (HWND hwnd);
		void PresetFileTitle (char *szSidFile);
};

class SaveAsFile : public OwnSaveFile
{
	public:
		SaveAsFile (HWND hwnd);
		void	PresetFileTitle (char *szSidFile);
		int	GetFileType ();
		char *Choose (const FileExtensions& exts,
						  char *szTitle=NULL, BOOL bSidplayer = FALSE);
	private:
		// TODO disable Open()
		void SetPSIDFilter (const FileExtensions& exts);
		void SetMUSFilter (const FileExtensions& exts);
		BOOL bMusFilter;
};

class OpenSidFile : public OwnOpenFile
{
	public:
		OpenSidFile (HWND hwnd);
		char *Choose (const FileExtensions& extensions, const char *szTitle);
};

class OpenListFile : public OwnOpenFile
{
	public:
		OpenListFile (HWND hwnd);
		char *Choose (const FileExtensions& extensions, const char *szTitle);
};

enum { TYPE_PSID = 0, TYPE_INFO, TYPE_DATA, TYPE_WAV, TYPE_MUS, TYPE_AWBI, TYPE_LIST, NUM_TYPES };

class FileExtensions
{
	public:
		FileExtensions();
		~FileExtensions();
		char *GetExtension (int type) const;
		void SetExtension (int type, char *szExt);
		void AppendExtension (char *szFile, int type);
      BOOL isCorrectExtension (char *szFile, int type);
		void ReadExtensions (char *szIniFile, char *szIniSection);
		void WriteExtensions (char *szIniFile, char *szIniSection);
		const char **MakeExtArray();
	private:
		char *_fullextensions[NUM_TYPES+1];
		char *_extensions[NUM_TYPES+1];
};

#endif // _OFILE_H_
