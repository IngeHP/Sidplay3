#pragma once

#include <Windows.h>
#include <mmsystem.h>

// Do we need these?
constexpr auto MIN_FREQUENCY = 4000;
constexpr auto MAX_FREQUENCY = 48000;
constexpr auto DEF_FREQUENCY = 44100;
constexpr auto DEF_PRECISION = 16;
constexpr auto MIN_NUM_BUFFERS = 2;
constexpr auto MAX_NUM_BUFFERS = 16;
constexpr auto DEF_NUM_BUFFERS = 4;
constexpr auto MIN_BUFSIZE_BASE = 8;
constexpr auto MAX_BUFSIZE_BASE = 16;
constexpr auto DEF_BUFSIZE_BASE = 13;
constexpr auto MIN_BUFSIZE = 1L << MIN_BUFSIZE_BASE;
constexpr auto MAX_BUFSIZE = 1L << MAX_BUFSIZE_BASE;
constexpr auto DEF_BUFSIZE = 1L << DEF_BUFSIZE_BASE;


typedef struct
{
	char			main_chunk[4];
	UINT32			length;
	char			chunk_type[4];
	char			sub_chunk[4];
	UINT32			clength;
	PCMWAVEFORMAT	fmt;
	char			data_chunk[4];
	UINT32			data_length;

} WAVFILEHEADER;


class AudioSettings
{
public:
	AudioSettings();

	UINT32 setFrequency(UINT32 f);
	UINT8 setChannels(UINT8 ch);
	UINT8 setBitsPerSample(UINT8 bps);
	UINT8 setNumBuffers(UINT8 nb);
	UINT32 setBufSize(UINT32 bs);
	UINT32 setBufSizeBase(UINT8 bsb);
	UINT32 setTimeToPlay(UINT32 ttp);

	inline UINT32 frequency() { return _frequency; }
	inline UINT8 channels() { return _channels; }
	inline UINT8 bitspersample() { return _bitspersample; }
	inline UINT8 numbuffers() { return _numbuffers; }
	inline UINT32 bufsize() { return _bufsize; }
	inline UINT32 timetoplay() { return _timetoplay; }

	UINT32 avgbytespersec();
	INT16 blockalign();
	UINT32 calcwavfilesize();
	bool isallset();
	bool isequal(AudioSettings& as);
	void correctsettings();

	WAVEFORMATEX MakeWaveFormatEx();
	PCMWAVEFORMAT MakePcmWaveFormat();

private:
	UINT32 _frequency;
	UINT8 _channels;
	UINT8 _bitspersample;
	UINT8 _numbuffers;
	UINT32 _bufsize;
	UINT32 _timetoplay;
	DWORD volume;
};

