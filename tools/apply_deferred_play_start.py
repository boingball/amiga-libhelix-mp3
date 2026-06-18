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
'''\tint artValid;
\tint artLoading;
''',
'''\tint artValid;
\tint artLoading;
\tint artRestartPending;
\tint playStartPending;
''',
'playStartPending;',
'GUI state fields not found')

rep(
'''static void StopPlayback(HelixAmp3Gui *gui)
{
\tif (!gui->playbackActive) {
\t\tSetStatus(gui, "Nothing is playing.");
\t\treturn;
\t}
''',
'''static void StopPlayback(HelixAmp3Gui *gui)
{
\tif (gui->playStartPending && !gui->playbackActive) {
\t\tgui->playStartPending = 0;
\t\tif (gui->artRestartPending) {
\t\t\tgui->artRestartPending = 0;
\t\t\tStartArtDecode(gui);
\t\t}
\t\tSetStatus(gui, "Playback start cancelled.");
\t\treturn;
\t}
\tif (!gui->playbackActive) {
\t\tSetStatus(gui, "Nothing is playing.");
\t\treturn;
\t}
''',
'Playback start cancelled.',
'StopPlayback entry not found')

rep(
'''\tcase GID_PLAY:
\t\tStartPlayback(gui);
\t\tbreak;
''',
'''\tcase GID_PLAY:
\t\tif (gui->playbackActive || gui->playbackDonePending ||
\t\t\tgui->playStartPending) {
\t\t\tSetStatus(gui, "Playback is already starting or active.");
\t\t\tbreak;
\t\t}
\t\t/* Always defer process creation to the next normal timer tick.  This lets
\t\t * the ASL/Intuition event unwind fully before a child inherits DOS state.
\t\t * If artwork is mid-decode, restart it after audio reaches PLAYING. */
\t\tif (gui->artDecode.active || gui->artLoading) {
\t\t\tgui->artDecode.active = 0;
\t\t\tgui->artRestartPending = 1;
\t\t\tgui->artLoading = 1;
\t\t}
\t\tgui->playStartPending = 1;
\t\tSetStatus(gui, "Preparing playback...");
\t\tSendTimerRequest(gui, TIMER_TICK_MICROS);
\t\tbreak;
''',
'Always defer process creation to the next normal timer tick.',
'GID_PLAY handler not found')

rep(
'''\tgui->timerPending = 0;
\tgui->timerIsArt = 0;

\t/* Poll the done port on every tick while playback is active so that a
''',
'''\tgui->timerPending = 0;
\tgui->timerIsArt = 0;

\tif (gui->playStartPending && !gui->playbackActive &&
\t\t!gui->playbackDonePending) {
\t\tgui->playStartPending = 0;
\t\tStartPlayback(gui);
\t}

\t/* Poll the done port on every tick while playback is active so that a
''',
'if (gui->playStartPending && !gui->playbackActive',
'HandleTimerSignal start not found')

rep(
'''\tcase GUIPLAY_PHASE_PLAYING: {
''',
'''\tcase GUIPLAY_PHASE_PLAYING: {
\t\t\tif (gui->artRestartPending) {
\t\t\t\tgui->artRestartPending = 0;
\t\t\t\tStartArtDecode(gui);
\t\t\t}
''',
'if (gui->artRestartPending)',
'PLAYING phase block not found')

rep(
'''\tgGuiPlayer.stopRequested = 0;
\tgPlaybackInterrupted = 0;
''',
'''\tgGuiPlayer.stopRequested = 0;
\tgPlaybackInterrupted = 0;
\tgui->playStartPending = 0;
\tif (gui->artRestartPending) {
\t\tgui->artRestartPending = 0;
\t\tStartArtDecode(gui);
\t}
''',
'gui->playStartPending = 0;',
'FinalizePlayback reset point not found')

p.write_text(s)
