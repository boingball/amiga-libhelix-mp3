from pathlib import Path

# Remove the accidental software low-pass path from the decoder.  The Amiga
# low-pass is a machine-wide analogue hardware filter, not a decoder option.
p = Path('amiga_mp3dec.c')
s = p.read_text()

def remove(text):
    global s
    if text in s:
        s = s.replace(text, '', 1)

remove('\tint lowPass;\n')
remove('\tprintf("  --low-pass   soften Paula output with a light one-pole low-pass filter\\n");\n')
remove('''\t\t} else if (!strcmp(argv[i], "--low-pass")) {
\t\t\topt->lowPass = 1;
''')
remove('''\tint lowPassReady;
\tint lowPassL;
\tint lowPassR;
''')
# Remove the helper function if present.
start = s.find('static void DecodeStreamLowPassPair(')
if start >= 0:
    end = s.find('\nstatic int DecodeStreamCopySpill(', start)
    if end < 0:
        raise SystemExit('DecodeStreamLowPassPair end not found')
    s = s[:start] + s[end+1:]
# Remove calls left inside conversion loops.
remove('''\t\t\tif (opt->lowPass)
\t\t\t\tDecodeStreamLowPassPair(stream, &wl, &wr);
''')
remove('''\t\t\tif (opt->lowPass)
\t\t\t\tDecodeStreamLowPassPair(stream, &wl, &wr);
''')
p.write_text(s)

# Turn the GUI button into a hardware audio-filter toggle.
p = Path('amiga_mp3gui.c')
s = p.read_text()

def rep(old, new, marker, error):
    global s
    if marker in s:
        return
    if old not in s:
        raise SystemExit(error)
    s = s.replace(old, new, 1)

rep('''#include <devices/timer.h>
#include <exec/io.h>
''','''#include <devices/timer.h>
#include <hardware/cia.h>
#include <exec/io.h>
''','#include <hardware/cia.h>','include timer block not found')

rep('''struct IntuitionBase *IntuitionBase;
struct Library *AslBase;
''','''struct IntuitionBase *IntuitionBase;
extern volatile struct CIA ciaa;
struct Library *AslBase;
''','extern volatile struct CIA ciaa;','library globals block not found')

# Rename the stored state from lowPass to hardwareFilter if the software field exists.
s = s.replace('int   lowPass;\n', 'int   hardwareFilter;\n')
s = s.replace('gui->lowPass', 'gui->hardwareFilter')
s = s.replace('"LowPass"', '"HardwareFilter"')

# Add hardware filter setter before the drawn button function.
if 'static void ApplyHardwareAudioFilter' not in s:
    pos = s.find('static void DrawDolbyButton(HelixAmp3Gui *gui)')
    if pos < 0:
        raise SystemExit('DrawDolbyButton insertion point not found')
    fn = r'''
static void ApplyHardwareAudioFilter(HelixAmp3Gui *gui)
{
#if defined(AMIGA_M68K)
	/* The Amiga/CD32 analogue audio filter is controlled through CIA-A port A,
	 * bit 1, the same bit used for the power LED brightness.  It is global to
	 * the machine and independent of audio.device's Paula channel ownership.
	 * Low bit enables the filter/bright LED; high bit disables it/dims LED. */
	Forbid();
	if (gui && gui->hardwareFilter)
		ciaa.ciapra &= (UBYTE)~CIAF_LED;
	else
		ciaa.ciapra |= CIAF_LED;
	Permit();
#else
	(void)gui;
#endif
}

'''
    s = s[:pos] + fn + s[pos:]

# Make the button text/status call it FILTER, not a software/Dolby effect.
s = s.replace('static void DrawDolbyButton(HelixAmp3Gui *gui)', 'static void DrawFilterButton(HelixAmp3Gui *gui)')
s = s.replace('DrawDolbyButton(gui);', 'DrawFilterButton(gui);')
s = s.replace('GID_LOWPASS', 'GID_HARDWARE_FILTER')
s = s.replace('gadLowPass', 'gadHardwareFilter')
s = s.replace('DOLBY_X', 'FILTER_X')
s = s.replace('DOLBY_W', 'FILTER_W')

# Rename constants/enum/gadget if the old names are still present from prior patch.
s = s.replace('#define DOLBY_X         (GUI_MARGIN_L + 390)\n#define DOLBY_W         54\n',
              '#define FILTER_X        (GUI_MARGIN_L + 390)\n#define FILTER_W        54\n')
s = s.replace('GID_LOWPASS,', 'GID_HARDWARE_FILTER,')
s = s.replace('struct Gadget  *gadLowPass;', 'struct Gadget  *gadHardwareFilter;')

# Update the drawn label to read FLT rather than stylised Dolby.  Replacing the whole
# drawer body keeps the earlier button geometry and state indicator.
start = s.find('static void DrawFilterButton(HelixAmp3Gui *gui)')
if start < 0:
    raise SystemExit('DrawFilterButton not found')
end = s.find('\nstatic void DrawArtPanel', start)
if end < 0:
    raise SystemExit('DrawFilterButton end not found')
new_draw = r'''static void DrawFilterButton(HelixAmp3Gui *gui)
{
	struct RastPort *rp;
	int x, y;

	if (!gui || !gui->win || !gui->gadHardwareFilter)
		return;
	rp = gui->win->RPort;
	x = gui->gadHardwareFilter->LeftEdge + 8;
	y = gui->gadHardwareFilter->TopEdge + 5;
	SetAPen(rp, 1);
	Move(rp, x, y);
	Draw(rp, x, y + 9);
	Move(rp, x, y);
	Draw(rp, x + 9, y);
	Move(rp, x, y + 4);
	Draw(rp, x + 7, y + 4);
	Move(rp, x + 14, y);
	Draw(rp, x + 14, y + 9);
	Draw(rp, x + 23, y + 9);
	Move(rp, x + 28, y);
	Draw(rp, x + 39, y);
	Move(rp, x + 33, y);
	Draw(rp, x + 33, y + 9);
	if (gui->hardwareFilter) {
		RectFill(rp, gui->gadHardwareFilter->LeftEdge + 2,
			gui->gadHardwareFilter->TopEdge + 2,
			gui->gadHardwareFilter->LeftEdge + 5,
			gui->gadHardwareFilter->TopEdge + 5);
	}
}

'''
s = s[:start] + new_draw + s[end+1:]

# Make button use renamed gadget/constant if not already transformed.
s = s.replace('gui->gadLowPass = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_LOWPASS,',
              'gui->gadHardwareFilter = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_HARDWARE_FILTER,')
s = s.replace('FILTER_X, ROW_BUTTONS, FILTER_W, TRANSPORT_H, "",',
              'FILTER_X, ROW_BUTTONS, FILTER_W, TRANSPORT_H, "",')

# Do not pass --low-pass to playback args.
s = s.replace('''\tif (gui->hardwareFilter)
\t\tAddArg(args, "--low-pass");
''', '')

# Toggle live and apply immediately.
old = '''\tcase GID_HARDWARE_FILTER:
\t\tif (gui->playbackActive || gui->playbackDonePending) {
\t\t\tSetStatus(gui, "Stop playback before changing low-pass.");
\t\t\tbreak;
\t\t}
\t\tgui->hardwareFilter = !gui->hardwareFilter;
\t\tDrawFilterButton(gui);
\t\tSetStatus(gui, gui->hardwareFilter ? "Low-pass enabled." : "Low-pass disabled.");
\t\tSaveGuiSettings(gui);
\t\tbreak;
'''
new = '''\tcase GID_HARDWARE_FILTER:
\t\tgui->hardwareFilter = !gui->hardwareFilter;
\t\tApplyHardwareAudioFilter(gui);
\t\tDrawFilterButton(gui);
\t\tSetStatus(gui, gui->hardwareFilter ?
\t\t\t"Hardware filter enabled." : "Hardware filter disabled.");
\t\tSaveGuiSettings(gui);
\t\tbreak;
'''
if old in s:
    s = s.replace(old, new, 1)
elif 'Hardware filter enabled.' not in s:
    raise SystemExit('hardware filter action block not found')

# Apply saved state at startup once the window/gadgets exist.
rep('''\tUpdateChannelGadgetState(gui);
''','''\tUpdateChannelGadgetState(gui);
\tApplyHardwareAudioFilter(gui);
''','ApplyHardwareAudioFilter(gui);','GuiOpen channel state block not found')

p.write_text(s)
