#ifndef PSCHIP8_LOG_H_
#define PSCHIP8_LOG_H_

#define loginfo printf
#define logerror(x) fprintf(stderr, #x)

#ifdef DEBUG
#define logdebug printf
#else
#define logdebug(...)
#endif


#endif
