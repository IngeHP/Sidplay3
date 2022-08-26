#pragma once

#include <Windows.h>
#include <string>
#include <sidplayfp/SidTune.h>

using namespace std;

struct cpudebug_param
{
	HFONT font;
	SidTune* tune;
};

const string defaultStartTimeStrings[5] = {
"0.2 seconds",
"0.5 seconds",
"1 second",
"2 seconds",
"3 seconds"
};

const int defaultStartTimesMillis[5] = { 200, 500, 1000, 2000, 3000 };

class CpuDebug
{
public:
	CpuDebug();
	CpuDebug(string sidFile, string outFile, int sub, long startMillis, long durMilis, unsigned int powerOnDelay);
	void Set(string sidFile, string outFile, int sub, long startMillis, long durMilis, unsigned int powerOnDelay);
	void Run();

private:
	string m_sidFileName;
	string m_outFileName;
	int m_subtune;
	long m_startMillis;
	long m_durMillis;
	unsigned int m_powerOnDelay;


};
