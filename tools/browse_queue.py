from pathlib import Path
p=Path('amiga_mp3gui.c')
s=p.read_text()
def r(a,b):
    global s
    if s.count(a)!=1: raise SystemExit(a[:80])
    s=s.replace(a,b)
r('\tstruct Gadget  *gadBrowse;\n','')
r('\tchar  inputName[HELIXAMP3_MAX_PATH];\n','\tchar  inputName[HELIXAMP3_MAX_PATH];\n\tchar  queuedInputName[HELIXAMP3_MAX_PATH];\n')
r('\tgui->gadBrowse = gad = MakeGadget(gui, gad, BUTTON_KIND, GID_BROWSE,\n','\tgad = MakeGadget(gui, gad, BUTTON_KIND, GID_BROWSE,\n')
r('\tif (gui->win && gui->gadBrowse)\n\t\tGT_SetGadgetAttrs(gui->gadBrowse, gui->win, NULL,\n\t\t\tGA_Disabled, FALSE, TAG_DONE);\n','')
r('\tif (gui->gadBrowse)\n\t\tGT_SetGadgetAttrs(gui->gadBrowse, gui->win, NULL,\n\t\t\tGA_Disabled, TRUE, TAG_DONE);\n','')
r('''\t/* Replacing tags/artwork while the playback child and artwork timer still
\t * reference the current selection can corrupt Intuition/ASL state and raise
\t * a recoverable alert.  File changes are intentionally a stopped-state action. */
\tif (gui->playbackActive || gui->playbackDonePending) {
\t\tSetStatus(gui, "Stop playback before choosing another file.");
\t\treturn;
\t}
''','')
r('\tif (AslRequest(req, NULL)) {\n','\tif (AslRequestTags(req, ASLFR_Window, (ULONG)gui->win,\n\t\tASLFR_SleepWindow, TRUE, TAG_DONE)) {\n')
old='''\t\tSafeCopy(gui->inputName, sizeof(gui->inputName), path);
\t\tSetFileDisplay(gui, gui->inputName);
\t\tReadMp3Tags(gui->inputName, &gui->tags, gui->artEnabled);
\t\tgui->totalSecs = gui->tags.durationSecs;
\t\tgui->elapsedSecs = 0;
\t\tUpdateTagDisplay(gui);
\t\tUpdateArtDisplay(gui);
\t\tDrawProgress(gui);
\t\tif (gui->artDecode.active)
\t\t\tSendTimerRequest(gui, ART_TIMER_MICROS);
\t\tif (!gui->artDecode.active) {
\t\t\tFormatReadyStatus(&gui->tags, gui->statusText, sizeof(gui->statusText));
\t\t\tSetStatus(gui, gui->statusText);
\t\t}
\t\tGuiDisableFastMemIfTooSmall(gui);
'''
new='''\t\tif (gui->playbackActive || gui->playbackDonePending) {
\t\t\tSafeCopy(gui->queuedInputName, sizeof(gui->queuedInputName), path);
\t\t\tSetStatus(gui, "Selected for next Play.");
\t\t} else {
\t\t\tCancelArtDecode(gui);
\t\t\tSafeCopy(gui->inputName, sizeof(gui->inputName), path);
\t\t\tSetFileDisplay(gui, gui->inputName);
\t\t\tReadMp3Tags(gui->inputName, &gui->tags, gui->artEnabled);
\t\t\tgui->totalSecs = gui->tags.durationSecs;
\t\t\tgui->elapsedSecs = 0;
\t\t\tUpdateTagDisplay(gui);
\t\t\tUpdateArtDisplay(gui);
\t\t\tDrawProgress(gui);
\t\t\tif (gui->artDecode.active)
\t\t\t\tSendTimerRequest(gui, ART_TIMER_MICROS);
\t\t\tif (!gui->artDecode.active) {
\t\t\t\tFormatReadyStatus(&gui->tags, gui->statusText, sizeof(gui->statusText));
\t\t\t\tSetStatus(gui, gui->statusText);
\t\t\t}
\t\t\tGuiDisableFastMemIfTooSmall(gui);
\t\t}
'''
r(old,new)
marker='''#endif
}

static void HandleTimerSignal(HelixAmp3Gui *gui)
'''
insert='''#endif
\tif (gui->closeRequested) {
\t\tgui->queuedInputName[0] = '\\0';
\t} else if (gui->queuedInputName[0]) {
\t\tchar queued[HELIXAMP3_MAX_PATH];
\t\tSafeCopy(queued, sizeof(queued), gui->queuedInputName);
\t\tgui->queuedInputName[0] = '\\0';
\t\tCancelArtDecode(gui);
\t\tSafeCopy(gui->inputName, sizeof(gui->inputName), queued);
\t\tSetFileDisplay(gui, gui->inputName);
\t\tReadMp3Tags(gui->inputName, &gui->tags, gui->artEnabled);
\t\tgui->totalSecs = gui->tags.durationSecs;
\t\tgui->elapsedSecs = 0;
\t\tgui->launchBufferSecs = 0;
\t\tUpdateTagDisplay(gui);
\t\tUpdateArtDisplay(gui);
\t\tDrawProgress(gui);
\t\tif (gui->artDecode.active)
\t\t\tSendTimerRequest(gui, ART_TIMER_MICROS);
\t\telse
\t\t\tSetStatus(gui, "Next file ready.");
\t}
}

static void HandleTimerSignal(HelixAmp3Gui *gui)
'''
r(marker,insert)
p.write_text(s)
