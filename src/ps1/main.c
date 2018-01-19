#include "system.h"
#include "pschip8.h"

void __attribute__((noreturn)) main(void)
{
	init_system();
	for (;;)
		pschip8();
}

