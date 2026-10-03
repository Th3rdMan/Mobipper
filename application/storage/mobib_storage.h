/*
 * MOBIB — dump persistence.
 *
 * Writes a `MobibDump` to the SD card as a FlipperFormat file under
 * `/ext/apps_data/mobipper/dumps/`. The format is human-readable, version
 * stamped and stable: every record is stored as a hex-array under a key
 * encoding its SFI and record index, so future tooling (decoder,
 * exporter) can iterate without parsing surprises.
 */
#pragma once

#include <core/string.h>
#include <stdbool.h>

#include "../nfc/mobib_nfc.h"
#include "../calypso/calypso_holder.h"

/** Directory where dumps are stored. Created on demand. */
#define MOBIB_DUMP_DIR "/ext/apps_data/mobipper/dumps"

/** Dump directory used before the app id became "mobipper" (v0.1). */
#define MOBIB_DUMP_DIR_LEGACY "/ext/apps_data/mobib/dumps"

/** File extension for raw dumps (purely cosmetic). */
#define MOBIB_DUMP_EXT ".mobibdump"

/** Current on-disk format version. Bump on incompatible layout changes. */
#define MOBIB_DUMP_FORMAT_VERSION 1

/**
 * @brief Decode the HOLDER_EXTENDED records carried by a dump.
 *
 * @return true iff the holder file is present and parsed.
 */
bool mobib_dump_holder(const MobibDump* dump, CalypsoHolder* out);

/**
 * @brief Format the holder name as "NOM Prenom".
 *
 * The card stores the name upper-case, first name first, then a separator
 * and the surname. The surname is kept upper-case and moved first, the
 * first name is capitalised.
 *
 * @return false (and empty `out`) if the holder has no name.
 */
bool mobib_holder_display_name(const CalypsoHolder* holder, FuriString* out);

/**
 * @brief Persist a dump to SD.
 *
 * If a dump of the same card (same PUPI) already exists, it is overwritten.
 * Otherwise personalised cards are saved as "NOM Prenom.mobibdump" (with a
 * numeric suffix when the name already exists), anonymous ones as
 * "<PUPI>_<UTC>.mobibdump".
 *
 * @param[in]  dump      Filled snapshot. Must not be NULL.
 * @param[out] path_out  On success, receives the absolute path of the file
 *                       just written. May be NULL if the caller does not
 *                       care.
 * @return true iff the file was created and fully written.
 */
bool mobib_storage_save_dump(const MobibDump* dump, FuriString* path_out);

/** Postal code table shipped as an app asset ("CODE;Commune" lines). */
#define MOBIB_POSTAL_CODES_PATH APP_ASSETS_PATH("postal_codes.txt")

/**
 * @brief Look up the commune of a Belgian postal code.
 *
 * @return false (and empty `commune`) if the code is unknown or the asset
 *         file is missing.
 */
bool mobib_postal_lookup(uint16_t code, FuriString* commune);

/**
 * @brief Move dumps saved by v0.1 (app id "mobib") to the current folder.
 *
 * Runs once: only when the legacy folder exists and the current one does not.
 */
void mobib_storage_migrate_legacy(void);

/**
 * @brief Path of the alphabetically first dump, or MOBIB_DUMP_DIR if none.
 */
void mobib_storage_first_dump(FuriString* path_out);

/**
 * @brief Load a previously persisted dump from SD.
 *
 * @param[in]  path  Absolute path to a .mobibdump FlipperFormat file.
 * @param[out] dump  Populated on success.
 * @return true iff the file was readable and at least the card / records
 *         section parsed cleanly. Holder fields are best-effort.
 */
bool mobib_storage_load_dump(const char* path, MobibDump* dump);
