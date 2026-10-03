# flipper-mobib

A [Flipper Zero](https://flipperzero.one/) app that reads and decodes Belgian
**MOBIB** transit cards — the Calypso/ISO 14443-B contactless card shared by
**STIB/MIVB**, **SNCB/NMBS**, **De Lijn** and **TEC**.

Tap any MOBIB card against the Flipper's back and the app identifies it,
dumps every readable file, decodes the human-meaningful fields and saves the
capture to the SD card so you can browse it again later without the card.

![About screen](docs/screenshots/about.png)

## What it decodes

| Section    | Fields                                                                                  |
|------------|-----------------------------------------------------------------------------------------|
| Overview   | PUPI, application serial, country, network, protocol version, expiry, counts           |
| Holder     | Name, gender, date of birth (personalised cards only), postal code                     |
| Contracts  | Per-slot tariff, sale date, duration, price in euros                                   |
| Journeys   | Date/time, transport mode, route/line, STIB metro station or bus stop name            |
| Records    | Grouped SFI listing with human labels and a hex preview of each record                 |
| FCI        | Parsed BER-TLV (AID, issuer extension, application serial) + raw dump                  |
| Deep scan  | Secondary Calypso applications (MPP/RT2/ETicket) and path-selected files we could read |

Every scan is auto-saved to `/ext/apps_data/mobib/dumps/<PUPI>_<UTC>.mobibdump`
as a human-readable [FlipperFormat](https://developer.flipper.net/flipperzero/doxygen/group__FlipperFormat.html)
file. Use **Saved cards** from the main menu to re-render any past capture
without a card present.

## What it explicitly does not do

**Writes are impossible** without the issuer's keys. Calypso writes
(`UPDATE RECORD`, `INCREASE`, `DECREASE`) require an authenticated Secure
Session opened with a 3DES/AES key held in a hardware SAM that only
transit operators possess. This app is read-only by design and cannot:

- Forge, modify or top up contracts
- Increment/decrement remaining-trip counters
- Decrypt the card's MAC authenticators
- Read files whose access conditions demand an open Secure Session

If someone offers you a Flipper app that "writes MOBIB", it is either a
scam or is misusing a leaked operator key — neither will survive contact
with an actual fare gate.

## Building

Target firmware: **Momentum** via [`ufbt`](https://github.com/flipperdevices/flipperzero-ufbt).

```sh
ufbt            # build the .fap into dist/
ufbt launch     # build, upload to a connected Flipper and launch
ufbt cli        # open a serial CLI to the device
```

The built `dist/mobib.fap` can also be copied manually to
`/ext/apps/NFC/` on the SD card.

## Repository layout

```
application/
  calypso/         APDU layer, BER-TLV walker, record decoders
                   (FCI, env, event, contract, holder, stations, bus)
  nfc/             Iso14443_4b poller wrapper + full-card dump
  storage/         FlipperFormat persistence (save + load)
  ui/              Section formatters (Overview/Holder/…/Deep scan)
  scenes/          Scene Manager scenes (start, scan, card, etc.)
docs/              Design notes + MOBIB protocol references
samples/           Git-ignored local .mobibdump captures
icons/             App icon (10x10 PNG, 1-bit)
application.fam    FAP manifest consumed by ufbt
```

## Credits & prior art

The app ships field layouts, station tables and alphabet rules ported from
two excellent open-source projects. They did the hard work; this is a
re-expression of their findings in a form the Flipper Zero firmware can run:

- **[metrodroid/metrodroid](https://github.com/metrodroid/metrodroid)** —
  Kotlin/Android transit reader. Source of MOBIB environment, event,
  contract and holder bit layouts; of the Calypso file map
  (`CalypsoApplication.kt`); of the 5-bit name alphabet. GPL-3.0.
- **[zoobab/mobib-extractor](https://github.com/zoobab/mobib-extractor)** —
  2009-era Python MOBIB decoder. Source of the STIB metro station table
  (70 rows) and bus stop database (~2200 rows) compiled into
  `calypso_stations.c` / `calypso_bus.c`.
- **Intercode v2** / **Calypso** public specifications.

## Updating the station tables

When STIB renumbers a stop or adds a new one, regenerate the tables from
the upstream CSVs:

```sh
python3 <<'PY'
import csv
with open('zoobab-data/Database/Metro.csv') as f:
    r = csv.reader(f); next(r)
    for row in r:
        _, zone, sz, st, line, name, *_ = [c.strip() for c in row]
        print(f'{{0x{int(zone,2):02X}, 0x{int(sz,2):X}, 0x{int(st,2):02X}, "{line}", "{name}"}},')
PY
```

The bus table uses the same CSV with line/code columns and is kept sorted
on `(line, code)` for binary search.

## License

GPL-3.0-or-later. See [`LICENSE`](./LICENSE).

Not affiliated with STIB/MIVB, SNCB/NMBS, De Lijn, TEC, Calypso Networks
Association, or any transit operator. Exists for interoperability research
and for reading your own cards. Respect your local laws around RFID and
personal data.
