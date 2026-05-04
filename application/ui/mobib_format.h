/*
 * MOBIB — Section formatters for the card overview UI.
 *
 * Each function fills `out` with a human-readable, line-wrapped
 * representation of one aspect of the card. Output is plain ASCII so
 * the Flipper TextBox can scroll it without surprises.
 */
#pragma once

#include <core/string.h>
#include "../nfc/mobib_nfc.h"

typedef enum {
    MobibSectionOverview,
    MobibSectionHolder,
    MobibSectionContracts,
    MobibSectionJourneys,
    MobibSectionRecords,
    MobibSectionFci,
    MobibSectionDeep,
    MobibSectionCount,
} MobibSection;

const char* mobib_section_title(MobibSection section);

void mobib_format_section(
    MobibSection      section,
    const MobibDump*  dump,
    FuriString*       out);
