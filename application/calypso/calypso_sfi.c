/*
 * MOBIB — SFI label table. See `calypso_sfi.h`.
 */

#include "calypso_sfi.h"

#include <stddef.h>

const char* calypso_sfi_label(uint8_t sfi) {
    switch(sfi) {
    case 0x01: return "Free";
    case 0x02: return "ICC";
    case 0x03: return "ID";
    case 0x04: return "AID";
    case 0x05: return "Display";
    case 0x06: return "Contracts 2";
    case 0x07: return "Environment";
    case 0x08: return "Log";
    case 0x09: return "Contracts";
    case 0x0A: return "Counters 1";
    case 0x0B: return "Counters 2";
    case 0x0C: return "Counters 3";
    case 0x0D: return "Counters 4";
    case 0x10: return "Counters 10";
    case 0x14: return "Load log";
    case 0x15: return "Purchase log";
    case 0x17: return "Event log";
    case 0x19: return "Counters 9";
    case 0x1D: return "Special events";
    case 0x1E: return "Contract list";
    default:   return NULL;
    }
}
