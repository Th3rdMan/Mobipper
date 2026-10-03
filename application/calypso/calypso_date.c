/*
 * MOBIB — EN 1545 date / time helpers. See `calypso_date.h`.
 */

#include "calypso_date.h"

bool calypso_date_from_days(
    uint16_t days,
    uint16_t* year_out,
    uint8_t* month_out,
    uint8_t* day_out) {
    static const uint8_t mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint16_t year = 1997;
    uint32_t remaining = days;

    while(true) {
        const bool leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        const uint16_t ydays = leap ? 366 : 365;
        if(remaining < ydays) break;
        remaining -= ydays;
        year++;
        if(year > 2100) return false;
    }

    uint8_t month = 1;
    while(month <= 12) {
        const bool leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        uint8_t md = mdays[month - 1];
        if(month == 2 && leap) md = 29;
        if(remaining < md) break;
        remaining -= md;
        month++;
    }

    *year_out = year;
    *month_out = month;
    *day_out = (uint8_t)(remaining + 1);
    return true;
}

void calypso_time_from_minutes(uint16_t minutes, uint8_t* hour, uint8_t* minute) {
    if(minutes >= 24 * 60) minutes = 24 * 60 - 1;
    *hour = (uint8_t)(minutes / 60);
    *minute = (uint8_t)(minutes % 60);
}
