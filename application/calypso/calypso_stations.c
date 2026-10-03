/*
 * MOBIB — STIB metro station table.
 *
 * Generated from zoobab/mobib-extractor `Database/Metro.csv` (use freely
 * per upstream README). Fields:
 *   zone   = 6 bits, top of LOCATION_ID
 *   subzone= 4 bits
 *   station= 7 bits, bottom of LOCATION_ID
 *
 * To regenerate, run:
 *   python3 - <<EOF
 *   import csv
 *   r = csv.reader(open("Database/Metro.csv"))
 *   next(r)
 *   for type_, zone, sz, station, line, name, *_ in r:
 *       print(f'    {{0x{int(zone,2):02X}, 0x{int(sz,2):X}, '
 *             f'0x{int(station,2):02X}, "{line}", "{name}"}},')
 *   EOF
 *
 * Since then, line labels follow the 2009+ network (1, 2, 5, 6; premetro
 * for the tram tunnels) and a few codes were added: Gare du Midi 0x34 (seen
 * by metrodroid) and Elisabeth 0x29 / 0x2A, codes read on real 2026 cards
 * right after Ribaucourt (0x1F) on the line 2/6 sub-zone. As for Gare du
 * Midi, one station can carry two consecutive codes.
 */

#include "calypso_stations.h"

static const CalypsoMetroStation kMetro[] = {
    {0x07, 0x3, 0x54, "1", "Stockel"},
    {0x07, 0x3, 0x4B, "1", "Crainhem"},
    {0x07, 0x3, 0x41, "1", "Alma"},
    {0x07, 0x3, 0x37, "1", "Vandervelde"},
    {0x07, 0x3, 0x2D, "1", "Roodebeek"},
    {0x07, 0x3, 0x23, "1", "Tomberg"},
    {0x07, 0x3, 0x1A, "1", "Gribaumont"},
    {0x07, 0x3, 0x10, "1", "Josephine-Charlotte"},
    {0x07, 0x3, 0x06, "1", "Montgomery"},
    {0x07, 0x2, 0x7C, "1/5", "Merode"},
    {0x07, 0x2, 0x73, "1/5", "Schuman"},
    {0x07, 0x2, 0x69, "1/5", "Maelbeek"},
    {0x07, 0x2, 0x5F, "1/2/5/6", "Arts-Loi"},
    {0x07, 0x2, 0x55, "1/5", "Park"},
    {0x07, 0x2, 0x4C, "1/5", "Gare Centrale"},
    {0x07, 0x2, 0x42, "1/5", "De Brouckere"},
    {0x07, 0x4, 0x40, "1/5", "Sainte Catherine"},
    {0x07, 0x4, 0x49, "1/5", "Comte de Flandre"},
    {0x07, 0x4, 0x53, "1/5", "Etangs Noirs"},
    {0x08, 0x7, 0x2C, "1/5", "Beekant"},
    {0x08, 0x7, 0x22, "1/2/5/6", "Gare de l'Ouest"},
    {0x1B, 0x8, 0x2F, "5", "Jacques Brel"},
    {0x1B, 0x8, 0x25, "5", "Aumale"},
    {0x1B, 0x8, 0x1B, "5", "Saint Guidon"},
    {0x1B, 0x8, 0x11, "5", "Veeweyde"},
    {0x1B, 0x8, 0x08, "5", "Bizet"},
    {0x1B, 0x7, 0x7E, "5", "La Roue"},
    {0x1B, 0x7, 0x74, "5", "CERIA"},
    {0x1B, 0x7, 0x6B, "5", "Eddy Merckx"},
    {0x1B, 0x7, 0x61, "5", "Erasme"},
    {0x20, 0x5, 0x40, "6", "Roi Baudouin"},
    {0x20, 0x5, 0x36, "6", "Heysel"},
    {0x20, 0x5, 0x2C, "6", "Houba-Brugmann"},
    {0x20, 0x5, 0x23, "6", "Stuyvenbergh"},
    {0x20, 0x5, 0x19, "6", "Bockstael"},
    {0x20, 0x5, 0x0F, "6", "Pannenhuis"},
    {0x20, 0x5, 0x05, "6", "Belgica"},
    {0x0D, 0x3, 0x5B, "2/6", "Osseghem"},
    {0x0D, 0x3, 0x65, "2/6", "Simonis"},
    {0x1A, 0x5, 0x12, "5", "Thieffry"},
    {0x1A, 0x5, 0x1C, "5", "Petillon"},
    {0x1A, 0x5, 0x25, "5", "Hankar"},
    {0x1A, 0x5, 0x2F, "5", "Delta"},
    {0x1A, 0x5, 0x39, "5", "Beaulieu"},
    {0x1A, 0x5, 0x43, "5", "Demey"},
    {0x1A, 0x5, 0x4C, "5", "Hermann-Debroux"},
    {0x0C, 0x2, 0x29, "2/6", "Elisabeth"},
    {0x0C, 0x2, 0x2A, "2/6", "Elisabeth"},
    {0x0C, 0x2, 0x1F, "2/6", "Ribaucourt"},
    {0x0C, 0x2, 0x0B, "2/6", "Yser"},
    {0x0C, 0x2, 0x02, "2/6", "Rogier"},
    {0x0C, 0x1, 0x78, "2/6", "Botanique"},
    {0x0C, 0x1, 0x6E, "2/6", "Madou"},
    {0x0C, 0x1, 0x03, "2/6", "Trone"},
    {0x0C, 0x1, 0x0C, "2/6", "Porte de Namur"},
    {0x0C, 0x1, 0x16, "2/6", "Louise"},
    {0x0C, 0x1, 0x20, "2/6", "Hotel des Monnaies"},
    {0x0C, 0x1, 0x2A, "2/6", "Porte de Hal"},
    {0x0C, 0x1, 0x33, "2/6", "Gare du Midi"},
    {0x0C, 0x1, 0x34, "2/6", "Gare du Midi"},
    {0x0C, 0x1, 0x3D, "2/6", "Clemenceau"},
    {0x0C, 0x1, 0x47, "2/6", "Delacroix"},
    {0x2D, 0xD, 0x44, "Premetro", "Albert"},
    {0x2D, 0xD, 0x3A, "Premetro", "Horta"},
    {0x2D, 0xD, 0x31, "Premetro", "Parvis Saint Gilles"},
    {0x2D, 0xD, 0x13, "Premetro", "Lemonnier"},
    {0x2D, 0xD, 0x09, "Premetro", "Anneessens"},
    {0x2D, 0xD, 0x00, "Premetro", "Bourse"},
    {0x2D, 0xC, 0x32, "Premetro", "Gare du Nord"},
    {0x1F, 0x9, 0x66, "Premetro", "Diamant"},
    {0x1F, 0x9, 0x5C, "Premetro", "Georges Henri"},
    {0x1F, 0xA, 0x34, "Premetro", "Boileau"},
};

static const size_t kMetroCount = sizeof(kMetro) / sizeof(kMetro[0]);

const CalypsoMetroStation* calypso_metro_station_lookup(
    uint8_t zone,
    uint8_t subzone,
    uint8_t station) {
    for(size_t i = 0; i < kMetroCount; ++i) {
        const CalypsoMetroStation* s = &kMetro[i];
        if(s->zone == zone && s->subzone == subzone && s->station == station) {
            return s;
        }
    }
    return NULL;
}

const CalypsoMetroStation* calypso_metro_station_lookup_id(uint32_t location_id) {
    /* LOCATION_ID = zone[6] | subzone[4] | station[7] (17 bits, MSB-first). */
    const uint8_t zone    = (uint8_t)((location_id >> 11) & 0x3F);
    const uint8_t subzone = (uint8_t)((location_id >> 7)  & 0x0F);
    const uint8_t station = (uint8_t)( location_id        & 0x7F);
    return calypso_metro_station_lookup(zone, subzone, station);
}
