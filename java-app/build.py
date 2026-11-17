"""Build dist/WatchStudio.jar: Watch Studio (app/) as a Java app.

    python java-app/build.py

Needs a JDK 11 or newer (javac and jar on the PATH, or JAVA_HOME set). The jar
runs on any Java 11+ and has no dependencies: it packs app/ inside itself.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
APP = os.path.join(ROOT, "app")
DIST = os.path.join(ROOT, "dist")
APP_FILES = ["index.html", "icon.svg", "icon.png", "manifest.webmanifest", "sw.js", "oled.js", "oled-assets.js"]  # as installer/build.py


def tool(name):
    home = os.environ.get("JAVA_HOME")
    if home:
        for ext in ("", ".exe"):
            p = os.path.join(home, "bin", name + ext)
            if os.path.isfile(p):
                return p
    p = shutil.which(name)
    if not p:
        sys.exit(f"{name} not found: install a JDK (11 or newer) or set JAVA_HOME")
    return p


def main():
    os.makedirs(DIST, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        classes = os.path.join(tmp, "classes")
        os.makedirs(os.path.join(classes, "app"))
        # javac cannot read paths outside the system code page (e.g. a Japanese folder name),
        # so compile a copy in the temporary folder
        src = os.path.join(tmp, "WatchStudio.java")
        shutil.copy(os.path.join(HERE, "src", "watchstudio", "WatchStudio.java"), src)
        subprocess.run([tool("javac"), "--release", "11", "-encoding", "UTF-8", "-d", "classes", "WatchStudio.java"],
                       check=True, cwd=tmp)
        for f in APP_FILES:
            path = os.path.join(APP, f)
            if os.path.exists(path):
                shutil.copy(path, os.path.join(classes, "app", f))
        if not os.path.exists(os.path.join(classes, "app", "index.html")):
            sys.exit("app/index.html is missing")
        subprocess.run([tool("jar"), "--create", "--file", "WatchStudio.jar", "--main-class", "watchstudio.WatchStudio",
                        "-C", "classes", "."], check=True, cwd=tmp)
        jar = os.path.join(DIST, "WatchStudio.jar")
        shutil.copy(os.path.join(tmp, "WatchStudio.jar"), jar)
    print(f"wrote {os.path.relpath(jar, ROOT)} ({os.path.getsize(jar) // 1024} kB) - run: java -jar dist/WatchStudio.jar")


if __name__ == "__main__":
    main()
