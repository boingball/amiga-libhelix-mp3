from pathlib import Path

p = Path('amiga_mp3gui.c')
s = p.read_text()

def r(old, new, count=1):
    global s
    actual = s.count(old)
    if actual != count:
        raise SystemExit(f'expected {count}, found {actual}: {old[:100]!r}')
    s = s.replace(old, new)

r('''typedef struct HelixAmp3Player {
\tvolatile int stopRequested;
\tint argc;
\tchar **argv;
\tstruct Process *process;
} HelixAmp3Player;
''','''typedef struct HelixAmp3Player {
\tvolatile int stopRequested;
\tint argc;
\tchar **argv;
\tstruct Process *process;
\tstruct Message *doneMsg;
} HelixAmp3Player;
''')

r('''static HelixAmp3Player gGuiPlayer;
static HelixAmp3Args gGuiArgs;
static struct Message gDoneMsg;
static struct MsgPort *gDonePort;
''','''static HelixAmp3Player gGuiPlayer;
static HelixAmp3Args gGuiArgs;
static struct MsgPort *gDonePort;
''')

r('''static void PlaybackEntry(void)
{
\tstruct MsgPort *donePort;
''','''static void PlaybackEntry(void)
{
\tstruct MsgPort *donePort;
\tstruct Message *doneMsg;
''')

r('''\t/* Only the GUI task owns the public process/lifecycle fields.  Publish a
\t * completion message and let HandleDoneSignal() clear them after it has
\t * actually received that message.
\t * Re-assert the node type immediately before PutMsg: StartPlayback()
\t * reinitialises gDoneMsg before launching, but guard here as well in case
\t * any future code path reaches PutMsg without going through StartPlayback. */
\tgDoneRunId = gGuiPlaybackStatus.runId;
\tdonePort = gDonePort;
\tif (donePort) {
\t\tgDoneMsg.mn_Node.ln_Type = NT_MESSAGE;
\t\tPutMsg(donePort, &gDoneMsg);
#ifdef MINIAMP3_DEBUG
\t\tprintf("miniamp3-debug: done message posted\\n");
#endif
\t}
''','''\t/* Each playback owns a distinct message node.  Reusing one static Exec
\t * message across subprocess generations can corrupt a port list if the node
\t * is still linked or being retired when the next Play starts. */
\tgDoneRunId = gGuiPlaybackStatus.runId;
\tdonePort = gDonePort;
\tdoneMsg = gGuiPlayer.doneMsg;
\tif (donePort && doneMsg) {
\t\tdoneMsg->mn_Node.ln_Type = NT_MESSAGE;
\t\tPutMsg(donePort, doneMsg);
#ifdef MINIAMP3_DEBUG
\t\tprintf("miniamp3-debug: per-run done message posted\\n");
#endif
\t}
''')

r('''\t/* Drain any stale done message from a previous cycle before launching.
\t * gDoneMsg is a single static Exec message node, so it must not remain
\t * queued when the next playback subprocess exits and posts it again.
\t * Re-initialise the node fields here: some AmigaOS exec implementations
\t * write NT_FREEMSG (0) into ln_Type when a message is removed from a port
\t * via GetMsg(), which would cause PutMsg() to silently mishandle the node
\t * on the second and subsequent play cycles, leaving the GUI permanently
\t * stuck on "Streaming playback started." */
\t{
\t\tstruct Message *stale;

\t\twhile ((stale = GetMsg(gui->donePort)) != NULL)
\t\t\t;
\t}
\tmemset(&gDoneMsg, 0, sizeof(gDoneMsg));
\tgDoneMsg.mn_Length = sizeof(gDoneMsg);
\tgDoneMsg.mn_Node.ln_Type = NT_MESSAGE;
''','''\t/* A completion node belongs to exactly one child generation.  Drain and
\t * release any orphaned node before allocating the next one. */
\t{
\t\tstruct Message *stale;

\t\twhile ((stale = GetMsg(gui->donePort)) != NULL) {
\t\t\tif (stale == gGuiPlayer.doneMsg)
\t\t\t\tgGuiPlayer.doneMsg = NULL;
\t\t\tFreeMem(stale, sizeof(struct Message));
\t\t}
\t}
\tif (gGuiPlayer.doneMsg) {
\t\tSetStatus(gui, "Previous playback completion is still retiring.");
\t\treturn;
\t}
\tgGuiPlayer.doneMsg = (struct Message *)AllocMem(sizeof(struct Message),
\t\tMEMF_ANY | MEMF_CLEAR);
\tif (!gGuiPlayer.doneMsg) {
\t\tSetStatus(gui, "Cannot allocate playback completion message.");
\t\treturn;
\t}
\tgGuiPlayer.doneMsg->mn_Length = sizeof(struct Message);
\tgGuiPlayer.doneMsg->mn_Node.ln_Type = NT_MESSAGE;
''')

r('''\tif (!gGuiPlayer.process) {
\t\tif (nilOut)
\t\t\tClose(nilOut);
\t\tif (dirLock)
\t\t\tUnLock(dirLock);
\t\tgDonePort = NULL;
\t\tSetStatus(gui, "Cannot start playback process.");
\t\treturn;
\t}
''','''\tif (!gGuiPlayer.process) {
\t\tif (nilOut)
\t\t\tClose(nilOut);
\t\tif (dirLock)
\t\t\tUnLock(dirLock);
\t\tgDonePort = NULL;
\t\tif (gGuiPlayer.doneMsg) {
\t\t\tFreeMem(gGuiPlayer.doneMsg, sizeof(struct Message));
\t\t\tgGuiPlayer.doneMsg = NULL;
\t\t}
\t\tSetStatus(gui, "Cannot start playback process.");
\t\treturn;
\t}
''')

r('''\tgui->playbackActive = 0;
\tgGuiPlayer.process = NULL;
\tgDonePort = NULL;
''','''\tgui->playbackActive = 0;
\tgGuiPlayer.process = NULL;
\tgDonePort = NULL;
\tif (gGuiPlayer.doneMsg) {
\t\tFreeMem(gGuiPlayer.doneMsg, sizeof(struct Message));
\t\tgGuiPlayer.doneMsg = NULL;
\t}
''')

r('''\tif (gui->donePort) {
\t\tstruct Message *msg;

\t\tgDonePort = NULL;
\t\twhile ((msg = GetMsg(gui->donePort)) != NULL)
\t\t\t;
\t\tDeleteMsgPort(gui->donePort);
\t\tgui->donePort = NULL;
\t}
''','''\tif (gui->donePort) {
\t\tstruct Message *msg;

\t\tgDonePort = NULL;
\t\twhile ((msg = GetMsg(gui->donePort)) != NULL) {
\t\t\tif (msg == gGuiPlayer.doneMsg)
\t\t\t\tgGuiPlayer.doneMsg = NULL;
\t\t\tFreeMem(msg, sizeof(struct Message));
\t\t}
\t\tif (gGuiPlayer.doneMsg) {
\t\t\tFreeMem(gGuiPlayer.doneMsg, sizeof(struct Message));
\t\t\tgGuiPlayer.doneMsg = NULL;
\t\t}
\t\tDeleteMsgPort(gui->donePort);
\t\tgui->donePort = NULL;
\t}
''')

p.write_text(s)
