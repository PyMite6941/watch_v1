#!/bin/bash
# Watch Studio setup for macOS - installs, changes and removes the app.
#
# Double-click this file in Finder. The first time, macOS may say it "cannot be
# opened because it is from an unidentified developer": right-click (or
# Control-click) the file, choose Open, then Open again. That is normal for
# something home-made and unsigned.
#
# It asks a few questions in ordinary Mac dialogs (Welcome -> Personal touches
# -> Options -> Ready -> Finished) and then makes
#     ~/Applications/Watch Studio.app      (or /Applications, if you choose)
# a normal Mac app of about 0.1 MB that you can find with Spotlight
# (Cmd-Space, "watch"), Launchpad or the Dock. Everything it needs is inside
# that one .app, so removing it is: drag it to the Bin. (Run this setup again
# and choose Remove to also take it out of the Dock and login items.)
#
# The window is drawn by Google Chrome or Microsoft Edge in "app mode" - its
# own window, no address bar - because Safari cannot talk to USB or Bluetooth
# devices. Nothing is downloaded.
#
# Command line (for testing):  "Setup Watch Studio.command" --silent [--dir DIR] [--for NAME]
set -u
APP_NAME="Watch Studio"
APP_VERSION="1.1"
BUNDLE_ID="org.watchv1.studio"
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/files/app"
ICON_PNG="$HERE/files/app/icon.png"

tell_user() { osascript -e "on run argv" -e "display dialog (item 1 of argv) with title \"$APP_NAME Setup\" buttons {\"OK\"} default button 1 with icon note" -e "end run" "$1" >/dev/null 2>&1; }

# Asks with buttons; prints the button pressed, or nothing if cancelled.
ask() { # text, buttons as "A","B","C", default
  osascript -e "on run argv" -e "try" \
    -e "set r to display dialog (item 1 of argv) with title \"$APP_NAME Setup\" buttons {$2} default button \"$3\" with icon note" \
    -e "return button returned of r" -e "on error" -e "return \"\"" -e "end try" -e "end run" "$1" 2>/dev/null
}

# Asks for text; prints it, or the word CANCEL.
ask_text() { # prompt, default
  osascript -e "on run argv" -e "try" \
    -e "set r to display dialog (item 1 of argv) default answer (item 2 of argv) with title \"$APP_NAME Setup\" buttons {\"Back\", \"Next\"} default button \"Next\"" \
    -e "if button returned of r is \"Back\" then return \"BACK\"" \
    -e "return text returned of r" -e "on error" -e "return \"CANCEL\"" -e "end try" -e "end run" "$1" "$2" 2>/dev/null
}

# Choose from a list; prints the chosen items one per line, or CANCEL.
ask_list() { # prompt, multiple(true/false), default items comma separated, items...
  local prompt="$1" multi="$2" defaults="$3"; shift 3
  osascript - "$prompt" "$multi" "$defaults" "$@" <<'OSA' 2>/dev/null
on run argv
  set theItems to items 4 thru -1 of argv
  set defs to {}
  set AppleScript's text item delimiters to ","
  if item 3 of argv is not "" then set defs to text items of (item 3 of argv)
  set AppleScript's text item delimiters to linefeed
  set r to choose from list theItems with title "Watch Studio Setup" with prompt (item 1 of argv) default items defs multiple selections allowed ((item 2 of argv) is "true") empty selection allowed true
  if r is false then return "CANCEL"
  return r as text
end run
OSA
}

find_engine() {
  for b in "Google Chrome" "Microsoft Edge" "Chromium" "Brave Browser"; do
    for root in /Applications "$HOME/Applications"; do
      [ -x "$root/$b.app/Contents/MacOS/$b" ] && { echo "$b"; return; }
    done
  done
}

existing() {
  for d in "$HOME/Applications" /Applications; do
    [ -f "$d/$APP_NAME.app/Contents/Resources/install.json" ] && { echo "$d/$APP_NAME.app"; return; }
  done
}

json_str() { # escapes a value for JSON
  printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/	/\\t/g' | awk 'BEGIN{ORS=""} NR>1{print "\\n"} {print}'
}

dock_remove() {
  # Takes our tile out of the Dock. Goes through "defaults" (not the plist file directly) so the change sticks.
  local pb=/usr/libexec/PlistBuddy tmp i=0 url found=0
  [ -x "$pb" ] || return 0
  tmp="$(mktemp -t watchstudio).plist"
  defaults export com.apple.dock "$tmp" 2>/dev/null || { rm -f "$tmp"; return 0; }
  while "$pb" -c "Print :persistent-apps:$i" "$tmp" >/dev/null 2>&1; do
    url="$("$pb" -c "Print :persistent-apps:$i:tile-data:file-data:_CFURLString" "$tmp" 2>/dev/null)"
    case "$url" in
      *"Watch%20Studio.app"*|*"Watch Studio.app"*) "$pb" -c "Delete :persistent-apps:$i" "$tmp"; found=1;;
      *) i=$((i + 1));;
    esac
  done
  [ $found = 1 ] && defaults import com.apple.dock "$tmp" && killall Dock 2>/dev/null
  rm -f "$tmp"
  return 0
}

uninstall() {
  local app="$1"
  [ -n "$app" ] && [ -f "$app/Contents/Resources/install.json" ] || { tell_user "Watch Studio is not installed."; return 1; }
  osascript -e "tell application \"System Events\" to delete (every login item whose name is \"$APP_NAME\")" >/dev/null 2>&1
  # Move to the Bin rather than delete outright, so it can be put back.
  osascript -e "on run argv" -e "tell application \"Finder\" to delete (POSIX file (item 1 of argv) as alias)" -e "end run" "$app" >/dev/null 2>&1 || rm -rf "$app"
  dock_remove "$app"
  return 0
}

make_app() { # dest_dir name message from accent theme
  local dest="$1/$APP_NAME.app"
  local tmp="$1/.$APP_NAME.app.new"
  rm -rf "$tmp"
  mkdir -p "$tmp/Contents/MacOS" "$tmp/Contents/Resources" || return 1
  cp "$SRC"/index.html "$SRC"/icon.svg "$SRC"/icon.png "$tmp/Contents/Resources/" 2>/dev/null
  [ -f "$tmp/Contents/Resources/index.html" ] || { tell_user "The app files are missing. Unzip the whole download, then run setup again."; return 1; }

  cat > "$tmp/Contents/Resources/config.js" <<EOF
// Written by Watch Studio setup. Run setup again to change it.
window.STUDIO_CONFIG = {"forName": "$(json_str "$2")", "message": "$(json_str "$3")", "from": "$(json_str "$4")", "accent": "$5", "theme": "$6"};
EOF
  cat > "$tmp/Contents/Resources/install.json" <<EOF
{"version": "$APP_VERSION", "forName": "$(json_str "$2")", "message": "$(json_str "$3")", "from": "$(json_str "$4")", "accent": "$5", "theme": "$6"}
EOF

  # The app itself: a few lines that open the page in Chrome/Edge app mode.
  cat > "$tmp/Contents/MacOS/watch-studio" <<'EOF'
#!/bin/bash
RES="$(cd "$(dirname "$0")/../Resources" && pwd)"
URL="file://$(printf '%s' "$RES/index.html" | sed -e 's/%/%25/g' -e 's/ /%20/g' -e 's/#/%23/g' -e 's/?/%3F/g')"
for b in "Google Chrome" "Microsoft Edge" "Chromium" "Brave Browser"; do
  for root in /Applications "$HOME/Applications"; do
    exe="$root/$b.app/Contents/MacOS/$b"
    if [ -x "$exe" ]; then
      "$exe" --app="$URL" --window-size=1000,880 >/dev/null 2>&1 &
      disown
      exit 0
    fi
  done
done
r=$(osascript -e 'display dialog "Watch Studio needs Google Chrome or Microsoft Edge to show its window (Safari cannot talk to USB or Bluetooth devices). Install either one, free, then open Watch Studio again." with title "Watch Studio" buttons {"Not now", "Get Chrome"} default button 2 with icon caution' -e 'button returned of result' 2>/dev/null)
[ "$r" = "Get Chrome" ] && open "https://www.google.com/chrome/"
exit 0
EOF
  chmod 755 "$tmp/Contents/MacOS/watch-studio"

  local icon_line=""
  if [ -f "$ICON_PNG" ] && command -v iconutil >/dev/null && command -v sips >/dev/null; then
    local set="$tmp/icon.iconset"
    mkdir -p "$set"
    for s in 16 32 128 256 512; do
      sips -z $s $s "$ICON_PNG" --out "$set/icon_${s}x${s}.png" >/dev/null 2>&1
      sips -z $((s*2)) $((s*2)) "$ICON_PNG" --out "$set/icon_${s}x${s}@2x.png" >/dev/null 2>&1
    done
    iconutil -c icns "$set" -o "$tmp/Contents/Resources/watch.icns" 2>/dev/null && icon_line="<key>CFBundleIconFile</key><string>watch</string>"
    rm -rf "$set"
  fi

  cat > "$tmp/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>$APP_NAME</string>
  <key>CFBundleDisplayName</key><string>$APP_NAME</string>
  <key>CFBundleIdentifier</key><string>$BUNDLE_ID</string>
  <key>CFBundleVersion</key><string>$APP_VERSION</string>
  <key>CFBundleShortVersionString</key><string>$APP_VERSION</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleExecutable</key><string>watch-studio</string>
  $icon_line
  <key>LSUIElement</key><true/>
  <key>LSMinimumSystemVersion</key><string>10.13</string>
</dict></plist>
EOF
  rm -rf "$dest" && mv "$tmp" "$dest" || return 1
  # Tell Launchpad/Spotlight about it straight away.
  /System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f "$dest" >/dev/null 2>&1
  echo "$dest"
}

dock_add() {
  local app="$1"
  defaults read com.apple.dock persistent-apps 2>/dev/null | grep -q "$APP_NAME.app" && return 0
  defaults write com.apple.dock persistent-apps -array-add "<dict><key>tile-data</key><dict><key>file-data</key><dict><key>_CFURLString</key><string>$app</string><key>_CFURLStringType</key><integer>0</integer></dict></dict></dict>" && killall Dock 2>/dev/null
}

login_add() {
  osascript -e "on run argv" -e "tell application \"System Events\" to make login item at end with properties {path:(item 1 of argv), hidden:false, name:\"$APP_NAME\"}" -e "end run" "$1" >/dev/null 2>&1
}

# ------------------------------------------------------------- silent mode
if [ "${1:-}" = "--silent" ]; then
  shift
  DIR="$HOME/Applications"; NAME=""
  while [ $# -gt 0 ]; do
    case "$1" in --dir) DIR="$2"; shift 2;; --for) NAME="$2"; shift 2;; *) shift;; esac
  done
  mkdir -p "$DIR" && make_app "$DIR" "$NAME" "" "" green auto
  exit $?
fi

# ------------------------------------------------------------------ wizard
PREV="$(existing)"
ENGINE="$(find_engine)"
FOR_NAME=""; MESSAGE=""; FROM=""; ACCENT="green"; THEME="auto"
if [ -n "$PREV" ]; then
  j="$PREV/Contents/Resources/install.json"
  # plutil reads JSON on macOS 12 and later; older Macs simply start with empty boxes.
  get() { plutil -extract "$1" raw -o - "$j" 2>/dev/null; }
  FOR_NAME="$(get forName)"; MESSAGE="$(get message)"; FROM="$(get from)"
  ACCENT="$(get accent)"; THEME="$(get theme)"
fi

step=0
while :; do
  case $step in
  0)
    if [ -n "$PREV" ]; then
      b=$(ask "Watch Studio is already installed:
$PREV

Continue to change its personal touches and options, or Remove it from this Mac." '"Cancel", "Remove", "Continue"' "Continue")
      case "$b" in
        Remove) uninstall "$PREV" && tell_user "Watch Studio has been moved to the Bin."; exit 0;;
        Continue) step=1;;
        *) exit 0;;
      esac
    else
      b=$(ask "This will install Watch Studio, the companion app for the watch: set it up, choose its screens, make your own apps for it, record its sensors and back it up.

It is a normal Mac app of about 0.1 MB. Find it with Spotlight (Cmd-Space, type watch), Launchpad or the Dock. To remove it, drag it to the Bin." '"Cancel", "Continue"' "Continue")
      [ "$b" = "Continue" ] && step=1 || exit 0
    fi
    if [ -z "$ENGINE" ]; then
      b=$(ask "Watch Studio shows its window with Google Chrome or Microsoft Edge (Safari cannot talk to USB or Bluetooth devices), and neither is on this Mac yet.

You can carry on and install either one afterwards." '"Cancel", "Get Chrome", "Carry on"' "Carry on")
      case "$b" in "Get Chrome") open "https://www.google.com/chrome/";; "Carry on") ;; *) exit 0;; esac
    fi
    ;;
  1)
    v=$(ask_text "Personal touches (step 1 of 3). Leave any of them empty.

Who is it for?" "$FOR_NAME")
    case "$v" in CANCEL) exit 0;; BACK) step=0; PREV="$(existing)";; *) FOR_NAME="$v"; step=2;; esac ;;
  2)
    v=$(ask_text "A short message for them (shown when they first open the app):" "$MESSAGE")
    case "$v" in CANCEL) exit 0;; BACK) step=1;; *) MESSAGE="$v"; step=3;; esac ;;
  3)
    v=$(ask_text "From:" "$FROM")
    case "$v" in CANCEL) exit 0;; BACK) step=2;; *) FROM="$v"; step=4;; esac ;;
  4)
    cap="$(printf '%s' "${ACCENT:-green}" | awk '{print toupper(substr($0,1,1)) substr($0,2)}')"
    v=$(ask_list "Colour (step 2 of 3):" false "$cap" Green Blue Purple Orange Red Teal)
    [ "$v" = "CANCEL" ] && { step=3; continue; }
    ACCENT="$(printf '%s' "${v:-Green}" | tr 'A-Z' 'a-z')"
    case "$THEME" in light) d="Light";; dark) d="Dark";; *) d="Follow the Mac";; esac
    v=$(ask_list "Appearance:" false "$d" "Follow the Mac" "Light" "Dark")
    [ "$v" = "CANCEL" ] && continue
    case "$v" in Light) THEME=light;; Dark) THEME=dark;; *) THEME=auto;; esac
    step=5 ;;
  5)
    WHERE=$(ask_list "Options (step 3 of 3). Install for:" false "Just me (~/Applications)" "Just me (~/Applications)" "Everyone on this Mac (/Applications)")
    [ "$WHERE" = "CANCEL" ] && { step=4; continue; }
    OPTS=$(ask_list "Also (Cmd-click to choose several):" true "Open it now" "Keep it in the Dock" "Open it when I log in" "Open it now")
    [ "$OPTS" = "CANCEL" ] && continue
    case "$WHERE" in Everyone*) DEST=/Applications;; *) DEST="$HOME/Applications";; esac
    step=6 ;;
  6)
    b=$(ask "Ready to install.

Where:  $DEST/$APP_NAME.app
For:  ${FOR_NAME:--}
From:  ${FROM:--}
Colour:  $ACCENT,  appearance: $THEME
Also:  $(printf '%s' "${OPTS:-nothing else}" | tr '\n' ',' | sed 's/,$//; s/,/, /g')
Space needed: about 0.1 MB" '"Back", "Install"' "Install")
    [ "$b" = "Install" ] && break
    [ "$b" = "Back" ] && { step=5; continue; }
    exit 0 ;;
  esac
done

mkdir -p "$DEST" 2>/dev/null
APP="$(make_app "$DEST" "$FOR_NAME" "$MESSAGE" "$FROM" "$ACCENT" "$THEME")" || { tell_user "Setup could not write to $DEST. Try installing just for you."; exit 1; }
# A copy in the other folder from a previous install would be confusing: remove it.
for d in "$HOME/Applications" /Applications; do
  [ "$d/$APP_NAME.app" != "$APP" ] && [ -f "$d/$APP_NAME.app/Contents/Resources/install.json" ] && rm -rf "$d/$APP_NAME.app"
done
case "$OPTS" in *Dock*) dock_add "$APP";; esac
case "$OPTS" in *"log in"*) login_add "$APP";; *) osascript -e "tell application \"System Events\" to delete (every login item whose name is \"$APP_NAME\")" >/dev/null 2>&1;; esac

tell_user "Watch Studio is installed.

Open it with Spotlight (Cmd-Space, type watch), from Launchpad$(case "$OPTS" in *Dock*) echo ' or the Dock';; esac).
To keep it in the Dock later: open it, then drag it there from Applications.
To remove it: drag $APP_NAME from Applications to the Bin."
case "$OPTS" in *"now"*) open "$APP";; esac
exit 0
