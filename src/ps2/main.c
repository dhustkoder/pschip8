#include "system.h"



void  __attribute__((noreturn)) main(void)
{

	init_system();

	for (;;)
		update_display(true);
	
}
