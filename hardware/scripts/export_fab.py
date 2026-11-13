"""Check the board and package it for a PCB factory.

    python export_fab.py

Runs KiCad's design-rule check first and refuses to export a board that has
unrouted connections or rule violations. Then writes
    ../fab/watch_v1_gerbers.zip    Gerbers + drill files: upload this to JLCPCB / PCBWay / ...
    ../fab/watch_v1_drc.rpt        the DRC report, for the record
"""
import os
import shutil
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
BOARD = os.path.join(HERE, "..", "kicad", "watch_v1.kicad_pcb")
FAB = os.path.join(HERE, "..", "fab")
CLI = os.environ.get("KICAD_CLI") or shutil.which("kicad-cli") or os.path.expandvars(
    r"%LOCALAPPDATA%\Programs\KiCad\10.0\bin\kicad-cli.exe")
LAYERS = "F.Cu,B.Cu,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts"
# Cosmetic findings that are expected on this board and do not affect manufacture:
# through-hole parts share the area under the display, and silkscreen is clipped at pads.
IGNORED = ("pth_inside_courtyard", "silk_over_copper", "silk_overlap", "silk_edge_clearance")


def run(*args):
    # Print file names only: a console in a legacy code page cannot show every folder name.
    print(">", "kicad-cli", *(os.path.basename(a.rstrip("\\/")) or a for a in args), flush=True)
    return subprocess.run([CLI, *args]).returncode


def main():
    os.makedirs(FAB, exist_ok=True)
    report = os.path.join(FAB, "watch_v1_drc.rpt")
    run("pcb", "drc", BOARD, "-o", report, "--severity-all")
    problems = []
    with open(report, encoding="utf-8") as f:
        for line in f:
            if line.startswith("[") and not line.startswith(tuple("[" + name for name in IGNORED)):
                problems.append(line.strip())
    if problems:
        print(f"\n{len(problems)} problem(s) - not exporting. First few:")
        for p in problems[:8]:
            print("  ", p)
        print("\n'Missing connection' means the board is not fully routed yet: see hardware/README.md.")
        return 1
    out = os.path.join(FAB, "gerbers")
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    if run("pcb", "export", "gerbers", BOARD, "-o", out + os.sep, "--layers", LAYERS, "--subtract-soldermask"):
        return 1
    if run("pcb", "export", "drill", BOARD, "-o", out + os.sep, "--format", "excellon", "--excellon-separate-th"):
        return 1
    archive = os.path.join(FAB, "watch_v1_gerbers.zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for name in sorted(os.listdir(out)):
            z.write(os.path.join(out, name), name)
    print("\nready to upload:", os.path.normpath(archive).encode("ascii", "replace").decode())
    return 0


if __name__ == "__main__":
    sys.exit(main())
