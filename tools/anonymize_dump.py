"""Turn a real .mobibdump into a fictional demo dump (for screenshots).

Usage: python anonymize.py <real.mobibdump> <demo.mobibdump>

Replaces: PUPI / serials, holder name, birth date, gender, card serials,
postal code, and all journeys (rebuilt from scratch at well-known stops).
Contracts are kept as-is.
"""
import datetime, re, sys

src, dst = sys.argv[1], sys.argv[2]
lines = open(src, encoding="utf-8").read().splitlines()

def get(key):
    for l in lines:
        if l.startswith(key + ": "):
            return l[len(key) + 2:]
    return None

def put(key, value):
    for i, l in enumerate(lines):
        if l.startswith(key + ": "):
            lines[i] = f"{key}: {value}"
            return
    raise KeyError(key)

def hexb(s): return bytearray(int(x, 16) for x in s.split())
def hexs(b): return " ".join(f"{x:02X}" for x in b)

def set_bits(buf, off, n, val):
    for i in range(n):
        bit = (val >> (n - 1 - i)) & 1
        p = off + i
        if bit: buf[p >> 3] |= 0x80 >> (p & 7)
        else:   buf[p >> 3] &= ~(0x80 >> (p & 7)) & 0xFF

def days(d): return (d - datetime.date(1997, 1, 1)).days

def bcd(n, digits):
    v = 0
    for ch in str(n).zfill(digits): v = (v << 4) | int(ch)
    return v

real_uid = hexb(get("PUPI"))
fake_uid = bytearray(b"\x4D\x4F\x42\x49")          # "MOBI"

# 1. Replace the card UID everywhere it appears in hex values.
for i, l in enumerate(lines):
    m = re.match(r"([^:#]+): ([0-9A-F]{2}(?: [0-9A-F]{2}){3,})$", l)
    if not m: continue
    b = hexb(m.group(2))
    j = b.find(real_uid)
    while j >= 0:
        b[j:j + len(real_uid)] = fake_uid
        j = b.find(real_uid, j + 1)
    lines[i] = f"{m.group(1)}: {hexs(b)}"

# 2. Holder (HOLDER_EXTENDED, 2 x 29 payload bytes, padded records).
recs = [hexb(get("HolderExt1")), hexb(get("HolderExt2"))]
flat = bytearray(58)
set_bits(flat, 168, 32, bcd(19900315, 8))          # birth date
set_bits(flat, 200, 2, 1)                          # gender: male
name = [ord(c) - 64 for c in "REGINALD"] + [31] + [ord(c) - 64 for c in "BLECHMAN"]
for k, v in enumerate(name):
    set_bits(flat, 205 + 5 * k, 5, v)
for r, n in ((0, len(recs[0])), (1, len(recs[1]))):
    out = bytearray(n)
    out[:29] = flat[29 * r:29 * r + 29]
    put(f"HolderExt{r + 1}", hexs(out))

# 3. Environment record (SFI 7, record 1): birth, card serial, postal code.
for i in range(int(get("Records"))):
    if get(f"Rec{i:02d}_SFI") == "7" and get(f"Rec{i:02d}_Index") == "1":
        env = hexb(get(f"Rec{i:02d}_Data"))
        set_bits(env, 42, 14, days(datetime.date(2029, 5, 12)))  # card validity end
        set_bits(env, 66, 32, bcd(19900315, 8))
        set_bits(env, 98, 76, 0)                   # card serial
        set_bits(env, 179, 14, 1000)               # postal code
        put(f"Rec{i:02d}_Data", hexs(env))

# 4. Journeys (SFI 23), most recent first.
D = datetime.date
journeys = [
    # date, hh, mm, provider, route, stop16, location17, first (hh, mm)
    (D(2026, 10, 2), 8, 47, 22, 92, 6314, 0, (8, 31)),     # Tram 92 Louise
    (D(2026, 10, 2), 8, 31, 0, 1, 0, (7 << 11) | (2 << 7) | 0x73, None),  # Metro Schuman
    (D(2026, 10, 1), 18, 12, 15, 71, 3372, 0, None),        # Bus 71 Flagey
    (D(2026, 9, 30), 17, 55, 22, 81, 2285, 0, None),        # Tram 81 Montgomery
]
ev_slots = [i for i in range(int(get("Records"))) if get(f"Rec{i:02d}_SFI") == "23"]
for slot, (d, hh, mm, prov, route, stop, loc, first) in zip(ev_slots, journeys):
    n = len(hexb(get(f"Rec{slot:02d}_Data")))
    b = bytearray(n)
    p = 0
    def w(bits, val):
        global p
        set_bits(b, p, bits, val); p += bits
    w(6, 3); w(14, days(d)); w(11, hh * 60 + mm); w(31, 0)
    w(5, 0b10011)                                  # stop, route, provider/location
    w(16, stop); w(16, route)
    w(5, prov); w(17, loc); w(10, 0)
    w(6, 0b10111)                                  # serial, unknown F, transfer, first stamp
    w(24, 40 + slot); w(16, 0); w(8, 0)
    fh, fm = first or (hh, mm)
    w(14, days(d)); w(11, fh * 60 + fm)
    put(f"Rec{slot:02d}_Data", hexs(b))

open(dst, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
print("wrote", dst)
