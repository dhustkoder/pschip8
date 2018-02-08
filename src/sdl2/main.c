#include <stdlib.h>
#include <SDL2/SDL_main.h>
#include "system.h"
#include "pschip8.h"


int main(int argc, char** argv)
{
	init_system();
	pschip8();
	term_system();
	return EXIT_SUCCESS;
}

