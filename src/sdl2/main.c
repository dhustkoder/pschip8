#include <stdlib.h>
#include <SDL2/SDL_main.h>
#include "system.h"
#include "pschip8.h"


int main(int argc, char** argv)
{
	char** p;
	uint8_t s;
	init_system();
	open_game_list(&p, &s);
	for (int i = 0; i < s; ++i)
		LOGINFO("GAME: %s", p[i]);
	close_game_list(p, s);
	pschip8();
	term_system();
	return EXIT_SUCCESS;
}

