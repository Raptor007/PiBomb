#include "Keyboard.h"
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>

Keyboard::Keyboard( void )
{
	OriginalFlags = 0;
	OriginalMode = 0;
	memset( &OriginalAttr, 0, sizeof(struct termios) );
	RawMode = false;
	
	memset( KeyMap, 0, sizeof(KeyMap) );
	KeyMap[ KEY_KP0 ] = '0';
	KeyMap[ KEY_KP1 ] = '1';
	KeyMap[ KEY_KP2 ] = '2';
	KeyMap[ KEY_KP3 ] = '3';
	KeyMap[ KEY_KP4 ] = '4';
	KeyMap[ KEY_KP5 ] = '5';
	KeyMap[ KEY_KP6 ] = '6';
	KeyMap[ KEY_KP7 ] = '7';
	KeyMap[ KEY_KP8 ] = '8';
	KeyMap[ KEY_KP9 ] = '9';
	KeyMap[ KEY_KP0 ] = '0';
	KeyMap[ KEY_1 ] = '1';
	KeyMap[ KEY_2 ] = '2';
	KeyMap[ KEY_3 ] = '3';
	KeyMap[ KEY_4 ] = '4';
	KeyMap[ KEY_5 ] = '5';
	KeyMap[ KEY_6 ] = '6';
	KeyMap[ KEY_7 ] = '7';
	KeyMap[ KEY_8 ] = '8';
	KeyMap[ KEY_9 ] = '9';
	KeyMap[ KEY_ENTER ] = 13;
	KeyMap[ KEY_KPENTER ] = 13;
	KeyMap[ KEY_KPMINUS ] = '-';
	KeyMap[ KEY_KPPLUS ] = '+';
	KeyMap[ KEY_Q ] = 'q';
	KeyMap[ KEY_GRAVE ] = '`';
}

Keyboard::~Keyboard()
{
	// Make sure we restore terminal settings!
	Stop();
}

void Keyboard::Start( void )
{
	// Get original values.
	OriginalRepeat.delay = -1;
	OriginalRepeat.period = -1;
	ioctl( 0, KDKBDREP, &OriginalRepeat );
	OriginalFlags = fcntl( 0, F_GETFL );
	ioctl( 0, KDGKBMODE, &OriginalMode );
	tcgetattr( 0, &OriginalAttr );
	
	// Set non-blocking stdin.
	int flags = OriginalFlags;
	flags |= O_NONBLOCK;
	fcntl( 0, F_SETFL, flags );
	
	// Disable keyboard repeat.
	struct kbd_repeat repeat;
	repeat.delay = 0;
	repeat.period = 0;
	ioctl( 0, KDKBDREP, &repeat );
	
	// Disable buffering and echo.
	struct termios attr;
	memcpy( &attr, &OriginalAttr, sizeof(struct termios) );
	attr.c_lflag &= ~(ICANON | ECHO | ISIG);
	attr.c_iflag &= ~(ISTRIP | INLCR | ICRNL | IGNCR | IXON | IXOFF);
	tcsetattr( 0, TCSANOW, &attr );
	
	// Set keyboard to raw mode.
	if( ioctl( 0, KDSKBMODE, K_MEDIUMRAW ) >= 0 )
		RawMode = true;
}

void Keyboard::Stop( void )
{
	// Restore original terminal settings.
	fcntl( 0, F_SETFL, OriginalFlags );
	ioctl( 0, KDSKBMODE, OriginalMode );
	tcsetattr( 0, TCSAFLUSH, &OriginalAttr );
	ioctl( 0, KDKBDREP, &OriginalRepeat );
	
	// Assume whatever we knew about keys being held down is no longer valid.
	KeysDown.clear();
}

std::deque<char> Keyboard::GetEvents( void )
{
	// If we couldn't enable raw mode (because we're using SSH), assume keys are down for one iteration.
	if( ! RawMode )
		KeysDown.clear();
	
	std::deque<char> events;
	
	char event = '\0';
	while( read( 0, &event, 1 ) >= 0 )
	{
		// In raw mode, translate key codes into ASCII values.
		if( RawMode )
			event = (event & 0x80) | KeyMap[ event & 0x7F ];
		
		// We'll return a list of all the raw up/down events.
		events.push_back( event );
		
		// The highest bit set means the key was released.
		if( event & 0x80 )
			KeysDown.erase( event & 0x7F );
		else
			KeysDown.insert( event & 0x7F );
	}
	
	return events;
}

bool Keyboard::KeyDown( char key )
{
	return( KeysDown.find(key) != KeysDown.end() );
}
