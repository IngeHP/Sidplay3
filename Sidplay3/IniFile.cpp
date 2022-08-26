#include "IniFile.h"
#include <stdio.h>
#include <Windows.h>
#include <ShlObj.h>
#include <string>

const char *iniDirName = "Sidplay3";
const char *iniFileName = "Sidplay3.ini";

/********************************************************************
* GetPrivateProfileBool  -  Läser en bool från en INI-fil
*
* In - szSection: 	Sektion
*      szEntry:	Variabelnamn
*      bDefault:	värde att returnera ifall variabeln inte finns i filen
*      szFile:		Vilken INI-fil att leta i
* Ut - Returnerar:	TRUE ifall variabeln har värdet "YES", "Y", "TRUE", "T"
*					eller "1", annars FALSE.
*
* Skapad av Undass Adam Lorentzon 1996-03-13
* AL19980727 - arbetar numera med riktiga booleans
*******************************************************************/

bool
GetMyProfileBool(const char szSection[], const char szEntry[],
bool bDefault, const char szFile[])
{
	char szInfo[4];

	GetPrivateProfileStringA(szSection, szEntry, bDefault ? "1" : "0", szInfo, 4, szFile);
	return(!_stricmp(szInfo, "Yes") || !_stricmp(szInfo, "True") ||
		!_stricmp(szInfo, "Y") || !_stricmp(szInfo, "T") ||
		!_stricmp(szInfo, "1")
		);
}


/********************************************************************
* WritePrivateProfileBool  -  Skriver en bool till en INI-fil
*
* In - szSection: 	Sektion
*      szEntry:	Variabelnamn
*      bValue:		Variabelns värde
*      szFile:		Vilken INI-fil att leta i
*
* Skapad av Undass Adam Lorentzon 1996-03-13
* AL19980727 - arbetar numera med riktiga booleans
*******************************************************************/

void
WriteMyProfileBool(const char szSection[], const char szEntry[], bool bValue, const char szFile[])
{
	char szInfo[4];

	if (bValue)
		sprintf_s(szInfo, "Yes");
	else
		sprintf_s(szInfo, "No");

	WritePrivateProfileStringA(szSection, szEntry, szInfo, szFile);
}


/********************************************************************
* WritePrivateProfileInt  -  Skriver en int till en INI-fil
*
* In - szSection: 	Sektion
*      szEntry:	Variabelnamn
*      nValue:		Variabelns värde
*      szFile:		Vilken INI-fil att skriva i
*
* Skapad av Undass Adam Lorentzon 1996-03-20
*******************************************************************/

void
WriteMyProfileInt(const char szSection[], const char szEntry[], int nValue, const char szFile[])
{
	char szInfo[18];		// _itoa skriver upp till 17 tecken

	_itoa_s(nValue, szInfo, sizeof(szInfo), 10);
	WritePrivateProfileStringA(szSection, szEntry, szInfo, szFile);
}


/********************************************************************
* WritePrivateProfileLong  -  Skriver en long till en INI-fil
*
* In - szSection: 	Sektion
*      szEntry:	Variabelnamn
*      lValue:		Variabelns värde
*      szFile:		Vilken INI-fil att skriva i
*
* Skapad av Undass Adam Lorentzon 1996-03-23
*******************************************************************/

void
WriteMyProfileLong(const char szSection[], const char szEntry[], long lValue, const char szFile[])
{
	char szInfo[34];		// ltoa skriver upp till 33 tecken

	_ltoa_s(lValue, szInfo, sizeof(szInfo), 10);
	WritePrivateProfileStringA(szSection, szEntry, szInfo, szFile);
}


/********************************************************************
* GetPrivateProfileLong  -  Läser en long från en INI-fil
*
* In - szSection: 	Sektion
*      szEntry:	Variabelnamn
*      lDefault:	värde att returnera ifall variabeln inte finns i filen
*					eller variabeln är 0
*      szFile:		Vilken INI-fil att skriva i
*
* Skapad av Undass Adam Lorentzon 1996-03-23
*******************************************************************/

long
GetMyProfileLong(const char szSection[], const char szEntry[], long lDefault, const char szFile[])
{
	char szInfo[80];
	long lReturn;

	GetPrivateProfileStringA(szSection, szEntry, "", szInfo, sizeof(szInfo), szFile);
	if ((lReturn = atol(szInfo)) == 0)	// Fel eller så var värdet verkligen 0
		return lDefault;
	else
		return lReturn;
}

int
GetMyProfileInt(const char szSection[], const char szEntry[], long lDefault, const char szFile[])
{
	char szInfo[80];
	long lReturn;

	GetPrivateProfileStringA(szSection, szEntry, "", szInfo, sizeof(szInfo), szFile);
	if ((lReturn = atoi(szInfo)) == 0)	// Fel eller så var värdet verkligen 0
		return lDefault;
	else
		return lReturn;
}
double GetMyProfileDouble(const char szSection[], const char szEntry[], double dDefault, const char szFile[])
{
	char szInfo[80];
	double dReturn;
	
	GetPrivateProfileStringA(szSection, szEntry, "", szInfo, sizeof(szInfo), szFile);
	if ((dReturn = atof(szInfo)) == 0)
		return dDefault;
	else
		return dReturn;
}

void WriteMyProfileDouble(const char szSection[], const char szEntry[], double dDefault, const char szFile[])
{
	// Using C++11 converting from Doubble to String
	std::string szInfo = std::to_string(dDefault);
	WritePrivateProfileStringA(szSection, szEntry, szInfo.c_str(), szFile);
}


char *GetIniFile()
{
	char *szPath = new char[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szPath) != S_OK)
	{
		char *path;
		size_t len;
		_dupenv_s(&path, &len, "USERPROFILE");
		if (!path)
			return NULL;
		strcpy_s(szPath, MAX_PATH, path);
		strcat_s(szPath, MAX_PATH, "\\Application Data");
	}

	strcat_s(szPath, MAX_PATH, "\\");
	strcat_s(szPath, MAX_PATH, iniDirName);
	
	// Create Appdir if it doesn't exist
	CreateDirectoryA(szPath, NULL);
	strcat_s(szPath, MAX_PATH, "\\");
	strcat_s(szPath, MAX_PATH, iniFileName);
	return szPath;
}

void ReadAudioSettings(audio_section *as, const char szFile[])
{
	char szTemp[16];
	DWORD dRead;

	as->frequency = GetMyProfileInt("Audio", "Frequency", 44100, szFile);
	dRead = GetPrivateProfileStringA("Audio", "Playback", "Mono", szTemp, 16, szFile);

	if (strcmp(CharLowerA(szTemp), "stereo") == 0)
	{
		as->playback = SidConfig::playback_t::STEREO;
	}
	else
	{
		as->playback = SidConfig::playback_t::MONO;
	}
	as->precision = GetMyProfileInt("Audio", "Precision", 16, szFile);
}

void WriteAudioSettings(audio_section *as, const char szFile[])
{
	WriteMyProfileInt("Audio", "Frequency", as->frequency, szFile);
	WriteMyProfileInt("Audio", "Precision", as->precision, szFile);
	if (as->playback == SidConfig::playback_t::MONO)
	{
		WritePrivateProfileStringA("Audio", "Playback", "MONO", szFile);
	}
	else
	{
		WritePrivateProfileStringA("Audio", "Playback", "STEREO", szFile);
	}
}

void ReadEmulationSettings(emulation_section *es, const char szFile[])
{
	char szTemp[32], szModel[10], szSid[10], szSampling[24];
	DWORD dRead;

	// Currently supporting residfp only, so skipping this
	//dRead = GetPrivateProfileStringA("Emulation", "Engine", "sidplayfp", szTemp, 32, szFile);

	// C64 Model
	dRead = GetPrivateProfileStringA("Emulation", "C64Model", "PAL", szModel, 10, szFile);
	if (dRead > 0)
	{
		if (strcmp(szModel, "PAL") == 0)
			es->c64modelDefault = SidConfig::c64_model_t::PAL;
		else if (strcmp(szModel, "NTSC") == 0)
			es->c64modelDefault = SidConfig::c64_model_t::NTSC;
		else if (strcmp(szModel, "OLD_NTSC") == 0)
			es->c64modelDefault = SidConfig::c64_model_t::OLD_NTSC;
		else if (strcmp(szModel, "DREAN") == 0)
			es->c64modelDefault = SidConfig::c64_model_t::DREAN;

	}
	es->c64modelForced = GetMyProfileBool("Emulation", "ForceC64Model", false, szFile);

	// Sid Model
	dRead = GetPrivateProfileStringA("Emulation", "SidModel", "MOS6581", szSid, 10, szFile);
	if (dRead > 0)
	{
		if (strcmp(szSid, "MOS6581") == 0)
			es->sidModel = SidConfig::sid_model_t::MOS6581;
		else if (strcmp(szSid, "MOS8580") == 0)
			es->sidModel = SidConfig::sid_model_t::MOS8580;
	}

	es->sidModelForced = GetMyProfileBool("Emulation", "ForceSidModel", false, szFile);
	es->digiBoost = GetMyProfileBool("Emulation", "DigiBoost", false, szFile);

	// Sampling method
	dRead = GetPrivateProfileStringA("Emulation", "SamplingMethod", "RESAMPLE_INTERPOLATE", szSampling, 24, szFile);
	if (dRead > 0)
	{
		if (strcmp(szSampling, "INTERPOLATE") == 0)
			es->samplingMethod = SidConfig::sampling_method_t::INTERPOLATE;
		else
			es->samplingMethod = SidConfig::sampling_method_t::RESAMPLE_INTERPOLATE;
	}

	// Filter settings
	es->filter = GetMyProfileBool("Emulation", "UseFilter", true, szFile);

	// es->bias = GetMyProfileDouble("Emulation", "FilterBias", 0.0, szFile);
	es->filterCurve6581 = GetMyProfileDouble("Emulation", "FilterCurve6581", 0.50, szFile);
	es->filterCurve8580 = GetMyProfileDouble("Emulation", "FilterCurve8580", 0.50, szFile);
	
	// To make sure the filter curve values are safe
	if ((es->filterCurve6581 < 0.0) || (es->filterCurve6581 > 1.0))
		es->filterCurve6581 = 0.50;
	if ((es->filterCurve8580 < 0) || (es->filterCurve8580 > 1.0))
		es->filterCurve8580 = 0.50;
}

void WriteEmulationSettings(emulation_section *es, const char szFile[])
{
	// C64 model
	if (es->c64modelDefault == SidConfig::c64_model_t::PAL)
		WritePrivateProfileStringA("Emulation", "C64Model", "PAL", szFile);
	if (es->c64modelDefault == SidConfig::c64_model_t::NTSC)
		WritePrivateProfileStringA("Emulation", "C64Model", "NTSC", szFile);
	if (es->c64modelDefault == SidConfig::c64_model_t::OLD_NTSC)
		WritePrivateProfileStringA("Emulation", "C64Model", "OLD_NTSC", szFile);
	if (es->c64modelDefault == SidConfig::c64_model_t::DREAN)
		WritePrivateProfileStringA("Emulation", "C64Model", "DREAN", szFile);

	WriteMyProfileBool("Emulation", "ForceC64Model", es->c64modelForced, szFile);

	// Sid Model
	if (es->sidModel == SidConfig::sid_model_t::MOS6581)
		WritePrivateProfileStringA("Emulation", "SidModel", "MOS6581", szFile);
	if (es->sidModel == SidConfig::sid_model_t::MOS8580)
		WritePrivateProfileStringA("Emulation", "SidModel", "MOS8580", szFile);

	WriteMyProfileBool("Emulation", "ForceSidModel", es->sidModelForced, szFile);
	WriteMyProfileBool("Emulation", "DigiBoost", es->digiBoost, szFile);

	// Sampling method
	if (es->samplingMethod == SidConfig::sampling_method_t::INTERPOLATE)
		WritePrivateProfileStringA("Emulation", "SamplingMethod", "INTERPOLATE", szFile);
	else
		WritePrivateProfileStringA("Emulation", "SamplingMethod", "RESAMPLE_INTERPOLATE", szFile);

	// Filter settings
	WriteMyProfileBool("Emulation", "UseFilter", es->filter, szFile);
	//WriteMyProfileDouble("Emulation", "FilterBias", es->bias, szFile);
	WriteMyProfileDouble("Emulation", "FilterCurve6581", es->filterCurve6581, szFile);
	WriteMyProfileDouble("Emulation", "FilterCurve8580", es->filterCurve8580, szFile);

}