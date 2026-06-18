from pathlib import Path
p=Path('amiga_mp3gui.c')
s=p.read_text()
def r(a,b):
    global s
    if s.count(a)!=1: raise SystemExit('missing: '+a[:100])
    s=s.replace(a,b)
r('#define BROWSE_X        (ART_X - BROWSE_W)\n','#define BROWSE_X        (ART_X - BROWSE_W - 6)\n')
r('''\tgui->gadFakeStereo = gad = MakeGadgetWithTextAttr(gui, gad,
\t\tCHECKBOX_KIND, GID_FAKE_STEREO,
\t\tGUI_MARGIN_L + 92, ROW_CHANNELS, 20, 12, &gTopaz8Attr, "Fake-st",
''','''\tgui->gadFakeStereo = gad = MakeGadget(gui, gad,
\t\tCHECKBOX_KIND, GID_FAKE_STEREO,
\t\tGUI_MARGIN_L + 92, ROW_CHANNELS, 20, 12, "Fake-st",
''')
r('''\tgui->gadFakeStereoWidth = gad = MakeGadgetWithTextAttr(gui, gad,
\t\tCYCLE_KIND, GID_FAKE_STEREO_WIDTH,
\t\tGUI_MARGIN_L + 232, ROW_CHANNELS - 2, 92, 16, &gTopaz8Attr, "Width:",
''','''\tgui->gadFakeStereoWidth = gad = MakeGadget(gui, gad,
\t\tCYCLE_KIND, GID_FAKE_STEREO_WIDTH,
\t\tGUI_MARGIN_L + 232, ROW_CHANNELS - 2, 92, 16, "Width:",
''')
r('''\tgui->gadFakeStereoDelay = gad = MakeGadgetWithTextAttr(gui, gad,
\t\tCYCLE_KIND, GID_FAKE_STEREO_DELAY,
\t\tGUI_MARGIN_L + 414, ROW_CHANNELS - 2, 70, 16, &gTopaz8Attr, "Delay:",
''','''\tgui->gadFakeStereoDelay = gad = MakeGadget(gui, gad,
\t\tCYCLE_KIND, GID_FAKE_STEREO_DELAY,
\t\tGUI_MARGIN_L + 414, ROW_CHANNELS - 2, 70, 16, "Delay:",
''')
r('''\tgui->gadVolume = gad = MakeSliderGadget(gui, gad, GID_VOLUME,
\t\tSLIDER_X, ROW_VOLUME, VOLUME_SLIDER_W, "Volume:",
\t\t0, 100, gui->volumePercent, "%ld%%", 4, 12);
''','''\tgui->gadVolume = gad = MakeSliderGadget(gui, gad, GID_VOLUME,
\t\tSLIDER_X, ROW_VOLUME, VOLUME_SLIDER_W, "Volume:",
\t\t0, 100, gui->volumePercent, "%ld%%", 4, 30);
''')
r('''\tgui->gadPlay = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_PLAY,
\t\tPLAY_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, "> PLAY",
''','''\tgui->gadPlay = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_PLAY,
\t\tPLAY_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, ">",
''')
r('''\tgui->gadStop = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_STOP,
\t\tSTOP_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, "[] STOP",
''','''\tgui->gadStop = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_STOP,
\t\tSTOP_X, ROW_BUTTONS, TRANSPORT_W, TRANSPORT_H, "[]",
''')
r('''\tResetCliParser();
\tResetDecoderStatics();
\tgGuiPlayer.stopRequested = 0;
''','''\tResetCliParser();
\t/* Decoder statics are reset by the next playback child immediately before
\t * entering the decoder.  Do not reset them again from the GUI task after
\t * teardown; keeping all decoder-global mutation in the child avoids a
\t * second-play race on shared process address space. */
\tgGuiPlayer.stopRequested = 0;
''')
r('''\tCancelArtDecode(gui);
\tDrawArtPanel(gui);
\tgui->elapsedSecs = 0;
''','''\t/* Artwork decoding belongs to the GUI task and may continue at the normal
\t * timer-pump rate while the playback child runs.  Keeping it active preserves
\t * the current cover and uses only the existing small per-tick work budget. */
\tgui->elapsedSecs = 0;
''')
p.write_text(s)
