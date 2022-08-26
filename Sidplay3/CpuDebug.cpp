#include "CpuDebug.h"
#include "Progress.h"
#include "Ofile.h"
#include "Misc.h"

#include <stdio.h>

#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidTuneInfo.h>


extern HWND hWnd;
extern SidConfig m_engine;
extern char* szIni;

CpuDebug::CpuDebug()
{
	m_sidFileName = "";
	m_outFileName = "";
	m_subtune = 1;
	m_startMillis = 200;
	m_durMillis = 1000;
	m_powerOnDelay = 200;
}

CpuDebug::CpuDebug(string sidFile, string outFile, int sub, long startMillis, long durMillis, unsigned int powerOnDelay)
{
	Set(sidFile, outFile, sub, startMillis, durMillis, powerOnDelay);
}

void CpuDebug::Set(string sidFile, string outFile, int sub, long startMillis, long durMillis, unsigned int powerOnDelay)
{
	m_sidFileName = sidFile;
	m_outFileName = outFile;
	m_subtune = sub;
	m_startMillis = startMillis;
	m_durMillis = durMillis;
	m_powerOnDelay = powerOnDelay;
}


void CpuDebug::Run()
{
	short buf[40];

	Progress* progress = new Progress();
	int percentdone = 0;

	progress->SetPercentage(percentdone);

	sidplayfp emu;
	SidConfig m_emuCfg;

	m_emuCfg.playback = SidConfig::MONO;
	m_emuCfg.sidEmulation = NULL;
	m_emuCfg.powerOnDelay = m_powerOnDelay;

	emu.config(m_emuCfg);

	SidTune sid(m_sidFileName.c_str());
	sid.selectSong(m_subtune);
	emu.load(&sid);

	unsigned long startTick = m_startMillis;
	unsigned long stopTick = m_startMillis + m_durMillis;

	if (m_startMillis > 0)
	{
		while (emu.timeMs() < startTick)
		{
			emu.play(buf, sizeof(buf));
			percentdone = emu.timeMs() / stopTick;
			progress->SetPercentage(percentdone);
			progress->PumpMessages();
			
			if (progress->GetStatus() == Progress::PROGRESS_DELETE)
				break;
		}
	}

	FILE* fp;
	errno_t err;
	err = fopen_s(&fp, m_outFileName.c_str(), "w");
	if (err == 0)
	{
		fprintf_s(fp, "%s, subtune %d", m_sidFileName.c_str(), m_subtune);
		fprintf_s(fp, " (%d:%0.2d.%0.3d - %d:%.02d.%.03d)\n",
			startTick / 60000, startTick / 1000, startTick % 1000 ,
			stopTick / 60000, stopTick / 1000, stopTick % 1000);

		fprintf(fp, "\n\n");

		emu.debug(true, fp);

		while (emu.timeMs() < stopTick)
		{
			emu.play(buf, sizeof(buf));
			percentdone = 100 * emu.timeMs() / stopTick;
			progress->SetPercentage(percentdone);
			progress->PumpMessages();

			if (progress->GetStatus() == Progress::PROGRESS_DELETE)
				break;
		}
		progress->SetPercentage(100);
		emu.debug(false, NULL);
		emu.stop();
		delete progress;
		fclose(fp);
	}
}

