# MOBIB / Calypso research notes

Working notes used to drive the implementation. Anything written here is
*best-effort* until confirmed against a real card with the Flipper.

## Card families seen in the wild

| Variant            | Chip family            | Notes                                  |
|--------------------|------------------------|----------------------------------------|
| MOBIB Basic        | Calypso Prime / Light  | Anonymous, sold at vending machines    |
| MOBIB (personal)   | Calypso Prime          | Named card, photo, multi-operator      |

All current MOBIB cards are **ISO 14443 Type B** and speak the
**Calypso** application protocol. Older "MOBIB" branded MIFARE Classic
cards are out of scope.

## Calypso essentials

- Selection: `SELECT` by AID `1TIC.ICA` (`31 54 49 43 2E 49 43 41`).
- File system: hierarchical (`MF` → `DF` → `EF`), each EF identified by an
  SFI (Short File Identifier) and addressed by record number.
- Read command: `READ RECORD` (`00 B2 <rec> <sfi<<3 | 4> <Le>`).
- Records are typically 29 bytes; the layout is described by Intercode v2
  for Belgian/French networks.

## SFIs of interest (to be verified per card)

| SFI    | EF name              | Content                                  |
|--------|----------------------|------------------------------------------|
| `0x07` | ICC                  | Serial number, issuer, country code      |
| `0x08` | ID                   | Card holder identifiers                  |
| `0x17` | Holder / Identity    | Name, date of birth (personal cards)     |
| `0x1D` | Display              | Pretty-printable card title              |
| `0x07` | Environment / Holder | Network id, application validity         |
| `0x09` | Contracts            | Up to 4 subscription records             |
| `0x10` | Event log            | Last ~3 validations / journeys           |
| `0x19` | Counters             | Trip counters per contract               |

> SFI numbers above are placeholders pulled from public Calypso/Intercode
> material; they will be replaced with values confirmed by dumping our two
> test cards in milestone M2.

## References

- Calypso Functional Specification — Card Application (public summary).
- Intercode v2 — French/Belgian transit data layout.
- prior art: `metrodroid` (Android), `kalypso` (Python), `farebot`.
