"""Route the watch board with Freerouting, without opening KiCad.

Nobody has to run this on their own computer: the GitHub workflow
.github/workflows/route-board.yml runs it (and gen_pcb.py and export_fab.py) in the
cloud whenever the placement changes, and opens a pull request with the routed board
and the Gerbers. Run it locally only to experiment.

    set FREEROUTING_JAR=C:\\path\\to\\freerouting-2.5.0.jar
    "%LOCALAPPDATA%\\Programs\\KiCad\\10.0\\bin\\python.exe" freeroute.py
    ... then gen_pcb.py (puts the routes on a freshly generated board) and export_fab.py

Steps:
  1. export kicad/watch_v1.kicad_pcb as Specctra DSN (the format Freerouting reads)
  2. run Freerouting headless on it -> a Specctra session file (.ses) of tracks and vias
  3. keep it as kicad/watch_v1.ses, with kicad/watch_v1.ses.json recording which
     placement it belongs to; gen_pcb.py imports it from then on

Get Freerouting from https://github.com/freerouting/freerouting/releases (the .jar;
it needs Java 25+). It is not kept in this repository.

Memory: Freerouting is a Java program and wants about 1 GB. FREEROUTING_MEM sets
the limit (default 1g); close other programs first on a busy machine.
Time: FREEROUTING_PASSES (default 40) caps the optimisation passes.
"""
import hashlib
import json
import os
import shutil
import subprocess
import sys

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
KICAD = os.path.join(HERE, "..", "kicad")
BOARD = os.path.join(KICAD, "watch_v1.kicad_pcb")
PADS = os.path.join(KICAD, "watch_v1.pads.json")
DSN = os.path.join(KICAD, "watch_v1.dsn")
SES = os.path.join(KICAD, "watch_v1.ses")


def main():
    jar = os.environ.get("FREEROUTING_JAR", "")
    if not os.path.isfile(jar):
        raise SystemExit("set FREEROUTING_JAR to the Freerouting .jar (see the top of this file)")
    java = shutil.which("java")
    if not java:
        raise SystemExit("Java not found: install Java 25 or newer (Freerouting 2.5 needs it)")

    board = pcbnew.LoadBoard(BOARD)
    if len(board.GetTracks()):
        raise SystemExit("the board already has tracks: run gen_pcb.py first for a clean, unrouted board")
    if not pcbnew.ExportSpecctraDSN(board, DSN):
        raise SystemExit("DSN export failed")
    print("exported", os.path.basename(DSN), flush=True)

    out = SES + ".new"
    if os.path.exists(out):
        os.remove(out)
    cmd = [java, "-Xmx" + os.environ.get("FREEROUTING_MEM", "1g"), "-Djava.awt.headless=true", "-jar", jar,
           "-de", DSN, "-do", out, "-mp", os.environ.get("FREEROUTING_PASSES", "40"), "-mt", "1",
           "--gui.enabled=false", "--api_server.enabled=false",
           "--usage_and_diagnostic_data.disable_analytics=true"]
    print("> freerouting", " ".join(cmd[5:9]), "...", flush=True)
    code = subprocess.run(cmd, cwd=KICAD).returncode
    if code != 0 or not os.path.exists(out):
        raise SystemExit(f"Freerouting failed (exit {code}); nothing changed")
    os.replace(out, SES)

    with open(PADS, "rb") as f:
        sha = hashlib.sha1(f.read()).hexdigest()
    with open(SES + ".json", "w", encoding="utf-8") as f:
        json.dump({"pads_sha1": sha, "router": os.path.basename(jar)}, f, indent=1)
        f.write("\n")
    os.remove(DSN)
    print("saved", os.path.basename(SES), "- now run gen_pcb.py, then export_fab.py")


if __name__ == "__main__":
    try:
        main()
        code = 0
    except SystemExit as e:
        print(e)
        code = 1
    sys.stdout.flush()
    os._exit(code)      # KiCad's Python can hang on a normal interpreter shutdown
