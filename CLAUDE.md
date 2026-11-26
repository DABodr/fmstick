# FmStick control app: notes for Claude

Qt6 desktop app (plus a built-in web UI) that drives USB FM/RDS transmitters
built on a Si4711 chip: the FmStick, and the Vast HiFi-FMT (USB id
`0451:2100`, firmware `V-FMT212`), which is the hardware this fork is
tested on. The user writes in French; code, comments and commits are in
English.

## Build and run

- Build out of tree with Qt6, never with the root `Makefile` (a stale Qt4
  file that fails):
  `mkdir -p build-qt6 && cd build-qt6 && /usr/lib/qt6/bin/qmake ../fmstick.pro && make -j4`
- Debug variant with AddressSanitizer, which prints the bytes sent to the
  chip (`USB: Sending device Cmd: ...`) and the app's `qDebug()`:
  `build-qt6-dbg/` with `qmake ../fmstick.pro CONFIG+=debug "QMAKE_CFLAGS+=-fsanitize=address" "QMAKE_CXXFLAGS+=-fsanitize=address" "QMAKE_LFLAGS+=-fsanitize=address"`,
  run with `ASAN_OPTIONS=detect_leaks=0`. The release build prints nothing.
- Dependencies: Qt6 widgets/network, `libusb-1.0`, ALSA (`alsa` pkg-config).
- Device access needs the udev rule
  `/etc/udev/rules.d/99-fmstick.rules` (MODE 0666 for `usb` and `hidraw`,
  idVendor 0451, idProduct 2100).
- Only one instance runs (QtSingleApplication): kill the running one
  (`pkill -x fmstick`) before launching a new build.

## Translations

- `translations/fmstick_fr.ts` (also `ru`, `uk`). Update with
  `lupdate` on the sources, **not** on the `.pro` (that hangs):
  `/usr/lib/qt6/bin/lupdate -no-obsolete -locations none *.cpp *.h controller/*.cpp mainwindow.ui -ts translations/fmstick_fr.ts`
  then fill the new `<translation>` entries and run `lrelease`.
- lupdate run on the `.pro` leaves a stray `*.json` in the repo root: delete it.
- French typography: no-break space (U+00A0) before `:` `;` `?` `!` `%`.
- Strings set in code must be re-set on `QEvent::LanguageChange` /
  `MainWindow::retranslateUi()`: the saved language is applied at startup,
  after the widgets are built, and `ui->retranslateUi()` resets every text
  of the `.ui`, field contents included. `MainWindow::retranslateUi()`
  saves and restores every line edit and combo box (signals blocked)
  around it: keep it that way, or PS/PI/login get reset at each startup.

## Architecture

- `fmtx.c`: Si4711 commands and properties over HID (`hid-libusb.c`, an old
  hidapi). Every call opens and closes the device.
- `MainWindow` (`mainwindow*.cpp`, `mainwindow.ui`): the original advanced
  tabs. Setters only reach the device when `liveEditingEnabled` is set (the
  "Live editing" option); otherwise only "Send to device" does.
- `SimpleView` (`simpleview.cpp`): the simple main screen. It never talks to
  the device: it drives the advanced tabs' widgets, wrapping changes in
  `MainWindow::applyLive()` so they apply whatever "Live editing" says.
  It resyncs from those widgets every second, because the app sometimes
  updates them with signals blocked.
- Web UI (`requestmapper.cpp`, `QtWebApp-v1.2.9/`, `html/app.*` in the Qt
  resources): `/` is a page mirroring the simple screen; `/api/state` is
  JSON prepared by `SimpleView::refresh()` in the GUI thread;
  `/api/set` (POST + `X-FmStick: 1` header) queues
  `SimpleView::webSet()` to the GUI thread. HTTP handlers run in their own
  threads: never touch widgets from them. Old pages live under `/classic`.
- `Querier`: 1 s timer that polls the device (ASQ level, RT file, RDS time).
  `HidAccessMutex` serialises device access inside the app.
- On connect, `RedrawEverything()` reads the EEPROM, then the running
  config. The chip cannot report the PS/RT it is sending, so the app shows
  the EEPROM image's, unless `onair.h` remembers what it last sent in the
  same USB plug session (boot id + USB device number): then it shows that.
- RadioText from a media player: `nowplaying.cpp` reads MPRIS over D-Bus
  (polled every 2 s by `SimpleView::pollPlayers()`). Match players by
  `MediaPlayer::id` (bus name without `.instanceNNN`), never by their
  identity, which some translate ("Lecteur multimédia VLC"). VLC owns two
  bus names per player; web radios put the track in `vlc:nowplaying`, the
  station in `xesam:title`. VLC 3 publishes no title at all for `https://`
  streams (its HTTPS access ignores ICY metadata): test with `http://`
  ones, e.g. `http://stream.radioparadise.com/mp3-128`.

## Hardware quirks (measured on the Vast HiFi-FMT, don't re-measure)

- The firmware scales the audio deviation by the USB sound card volume
  (ALSA control `PCM`, 0..31): `on air = 50 Hz + (setting - 50) * vol / 31`.
  Volume 0 = carrier and RDS only, no sound. Reading `TX_AUDIO_DEVIATION`
  returns the scaled value; writing sets the ceiling. PulseAudio sees a
  bogus dB range (-0.12..0 dB) for that control. See `usbvolume.c`.
- The firmware refuses raw `TX_ASQ_STATUS`, `TX_TUNE_STATUS`,
  `GET/SET_PROPERTY`, `POWER_UP/DOWN` (use the dedicated requests in
  `fmtx.c`). Its overmodulation flag is latched forever after the first
  event: don't use it; use the measured input level.
- `TX_TUNE_MEASURE` (received noise level) returns ERR: the Si4711 has no
  RPS (Si4712/13 only). No "find a free frequency".
- RDS text must go through `toRdsCharset()` (`rdstext.h`): receivers use the
  RDS character set, not UTF-8.
- TEF6686 receivers (PE5PVB firmware, source on GitHub) with "RT buffering"
  only show a RadioText when the A/B flag toggles again, i.e. when the next
  text starts. So each new text is sent again 6 s later with the flag
  toggled (`MainWindow::rtSent()`), and the flag of the text on air is kept
  across restarts (`onair.h`). Texts end with 0x0D: without it, they show
  all 64 characters, padding and reception errors included.
- Changing the power briefly drops the carrier: receivers may lose RDS
  until they are retuned. Testing into a 50 ohm dummy load gives a weak,
  unstable signal (~25 dBuV on a nearby receiver) and RDS errors.

## Testing on the real device

- The transmitter is usually on air and the user may be listening or using
  the app. Say before anything that changes what is broadcast, and put
  settings back afterwards.
- Don't access the device from a separate tool while the app runs: the
  app's polling fails, it thinks the device is gone and reconnects (and
  ignores clicks meanwhile). Close the app first (`pkill -x fmstick`).
- Small CLI test programs can link `fmtx.c bistream.c logging.c hid-libusb.c`
  (`-I. -I/usr/include/libusb-1.0 -lusb-1.0 -lpthread`); keep them out of
  the repo.
- GUI automation with `xdotool`: the window title is translated
  ("FmStick – Contrôle de l'émetteur" in French). Never drive the screen
  while the user is using it: clicks land in whatever window is in front.
  The first click after launch only focuses the window.
- Media players: `cvlc --aout=dummy --vout=dummy --control dbus <file>`
  plays without sound on air and exposes MPRIS. A web radio can be faked
  with a small Python server sending ICY metadata (`icy-metaint`).
- Web UI: `curl -u <login>:<password> http://127.0.0.1:8080/api/state`.
- What is on air, without asking the user: an RTL-SDR dongle (DVB-T driver
  blacklisted in `/etc/modprobe.d/blacklist-rtl-sdr.conf`) and redsea
  (built into `~/.local/bin`):
  `timeout 25 sh -c 'rtl_fm -M fm -l 0 -A std -p 0 -s 171k -g 20 -F 9 -f 107.4M | ~/.local/bin/redsea -r 171k -E' > rds.json`
  gives one JSON line per RDS group: PI, PS, RadioText, PTY, group types
  and the block error rate (`bler`). Keep the dongle away from the
  transmitter, or lower the power, or it saturates.
  For screenshots, use headless Chrome with a throwaway `--user-data-dir`
  and a local relay that adds the Basic auth header, not the user's Chrome.

## Conventions

- GPL v2. Every file changed gets a dated notice in its header, added in
  the commit that changes it, e.g.
  ` * Modified 2026-11-25 by Aurel: <what changed>.` (keep original
  authors' headers). New files get `Copyright (C) <year> Aurel` and the GPL
  v2-or-later notice. Credits also go in `AUTHORS.txt` and
  `debian/copyright`.
- Many files use CRLF line endings (`mainwindow_helpers.cpp`,
  `mainwindow_cfg.cpp`, `mainwindow.ui`, several `controller/` and
  `QtWebApp` files). Preserve them: when editing with a script, open with
  `newline=''` (Python), or diffs balloon to the whole file.
- One commit per topic, each building on its own; work on a branch.
- Pull requests go to the user's fork `DABodr/fmstick` (base `master`),
  never to upstream `piratfm/fmstick`: `gh pr create` defaults to the
  parent repo in a fork, so always pass `--repo DABodr/fmstick`.
  `gh pr edit` fails (Projects classic); use `gh api` instead.
