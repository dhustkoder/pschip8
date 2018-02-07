#include <SDL_main.h>
#include "system.h"
#include "pschip8.h"



int main(int argc, char** argv)
{
	init_system();
	pschip8();
	return 0;
}

