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
 */

#include "calypso_stations.h"

static const CalypsoMetroStation kMetro[] = {
    {0x07, 0x3, 0x54, "1B", "Stockel"},
    {0x07, 0x3, 0x4B, "1B", "Crainhem"},
    {0x07, 0x3, 0x41, "1B", "Alma"},
    {0x07, 0x3, 0x37, "1B", "Vandervelde"},
    {0x07, 0x3, 0x2D, "1B", "Roodebeek"},
    {0x07, 0x3, 0x23, "1B", "Tomberg"},
    {0x07, 0x3, 0x1A, "1B", "Gribaumont"},
    {0x07, 0x3, 0x10, "1B", "Josephine-Charlotte"},
    {0x07, 0x3, 0x06, "1B", "Montgomery"},
    {0x07, 0x2, 0x7C, "1A/1B", "Merode"},
    {0x07, 0x2, 0x73, "1A/1B", "Schuman"},
    {0x07, 0x2, 0x69, "1A/1B", "Maelbeek"},
    {0x07, 0x2, 0x5F, "1A/1B/2", "Art-Loi"},
    {0x07, 0x2, 0x55, "1A/1B", "Park"},
    {0x07, 0x2, 0x4C, "1A/1B", "Gare Centrale"},
    {0x07, 0x2, 0x42, "1A/1B", "De Brouckere"},
    {0x07, 0x4, 0x40, "1A/1B", "Sainte Catherine"},
    {0x07, 0x4, 0x49, "1A/1B", "Comte de Flandres"},
    {0x07, 0x4, 0x53, "1A/1B", "Etangs Noirs"},
    {0x08, 0x7, 0x2C, "1A/1B", "Beekant"},
    {0x08, 0x7, 0x22, "1B", "Gare de l'Ouest"},
    {0x1B, 0x8, 0x2F, "1B", "Jacques Brel"},
    {0x1B, 0x8, 0x25, "1B", "Aumale"},
    {0x1B, 0x8, 0x1B, "1B", "Saint Guidon"},
    {0x1B, 0x8, 0x11, "1B", "Veeweyde"},
    {0x1B, 0x8, 0x08, "1B", "Bizet"},
    {0x1B, 0x7, 0x7E, "1B", "La Roue"},
    {0x1B, 0x7, 0x74, "1B", "CERIA"},
    {0x1B, 0x7, 0x6B, "1B", "Eddy Merckx"},
    {0x1B, 0x7, 0x61, "1B", "Erasme"},
    {0x20, 0x5, 0x40, "1A", "Roi Baudouin"},
    {0x20, 0x5, 0x36, "1A", "Heysel"},
    {0x20, 0x5, 0x2C, "1A", "Houba-Brugmann"},
    {0x20, 0x5, 0x23, "1A", "Stuyvenbergh"},
    {0x20, 0x5, 0x19, "1A", "Bockstael"},
    {0x20, 0x5, 0x0F, "1A", "Pannehuis"},
    {0x20, 0x5, 0x05, "1A", "Belgica"},
    {0x0D, 0x3, 0x5B, "1A", "Osseghem"},
    {0x0D, 0x3, 0x65, "1A/2", "Simonis"},
    {0x1A, 0x5, 0x12, "1A", "Thieffry"},
    {0x1A, 0x5, 0x1C, "1A", "Petillon"},
    {0x1A, 0x5, 0x25, "1A", "Hankar"},
    {0x1A, 0x5, 0x2F, "1A", "Delta"},
    {0x1A, 0x5, 0x39, "1A", "Beaulieu"},
    {0x1A, 0x5, 0x43, "1A", "Demey"},
    {0x1A, 0x5, 0x4C, "1A", "Hermann-Debroux"},
    {0x0C, 0x2, 0x1F, "2", "Ribaucourt"},
    {0x0C, 0x2, 0x0B, "2", "Yser"},
    {0x0C, 0x2, 0x02, "2/PremNS", "Rogier"},
    {0x0C, 0x1, 0x78, "2", "Botanique"},
    {0x0C, 0x1, 0x6E, "2", "Madou"},
    {0x0C, 0x1, 0x03, "2", "Trone"},
    {0x0C, 0x1, 0x0C, "2", "Porte de Namur"},
    {0x0C, 0x1, 0x16, "2", "Louise"},
    {0x0C, 0x1, 0x20, "2", "Hotel des Monnaies"},
    {0x0C, 0x1, 0x2A, "2/PremNS", "Porte de Hal"},
    {0x0C, 0x1, 0x33, "2/PremNS", "Gare du Midi"},
    {0x0C, 0x1, 0x3D, "2", "Clemenceau"},
    {0x0C, 0x1, 0x47, "2", "Delacroix"},
    {0x2D, 0xD, 0x44, "North-South", "Albert"},
    {0x2D, 0xD, 0x3A, "North-South", "Horta"},
    {0x2D, 0xD, 0x31, "North-South", "Parvis Saint Gilles"},
    {0x2D, 0xD, 0x13, "North-South", "Lemonnier"},
    {0x2D, 0xD, 0x09, "North-South", "Anneessens"},
    {0x2D, 0xD, 0x00, "North-South", "Bourse"},
    {0x2D, 0xC, 0x32, "North-South", "Gare du Nord"},
    {0x1F, 0x9, 0x66, "West", "Diamant"},
    {0x1F, 0x9, 0x5C, "West", "Georges Henri"},
    {0x1F, 0xA, 0x34, "West", "Boileau"},
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
