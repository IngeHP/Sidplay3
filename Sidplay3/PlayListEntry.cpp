#include "PlayListEntry.h"
#include "Ofile.h"
#include "Songlength.h"

#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidTuneInfo.h>

using namespace std;
extern Songlength* mySLDB;

PlayListEntry::PlayListEntry() : prev(0), next(0)
{
	filename = "";
	title = "";
	author = "";
	released = "";
	nSubSongs = 0;
	_subSong = 0;
	nPlayTime = 0;
	nFadeOutTime = -1;
	sldb = true;
}

PlayListEntry::PlayListEntry(const string FileName) : prev(0), next(0)
{
	FileExtensions ext;
	const char** extArray = ext.MakeExtArray();
	// Open the chosen file and copy the relevant info
	SidTune sid(FileName.c_str(), extArray);
	delete[] extArray;

	if (!sid.getStatus())    // Error while loading
	{
		filename = "";
		return;
	}

	const SidTuneInfo *info = sid.getInfo();

	filename = FileName;
	title = string(info->infoString(0));
	author = string(info->infoString(1));
	released = string(info->infoString(2));
	nFadeOutTime = -1;
	nPlayTime = 0;
	sldb = true;
	SetNumSubSongs(info->songs());
	SetSubSong(info->startSong());

}

PlayListEntry::~PlayListEntry()
{
}

PlayListEntry::operator bool()
{
	return (filename != "");
}

void PlayListEntry::SetSubSong(int song)
{
	_subSong = song;
	if (sldb == true)
	{
		FileExtensions ext;
		const char** extArray = ext.MakeExtArray();
		SidTune sid(filename.c_str(), extArray);
		delete[] extArray;
		char md5[SidTune::MD5_LENGTH + 1];
		sid.createMD5New(md5);
		nPlayTime = mySLDB->Length(md5, song);
	}
}

void PlayListEntry::SetNumSubSongs(int num)
{
	if (num < 1)
		num = 1;
	nSubSongs = num;
}

void PlayListEntry::SetTime(int time)
{
	if (time < 0)
		time = 0;
	nPlayTime = time;
}

void PlayListEntry::SetFileName(const string NewFileName)
{
	filename = NewFileName;
}

void PlayListEntry::SetTitle(const string NewTitle)
{
	title = NewTitle;
}

void PlayListEntry::SetAuthor(const string NewAuthor)
{
	author = NewAuthor;
}

void PlayListEntry::SetReleased(const string NewReleased)
{
	released = NewReleased;
}

void PlayListEntry::SetSLDB(const bool useSldb)
{
	sldb = useSldb;
}

void PlayListEntry::FillComboBoxWithSubSongs(HWND hCB)
{
	char szTmp[10];
	SendMessage(hCB, CB_RESETCONTENT, 0, 0);
	if (nSubSongs < 1)
		nSubSongs = 1;
	for (int i = 1; i <= nSubSongs; ++i)
	{
		_itoa_s(i, szTmp, 10,10);
		SendMessageA(hCB, CB_ADDSTRING, 0, (LPARAM)(LPSTR)szTmp);
	}

	SendMessage(hCB, CB_SETCURSEL, _subSong - 1, 0);
	EnableWindow(hCB, nSubSongs > 1);
}