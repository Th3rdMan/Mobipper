/*
 * MOBIB — dump persistence. Contract documented in `mobib_storage.h`.
 */

#include "mobib_storage.h"

#include <furi.h>
#include <furi_hal_rtc.h>
#include <datetime/datetime.h>
#include <storage/storage.h>
#include <flipper_format/flipper_format.h>

#define TAG "MobibStorage"

static void mobib_storage_format_pupi(const MobibCardInfo* card, FuriString* out) {
    furi_string_reset(out);
    for(size_t i = 0; i < card->pupi_len; ++i) {
        furi_string_cat_printf(out, "%02X", card->pupi[i]);
    }
    if(card->pupi_len == 0) furi_string_set_str(out, "UNKNOWN");
}

static void mobib_storage_build_path(
    const MobibDump* dump,
    const DateTime* now,
    FuriString* path_out) {
    FuriString* pupi = furi_string_alloc();
    mobib_storage_format_pupi(&dump->card, pupi);

    furi_string_printf(
        path_out,
        "%s/%s_%04u%02u%02uT%02u%02u%02u%s",
        MOBIB_DUMP_DIR,
        furi_string_get_cstr(pupi),
        now->year, now->month, now->day,
        now->hour, now->minute, now->second,
        MOBIB_DUMP_EXT);

    furi_string_free(pupi);
}

static bool mobib_storage_write_card(FlipperFormat* ff, const MobibCardInfo* card) {
    if(!flipper_format_write_hex(ff, "PUPI", card->pupi, card->pupi_len)) return false;
    if(!flipper_format_write_hex(ff, "ApplicationData", card->app_data, sizeof(card->app_data)))
        return false;

    const uint32_t iso4 = card->supports_iso14443_4 ? 1 : 0;
    if(!flipper_format_write_uint32(ff, "ISO14443_4", &iso4, 1)) return false;

    const uint32_t fsm = card->frame_size_max;
    if(!flipper_format_write_uint32(ff, "FrameSizeMax", &fsm, 1)) return false;

    const uint32_t fwt = card->fwt_fc_max;
    if(!flipper_format_write_uint32(ff, "FWTfcMax", &fwt, 1)) return false;
    return true;
}

static bool mobib_storage_write_records(FlipperFormat* ff, const MobibDump* dump) {
    const uint32_t calypso = dump->calypso_selected ? 1 : 0;
    if(!flipper_format_write_uint32(ff, "CalypsoSelected", &calypso, 1)) return false;

    if(dump->calypso_selected && dump->fci_len > 0) {
        if(!flipper_format_write_hex(ff, "CalypsoFCI", dump->fci, dump->fci_len))
            return false;
    }

    const uint32_t count = dump->record_count;
    if(!flipper_format_write_uint32(ff, "Records", &count, 1)) return false;

    char key[24];
    for(size_t i = 0; i < dump->record_count; ++i) {
        const MobibRecord* r = &dump->records[i];

        snprintf(key, sizeof(key), "Rec%02u_SFI", (unsigned)i);
        const uint32_t sfi32 = r->sfi;
        if(!flipper_format_write_uint32(ff, key, &sfi32, 1)) return false;

        snprintf(key, sizeof(key), "Rec%02u_Index", (unsigned)i);
        const uint32_t idx32 = r->record;
        if(!flipper_format_write_uint32(ff, key, &idx32, 1)) return false;

        snprintf(key, sizeof(key), "Rec%02u_Data", (unsigned)i);
        if(!flipper_format_write_hex(ff, key, r->data, r->len)) return false;
    }
    return true;
}

bool mobib_storage_save_dump(const MobibDump* dump, FuriString* path_out) {
    furi_assert(dump);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    /* `storage_simply_mkdir` is non-recursive, so walk the path and
     * create every intermediate directory. Existing dirs are reported as
     * failures by the API but we don't care — only the final stat
     * matters and we let `flipper_format_file_open_new` surface that. */
    storage_simply_mkdir(storage, "/ext/apps_data");
    storage_simply_mkdir(storage, "/ext/apps_data/mobib");
    storage_simply_mkdir(storage, MOBIB_DUMP_DIR);

    DateTime now;
    furi_hal_rtc_get_datetime(&now);

    FuriString* path = furi_string_alloc();
    mobib_storage_build_path(dump, &now, path);

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool ok = false;

    do {
        if(!flipper_format_file_open_new(ff, furi_string_get_cstr(path))) {
            FURI_LOG_E(TAG, "open_new failed: %s", furi_string_get_cstr(path));
            break;
        }
        if(!flipper_format_write_header_cstr(ff, "MOBIB raw dump", MOBIB_DUMP_FORMAT_VERSION))
            break;
        if(!flipper_format_write_comment_cstr(
               ff, "Captured by flipper-mobib. See docs/MOBIB_NOTES.md."))
            break;
        if(!mobib_storage_write_card(ff, &dump->card)) break;
        if(!mobib_storage_write_records(ff, dump)) break;

        ok = true;
    } while(0);

    flipper_format_file_close(ff);
    flipper_format_free(ff);

    if(ok && path_out) furi_string_set(path_out, path);
    if(!ok) {
        FURI_LOG_E(TAG, "write failed for %s", furi_string_get_cstr(path));
        storage_simply_remove(storage, furi_string_get_cstr(path));
    }

    furi_string_free(path);
    furi_record_close(RECORD_STORAGE);
    return ok;
}
