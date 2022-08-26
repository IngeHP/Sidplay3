#include "Songlength.h"
#include <string>
#include <Windows.h>

#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"

Songlength::Songlength()
{
	Init();
};

Songlength::~Songlength()
{};

Songlength::Songlength(const char *filename)
{
	Init();
	if (SetPath(filename) != 0)
	{
		sldbpath.clear();
	}
}

errno_t Songlength::SetPath(const char *filename)
{
	sldbpath = std::string(filename);

	if (sldbpath.length() < 1)
		return -1;
	else
		sldbpath.append("\\DOCUMENTS\\Songlengths.md5");
	return 0;
}

bool Songlength::parse(char *md5)
{

	if (sldbpath.length() > 0)
	{
		GetPrivateProfileStringA("Database", md5, "", buffer, 1024, sldbpath.c_str());
		if (strnlen_s(buffer, 1024) > 0)
		{
			int pos = 0;
			int t = 0;

			char buf[4];
			bool reader = true;
			while (reader)
			{
				int b = 0;
				int time = 0;
				int mill = 0;

				while ((buffer[pos] != ':') && (buffer[pos] != '\0'))
					buf[b++] = buffer[pos++];
				if (buffer[pos] == '\0')
				{
					reader = false;
					break;
				}

				buf[b] = '\0';
				pos++;

				time = atol(buf) * 60;
				buf[0] = buffer[pos++];
				buf[1] = buffer[pos++];
				buf[2] = '\0';

				time += atol(buf);
				if (buffer[pos] == '.')
				{
					pos++;
					buf[0] = buf[1] = buf[2] = '0';
					buf[3] = '\0';
					b = 0;
					while ((buffer[pos] >= '0') && (buffer[pos] <= '9'))
						buf[b++] = buffer[pos++];

					mill = atol(buf);
					mill = mill << 16;
				}

				lengths[t++] = time | mill;
				curTunes++;

				if ((buffer[pos] != '(') && (buffer[pos] != ' ') && (buffer[pos] != '\0'))
				{
					SendMessage(hPlayer, WM_COMMAND, IDB_PLAYPAUSE, true);
					int answer = MessageBox(NULL, L"Syntax error in Songlengths.txt.\r\nCannot read correct length of current tune.", L"SLDB ERROR", MB_OK);
					SendMessage(hPlayer, WM_COMMAND, IDB_PLAYPAUSE, true);
					return false;
				}

				if (buffer[pos] == '(')
					pos += 3;
				if (buffer[pos] == ' ')
					pos++;
				if (buffer[pos] == '\0')
					reader = false;
			}

			strcpy_s(curMd5, SidTune::MD5_LENGTH + 1, md5);
			return true;

		}
		else
			return false;
	}
	else
		return false;
}
uint_least32_t Songlength::Length(SidTune &tune)
{
	const unsigned int song = tune.getInfo()->currentSong();

	if (!song)
	{
		return -1;
	}

	char md5[SidTune::MD5_LENGTH + 1];
	tune.createMD5New(md5);
	return Length(md5, song);

}

uint_least32_t Songlength::Length(char *md5, unsigned int tune)
{
	if (tune > 0)
	{
		if (strncmp(curMd5, md5, SidTune::MD5_LENGTH) == 0)
			return lengths[tune - 1];

		if (parse(md5))
			return lengths[tune - 1];
		else
			return 0;
	}
	else
		return 0;
}

void Songlength::Init()
{
	curTunes = 0;
	sldbpath.clear();
	for (int i = 0; i < 256; i++)
		lengths[i] = 0;
}