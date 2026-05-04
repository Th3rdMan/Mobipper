/*
 * MOBIB — Section formatters. See `mobib_format.h`.
 *
 * Aims for "screenshot-grade" clarity: each section opens with a bold
 * `===` divider, fields are aligned, dates are normalised, and unknown
 * data is labelled rather than dumped raw whenever we can avoid it.
 */

#include "mobib_format.h"

#include "../calypso/calypso_fci.h"
#include "../calypso/calypso_env.h"
#include "../calypso/calypso_event.h"
#include "../calypso/calypso_contract.h"
#include "../calypso/calypso_stations.h"
#include "../calypso/calypso_sfi.h"

#include <inttypes.h>

const char* mobib_section_title(MobibSection s) {
    switch(s) {
    case MobibSectionOverview:  return "Overview";
    case MobibSectionHolder:    return "Holder";
    case MobibSectionContracts: return "Contracts";
    case MobibSectionJourneys:  return "Journeys";
    case MobibSectionRecords:   return "Records";
    case MobibSectionFci:       return "FCI";
    default:                    return "?";
    }
}

/* ------------------------------- helpers ----------------------------- */

static void append_divider(FuriString* s, const char* title) {
    furi_string_cat_printf(s, "=== %s ===\n\n", title);
}

static void append_pupi_compact(FuriString* s, const MobibCardInfo* c) {
    for(size_t i = 0; i < c->pupi_len; ++i) {
        furi_string_cat_printf(s, "%02X", c->pupi[i]);
    }
}

static const MobibRecord* find_record(const MobibDump* d, uint8_t sfi, uint8_t rec) {
    for(size_t i = 0; i < d->record_count; ++i) {
        if(d->records[i].sfi == sfi && d->records[i].record == rec) return &d->records[i];
    }
    return NULL;
}

static bool parse_env(const MobibDump* d, CalypsoEnvironment* env) {
    const MobibRecord* r = find_record(d, 0x07, 1);
    if(!r) return false;
    return calypso_env_parse(r->data, r->len, env);
}

/* ----------------------------- Overview ----------------------------- */

static void format_overview(const MobibDump* d, FuriString* s) {
    append_divider(s, "MOBIB CARD");

    furi_string_cat_str(s, "Card N\xc2\xb0  ");
    append_pupi_compact(s, &d->card);
    furi_string_cat_str(s, "\n\n");

    CalypsoEnvironment env;
    bool have_env = parse_env(d, &env);
    if(have_env) {
        furi_string_cat_printf(
            s, "Issued by\n  %s\n",
            env.country_name ? env.country_name : "?");
        furi_string_cat_printf(
            s, "Network\n  %s\n",
            env.network_name ? env.network_name : "?");
        furi_string_cat_printf(s, "App version  %u\n", env.version);
        if(env.validity_end_year) {
            furi_string_cat_printf(
                s, "Valid until\n  %04u-%02u-%02u\n",
                env.validity_end_year, env.validity_end_month, env.validity_end_day);
        }
        furi_string_cat_str(s, "\n");
    }

    /* Counts */
    size_t contracts = 0, journeys = 0;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi == 0x09) {
            CalypsoContract c;
            if(calypso_contract_parse(r->data, r->len, &c)) contracts++;
        } else if(r->sfi == 0x17) {
            CalypsoEvent e;
            if(calypso_event_parse(r->data, r->len, &e)) journeys++;
        }
    }
    furi_string_cat_printf(s, "%zu contract%s\n", contracts, contracts == 1 ? "" : "s");
    furi_string_cat_printf(s, "%zu journey%s\n", journeys, journeys == 1 ? "" : "s");
    furi_string_cat_printf(s, "%zu record%s on card\n",
        d->record_count, d->record_count == 1 ? "" : "s");

    if(d->calypso_selected) {
        CalypsoFci fci;
        if(calypso_fci_parse(d->fci, d->fci_len, &fci) && fci.app_serial_len > 0) {
            furi_string_cat_str(s, "\nApp serial\n  ");
            for(size_t i = 0; i < fci.app_serial_len; ++i) {
                furi_string_cat_printf(s, "%02X", fci.app_serial[i]);
            }
            furi_string_cat_str(s, "\n");
        }
    }
}

/* ------------------------------ Holder ------------------------------ */

static void format_holder(const MobibDump* d, FuriString* s) {
    append_divider(s, "HOLDER");

    CalypsoEnvironment env;
    if(!parse_env(d, &env)) {
        furi_string_cat_str(s, "Environment record\nis not readable.\n");
        return;
    }

    const bool any_birth =
        env.birth_year_top2 || env.birth_year_bot2 ||
        env.birth_month_bcd || env.birth_day_bcd;
    const bool any_holder = any_birth || env.holder_postal_code;

    if(!any_holder) {
        furi_string_cat_str(s, "Anonymous card.\n\n");
        furi_string_cat_str(s, "MOBIB Basic carries\n");
        furi_string_cat_str(s, "no personal data.\n");
        furi_string_cat_str(s, "Use the trip log to\n");
        furi_string_cat_str(s, "see what was done.\n");
        return;
    }

    if(any_birth) {
        furi_string_cat_printf(
            s, "Born  %02X%02X-%02X-%02X\n",
            env.birth_year_top2, env.birth_year_bot2,
            env.birth_month_bcd, env.birth_day_bcd);
    }
    if(env.holder_postal_code) {
        furi_string_cat_printf(s, "Postal code  %u\n", env.holder_postal_code);
    }

    furi_string_cat_str(s, "\nName and gender are\n");
    furi_string_cat_str(s, "stored separately\n");
    furi_string_cat_str(s, "(HOLDER_EXTENDED at\n");
    furi_string_cat_str(s, "ISO file 0x3F1C);\n");
    furi_string_cat_str(s, "this app does not\n");
    furi_string_cat_str(s, "yet read it.\n");
}

/* ----------------------------- Contracts ---------------------------- */

static void format_contracts(const MobibDump* d, FuriString* s) {
    append_divider(s, "CONTRACTS");

    size_t shown = 0;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi != 0x09) continue;
        CalypsoContract c;
        if(!calypso_contract_parse(r->data, r->len, &c)) continue;

        shown++;
        furi_string_cat_printf(s, "Slot %u\n", (unsigned)r->record);

        const char* tname = calypso_contract_tariff_name(c.tariff);
        if(tname) {
            furi_string_cat_printf(s, "  %s\n", tname);
        } else {
            furi_string_cat_printf(s, "  Tariff 0x%04X\n", c.tariff);
        }

        if(c.flags & CALYPSO_CONTRACT_HAS_SALE && c.sale_year) {
            furi_string_cat_printf(s, "  Sold %04u-%02u-%02u\n",
                c.sale_year, c.sale_month, c.sale_day);
        }
        if(c.flags & CALYPSO_CONTRACT_HAS_DURATION) {
            const char* unit = "?";
            switch(c.duration_units) {
            case 0: unit = "days";   break;
            case 1: unit = "weeks";  break;
            case 2: unit = "months"; break;
            }
            furi_string_cat_printf(s, "  Length %u %s\n", c.duration, unit);
        }
        if(c.flags & CALYPSO_CONTRACT_HAS_PRICE && c.price_amount) {
            furi_string_cat_printf(s, "  Price %u\n", c.price_amount);
        }
        furi_string_cat_str(s, "\n");
    }
    if(shown == 0) furi_string_cat_str(s, "No active contracts.\n");
}

/* ----------------------------- Journeys ----------------------------- */

static int compare_events_desc(const CalypsoEvent* a, const CalypsoEvent* b) {
    /* Sort by (date, time) descending. */
    if(a->event_date_days != b->event_date_days)
        return a->event_date_days > b->event_date_days ? -1 : 1;
    if(a->event_time_minutes != b->event_time_minutes)
        return a->event_time_minutes > b->event_time_minutes ? -1 : 1;
    return 0;
}

static void format_journeys(const MobibDump* d, FuriString* s) {
    append_divider(s, "JOURNEYS");

    /* Collect all parseable events. */
    CalypsoEvent events[MOBIB_DUMP_RECORD_MAX];
    size_t count = 0;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi != 0x17) continue;
        if(count >= sizeof(events) / sizeof(events[0])) break;
        if(!calypso_event_parse(r->data, r->len, &events[count])) continue;
        count++;
    }

    /* Insertion sort, descending. Tiny list (≤ ~10) — O(n²) is fine. */
    for(size_t i = 1; i < count; ++i) {
        CalypsoEvent tmp = events[i];
        size_t j = i;
        while(j > 0 && compare_events_desc(&tmp, &events[j - 1]) < 0) {
            events[j] = events[j - 1];
            --j;
        }
        events[j] = tmp;
    }

    if(count == 0) {
        furi_string_cat_str(s, "No journeys logged.\n");
        return;
    }

    for(size_t i = 0; i < count; ++i) {
        const CalypsoEvent* e = &events[i];

        /* Date line. */
        if(e->event_year) {
            furi_string_cat_printf(
                s, "%04u-%02u-%02u  %02u:%02u\n",
                e->event_year, e->event_month, e->event_day,
                e->event_hour, e->event_minute);
        } else {
            furi_string_cat_str(s, "(date invalid)\n");
        }

        /* Provider + location line. */
        if(e->flags & CALYPSO_EVENT_HAS_PROVIDER) {
            const char* prov = calypso_event_provider_name(e->service_provider);
            furi_string_cat_printf(s, "  %s",
                prov ? prov : "Unknown");

            if(e->service_provider == CALYPSO_PROVIDER_METRO ||
               e->service_provider == CALYPSO_PROVIDER_PREMETRO) {
                const CalypsoMetroStation* st =
                    calypso_metro_station_lookup_id(e->location_id);
                if(st) {
                    furi_string_cat_printf(s, " %s\n  %s\n", st->line, st->name);
                } else {
                    furi_string_cat_printf(s, "\n  loc %lu\n",
                        (unsigned long)e->location_id);
                }
            } else if(e->flags & CALYPSO_EVENT_HAS_ROUTE) {
                furi_string_cat_printf(s, " r%u\n", e->route_number);
            } else {
                furi_string_cat_str(s, "\n");
            }
        }

        if(e->flags & CALYPSO_EVENT_HAS_SERIAL) {
            furi_string_cat_printf(s, "  trip #%lu\n",
                (unsigned long)e->serial_number);
        }
        furi_string_cat_str(s, "\n");
    }
}

/* ------------------------------ Records ----------------------------- */

static void format_records(const MobibDump* d, FuriString* s) {
    furi_string_cat_printf(s, "=== RECORDS (%zu) ===\n\n", d->record_count);

    /* Group by SFI; emit a header when the SFI changes. */
    uint8_t last_sfi = 0xFF;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];

        if(r->sfi != last_sfi) {
            const char* lab = calypso_sfi_label(r->sfi);
            if(lab) {
                furi_string_cat_printf(s, "%s (SFI %02X)\n", lab, r->sfi);
            } else {
                furi_string_cat_printf(s, "SFI %02X\n", r->sfi);
            }
            last_sfi = r->sfi;
        }

        furi_string_cat_printf(s, "  r%u (%uB)\n   ", r->record, r->len);

        /* Detect all-zero record and label it instead of dumping. */
        bool all_zero = true;
        for(size_t b = 0; b < r->len; ++b) if(r->data[b]) { all_zero = false; break; }

        if(all_zero) {
            furi_string_cat_str(s, "(empty)\n");
        } else {
            const size_t n = r->len < 16 ? r->len : 16;
            for(size_t b = 0; b < n; ++b) {
                furi_string_cat_printf(s, "%02X", r->data[b]);
                if((b + 1) % 4 == 0 && b + 1 < n) furi_string_cat_str(s, " ");
            }
            if(r->len > 16) furi_string_cat_str(s, "...");
            furi_string_cat_str(s, "\n");
        }
    }
}

/* -------------------------------- FCI -------------------------------- */

static void format_fci(const MobibDump* d, FuriString* s) {
    append_divider(s, "FCI");

    if(!d->calypso_selected || d->fci_len == 0) {
        furi_string_cat_str(s, "Not available.\n");
        return;
    }

    CalypsoFci fci;
    if(calypso_fci_parse(d->fci, d->fci_len, &fci) && fci.valid) {
        furi_string_cat_str(s, "AID  ");
        if(fci.is_calypso_aid) {
            furi_string_cat_str(s, "1TIC.ICA\n");
            if(fci.is_mobib_extension) {
                furi_string_cat_str(s, "      (MOBIB family)\n");
            }
            if(fci.has_aid_extension) {
                furi_string_cat_str(s, "Ext  ");
                for(size_t i = 0; i < CALYPSO_AID_EXTENSION_LEN; ++i) {
                    furi_string_cat_printf(s, "%02X ", fci.aid_extension[i]);
                }
                furi_string_cat_str(s, "\n");
            }
        } else {
            for(size_t i = 0; i < fci.df_name_len; ++i) {
                furi_string_cat_printf(s, "%02X", fci.df_name[i]);
            }
            furi_string_cat_str(s, "\n");
        }
        if(fci.app_serial_len > 0) {
            furi_string_cat_str(s, "Ser  ");
            for(size_t i = 0; i < fci.app_serial_len; ++i) {
                furi_string_cat_printf(s, "%02X", fci.app_serial[i]);
            }
            furi_string_cat_str(s, "\n");
        }
        furi_string_cat_str(s, "\n");
    }

    furi_string_cat_printf(s, "Raw (%zu bytes)\n", d->fci_len);
    for(size_t i = 0; i < d->fci_len; ++i) {
        furi_string_cat_printf(s, "%02X", d->fci[i]);
        if((i + 1) % 8 == 0) furi_string_cat_str(s, "\n");
        else if(i + 1 < d->fci_len) furi_string_cat_str(s, " ");
    }
    if(d->fci_len % 8 != 0) furi_string_cat_str(s, "\n");
}

/* ------------------------------- public ----------------------------- */

void mobib_format_section(
    MobibSection      section,
    const MobibDump*  dump,
    FuriString*       out) {
    furi_string_reset(out);
    if(!dump) {
        furi_string_set_str(out, "No card loaded.\n");
        return;
    }

    switch(section) {
    case MobibSectionOverview:  format_overview (dump, out); break;
    case MobibSectionHolder:    format_holder   (dump, out); break;
    case MobibSectionContracts: format_contracts(dump, out); break;
    case MobibSectionJourneys:  format_journeys (dump, out); break;
    case MobibSectionRecords:   format_records  (dump, out); break;
    case MobibSectionFci:       format_fci      (dump, out); break;
    default: break;
    }
}
