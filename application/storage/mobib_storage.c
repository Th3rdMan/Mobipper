/*
 * MOBIB — dump persistence. Contract documented in `mobib_storage.h`.
 */

#include "mobib_storage.h"

#include <furi.h>
#include <string.h>
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

bool mobib_dump_holder(const MobibDump* dump, CalypsoHolder* out) {
    if(!dump->holder_ext_present) return false;

    /* Each HOLDER_EXTENDED record contains 29 bytes of payload; the
     * Calypso layer pads responses to 32 bytes. Concatenating the
     * full 32-byte slots leaves three zero bytes embedded in the
     * middle of the name field and breaks the bit alignment. We
     * trim to 29 bytes per record to match metrodroid / zoobab. */
    const size_t kRealRecLen = 29;

    uint8_t flat[MOBIB_HOLDER_EXT_RECS * 29];
    size_t flat_len = 0;
    for(size_t i = 0; i < MOBIB_HOLDER_EXT_RECS; ++i) {
        size_t n = dump->holder_ext_len[i];
        if(n > kRealRecLen) n = kRealRecLen;
        if(n > 0) {
            memcpy(&flat[flat_len], dump->holder_ext[i], n);
            flat_len += n;
        }
    }
    return calypso_holder_parse(flat, flat_len, out) && out->valid;
}

bool mobib_holder_display_name(const CalypsoHolder* holder, FuriString* out) {
    furi_string_reset(out);

    /* The card stores "FIRSTNAME<sep>SURNAME" (separator rendered as a
     * space by the 5-bit decoder). Everything after the first separator
     * is the surname, so compound surnames ("VAN DAMME") stay whole. */
    const char* name = holder->name;
    const char* sep = strchr(name, ' ');
    if(!sep) {
        furi_string_set_str(out, name);
        return !furi_string_empty(out);
    }

    const char* surname = sep;
    while(*surname == ' ') surname++;
    furi_string_set_str(out, surname);
    furi_string_push_back(out, ' ');

    /* First name capitalised: "KEVIN" -> "Kevin". */
    for(const char* c = name; c < sep; ++c) {
        furi_string_push_back(out, c == name ? *c : (char)(*c - 'A' + 'a'));
    }
    furi_string_trim(out);
    return !furi_string_empty(out);
}

static void mobib_storage_build_path(
    Storage* storage,
    const MobibDump* dump,
    const DateTime* now,
    FuriString* path_out) {
    CalypsoHolder holder;
    FuriString* name = furi_string_alloc();

    if(mobib_dump_holder(dump, &holder) && mobib_holder_display_name(&holder, name)) {
        furi_string_printf(
            path_out, "%s/%s%s", MOBIB_DUMP_DIR, furi_string_get_cstr(name), MOBIB_DUMP_EXT);
        for(unsigned n = 2; storage_common_exists(storage, furi_string_get_cstr(path_out));
            ++n) {
            furi_string_printf(
                path_out,
                "%s/%s %u%s",
                MOBIB_DUMP_DIR,
                furi_string_get_cstr(name),
                n,
                MOBIB_DUMP_EXT);
        }
    } else {
        mobib_storage_format_pupi(&dump->card, name);
        furi_string_printf(
            path_out,
            "%s/%s_%04u%02u%02uT%02u%02u%02u%s",
            MOBIB_DUMP_DIR,
            furi_string_get_cstr(name),
            now->year, now->month, now->day,
            now->hour, now->minute, now->second,
            MOBIB_DUMP_EXT);
    }

    furi_string_free(name);
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

    /* Deep-scan secondary applications. */
    const uint32_t mpp = dump->has_mpp     ? 1 : 0;
    const uint32_t rt2 = dump->has_rt2     ? 1 : 0;
    const uint32_t et  = dump->has_eticket ? 1 : 0;
    if(!flipper_format_write_uint32(ff, "AppMpp",     &mpp, 1)) return false;
    if(!flipper_format_write_uint32(ff, "AppRt2",     &rt2, 1)) return false;
    if(!flipper_format_write_uint32(ff, "AppEticket", &et,  1)) return false;

    /* Deep-scan path-selected extras: stored as parallel keys so the
     * loader can iterate without a registry. */
    const uint32_t ex_count = dump->extra_count;
    if(!flipper_format_write_uint32(ff, "ExtraFiles", &ex_count, 1)) return false;
    for(size_t i = 0; i < dump->extra_count; ++i) {
        const MobibExtraFile* e = &dump->extras[i];
        char k[24];

        snprintf(k, sizeof(k), "Ex%02zu_Id", i);
        const uint32_t fid = e->file_id;
        if(!flipper_format_write_uint32(ff, k, &fid, 1)) return false;

        snprintf(k, sizeof(k), "Ex%02zu_Label", i);
        if(!flipper_format_write_string_cstr(ff, k, e->label)) return false;

        snprintf(k, sizeof(k), "Ex%02zu_Data", i);
        if(e->len > 0 && !flipper_format_write_hex(ff, k, e->data, e->len))
            return false;
    }

    /* Path-selected HOLDER_EXTENDED file. Persisted as a single hex blob
     * since the records have no schema other than concatenation. */
    if(dump->holder_ext_present) {
        const uint32_t one = 1;
        if(!flipper_format_write_uint32(ff, "HolderExtPresent", &one, 1)) return false;
        for(size_t i = 0; i < MOBIB_HOLDER_EXT_RECS; ++i) {
            if(dump->holder_ext_len[i] == 0) continue;
            char hkey[24];
            snprintf(hkey, sizeof(hkey), "HolderExt%u", (unsigned)(i + 1));
            if(!flipper_format_write_hex(
                   ff, hkey, dump->holder_ext[i], dump->holder_ext_len[i]))
                return false;
        }
    }

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
    mobib_storage_build_path(storage, dump, &now, path);

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

void mobib_storage_first_dump(FuriString* path_out) {
    furi_string_set_str(path_out, MOBIB_DUMP_DIR);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* dir = storage_file_alloc(storage);
    FuriString* best = furi_string_alloc();
    char name[128];

    if(storage_dir_open(dir, MOBIB_DUMP_DIR)) {
        FileInfo info;
        while(storage_dir_read(dir, &info, name, sizeof(name))) {
            if(file_info_is_dir(&info)) continue;
            const size_t n = strlen(name);
            const size_t e = strlen(MOBIB_DUMP_EXT);
            if(n <= e || strcmp(name + n - e, MOBIB_DUMP_EXT) != 0) continue;
            if(furi_string_empty(best) || strcasecmp(name, furi_string_get_cstr(best)) < 0) {
                furi_string_set_str(best, name);
            }
        }
    }
    storage_dir_close(dir);
    storage_file_free(dir);
    furi_record_close(RECORD_STORAGE);

    if(!furi_string_empty(best)) {
        furi_string_printf(path_out, "%s/%s", MOBIB_DUMP_DIR, furi_string_get_cstr(best));
    }
    furi_string_free(best);
}

/* ----------------------------------------------------------------- load */

static void load_card_info(FlipperFormat* ff, MobibCardInfo* card) {
    /* PUPI is stored as a variable-length hex array; read its length first. */
    uint32_t len = 0;
    if(flipper_format_get_value_count(ff, "PUPI", &len) && len > 0) {
        if(len > sizeof(card->pupi)) len = sizeof(card->pupi);
        if(flipper_format_read_hex(ff, "PUPI", card->pupi, len)) {
            card->pupi_len = len;
        }
    }
    /* Fixed-length fields. */
    flipper_format_read_hex(ff, "ApplicationData", card->app_data, sizeof(card->app_data));

    uint32_t v = 0;
    if(flipper_format_read_uint32(ff, "ISO14443_4", &v, 1)) card->supports_iso14443_4 = v != 0;
    if(flipper_format_read_uint32(ff, "FrameSizeMax", &v, 1)) card->frame_size_max = (uint16_t)v;
    if(flipper_format_read_uint32(ff, "FWTfcMax", &v, 1)) card->fwt_fc_max = v;
}

static bool load_records_loop(FlipperFormat* ff, MobibDump* dump, uint32_t count) {
    if(count > MOBIB_DUMP_RECORD_MAX) count = MOBIB_DUMP_RECORD_MAX;

    char key[24];
    for(uint32_t i = 0; i < count; ++i) {
        MobibRecord* r = &dump->records[dump->record_count];

        snprintf(key, sizeof(key), "Rec%02u_SFI", (unsigned)i);
        uint32_t v32 = 0;
        if(!flipper_format_read_uint32(ff, key, &v32, 1)) continue;
        r->sfi = (uint8_t)v32;

        snprintf(key, sizeof(key), "Rec%02u_Index", (unsigned)i);
        if(!flipper_format_read_uint32(ff, key, &v32, 1)) continue;
        r->record = (uint8_t)v32;

        snprintf(key, sizeof(key), "Rec%02u_Data", (unsigned)i);
        uint32_t dlen = 0;
        if(!flipper_format_get_value_count(ff, key, &dlen)) continue;
        if(dlen > sizeof(r->data)) dlen = sizeof(r->data);
        if(!flipper_format_read_hex(ff, key, r->data, dlen)) continue;
        r->len = (uint8_t)dlen;

        dump->record_count++;
    }
    return true;
}

bool mobib_storage_load_dump(const char* path, MobibDump* dump) {
    furi_assert(path);
    furi_assert(dump);

    memset(dump, 0, sizeof(*dump));

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);

    bool ok = false;
    do {
        if(!flipper_format_buffered_file_open_existing(ff, path)) break;

        FuriString* type = furi_string_alloc();
        uint32_t version = 0;
        const bool header_ok = flipper_format_read_header(ff, type, &version);
        furi_string_free(type);
        if(!header_ok) break;

        load_card_info(ff, &dump->card);

        /* Reads must follow the same order as writes in mobib_storage_write_*.
         * The save sequence is: card info -> CalypsoSelected -> CalypsoFCI (if
         * selected) -> Records -> per-record fields -> HolderExtPresent ->
         * HolderExt1/2 (HolderExt block sits between FCI and Records in the
         * current writer; verify there before changing). */
        uint32_t selected = 0;
        if(flipper_format_read_uint32(ff, "CalypsoSelected", &selected, 1)) {
            dump->calypso_selected = selected != 0;
        }

        if(dump->calypso_selected) {
            uint32_t fci_len = 0;
            if(flipper_format_get_value_count(ff, "CalypsoFCI", &fci_len) &&
               fci_len > 0) {
                if(fci_len > sizeof(dump->fci)) fci_len = sizeof(dump->fci);
                if(flipper_format_read_hex(ff, "CalypsoFCI", dump->fci, fci_len)) {
                    dump->fci_len = fci_len;
                }
            }
        }

        /* Records count is written *before* HolderExt; the per-record
         * fields (Rec00_SFI etc.) come after. */
        uint32_t rec_count = 0;
        flipper_format_read_uint32(ff, "Records", &rec_count, 1);

        /* Deep-scan apps. Ignore failures — older dumps predate them. */
        uint32_t flag = 0;
        if(flipper_format_read_uint32(ff, "AppMpp",     &flag, 1)) dump->has_mpp     = flag != 0;
        if(flipper_format_read_uint32(ff, "AppRt2",     &flag, 1)) dump->has_rt2     = flag != 0;
        if(flipper_format_read_uint32(ff, "AppEticket", &flag, 1)) dump->has_eticket = flag != 0;

        /* Deep-scan extras — optional block. */
        uint32_t ex_count = 0;
        if(flipper_format_read_uint32(ff, "ExtraFiles", &ex_count, 1)) {
            if(ex_count > MOBIB_EXTRA_FILES) ex_count = MOBIB_EXTRA_FILES;
            for(uint32_t i = 0; i < ex_count; ++i) {
                MobibExtraFile* e = &dump->extras[dump->extra_count];
                char k[24];

                snprintf(k, sizeof(k), "Ex%02u_Id", (unsigned)i);
                uint32_t fid = 0;
                if(!flipper_format_read_uint32(ff, k, &fid, 1)) continue;
                e->file_id = (uint16_t)fid;

                snprintf(k, sizeof(k), "Ex%02u_Label", (unsigned)i);
                FuriString* lab = furi_string_alloc();
                if(flipper_format_read_string(ff, k, lab)) {
                    const size_t n = furi_string_size(lab);
                    const size_t cap = sizeof(e->label) - 1;
                    const size_t c = n < cap ? n : cap;
                    memcpy(e->label, furi_string_get_cstr(lab), c);
                    e->label[c] = '\0';
                }
                furi_string_free(lab);

                snprintf(k, sizeof(k), "Ex%02u_Data", (unsigned)i);
                uint32_t dl = 0;
                if(flipper_format_get_value_count(ff, k, &dl) && dl > 0) {
                    if(dl > sizeof(e->data)) dl = sizeof(e->data);
                    if(flipper_format_read_hex(ff, k, e->data, dl)) {
                        e->len = (uint8_t)dl;
                    }
                }
                dump->extra_count++;
            }
        }

        /* HolderExt1 / HolderExt2 — best effort. */
        uint32_t ext_present = 0;
        if(flipper_format_read_uint32(ff, "HolderExtPresent", &ext_present, 1) &&
           ext_present) {
            for(size_t i = 0; i < MOBIB_HOLDER_EXT_RECS; ++i) {
                char hkey[16];
                snprintf(hkey, sizeof(hkey), "HolderExt%u", (unsigned)(i + 1));
                uint32_t l = 0;
                if(!flipper_format_get_value_count(ff, hkey, &l)) continue;
                if(l > MOBIB_HOLDER_EXT_REC_SZ) l = MOBIB_HOLDER_EXT_REC_SZ;
                if(flipper_format_read_hex(ff, hkey, dump->holder_ext[i], l)) {
                    dump->holder_ext_len[i] = (uint8_t)l;
                    dump->holder_ext_present = true;
                }
            }
        }

        ok = load_records_loop(ff, dump, rec_count);
    } while(0);

    flipper_format_buffered_file_close(ff);
    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);
    return ok;
}
