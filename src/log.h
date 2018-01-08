#ifndef PSCHIP8_LOG_H_
#define PSCHIP8_LOG_H_
#include <stdio.h>

#define loginfo(...)  printf("INFO: " __VA_ARGS__)
#define logerror(...) printf("ERROR: " __VA_ARGS__)

#ifdef DEBUG
#define logdebug(...) printf("DEBUG: " __VA_ARGS__)
#else
#define logdebug(...)
#endif


#endif
