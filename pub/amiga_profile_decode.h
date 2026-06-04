#ifndef _AMIGA_PROFILE_DECODE_H
#define _AMIGA_PROFILE_DECODE_H

#include "mp3dec.h"

#ifdef AMIGA_PROFILE_DECODE
#define AMIGA_PROFILE_START(t) \
	do { if (MP3DecodeCoreProfileIsEnabled()) (t) = clock(); else (t) = 0; } while (0)
#define AMIGA_PROFILE_STOP(bucket, t) \
	do { if (MP3DecodeCoreProfileIsEnabled()) MP3AddDecodeCoreProfile((bucket), clock() - (t)); } while (0)
#else
#define AMIGA_PROFILE_START(t) ((void)(t))
#define AMIGA_PROFILE_STOP(bucket, t) \
	do { (void)(bucket); (void)(t); } while (0)
#endif

#endif /* _AMIGA_PROFILE_DECODE_H */
