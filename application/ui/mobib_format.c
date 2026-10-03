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
#include "../calypso/calypso_bus.h"
#include "../calypso/calypso_sfi.h"
#include "../calypso/calypso_holder.h"
#include "../storage/mobib_storage.h"

#include "../calypso/calypso_date.h"

#include <furi_hal_rtc.h>
#include <inttypes.h>
#include <string.h>

const char* mobib_section_title(MobibSection s) {
    switch(s) {
    case MobibSectionOverview:
        return "Resume";
    case MobibSectionHolder:
        return "Titulaire";
    case MobibSectionContracts:
        return "Abonnements";
    case MobibSectionJourneys:
        return "Trajets";
    case MobibSectionRecords:
        return "Enregistrements";
    case MobibSectionFci:
        return "FCI";
    case MobibSectionDeep:
        return "Analyse avancee";
    default:
        return "?";
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

/* ----------------------------- Contracts ---------------------------- */

static uint8_t days_in_month(uint16_t year, uint8_t month) {
    static const uint8_t kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if(month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) return 29;
    return kDays[month - 1];
}

/* Last valid day of a time-based contract: sale date + duration - 1 day.
 * Duration units: 0 = days, 1 = weeks, 2 and 3 = months. Units 3 is what
 * real 2023-2026 STIB passes carry (49 EUR for 1 = monthly pass, 12 for
 * the yearly one). Returns false for trip tickets and undated contracts. */
static bool contract_end(const CalypsoContract* c, uint16_t* y, uint8_t* m, uint8_t* d) {
    if(!(c->flags & CALYPSO_CONTRACT_HAS_SALE) || !c->sale_year) return false;
    if(!(c->flags & CALYPSO_CONTRACT_HAS_DURATION) || !c->duration) return false;
    if(c->tariff == CALYPSO_TARIFF_JUMP_1_TRIP || c->tariff == CALYPSO_TARIFF_JUMP_10_TRIPS)
        return false;

    if(c->duration_units <= 1) {
        const uint32_t days = (uint32_t)c->duration * (c->duration_units ? 7 : 1);
        return calypso_date_from_days((uint16_t)(c->sale_days + days - 1), y, m, d);
    }

    uint32_t months = (uint32_t)c->sale_month - 1 + c->duration;
    *y = (uint16_t)(c->sale_year + months / 12);
    *m = (uint8_t)(months % 12 + 1);
    const uint8_t dim = days_in_month(*y, *m);
    *d = c->sale_day > dim ? dim : c->sale_day;
    /* minus one day */
    if(*d > 1) {
        (*d)--;
    } else {
        if(*m == 1) {
            *m = 12;
            (*y)--;
        } else {
            (*m)--;
        }
        *d = days_in_month(*y, *m);
    }
    return true;
}

/* Remaining trips of the contract in slot `slot` (1-based), per metrodroid
 * Calypso1545TransitData.getCounter + MobibSubscription: slots 1..4 only,
 * 24-bit counter taken from the shared counter file (SFI 19, 3 bytes per
 * slot) or else from the slot's own file (SFI 0A..0D). Contracts whose
 * tariff bits 10-12 equal 4 do not count trips. Time-based passes also
 * carry a counter (0 on real 2026 STIB monthly passes): only trip tickets
 * (known Jump tariffs, or contracts without a duration) are reported. */
static bool contract_trips_left(
    const MobibDump* d,
    const CalypsoContract* c,
    uint8_t slot,
    uint32_t* trips) {
    if(slot < 1 || slot > 4) return false;
    if(((c->tariff >> 10) & 7) == 4) return false;
    const bool trip_ticket = c->tariff == CALYPSO_TARIFF_JUMP_1_TRIP ||
                             c->tariff == CALYPSO_TARIFF_JUMP_10_TRIPS;
    if(!trip_ticket && (c->flags & CALYPSO_CONTRACT_HAS_DURATION) && c->duration) return false;

    const MobibRecord* r = find_record(d, 0x19, 1);
    size_t off = 3u * (slot - 1);
    if(!r || r->len < off + 3) {
        r = find_record(d, (uint8_t)(0x0A + slot - 1), 1);
        off = 0;
        if(!r || r->len < 3) return false;
    }
    *trips = ((uint32_t)r->data[off] << 16) | ((uint32_t)r->data[off + 1] << 8) | r->data[off + 2];
    return true;
}

/* ----------------------------- Journeys ----------------------------- */

typedef struct {
    bool has_route;
    bool is_metro;
    const char* mode; /**< "Metro", "Bus", "Tram" or NULL. */
    const CalypsoBusStop* stop;
    const CalypsoMetroStation* station;
} JourneyInfo;

static void journey_describe(const CalypsoEvent* e, JourneyInfo* j) {
    memset(j, 0, sizeof(*j));
    const bool has_provider = e->flags & CALYPSO_EVENT_HAS_PROVIDER;
    j->has_route = (e->flags & CALYPSO_EVENT_HAS_ROUTE) && e->route_number;
    /* zoobab: METRO = 0. Confirmed on real 2024-2026 v3 cards (metro
     * lines 1/2/6 with metro station location ids). Metro events also
     * carry a LOCATION_ID_BUS value, but it is not a STIB stop id. */
    j->is_metro = has_provider && e->service_provider == CALYPSO_PROVIDER_METRO;

    /* Resolve the place: STIB stop for surface lines, metro station
     * (zone / sub-zone / station) otherwise. */
    if(!j->is_metro && e->flags & CALYPSO_EVENT_HAS_LOCATION_BUS && e->location_id_bus &&
       j->has_route) {
        /* zoobab/mobib-extractor: bus line = low 7 bits of ROUTE_NUMBER. */
        j->stop = calypso_bus_stop_lookup(e->route_number & 0x7F, e->location_id_bus);
    }
    if(!j->stop && has_provider) {
        j->station = calypso_metro_station_lookup_id(e->location_id);
    }

    if(j->is_metro || j->station) {
        j->mode = "Metro";
    } else if(has_provider) {
        j->mode = calypso_event_provider_name(e->service_provider);
    }
}

/* "Metro 2", "Bus 71", "Ligne 12"... Returns false if nothing is known. */
static bool append_journey_mode(FuriString* s, const CalypsoEvent* e, const JourneyInfo* j) {
    if(!j->mode && !j->has_route) return false;
    furi_string_cat_str(s, j->mode ? j->mode : "Ligne");
    if(j->has_route) {
        furi_string_cat_printf(s, " %u", e->route_number & 0x7F);
    } else if(j->station) {
        furi_string_cat_printf(s, " %s", j->station->line);
    }
    return true;
}

static const char* journey_place_name(const JourneyInfo* j) {
    if(j->stop) return j->stop->name;
    if(j->station) return j->station->name;
    return NULL;
}

static int compare_events_desc(const CalypsoEvent* a, const CalypsoEvent* b) {
    /* Sort by (date, time) descending. */
    if(a->event_date_days != b->event_date_days)
        return a->event_date_days > b->event_date_days ? -1 : 1;
    if(a->event_time_minutes != b->event_time_minutes)
        return a->event_time_minutes > b->event_time_minutes ? -1 : 1;
    return 0;
}

/* ----------------------------- Overview ----------------------------- */

static void format_overview(const MobibDump* d, FuriString* s) {
    append_divider(s, "CARTE MOBIB");

    CalypsoEnvironment env;
    const bool have_env = parse_env(d, &env);

    /* Holder block, when the card is personalised: name, commune, birth. */
    CalypsoHolder holder;
    const bool have_holder = mobib_dump_holder(d, &holder);
    bool holder_block = false;
    FuriString* tmp = furi_string_alloc();
    if(have_holder && mobib_holder_display_name(&holder, tmp)) {
        furi_string_cat_printf(s, "%s\n", furi_string_get_cstr(tmp));
        holder_block = true;
    }
    if(have_env && env.holder_postal_code) {
        if(mobib_postal_lookup(env.holder_postal_code, tmp)) {
            furi_string_cat_printf(
                s, "%u %s\n", env.holder_postal_code, furi_string_get_cstr(tmp));
        } else {
            furi_string_cat_printf(s, "%u\n", env.holder_postal_code);
        }
        holder_block = true;
    }
    furi_string_free(tmp);
    if(have_holder && (holder.birth_year_top2 || holder.birth_year_bot2 || holder.birth_month ||
                       holder.birth_day)) {
        /* BCD packed YYYYMMDD: %02X prints the decimal digits. */
        furi_string_cat_printf(
            s,
            "%s le %02X/%02X/%02X%02X\n",
            holder.gender == 2 ? "Nee" : "Ne",
            holder.birth_day,
            holder.birth_month,
            holder.birth_year_top2,
            holder.birth_year_bot2);
        holder_block = true;
    }
    if(holder_block) furi_string_cat_str(s, "\n");

    /* Card block. */
    if(have_env) {
        if(env.network_name) {
            furi_string_cat_printf(s, "Reseau %s\n", env.network_name);
        } else {
            furi_string_cat_printf(s, "Reseau 0x%03X\n", env.network_id);
        }
    }
    furi_string_cat_str(s, "Carte n. ");
    append_pupi_compact(s, &d->card);
    furi_string_cat_str(s, "\n");
    if(have_env && env.validity_end_year) {
        furi_string_cat_printf(
            s,
            "Valable jusqu'au %02u/%02u/%04u\n",
            env.validity_end_day,
            env.validity_end_month,
            env.validity_end_year);
    }
    furi_string_cat_str(s, "\n");

    /* Counts, latest subscription end and latest journey in one pass. */
    size_t contracts = 0, journeys = 0;
    uint32_t best_end = 0; /* YYYYMMDD */
    uint32_t trips_left = 0;
    bool have_trips = false;
    CalypsoEvent last;
    bool have_last = false;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi == 0x09) {
            CalypsoContract c;
            if(!calypso_contract_parse(r->data, r->len, &c)) continue;
            contracts++;
            uint32_t t;
            /* Used-up tickets stay on the card: only mention trips left. */
            if(contract_trips_left(d, &c, r->record, &t) && t) {
                trips_left += t;
                have_trips = true;
            }
            uint16_t y;
            uint8_t m, dd;
            if(contract_end(&c, &y, &m, &dd)) {
                const uint32_t end = (uint32_t)y * 10000 + m * 100 + dd;
                if(end > best_end) best_end = end;
            }
        } else if(r->sfi == 0x17) {
            CalypsoEvent e;
            if(!calypso_event_parse(r->data, r->len, &e)) continue;
            journeys++;
            if(!have_last || compare_events_desc(&e, &last) < 0) {
                last = e;
                have_last = true;
            }
        }
    }

    if(best_end) {
        DateTime now;
        furi_hal_rtc_get_datetime(&now);
        const uint32_t today = (uint32_t)now.year * 10000 + now.month * 100 + now.day;
        furi_string_cat_printf(
            s,
            "Abonnement %02lu/%02lu/%04lu%s\n",
            (unsigned long)(best_end % 100),
            (unsigned long)(best_end / 100 % 100),
            (unsigned long)(best_end / 10000),
            best_end < today ? " (echu)" : "");
    }
    if(have_trips) {
        furi_string_cat_printf(s, "Voyages restants : %lu\n", (unsigned long)trips_left);
    }

    if(have_last) {
        JourneyInfo j;
        journey_describe(&last, &j);
        const char* place = journey_place_name(&j);
        /* The label alone nearly fills the line: the stop goes below. */
        furi_string_cat_str(s, "Derniere utilisation :\n");
        if(place) furi_string_cat_printf(s, "  %s\n", place);
        if(last.event_year) {
            furi_string_cat_printf(
                s,
                "%02u/%02u %02u:%02u",
                last.event_day,
                last.event_month,
                last.event_hour,
                last.event_minute);
        }
        FuriString* mode = furi_string_alloc();
        if(append_journey_mode(mode, &last, &j)) {
            furi_string_cat_printf(s, " - %s", furi_string_get_cstr(mode));
        }
        furi_string_free(mode);
        furi_string_cat_str(s, "\n");
    }
    if(best_end || have_trips || have_last) furi_string_cat_str(s, "\n");

    furi_string_cat_printf(s, "%zu abonnement%s\n", contracts, contracts > 1 ? "s" : "");
    furi_string_cat_printf(s, "%zu trajet%s\n", journeys, journeys > 1 ? "s" : "");
}

/* ------------------------------ Holder ------------------------------ */

static void format_holder(const MobibDump* d, FuriString* s) {
    append_divider(s, "TITULAIRE");

    /* Try the rich source first: the path-selected HOLDER_EXTENDED file. */
    if(d->holder_ext_present) {
        CalypsoHolder h;
        if(mobib_dump_holder(d, &h)) {
            const char* gender = calypso_holder_gender_name(h.gender);
            const char* title = calypso_holder_gender_title(h.gender);

            if(h.gender == 0 && h.name_len == 0) {
                furi_string_cat_str(s, "Carte anonyme.\n");
                furi_string_cat_str(s, "(fichier titulaire\n present mais vide)\n");
            } else {
                FuriString* name = furi_string_alloc();
                if(mobib_holder_display_name(&h, name)) {
                    if(title) {
                        furi_string_cat_printf(s, "%s %s\n", title, furi_string_get_cstr(name));
                    } else {
                        furi_string_cat_printf(s, "%s\n", furi_string_get_cstr(name));
                    }
                }
                furi_string_free(name);
                if(gender) {
                    furi_string_cat_printf(s, "Sexe  %s\n", gender);
                }
                if(h.birth_year_top2 || h.birth_year_bot2 || h.birth_month || h.birth_day) {
                    /* Birth date is BCD packed YYYYMMDD; the hex digits of
                     * each byte ARE the decimal digits, so %02X reads as
                     * the human number. */
                    furi_string_cat_printf(
                        s,
                        "Ne(e) le  %02X/%02X/%02X%02X\n",
                        h.birth_day,
                        h.birth_month,
                        h.birth_year_top2,
                        h.birth_year_bot2);
                }
            }
            furi_string_cat_str(s, "\n");
        } else {
            furi_string_cat_str(s, "Fichier titulaire\nillisible.\n\n");
        }
    }

    /* Then add what the Environment record carries (postal code is here). */
    CalypsoEnvironment env;
    if(parse_env(d, &env) && env.holder_postal_code) {
        furi_string_cat_printf(s, "Code postal  %u\n", env.holder_postal_code);
    }

    if(!d->holder_ext_present) {
        furi_string_cat_str(s, "Carte anonyme.\n\n");
        furi_string_cat_str(s, "La MOBIB Basic ne\n");
        furi_string_cat_str(s, "contient pas de\n");
        furi_string_cat_str(s, "donnees titulaire.\n");
        furi_string_cat_str(s, "Les cartes nominatives\n");
        furi_string_cat_str(s, "(SNCB / TEC / De Lijn)\n");
        furi_string_cat_str(s, "exposent nom et sexe\n");
        furi_string_cat_str(s, "dans HOLDER_EXTENDED.\n");
    }
}

/* ----------------------------- Contracts ---------------------------- */

static void format_contracts(const MobibDump* d, FuriString* s) {
    append_divider(s, "ABONNEMENTS");

    size_t shown = 0;
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        if(r->sfi != 0x09) continue;
        CalypsoContract c;
        if(!calypso_contract_parse(r->data, r->len, &c)) continue;

        shown++;
        furi_string_cat_printf(s, "Emplacement %u\n", (unsigned)r->record);

        const char* tname = calypso_contract_tariff_name(c.tariff);
        if(tname) {
            furi_string_cat_printf(s, "  %s\n", tname);
        } else {
            furi_string_cat_printf(s, "  Tarif 0x%04X\n", c.tariff);
        }

        if(c.flags & CALYPSO_CONTRACT_HAS_SALE && c.sale_year) {
            furi_string_cat_printf(
                s, "  Achete le %02u/%02u/%04u\n", c.sale_day, c.sale_month, c.sale_year);
        }
        if(c.flags & CALYPSO_CONTRACT_HAS_DURATION) {
            /* metrodroid documents 0/1/2. Units 3 means months on real
             * 2023-2026 STIB cards: a 49 EUR monthly pass reads 1, the
             * yearly pass 12 (see contract_end). */
            const char* unit = "?";
            switch(c.duration_units) {
            case 0:
                unit = "jours";
                break;
            case 1:
                unit = "semaines";
                break;
            case 2:
            case 3:
                unit = "mois";
                break;
            }
            furi_string_cat_printf(s, "  Duree %u %s\n", c.duration, unit);
        }
        if(c.flags & CALYPSO_CONTRACT_HAS_PRICE && c.price_amount) {
            /* Calypso stores the price in cents (centimes) per metrodroid's
             * En1545LookupSTR.parseCurrency → TransitCurrency.EUR(price). */
            furi_string_cat_printf(
                s, "  Prix %u,%02u EUR\n", c.price_amount / 100, c.price_amount % 100);
        }
        uint32_t trips;
        if(contract_trips_left(d, &c, r->record, &trips)) {
            furi_string_cat_printf(s, "  Voyages restants %lu\n", (unsigned long)trips);
        }
        furi_string_cat_str(s, "\n");
    }
    if(shown == 0) furi_string_cat_str(s, "Aucun abonnement actif.\n");
}

/* ----------------------------- Journeys ----------------------------- */

static void format_journeys(const MobibDump* d, FuriString* s) {
    append_divider(s, "TRAJETS");

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
        furi_string_cat_str(s, "Aucun trajet.\n");
        return;
    }

    for(size_t i = 0; i < count; ++i) {
        const CalypsoEvent* e = &events[i];

        if(i > 0) furi_string_cat_str(s, "----------------------\n");

        /* 1. Date - heure. */
        if(e->event_year) {
            furi_string_cat_printf(
                s,
                "%02u/%02u/%04u - %02u:%02u\n",
                e->event_day,
                e->event_month,
                e->event_year,
                e->event_hour,
                e->event_minute);
        } else {
            furi_string_cat_str(s, "(date invalide)\n");
        }

        JourneyInfo j;
        journey_describe(e, &j);
        const bool has_provider = e->flags & CALYPSO_EVENT_HAS_PROVIDER;

        /* 2. Mode + ligne. */
        if(append_journey_mode(s, e, &j)) {
            furi_string_cat_str(s, "\n");
        } else if(has_provider) {
            furi_string_cat_printf(s, "Mode inconnu (%u)\n", e->service_provider);
        }

        /* 3. Lieu. */
        if(j.stop) {
            furi_string_cat_printf(s, "Arret : %s\n", j.stop->name);
        } else if(j.station) {
            furi_string_cat_printf(s, "Station : %s\n", j.station->name);
        } else if(!j.is_metro && e->flags & CALYPSO_EVENT_HAS_LOCATION_BUS && e->location_id_bus) {
            furi_string_cat_printf(s, "Arret n. %u\n", e->location_id_bus);
        } else if(has_provider && e->location_id) {
            furi_string_cat_printf(
                s, "%s n. %lu\n", j.is_metro ? "Station" : "Lieu", (unsigned long)e->location_id);
        }

        /* 4. Correspondance: the first validation of the journey differs
         * from this one when the passenger changed vehicle. */
        const bool first_differs =
            (e->flags & CALYPSO_EVENT_HAS_FIRST_STAMP) &&
            (e->first_stamp_year != e->event_year || e->first_stamp_month != e->event_month ||
             e->first_stamp_day != e->event_day || e->first_stamp_hour != e->event_hour ||
             e->first_stamp_minute != e->event_minute);
        if(first_differs) {
            furi_string_cat_printf(
                s,
                "Correspondance\n  depart a %02u:%02u\n",
                e->first_stamp_hour,
                e->first_stamp_minute);
        } else if(e->flags & CALYPSO_EVENT_HAS_FIRST_STAMP) {
            furi_string_cat_str(s, "Debut de trajet\n");
        }
        if(e->flags & CALYPSO_EVENT_HAS_TRANSFER && e->transfer_number) {
            furi_string_cat_printf(s, "Correspondance n. %u\n", e->transfer_number);
        }

        if(e->flags & CALYPSO_EVENT_HAS_SERIAL) {
            furi_string_cat_printf(s, "Validation n. %lu\n", (unsigned long)e->serial_number);
        }
        furi_string_cat_str(s, "\n");
    }
}

/* ------------------------------ Records ----------------------------- */

static void format_records(const MobibDump* d, FuriString* s) {
    furi_string_cat_printf(s, "=== ENREGISTREMENTS (%zu) ===\n\n", d->record_count);

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
        for(size_t b = 0; b < r->len; ++b)
            if(r->data[b]) {
                all_zero = false;
                break;
            }

        if(all_zero) {
            furi_string_cat_str(s, "(vide)\n");
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
        furi_string_cat_str(s, "Non disponible.\n");
        return;
    }

    CalypsoFci fci;
    if(calypso_fci_parse(d->fci, d->fci_len, &fci) && fci.valid) {
        furi_string_cat_str(s, "AID  ");
        if(fci.is_calypso_aid) {
            furi_string_cat_str(s, "1TIC.ICA\n");
            if(fci.is_mobib_extension) {
                furi_string_cat_str(s, "      (famille MOBIB)\n");
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

    furi_string_cat_printf(s, "Brut (%zu octets)\n", d->fci_len);
    for(size_t i = 0; i < d->fci_len; ++i) {
        furi_string_cat_printf(s, "%02X", d->fci[i]);
        if((i + 1) % 8 == 0)
            furi_string_cat_str(s, "\n");
        else if(i + 1 < d->fci_len)
            furi_string_cat_str(s, " ");
    }
    if(d->fci_len % 8 != 0) furi_string_cat_str(s, "\n");
}

/* ------------------------------- Deep scan --------------------------- */

static void format_deep(const MobibDump* d, FuriString* s) {
    append_divider(s, "ANALYSE AVANCEE");

    furi_string_cat_str(s, "Applis presentes :\n");
    furi_string_cat_printf(s, "  1TIC.ICA  %s\n", d->calypso_selected ? "oui" : "non");
    furi_string_cat_printf(s, "  3MTR.ICA  %s\n", d->has_mpp ? "oui" : "-");
    furi_string_cat_printf(s, "  3TCW.ICA  %s\n", d->has_rt2 ? "oui" : "-");
    furi_string_cat_printf(s, "  2TIC.ICA  %s\n", d->has_eticket ? "oui" : "-");

    furi_string_cat_str(s, "\nFichiers en plus :\n");
    if(d->extra_count == 0) {
        furi_string_cat_str(s, "  (aucun lisible)\n");
    } else {
        for(size_t i = 0; i < d->extra_count; ++i) {
            const MobibExtraFile* e = &d->extras[i];
            furi_string_cat_printf(s, "  %s (0x%04X, %uB)\n", e->label, e->file_id, e->len);
            const size_t n = e->len < 12 ? e->len : 12;
            furi_string_cat_str(s, "    ");
            for(size_t b = 0; b < n; ++b) {
                furi_string_cat_printf(s, "%02X", e->data[b]);
                if(b + 1 < n) furi_string_cat_str(s, " ");
            }
            if(e->len > n) furi_string_cat_str(s, "...");
            furi_string_cat_str(s, "\n");
        }
    }

    furi_string_cat_str(
        s,
        "\nNote : ecrire dans un\n"
        "fichier Calypso exige\n"
        "les cles de l'emetteur,\n"
        "stockees par STIB/SNCB/\n"
        "TEC/De Lijn dans un SAM\n"
        "materiel. Impossible\n"
        "de les extraire de la\n"
        "carte.\n");
}

/* ------------------------------- public ----------------------------- */

void mobib_format_section(MobibSection section, const MobibDump* dump, FuriString* out) {
    furi_string_reset(out);
    if(!dump) {
        furi_string_set_str(out, "Aucune carte chargee.\n");
        return;
    }

    switch(section) {
    case MobibSectionOverview:
        format_overview(dump, out);
        break;
    case MobibSectionHolder:
        format_holder(dump, out);
        break;
    case MobibSectionContracts:
        format_contracts(dump, out);
        break;
    case MobibSectionJourneys:
        format_journeys(dump, out);
        break;
    case MobibSectionRecords:
        format_records(dump, out);
        break;
    case MobibSectionFci:
        format_fci(dump, out);
        break;
    case MobibSectionDeep:
        format_deep(dump, out);
        break;
    default:
        break;
    }
}
