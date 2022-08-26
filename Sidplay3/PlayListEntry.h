#pragma once

#include <string>
#include <Windows.h>

using namespace std;

class PlayListEntry
{
	friend class PlayList;

private:
	string filename;
	string archive;
	string tempname;
	string title;
	string author;
	string released;
	int nSubSongs;
	int _subSong;
	int nPlayTime;
	int nFadeOutTime;
	bool sldb;

	PlayListEntry* prev;
	PlayListEntry* next;

	bool bPlayed;
	void SetNumSubSongs(int num);
	void SetFileName(const string newFilename);
	void SetTitle(const string newTitle);
	void SetAuthor(const string newAuthor);
	void SetReleased(const string newReleased);

public:
	PlayListEntry();
	PlayListEntry(string fileName);
	~PlayListEntry();

	void FillComboBoxWithSubSongs(HWND hCB);
	void SetSubSong(int song);
	void SetTime(int time);
	void SetFadeout(int time);
	void SetSLDB(bool useSldb);
	const string Title();
	const string Author();
	const string Released();
	const string Filename();
	int SubSong();
	int NumSubSongs();
	int PlayTime();
	int FadeoutTime();
	bool UseSLDB();
	operator bool();

};

inline const string PlayListEntry::Title()
{
	return title;
}

inline const string PlayListEntry::Author()
{
	return author;
}

inline const string PlayListEntry::Released()
{
	return released;
}

inline const string PlayListEntry::Filename()
{
	return filename;
}

inline int PlayListEntry::SubSong()
{
	return _subSong;
}

inline int PlayListEntry::NumSubSongs()
{
	return nSubSongs;
}

inline int PlayListEntry::PlayTime()
{
	return nPlayTime;
}

inline void PlayListEntry::SetFadeout(int time)
{
	nFadeOutTime = time;
}

inline int PlayListEntry::FadeoutTime()
{
	return nFadeOutTime;
}

inline bool PlayListEntry::UseSLDB()
{
	return sldb;
}