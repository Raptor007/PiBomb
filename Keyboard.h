#include <termios.h>
#include <linux/kd.h>
#include <deque>
#include <set>
#include <map>

class Keyboard
{
public:
	int OriginalFlags;
	int OriginalMode;
	struct kbd_repeat OriginalRepeat;
	struct termios OriginalAttr;
	char KeyMap[ 128 ];
	bool RawMode;
	std::set<char> KeysDown;
	
	Keyboard( void );
	virtual ~Keyboard();
	
	void Start( void );
	void Stop( void );
	
	std::deque<char> GetEvents( void );
	bool KeyDown( char key );
};
