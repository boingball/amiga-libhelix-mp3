#ifndef _AMIGA_PROFILE_DECODE_H
#define _AMIGA_PROFILE_DECODE_H

#include "mp3dec.h"

#ifdef AMIGA_PROFILE_DECODE
#define AMIGA_PROFILE_START(t) ((t) = clock())
#define AMIGA_PROFILE_STOP(bucket, t) \
	do { MP3AddDecodeCoreProfile((bucket), clock() - (t)); } while (0)
#else
#define AMIGA_PROFILE_START(t) ((void)(t))
#define AMIGA_PROFILE_STOP(bucket, t) \
	do { (void)(bucket); (void)(t); } while (0)
#endif

#endif /* _AMIGA_PROFILE_DECODE_H */
