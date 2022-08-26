#pragma once
#include <cstdint>
#include <cstdlib>
#include <string>
#include <Windows.h>

#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"
#include "resource.h"

extern HWND hPlayer;


class Songlength
{
private:
	std::string sldbpath;
	//std::string buffer;
	char buffer[1024];
	unsigned int lengths[256];
	int curTunes;
	char curMd5[SidTune::MD5_LENGTH + 1];

public:
	Songlength();
	~Songlength();

	Songlength(const char *);

	errno_t SetPath(const char *);

	uint_least32_t Length(SidTune &tune);

	uint_least32_t Length(char *md5, unsigned int tune);


private:
	void Init(void);
	bool parse(char *md5);
};