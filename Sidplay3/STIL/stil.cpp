//
// STIL class - Implementation file
//
// AUTHOR: LaLa
// Email : LaLa@C64.org
// Copyright (C) 1998, 2002 by LaLa
// Copyright (C) 2019 by Inge H. Pedersen

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include "stil.h"

using namespace std;


#define STILopenFlags (ios::in | ios::binary)

const double VERSION_NO = 2.20;

#define CERR_STIL_DEBUG if (STIL_DEBUG) cerr << "Line #" << __LINE__ << " STIL::"

STIL::STIL()
{
	ostringstream vs;
	vs << std::fixed << std::setprecision(2) << VERSION_NO;
	versionString = "STILView v" + vs.str() + "\n(C) 1998, 2002 by LaLa (LaLa@C64.org).\n(C) 2019 by Inge H. Pedersen (ingehp@outlook.com)\n";

	STILVersion = 0.0;
	baseDir = "";
	baseDirLength = 0;
	entrybuf = "";
	globalbuf = "";
	bugbuf = "";
	resultEntry = "";
	resultBug = "";


	// Just so we have the first one in here
	stilDirs = createOneDir();
	bugDirs = createOneDir();
	
	STIL_EOL = '\n';
	STIL_EOL2 = '\0';

	STIL_DEBUG = false;
	lastError = NO_STIL_ERROR;
}

STIL::~STIL()
{

}

const string
STIL::getVersion()
{
	lastError = NO_STIL_ERROR;
	return versionString;
}

const double
STIL::getVersionNo()
{
	lastError = NO_STIL_ERROR;
	return VERSION_NO;
}

double
STIL::getSTILVersionNo()
{
	lastError = NO_STIL_ERROR;
	return STILVersion;
}

bool
STIL::setBaseDir(const char* pathToHVSC)
{
	string tempName;
	string tempBaseDir;

	// Temporary version number/copyright string
	string tempVersionString;

	// Temporary placeholder for STIL.txt's verion number
	double tempSTILVersion = STILVersion;

	// Temporary placeholders for lists of sections
	dirList* tempStilDirs, * tempBugDirs;

	lastError = NO_STIL_ERROR;

	CERR_STIL_DEBUG << "setBaseDir() called, pathToHVSC=" << pathToHVSC << endl;

	// Sanity check the length.
	if (strlen(pathToHVSC) < 1) {
		CERR_STIL_DEBUG << "setBaseDir() has problem with the size of pathToHVSC" << endl;
		lastError = BASE_DIR_LENGTH;
		return false;
	}

	tempBaseDir = string(pathToHVSC);

	// Chop trailing slash
	if (tempBaseDir[tempBaseDir.length() - 1] == SLASH)
	{
		tempBaseDir = tempBaseDir.substr(0, tempBaseDir.length() - 1);
	}

	// Create full path + file name to STIL
	tempName = tempBaseDir + PATH_TO_STIL;
	convertSlashes(tempName);

	// Attempt to open STIL
	stilFile.clear();
	stilFile.open(tempName, STILopenFlags);

	if (stilFile.fail()) {
		stilFile.close();
		CERR_STIL_DEBUG << "setBaseDir() open failed for " << tempName << endl;
		lastError = STIL_OPEN;
		return false;
	}


	CERR_STIL_DEBUG << "setBaseDir(): open succeeded for " << tempName << endl;

	// Create full path + file name to BUGlist
	tempName = tempBaseDir + PATH_TO_BUGLIST;
	convertSlashes(tempName);

	// Attempt to open BUGList
	bugFile.clear();
	bugFile.open(tempName, STILopenFlags);


	if (bugFile.fail()) {

		// This is not a critical error - some earlier versions of HVSC did
		// not have a BUGlist.txt file at all.

		bugFile.close();
		CERR_STIL_DEBUG << "setBaseDir() open failed for " << tempName << endl;
		lastError = BUG_OPEN;

	}
	
	CERR_STIL_DEBUG << "setBaseDir(): open succeeded for " << tempName << endl;

	// Find out what the EOL really is
	if (determineEOL() != true) {
		stilFile.close();
		bugFile.close();
		CERR_STIL_DEBUG << "determinEOL() failed" << endl;
		lastError = NO_EOL;
		return false;
	}

	// Save away the current string so we can restore it if needed.
	tempVersionString = versionString;
	ostringstream vs;
	vs << std::fixed << std::setprecision(2) << VERSION_NO;
	versionString = "STILView v" + vs.str() + "\n(C) 1998, 2002 by LaLa (LaLa@C64.org).\n(C) 2019 by Inge H. Pedersen (ingehp@outlook.com)\n";

	// This is necessary so the version number gets scanned in from the new file, too.
	STILVersion = 0.0;

	// These will populate the tempStilDirs and tempBugDirs arrays (or not :)
	tempStilDirs = createOneDir();
	tempBugDirs = createOneDir();

	if (getDirs(stilFile, tempStilDirs, true) != true)
	{
		stilFile.close();
		bugFile.close();
		CERR_STIL_DEBUG << "getDirs() failed for stilFile" << endl;
		lastError = NO_STIL_DIRS;

		// Clean up and restore things
		deleteDirList(tempStilDirs);
		deleteDirList(tempBugDirs);
		versionString = tempVersionString;
		STILVersion = tempSTILVersion;
		return false;
	}

	if (bugFile.good())
	{
		if (getDirs(bugFile, tempBugDirs, false) != true) {

			// This is not a critical error - it is possible that the
			// BUGlist.txt file has no entries in it at all (in fact, that's
			// good!).

			CERR_STIL_DEBUG << "getDirs() failed for bugFile" << endl;
			lastError = BUG_OPEN;
		}
	}

	stilFile.close();
	bugFile.close();

	// Now we can copy the stuff into private data.
	// NOTE: At this point, STILVersion and the versionString should contain
	// the new info!

	// First, delete what may have been there previously.
	if (baseDir != "") {
		baseDir.clear();
	}

	baseDir = tempBaseDir;

	// First, delete whatever may have been there previously
	deleteDirList(stilDirs);
	deleteDirList(bugDirs);

	stilDirs = createOneDir();
	bugDirs = createOneDir();

	// New proceed with copy
	copyDirList(stilDirs, tempStilDirs);
	copyDirList(bugDirs, tempBugDirs);

	// Cleanup
	deleteDirList(tempStilDirs);
	deleteDirList(tempBugDirs);
	
	CERR_STIL_DEBUG << "setBaseDir() succeeded" << endl;

	return true;
}

string
STIL::getAbsEntry(const string absPathToEntry, int tuneNo, STILField field)
{
	string tempDir;

	lastError = NO_STIL_ERROR;

	CERR_STIL_DEBUG << "getAbsEntry() called, absPathToEntry=" << absPathToEntry << endl;

	if (baseDir == "")
	{
		CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
		lastError = STIL_OPEN;

		return "";
	}

	// Determine if the baseDir is in the given pathname
	if (absPathToEntry.length() > baseDir.length())
	{
		if (absPathToEntry.substr(0, baseDir.length()) != baseDir)
		{
			CERR_STIL_DEBUG << "getAbsEntry() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
			lastError = WRONG_DIR;
			return "";
		}
	}
	else
	{
		CERR_STIL_DEBUG << "getAbsEntry() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
		lastError = WRONG_DIR;
		return "";
	}

	int l = absPathToEntry.length() - baseDir.length();
	tempDir = absPathToEntry.substr(baseDir.length(), l);
	convertToSlashes(tempDir);
	return (getEntry(tempDir, tuneNo, field));

}

string
STIL::getEntry(const string relPathToEntry, int tuneNo, STILField field)
{
	string tempName;

	lastError = NO_STIL_ERROR;

	CERR_STIL_DEBUG << "getEntry() called, relPath=" << relPathToEntry << ", rest=" << tuneNo << "," << field << endl;

	if (baseDir == "") {
		CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
		lastError = STIL_OPEN;
		return "";
	}

	// Fail if a section-global comment was asked for
	if (relPathToEntry[relPathToEntry.length() - 1] == '/')
	{
		CERR_STIL_DEBUG << "getEntry() section-global comment was asked for - failed" << endl;
		lastError = WRONG_ENTRY;
		return "";
	}

	if (STILVersion < 2.59) {

		// Older version of STIL is detected.

		tuneNo = 0;
		field = all;
	}


	// Find out whether we have this entry in the buffer

		if ((entrybuf.compare(0,relPathToEntry.length(),relPathToEntry) != 0) || ((entrybuf.find_first_of('\n') != relPathToEntry.length()) && (STILVersion > 2.59)))
		{
			// The relative pathnames don't match or they're not the same length:
			// we don't have it in the buffer, so pull it in.

			CERR_STIL_DEBUG << "getEntry(): entry not in buffer" << endl;

			tempName = baseDir + PATH_TO_STIL;
			convertSlashes(tempName);

			stilFile.clear();
			stilFile.open(tempName, STILopenFlags);
			if (stilFile.fail()) {
				stilFile.close();
				CERR_STIL_DEBUG << "getEntry() open failed for stilFile" << endl;
				lastError = STIL_OPEN;
				return "";
			}

			CERR_STIL_DEBUG << "getEntry() open succeeded for stilFile" << endl;

			if (positionToEntry(relPathToEntry, stilFile, stilDirs) == false)
			{
				entrybuf = relPathToEntry + "\n";
				CERR_STIL_DEBUG << "getEntry() posToEntry() failed" << endl;
				lastError = NOT_IN_STIL;
			}
			else
			{
				entrybuf = "";
				readEntry(stilFile, entrybuf);
			}

			stilFile.close();
		}
		
		// Put the requested field into the result string

		if (getField(resultEntry, entrybuf, tuneNo, field) != true)
		{
			return "";
		}
		else
		{
			return resultEntry;
		}
}

string
STIL::getAbsBug(const string absPathToEntry, int tuneNo)
{
	string tempDir;
	int tempDirLength;
	
	lastError = NO_STIL_ERROR;

	CERR_STIL_DEBUG << "getAbsBug() called, absPathToEntry=" << absPathToEntry << endl;

	if (baseDir == "") {
		CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
		lastError = BUG_OPEN;
		return "";
	}

	// Determine if the baseDir is in the given pathname
	if (absPathToEntry.substr(0, baseDir.length()) != baseDir)
	{
		CERR_STIL_DEBUG << "getAbsBug() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
		lastError = WRONG_DIR;
		return "";
	}

	tempDirLength = absPathToEntry.length() - baseDir.length();
	tempDir = absPathToEntry.substr(baseDir.length(), tempDirLength);
	convertToSlashes(tempDir);

	return getBug(tempDir, tuneNo);
}

string
STIL::getBug(const string relPathToEntry, int tuneNo)
{
	string tempName;


	lastError = NO_STIL_ERROR;

	CERR_STIL_DEBUG << "getBug() called, relPath=" << relPathToEntry << ", rest=" << tuneNo << endl;

	if (baseDir == "") {
		CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
		lastError = BUG_OPEN;
		return "";
	}

	// Older version of STIL is detected.

	if (STILVersion < 2.59) {
		tuneNo = 0;
	}

	// Find out whether we have this bug entry in the buffer.
	// If the baseDir was changed, we'll have to read it in again,
	// even if it might be in the buffer already.
	if ((bugbuf.compare(0, relPathToEntry.length(), relPathToEntry) != 0) || ((bugbuf.find_first_of('\n') != relPathToEntry.length()) && (STILVersion > 2.59)))
	{
		// The relative pathnames don't match or they're not the same length:
		// we don't have it in the buffer, so pull it in.
		CERR_STIL_DEBUG << "getBug(): entry not in buffer" << endl;

		tempName = baseDir + PATH_TO_BUGLIST;
		convertSlashes(tempName);

		bugFile.clear();
		bugFile.open(tempName, STILopenFlags);

		if (bugFile.fail())
		{
			bugFile.close();
			CERR_STIL_DEBUG << "getBug() open failed for bugFile" << endl;
			lastError = BUG_OPEN;
			return "";
		}

		CERR_STIL_DEBUG << "getBug() open succeeded for bugFile" << endl;

		if (positionToEntry(relPathToEntry, bugFile, bugDirs) == false)
		{
			bugbuf = relPathToEntry + '\n';
			CERR_STIL_DEBUG << "getBug() posToEntry() failed" << endl;
			lastError = NOT_IN_BUG;
		}
		else
		{
			bugbuf = "";
			readEntry(bugFile, bugbuf);
			CERR_STIL_DEBUG << "getBug() entry read" << endl;
		}
		bugFile.close();
	}
	if (getField(resultBug, bugbuf, tuneNo) != true)
	{
		return "";
	}
	else
	{
		return resultBug;
	}
}

string
STIL::getAbsGlobalComment(const string absPathToEntry)
{
	string tempDir;
	int tempDirLength;

	lastError = NO_STIL_ERROR;

	CERR_STIL_DEBUG << "getAbsGC() called, absPathToEntry=" << absPathToEntry << endl;

	if (baseDir == "") {
		CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
		lastError = STIL_OPEN;
		return "";
	}

	if (absPathToEntry.substr(0, baseDir.length()) != baseDir)
	{
		CERR_STIL_DEBUG << "getAbsGC() failed: baseDir=" << baseDir << ", absPath=" << absPathToEntry << endl;
		lastError = WRONG_DIR;
		return "";
	}

	tempDirLength = absPathToEntry.length() - baseDir.length();
	tempDir = absPathToEntry.substr(baseDir.length(), tempDirLength);
	convertToSlashes(tempDir);

	return getGlobalComment(tempDir);
}

string
STIL::getGlobalComment(const string relPathToEntry)
{
	string dir;
	string tempName;
	size_t pathLen;
	int temp;
	int lastSlash;


	lastError = NO_STIL_ERROR;

	CERR_STIL_DEBUG << "getGC() called, relPath=" << relPathToEntry << endl;

	if (baseDir == "") {
		CERR_STIL_DEBUG << "HVSC baseDir is not yet set!" << endl;
		lastError = STIL_OPEN;
		return "";
	}

	// Save the path
	lastSlash = relPathToEntry.find_last_of('/');

	if (lastSlash == string::npos)
	{
		lastError = WRONG_DIR;
		return "";
	}

	pathLen = lastSlash + 1;
	dir = relPathToEntry.substr(0, pathLen);

	// Find out whether we have this global comment in the buffer.
	// If the baseDir was changed, we'll have to read it in again,
	// even if it might be in the buffer already.
	if ((globalbuf.compare(0, pathLen, dir) != 0) || ((globalbuf.find_first_of('\n') != pathLen) && (STILVersion > 2.59)))
	{
		// The Relative pathnames don't match or they're not the same length:
		// We don't have it in the buffer so pull it in
		CERR_STIL_DEBUG << "getGC(): entry not in buffer" << endl;

		tempName = baseDir + PATH_TO_STIL;
		convertSlashes(tempName);

		stilFile.clear();
		stilFile.open(tempName, STILopenFlags);

		if (stilFile.fail())
		{
			stilFile.close();
			CERR_STIL_DEBUG << "getGC() open failed for stilFile" << endl;
			lastError = STIL_OPEN;
			return "";
		}

		if (positionToEntry(dir, stilFile, stilDirs) == false)
		{
			globalbuf = dir + '\n';
			CERR_STIL_DEBUG << "getGC() posToEntry() failed" << endl;
			lastError = NOT_IN_STIL;
		}
		else
		{
			globalbuf = "";
			readEntry(stilFile, globalbuf);
			CERR_STIL_DEBUG << "getGC() entry read" << endl;
		}
		stilFile.close();
	}

	CERR_STIL_DEBUG << "getGC() globalbuf=" << globalbuf << endl;
	CERR_STIL_DEBUG << "-=END=-" << endl;

	// Position pointer to the global comment field.
	temp = globalbuf.find_first_of('\n');
	temp++;

	if (temp == globalbuf.length())
	{
		return "";
	}
	else
	{
		return globalbuf.substr(temp, STIL_MAX_ENTRY_SIZE);
	}

}
//////// PRIVATE

bool
STIL::determineEOL()
{
	char line[STIL_MAX_LINE_SIZE + 5];
	int i = 0;

	CERR_STIL_DEBUG << "detEOL() called" << endl;

	if (stilFile.fail()) {
		CERR_STIL_DEBUG << "detEOL() open failed" << endl;
		return false;
	}

	stilFile.seekg(0);

	// Read in the first line from stilFile to determine what the
	// EOL character is (it can be different from OS to OS).

	stilFile.read(line, sizeof(line) - 1);
	line[sizeof(line) - 1] = '\0';

	CERR_STIL_DEBUG << "detEOL() line=" << line << endl;

	// Now find out what the EOL char is (or are).

	STIL_EOL = '\0';
	STIL_EOL2 = '\0';

	while (line[i] != '\0') {
		if ((line[i] == 0x0d) || (line[i] == 0x0a)) {
			if (STIL_EOL == '\0') {
				STIL_EOL = line[i];
			}
			else {
				if (line[i] != STIL_EOL) {
					STIL_EOL2 = line[i];
				}
			}
		}
		i++;
	}

	if (STIL_EOL == '\0') {
		// Something is wrong - no EOL-like char was found.
		CERR_STIL_DEBUG << "detEOL() no EOL found" << endl;
		return false;
	}

	CERR_STIL_DEBUG << "detEOL() EOL1=0x" << hex << (int)STIL_EOL << " EOL2=0x" << hex << (int)STIL_EOL2 << dec << endl;

	return true;
}

bool
STIL::getDirs(ifstream& inFile, dirList* dirs, bool isSTILFile)
{
	char line[STIL_MAX_LINE_SIZE];
	int i = 0;
	size_t j;
	bool newDir;
	dirList* prevDir;

	if (isSTILFile)
		newDir = false;
	else
		newDir = true;

	prevDir = dirs;

	CERR_STIL_DEBUG << "getDirs() called" << endl;

	inFile.seekg(0);

	while (inFile.good())
	{
		getStilLine(inFile, line);

		if (!isSTILFile)
			CERR_STIL_DEBUG << line << '\n';

		// Try to extract STIL's bersion number if it's not done yet

		if (isSTILFile && (STILVersion == 0.0))
		{
			if (strncmp(line, "#  STIL v", 9) == 0)
			{
				STILVersion = atof(line + 9);
				ostringstream vs;
				vs << std::fixed << std::setprecision(2) << STILVersion;
				versionString = "SID Tune Information List (STIL) v" + vs.str() + "\n";

				CERR_STIL_DEBUG << "getDirs() STILVersion=" << STILVersion << endl;

				continue;
			}
		}

		// Search for the start of a dir separator first
		if (isSTILFile && !newDir && (MYSTRNICMP(line, "### ", 4) == 0))
		{
			newDir = true;
			continue;
		}

		// Is this the start of an entry immediately following a dir separator?
		if (newDir && (*line == '/'))
		{
			// Get the Directory only
			j = strrchr(line, '/') - line + 1;

			if (!isSTILFile)
			{
				// Compare to the last stored dirname
				if (i == 0)
				{
					newDir = true;
				}
				else if (prevDir->dirName.substr(0, j) != line)
				{
					newDir = true;
				}
				else
				{
					newDir = false;
				}
			}

			// Store the info
			if (newDir)
			{
				string tempStr = line;
				dirs->dirName = tempStr.substr(0, j);
				dirs->position = inFile.tellg() - (streampos)strlen(line) - 1L;

				CERR_STIL_DEBUG << "getDirs() i=" << i << ", dirName=" << dirs->dirName << ", pos=" << dirs->position << endl;


				prevDir = dirs;

				// We create the entries one ahead. This way we also assure that
				// the last entry will always be a NULL (empty) entry.
				dirs->next = createOneDir();
				dirs = dirs->next;

				i++;
			}

			if (isSTILFile)
			{
				newDir = false;
			}
			else
			{
				newDir = true;
			}
		}
	}

	if (i == 0) {
		// No entries found - something is wrong.
		// NOTE: It's perfectly valid to have a BUGlist.txt file with no
		// entries in it!
		CERR_STIL_DEBUG << "getDirs() no dirs found" << endl;
		return false;
	}

	CERR_STIL_DEBUG << "getDirs() successful" << endl;

	return true;
}

bool
STIL::positionToEntry(const string entryStr, ifstream& inFile, dirList* dirs)
{
	char line[STIL_MAX_LINE_SIZE];
	int temp;
	bool foundIt = false;
	bool globComm = false;

	CERR_STIL_DEBUG << "pos2Entry() called, entryStr=" << entryStr << endl;

	inFile.seekg(0);

	// Get the dirpath.
	int chrptr = entryStr.find_last_of('/');

	// If no slash was found, something is screwed up in the entryStr
	if (chrptr == string::npos)
	{
		return false;
	}

	int pathLen = chrptr + 1;
	// Determine whether a section-global comment is asked for
	if (pathLen == entryStr.length())
	{
		globComm = true;
	}

	// Find it in the table
	while (dirs->dirName != "")
	{
		if ((entryStr.compare(0, pathLen, dirs->dirName) == 0) && (dirs->dirName.length() == pathLen))
		{
			CERR_STIL_DEBUG << "pos2Entry() found dir, dirName=" << dirs->dirName << endl;
			foundIt = true;
			break;
		}

		dirs = dirs->next;
	}

	if (!foundIt)
	{
		// The directory was not found.
		CERR_STIL_DEBUG << "pos2Entry() did not find the dir" << endl;
		return false;
	}

	// Jump to the first entry of this section.
	inFile.seekg(dirs->position);
	foundIt = false;

	// Now find the desired entry
	do
	{
		getStilLine(inFile, line);
		if (inFile.eof()) {
			break;
		}

		// Check if it is the start of an entry
		if (*line == '/')
		{
			if (MYSTRNICMP(dirs->dirName.c_str(), line, pathLen) != 0)
			{
				// We are outside the section - get out of the loop
				// which will fail the search
				break;
			}

			// Check whether we need to find a section-global comment or
			// a specific entry
			if (globComm || (STILVersion > 2.59))
			{
				temp = MYSTRICMP(line, entryStr.c_str());
			}
			else
			{
				// To be compatible with older versions of STIL, which may have
				// the tune designation on the first line of a STIL entry
				// together with the pathname
				temp = MYSTRNICMP(line, entryStr.c_str(), entryStr.length());
			}

			CERR_STIL_DEBUG << "pos2Entry() line=" << line << endl;

			if (temp == 0)
			{
				// Found it!
				foundIt = true;
			}
		}
	} while (!foundIt);

	if (foundIt)
	{
		// Reposition the file pointer back to the start of the entry
		inFile.seekg(inFile.tellg() - (streampos)strlen(line) - 1L);
		CERR_STIL_DEBUG << "pos2Entry() entry found" << endl;
		return true;
	}
	else
	{
		CERR_STIL_DEBUG << "pos2Entry() entry not found" << endl;
		return false;
	}
}

void
STIL::readEntry(ifstream& inFile, string& buffer)
{
	char line[STIL_MAX_LINE_SIZE];

	do
	{
		getStilLine(inFile, line);
		buffer += line;
		if (*line != '\0')
		{
			buffer += '\n';
		}

	} while (*line != '\0');
}

bool
STIL::getField(string& result, string& buffer, int tuneNo, STILField field)
{
	size_t start, firstTuneNo, temp, temp2 = 0;

	CERR_STIL_DEBUG << "getField() called, buffer=" << buffer << ", rest=" << tuneNo << "," << field << endl;

	// Clean out thre result buffer first
	result.clear();

	// Position pointer to the first char beyond the file designation.
	start = buffer.find_first_of('\n');
	start++;

	// Check whether this is a NULL entry or not
	if (start == buffer.length())
	{
		CERR_STIL_DEBUG << "getField() null entry" << endl;
		return false;
	}

	// Is this a multitune entry?
	firstTuneNo = buffer.find("(#", start);

	// This is a tune designation onlu if the previous char was
	// a newline (ie if the "(#" is on the beginning of a line"
	if ((firstTuneNo != string::npos) && (buffer[firstTuneNo - 1] != '\n'))
	{
		firstTuneNo = string::npos;
	}

	if (firstTuneNo == string::npos)
	{
		//-------------------//
		// SINGLE TUNE ENTRY //
		//-------------------//

		// Is the first thing in this STIL entry the COMMENT?
		temp = buffer.find(_COMMENT_STR, start);
		
		// Sarch for other potential fields beyond the COMMENT
		if (temp == start)
		{
			temp2 = buffer.find(_NAME_STR, start);
			if (temp2 == string::npos)
			{
				temp2 = buffer.find(_AUTHOR_STR, start);
				if (temp2 == string::npos)
				{
					temp2 = buffer.find(_TITLE_STR, start);
					if (temp2 == string::npos)
					{
						temp2 = buffer.find(_ARTIST_STR, start);
					}
				}
			}
		}

		if (temp == start)
		{
			// Yes. So it's assumed to be a file-global comment.

			CERR_STIL_DEBUG << "getField() single-tune entry, COMMENT only" << endl;

			if ((tuneNo == 0) && ((field == all) || ((field == comment) && (temp2 == string::npos))))
			{
				// Simply copy the stuff in
				result = buffer.substr(start, STIL_MAX_ENTRY_SIZE - 1);
				CERR_STIL_DEBUG << "getField() copied to resultbuf" << endl;
				return true;
			}
			else if ((tuneNo == 0) && (field == comment))
			{
				// Copy just the comment
				result = buffer.substr(start, temp2 - start);
				CERR_STIL_DEBUG << "getField() copied to just the COMMENT to resultbuf" << endl;
				return true;
			}
			else if ((tuneNo == 1) && (temp2 != string::npos))
			{
				// A specified field was asked for
				CERR_STIL_DEBUG << "getField() copying COMMENT to resultbuf" << endl;
				return getOneField(result, buffer, temp2, buffer.length() - 1, field);
			}
			else
			{

				// Anything else is invalid as of v2.00.
				CERR_STIL_DEBUG << "getField() invalid parameter combo: single tune, tuneNo=" << tuneNo << ", field=" << field << endl;
				return false;
			}
		}
		else
		{
			// No. Handle it as a regular entry
			CERR_STIL_DEBUG << "getField() single-tune regular entry" << endl;
			if ((field == all) && ((tuneNo == 0) || (tuneNo == 1)))
			{
				result = buffer.substr(start, STIL_MAX_ENTRY_SIZE);
				CERR_STIL_DEBUG << "getField() copied to resultbuf" << endl;
				return true;
			}
			else if (tuneNo == 1)
			{
				// A specific field was asked for
				CERR_STIL_DEBUG << "getField() copying COMMENT to resultbuf" << endl;
				return getOneField(result, buffer, start, buffer.length() - 1, field);
			}
			else
			{
				// Anything else is invalid as of v2.00.
				CERR_STIL_DEBUG << "getField() invalid parameter combo: single tune, tuneNo=" << tuneNo << ", field=" << field << endl;
				return false;
			}
		}
	}
	else
	{
		//-------------------//
		// MULTITUNE ENTRY
		//-------------------//

		CERR_STIL_DEBUG << "getField() multitune entry" << endl;

		// Was the complete entry asked for?
		if (tuneNo == 0)
		{
			switch (field)
			{
			case all:

				// Yes, simply copy the stuff in
				result = buffer.substr(start, STIL_MAX_ENTRY_SIZE - 1);
				CERR_STIL_DEBUG << "getField() copied all to resultbuf" << endl;
				return true;
				break;

			case comment:

				// Only the file-global comment field was asked for
				if (firstTuneNo != start)
				{
					CERR_STIL_DEBUG << "getField() copying file-global comment to resultbuf" << endl;
					return getOneField(result, buffer, start, firstTuneNo, comment);
				}
				else
				{
					CERR_STIL_DEBUG << "getField() no file-global comment" << endl;
					return false;
				}
				break;

			default:

				// If a specific field other than a comment is
				// asked for tuneNo = 0, this is illegal
				CERR_STIL_DEBUG << "getField() invalid parameter combo: multitune, tuneNo=" << tuneNo << ", field=" << field << endl;
				return false;
				break;
			}
		}

		int myTuneNo, nextTuneNo;
		char tuneNoStr[8];

		_snprintf_s(tuneNoStr, 7, "(#%d)", tuneNo);
		tuneNoStr[7] = '\0';
		myTuneNo = buffer.find(tuneNoStr, start);

		if (myTuneNo != string::npos)
		{

			// We found the requested tune number.
			// Set the pointer beyond it
			myTuneNo = buffer.find('\n', myTuneNo) + 1;

			// Where is the next one ?
			nextTuneNo = buffer.find("\n(#", myTuneNo);
			if (nextTuneNo == string::npos)
			{
				// There is no next one - set pointer to end of entry
				nextTuneNo = buffer.length();
			}
			else
			{
				nextTuneNo++;
			}

			// Put the desired fields into the result (which may be 'all')
			CERR_STIL_DEBUG << "getField() myTuneNo=" << myTuneNo << ", nextTuneNo=" << nextTuneNo << endl;
			return getOneField(result, buffer, myTuneNo, nextTuneNo, field);
		}

		else
		{
			CERR_STIL_DEBUG << "getField() nothing found" << endl;
			return false;
		}
	}
}

bool
STIL::getOneField(string& result, string& buffer, int start, int end, STILField field)
{
	int temp = 0;

	// Sanity check
	if ((end < start) || (buffer[end - 1] != '\n'))
	{
		result = "";
		CERR_STIL_DEBUG << "getOneField() illegal parameters" << endl;
		return false;
	}

	CERR_STIL_DEBUG << "getOneField() called, start=" << start << ", rest=" << field << endl;

	switch (field)
	{
	case all:
		result += buffer.substr(start, end - start);
		return true;
		break;

	case name:
		temp = buffer.find(_NAME_STR, start);
		break;

	case author:
		temp = buffer.find(_AUTHOR_STR, start);
		break;

	case title:
		temp = buffer.find(_TITLE_STR, start);
		break;

	case artist:
		temp = buffer.find(_ARTIST_STR, start);
		break;

	case comment:
		temp = buffer.find(_COMMENT_STR, start);
		break;

	default:
		break;

	}

	// If the field was not found or it is not in between 'start'
	// and 'end, it is declared a failure.
	if ((temp == string::npos) || (temp < start) || (temp > end))
	{
		result = "";
		return false;
	}

	// Search for end of this field. This is done by finding
	// where the next field starts.
	int nextName, nextAuthor, nextTitle, nextArtist, nextComment, nextField;

	nextName = buffer.find(_NAME_STR, temp + 1);
	nextAuthor = buffer.find(_AUTHOR_STR, temp + 1);
	nextTitle = buffer.find(_TITLE_STR, temp + 1);
	nextArtist = buffer.find(_ARTIST_STR, temp + 1);
	nextComment = buffer.find(_COMMENT_STR, temp + 1);

	// If any of these fields is beyond 'end', they are ignored
	if ((nextName != string::npos) && (nextName >= end))
	{
		nextName = string::npos;
	}

	if ((nextAuthor != string::npos) && (nextAuthor >= end))
	{
		nextAuthor = string::npos;
	}

	if ((nextTitle != string::npos) && (nextTitle >= end))
	{
		nextTitle = string::npos;
	}

	if ((nextArtist != string::npos) && (nextArtist >= end))
	{
		nextArtist = string::npos;
	}

	if ((nextComment != string::npos) && (nextComment >= end))
	{
		nextComment = string::npos;
	}

	// Now determine which one is the closest to our field - that one
	// will mark the end of the required field

	nextField = nextName;

	if (nextField == string::npos)
	{
		nextField = nextAuthor;
	}
	else if ((nextAuthor != string::npos) && (nextAuthor < nextField))
	{
		nextField = nextAuthor;
	}

	if (nextField == string::npos)
	{
		nextField = nextTitle;
	}
	else if ((nextTitle != string::npos) && (nextTitle < nextField))
	{
		nextField = nextTitle;
	}

	if (nextField == string::npos)
	{
		nextField = nextArtist;
	}
	else if ((nextArtist != string::npos) && (nextArtist < nextField))
	{
		nextField = nextArtist;
	}

	if (nextField == string::npos)
	{
		nextField = nextComment;
	}
	else if ((nextComment != string::npos) && (nextComment < nextField))
	{
		nextField = nextComment;
	}

	if (nextField == string::npos)
	{
		nextField = end;
	}

	result += buffer.substr(temp, nextField - temp);
	return true;
}

void
STIL::getStilLine(ifstream& infile, char* line)
{
	char temp;

	if (STIL_EOL2 != '\0')
	{
		// If there was a remaining EOL char from the previous read, eat it up
		temp = infile.peek();
		if ((temp == 0x0d) || (temp == 0x0a))
		{
			infile.get(temp);
		}
	}

	infile.getline(line, STIL_MAX_LINE_SIZE, STIL_EOL);
}

void
STIL::deleteDirList(dirList* dirs)
{
	dirList* ptr;

	do
	{
		ptr = dirs->next;
		delete dirs;
		dirs = ptr;
	} while (ptr);
}
void
STIL::copyDirList(dirList* toPtr, dirList* fromPtr)
{
	if ((toPtr != NULL) && (fromPtr != NULL))
	{
		do
		{
			toPtr->position = fromPtr->position;
			if (fromPtr->dirName != "")
			{
				toPtr->dirName = fromPtr->dirName;
			}
			else
			{
				toPtr->dirName = "";
			}

			if (fromPtr->next)
			{
				toPtr->next = createOneDir();
			}
			else
			{
				toPtr->next = NULL;
			}

			fromPtr = fromPtr->next;
			toPtr = toPtr->next;
		}
		while(fromPtr);
	}

}

STIL::dirList*
STIL::createOneDir(void)
{
	dirList* ptr;

	ptr = new dirList;
	ptr->dirName = "";
	ptr->position = 0;
	ptr->next = NULL;

	return ptr;
}

const std::string STIL::STIL_ERROR_STR[] = {
	"No error.",
	"Failed to open BUGlist.txt.",
	"Base dir path is not the HVSC base dir path.",
	"The entry was not found in STIL.txt.",
	"The entry was not found in BUGlist.txt.",
	"A section-global comment was asked for in the wrong way.",
	"",
	"",
	"",
	"",
	"CRITICAL ERROR",
	"Incorrect HVSC base dir length!",
	"Failed to open STIL.txt!",
	"Failed to determine EOL from STIL.txt!",
	"No STIL sections were found in STIL.txt!",
	"No STIL sections were found in BUGlist.txt!"
};