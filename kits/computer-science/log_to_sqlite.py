"""Copy the watch's sensor log into a SQLite database - a real data set for SQL lessons.

    pip install pyserial
    python log_to_sqlite.py                  # -> watch_log.db, table "samples"
    python log_to_sqlite.py --demo           # no watch? makes three days of made-up samples

The log needs an environment pack (BME280 / BMP280) and "Log a sample every ...
minutes" switched on in Watch Studio. Running it again adds only new rows: the
watch's time stamp is the primary key.

Then, for example:
    sqlite3 watch_log.db
    SELECT date(t, 'unixepoch') AS day, round(avg(temp_c), 1), max(temp_c) FROM samples GROUP BY day;
    SELECT * FROM samples WHERE humidity > 70 ORDER BY t DESC LIMIT 10;
"""
import math
import sqlite3
import sys
import time

SCHEMA = """
CREATE TABLE IF NOT EXISTS samples (
    t         INTEGER PRIMARY KEY,   -- Unix time (seconds, UTC) the watch took the sample
    temp_c    REAL,                  -- NULL when there was no reading
    humidity  REAL,                  -- % relative humidity; NULL with a BMP280 (no humidity)
    hpa       REAL,                  -- air pressure, hectopascals
    light_mv  INTEGER                -- light sensor, millivolts: higher = brighter
);
"""


def rows_from_watch():
    from watch import Watch           # same folder
    with Watch() as w:
        rows, total = [], None
        while total is None or len(rows) < total:
            reply = w.send({"cmd": "log_get", "from": len(rows)})
            total = reply["total"]
            if not reply["rows"]:
                break
            rows += reply["rows"]
        return rows


def rows_demo():
    now = int(time.time()) // 1800 * 1800
    out = []
    for k in range(144):              # three days, every 30 minutes
        t = now - (143 - k) * 1800
        day = math.sin((t % 86400) / 86400 * 2 * math.pi - math.pi / 2)
        out.append([t, round(24 + 4 * day, 1), round(60 - 12 * day, 1), round(1009 + 3 * math.sin(k / 20), 1),
                    int(max(0, 1500 * day))])
    return out


def main():
    rows = rows_demo() if "--demo" in sys.argv else rows_from_watch()
    db = sqlite3.connect("watch_log.db")
    db.executescript(SCHEMA)
    before = db.execute("SELECT count(*) FROM samples").fetchone()[0]
    db.executemany("INSERT OR IGNORE INTO samples VALUES (?, ?, ?, ?, ?)", [tuple(r) for r in rows])
    db.commit()
    after = db.execute("SELECT count(*) FROM samples").fetchone()[0]
    print(f"{len(rows)} rows read, {after - before} new; watch_log.db now holds {after}")


if __name__ == "__main__":
    main()
