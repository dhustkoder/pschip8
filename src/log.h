#ifndef PSCHIP8_LOG_H_
#define PSCHIP8_LOG_H_

#define loginfo printf
#define logerror(x) fprintf(stderr, #x)

#ifdef DEBUG
#define logdebug printf
#else
#define logdebug ((void)0)
#endif


#endif
