extern "C"
{
#include "picolcd.h"
}

class LCD
{
public:
	usblcd_operations *RawLCD;
	char Line[ 2 ][ 21 ];
	
	LCD( void );
	virtual ~LCD();
	
	void Start( void );
	void Stop( void );
	
	void SetBacklight( bool on );
	void SetText( int line, const char *text );
	void SetLED( int led, bool on );
};
