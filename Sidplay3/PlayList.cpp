#include "PlayList.h"
#include <windowsx.h>
#include <iostream>
#include <fstream>
#include <string>
#include <direct.h>

#include "Misc.h"
#include "Ofile.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "resource.h"
#include "Songlength.h"

#include "IniFile.h"

extern PlayList* myPlayList;
extern Songlength* mySLDB;
extern char szIni[MAX_PATH];

#ifndef OWM_LISTCHANGED
	#define OWM_PLAYENTRY			(WM_USER+7)
	#define OWM_LISTCHANGED			(WM_USER+8)
	#define OWM_SETFILEEXTENSIONS	(WM_USER+9)
#endif // !OWM_LISTCHANGED


constexpr auto MAX_DEPENDANTS = 16;
HWND plDependants[MAX_DEPENDANTS] = { NULL };

const string PlayList::listSection = "SIDPLAY/Windows Playlist";
const string PlayList::theFormat = "Format";
const string PlayList::theBasedir = "Basedir";
const string PlayList::theFilename = "file";
const string PlayList::theSubtune = "subtune";
const string PlayList::theSongs = "song";
const string PlayList::theTime = "time";
const string PlayList::theFadeout = "fadeout";
const string PlayList::theName = "name";
const string PlayList::theAuthor = "author";
const string PlayList::theReleased = "released";
const string PlayList::theSLDB = "SLDB";

PlayList::PlayList()
{
	first = last = current = NULL;
	iCurrentEntry = -1;
	nNumEntries = 0;
	bRepeat = false;
	order = PLAYLIST_ORDER_NORMAL;
	bStatus = true;
	nDefPlayTime = 0;
	nDefFadeoutTime = 0;
	isPending = false;
}

PlayList::PlayList(string file)
{
	// Init all members in case something should go wrong
	first = last = current = NULL;
	iCurrentEntry = -1;
	nNumEntries = 0;
	bRepeat = false;
	order = PLAYLIST_ORDER_NORMAL;
	bStatus = true;
	nDefPlayTime = 0;
	nDefFadeoutTime = 0;
	isPending = false;
	Load(file);
}

PlayList::~PlayList()
{
	Clear();
}

void PlayList::Changed()
{
	MSG msg;

	for (int i = 0; (i < MAX_DEPENDANTS) && (plDependants[i] != NULL); ++i)
	{
		if (!PeekMessage(&msg, plDependants[i], OWM_LISTCHANGED, OWM_LISTCHANGED, PM_NOYIELD | PM_NOREMOVE))
			PostMessage(plDependants[i], OWM_LISTCHANGED, 0, 0);
	}
}

bool PlayList::AddDependent(HWND hDep)
{
	for (int i = 0; i < MAX_DEPENDANTS; ++i)
	{
		if (plDependants[i] == NULL || plDependants[i] == hDep)
		{
			plDependants[i] = hDep;
			return true;
		}
	}
	return false; // Array full
}

bool PlayList::RemoveDependent(HWND hDep)
{
	for (int i = 0; (i < MAX_DEPENDANTS) && (plDependants[i] != NULL); ++i)
	{
		if (plDependants[i] == hDep)
		{
			for (int j = i; j < MAX_DEPENDANTS && plDependants[j] != NULL; ++j)
			{
				plDependants[j] = plDependants[j + 1];
			}
			return true;
		}
	}
	return false;
}

void PlayList::Clear()
{
	PlayListEntry* tmp = first;
	while (tmp)
	{
		PlayListEntry* toDelete = tmp;
		tmp = tmp->next;
		delete toDelete;
	}
	current = first = last = NULL;
	nNumEntries = 0;
	Changed();
}

int PlayList::CurIndex()
{
	PlayListEntry* tmp = first;
	int i = 0;
	if (current == NULL)		// No current entry
		return -1;

	while (tmp)
	{
		if (tmp == current)
			return i;
		tmp = tmp->next;
		++i;
	}
	// Current doesn't exist anymore. Set it to first, if existent
	if (first)
	{
		current = first;
		return 0;
	}
	return -1;
}

bool PlayList::ExistsNext()
{
	// If there is at least one entry, there will be a next one if
	// a) none is current
	// b) random mode is set
	// c) there is a current and it has a next one
	// d) repeat is active
	return (nNumEntries > 0 && (!current || order == PLAYLIST_ORDER_RANDOM || current && current->next || bRepeat));
}

bool PlayList::ExistsPrev()
{
	// If there is at least one entry, and random mode is NOT active,
	// there will bi a nex one if
	// a) none is current (start from the last)
	// b) there is a current and it has a previous one
	// c) repeat is active
	return (nNumEntries > 0 && order != PLAYLIST_ORDER_RANDOM && (!current || current && current->prev || bRepeat));
}

PlayListEntry* PlayList::Next()
{
	// Make sure there really is a next entry
	if (!ExistsNext())
		return NULL;
	
	if (order == PLAYLIST_ORDER_RANDOM)
	{
		// Pick a new entry at random
		int newIndex;
		int currIndex = CurIndex();
		do
		{
			newIndex = ((long)nNumEntries * rand()) / RAND_MAX;
		} while (newIndex == currIndex);

	}
	else if (current)
	{
		if (!current->next && bRepeat)
			current = first;
		else
			current = current->next;
	}
	else
		current = first;

	Changed();
	return current;
}

PlayListEntry* PlayList::Prev()
{
	if (!ExistsPrev())
		return NULL;

	if (current)
	{
		if (!current->prev)
		{
			if (bRepeat)
				current = last;
		}
		else
			current = current->prev;
	}
	else
		current = last;

	Changed();
	return current;
}

PlayListEntry* PlayList::AtIndex(int index)
{
	PlayListEntry* tmp = first;
	if (index >= 0 && index < nNumEntries)
	{
		for (int i = 0; tmp != NULL; ++i)
		{
			if (i == index)
				return tmp;
			tmp = tmp->next;
		}
		
	}
	return NULL;
}

bool PlayList::SetCurrent(int index)
{
	PlayListEntry* tmp;

	if ((tmp = AtIndex(index)) != NULL)
	{
		current = tmp;
		Changed();
		return true;
	}
	return false;
}

PlayListEntry* PlayList::Remove(int index)
{
	PlayListEntry* tmp = first;

	if ((tmp = AtIndex(index)) != NULL)
	{
		if (tmp == first)
			first = tmp->next;
		if (tmp == last)
			last = tmp->prev;
		if (tmp->prev)
			tmp->prev->next = tmp->next;
		if (tmp->next)
			tmp->next->prev = tmp->prev;
		nNumEntries--;

		// If we're removing the current entry, set current to
		// i) the next, ii) the previous, or iii) NULL
		if (tmp == current)
		{
			if (tmp->next)
				current = tmp->next;
			else
				current = tmp->prev;
		}
		Changed();
		return tmp;
	}
	return NULL;
}

bool PlayList::Delete(int index)
{
	PlayListEntry* tmp = Remove(index);

	if (tmp)
	{
		delete tmp;
		return true;
	}
	return false;
}

bool PlayList::Insert(PlayListEntry* newEntry, int index)
{
	if (!*newEntry)			// Reject bad entries
		return false;

	// Insert at the very end of the list (or pos 0 in empty list)
	if (index == nNumEntries)
	{
		Add(newEntry);
		return true;
	}

	PlayListEntry* tmp = first;
	for (int i = 0; tmp != NULL; ++i)
	{
		tmp = tmp->next;

		if (i == index)
		{
			newEntry->prev = tmp->prev;
			newEntry->next = tmp;
			if (tmp == first)
				first = newEntry;
			if (tmp->prev)
				tmp->prev->next = newEntry;
			tmp->prev = newEntry;
			nNumEntries++;
			Changed();
			return true;
		}
	}
	// If we came here, the index must be out of range
	return false;
}

void PlayList::Add(PlayListEntry* newEntry)
{
	if (!*newEntry)		// Reject bad entries
		return;
	newEntry->prev = last;
	newEntry->next = NULL;
	if (last)
		last->next = newEntry;
	last = newEntry;
	if (!first)
		first = newEntry;
	nNumEntries++;
	Changed();
}

bool PlayList::Move(int iCurPos, int iNewPos)
{
	if (iCurPos < 0 || iCurPos >= nNumEntries || iNewPos < 0 || iNewPos >= nNumEntries)
		return false;

	PlayListEntry* tmp = Remove(iCurPos);
	if (tmp)
	{
		Insert(tmp, iNewPos);
		return true;
	}
	return false;
}

bool PlayList::SaveOldFormat(const string File)
{
	ofstream out;

	out.open(File);
	if (out.is_open())
	{
		out << "[" << listSection << "]\n";

		out << theFormat << "=1\n";

		out << "Order=";
		if (order == PLAYLIST_ORDER_RANDOM)
			out << "Random\n";
		else if (order == PLAYLIST_ORDER_SHUFFLED)
			out << "Shuffled\n";
		else
			out << "Normal\n";

		out << "Repeat=" << std::boolalpha << bRepeat << "\n";
		out << "PlayTime=" << nDefPlayTime << "\n";
		out << "FadeOutTime=" << nDefFadeoutTime << "\n\n";

		int i = 1;
		for (PlayListEntry* tmp = first; tmp != NULL; tmp = tmp->next)
		{
			out << "[" << i << "]\n";
			out << "Filename=" << tmp->filename << "\n";
			out << "Subsong=" << tmp->_subSong << "\n";
			out << "Time=" << tmp->nPlayTime << "\n";
			if (tmp->nFadeOutTime >= 0)
				out << "Fadeout=" << tmp->nFadeOutTime << "\n";
			i++;
			out << "\n";
		}
		out.close();
		return true;
	}
	else
		return false;
}


bool PlayList::Load(string File)
{
	bool rv;

	Clear();

	int listversion = GetMyProfileInt(listSection.c_str(), theFormat.c_str(), 0, File.c_str());
	switch (listversion)
	{
	case 1:
		rv = LoadOldFormat(File);
		break;
	case 2:
		rv = LoadNewFormat(File);
		break;

	default:
		rv = false;
		break;
	}
	return rv;
}

bool PlayList::LoadOldFormat(string File)
{
	string sEntrySection;
	char szTmp[MAX_PATH + 1];

	if (GetMyProfileInt(listSection.c_str(), theFormat.c_str(), 0, File.c_str()))
	{
		bStatus = false;
		return false;
	}

	// Read list details
	GetPrivateProfileStringA(listSection.c_str(), "Order", "Normal", szTmp, sizeof(szTmp), File.c_str());
	if (!_stricmp(szTmp, "Shuffled"))
		order = PLAYLIST_ORDER_SHUFFLED;
	else if (!_stricmp(szTmp, "Random"))
		order = PLAYLIST_ORDER_RANDOM;
	else
		order = PLAYLIST_ORDER_NORMAL;
	bRepeat = GetMyProfileBool(listSection.c_str(), "Repeat", false, File.c_str());

	// If base directory specified
	GetPrivateProfileStringA(listSection.c_str(), theBasedir.c_str(), "", szTmp, sizeof(szTmp), File.c_str());
	int basedirlen = strlen(szTmp);
	nDefPlayTime = GetMyProfileInt(listSection.c_str(), "PlayTime", 0, File.c_str());
	nDefFadeoutTime = GetMyProfileInt(listSection.c_str(), "FadeoutTime", 0, File.c_str());

	for (int i = 1; ; ++i)
	{
		PlayListEntry* entry;
		sEntrySection = to_string(i);
		GetPrivateProfileStringA(sEntrySection.c_str(), "Filename", "", szTmp + basedirlen, sizeof(szTmp) - basedirlen, File.c_str());
		if (!*(szTmp + basedirlen))
			break;			// No such entry. We're finished
		entry = new PlayListEntry(szTmp);
		entry->SetSubSong(GetMyProfileInt(sEntrySection.c_str(), "Subsong", 0, File.c_str()));
		entry->SetTime(GetMyProfileInt(sEntrySection.c_str(), "Time", 0, File.c_str()));
		entry->SetFadeout(GetMyProfileInt(sEntrySection.c_str(), "Fadeout", -1, File.c_str()));

		Add(entry);
	}
	return true;
}

bool PlayList::LoadNewFormat(string File)
{
	FileReader in(File);
	if (in.Status() != FileReader::OK)
		return false;

	string fullName;
	string tmp;
	bool bLoadEntries = false;

	while (in.ReadLine())
	{
		if ((tmp = in.ParsePairStr(theBasedir)) != "")
		{
			fullName = tmp;
		}
		else if ((tmp = in.ParsePairStr(theFilename)) != "")
		{
			bLoadEntries = true;
			break;
		}
	}

	if (!bLoadEntries)
		return true;

	// Read the entries
	PlayListEntry* entry = NULL;
	int num;
	bool b;

	do
	{
		if ((fullName = in.ParsePairStr(theFilename)) != "")
		{
			if (entry != NULL)
			{
				// Finished reading the last entry. Now add it.
				Add(entry);
			}
			entry = new PlayListEntry();
			entry->SetFileName(fullName);
		}
		else if ((num = in.ParsePairInt(theSubtune)) != -255)
			entry->SetSubSong(num);
		else if ((num = in.ParsePairInt(theSongs)) != -255)
			entry->SetNumSubSongs(num);
		else if ((num = in.ParsePairInt(theTime)) != -255)
			entry->SetTime(num);
		else if ((num = in.ParsePairInt(theFadeout)) != -255)
			entry->SetFadeout(num);
		else if ((tmp = in.ParsePairStr(theName)) != "")
			entry->SetTitle(tmp);
		else if ((tmp = in.ParsePairStr(theAuthor)) != "")
			entry->SetAuthor(tmp);
		else if ((tmp = in.ParsePairStr(theReleased)) != "")
			entry->SetReleased(tmp);
		else if ((b = in.ParsePairBool(theSLDB)) != false)
			entry->SetSLDB(b);
	} while (in.ReadLine());

	if (entry != NULL)
	{
		Add(entry);
	}
	return true;
}

int PlayList::LastSlash(const string str)
{
	int b = str.find_last_of('\\');
	int f = str.find_last_of('/');

	if (b > f)
		return b;
	else
		return f;
}

int PlayList::DirCompare(const string str1, const string str2, int len)
{
	for (int i = 0; i < len; i++)
	{
		if (!((tolower(str1[i]) == tolower(str2[i])) || ((str1[i] == '\\' || str1[i] == '/') && (str2[i] == '\\' || str2[i] == '/'))))
		{
			return i;
		}
	}
	return 0;
}

string PlayList::GetBaseDir()
{
	int baselen = 0;
	string basedir = "";
	int c = 0;

	PlayListEntry* tmp = first;
	if (tmp != NULL)
	{
		baselen = LastSlash(tmp->filename);
		if (baselen > 1)
		{
			basedir = tmp->filename.substr(0, baselen);

			for (; tmp != NULL; tmp = tmp->next)
			{
				if ((c = DirCompare(basedir, tmp->filename, baselen)) != 0)
				{
					baselen = c;
					basedir = tmp->filename.substr(0, baselen);
				}
				if (LastSlash(basedir) < 1)
				{
					// We failed to find a common basedir
					basedir = "";
					break;
				}
			}
		}
	}

	return basedir;
}

bool PlayList::Save(const string filename, int format)
{
	if (format == 1)
		return SaveOldFormat(filename);

	FileWriter out(filename);

	if (out.Status() != FileWriter::OK)
		return false;

	string basedir = GetBaseDir();

	out.Write("[SIDPLAY/Windows Playlist]");
	out.WritePairInt(theFormat, 2);
	if (basedir.length() > 0)
		out.WritePairStr(theBasedir, basedir);
	out.NewLine();

	for (PlayListEntry* tmp = first; tmp != NULL; tmp = tmp->next)
	{
		out.WritePairStr(theFilename, tmp->filename);
		out.WritePairStr(theName, tmp->title);
		out.WritePairStr(theAuthor, tmp->author);
		out.WritePairStr(theReleased, tmp->released);
		out.WritePairInt(theSongs, tmp->nSubSongs);
		out.WritePairInt(theSubtune, tmp->_subSong);
		if (tmp->nPlayTime != 0)
			out.WritePairInt(theTime, tmp->nPlayTime);
		if (tmp->nFadeOutTime != 0)
			out.WritePairInt(theFadeout, tmp->nFadeOutTime);
		if (tmp->UseSLDB() != false)
			out.WritePairBool(theSLDB, tmp->UseSLDB());
		out.NewLine();
	}

	return true;
}

bool PlayList::AddDir(const string dir, const char * filter, bool bAddSubsongs)
{
	extern HINSTANCE hInst;

	char szCurrentDir[MAX_PATH + 1];
	HWND hList;

	strcpy_s(szCurrentDir, MAX_PATH + 1, dir.c_str());
	string tmpdir = dir;

	// Create hidden listboxes for the directory contents
	hList = CreateWindow(L"listbox", NULL,
		LBS_HASSTRINGS,
		0, 0, 80, 80,
		0, 0,
		hInst, NULL);

	// Add all drives and files in current dir
	DirFillListLong(hList, szCurrentDir, filter, (0 * 16 + 1 * 4 + 2 * 1));

	char* szSel;
	int nCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
	if (tmpdir[tmpdir.length() - 1] != '\\')
		tmpdir = tmpdir + "\\";

	for (int i = 0; i < nCount; ++i)
	{
		DWORD dwLength = SendMessage(hList, LB_GETTEXTLEN, i, 0);
		szSel = new char[dwLength + 1];
		SendMessage(hList, LB_GETTEXT, i, (LPARAM)szSel);

		// Directory selected ?
		if (szSel[0] == '[' && szSel[dwLength - 1] == ']')
		{
			if (szSel[1] != '.')
			{
				// Recursively add dir
				string tmp = string(szSel);
				tmp = tmp.substr(1, tmp.length() - 2);
				string newsel = tmpdir + tmp;
				AddDir(newsel, filter, bAddSubsongs);
			}
			delete[] szSel;
			continue;
		}
		string newdir = tmpdir + string(szSel);
		delete[] szSel;

		PlayListEntry* entry = new PlayListEntry(newdir);
		if (bAddSubsongs && entry->NumSubSongs() > 1)
		{
			entry->SetSubSong(1);
			Add(entry);
			for (int i = 2; i < entry->NumSubSongs(); ++i)
			{
				entry = new PlayListEntry(newdir);
				entry->SetSubSong(i);
				Add(entry);
			}
		}
		else
			Add(entry);
	}

	// Clean up
	DestroyWindow(hList);
	
	return true;
}

void PlayList::FillListBox(HWND hLB, BOOL bMarkCurrent = FALSE)
{
	char LBentry[128];		// Should suffice for title + "[%d/%d]"
	
	SendMessage(hLB, WM_SETREDRAW, FALSE, 0);	// Turn off redraw for faster update
	SendMessage(hLB, LB_RESETCONTENT, 0, 0);	// Clear listbox

	for (PlayListEntry* tmp = first; tmp != NULL; tmp = tmp->next)
	{
		string title = tmp->title;
		string tmpstr;

		if (title == "<?>")
		{
			int c = tmp->filename.find_last_of('\\');

			if (c > 0)
			{
				title = tmp->filename.substr(c + 1, tmp->filename.length() - 1 - c);
			}

		}

		if (tmp->nSubSongs > 1)
		{
			if (bMarkCurrent && current == tmp)
			{
				tmpstr = "--> " + title + " [" + to_string(tmp->_subSong) + "/" + to_string(tmp->nSubSongs) + "] <--";
			}
			else
				tmpstr = title + " [" + to_string(tmp->_subSong) + "/" + to_string(tmp->nSubSongs) + "]";
			strcpy_s(LBentry, 128, tmpstr.c_str());

			SendMessageA(hLB, LB_ADDSTRING, 0, (LPARAM)LBentry);
		}
		else if (bMarkCurrent && current == tmp)
		{
			tmpstr = "--> " + title + " <--";
			strcpy_s(LBentry, 128, tmpstr.c_str());
			SendMessageA(hLB, LB_ADDSTRING, 0, (LPARAM)LBentry);
		}
		else
		{
			tmpstr = title;
			strcpy_s(LBentry, 128, tmpstr.c_str());
			SendMessageA(hLB, LB_ADDSTRING, 0, (LPARAM)LBentry);
		}
	}

	// Turn on redraw again and force a repaint
	SendMessage(hLB, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(hLB, NULL, TRUE);
}

INT_PTR CALLBACK PlayList::EditPlayListProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char szCurrentDir[MAX_PATH + 1];
	static char szFilter[80];
	int idControl;
	LONG nCount;
	LONG dwLength;
	static LONG dwIndex;
	static DWORD dwDirSettings;
	static FileExtensions* extensions;
	static bool bDontChange = false;


	switch (message)
	{

	case WM_INITDIALOG:
	{

		extensions = (FileExtensions*)lParam;
		if (!myPlayList)
		{
			myPlayList = new PlayList;
		}

		GetPrivateProfileStringA("SIDTUNES", "Directory", "C:\\", szCurrentDir, sizeof(szCurrentDir), szIni);
		GetPrivateProfileStringA("SIDTUNES", "Filter", "*sid;*mus", szFilter, sizeof(szFilter), szIni);
		dwDirSettings = GetPrivateProfileIntA("DIR LIST", "Settings", DIR_DIFIDR, szIni);
		_chdrive(tolower(szCurrentDir[0]) - 'a');
		_chdir(szCurrentDir);
		_getcwd(szCurrentDir, MAX_PATH);
		SetDlgItemTextA(hDlg, IDC_LISTDIR, szCurrentDir);
		DirFillListLong(GetDlgItem(hDlg, IDLB_LISTFILES), szCurrentDir, szFilter, dwDirSettings);
		SendMessage(hDlg, OWM_SETLISTINFO, 0, 0);
		SetFocus(GetDlgItem(hDlg, IDLB_LISTFILES));

		return FALSE;
	}

	case WM_VKEYTOITEM:
		if (LOWORD(wParam) == VK_BACK)
		{
			if (SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_FINDSTRING, -1, (LPARAM)"..") == LB_ERR)
			{
				return -1;
			}
			SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_SETCARETINDEX, SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_FINDSTRING, -1, (LPARAM)".."), 0);
			SendMessage(hDlg, WM_COMMAND, MAKELONG(IDLB_LISTFILES, LBN_DBLCLK), 0);
			return -2;
		}
		return -1;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDLB_LISTFILES:
			idControl = LOWORD(wParam);
			if (HIWORD(wParam) == LBN_DBLCLK)
			{
				char* szSel;
				dwIndex = SendDlgItemMessage(hDlg, idControl, LB_GETCARETINDEX, 0, 0);
				if (dwIndex == (DWORD)LB_ERR)
				{
					return FALSE;
				}
				dwLength = SendDlgItemMessageA(hDlg, idControl, LB_GETTEXTLEN, dwIndex, 0);
				szSel = new char[dwLength + 1];
				SendDlgItemMessageA(hDlg, idControl, LB_GETTEXT, dwIndex, (LPARAM)szSel);

				// Get the right directory and set it to current
				GetDlgItemTextA(hDlg, IDC_LISTDIR, szCurrentDir, MAX_PATH);
				_chdrive(tolower(szCurrentDir[0]) - 'a' + 1);
				_chdir(szCurrentDir);
				_getcwd(szCurrentDir, MAX_PATH);

				// Drive or directory selected?
				if (szSel[0] == '[' && szSel[dwLength - 1] == ']')
				{
					// A drive selected - change drive
					if (szSel[1] == '-' && szSel[dwLength - 2] == '-')
					{
						_chdrive(tolower(szSel[2]) - 'a');
						_getcwd(szCurrentDir, MAX_PATH);
						SetDlgItemTextA(hDlg, idControl - 1, szCurrentDir);
						DirFillListLong(GetDlgItem(hDlg, idControl), szCurrentDir, szFilter, dwDirSettings);
					}
					else
					{
						// A directory is selected
						char* cp, * olddir = NULL;
						BOOL bRememberDir = FALSE;

						if (!strcmp(szSel, "[..]") && szCurrentDir[strlen(szCurrentDir) - 1] != '\\')
						{
							// Remember what dir we are in
							if ((cp = strrchr(szCurrentDir, '\\')) != NULL)
							{
								cp++;
								bRememberDir = TRUE;
								olddir = new char[strlen(cp) + 3];
								sprintf_s(olddir, strlen(cp)+3, "[%s]", cp);
							}
						}

						// Change dir
						szSel[dwLength - 1] = '\0';
						_chdir(szSel + 1);
						_getcwd(szCurrentDir, MAX_PATH);
						SetDlgItemTextA(hDlg, IDC_LISTDIR, szCurrentDir);
						DirFillListLong(GetDlgItem(hDlg, idControl), szCurrentDir, szFilter, dwDirSettings);

						// Make the old dir selected if we went up in the hierarchy
						if (bRememberDir)
						{
							SendDlgItemMessage(hDlg, idControl, LB_SETCARETINDEX, SendDlgItemMessageA(hDlg, idControl, LB_FINDSTRING, -1, (LPARAM)olddir), 0);
							delete[] olddir;
						}
					}
				}
				else
				{
					SendMessage(hDlg, OWM_ADDINDICESTOLIST, 1, (LPARAM)&dwIndex);
				}
				delete[] szSel;
				return TRUE;
			}
			return FALSE;

		case IDLB_LISTCONTENTS:
		{
			WORD notification = HIWORD(wParam);
			dwIndex = SendDlgItemMessage(hDlg, IDLB_LISTCONTENTS, LB_GETCARETINDEX, 0, 0);
			if (notification == LBN_DBLCLK)
			{
				SendMessage(hDlg, WM_COMMAND, IDB_LISTREMOVE, 0);
				return TRUE;
			}
			else if (notification == LBN_SELCHANGE)
			{
				SendMessage(hDlg, OWM_SETSELECTEDINFO, 0, 0);
				return TRUE;
			}
			return FALSE;
		}

		case IDB_LISTLOAD:
		{
			OpenListFile ofile(hDlg);
			GetPrivateProfileStringA("PLAYLISTS", "Directory", "", ofile.GetDirectory(), MAX_PATH, szIni);
			if (ofile.Choose(*extensions, "Open PlayList"))
			{
				if (myPlayList->Load(ofile.GetFileName()))
				{
					WritePrivateProfileStringA("PLAYLISTS", "Directory", ofile.GetDirectory(), szIni);
					SendMessage(hDlg, OWM_SETLISTINFO, 0, 0);
				}
				else
				{
					MessageBoxA(hDlg, "Error reading playlist file", ofile.GetFileName(), MB_OK | MB_ICONEXCLAMATION);
				}
			}
			break;
		}
		case IDB_LISTSAVE:
		{
			SaveListFile ofile(hDlg);
			GetPrivateProfileStringA("PLAYLISTS", "Directory", "", ofile.GetDirectory(), MAX_PATH, szIni);
			if (ofile.Choose("Save Playlist"))
			{
				WritePrivateProfileStringA("PLAYLISTS", "Directory", ofile.GetDirectory(), szIni);
				myPlayList->Save(ofile.GetFileName(), 2);
			}
			break;
		}

		case IDB_LISTREMOVE:
		{
			dwIndex = SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_GETCURSEL, 0, 0);
			if (dwIndex == LB_ERR)
				return TRUE;
			int topIndex = SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_GETTOPINDEX, 0, 0);
			myPlayList->Delete(dwIndex);
			myPlayList->FillListBox(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
			SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_SETTOPINDEX, topIndex, 0);
			SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_SETCURSEL, (WPARAM)dwIndex, 0);
			SendMessage(hDlg, OWM_SETSELECTEDINFO, 0, 0);
			return TRUE;
		}

		case IDB_LISTREMOVEALL:
			myPlayList->Clear();
			myPlayList->FillListBox(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
			SendMessage(hDlg, OWM_CLEARINFO, 0, 0);
			return TRUE;

		case IDB_LISTADD:
		{
			nCount = SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_GETSELCOUNT, 0, 0);
			if (nCount == LB_ERR)
				return TRUE;
			int* indices = new int[nCount];
			if (SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_GETSELITEMS, nCount, (LPARAM)indices) != LB_ERR)
			{
				SendMessage(hDlg, OWM_ADDINDICESTOLIST, nCount, (LPARAM)indices);
			}
			delete[] indices;
			return TRUE;
		}

		case IDB_LISTADDALL:
		{
			nCount = SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_GETCOUNT, 0, 0);
			if (nCount == LB_ERR)
				return TRUE;
			int* indices = new int[nCount];
			for (int i = 0; i < nCount; ++i)
			{
				indices[i] = i;
			}
			SendMessage(hDlg, OWM_ADDINDICESTOLIST, nCount, (LPARAM)indices);
			delete[] indices;
			return TRUE;
		}

		case IDB_LISTUP:
			dwIndex = SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_GETCURSEL, 0, 0);
			if (dwIndex != LB_ERR && dwIndex > 0)
			{
				myPlayList->Move(dwIndex, dwIndex - 1);
				myPlayList->FillListBox(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
				SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_SETCURSEL, dwIndex - 1, 0);
				SetFocus(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
			}
			return TRUE;

		case IDB_LISTDOWN:
			dwIndex = SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_GETCURSEL, 0, 0);
			if (dwIndex != LB_ERR && dwIndex < myPlayList->NumEntries() - 1)
			{
				myPlayList->Move(dwIndex, dwIndex + 1);
				myPlayList->FillListBox(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
				SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_SETCURSEL, dwIndex + 1, 0);
				SetFocus(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
			}
			return TRUE;

		case IDC_LISTREPEAT:
			myPlayList->bRepeat = !myPlayList->bRepeat;
			myPlayList->Changed();
			CheckDlgButton(hDlg, IDC_LISTREPEAT, myPlayList->bRepeat ? 1 : 0);
			return TRUE;

		case IDCB_LISTSUBSONGS:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int song = 1 + SendDlgItemMessageA(hDlg, IDCB_LISTSUBSONGS, CB_GETCURSEL, 0, 0);
				dwIndex = SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_GETCURSEL, 0, 0);
				PlayListEntry* tmp = myPlayList->AtIndex(dwIndex);
				if (tmp != NULL)
				{
					tmp->SetSubSong(song);
					myPlayList->FillListBox(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
					SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_SETCURSEL, dwIndex, 0);
					SetDlgItemTime(hDlg, IDE_LISTTIME, tmp->PlayTime());
				}
				return TRUE;
			}
			break;

		case IDE_LISTTIME:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (bDontChange)
					return TRUE;
				dwIndex = SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_GETCARETINDEX, 0, 0);
				PlayListEntry* tmp = myPlayList->AtIndex(dwIndex);
				if (tmp != NULL)
				{
					tmp->SetTime(GetDlgItemTime(hDlg, IDE_LISTTIME) / 10);
				}
				return TRUE;
			}
			break;

		case IDE_LISTFADEOUT:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (bDontChange)
					return TRUE;
				dwIndex = SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_GETCARETINDEX, 0, 0);
				PlayListEntry* tmp = myPlayList->AtIndex(dwIndex);
				if (tmp != NULL)
				{
					// If edit box is empty, set playlist default
					if (GetDlgItemTextLen(hDlg, IDE_LISTFADEOUT) == 0)
						tmp->SetFadeout(-1);
					else
						tmp->SetFadeout(GetDlgItemTime(hDlg, IDE_LISTFADEOUT) / 10);
				}
				return TRUE;
			}
			break;

		case IDC_CBSLDB:
		{
			PlayListEntry* sel;
			LONG selIndex = SendDlgItemMessage(hDlg, IDLB_LISTCONTENTS, LB_GETCURSEL, 0, 0);
			if (!(selIndex == LB_ERR || (sel = myPlayList->AtIndex(selIndex)) == NULL))
			{
				BOOL val = CheckDlgButton(hDlg, IDC_CBSLDB, TRUE);
				sel->SetSLDB(val == TRUE ? true : false);
			}
			break;
		}

		case IDE_LISTDEFAULTTIME:
			if (HIWORD(wParam) == EN_CHANGE)
				myPlayList->nDefPlayTime = GetDlgItemTime(hDlg, IDE_LISTDEFAULTTIME) / 10;
			break;

		case IDE_LISTDEFAULTFADEOUT:
			if (HIWORD(wParam) == EN_CHANGE)
				myPlayList->nDefFadeoutTime = GetDlgItemTime(hDlg, IDE_LISTDEFAULTFADEOUT) / 10;
			break;

		case IDB_LISTORDERNORMAL:
		case IDB_LISTORDERSHUFFLED:
		case IDB_LISTORDERRANDOM:
		{
			for (int i = IDB_LISTORDERNORMAL; i <= IDB_LISTORDERRANDOM; ++i)
			{
				if (IsDlgButtonChecked(hDlg, i) == BST_CHECKED)
				{
					myPlayList->order = i - IDB_LISTORDERNORMAL;
					break;
				}
			}
			myPlayList->Changed();		// So that Prev and Next buttons can be updated
			break;
		}
		case IDOK:
		case IDB_DEF:
		{
			HWND hwndFocus = GetFocus();
			if (hwndFocus == GetDlgItem(hDlg, IDLB_LISTCONTENTS))
				idControl = IDLB_LISTCONTENTS;
			else if (hwndFocus = GetDlgItem(hDlg, IDLB_LISTFILES))
				idControl = IDLB_LISTFILES;
			else
				return FALSE;
			SendMessage(hDlg, WM_COMMAND, MAKELONG(idControl, LBN_DBLCLK), (LPARAM)hwndFocus);
			return FALSE;
		}

		case IDB_DONE:
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;
		}	// Case WM_COMMAND ends here
		break;

	case OWM_CLEARINFO:
		// Clean the info fields
		bDontChange = true;
		SetDlgItemTextA(hDlg, IDC_LISTTITLE, "");
		SetDlgItemTextA(hDlg, IDC_LISTAUTHOR, "");
		SetDlgItemTextA(hDlg, IDC_LISTCOPYRIGHT, "");
		SetDlgItemTextA(hDlg, IDE_LISTTIME, "");
		SetDlgItemTextA(hDlg, IDE_LISTFADEOUT, "");
		SendDlgItemMessage(hDlg, IDCB_LISTSUBSONGS, CB_RESETCONTENT, 0, 0);
		dwIndex = LB_ERR;				// No valid selection
		bDontChange = false;
		break;

	case OWM_SETSELECTEDINFO:
	{
		PlayListEntry* sel;
		LONG selIndex = SendDlgItemMessage(hDlg, IDLB_LISTCONTENTS, LB_GETCURSEL, 0, 0);
		if (selIndex == LB_ERR || (sel = myPlayList->AtIndex(selIndex)) == NULL)
			SendMessage(hDlg, OWM_CLEARINFO, 0, 0);
		else
		{
			SetDlgItemTextA(hDlg, IDC_LISTFILENAME, sel->Filename().c_str());
			SetDlgItemTextA(hDlg, IDC_LISTTITLE, sel->Title().c_str());
			SetDlgItemTextA(hDlg, IDC_LISTAUTHOR, sel->Author().c_str());
			SetDlgItemTextA(hDlg, IDC_LISTCOPYRIGHT, sel->Released().c_str());
			SetDlgItemTime(hDlg, IDE_LISTTIME, sel->PlayTime());
			if (sel->FadeoutTime() >= 0)
				SetDlgItemInt(hDlg, IDE_LISTFADEOUT, sel->FadeoutTime(), TRUE);
			else
				SetDlgItemTextA(hDlg, IDE_LISTFADEOUT, "");
			sel->FillComboBoxWithSubSongs(GetDlgItem(hDlg, IDCB_LISTSUBSONGS));
			if (sel->UseSLDB())
				CheckDlgButton(hDlg, IDC_CBSLDB, TRUE);
			else
				CheckDlgButton(hDlg, IDC_CBSLDB, FALSE);
		}
		break;
	}

	case OWM_SETLISTINFO:
		// Clear selected entry info
		SendMessage(hDlg, OWM_CLEARINFO, 0, 0);
		myPlayList->FillListBox(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
		CheckDlgButton(hDlg, IDC_LISTREPEAT, myPlayList->bRepeat ? BST_CHECKED : BST_UNCHECKED);
		CheckRadioButton(hDlg, IDB_LISTORDERNORMAL, IDB_LISTORDERRANDOM, IDB_LISTORDERNORMAL + myPlayList->order);
		SetDlgItemInt(hDlg, IDE_LISTDEFAULTTIME, myPlayList->nDefPlayTime, TRUE);
		SetDlgItemInt(hDlg, IDE_LISTDEFAULTFADEOUT, myPlayList->nDefFadeoutTime, TRUE);

		// Shuffled order not yet implemented
		EnableWindow(GetDlgItem(hDlg, IDB_LISTORDERSHUFFLED),FALSE);
		break;

	case OWM_ADDINDICESTOLIST:
	{
		char* szSel;
		nCount = wParam;
		string newdir;
		int* indices = (int*)lParam;
		LONG idx;
		bool bAddSubsongs = (IsDlgButtonChecked(hDlg, IDC_LISTADDSUBSONGS) == BST_CHECKED);
		bool bAddSubdirs = (IsDlgButtonChecked(hDlg, IDC_LISTRECURSEDIRS) == BST_CHECKED);

		// Get the right directory
		// Todo: Get rid of char pointers for more safe memory operations
		GetDlgItemTextA(hDlg, IDC_LISTDIR, szCurrentDir, MAX_PATH);
		string currentDir(szCurrentDir);
		if (currentDir.substr(currentDir.length() - 1, 1) != "\\")
			currentDir += "\\";

		for (int i = 0; i < nCount; ++i)
		{
			idx = indices[i];
			dwLength = SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_GETTEXTLEN, idx, 0);
			szSel = new char[dwLength + 1];
			SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_GETTEXT, idx, (LPARAM)szSel);

			// Remove selection
			SendDlgItemMessageA(hDlg, IDLB_LISTFILES, LB_SETSEL, FALSE, idx);

			// Drive or directory selected?
			if (szSel[0] == '[' && szSel[dwLength - 1] == ']')
			{
				if (bAddSubdirs && szSel[1] != '.' && szSel[1] != '-')
				{
					// Recursively add Dir
					string newdir = currentDir + string(szSel);
					myPlayList->AddDir(newdir, szFilter, bAddSubsongs);
				}
				delete[] szSel;
				continue;
			}

			newdir = currentDir + string(szSel);
			delete[] szSel;
			PlayListEntry* entry = new PlayListEntry(newdir);
			if (bAddSubsongs && entry->NumSubSongs() > 1)
			{
				entry->SetSubSong(1);
				myPlayList->Add(entry);
				for (int i = 2; i <= entry->NumSubSongs(); ++i)
				{
					entry = new PlayListEntry(newdir);
					entry->SetSubSong(i);
					myPlayList->Add(entry);
				}
			}
			else
			{
				myPlayList->Add(entry);
			}
		}
		myPlayList->FillListBox(GetDlgItem(hDlg, IDLB_LISTCONTENTS));
		SendDlgItemMessageA(hDlg, IDLB_LISTCONTENTS, LB_SETCURSEL, dwIndex, 0);
		return TRUE;

	}

	}
	return FALSE;
}

extern HWND hMain;
extern HINSTANCE hInst;

LRESULT CALLBACK PlayList::ViewPlayListProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD dwIndex;
	static FileExtensions* extensions;
	static HWND hListView, hButtEdit;
	static HFONT myFont;

	switch (message)
	{
	case WM_CREATE:
	{
		RECT rc;
		HFONT hSysFont;

		DragAcceptFiles(hwnd, TRUE);
		GetWindowRect(hwnd, &rc);
		hListView = CreateWindow(L"Listbox", NULL,
			WS_CHILDWINDOW | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT,
			0, 0, 80, 80,
			hwnd, (HMENU)IDLB_LISTVIEW,
			hInst, NULL);
		hButtEdit = CreateWindow(L"button", L"Edit",
			WS_CHILDWINDOW | WS_VISIBLE | BS_PUSHBUTTON,
			100, 100, 50, 25,
			hwnd, (HMENU)IDB_LISTEDIT,
			hInst, NULL);

		hSysFont = GetStockFont(DEFAULT_GUI_FONT);
		SendMessage(hListView, WM_SETFONT, (WPARAM)hSysFont, 0);
		SendMessage(hButtEdit, WM_SETFONT, (WPARAM)hSysFont, 0);
		if (myPlayList)
			myPlayList->FillListBox(hListView, TRUE);
		myPlayList->AddDependent(hwnd);
		return FALSE;
	}

	case OWM_SETFILEEXTENSIONS:
		// TODO must be easier way to pass init data to a window
		extensions = (FileExtensions*)lParam;
		break;

	case OWM_LISTCHANGED:
		if (myPlayList)
		{
			DWORD dwTopIndex = SendMessage(hListView, LB_GETTOPINDEX, 0, 0);
			dwIndex = SendMessage(hListView, LB_GETCURSEL, 0, 0);
			myPlayList->FillListBox(hListView, TRUE);
			SendMessage(hListView, LB_SETTOPINDEX, dwTopIndex, 0);
			SendMessage(hListView, LB_SETCURSEL, dwIndex, 0);
			SendMessage(hListView, WM_SETFONT, (WPARAM)myFont, 0);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDLB_LISTVIEW:
			switch (HIWORD(wParam))
			{
			case LBN_DBLCLK:
				dwIndex = SendMessage(hListView, LB_GETCURSEL, 0, 0);
				if (dwIndex == (DWORD)LB_ERR)
					return FALSE;
				if (myPlayList)
				{
					myPlayList->SetCurrent(dwIndex);
					SendMessage(hListView, LB_SETCURSEL, dwIndex, 0);
					SendMessage(hMain, OWM_PLAYENTRY, 0, 0);
					// TODO varför funkar hMain, men inte GetParent(hwnd) ???
				}
				break;

			case CBN_SELCHANGE:
				// TODO show appropriate info
				break;
			}
			break;

		case IDB_LISTEDIT:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_EDITPLAYLIST), hwnd, (DLGPROC)EditPlayListProc, (LPARAM)extensions);
			if (myPlayList)
				myPlayList->FillListBox(hListView, TRUE);
			break;

		case IDCANCEL:
			SendMessage(hMain, WM_COMMAND, ID_VIEW_PLAYLIST, 0L);
			return TRUE;
		}
		break;

	case WM_VKEYTOITEM:
		if (LOWORD(wParam) == VK_RETURN)
		{
			SendMessage(hwnd, WM_COMMAND, MAKELONG(IDLB_LISTVIEW, LBN_DBLCLK), (LPARAM)hListView);
			return -2;
		}
		return -1;

	case WM_DROPFILES:
	{
		char filename[MAX_PATH];
		int count = DragQueryFile((HDROP)wParam, -1, NULL, 0);
		for (int i = 0; i < count; ++i)
		{
			if (myPlayList)
			{
				DragQueryFileA((HDROP)wParam, i, filename, sizeof(filename));
				myPlayList->Add(new PlayListEntry(string(filename)));

			}
		}
		DragFinish((HDROP)wParam);
		break;
	}
	
	case WM_SIZE:
		MoveWindow(hListView, 4, 4, LOWORD(lParam) - 4 - 4, HIWORD(lParam) - 30 - 10 - 4, TRUE);
		MoveWindow(hButtEdit, LOWORD(lParam) - 4 - 50, HIWORD(lParam) - 25 - 4, 50, 25, TRUE);
		break;

	case WM_CLOSE:
		SendMessage(hMain, WM_COMMAND, ID_VIEW_PLAYLIST, 0L);
		SetFocus(hMain);
		break;

	case WM_SETFONT:
		myFont = (HFONT)wParam;
		SendMessage(hListView, WM_COMMAND, MAKELONG(IDLB_LISTVIEW, WM_SETFONT), (LPARAM)myFont);
		SendMessage(hListView, WM_COMMAND, MAKELONG(IDB_LISTEDIT, WM_SETFONT), (LPARAM)myFont);
		break;

	case WM_DESTROY:
	{
		WINDOWPLACEMENT wpl;
		wpl.length = sizeof(WINDOWPLACEMENT);
		if (GetWindowPlacement(hwnd, &wpl))
		{
			WriteMyProfileInt("WINDOWS", "ListViewPosX", wpl.rcNormalPosition.left,szIni);
			WriteMyProfileInt("WINDOWS", "ListViewPosY", wpl.rcNormalPosition.top, szIni);
			WriteMyProfileInt("WINDOWS", "ListViewExtX", wpl.rcNormalPosition.right - wpl.rcNormalPosition.left, szIni);
			WriteMyProfileInt("Windows", "ListViewExtY", wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top, szIni);
		}
		myPlayList->RemoveDependent(hwnd);
		break;
	}
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

HWND PlayList::CreateListViewWindow(HWND hParent)
{
	const wchar_t* szClassName = L"ListView";

	WNDCLASSEX wndclass;
	if (GetClassInfoEx(hInst, szClassName, &wndclass) == FALSE)
	{
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = ViewPlayListProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInst;
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = szClassName;
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.hIconSm = (HICON)LoadImageA(hInst, MAKEINTRESOURCEA(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		RegisterClassEx(&wndclass);
	}

	HWND tmp =  CreateWindow(szClassName,
		L"Playlist view",
		WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX,
		GetMyProfileInt("WINDOWS", "ListViewPosX", CW_USEDEFAULT, szIni),
		GetMyProfileInt("WINDOWS", "ListViewPosY", CW_USEDEFAULT, szIni),
		GetMyProfileInt("WINDOWS", "ListViewExtX", CW_USEDEFAULT, szIni),
		GetMyProfileInt("WINDOWS", "ListViewExtY", CW_USEDEFAULT, szIni),
		hParent,
		(HMENU)NULL,
		hInst,
		(LPVOID)NULL);

	return tmp;
}