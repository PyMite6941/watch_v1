# Watch Studio installers

Watch Studio (the companion app, [`app/index.html`](../app/index.html)) is handed out as a small desktop app for Windows and macOS. This folder makes the installers.

```
build.py                          python installer/build.py  ->  dist/*.zip, and draws the icons
README-gift.txt                   the "READ ME" that goes in each zip, for the person receiving the watch
windows/Setup.cmd                 what they double-click
windows/setup.ps1                 the installer window, the uninstaller, and silent mode
windows/watch.ico                 drawn by build.py
macos/Setup Watch Studio.command  the Mac installer (questions in ordinary Mac dialogs)
```

## Making the gift files

```bash
python installer/build.py
```

gives `dist/WatchStudio-Windows.zip` (about 35 kB) and `dist/WatchStudio-macOS.zip` (about 30 kB). Put them on a USB stick or a shared folder with the watch, or email them. `dist/` is not committed. Run it again after changing anything in `app/`.

## What the person receiving it sees

### Windows

Unzip, double-click **Setup.cmd**. A normal installer window opens:

1. **Welcome** — what it is, and that it takes about 0.1 MB. If it is already installed: change it, or **Remove**.
2. **Personal touches** — who it is for, a message, who it is from, colour (green, blue, purple, orange, red, teal), light/dark/follow Windows. The name appears in the app's title; the message is shown when the app first opens, until closed.
3. **Where and how it opens** — install folder (default `%LOCALAPPDATA%\Programs\Watch Studio`, no administrator rights), Start menu shortcut, desktop shortcut, keyboard shortcut **Ctrl + Alt + W** (or S, J, K, Q, Z), open when signing in to Windows, and Edge or Chrome.
4. **Ready** — a summary.
5. **Finished** — how to open, pin and remove it; "Open Watch Studio now".

Removing: *Settings → Apps → Installed apps → Watch Studio → Uninstall* (or run Setup again → Remove). It deletes the shortcuts, the Settings entry and the files it installed — by name, then the folder only if it is empty — so nothing else can go with it.

Windows SmartScreen may say "Windows protected your PC" because the files are not code-signed: *More info → Run anyway*. Signing needs a paid certificate; not worth it for a handful of gifts.

### macOS

Unzip, then right-click **Setup Watch Studio.command** → *Open* → *Open* (Gatekeeper asks once for any unsigned script). Dialogs ask the same things as on Windows; then options: install for just me (`~/Applications`) or everyone (`/Applications`), keep in the Dock, open at login, open now.

The result is `Watch Studio.app`: Spotlight (Cmd-Space, "watch"), Launchpad, Dock. **Removing it is dragging it to the Bin** — everything is inside the `.app`. Running setup again and choosing *Remove* also takes it out of the Dock and login items.

## How it stays so small

The app is one HTML page (≈60 kB) plus icons. It is not a web site: nothing is fetched from the internet, there is no server, and it works offline. What draws the window is the browser engine the computer already has — Microsoft Edge on every Windows 10/11, Chrome or Edge on a Mac — started in *app mode* (`--app=file:///…/index.html`): its own window and taskbar/Dock icon, no address bar, no tabs, no browser menus. That engine is also what lets it talk to the watch over USB (Web Serial) and Bluetooth (Web Bluetooth); Safari and Firefox cannot, which is why a Mac needs Chrome or Edge.

The alternatives considered, and why not:

| Approach | Size | Problem |
|---|---|---|
| Electron | 150–250 MB | Big; bundles a whole second copy of Chrome |
| Tauri / native WebView | 5–15 MB | Needs a Rust build per platform, and a Mac to build the Mac one; the system web views (WebView2, WKWebView) cannot be relied on for Web Serial, so USB would likely need native code |
| Python + Tk | needs Python (~100 MB) | Not installed on most teachers' computers |
| Native per OS (C#, Swift) | 1–5 MB | Two separate apps to write and keep in step |
| **Engine already on the computer** | **0.1 MB** | Needs Edge or Chrome — Windows always has Edge |

## About "open from the lock screen"

Neither Windows nor macOS lets ordinary apps be started from the lock screen — only the system's own items (camera, notifications, accessibility) appear there, by design. The quickest ways in once signed in:

- **Windows**: Ctrl + Alt + W from anywhere; Start → type "watch"; pin to the taskbar (right-click its taskbar icon while open → *Pin to taskbar*; Windows does not allow installers to pin); or open at sign-in.
- **macOS**: Cmd-Space → "watch"; the Dock; open at login.

## Testing and scripting

```powershell
# Windows: install without the window (no Settings entry with -NoRegister), then remove
files\setup.ps1 -Silent -InstallDir C:\temp\ws -ForName "Ms Rivera" -Message "Thank you!" -From Matt -Accent purple -NoRegister
"C:\temp\ws\Watch Studio\setup.ps1" -Uninstall -Quiet

# Windows: pictures of each installer page instead of running it
$env:WATCHSTUDIO_SNAPSHOT = "C:\temp"; files\setup.ps1
```

```bash
# macOS
"./Setup Watch Studio.command" --silent --dir /tmp/ws --for "Ms Rivera"
```

What has been checked (2026-11-14, Windows 11): build, silent install, the installed app opening with the personal touches, the demo watch with Quick setup and every tab (Edge headless), quiet uninstall leaving no files, and pictures of all installer pages. **Not yet tried**: the interactive Windows installer clicked through by a person, the Settings → Apps entry, the shortcuts and Ctrl+Alt+W, and anything on a Mac (on the project to-do list).
