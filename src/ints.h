#ifndef STDINT_H
#ifndef PSCHIP8_INT_H_
#define PSCHIP8_INT_H_

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;


typedef uint8_t bool;
#define false ((bool)0)
#define true  ((bool)1)


#define UINT8_MAX  ((uint8_t)0xFF)
#define UINT8_MIN  ((uint8_t)0x00)
#define UINT16_MAX ((uint16_t)0xFFFF)
#define UINT16_MIN ((uint16_t)0x0000)
#define UINT32_MAX ((uint32_t)0xFFFFFFFF)
#define UINT32_MIN ((uint32_t)0x00000000)
#define INT8_MAX   ((int8_t)0x7F)
#define INT8_MIN   ((int8_t)0xFF)
#define INT16_MAX  ((int16_t)0x7FFF)
#define INT16_MIN  ((int16_t)0xFFFF)
#define INT32_MAX  ((int32_t)0x7FFFFFFF)
#define INT32_MIN  ((int32_t)0xFFFFFFFF)

#endif
#endif
