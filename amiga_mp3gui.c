/*
 * HelixAMP3 - compact AmigaOS mini-player frontend for the Helix fixed-point
 * MP3 decoder.  The GUI wraps the existing amiga_mp3dec playback frontend so
 * the same Paula streaming path, fast-lowrate options, and buffer handling are
 * used from either Shell or Workbench.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(AMIGA_M68K)
#define main HelixAmp3CliMain
#include "amiga_mp3dec.c"
#undef main
#endif

#ifdef AMIGA_M68K
#include <exec/types.h>
#include <exec/tasks.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/gadtools.h>

#define HELIXAMP3_MAX_PATH 256
#define HELIXAMP3_ARGC_MAX 12
#define HELIXAMP3_SIGMASK(gui) (1UL << (gui)->win->UserPort->mp_SigBit)

#ifndef PLACETEXT_LEFT
#define PLACETEXT_LEFT 0x0001
#endif
#ifndef PLACETEXT_ABOVE
#define PLACETEXT_ABOVE 0x0002
#endif
#ifndef PLACETEXT_IN
#define PLACETEXT_IN 0x0004
#endif
#ifndef PLACETEXT_RIGHT
#define PLACETEXT_RIGHT 0x0008
#endif

enum {
	GID_FILE = 1,
	GID_BROWSE,
	GID_FAST_LOWRATE,
	GID_FAST_MEM,
	GID_MONO,
	GID_RATE,
	GID_BUFFER,
	GID_QUALITY,
	GID_PLAY,
	GID_STOP,
	GID_STATUS,
	GID_COUNT
};

typedef struct HelixAmp3Gui {
	struct Window  *win;
	struct Gadget  *gadgets;
	struct Gadget  *gadContext;
	struct Gadget  *gadFile;
	struct Gadget  *gadStatus;
	struct Gadget  *gadBuffer;
	struct Gadget  *gadPlay;
	struct Gadget  *gadStop;
	struct Gadget  *gadFastLowrate;
	struct Gadget  *gadFastMem;
	struct Gadget  *gadMono;
	struct Gadget  *gadRate;
	struct Gadget  *gadQuality;
	char  inputName[HELIXAMP3_MAX_PATH];
	char  statusText[128];
	int   fastLowrate;
	int   fastMem;
	int   mono;
	int   rateIndex;
	int   bufferSeconds;
	int   qualityIndex;
	int   closeRequested;
} HelixAmp3Gui;

typedef struct HelixAmp3Player {
	volatile int running;
	volatile int stopRequested;
	int argc;
	char *argv[HELIXAMP3_ARGC_MAX];
	char argvStorage[HELIXAMP3_ARGC_MAX][HELIXAMP3_MAX_PATH];
	struct Process *process;
} HelixAmp3Player;

static HelixAmp3Player gGuiPlayer;
struct Library *GadToolsBase;

static const char * const kRates[] = {
	"8287",
	"8820",
	"11025",
	"22050"
};

static UBYTE *kRateLabels[] = {
	(UBYTE *)"8287",
	(UBYTE *)"8820",
	(UBYTE *)"11025",
	(UBYTE *)"22050",
	NULL
};

static UBYTE *kQualityLabels[] = {
	(UBYTE *)"Fast",
	(UBYTE *)"Normal",
	(UBYTE *)"Best",
	NULL
};

static void GuiClose(HelixAmp3Gui *gui);

static void SafeCopy(char *dst, size_t dstSize, const char *src)
{
	if (!dst || dstSize == 0)
		return;
	if (!src)
		src = "";
	strncpy(dst, src, dstSize - 1);
	dst[dstSize - 1] = '\0';
}

static void SetStatus(HelixAmp3Gui *gui, const char *text)
{
	SafeCopy(gui->statusText, sizeof(gui->statusText), text);
	if (gui->win && gui->gadStatus) {
		GT_SetGadgetAttrs(gui->gadStatus, gui->win, NULL,
			GTTX_Text, (ULONG)gui->statusText,
			TAG_DONE);
	}
}

static void UpdateFileDisplay(HelixAmp3Gui *gui)
{
	const char *text = gui->inputName[0] ? gui->inputName : "<choose an MP3 file>";

	if (gui->win && gui->gadFile) {
		GT_SetGadgetAttrs(gui->gadFile, gui->win, NULL,
			GTST_String, (ULONG)text,
			TAG_DONE);
	}
}

static void UpdateBufferDisplay(HelixAmp3Gui *gui)
{
	if (gui->win && gui->gadBuffer) {
		GT_SetGadgetAttrs(gui->gadBuffer, gui->win, NULL,
			GTSL_Level, gui->bufferSeconds,
			TAG_DONE);
	}
}

static struct Gadget *AddGadget(HelixAmp3Gui *gui, struct Gadget *prev,
	ULONG kind, UWORD gid, WORD left, WORD top, WORD width, WORD height,
	UBYTE *label, ULONG labelPlace, ULONG tag1, ULONG data1,
	ULONG tag2, ULONG data2, ULONG tag3, ULONG data3, ULONG tag4, ULONG data4)
{
	struct NewGadget ng;

	memset(&ng, 0, sizeof(ng));
	ng.ng_LeftEdge = left;
	ng.ng_TopEdge = top;
	ng.ng_Width = width;
	ng.ng_Height = height;
	ng.ng_GadgetText = label;
	ng.ng_GadgetID = gid;
	ng.ng_Flags = labelPlace;
	ng.ng_TextAttr = NULL;
	ng.ng_VisualInfo = NULL;
	return CreateGadget(kind, prev, &ng,
		tag1, data1,
		tag2, data2,
		tag3, data3,
		tag4, data4,
		TAG_DONE);
}

static int GuiCreateGadgets(HelixAmp3Gui *gui)
{
	struct Gadget *gad;

	gui->gadContext = CreateContext(&gui->gadgets);
	if (!gui->gadContext)
		return -1;
	gad = gui->gadContext;

	gad = AddGadget(gui, gad, STRING_KIND, GID_FILE, 58, 14, 260, 18,
		(UBYTE *)"File:", PLACETEXT_LEFT,
		GTST_String, (ULONG)"<choose an MP3 file>",
		GTST_MaxChars, HELIXAMP3_MAX_PATH - 1,
		GA_Disabled, TRUE,
		TAG_IGNORE, 0);
	gui->gadFile = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, BUTTON_KIND, GID_BROWSE, 330, 14, 78, 18,
		(UBYTE *)"Browse...", PLACETEXT_IN,
		TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0);
	if (!gad) return -1;

	gad = AddGadget(gui, gad, CHECKBOX_KIND, GID_FAST_LOWRATE, 28, 46, 26, 14,
		(UBYTE *)"Fast-lowrate", PLACETEXT_RIGHT,
		GTCB_Checked, gui->fastLowrate,
		TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0);
	gui->gadFastLowrate = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, CHECKBOX_KIND, GID_FAST_MEM, 168, 46, 26, 14,
		(UBYTE *)"Fast mem", PLACETEXT_RIGHT,
		GTCB_Checked, gui->fastMem,
		TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0);
	gui->gadFastMem = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, CHECKBOX_KIND, GID_MONO, 288, 46, 26, 14,
		(UBYTE *)"Mono", PLACETEXT_RIGHT,
		GTCB_Checked, gui->mono,
		TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0);
	gui->gadMono = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, CYCLE_KIND, GID_RATE, 58, 76, 102, 18,
		(UBYTE *)"Rate:", PLACETEXT_LEFT,
		GTCY_Labels, (ULONG)kRateLabels,
		GTCY_Active, gui->rateIndex,
		TAG_IGNORE, 0, TAG_IGNORE, 0);
	gui->gadRate = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, CYCLE_KIND, GID_QUALITY, 250, 76, 106, 18,
		(UBYTE *)"Quality:", PLACETEXT_LEFT,
		GTCY_Labels, (ULONG)kQualityLabels,
		GTCY_Active, gui->qualityIndex,
		TAG_IGNORE, 0, TAG_IGNORE, 0);
	gui->gadQuality = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, SLIDER_KIND, GID_BUFFER, 72, 108, 210, 18,
		(UBYTE *)"Buffer:", PLACETEXT_LEFT,
		GTSL_Min, 1,
		GTSL_Max, 30,
		GTSL_Level, gui->bufferSeconds,
		GTSL_LevelFormat, (ULONG)"%ld sec");
	gui->gadBuffer = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, BUTTON_KIND, GID_PLAY, 76, 142, 78, 20,
		(UBYTE *)"Play", PLACETEXT_IN,
		TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0);
	gui->gadPlay = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, BUTTON_KIND, GID_STOP, 286, 142, 78, 20,
		(UBYTE *)"Stop", PLACETEXT_IN,
		TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0, TAG_IGNORE, 0);
	gui->gadStop = gad;
	if (!gad) return -1;

	gad = AddGadget(gui, gad, TEXT_KIND, GID_STATUS, 58, 174, 340, 14,
		(UBYTE *)"Status:", PLACETEXT_LEFT,
		GTTX_Text, (ULONG)gui->statusText,
		GTTX_Border, TRUE,
		TAG_IGNORE, 0, TAG_IGNORE, 0);
	gui->gadStatus = gad;
	if (!gad) return -1;

	return 0;
}

static int GuiOpen(HelixAmp3Gui *gui)
{
	struct NewWindow nw;

	memset(gui, 0, sizeof(*gui));
	gui->fastLowrate = 1;
	gui->fastMem = 1;
	gui->mono = 1;
	gui->rateIndex = 2;
	gui->bufferSeconds = 10;
	gui->qualityIndex = 0;
	SafeCopy(gui->statusText, sizeof(gui->statusText), "Ready.");

	GadToolsBase = OpenLibrary("gadtools.library", 37);
	if (!GadToolsBase) {
		fprintf(stderr, "MiniAMP3 requires gadtools.library version 37 or newer.\n");
		return -1;
	}

	if (GuiCreateGadgets(gui) != 0) {
		fprintf(stderr, "cannot create MiniAMP3 GadTools gadgets\n");
		GuiClose(gui);
		return -1;
	}

	memset(&nw, 0, sizeof(nw));
	nw.LeftEdge = 40;
	nw.TopEdge = 30;
	nw.Width = 420;
	nw.Height = 200;
	nw.DetailPen = 0;
	nw.BlockPen = 1;
	nw.IDCMPFlags = IDCMP_GADGETUP | IDCMP_MOUSEMOVE | IDCMP_CLOSEWINDOW |
		IDCMP_REFRESHWINDOW | IDCMP_ACTIVEWINDOW;
	nw.Flags = WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET |
		WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM | WFLG_ACTIVATE |
		WFLG_SIMPLE_REFRESH;
	nw.FirstGadget = gui->gadgets;
	nw.Title = (UBYTE *)"MiniAMP3";
	nw.MinWidth = 420;
	nw.MinHeight = 200;
	nw.MaxWidth = 640;
	nw.MaxHeight = 260;
	nw.Type = WBENCHSCREEN;
	gui->win = OpenWindow(&nw);
	if (!gui->win) {
		fprintf(stderr, "cannot open MiniAMP3 window\n");
		GuiClose(gui);
		return -1;
	}
	return 0;
}

static void GuiClose(HelixAmp3Gui *gui)
{
	if (gui->win) {
		CloseWindow(gui->win);
		gui->win = NULL;
	}
	if (gui->gadgets) {
		FreeGadgets(gui->gadgets);
		gui->gadgets = NULL;
		gui->gadContext = NULL;
	}
	if (GadToolsBase) {
		CloseLibrary(GadToolsBase);
		GadToolsBase = NULL;
	}
}

static void ChooseMp3(HelixAmp3Gui *gui)
{
	struct FileRequester *req;
	char path[HELIXAMP3_MAX_PATH];

	req = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
		ASLFR_TitleText, (ULONG)"Select MP3 for MiniAMP3",
		ASLFR_DoPatterns, TRUE,
		ASLFR_InitialPattern, (ULONG)"#?.mp3",
		TAG_DONE);
	if (!req) {
		SetStatus(gui, "Cannot allocate ASL file requester.");
		return;
	}
	if (AslRequest(req, NULL)) {
		path[0] = '\0';
		if (req->fr_Drawer && req->fr_Drawer[0]) {
			SafeCopy(path, sizeof(path), req->fr_Drawer);
			AddPart(path, req->fr_File, sizeof(path));
		} else {
			SafeCopy(path, sizeof(path), req->fr_File);
		}
		SafeCopy(gui->inputName, sizeof(gui->inputName), path);
		UpdateFileDisplay(gui);
		SetStatus(gui, "File selected. Ready to play.");
	}
	FreeAslRequest(req);
}

static void AddArg(HelixAmp3Player *player, const char *text)
{
	if (player->argc >= HELIXAMP3_ARGC_MAX)
		return;
	SafeCopy(player->argvStorage[player->argc], HELIXAMP3_MAX_PATH, text);
	player->argv[player->argc] = player->argvStorage[player->argc];
	player->argc++;
}

static void BuildPlaybackArgs(HelixAmp3Gui *gui, HelixAmp3Player *player)
{
	char num[16];

	memset(player, 0, sizeof(*player));
	AddArg(player, "amiga_mp3dec");
	AddArg(player, "--play");
	if (gui->fastMem || gui->qualityIndex == 0 || gui->qualityIndex == 1)
		AddArg(player, "--fast-mem");
	if (gui->fastLowrate)
		AddArg(player, "--fast-lowrate");
	if (gui->mono)
		AddArg(player, "--mono");
	AddArg(player, "--rate");
	AddArg(player, kRates[gui->rateIndex]);
	AddArg(player, "--buffer-seconds");
	sprintf(num, "%d", gui->bufferSeconds);
	AddArg(player, num);
	if (gui->qualityIndex == 0)
		AddArg(player, "--play-fast-path");
	AddArg(player, gui->inputName);
	player->argv[player->argc] = NULL;
}

static void PlaybackEntry(void)
{
	gGuiPlayer.running = 1;
	gGuiPlayer.stopRequested = 0;
	gPlaybackInterrupted = 0;
	HelixAmp3CliMain(gGuiPlayer.argc, gGuiPlayer.argv);
	gGuiPlayer.running = 0;
	Forbid();
	gGuiPlayer.process = NULL;
	Permit();
}

static void StartPlayback(HelixAmp3Gui *gui)
{
	if (!gui->inputName[0]) {
		SetStatus(gui, "Browse to an MP3 first.");
		return;
	}
	if (gGuiPlayer.running) {
		SetStatus(gui, "Already playing; press Stop first.");
		return;
	}
	BuildPlaybackArgs(gui, &gGuiPlayer);
	gGuiPlayer.process = CreateNewProcTags(NP_Entry, (ULONG)PlaybackEntry,
		NP_Name, (ULONG)"MiniAMP3 playback",
		NP_StackSize, 32768,
		TAG_DONE);
	if (!gGuiPlayer.process) {
		SetStatus(gui, "Cannot start playback process.");
		return;
	}
	gGuiPlayer.running = 1;
	SetStatus(gui, "Playing through amiga_mp3dec Paula path.");
}

static void StopPlayback(HelixAmp3Gui *gui)
{
	if (!gGuiPlayer.running) {
		SetStatus(gui, "Nothing is playing.");
		return;
	}
	gGuiPlayer.stopRequested = 1;
	gPlaybackInterrupted = 1;
	SetStatus(gui, "Stop requested; waiting for playback loop to exit.");
}

static void HandleGuiAction(HelixAmp3Gui *gui, UWORD action, UWORD code)
{
	switch (action) {
	case GID_BROWSE:
		ChooseMp3(gui);
		break;
	case GID_FAST_LOWRATE:
		gui->fastLowrate = code ? 1 : 0;
		SetStatus(gui, gui->fastLowrate ? "Fast-lowrate enabled." : "Fast-lowrate disabled.");
		break;
	case GID_FAST_MEM:
		gui->fastMem = code ? 1 : 0;
		SetStatus(gui, gui->fastMem ? "Fast mem enabled." : "Fast mem disabled.");
		break;
	case GID_MONO:
		gui->mono = code ? 1 : 0;
		SetStatus(gui, gui->mono ? "Mono output enabled." : "Mono output disabled.");
		break;
	case GID_RATE:
		if (code < 4)
			gui->rateIndex = code;
		SetStatus(gui, "Output rate changed.");
		break;
	case GID_BUFFER:
		if (code < 1)
			code = 1;
		if (code > 30)
			code = 30;
		gui->bufferSeconds = code;
		UpdateBufferDisplay(gui);
		SetStatus(gui, "Buffer depth changed.");
		break;
	case GID_QUALITY:
		if (code < 3)
			gui->qualityIndex = code;
		SetStatus(gui, "Quality mode changed.");
		break;
	case GID_PLAY:
		StartPlayback(gui);
		break;
	case GID_STOP:
		StopPlayback(gui);
		break;
	}
}

static void GuiPoll(HelixAmp3Gui *gui)
{
	struct IntuiMessage *msg;
	ULONG classValue;
	UWORD code;
	struct Gadget *gad;

	while (gui->win && (msg = GT_GetIMsg(gui->win->UserPort)) != NULL) {
		classValue = msg->Class;
		code = msg->Code;
		gad = (struct Gadget *)msg->IAddress;
		GT_ReplyIMsg(msg);
		if (classValue == IDCMP_CLOSEWINDOW)
			gui->closeRequested = 1;
		else if (classValue == IDCMP_REFRESHWINDOW) {
			GT_BeginRefresh(gui->win);
			GT_EndRefresh(gui->win, TRUE);
		} else if ((classValue == IDCMP_GADGETUP || classValue == IDCMP_MOUSEMOVE) && gad) {
			HandleGuiAction(gui, gad->GadgetID, code);
		}
	}
}

int main(int argc, char **argv)
{
	HelixAmp3Gui gui;

	(void)argc;
	(void)argv;
	if (GuiOpen(&gui) != 0)
		return 1;
	while (!gui.closeRequested) {
		Wait(HELIXAMP3_SIGMASK(&gui) | SIGBREAKF_CTRL_C);
		if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
			gui.closeRequested = 1;
		GuiPoll(&gui);
	}
	if (gGuiPlayer.running)
		StopPlayback(&gui);
	GuiClose(&gui);
	return 0;
}

#else

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	fprintf(stderr, "MiniAMP3 requires an AMIGA_M68K Intuition/GadTools/ASL build.\n");
	fprintf(stderr, "Use amiga_mp3dec --play --rate 11025 --buffer-seconds 10 file.mp3 on this host.\n");
	return 1;
}

#endif
