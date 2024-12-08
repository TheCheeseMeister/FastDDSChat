#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>

extern std::vector<std::string> endThreadSignal = {};
//extern std::vector<std::string> curr_chat_tab;

// for colors
#ifdef _WIN32
enum Color {
	// Windows Color Values
	DEFAULT_WHITE = 7,
	//BLACK = 0,
	BLUE = 1,
	GREEN = 2,
	CYAN = 3,
	RED = 4,
	MAGENTA = 5,
	YELLOW = 14,
	WHITE = 15,
};
#else
enum Color {
	// Linux ANSI Codes
	DEFAULT_WHITE = 0,
	//BLACK = 30,
	RED = 31,
	GREEN = 32,
	YELLOW = 33,
	BLUE = 34,
	MAGENTA = 35,
	CYAN = 36,
	WHITE = 37,
};
#endif

extern void setTextColor(Color color);
extern void resetTextColor();

#endif