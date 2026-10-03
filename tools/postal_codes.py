"""Build the postal code -> commune file shipped with the app.

Source: bpost "Liste des codes postaux belges (communes)" on Open Data
Wallonie-Bruxelles (odwb.be), CSV export with ';' delimiter and columns
post_code, municipality_name_french/dutch/german,
sub_municipality_name_french/dutch.

Usage: python postal_codes.py <postal_codes.csv> <out.txt> [--report]

Output: one "CODE;Commune" line per postal code, sorted by code, ASCII only
(Flipper fonts have no accents). French name first, else Dutch, else
German. When several municipalities share a code (only 1000, 1040 and 1050
in the 2025 file), OVERRIDES gives bpost's usual name.
"""
import collections, csv, sys, unicodedata


def ascii_only(s):
    s = s.replace("œ", "oe").replace("Œ", "Oe")
    return unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode()


def name_of(row, prefix):
    for lang in ("french", "dutch", "german"):
        v = row.get(f"{prefix}_name_{lang}", "").strip()
        if v:
            return v
    return ""


# Codes shared by several Brussels municipalities: use bpost's usual name.
OVERRIDES = {1000: "Bruxelles", 1040: "Etterbeek", 1050: "Ixelles"}

src, out = sys.argv[1], sys.argv[2]
report = "--report" in sys.argv

by_code = collections.defaultdict(list)
with open(src, encoding="utf-8-sig", newline="") as f:
    for row in csv.DictReader(f, delimiter=";"):
        code = row["post_code"].strip()
        if not code.isdigit():
            continue
        by_code[int(code)].append((name_of(row, "municipality"), name_of(row, "sub_municipality")))

result = {}
ambiguous = 0
for code, rows in by_code.items():
    muns = collections.Counter(m for m, _ in rows)
    main = {m for m, sub in rows if sub and sub == m}
    if len(muns) > 1:
        ambiguous += 1
    candidates = [m for m in muns if m in main] or list(muns)
    best = OVERRIDES.get(code) or max(candidates, key=lambda m: (muns[m], -len(m)))
    result[code] = ascii_only(best)
    if report and len(muns) > 1:
        print(f"{code}: {dict(muns)} -> {best}")

with open(out, "w", encoding="ascii", newline="\n") as f:
    for code in sorted(result):
        f.write(f"{code};{result[code]}\n")

print(f"codes: {len(result)}  ambiguous: {ambiguous}  wrote {out}", file=sys.stderr)
