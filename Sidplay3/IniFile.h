#ifndef INIFILE_H
#define INIFILE_H

#include <Windows.h>
#include <sidplayfp\SidConfig.h>

struct sidplay2_section
{
	int            version;
	char          *database;
	uint_least32_t playLength;
	uint_least32_t recordLength;
	//char          *kernalRom;
	//char          *basicRom;
	//char          *chargenRom;
};

struct audio_section
{   // INI Section - [Audio]
	int		frequency;
	SidConfig::playback_t playback;
	int		precision;
};

struct emulation_section
{   // INI Section - [Emulation]
	char         *engine;
	SidConfig::c64_model_t  c64modelDefault;
	bool          c64modelForced;
	SidConfig::sid_model_t  sidModel;
	bool          sidModelForced;
	SidConfig::sampling_method_t samplingMethod;
	bool          filter;
	bool          digiBoost;
	double        bias;
	double        filterCurve6581;
	double        filterCurve8580;
};

bool GetMyProfileBool(const char szSection[], const char szEntry[], bool bDefault, const char szFile[]);
void WriteMyProfileBool(const char szSection[], const char szEntry[], bool bValue, const char szFile[]);
void WriteMyProfileInt(const char szSection[], const char szEntry[], int nValue, const char szFile[]);
void WriteMyProfileLong(const char szSection[], const char szEntry[], long lValue, const char szFile[]);
long GetMyProfileLong(const char szSection[], const char szEntry[], long lDefault, const char szFile[]);
int GetMyProfileInt(const char szSection[], const char szEntry[], long lDefault, const char szFile[]);
double GetMyProfileDouble(const char szSection[], const char szEntry[], double dDefault, const char szFile[]);
void WriteMyProfileDouble(const char szSection[], const char szEntry[], double dDefault, const char szFile[]);

char *GetIniFile();
void ReadAudioSettings(audio_section *as, const char szFile[]);
void ReadEmulationSettings(emulation_section *es, const char szFile[]);
void ReadSidSettings(sidplay2_section *ss, const char szFile[]);
void WriteAudioSettings(audio_section *as, const char szFile[]);
void WriteEmulationSettings(emulation_section *es, const char szFile[]);
void WriteSidSettings(sidplay2_section *ss, const char szFile[]);

#endif