/*
 * MOBIB — SFI label table. See `calypso_sfi.h`.
 */

#include "calypso_sfi.h"

#include <stddef.h>

const char* calypso_sfi_label(uint8_t sfi) {
    switch(sfi) {
    case 0x01:
        return "Libre";
    case 0x02:
        return "ICC";
    case 0x03:
        return "ID";
    case 0x04:
        return "AID";
    case 0x05:
        return "Affichage";
    case 0x06:
        return "Abonnements 2";
    case 0x07:
        return "Environnement";
    case 0x08:
        return "Journal";
    case 0x09:
        return "Abonnements";
    case 0x0A:
        return "Compteurs 1";
    case 0x0B:
        return "Compteurs 2";
    case 0x0C:
        return "Compteurs 3";
    case 0x0D:
        return "Compteurs 4";
    case 0x10:
        return "Compteurs 10";
    case 0x14:
        return "Journal recharges";
    case 0x15:
        return "Journal achats";
    case 0x17:
        return "Journal trajets";
    case 0x19:
        return "Compteurs 9";
    case 0x1D:
        return "Evenements speciaux";
    case 0x1E:
        return "Liste abonnements";
    default:
        return NULL;
    }
}
