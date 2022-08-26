#pragma once
#include <string>
#include <Windows.h>

#include "PlayListEntry.h"
#include "IniFile.h"
#include "FileReader.h"
#include "FileWriter.h"

#define PLAYLIST_ORDER_NORMAL		0
#define PLAYLIST_ORDER_SHUFFLED		1
#define PLAYLIST_ORDER_RANDOM		2
#define OWM_ADDINDICESTOLIST		(WM_USER+1)
#define OWM_CLEARINFO				(WM_USER+2)
#define OWM_SETLISTINFO				(WM_USER+3)
#define OWM_SETSELECTEDINFO			(WM_USER+4)

using namespace std;

class PlayList
{
protected:
	PlayListEntry* first;
	PlayListEntry* last;
	PlayListEntry* current;

	int nNumEntries;
	int iCurrentEntry;
	bool bStatus;
	void Changed();
	int nDefPlayTime;
	int nDefFadeoutTime;

	bool Load(string File);
	bool LoadOldFormat(const string File);
	bool LoadNewFormat(const string File);
	bool SaveOldFormat(const string File);
	string GetBaseDir();
	int LastSlash(const string str);
	int DirCompare(const string str1, const string str2, int len);


public:
	static INT_PTR CALLBACK EditPlayListProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK ViewPlayListProc(HWND, UINT, WPARAM, LPARAM);
	HWND CreateListViewWindow(HWND hParent);


	string inifile;

	bool bRepeat;
	int order;
	bool isPending;

	PlayList();
	PlayList(string file);
	~PlayList();
	bool Status();

	int NumEntries();
	int CurIndex();
	PlayListEntry* Cur();
	PlayListEntry* Next();
	PlayListEntry* Prev();
	PlayListEntry* AtIndex(int index);
	PlayListEntry* Remove(int index);
	bool SetCurrent(int index);
	bool Delete(int index);
	void Clear();
	bool Insert(PlayListEntry* newEntry, int index);
	void Add(PlayListEntry* newEntry);
	bool Move(int iCurPos, int iNewPos);
	bool Save(const string File, int format=2);
	bool AddDir(const string dir, const char* filter, bool bAddSubSongs);
	bool ExistsPrev();
	bool ExistsNext();
	bool AddDependent(HWND hDep);
	bool RemoveDependent(HWND hDep);
	void FillListBox(HWND hLB, BOOL bMarkCurrent);
	void SetFont(HFONT font);
	int PlayTime(PlayListEntry* entry);
	int FadeoutTime(PlayListEntry* entry);

	static const string listSection;

	static const string theFormat;
	static const string theBasedir;
	static const string theFilename;
	static const string theSubtune;
	static const string theSongs;
	static const string theTime;
	static const string theFadeout;
	static const string theName;
	static const string theAuthor;
	static const string theReleased;
	static const string theSLDB;
};

inline PlayListEntry* PlayList::Cur()
{
	return current;
}

inline int PlayList::NumEntries()
{
	return nNumEntries;
}

inline bool PlayList::Status()
{
	return bStatus;
}

inline int PlayList::PlayTime(PlayListEntry* entry)
{
	if (entry && entry->nPlayTime > 0)
		return entry->nPlayTime;
	else
		return nDefPlayTime;
}

inline int PlayList::FadeoutTime(PlayListEntry* entry)
{
	if (entry && entry->nFadeOutTime > 0)
		return entry->nFadeOutTime;
	else
		return nDefFadeoutTime;
}

