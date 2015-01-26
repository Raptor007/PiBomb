#include "LCD.h"
#include <cstdio>
#include <cstring>

LCD::LCD( void )
{
	RawLCD = NULL;
	memset( Line[ 0 ], ' ', 20 );
	memset( Line[ 1 ], ' ', 20 );
	Line[ 0 ][ 20 ] = '\0';
	Line[ 1 ][ 20 ] = '\0';
}

LCD::~LCD()
{
}

void LCD::Start( void )
{	
	// Initialize LCD.
	RawLCD = new_driver( PICOLCD20x2 );
	RawLCD->init( RawLCD );
	SetBacklight( true );
	SetText( 0, "" );
	SetText( 1, "" );
}

void LCD::Stop( void )	
{
	if( RawLCD )
	{
		// Cleanup LCD.
		SetText( 0, "" );
		SetText( 1, "" );
		SetBacklight( false );
		RawLCD->close( RawLCD );
		RawLCD = NULL;
	}
}

void LCD::SetBacklight( bool on )
{
	if( RawLCD )
		RawLCD->backlight( RawLCD, on ? 1 : 0 );
}

void LCD::SetText( int line, const char *text )
{
	snprintf( Line[ line ], 21, "%-20.20s", text );
	if( RawLCD )
		RawLCD->settext( RawLCD, line, 0, Line[ line ] );
}

void LCD::SetLED( int led, bool on )
{
	if( RawLCD )
		RawLCD->setled( RawLCD, led, on ? 1 : 0 );
}
