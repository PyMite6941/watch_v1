"""Talk to a Watch V1 from Python over USB - a starting point for student projects.

    pip install pyserial
    python watch.py                          # finds the watch, prints its info
    python watch.py COM5 '{"cmd":"buzz","ms":300}'
    python watch.py --list                   # which serial ports exist

Use it from your own code:

    from watch import Watch
    with Watch() as w:
        print(w.send({"cmd": "info"}))
        w.send({"cmd": "show", "text": "HI", "ms": 2000})
        for reading in w.stream(500):        # live sensor data, twice a second
            print(reading.get("hpa"))        # Ctrl+C to stop

The protocol is one JSON object per line, both ways (docs/CONFIGURATION.md >
The command protocol). Every command gets exactly one reply; lines with an
"ev" field are events, and lines not starting with "{" are debug chatter.
"""
import itertools
import json
import sys
import time

import serial                      # pip install pyserial
from serial.tools import list_ports

ESPRESSIF_USB = 0x303A             # the ESP32-C3's built-in USB


class WatchError(Exception):
    pass


class Watch:
    def __init__(self, port=None, timeout=5.0):
        port = port or self.find()
        self.ser = serial.Serial(port, 115200, timeout=0.2)
        self.timeout = timeout
        self.events = []           # {"ev": ...} lines that arrived while waiting for a reply
        self._ids = itertools.count(1)
        time.sleep(0.3)            # the port can drop the first bytes right after opening
        self.ser.reset_input_buffer()

    @staticmethod
    def find():
        ports = [p for p in list_ports.comports() if p.vid == ESPRESSIF_USB]
        if not ports:
            raise WatchError("no watch found: plug it in by its right-hand USB socket (the left one only charges)")
        return ports[0].device

    def send(self, command):
        """Send one command (a dict) and return the reply (a dict). Raises WatchError on {"ok": false}."""
        msg_id = next(self._ids)
        self.ser.write((json.dumps(dict(command, id=msg_id)) + "\n").encode())
        deadline = time.time() + self.timeout
        while time.time() < deadline:
            line = self.ser.readline().decode(errors="replace").strip()
            if not line.startswith("{"):
                continue           # debug chatter, or nothing yet
            try:
                msg = json.loads(line)
            except json.JSONDecodeError:
                continue
            if "ev" in msg:
                self.events.append(msg)
            elif msg.get("id") == msg_id:
                if not msg.get("ok"):
                    raise WatchError(msg.get("err", "the watch said no"))
                return msg
        raise WatchError(f"no reply to {command.get('cmd')} within {self.timeout} s")

    def stream(self, ms=500):
        """Yield live {"ev":"data",...} readings every `ms` until the loop is broken; then stop the stream."""
        self.send({"cmd": "stream", "ms": ms})
        try:
            while True:
                line = self.ser.readline().decode(errors="replace").strip()
                if line.startswith("{"):
                    try:
                        msg = json.loads(line)
                    except json.JSONDecodeError:
                        continue
                    if msg.get("ev") == "data":
                        yield msg
                    elif "ev" in msg:
                        self.events.append(msg)
        finally:
            self.send({"cmd": "stream", "ms": 0})

    def close(self):
        self.ser.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


def main(argv):
    if "--list" in argv:
        for p in list_ports.comports():
            print(p.device, p.description, "<- watch" if p.vid == ESPRESSIF_USB else "")
        return 0
    port = argv[0] if argv and not argv[0].startswith("{") else None
    cmd = json.loads(argv[-1]) if argv and argv[-1].startswith("{") else {"cmd": "info"}
    with Watch(port) as w:
        print(json.dumps(w.send(cmd), indent=2))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except WatchError as e:
        print(e, file=sys.stderr)
        sys.exit(1)
