/*
 * MOBIB — About scene: short description and version string.
 */

#include "../mobib_app.h"

void mobib_scene_about_on_enter(void* context) {
    MobibApp* app = context;
    Widget*   w   = app->widget;

    widget_reset(w);
    widget_add_text_box_element(
        w, 0, 0, 128, 14, AlignCenter, AlignTop, "\e#MOBIB\e#", false);
    widget_add_text_scroll_element(
        w, 0, 16, 128, 48,
        "Reader for Belgian MOBIB\n"
        "transit cards over NFC\n"
        "(Calypso / ISO 14443-B).\n"
        "\n"
        "Decodes:\n"
        "- FCI & application ID\n"
        "- Environment record\n"
        "  (country, network,\n"
        "   expiry, postcode)\n"
        "- Holder (name, gender,\n"
        "  birth date via\n"
        "  HOLDER_EXTENDED)\n"
        "- Contracts (tariff,\n"
        "  sale date, price)\n"
        "- Event log (journeys\n"
        "  with date/time,\n"
        "  transport mode, line,\n"
        "  STIB stop / station)\n"
        "- Deep scan: secondary\n"
        "  Calypso apps and\n"
        "  path-selected files\n"
        "\n"
        "Read-only. Writes to\n"
        "Calypso files require\n"
        "issuer keys held by\n"
        "STIB/SNCB/TEC/De Lijn\n"
        "in a hardware SAM and\n"
        "cannot be extracted.\n"
        "\n"
        "Station tables sourced\n"
        "from zoobab/mobib-\n"
        "extractor (MIT-style).\n"
        "Field layouts ported\n"
        "from metrodroid/metro-\n"
        "droid (GPL-3.0).\n"
        "\n"
        "flipper-mobib v0.1\n"
        "GPL-3.0-or-later\n"
        "\n"
        "Not affiliated with\n"
        "STIB/MIVB, SNCB/NMBS,\n"
        "De Lijn or TEC.");

    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewWidget);
}

bool mobib_scene_about_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void mobib_scene_about_on_exit(void* context) {
    MobibApp* app = context;
    widget_reset(app->widget);
}
