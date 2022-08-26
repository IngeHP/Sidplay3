#include "FileReader.h"
#include <string>

FileReader::FileReader(const string filename) : myStatus(OK)
{
	myFile = ifstream(filename);

	if (myFile.bad())
	{
		myStatus = OPEN_ERROR;
	}
}

FileReader::~FileReader()
{
	if (myFile.is_open())
	{
		myFile.close();
	}
}

int FileReader::Status()
{
	return myStatus;
}

bool FileReader::ReadLine()
{
	if (myFile.is_open() && !myFile.eof())
	{
		getline(myFile, myLine);

		if (myLine != "")
		{
			int c = myLine.find_first_of('#');
			if (c > 0)
				myLine = myLine.substr(0, c);
			c = myLine.find_first_of(';');
			if (c > 0)
				myLine = myLine.substr(0, c);
		}
		return true;
	}
	else
		return false;
}

string FileReader::ParsePairStr(const string name)
{
	return FindPair(name);
}

long FileReader::ParsePairLong(const string name)
{
	string val = FindPair(name);
	long l = -255;

	if (val != "")
	{
		string::size_type sz;
		l = stol(val, &sz);
	}
	return l;
}

int FileReader::ParsePairInt(const string name)
{
	string val = FindPair(name);
	int i = -255;

	if (val != "")
	{
		string::size_type sz;
		i = stoi(val, &sz);
	}
	return i;
}

bool FileReader::ParsePairBool(const string name)
{
	string val = FindPair(name);

	if (val == "Yes" || val == "True" || val == "Y" || val == "1" || val == "T")
		return true;
	return false;
}

string FileReader::FindPair(const string name)
{
	int p = myLine.find_first_of('=');

	if (p > 0)
	{
		string::size_type n = myLine.find(name);
		if (n != string::npos)
		{
			string retstr = (myLine.substr(p + 1, myLine.length() - (p - 1)));
			return retstr;
		}
	}
	return "";
}