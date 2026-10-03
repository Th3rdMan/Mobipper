/*
 * MOBIB — Calypso FCI decoder. Contract documented in `calypso_fci.h`.
 */

#include "calypso_fci.h"
#include "calypso_tlv.h"
#include "calypso.h"

#include <string.h>

#define TAG_FCI_TEMPLATE 0x6Fu
#define TAG_DF_NAME      0x84u
#define TAG_PROP_FCI     0xA5u
#define TAG_ISSUER_DISCR 0xBF0Cu
#define TAG_APP_SERIAL   0xC7u

bool calypso_fci_parse(const uint8_t* fci, size_t fci_len, CalypsoFci* out) {
    if(!fci || !out) return false;
    memset(out, 0, sizeof(*out));

    /* Locate the FCI Template (tag 6F). The cursor starts at the raw
     * response so a top-level search is enough. */
    CalypsoTlv tmpl;
    if(!calypso_tlv_find(fci, fci_len, TAG_FCI_TEMPLATE, &tmpl)) return false;

    /* DF Name. */
    CalypsoTlv df;
    if(calypso_tlv_find(tmpl.value, tmpl.length, TAG_DF_NAME, &df)) {
        const size_t n = df.length < CALYPSO_DF_NAME_MAX ? df.length : CALYPSO_DF_NAME_MAX;
        memcpy(out->df_name, df.value, n);
        out->df_name_len = n;

        if(n >= CALYPSO_AID_LEN && memcmp(df.value, CALYPSO_AID, CALYPSO_AID_LEN) == 0) {
            out->is_calypso_aid = true;
            if(n >= CALYPSO_AID_LEN + CALYPSO_AID_EXTENSION_LEN) {
                memcpy(out->aid_extension, df.value + CALYPSO_AID_LEN, CALYPSO_AID_EXTENSION_LEN);
                out->has_aid_extension = true;
                /* `D0 56` is the marker assigned by Calypso Networks
                 * Association and used by every MOBIB card observed. */
                out->is_mobib_extension = out->aid_extension[0] == 0xD0 &&
                                          out->aid_extension[1] == 0x56;
            }
        }
    }

    /* Application Serial Number. */
    CalypsoTlv asn;
    if(calypso_tlv_find(tmpl.value, tmpl.length, TAG_APP_SERIAL, &asn)) {
        const size_t n = asn.length < CALYPSO_APP_SERIAL_MAX ? asn.length : CALYPSO_APP_SERIAL_MAX;
        memcpy(out->app_serial, asn.value, n);
        out->app_serial_len = n;
    }

    out->valid = out->df_name_len > 0;
    return out->valid;
}
