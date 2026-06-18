#!/usr/bin/env python3
from pathlib import Path

path = Path("amiga_mp3dec.c")
text = path.read_text(encoding="utf-8")

replacements = []

replacements.append((
"""volatile int gMiniAmp3EmbeddedPlayback;

static int MiniAmp3ConsoleSuppressed(void)
""",
"""volatile int gMiniAmp3EmbeddedPlayback;

#if defined(AMIGA_M68K)
/* Shared GUI/decoder stop latch. */
static volatile int gPlaybackInterrupted;
#else
static volatile sig_atomic_t gPlaybackInterrupted;
#endif

static int MiniAmp3ConsoleSuppressed(void)
"""))

replacements.append((
"""static int InputSourcePreloadFastMemory(InputSource *input)
{
\tlong fileSize;
\tunsigned char *memory;
\tsize_t nRead;
#ifdef HAVE_AMIGA_AUDIO_DEVICE
\tif (input->useAmigaDos) {
\t\tLONG oldPos;
\t\tLONG endPos;

\t\tif (!input->amigaFile)
\t\t\treturn -1;
\t\toldPos = Seek(input->amigaFile, 0, OFFSET_END);
\t\tif (oldPos < 0) {
\t\t\tSeek(input->amigaFile, 0, OFFSET_BEGINNING);
\t\t\treturn -1;
\t\t}
\t\tendPos = Seek(input->amigaFile, 0, OFFSET_CURRENT);
\t\tif (endPos <= 0 || (unsigned long)endPos > (unsigned long)(size_t)-1) {
\t\t\tSeek(input->amigaFile, 0, OFFSET_BEGINNING);
\t\t\treturn -1;
\t\t}
\t\tfileSize = endPos;
\t\tif (Seek(input->amigaFile, 0, OFFSET_BEGINNING) < 0)
\t\t\treturn -1;
\t\tmemory = (unsigned char *)AllocFastInputMemory((unsigned long)fileSize);
\t\tif (!memory)
\t\t\treturn -1;
\t\tnRead = InputSourceRead(input, memory, (size_t)fileSize);
\t\tif (nRead != (size_t)fileSize) {
\t\t\tFreeFastInputMemory(memory, (unsigned long)fileSize);
\t\t\tSeek(input->amigaFile, 0, OFFSET_BEGINNING);
\t\t\treturn -1;
\t\t}
\t\tinput->memory = memory;
\t\tinput->memorySize = (unsigned long)fileSize;
\t\tinput->memoryPos = 0;
\t\tprintf("fast-mem input preload: copying %lu bytes to Fast RAM\\n", input->memorySize);
\t\treturn 0;
\t}
#endif
\tif (fseek(input->file, 0, SEEK_END) != 0)
\t\treturn -1;
\tfileSize = ftell(input->file);
\tif (fileSize <= 0 || (unsigned long)fileSize > (unsigned long)(size_t)-1) {
\t\tfseek(input->file, 0, SEEK_SET);
\t\treturn -1;
\t}
\tif (fseek(input->file, 0, SEEK_SET) != 0)
\t\treturn -1;
\tmemory = (unsigned char *)AllocFastInputMemory((unsigned long)fileSize);
\tif (!memory)
\t\treturn -1;
\tnRead = fread(memory, 1, (size_t)fileSize, input->file);
\tif (nRead != (size_t)fileSize) {
\t\tFreeFastInputMemory(memory, (unsigned long)fileSize);
\t\tfseek(input->file, 0, SEEK_SET);
\t\treturn -1;
\t}
\tinput->memory = memory;
\tinput->memorySize = (unsigned long)fileSize;
\tinput->memoryPos = 0;
\tprintf("fast-mem input preload: copying %lu bytes to Fast RAM\\n", input->memorySize);
\treturn 0;
}
""",
"""#define FAST_INPUT_PRELOAD_CHUNK 32768UL

static int FastInputPreloadStopRequested(void)
{
#ifdef HAVE_AMIGA_AUDIO_DEVICE
\tif (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
\t\tgPlaybackInterrupted = 1;
#endif
\treturn gPlaybackInterrupted != 0;
}

static int InputSourcePreloadFastMemory(InputSource *input)
{
\tlong fileSize;
\tunsigned char *memory;
\tsize_t copied;
#ifdef HAVE_AMIGA_AUDIO_DEVICE
\tif (input->useAmigaDos) {
\t\tLONG oldPos;
\t\tLONG endPos;

\t\tif (!input->amigaFile)
\t\t\treturn -1;
\t\toldPos = Seek(input->amigaFile, 0, OFFSET_END);
\t\tif (oldPos < 0) {
\t\t\tSeek(input->amigaFile, 0, OFFSET_BEGINNING);
\t\t\treturn -1;
\t\t}
\t\tendPos = Seek(input->amigaFile, 0, OFFSET_CURRENT);
\t\tif (endPos <= 0 || (unsigned long)endPos > (unsigned long)(size_t)-1) {
\t\t\tSeek(input->amigaFile, 0, OFFSET_BEGINNING);
\t\t\treturn -1;
\t\t}
\t\tfileSize = endPos;
\t\tif (Seek(input->amigaFile, 0, OFFSET_BEGINNING) < 0)
\t\t\treturn -1;
\t} else
#endif
\t{
\t\tif (fseek(input->file, 0, SEEK_END) != 0)
\t\t\treturn -1;
\t\tfileSize = ftell(input->file);
\t\tif (fileSize <= 0 || (unsigned long)fileSize > (unsigned long)(size_t)-1) {
\t\t\tfseek(input->file, 0, SEEK_SET);
\t\t\treturn -1;
\t\t}
\t\tif (fseek(input->file, 0, SEEK_SET) != 0)
\t\t\treturn -1;
\t}

\tif (FastInputPreloadStopRequested())
\t\treturn 1;
\tmemory = (unsigned char *)AllocFastInputMemory((unsigned long)fileSize);
\tif (!memory)
\t\treturn -1;

\tcopied = 0;
\twhile (copied < (size_t)fileSize) {
\t\tsize_t chunk;
\t\tsize_t nRead;

\t\tif (FastInputPreloadStopRequested()) {
\t\t\tFreeFastInputMemory(memory, (unsigned long)fileSize);
\t\t\tInputSourceSeek(input, 0);
\t\t\treturn 1;
\t\t}
\t\tchunk = (size_t)fileSize - copied;
\t\tif (chunk > (size_t)FAST_INPUT_PRELOAD_CHUNK)
\t\t\tchunk = (size_t)FAST_INPUT_PRELOAD_CHUNK;
\t\tnRead = InputSourceRead(input, memory + copied, chunk);
\t\tif (nRead != chunk) {
\t\t\tFreeFastInputMemory(memory, (unsigned long)fileSize);
\t\t\tInputSourceSeek(input, 0);
\t\t\treturn -1;
\t\t}
\t\tcopied += nRead;
\t}
\tif (FastInputPreloadStopRequested()) {
\t\tFreeFastInputMemory(memory, (unsigned long)fileSize);
\t\tInputSourceSeek(input, 0);
\t\treturn 1;
\t}

\tinput->memory = memory;
\tinput->memorySize = (unsigned long)fileSize;
\tinput->memoryPos = 0;
\tprintf("fast-mem input preload: copying %lu bytes to Fast RAM\\n", input->memorySize);
\treturn 0;
}
"""))

replacements.append((
"""#ifdef AMIGA_M68K
/* Ctrl-C signal handling is unavailable in the libnix build for now. */
static volatile int gPlaybackInterrupted;
#else
static volatile sig_atomic_t gPlaybackInterrupted;

#endif

""",
""""""))

replacements.append((
"""\tif (opt.fastMem)
\t\tGuiPublishStartupStage(GUISTART_INPUT_PRELOAD_FASTMEM);
\tif (opt.fastMem && InputSourcePreloadFastMemory(&input) != 0) {
\t\tfprintf(stderr, "cannot preload input into Fast RAM: %s\\n", opt.inName);
\t\tInputSourceClose(&input);
\t\tCloseInputFile(&infile, opt.debugCleanup);
\t\tfree(resolvedOutName);
\t\tAmigaFreeNormalizedArgs(&normalized);
\t\treturn 1;
\t}
""",
"""\tif (opt.fastMem) {
\t\tint preloadResult;

\t\tGuiPublishStartupStage(GUISTART_INPUT_PRELOAD_FASTMEM);
\t\tpreloadResult = InputSourcePreloadFastMemory(&input);
\t\tif (preloadResult != 0) {
\t\t\tif (preloadResult < 0)
\t\t\t\tfprintf(stderr, "cannot preload input into Fast RAM: %s\\n", opt.inName);
\t\t\tInputSourceClose(&input);
\t\t\tCloseInputFile(&infile, opt.debugCleanup);
\t\t\tfree(resolvedOutName);
\t\t\tAmigaFreeNormalizedArgs(&normalized);
\t\t\treturn 1;
\t\t}
\t}
"""))

for index, (old, new) in enumerate(replacements, 1):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"replacement {index}: expected one match, found {count}")
    text = text.replace(old, new, 1)

path.write_text(text, encoding="utf-8")
