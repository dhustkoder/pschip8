#include <stdlib.h>
#include <SDL2/SDL_main.h>
#include "system.h"
#include "pschip8.h"


int main(int argc, char** argv)
{
	init_system();
	struct game_list* gamelist = open_game_list();
	for (int i = 0; i < gamelist->size; ++i)
		LOGINFO("GAME: %s", gamelist->files[i]);
	close_game_list(gamelist);
	pschip8();
	term_system();
	return EXIT_SUCCESS;
}

