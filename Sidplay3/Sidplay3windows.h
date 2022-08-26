#ifndef SP3_H
#define SP3_H

#include "resource.h"

// sidplayfp-specific
#include <sidplayfp/sidplayfp.h>
#include "sidplayfp/SidTune.h"

#include "config.h"

// Audio-specific
#include "Audio/IAudio.h"
#include "Audio/AudioConfig.h"
#include "Audio/null/null.h"


enum player_state_t : int
{
	playerError = 0, playerRunning, playerPaused, playerStopped,
	playerRestart, playerExit, playerFast = 128,
	playerFastRestart = playerRestart | playerFast,
	playerFastExit = playerExit | playerFast
} ;


struct thread_state_t
{
	bool threadPause = false;
	bool threadPlaying = false;

} thread_state;

typedef enum
{
	/* Same as EMU_DEFAULT except no soundcard.
	Still allows wav generation */
	EMU_NONE = 0,
	/* The following require a soundcard */
	EMU_DEFAULT, EMU_RESIDFP, EMU_RESID,
	/* The following should disable the soundcard */
	EMU_HARDSID, EMU_SIDSTATION, EMU_COMMODORE,
	EMU_SIDSYN, EMU_END
} SIDEMUS;

typedef enum
{
	/* Define possible output sources */
	OUT_NULL = 0,
	/* Hardware */
	OUT_SOUNDCARD,
	/* File creation support */
	OUT_WAV, OUT_AU, OUT_END
} OUTPUTS;

int     m_precision;

struct m_filter_t
{
	// Filter parameter for reSID
	double         bias;
	// Filter parameters for reSIDfp
	double         filterCurve6581;
	double         filterCurve8580;

	bool           enabled;
} m_filter;

struct m_driver_t
{
	OUTPUTS        output;   // Selected output type
	SIDEMUS        sid;      // Sid emulation
	bool           file;     // File based driver
	AudioConfig    cfg;
	IAudio*        selected; // Selected Output Driver
	IAudio*        device;   // HW/File Driver
	Audio_Null     null;     // Used for everything
} m_driver;

struct m_timer_t
{   // secs
	uint_least32_t start;
	uint_least32_t current;
	uint_least32_t stop;
	uint_least32_t length;
	bool           valid;
} m_timer;

struct m_track_t
{
	uint_least16_t first;
	uint_least16_t selected;
	uint_least16_t songs;
	bool           loop;
	bool           single;
} m_track;

struct m_speed_t
{
	uint_least8_t current;
	uint_least8_t max;
} m_speed;

bool	v1mute, v2mute, v3mute, v4mute, v5mute, v6mute, v7mute, v8mute, v9mute;
bool	showMillis = false;

// Own Windows messages
#define OWM_PLAYFILE 			(WM_USER+0)
#define OWM_CHANNELSCHANGED 	(WM_USER+1)
#define OWM_PLAYATOMFILE 		(WM_USER+3)
#define OWM_SETTIMEPLAYED 		(WM_USER+4)
#define OWM_FULLSTOP			(WM_USER+5)
#define OWM_NOBUFS		 		(WM_USER+6)
#define OWM_PLAYENTRY			(WM_USER+7)
#define OWM_LISTCHANGED			(WM_USER+8)
#define OWM_SETFILEEXTENSIONS	(WM_USER+9)
#define OWM_REFRESH				(WM_USER+10)
// A new sid file has been loaded. (Update properties window!)
#define OWM_NEWSID				(WM_USER+11)

// For Audio output
#define WM_AUDIO_MSG			(WM_USER+12)
#define WM_AUDIO_STOP			(WM_USER+13)

// Update Mixer
#define OWM_MIXERUPDATE			(WM_USER+14)

// Update Songlengths
#define OWM_UPDATESL			(WM_USER+15)

// Update emulation settings
#define OWM_UPDATE_ES			(WM_USER+16)

#endif SP3_H