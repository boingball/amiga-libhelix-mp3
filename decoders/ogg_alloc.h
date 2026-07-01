#ifndef DECODERS_OGG_ALLOC_H
#define DECODERS_OGG_ALLOC_H

#include <stddef.h>
#include <stdio.h>

void *OggModuleMalloc(size_t bytes);
void *OggModuleCalloc(size_t count, size_t bytes);
void *OggModuleRealloc(void *ptr, size_t bytes);
void  OggModuleFree(void *ptr);
#if defined(__GNUC__)
void  OggModuleExit(int status) __attribute__((noreturn));
#else
void  OggModuleExit(int status);
#endif
void  OggModuleSetExecBase(void *execBase);

/*
 * Dead-code stubs for Tremor's FILE*-based convenience API (ov_fopen()/
 * ov_open()), which this module never calls — we always go through
 * ov_open_callbacks().  Linking the real newlib fopen/fread/etc would pull
 * in their low-level syscall stubs (dummy.o), which reference SysBase and
 * cannot resolve in a -nostartfiles decoder module.  These just need to
 * exist so the linker is satisfied; they are never actually reached.
 */
FILE  *OggModuleFopen(const char *path, const char *mode);
int    OggModuleFclose(FILE *fp);
size_t OggModuleFread(void *ptr, size_t size, size_t nmemb, FILE *fp);
int    OggModuleFseek(FILE *fp, long offset, int whence);
long   OggModuleFtell(FILE *fp);

/*
 * Preempt newlib's low-level syscall stubs (normally provided by
 * lib_a-dummy.o in this toolchain's libc.a).  Something inside libc.a that
 * we DO need transitively reaches for one of _fstat/_getpid/_kill/_lseek/
 * _unlink/lseek/ioctl (exact chain not identified), which pulls in the
 * whole dummy.o unit — and dummy.o's own _getpid body references SysBase,
 * which cannot resolve in a -nostartfiles decoder module.  Providing the
 * real symbol names ourselves (declared/defined in ogg_alloc.c, no -D
 * redirection needed) means the linker never needs to touch dummy.o at
 * all: object files on the link command line always resolve before
 * archive search.  None of these are ever meaningfully reached at runtime
 * by this module.
 */

#endif /* DECODERS_OGG_ALLOC_H */
