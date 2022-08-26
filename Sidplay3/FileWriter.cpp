#include "FileWriter.h"
#include <string>

FileWriter::FileWriter(const string filename) : myStatus(OK)
{
	myFile = ofstream(filename);
	if (!myFile.is_open())
		myStatus = -1;
}

FileWriter::~FileWriter()
{
	if (myFile.is_open())
	{
		myFile.close();
	}
}

int FileWriter::Status()
{
	return myStatus;
}

void FileWriter::Write(const string name)
{
	myFile << name << endl;
}

void FileWriter::WritePairStr(const string name, const string value)
{
	string tmp = name + "=" + value;
	myFile << tmp << endl;
}

void FileWriter::WritePairInt(const string name, const int value)
{
	string tmp = name + "=" + std::to_string(value);
	myFile << tmp << endl;
}

void FileWriter::WritePairLong(const string name, const long value)
{
	string tmp = name + "=" + std::to_string(value);
	myFile << tmp << endl;
}

void FileWriter::WritePairBool(const string name, bool value)
{
	string tmp = name + "=" + ((value == true) ? "Yes" : "No");
	myFile << tmp << endl;
}

void FileWriter::NewLine()
{
	myFile << endl;
}