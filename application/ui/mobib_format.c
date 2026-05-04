/*
 * MOBIB — Section formatters. See `mobib_format.h`.
 *
 * Each formatter lives behind a uniform `(section, dump, out)` interface
 * so the UI scenes can stay dumb. Decoders are reused as-is from the
 * `calypso` modules — formatting is purely a presentation concern.
 */

#include "mobib_format.h"

#include "../calypso/calypso_fci.h"
#include "../calypso/calypso_env.h"
#include "../calypso/calypso_event.h"
#include "../calypso/calypso_contract.h"
#include "../calypso/calypso_stations.h"

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

static void append_hex(FuriString* s, const uint8_t* buf, size_t len, char sep) {
    for(size_t i = 0; i < len; ++i) {
        furi_string_cat_printf(s, "%02X", buf[i]);
        if(sep && i + 1 < len) furi_string_cat_printf(s, "%c", sep);
    }
}

static void append_pupi(FuriString* s, const MobibCardInfo* c) {
    furi_string_cat_str(s, "PUPI: ");
    append_hex(s, c->pupi, c->pupi_len, 0);
    furi_string_cat_str(s, "\n");
}

/* ----------------------------- Overview ----------------------------- */

static void format_overview(const MobibDump* d, FuriString* s) {
    append_pupi(s, &d->card);

    if(d->calypso_selected && d->fci_len > 0) {
        CalypsoFci fci;
        if(calypso_fci_parse(d->fci, d->fci_len, &fci) && fci.valid) {
            if(fci.is_calypso_aid) {
                furi_string_cat_str(s, "AID: 1TIC.ICA");
                if(fci.is_mobib_extension) furi_string_cat_str(s, " (MOBIB)");
                furi_string_cat_str(s, "\n");
            }
            if(fci.app_serial_len > 0) {
                furi_string_cat_str(s, "Serial: ");
                append_hex(s, fci.app_serial, fci.app_serial_len, 0);
                furi_string_cat_str(s, "\n");
            }
        }
    }

    /* Find SFI 7 record 1 → Environment. */
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi != 0x07 || r->record != 1) continue;

        CalypsoEnvironment env;
        if(!calypso_env_parse(r->data, r->len, &env)) break;

        furi_string_cat_printf(s, "Country: %s\n",
            env.country_name ? env.country_name : "?");
        furi_string_cat_printf(s, "  code  : %u\n", env.country_code);

        furi_string_cat_printf(s, "Network: %s\n",
            env.network_name ? env.network_name : "?");
        furi_string_cat_printf(s, "  id    : 0x%03X\n", env.network_id);

        furi_string_cat_printf(s, "Version: %u\n", env.version);

        if(env.validity_end_year) {
            furi_string_cat_printf(s, "Expires: %04u-%02u-%02u\n",
                env.validity_end_year, env.validity_end_month, env.validity_end_day);
        }
        break;
    }

    /* Counters at the bottom. */
    size_t contract_records = 0, journey_records = 0;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi == 0x09) {
            CalypsoContract c;
            if(calypso_contract_parse(r->data, r->len, &c)) contract_records++;
        } else if(r->sfi == 0x17) {
            CalypsoEvent e;
            if(calypso_event_parse(r->data, r->len, &e)) journey_records++;
        }
    }
    furi_string_cat_printf(s, "Contracts: %u\n", (unsigned)contract_records);
    furi_string_cat_printf(s, "Journeys : %u\n", (unsigned)journey_records);
    furi_string_cat_printf(s, "Records  : %u\n", (unsigned)d->record_count);
}

/* ------------------------------ Holder ------------------------------ */

static void format_holder(const MobibDump* d, FuriString* s) {
    bool found = false;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi != 0x07 || r->record != 1) continue;
        CalypsoEnvironment env;
        if(!calypso_env_parse(r->data, r->len, &env)) break;
        found = true;

        const bool any_birth = env.birth_year_top2 || env.birth_year_bot2 ||
                               env.birth_month_bcd || env.birth_day_bcd;

        if(!any_birth && !env.holder_postal_code) {
            furi_string_cat_str(s, "Anonymous card.\n");
            furi_string_cat_str(s, "No holder data is\n");
            furi_string_cat_str(s, "stored on MOBIB Basic.\n\n");
        }

        if(any_birth) {
            furi_string_cat_printf(
                s, "Birth: %02X%02X-%02X-%02X\n",
                env.birth_year_top2, env.birth_year_bot2,
                env.birth_month_bcd, env.birth_day_bcd);
        }
        if(env.holder_postal_code) {
            furi_string_cat_printf(s, "Postal code: %u\n", env.holder_postal_code);
        }
        break;
    }
    if(!found) furi_string_cat_str(s, "Environment record absent.\n");

    furi_string_cat_str(s, "\nNote: holder name and\n");
    furi_string_cat_str(s, "gender live in the\n");
    furi_string_cat_str(s, "HOLDER_EXTENDED file\n");
    furi_string_cat_str(s, "(path 0x3F1C) which is\n");
    furi_string_cat_str(s, "selected by file path,\n");
    furi_string_cat_str(s, "not by SFI; not yet\n");
    furi_string_cat_str(s, "implemented.\n");
}

/* ----------------------------- Contracts ---------------------------- */

static void format_contracts(const MobibDump* d, FuriString* s) {
    size_t shown = 0;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi != 0x09) continue;
        CalypsoContract c;
        if(!calypso_contract_parse(r->data, r->len, &c)) continue;

        shown++;
        furi_string_cat_printf(s, "Slot %u (v%u)\n", (unsigned)r->record, c.version);
        const char* tname = calypso_contract_tariff_name(c.tariff);
        if(tname) {
            furi_string_cat_printf(s, "  Tariff: %s\n", tname);
        } else {
            furi_string_cat_printf(s, "  Tariff: 0x%04X\n", c.tariff);
        }
        if(c.flags & CALYPSO_CONTRACT_HAS_SALE && c.sale_year) {
            furi_string_cat_printf(s, "  Sold  : %04u-%02u-%02u\n",
                c.sale_year, c.sale_month, c.sale_day);
        }
        if(c.flags & CALYPSO_CONTRACT_HAS_DURATION) {
            const char* unit = "?";
            switch(c.duration_units) {
            case 0: unit = "d"; break;
            case 1: unit = "w"; break;
            case 2: unit = "mo"; break;
            }
            furi_string_cat_printf(s, "  Length: %u%s\n", c.duration, unit);
        }
        if(c.flags & CALYPSO_CONTRACT_HAS_PRICE) {
            furi_string_cat_printf(s, "  Price : %u\n", c.price_amount);
        }
        furi_string_cat_str(s, "\n");
    }
    if(shown == 0) furi_string_cat_str(s, "No active contracts.\n");
}

/* ----------------------------- Journeys ----------------------------- */

static void format_journeys(const MobibDump* d, FuriString* s) {
    size_t shown = 0;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi != 0x17) continue;
        CalypsoEvent e;
        if(!calypso_event_parse(r->data, r->len, &e)) continue;

        shown++;
        if(e.event_year) {
            furi_string_cat_printf(s, "%04u-%02u-%02u %02u:%02u\n",
                e.event_year, e.event_month, e.event_day,
                e.event_hour, e.event_minute);
        } else {
            furi_string_cat_printf(s, "(invalid date)\n");
        }

        if(e.flags & CALYPSO_EVENT_HAS_PROVIDER) {
            const char* prov = calypso_event_provider_name(e.service_provider);
            furi_string_cat_printf(s, "  %s",
                prov ? prov : "?");
            if(!prov) furi_string_cat_printf(s, " (0x%02X)", e.service_provider);
            furi_string_cat_str(s, "\n");

            if(e.service_provider == CALYPSO_PROVIDER_METRO ||
               e.service_provider == CALYPSO_PROVIDER_PREMETRO) {
                const CalypsoMetroStation* st =
                    calypso_metro_station_lookup_id(e.location_id);
                if(st) {
                    furi_string_cat_printf(s, "  Line %s\n", st->line);
                    furi_string_cat_printf(s, "  %s\n", st->name);
                } else {
                    furi_string_cat_printf(s, "  loc %lu\n",
                        (unsigned long)e.location_id);
                }
            } else if(e.flags & CALYPSO_EVENT_HAS_ROUTE) {
                furi_string_cat_printf(s, "  route %u\n", e.route_number);
            }
        }
        if(e.flags & CALYPSO_EVENT_HAS_SERIAL) {
            furi_string_cat_printf(s, "  #%lu\n", (unsigned long)e.serial_number);
        }
        furi_string_cat_str(s, "\n");
    }
    if(shown == 0) furi_string_cat_str(s, "No journeys recorded.\n");
}

/* ------------------------------ Records ----------------------------- */

static void format_records(const MobibDump* d, FuriString* s) {
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        furi_string_cat_printf(s, "SFI%02X r%u (%uB)\n", r->sfi, r->record, r->len);
        const size_t n = r->len < 24 ? r->len : 24;
        for(size_t b = 0; b < n; ++b) {
            furi_string_cat_printf(s, "%02X", r->data[b]);
            if(b + 1 < n && (b + 1) % 8 == 0) furi_string_cat_str(s, "\n");
            else if(b + 1 < n)                 furi_string_cat_str(s, " ");
        }
        if(r->len > 24) furi_string_cat_str(s, "...");
        furi_string_cat_str(s, "\n\n");
    }
}

/* -------------------------------- FCI -------------------------------- */

static void format_fci(const MobibDump* d, FuriString* s) {
    if(!d->calypso_selected || d->fci_len == 0) {
        furi_string_cat_str(s, "FCI not available.\n");
        return;
    }
    furi_string_cat_printf(s, "%u bytes\n\n", (unsigned)d->fci_len);
    for(size_t i = 0; i < d->fci_len; ++i) {
        furi_string_cat_printf(s, "%02X", d->fci[i]);
        if((i + 1) % 8 == 0)      furi_string_cat_str(s, "\n");
        else                       furi_string_cat_str(s, " ");
    }
    furi_string_cat_str(s, "\n");
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
