#include "system.h"
#include "pschip8.h"

void __attribute__((noreturn)) main(void)
{
	init_system();
	pschip8();
	exit(0);
}

