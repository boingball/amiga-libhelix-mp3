from pathlib import Path
import re

p = Path('amiga_mp3dec.c')
s = p.read_text()

probe_decl = '''#define GUISTART_FILL_PLANAR_ENTER     201
#define GUISTART_FILL_PLANAR_READ      202
#define GUISTART_FILL_PLANAR_SYNC      203
#define GUISTART_FILL_PLANAR_DECODE    204
#define GUISTART_FILL_PLANAR_DECODED   205
#define GUISTART_FILL_PLANAR_CONVERT   206
#define GUISTART_FILL_PLANAR_COPIED    207
static volatile int gFirstFillDiagnosticStage;

'''
if 'static volatile int gFirstFillDiagnosticStage;' not in s:
    marker = 'typedef struct DecodeStream {'
    pos = s.find(marker)
    if pos < 0:
        raise SystemExit('DecodeStream declaration not found')
    s = s[:pos] + probe_decl + s[pos:]

start = s.find('static int DecodeStreamFillPlanarS8(')
end = s.find('\n/* Shared status block', start)
if start < 0 or end < 0:
    raise SystemExit('planar fill function bounds not found')
f = s[start:end]

f = re.sub(
    r'\t(?:if \(gGuiPlaybackStatus\.startupStage[^\n]*\)\n\t+)?gGuiPlaybackStatus\.startupStage = GUISTART_FILL_PLANAR_[A-Z]+;\n',
    '', f)

def add_before(needle, code, marker):
    global f
    if marker in f:
        return
    pos = f.find(needle)
    if pos < 0:
        raise SystemExit('missing marker: ' + marker)
    f = f[:pos] + code + f[pos:]

add_before(
    '\tproduced = 0;\n',
    '\tif (gFirstFillDiagnosticStage == 0)\n'
    '\t\tgFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_ENTER;\n',
    'gFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_ENTER;')
add_before(
    '\t\t\tnRead = FillReadBuffer(',
    '\t\t\tif (gFirstFillDiagnosticStage > 0 && gFirstFillDiagnosticStage < 208)\n'
    '\t\t\t\tgFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_READ;\n',
    'gFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_READ;')
add_before(
    '\t\toffset = FindValidatedMpegSync(',
    '\t\tif (gFirstFillDiagnosticStage > 0 && gFirstFillDiagnosticStage < 208)\n'
    '\t\t\tgFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_SYNC;\n',
    'gFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_SYNC;')
add_before(
    '\t\tif (stream->timing) {\n',
    '\t\tif (gFirstFillDiagnosticStage > 0 && gFirstFillDiagnosticStage < 208)\n'
    '\t\t\tgFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_DECODE;\n',
    'gFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_DECODE;')
add_before(
    '\t\tif (gPlaybackInterrupted)\n\t\t\tbreak;\n',
    '\t\tif (gFirstFillDiagnosticStage > 0 && gFirstFillDiagnosticStage < 208)\n'
    '\t\t\tgFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_DECODED;\n',
    'gFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_DECODED;')
add_before(
    '\t\tMP3GetLastFrameInfo(stream->decoder, &info);\n',
    '\t\tif (gFirstFillDiagnosticStage > 0 && gFirstFillDiagnosticStage < 208)\n'
    '\t\t\tgFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_CONVERT;\n',
    'gFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_CONVERT;')
add_before(
    '\t\tif (stream->timing)\n\t\t\tstream->timing->pcmConvert += clock() - t0;\n',
    '\t\tif (gFirstFillDiagnosticStage > 0 && gFirstFillDiagnosticStage < 208)\n'
    '\t\t\tgFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_COPIED;\n',
    'gFirstFillDiagnosticStage = GUISTART_FILL_PLANAR_COPIED;')

s = s[:start] + f + s[end:]
p.write_text(s)
