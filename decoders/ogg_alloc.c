/*
 * ogg_alloc.c - libc-free allocation shim for the OGG/Vorbis decoder module.
 *
 * Same rationale as aac_alloc.c / flac_alloc.c: decoder modules are linked
 * with -nostartfiles and loaded with LoadSeg(), so pulling in the C runtime
 * allocator would drag in startup/exit dependencies that are not available
 * to standalone decoder hunks.  This provides the small malloc/free/calloc/
 * realloc surface needed by libogg and Tremor, backed by Exec
 * AllocMem/FreeMem without referencing the CRT SysBase/DOSBase globals.
 */

#define OGG_ALLOC_IMPLEMENTATION
#include "ogg_alloc.h"

#ifndef HAVE_AMIGA_AUDIO_DEVICE
#error "ogg_alloc.c is for the Amiga decoder module build and must not fall back to libc allocation"
#endif

#include <exec/types.h>
#include <exec/memory.h>

/* magic distinguishes a live allocation from one this shim already freed:
 * Tremor/libogg is third-party code this module cannot audit at the source
 * level, and a double free() reaching straight through to Exec's FreeMem()
 * is what corrupts the memory free-list (observed on real hardware as
 * AN_FreeTwice/AN_BadFreeAddr alerts on the very first OGG stream of a
 * session).  Catching a repeat free() here and turning it into a logged
 * no-op is a safety net around a bug this module can't otherwise pin down,
 * not a fix for whatever in Tremor/libogg is calling free() twice. */
#define OGG_ALLOC_LIVE_MAGIC   0x4F41474CUL /* 'OAGL' */
#define OGG_ALLOC_FREED_MAGIC  0x4F414652UL /* 'OAFR' */

typedef struct OggAllocHeader {
    unsigned long magic;
    unsigned long size;
} OggAllocHeader;

static void *gOggExecBase;

void OggModuleSetExecBase(void *execBase)
{
    gOggExecBase = execBase;
}

static void *OggExecAllocMem(unsigned long bytes, unsigned long flags)
{
    register void *a6 __asm("a6") = gOggExecBase;
    register unsigned long d0 __asm("d0") = bytes;
    register unsigned long d1 __asm("d1") = flags;

    __asm volatile ("jsr a6@(-198:W)"
                    : "+r" (d0)
                    : "r" (d1), "r" (a6)
                    : "a0", "a1", "d2", "cc", "memory");
    return (void *)d0;
}

static void OggExecFreeMem(void *ptr, unsigned long bytes)
{
    register void *a6 __asm("a6") = gOggExecBase;
    register void *a1 __asm("a1") = ptr;
    register unsigned long d0 __asm("d0") = bytes;

    __asm volatile ("jsr a6@(-210:W)"
                    :
                    : "r" (a1), "r" (d0), "r" (a6)
                    : "a0", "d1", "d2", "cc", "memory");
}

static void OggModuleZero(void *ptr, unsigned long bytes)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (bytes--)
        *p++ = 0;
}

static void OggModuleCopy(void *dst, const void *src, unsigned long bytes)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (bytes--)
        *d++ = *s++;
}

void *OggModuleMalloc(size_t bytes)
{
    unsigned long payload = (unsigned long)bytes;
    unsigned long total = payload + (unsigned long)sizeof(OggAllocHeader);
    OggAllocHeader *hdr;

    if (total < payload || !gOggExecBase)
        return NULL;

    hdr = (OggAllocHeader *)OggExecAllocMem(total, MEMF_FAST | MEMF_CLEAR);
    if (!hdr)
        return NULL;

    hdr->magic = OGG_ALLOC_LIVE_MAGIC;
    hdr->size = payload;
    return (void *)(hdr + 1);
}

void *OggModuleCalloc(size_t count, size_t bytes)
{
    size_t total = count * bytes;
    if (bytes != 0 && total / bytes != count)
        return NULL;
    return OggModuleMalloc(total);
}

void *OggModuleRealloc(void *ptr, size_t bytes)
{
    OggAllocHeader *oldHdr;
    void *newPtr;
    unsigned long oldSize;
    unsigned long copySize;

    if (!ptr)
        return OggModuleMalloc(bytes);
    if (bytes == 0) {
        OggModuleFree(ptr);
        return NULL;
    }

    oldHdr = ((OggAllocHeader *)ptr) - 1;
    if (oldHdr->magic != OGG_ALLOC_LIVE_MAGIC)
        return NULL;
    oldSize = oldHdr->size;
    newPtr = OggModuleMalloc(bytes);
    if (!newPtr)
        return NULL;

    copySize = oldSize < (unsigned long)bytes ? oldSize : (unsigned long)bytes;
    OggModuleCopy(newPtr, ptr, copySize);
    if ((unsigned long)bytes > copySize)
        OggModuleZero((unsigned char *)newPtr + copySize, (unsigned long)bytes - copySize);
    OggModuleFree(ptr);
    return newPtr;
}

void OggModuleFree(void *ptr)
{
    OggAllocHeader *hdr;
    unsigned long total;

    if (!ptr)
        return;

    hdr = ((OggAllocHeader *)ptr) - 1;
    if (hdr->magic != OGG_ALLOC_LIVE_MAGIC) {
        /* Already freed by an earlier call (or never a valid allocation from
         * this shim): do NOT trust hdr->size/hdr and hand it to FreeMem --
         * that is exactly how a double free corrupts Exec's memory
         * free-list.  Treat as a no-op instead. */
        return;
    }
    hdr->magic = OGG_ALLOC_FREED_MAGIC;
    total = hdr->size + (unsigned long)sizeof(OggAllocHeader);
    OggExecFreeMem(hdr, total);
}

void OggModuleExit(int status)
{
    (void)status;
    for (;;)
        ;
}

/*
 * Provide libc-compatible symbol names for libogg/Tremor code that calls
 * them directly (including via the _ogg_malloc/_ogg_free aliases which
 * expand to plain malloc/free in the vendored headers).  Redirect them all
 * to our Exec-backed shim.
 */
void *malloc(size_t bytes)
{
    return OggModuleMalloc(bytes);
}

void *calloc(size_t count, size_t bytes)
{
    return OggModuleCalloc(count, bytes);
}

void *realloc(void *ptr, size_t bytes)
{
    return OggModuleRealloc(ptr, bytes);
}

void free(void *ptr)
{
    OggModuleFree(ptr);
}

void *_malloc_r(void *reent, size_t bytes)
{
    (void)reent;
    return OggModuleMalloc(bytes);
}

void *_calloc_r(void *reent, size_t count, size_t bytes)
{
    (void)reent;
    return OggModuleCalloc(count, bytes);
}

void *_realloc_r(void *reent, void *ptr, size_t bytes)
{
    (void)reent;
    return OggModuleRealloc(ptr, bytes);
}

void _free_r(void *reent, void *ptr)
{
    (void)reent;
    OggModuleFree(ptr);
}

#if defined(__GNUC__)
void exit(int status) __attribute__((noreturn));
#endif
void exit(int status)
{
    OggModuleExit(status);
    for (;;)
        ;
}

/*
 * Dead-code stubs for Tremor's FILE*-based ov_fopen()/ov_open() API — see
 * the comment in ogg_alloc.h.  None of these are ever actually invoked at
 * runtime; they only need to exist so the linker doesn't reach for real
 * newlib fopen/fread/etc (which would pull in dummy.o's SysBase-dependent
 * syscall stubs).
 */
FILE *OggModuleFopen(const char *path, const char *mode)
{
    (void)path;
    (void)mode;
    return NULL;
}

int OggModuleFclose(FILE *fp)
{
    (void)fp;
    return 0;
}

size_t OggModuleFread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    (void)ptr;
    (void)size;
    (void)nmemb;
    (void)fp;
    return 0;
}

int OggModuleFseek(FILE *fp, long offset, int whence)
{
    (void)fp;
    (void)offset;
    (void)whence;
    return -1;
}

long OggModuleFtell(FILE *fp)
{
    (void)fp;
    return -1;
}

/*
 * Preempt lib_a-dummy.o — see the comment in ogg_alloc.h.  These use the
 * real newlib syscall-stub names directly (no -D redirection anywhere),
 * so the linker resolves them from this object before ever searching
 * libc.a for dummy.o.  None are reachable at runtime by this module; the
 * bodies just fail gracefully in whatever unidentified newlib fallback
 * path was reaching for them.
 */
struct stat; /* opaque: we only need a pointer, never dereference it */

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    (void)st;
    return -1;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    return -1;
}

long _lseek(int fd, long offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    return -1;
}

long lseek(int fd, long offset, int whence)
{
    return _lseek(fd, offset, whence);
}

int _unlink(const char *path)
{
    (void)path;
    return -1;
}

int ioctl(int fd, long request, ...)
{
    (void)fd;
    (void)request;
    return -1;
}

/*
 * Preempt newlib's _impure_ptr (the reentrancy-struct pointer behind the
 * errno macro, among other things).  Tremor's vorbisfile.o references it
 * directly -- almost certainly from an errno check inside the dead
 * ov_fopen()/ov_open() code paths, which --gc-sections does discard from
 * the final binary, but only *after* the linker has already resolved
 * every reference and pulled in real lib_a-impure.o.  That object in turn
 * needs __sf_fake_stderr, which cascades into the entire real stdio
 * implementation and finally lib_a-__dosbase.o's DOSBase -- exactly the
 * class of runtime dependency a -nostartfiles module can't have.
 *
 * A plain zeroed buffer is a safe stand-in: _errno is always the first
 * field of struct _reent across newlib configurations, so any errno
 * read/write against this pointer is a harmless no-op rather than a wild
 * write, and since the only referencing code is dead/unreached, nothing
 * else in struct _reent is ever actually touched.
 */
static unsigned char gOggFakeReent[1024];
void *OggModuleImpurePtr = (void *)gOggFakeReent;

/*
 * Real lib_a-locale.o (pulled in for __locale_ctype_ptr, which we do NOT
 * redirect -- see the Makefile comment) turns out to reference _impure_ptr
 * internally too.  That's pre-compiled newlib code; our -D redirection
 * only rewrites source we compile ourselves, so it can't stop locale.o
 * from reopening the same impure.o -> findfp.o -> stdio.o -> DOSBase
 * cascade through its own reference.  Define the real symbol name
 * directly (same direct-preemption approach as _getpid/_fstat/etc. above)
 * so it's satisfied regardless of who asks for it.
 */
struct _reent *_impure_ptr = (struct _reent *)(void *)gOggFakeReent;
