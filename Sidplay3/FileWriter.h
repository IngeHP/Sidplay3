#pragma once

#include <fstream>

using namespace std;

class FileWriter
{
public:
	FileWriter(const string filename);
	~FileWriter();

	int Status();

	void Write(const string name);
	void NewLine();

	void WritePairStr(const string name, const string value);
	void WritePairLong(const string name, const long value);
	void WritePairInt(const string name, const int value);
	void WritePairBool(const string name, bool value);

	enum { OK = 0, OPEN_ERROR = 1};

protected:
	ofstream myFile;
	int myStatus;
};

