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
'''\tplayX = gui->gadPlay->LeftEdge + (gui->gadPlay->Width / 2) - 5;
\tplayY = gui->gadPlay->TopEdge + (gui->gadPlay->Height / 2) - 5;
\tfor (i = 0; i < 10; i++) {
\t\tint half = i / 2;
\t\tRectFill(rp, playX + i, playY + 5 - half, playX + i,
\t\t\tplayY + 5 + half);
\t}
\tstopX = gui->gadStop->LeftEdge + (gui->gadStop->Width / 2) - 6;
\tstopY = gui->gadStop->TopEdge + (gui->gadStop->Height / 2) - 5;
\tRectFill(rp, stopX, stopY, stopX + 3, stopY + 9);
\tRectFill(rp, stopX + 8, stopY, stopX + 11, stopY + 9);
''',
'''\tplayX = gui->gadPlay->LeftEdge + (gui->gadPlay->Width / 2) - 5;
\tplayY = gui->gadPlay->TopEdge + (gui->gadPlay->Height / 2) - 5;
\tfor (i = 0; i < 10; i++) {
\t\tint half = (9 - i) / 2;
\t\tRectFill(rp, playX + i, playY + 5 - half, playX + i,
\t\t\tplayY + 5 + half);
\t}
\tstopX = gui->gadStop->LeftEdge + (gui->gadStop->Width / 2) - 5;
\tstopY = gui->gadStop->TopEdge + (gui->gadStop->Height / 2) - 5;
\tRectFill(rp, stopX, stopY, stopX + 9, stopY + 9);
''',
'int half = (9 - i) / 2;',
'transport icon draw block not found')

rep(
'''\t\t} else if (classValue == IDCMP_GADGETUP) {
\t\t\tHandleGuiAction(gui, gad, code, classValue, TRUE);
\t\t} else if (classValue == IDCMP_MOUSEMOVE) {
''',
'''\t\t} else if (classValue == IDCMP_GADGETUP) {
\t\t\tHandleGuiAction(gui, gad, code, classValue, TRUE);
\t\t\t/* GadTools redraws the button face after a press, so repaint our
\t\t\t * hand-drawn transport icons once the gadget has popped back up. */
\t\t\tDrawTransportIcons(gui);
\t\t} else if (classValue == IDCMP_MOUSEMOVE) {
''',
'GadTools redraws the button face after a press',
'GADGETUP transport redraw block not found')

p.write_text(s)
