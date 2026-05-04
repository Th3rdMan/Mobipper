/*
 * MOBIB — SFI label table.
 *
 * Maps Calypso Short File Identifiers to their human role on a MOBIB
 * transit card. Sourced from metrodroid `CalypsoApplication.kt`. Labels
 * are intentionally short (≤14 chars) so they fit the Flipper TextBox.
 *
 * MOBIB-specific deviations from the generic Calypso layout: 0x17 is
 * the active event log on MOBIB cards rather than the parking-app's
 * public-parameters file (`MPP_PUBLIC_PARAMETERS`) it represents on
 * other Calypso applications.
 */
#pragma once

#include <stdint.h>

/** Returns a static short label for the given SFI, or NULL if unknown. */
const char* calypso_sfi_label(uint8_t sfi);
