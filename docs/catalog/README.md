# Mobipper

Reader for **MOBIB** cards, the public transport cards of Belgium (STIB/MIVB in Brussels, SNCB/NMBS, De Lijn, TEC). The interface is in **French**.

Mobipper is based on **flipper-mobib** by **i12bp8**, who wrote the NFC reader and the Calypso decoders: https://github.com/i12bp8/flipper-mobib

## What it shows

- **Overview** right after the scan: holder name, postal code and municipality, birth date, card validity, subscription end, remaining trips, last use
- **Holder**, **Contracts** (tariff, purchase date, duration, price, remaining trips)
- **Journeys**: date and time, metro / tram / bus and line, STIB stop or metro station, transfers
- **Records**, **FCI** and a deep scan of secondary Calypso applications

## How to use

1. Open **Lire une carte** and hold the card flat against the back of the Flipper
2. The overview opens; press Back to see the other sections
3. Every scan is saved as "NAME Firstname" in **Sauvegardes**; scanning the same card again updates its save

## Notes

- Read only: writing to a MOBIB card needs the operators' keys, which never leave their hardware security modules
- The card itself only keeps its last few journeys
- Only read your own card, or a card whose owner agreed to it: it holds personal data
- STIB stop names: STIB-MIVB open data (CC BY 4.0). Postal codes: bpost / NGI-IGN

Source code, screenshots and full documentation (in French): https://github.com/Th3rdMan/Mobipper
