#include <cmath>
#include <unistd.h>
#include <signal.h>
#include "LCD.h"
#include "Sound.h"
#include "Keyboard.h"

#define PLANT_DIGITS 7
#define BOMB_TIME   45
#define DEFUSE_TIME 10

volatile bool running;
void sigterm( int arg );

int main( int argc, char **argv )
{
	running = true;
	signal( SIGTERM, &sigterm );
	
	LCD lcd;
	Sound sound;
	Keyboard keyboard;
	
	// Initialize LCD if root.
	if( getuid() == 0 )
		lcd.Start();
	
	// Show some intro text.
	lcd.SetText( 0, "PiBomb by Raptor007" );
	lcd.SetText( 1, "" );
	lcd.SetLED( 0, false );
	
	// Initialize audio.
	sound.Start();
	
	// Load samples.
	sound.Load( "c4_beep1.wav" );
	sound.Load( "c4_explode1.wav" );
	sound.Load( "c4_disarm.wav" );
	sound.Load( "bombpl.wav" );
	sound.Load( "bombdef.wav" );
	sound.Load( "terwin.wav" );
	sound.Load( "ctwin.wav" );
	sound.Load( "letsgo.wav" );
	
	// Gameplay variables.
	bool planted = false;
	bool need_planted_message = false;
	bool defusing = false;
	bool defusing_test = false;
	char digits[ PLANT_DIGITS + 1 ] = "";
	double planted_for = 0.;
	double defusing_for = 0.;
	double next_beep_at = 0.;
	char str[ 21 ] = "";
	
	// Initialize keyboard.
	keyboard.Start();
	
	// Ready to start.
	lcd.SetText( 1, "Type digits to plant" );
	sound.Play( "letsgo.wav" );
	
	// Main loop.
	while( running )
	{
		// Update audio playback.
		sound.Update();
		
		// Get keyboard input.
		std::deque<char> key_events = keyboard.GetEvents();
		
		if( ! planted )
		{
			// Bomb is not planted yet.
			
			double beep_delay = 0.;
			
			while( key_events.size() )
			{
				char key = key_events.front();
				key_events.pop_front();
				
				if( ! (key & 0x80) )
				{
					// Key down event.
					if( (key >= '0') && (key <= '9') )
					{
						sound.PlayLater( "c4_beep1.wav", beep_delay );
						beep_delay += 0.1;
						lcd.SetLED( 0, false );
						size_t len = strlen(digits);
						snprintf( digits + len, PLANT_DIGITS + 1 - len, "%c", key );
						len ++;
						
						// Show digits pressed so far.
						snprintf( str, 21, "%-20.20s", "" );
						size_t advance = (PLANT_DIGITS > 10) ? 1 : 2;
						size_t start = (20 - (PLANT_DIGITS * advance - 1)) / 2;
						for( size_t i = 0; i < len; i ++ )
							str[ start + i*advance ] = digits[ i ];
						lcd.SetText( 0, str );
						
						if( len < PLANT_DIGITS )
						{
							// Prompt for more digits.
							snprintf( str, 21, " Type %i more digit%c", PLANT_DIGITS - len, (PLANT_DIGITS - len == 1) ? ' ' : 's' );
							lcd.SetText( 1, str );
						}
						else
						{
							planted = true;
							planted_for = 0.;
							defusing_for = 0.;
							sound.PlayLater( "bombpl.wav", 1. );
							next_beep_at = 3.;
							lcd.SetText( 1, "" );
							need_planted_message = true;
							break;
						}
					}
					else if( key == '-' )
					{
						system( "/usr/bin/amixer set PCM 6dB-" );
						sound.Play( "c4_explode1.wav" );
					}
					else if( key == '+' )
					{
						system( "/usr/bin/amixer set PCM 6dB+" );
						sound.Play( "c4_explode1.wav" );
					}
				}
			}
		}
		else
		{
			// Bomb is already planted.
			
			if( need_planted_message && (planted_for >= 1.) )
			{
				lcd.SetText( 0, "" );
				lcd.SetText( 0, "  Bomb is planted!" );
				lcd.SetText( 1, "Hold ENTER to defuse" );
				need_planted_message = false;
			}
			
			if( planted_for >= BOMB_TIME )
			{
				sound.Play( "c4_explode1.wav" );
				sound.PlayLater( "terwin.wav", 3. );
				lcd.SetLED( 0, true );
				
				planted = false;
				planted_for = 0.;
				memset( digits, 0, 8 );
				lcd.SetText( 0, "   Bomb detonated!" );
				lcd.SetText( 1, "Type digits to plant" );
			}
			else if( planted_for >= next_beep_at )
			{
				sound.Play( "c4_beep1.wav" );
				lcd.SetLED( 0, true );
				
				if( planted_for < (BOMB_TIME - 26) )
					next_beep_at += 2.;
				else if( planted_for < (BOMB_TIME - 18) )
					next_beep_at += 1.;
				else if( planted_for < (BOMB_TIME - 14) )
					next_beep_at += 0.5;
				else if( planted_for < (BOMB_TIME - 6) )
					next_beep_at += 0.25;
				else
					next_beep_at += 0.2;
			}
			
			bool was_defusing = defusing;
			if( key_events.size() && (key_events.front() == '`') )
				defusing_test = ! defusing_test;
			defusing = (defusing_test || keyboard.KeyDown(13));
			
			if( defusing )
			{
				if( ! was_defusing )
				{
					lcd.SetText( 1, "     Defusing..." );
					sound.Play( "c4_disarm.wav" );
				}
				else if( defusing_for > DEFUSE_TIME )
				{
					sound.Play( "bombdef.wav" );
					sound.PlayLater( "ctwin.wav", 3. );
					planted = false;
					planted_for = 0.;
					memset( digits, 0, 8 );
					lcd.SetText( 0, "    Bomb defused!" );
					lcd.SetText( 1, "Type digits to plant" );
				}
				else
				{
					// Show digits defused so far.
					size_t defused_digits = PLANT_DIGITS * defusing_for / DEFUSE_TIME;
					snprintf( str, 21, "%-20.20s", "" );
					size_t advance = (PLANT_DIGITS > 10) ? 1 : 2;
					size_t start = (20 - (PLANT_DIGITS * advance - 1)) / 2;
					for( size_t i = 0; i < defused_digits; i ++ )
						str[ start + i*advance ] = digits[ i ];
					if( defused_digits < strlen(digits) )
						str[ start + defused_digits*advance ] = '0' + (rand() % 10);
					lcd.SetText( 0, str );
				}
			}
			else if( was_defusing )
			{
				defusing_for = 0.;
				lcd.SetText( 0, "  Bomb is planted!" );
				lcd.SetText( 1, "Hold ENTER to defuse" );
			}
		}
		
		if( keyboard.KeyDown('q') || keyboard.KeyDown('\t') )
			running = false;
		
		usleep( 50 * 1000 );
		if( planted )
			planted_for += 0.05;
		if( defusing )
			defusing_for += 0.05;
	}
	
	// Reset keyboard.
	keyboard.Stop();
	
	// Cleanup audio.
	sound.Stop();
	
	// Cleanup LCD.
	lcd.Stop();
	
	return 0;
}

void sigterm( int arg )
{
	running = false;
}
