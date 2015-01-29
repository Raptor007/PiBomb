#include "Sound.h"
#include <cmath>

Sound::Sound( void )
{
	PCMHandle = NULL;
	Params = NULL;
	Channels = 2;
	Rate = 22050;
	BufferFrames = 2048;
	BufferSize = 8192;
	Buffer = NULL;
}

Sound::~Sound()
{
}

void Sound::Start( void )
{
	// Initialize audio.
	snd_pcm_open( &PCMHandle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK );
	snd_pcm_hw_params_alloca( &Params );
	snd_pcm_hw_params_any( PCMHandle, Params );
	snd_pcm_hw_params_set_access( PCMHandle, Params, SND_PCM_ACCESS_RW_INTERLEAVED );
	snd_pcm_hw_params_set_format( PCMHandle, Params, SND_PCM_FORMAT_S16_LE );
	snd_pcm_hw_params_set_channels( PCMHandle, Params, Channels );
	snd_pcm_hw_params_set_rate_near( PCMHandle, Params, &Rate, 0 );
	BufferSize = BufferFrames * 2 * Channels;
	snd_pcm_hw_params_set_buffer_size( PCMHandle, Params, BufferSize );
	snd_pcm_hw_params( PCMHandle, Params );
	
	// Create the mixing buffer.
	Buffer = (char*) malloc( BufferSize );
	memset( Buffer, BufferSize, 0 );
}

void Sound::Stop( void )
{
	// Cleanup audio.
	snd_pcm_drain( PCMHandle );
	snd_pcm_close( PCMHandle );
	free( Buffer );
}

void Sound::Load( std::string filename )
{
	Loaded[ filename ] = new SoundSample( filename );
}

void Sound::Play( std::string filename, double volume )
{
	if( Loaded.find(filename) == Loaded.end() )
		Load( filename );
	
	Playing.push_back( PlayingSound( Loaded[ filename ], 0, volume ) );
}

void Sound::PlayLater( std::string filename, double secs, double volume )
{
	if( Loaded.find(filename) == Loaded.end() )
		Load( filename );
	
	Playing.push_back( PlayingSound( Loaded[ filename ], (-1. * secs * Loaded[ filename ]->Rate + 0.5), volume ) );
}

void Sound::Update( void )
{
	// If we have no playing or queued sounds, there's no need to do anything.
	if( Playing.empty() )
		return;
	
	memset( Buffer, 0, BufferSize );
	
	// Fill the output buffer with mixed audio from all playing samples.
	for( std::list<PlayingSound>::iterator playing_iter = Playing.begin(); playing_iter != Playing.end(); playing_iter ++ )
	{
		for( size_t i = 0; i < BufferFrames; i ++ )
		{
			double source_frame = playing_iter->CurrentFrame + (i * playing_iter->Sample->Rate / (double) Rate);
			
			for( size_t j = 0; j < Channels; j ++ )
			{
				double value = (((int16_t*) Buffer)[ i * Channels + j ]) / 32767. + playing_iter->Sample->ValueAt( (long) source_frame, j ) * playing_iter->Volume;
				if( value > 1. )
					((int16_t*) Buffer)[ i * Channels + j ] = 32767;
				else if( value < -1. )
					((int16_t*) Buffer)[ i * Channels + j ] = -32768;
				else
					((int16_t*) Buffer)[ i * Channels + j ] = value * 32767.;
			}
		}
	}
	
	// Send data to the output buffer.
	int wrote_frames = snd_pcm_writei( PCMHandle, Buffer, BufferFrames );
	if( wrote_frames < 0 )
		// Something went wrong, so make sure we can still play audio.
		snd_pcm_prepare( PCMHandle );
	else
	{
		// We successfully added some data to the output buffer.
		for( std::list<PlayingSound>::iterator playing_iter = Playing.begin(); playing_iter != Playing.end(); )
		{
			// Advance the position in each playing sound.
			playing_iter->CurrentFrame += wrote_frames;
			
			// Remove completed sounds from playback list.
			if( playing_iter->CurrentFrame >= (long)( playing_iter->Sample->Frames ) )
			{
				std::list<PlayingSound>::iterator playing_iter_next = playing_iter;
				playing_iter_next ++;
				
				Playing.erase( playing_iter );
				
				playing_iter = playing_iter_next;
			}
			else
				playing_iter ++;
		}
	}
}

SoundSample::SoundSample( std::string filename )
{
	Rate = 22050;
	Channels = 1;
	Frames = 0;
	Data = NULL;
	
	FILE *file = fopen( filename.c_str(), "rb" );
	if( file )
	{
		uint8_t bytes[ 4 ] = {0,0,0,0};
		
		// Read the WAVE headers we care about.
		fseek( file, 22, SEEK_SET );
		fread( bytes, 1, 2, file );
		Channels = bytes[ 0 ] + (bytes[ 1 ] * 256);
		fread( bytes, 1, 4, file );
		Rate = bytes[ 0 ] + (bytes[ 1 ] * 256) + (bytes[ 2 ] * 256*256) + (bytes[ 3 ] * 256*256*256);
		fseek( file, 34, SEEK_SET );
		fread( bytes, 1, 2, file );
		ByteDepth = (bytes[ 0 ] + (bytes[ 1 ] * 256)) / 8;
		fseek( file, 40, SEEK_SET );
		fread( bytes, 1, 4, file );
		size_t data_size = bytes[ 0 ] + (bytes[ 1 ] * 256) + (bytes[ 2 ] * 256*256) + (bytes[ 3 ] * 256*256*256);
		Frames = data_size / (ByteDepth * Channels);
		
		// Read the sound data.
		Data = malloc( data_size );
		memset( Data, 0, data_size );
		fread( Data, 1, data_size, file );
		
		fclose( file );
		file = NULL;
	}
}

SoundSample::~SoundSample()
{
	free( Data );
	Data = NULL;
	Frames = 0;
}

double SoundSample::ValueAt( long frame, size_t channel ) const
{
	if( frame < 0 )
		return 0.;
	if( (size_t) frame >= Frames )
		return 0.;
	
	if( Channels == 1 )
		channel = 0;
	else
		channel %= Channels;
	
	if( ByteDepth == 1 )
	{
		// 8-bit unsigned sample.
		uint8_t *data8 = (uint8_t*) Data;
		return (data8[ frame * Channels + channel ] - 128) / 127.;
	}
	else if( ByteDepth == 2 )
	{
		// 16-bit signed little-endian sample.
		int16_t *data16 = (int16_t*) Data;
		return (data16[ frame * Channels + channel ]) / 32767.;
	}
	
	return 0.;
}

double SoundSample::ValueAt( double frame, size_t channel ) const
{
	double frame_a = 0.;
	double b_part = modf( frame, &frame_a );
	double a = ValueAt( (long)( frame_a + 0.5 ), channel );
	double b = ValueAt( (long)( frame_a + 1.5 ), channel );
	return a * (1 - b_part) + b * b_part;
}

PlayingSound::PlayingSound( const SoundSample *sample, long starting_frame, double volume )
{
	Sample = sample;
	CurrentFrame = starting_frame;
	Volume = volume;
}

PlayingSound::~PlayingSound()
{
}
