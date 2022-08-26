#pragma once

#include <fstream>

using namespace std;

class FileReader
{
public:
	FileReader(const string filename);
	~FileReader();

	int Status();

	// Reads the next non-empty, non-comment line and places it in an internal string
	bool ReadLine();

	long ParsePairLong(const string name);
	int ParsePairInt(const string name);
	string ParsePairStr(const string name);
	bool ParsePairBool(const string name);

	enum { OK = 0, OPEN_ERROR = 1};

protected:
	int myStatus;
	ifstream myFile;
	string myLine;

	string FindPair(const string name);
};

