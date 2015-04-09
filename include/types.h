#ifndef TYPES_H
#define TYPES_H

typedef signed char s8;
typedef short s16;
typedef int s32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef s8 int8_t;
typedef s16 int16_t;

typedef u16 __le16;
typedef u32 __le32;

#ifndef __cplusplus
#define true 1
#define false 0
typedef int bool;
#endif

#ifdef __GNUC__
#define __packed __attribute__((packed))
#endif

#ifdef _MSC_VER
#define __packed
#ifndef __cplusplus
#define inline __inline
#endif
#endif

#endif //TYPES_H
