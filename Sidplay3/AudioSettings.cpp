#include "AudioSettings.h"

AudioSettings::AudioSettings()
{
	_frequency = DEF_FREQUENCY;
	_channels = 2;
	_bitspersample = 16;
	_numbuffers = 4;
	_bufsize = 8192;
	_timetoplay = 180;
}

UINT32 AudioSettings::setFrequency(UINT32 f)
{
	if ((f > MIN_FREQUENCY) && (f < MAX_FREQUENCY))
		_frequency = f;
	return _frequency;
}

UINT8 AudioSettings::setChannels(UINT8 ch)
{
	if ((ch == 1) || (ch == 2))
		_channels = ch;
	return _channels;
}

UINT8 AudioSettings::setBitsPerSample(UINT8 bps)
{
	if ((bps == 8) || (bps == 16))
		_bitspersample = bps;
	return _bitspersample;
}

UINT8 AudioSettings::setNumBuffers(UINT8 nb)
{
	if ((nb >= MIN_NUM_BUFFERS) && (nb <= MAX_NUM_BUFFERS))
		_numbuffers = nb;
	return _numbuffers;
}

UINT32 AudioSettings::setBufSize(UINT32 bs)
{
	if ((bs >= MIN_BUFSIZE) && (bs <= MAX_BUFSIZE))
		_bufsize = bs;
	return _bufsize;
}

UINT32 AudioSettings::setBufSizeBase(UINT8 bsb)
{
	if ((bsb >= MIN_BUFSIZE_BASE) && (bsb <= MAX_BUFSIZE_BASE))
		_bufsize = 1UL << bsb;
	return _bufsize;
}

UINT32 AudioSettings::setTimeToPlay(UINT32 ttp)
{
	if (ttp > 0)
		_timetoplay = ttp;
	return _timetoplay;
}

bool AudioSettings::isallset()
{
	return _frequency && _channels && _bitspersample && _numbuffers && _bufsize;
}

bool AudioSettings::isequal(AudioSettings& as)
{
	return	_frequency == as._frequency &&
		_channels == as._channels &&
		_bitspersample == as._bitspersample &&
		_numbuffers == as._numbuffers &&
		_bufsize == as._bufsize;
}

UINT32 AudioSettings::avgbytespersec()
{
	return (_frequency * _channels * _bitspersample) / 8;
}

INT16 AudioSettings::blockalign()
{
	return (_channels * _bitspersample) / 8;
}

UINT32 AudioSettings::calcwavfilesize()
{
	return avgbytespersec() * _timetoplay + sizeof(WAVFILEHEADER);
}

void AudioSettings::correctsettings()
{
	if (_frequency < MIN_FREQUENCY || _frequency > MAX_FREQUENCY)
		_frequency = DEF_FREQUENCY;
	if (_channels < 1 || _channels > 2)
		_channels = 2;
	if (_bitspersample != 8 && _bitspersample != 16)
		_bitspersample = 8;
}

WAVEFORMATEX AudioSettings::MakeWaveFormatEx()
{
	WAVEFORMATEX wavefmt;

	correctsettings();

	wavefmt.wFormatTag = WAVE_FORMAT_PCM;
	wavefmt.nChannels = _channels;
	wavefmt.nSamplesPerSec = _frequency;
	wavefmt.nAvgBytesPerSec = avgbytespersec();
	wavefmt.nBlockAlign = blockalign();
	wavefmt.wBitsPerSample = _bitspersample;
	wavefmt.cbSize = 0;

	return wavefmt;
}

PCMWAVEFORMAT AudioSettings::MakePcmWaveFormat()
{
	PCMWAVEFORMAT wavefmt;

	correctsettings();

	wavefmt.wf.wFormatTag = WAVE_FORMAT_PCM;
	wavefmt.wf.nChannels = _channels;
	wavefmt.wf.nSamplesPerSec = _frequency;
	wavefmt.wf.nAvgBytesPerSec = avgbytespersec();
	wavefmt.wf.nBlockAlign = blockalign();
	wavefmt.wBitsPerSample = _bitspersample;

	return wavefmt;
}