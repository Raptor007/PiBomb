class Sound;
class SoundSample;
class PlayingSound;

#include <stdint.h>
#include <string>
#include <map>
#include <list>

extern "C"
{
#include <alsa/asoundlib.h>
}

class Sound
{
public:
	snd_pcm_t *PCMHandle;
	snd_pcm_hw_params_t *Params;
	unsigned int Rate, Channels;
	snd_pcm_uframes_t BufferFrames, BufferSize;
	void *Buffer;
	
	std::map<std::string,SoundSample*> Loaded;
	std::list<PlayingSound> Playing;
	
	Sound( void );
	virtual ~Sound();
	
	void Start( void );
	void Stop( void );
	
	void Load( std::string filename );
	void Play( std::string filename, double volume = 1. );
	void PlayLater( std::string filename, double secs, double volume = 1. );
	void Update( void );
};

class SoundSample
{
public:
	unsigned int ByteDepth, Rate, Channels;
	size_t Frames;
	void *Data;
	
	SoundSample( std::string filename );
	virtual ~SoundSample();
	
	double ValueAt( long frame, size_t channel ) const;
	double ValueAt( double frame, size_t channel ) const;
};

class PlayingSound
{
public:
	const SoundSample *Sample;
	double Volume;
	long CurrentFrame;
	
	PlayingSound( const SoundSample *sample, long starting_frame = 0L, double volume = 1. );
	virtual ~PlayingSound();
};
