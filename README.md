# flipper-mobib

A Flipper Zero application to read and explore **MOBIB** cards — the
contactless transit card used across Belgium (STIB/MIVB, SNCB/NMBS, De Lijn,
TEC).

MOBIB is a [Calypso](https://en.wikipedia.org/wiki/Calypso_(electronic_ticketing_system))
card built on ISO/IEC 14443 Type B. This app aims to:

- Detect a MOBIB card on the Flipper's NFC field.
- Walk the Calypso file system and dump every readable record.
- Decode the card holder, environment, contracts and event log
  (Intercode v2 / Calypso profiles applicable to Belgium).
- Present trips and subscriptions in a friendly UI on the Flipper.
- Export the raw dump and decoded JSON to the SD card for offline analysis.

> **Status:** very early. The current build is a UI skeleton; NFC reading
> lands in the next milestones. See `docs/ARCHITECTURE.md` and
> `docs/MOBIB_NOTES.md`.

## Building

This app targets the **Momentum** firmware via [`ufbt`](https://github.com/flipperdevices/flipperzero-ufbt).

```sh
ufbt            # build the .fap into dist/
ufbt launch     # build, upload to a connected Flipper and launch
ufbt cli        # open a serial CLI to the device
```

The generated `dist/mobib.fap` can also be copied manually to
`/ext/apps/NFC/` on the SD card.

## Repository layout

```
application/        C sources for the FAP (entry point + scenes)
  scenes/           one file per Scene Manager scene
docs/               design notes and protocol references
icons/              app icon (10x10 PNG, 1-bit)
assets/             reserved for parser tables and sample dumps
application.fam     FAP manifest consumed by ufbt
```

## License

GPL-3.0-or-later. See [`LICENSE`](./LICENSE).

This project is **not affiliated with** STIB/MIVB, SNCB/NMBS, De Lijn, TEC,
Calypso Networks Association or any transit operator. It exists for
interoperability research and personal use with cards you legitimately own.
