"""Build (line, stop_code) -> name from the STIB GTFS and compare with the
2009 zoobab table compiled into flipper-mobib.

Usage: python gtfs_stops.py <gtfs_dir> <calypso_bus_table.inc> [out.inc]
"""
import csv, re, sys, collections

gtfs, old_inc = sys.argv[1], sys.argv[2]
out_inc = sys.argv[3] if len(sys.argv) > 3 else None

def rows(name):
    with open(f"{gtfs}/{name}", encoding="utf-8-sig", newline="") as f:
        yield from csv.DictReader(f)

routes = {r["route_id"]: (r["route_short_name"], int(r["route_type"])) for r in rows("routes.txt")}
trip_route = {t["trip_id"]: t["route_id"] for t in rows("trips.txt")}
stops = {s["stop_id"]: s["stop_name"] for s in rows("stops.txt")}

# French names: translations.txt keyed by the original (upper-case) stop_name.
fr = {}
for t in rows("translations.txt"):
    if t["table_name"] == "stops" and t["field_name"] == "stop_name" and t["language"] == "fr":
        fr[t["field_value"]] = t["translation"]

pairs = set()
for st in rows("stop_times.txt"):
    rid = trip_route.get(st["trip_id"])
    if rid is not None:
        pairs.add((rid, st["stop_id"]))

def code_of(stop_id):
    m = re.match(r"(\d+)", stop_id)
    return int(m.group(1)) if m else None

def ascii_only(name):
    # Flipper fonts are ASCII-only: drop accents.
    import unicodedata
    return unicodedata.normalize("NFKD", name).encode("ascii", "ignore").decode()

def nice(name):
    # Fallback when no French translation: Title-case the upper-case name.
    words = re.split(r"([ \-'/().])", name.lower())
    return "".join(w[:1].upper() + w[1:] for w in words)

new = {}
modes = collections.Counter()
for rid, sid in pairs:
    line, rtype = routes[rid]
    if not line.isdigit():
        continue
    code = code_of(sid)
    if code is None:
        continue
    raw = stops.get(sid, "")
    name = fr[raw] if raw in fr else nice(raw)
    new[(int(line), code)] = ascii_only(name)[:24]
    modes[rtype] += 1

old = {}
for l in open(old_inc, encoding="utf-8"):
    m = re.search(r'\{\s*(\d+),\s*(\d+),\s*"(.*)"\}', l)
    if m:
        old[(int(m.group(1)), int(m.group(2)))] = m.group(3)

def norm(s):
    return re.sub(r"[^a-z]", "", s.lower())

common = set(old) & set(new)
same = sum(1 for k in common if norm(old[k])[:6] == norm(new[k])[:6])
codes_old = {k[1] for k in old}
codes_new = {k[1] for k in new}
code_names_new = {}
for (l, c), n in new.items():
    code_names_new.setdefault(c, n)
code_same = sum(1 for (l, c), n in old.items() if c in code_names_new and norm(code_names_new[c])[:6] == norm(n)[:6])

print(f"new pairs: {len(new)}  lines: {len({k[0] for k in new})}  route types: {dict(modes)}")
print(f"new code range: {min(codes_new)}..{max(codes_new)}  >4095: {sum(1 for c in codes_new if c > 4095)}/{len(codes_new)}")
print(f"old pairs: {len(old)}; same (line,code) in new: {len(common)}; same name among those: {same}")
print(f"old codes found in new (any line): {len(codes_old & codes_new)}/{len(codes_old)}; same name by code: {code_same}/{len(old)}")
for k in sorted(common)[:8]:
    print("  ", k, repr(old[k]), "->", repr(new[k]))

if out_inc:
    with open(out_inc, "w", encoding="ascii") as f:
        for (l, c) in sorted(new):
            n = new[(l, c)].replace("\\", "").replace('"', "'")
            f.write(f'    {{{l:4d}, {c:5d}, "{n}"}},\n')
    print("wrote", out_inc)
