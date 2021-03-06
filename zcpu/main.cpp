#include <iostream>
#include <cstdlib>
#include "zcpu.h"
#include "../zmemory/zmemory.h"

#include "../zglobal/zglobaldefines.h"

#define PLATFORM_LINUX_CONSOLE

#if defined(PLATFORM_LINUX_CONSOLE)
#include <curses.h>
#endif

int zVersion=3;

using namespace std;

long filesize(FILE *stream)
{
   long curpos, length;
   curpos = ftell(stream);
   fseek(stream, 0L, SEEK_END);
   length = ftell(stream);
   fseek(stream, curpos, SEEK_SET);
   return length;
}

int main()
{
	#if defined PLATFORM_LINUX_CONSOLE
	initscr();
	echo();
	clear(); refresh();
	keypad(stdscr, true);
	scrollok(stdscr, true);
	printw("Zebra Z-Machine Interpreter v1.0 alpha\n");
	printw("Code licensed under GPL v3 by Stephen Zhang\n");
	printw("You are running the Linux version, powered by NCURSES\n\n");
	refresh();
	#endif
  FILE* storyFile=fopen("zork2.z3", "r");
  if(storyFile==NULL) return -1;
  zbyte* storyData=new zbyte[filesize(storyFile)];
  fread(storyData, filesize(storyFile), 1, storyFile);
  ZMemory zMem(storyData, filesize(storyFile));
	ZHeaderData header;
	// initalize header
	header.status_line_not_available=true;
	header.screen_splitting_available=false;
	header.variable_pitch_font_default=false;
	header.colours_available=false;
	header.picture_disp_available=false;
	header.boldface_available=false;
	header.italic_available=false;
	header.fixed_space_font_available=true;
	header.sound_effects_available=false;
	header.timed_kb_input_available=false;
	header.transcripting_on=false;
	header.force_printing_fixed_pitch_font=true;
	header.game_wants_pictures=false;
	header.game_wants_undo=false;
	header.game_wants_mouse=false;
	header.game_wants_colours=false;
	header.game_wants_sound_effects=false;
	header.game_wants_menus=false;
	header.interpreter_number=ZHeaderData::IBM_PC;
	header.interpreter_version=1;
	header.screen_height=25;
	header.screen_width=80;
	header.standard_revision_number=1;
	zMem.writeHeaderData(header);
	ZObjectTable zObj(&zMem);
	ZStack zStack;
	ZInOut zInOut;
	ZDictionary zDict(&zMem);
	ZCpu c(zMem, zStack, zObj, zInOut, zDict);
	c.startExecution();
	#ifdef PLATFORM_LINUX_CONSOLE
	printw("Looks like interpretation thread has exited. Press any key to quit\n");
	getch();
	endwin();
	#endif
    return 0;
}
