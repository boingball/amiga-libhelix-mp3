from pathlib import Path

p = Path('amiga_mp3gui.c')
s = p.read_text()

def rep(old, new, marker, error):
    global s
    if marker in s:
        return
    if old not in s:
        raise SystemExit(error)
    s = s.replace(old, new, 1)

rep(
'''#define TRANSPORT_W     96
#define TRANSPORT_H     22
#define PLAY_X          (GUI_MARGIN_L + 126)
#define STOP_X          (GUI_MARGIN_L + 330)
''',
'''#define TRANSPORT_W     48
#define TRANSPORT_H     20
#define PLAY_X          (GUI_MARGIN_L + 154)
#define STOP_X          (GUI_MARGIN_L + 306)
''',
'#define TRANSPORT_W     48',
'transport geometry block not found')

# Add a small custom icon drawer.  The buttons stay native GadTools buttons, but
# the symbols are drawn manually so they are not dependent on the Workbench font.
insert_after = '''static void DrawArtPanel(HelixAmp3Gui *gui)
{
'''
icon_func = r'''static void DrawTransportIcons(HelixAmp3Gui *gui)
{
	struct RastPort *rp;
	int playX;
	int playY;
	int stopX;
	int stopY;
	int i;

	if (!gui || !gui->win || !gui->gadPlay || !gui->gadStop)
		return;
	rp = gui->win->RPort;
	SetAPen(rp, 1);
	playX = gui->gadPlay->LeftEdge + (gui->gadPlay->Width / 2) - 5;
	playY = gui->gadPlay->TopEdge + (gui->gadPlay->Height / 2) - 5;
	for (i = 0; i < 10; i++) {
		int half = i / 2;
		RectFill(rp, playX + i, playY + 5 - half, playX + i,
			playY + 5 + half);
	}
	stopX = gui->gadStop->LeftEdge + (gui->gadStop->Width / 2) - 6;
	stopY = gui->gadStop->TopEdge + (gui->gadStop->Height / 2) - 5;
	RectFill(rp, stopX, stopY, stopX + 3, stopY + 9);
	RectFill(rp, stopX + 8, stopY, stopX + 11, stopY + 9);
}

'''
if 'static void DrawTransportIcons(HelixAmp3Gui *gui)' not in s:
    pos = s.find(insert_after)
    if pos < 0:
        raise SystemExit('DrawArtPanel insertion point not found')
    s = s[:pos] + icon_func + s[pos:]

rep(
'''\tgui->gadFastLowrate = gad = MakeGadget(gui, gad, CHECKBOX_KIND, GID_FAST_LOWRATE,
\t\tGUI_MARGIN_L + 14, ROW_CHECKS, 20, 12, "Fast-lr",
''',
'''\tgad = MakeGadget(gui, gad, TEXT_KIND, GID_COUNT,
\t\tGUI_MARGIN_L + 14, ROW_CHECKS - 1, 68, 16, "",
\t\tGTTX_Text, (ULONG)"Processing:",
\t\tTAG_IGNORE, 0,
\t\tTAG_IGNORE, 0,
\t\tTAG_IGNORE, 0);
\tif (!gad)
\t\treturn -1;

\tgui->gadFastLowrate = gad = MakeGadget(gui, gad, CHECKBOX_KIND, GID_FAST_LOWRATE,
\t\tGUI_MARGIN_L + 96, ROW_CHECKS, 20, 12, "Fast",
''',
'Processing:',
'fast-lowrate checkbox block not found')

rep(
'''\tgui->gadSuperfastLowrate = gad = MakeGadget(gui, gad, CHECKBOX_KIND, GID_SUPERFAST_LOWRATE,
\t\tGUI_MARGIN_L + 116, ROW_CHECKS, 20, 12, "Superfast",
''',
'''\tgui->gadSuperfastLowrate = gad = MakeGadget(gui, gad, CHECKBOX_KIND, GID_SUPERFAST_LOWRATE,
\t\tGUI_MARGIN_L + 178, ROW_CHECKS, 20, 12, "Superfast",
''',
'GUI_MARGIN_L + 178, ROW_CHECKS',
'superfast checkbox block not found')

rep(
'''\tgui->gadFastMem = gad = MakeGadget(gui, gad, CHECKBOX_KIND, GID_FAST_MEM,
\t\tGUI_MARGIN_L + 240, ROW_CHECKS, 20, 12, "Fast-mem",
''',
'''\tgui->gadFastMem = gad = MakeGadget(gui, gad, CHECKBOX_KIND, GID_FAST_MEM,
\t\tGUI_MARGIN_L + 314, ROW_CHECKS, 20, 12, "Fast-mem",
''',
'GUI_MARGIN_L + 314, ROW_CHECKS',
'fast-mem checkbox block not found')

rep(
'''\tgui->gadPlay = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_PLAY,
\t\tPLAY_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, ">",
''',
'''\tgui->gadPlay = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_PLAY,
\t\tPLAY_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, "",
''',
'PLAY_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, ""',
'play button block not found')

rep(
'''\tgui->gadStop = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_STOP,
\t\tSTOP_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, "[]",
''',
'''\tgui->gadStop = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_STOP,
\t\tSTOP_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, "",
''',
'STOP_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, ""',
'stop button block not found')

rep(
'''\tDrawArtPanel(gui);
}
''',
'''\tDrawArtPanel(gui);
\tDrawTransportIcons(gui);
}
''',
'DrawTransportIcons(gui);',
'GuiRefresh icon redraw block not found')

rep(
'''\tDrawProgress(gui);
\tDrawArtPanel(gui);
\tif (gui->timerOpen)
''',
'''\tDrawProgress(gui);
\tDrawArtPanel(gui);
\tDrawTransportIcons(gui);
\tif (gui->timerOpen)
''',
'DrawTransportIcons(gui);\n\tif (gui->timerOpen)',
'GuiOpen icon draw block not found')

p.write_text(s)
