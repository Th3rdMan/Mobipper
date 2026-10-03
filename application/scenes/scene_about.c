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
        "Lecteur NFC de cartes\n"
        "de transport belges\n"
        "MOBIB (Calypso /\n"
        "ISO 14443-B).\n"
        "\n"
        "Decode :\n"
        "- FCI et ID appli\n"
        "- Environnement\n"
        "  (pays, reseau,\n"
        "   expiration, CP)\n"
        "- Titulaire (nom, sexe,\n"
        "  date de naissance via\n"
        "  HOLDER_EXTENDED)\n"
        "- Abonnements (tarif,\n"
        "  date d'achat, prix)\n"
        "- Journal (trajets avec\n"
        "  date/heure, mode,\n"
        "  ligne, arret/station\n"
        "  STIB)\n"
        "- Analyse avancee :\n"
        "  applis Calypso\n"
        "  secondaires et\n"
        "  fichiers par chemin\n"
        "\n"
        "Lecture seule. Ecrire\n"
        "dans la carte exige les\n"
        "cles de STIB/SNCB/TEC/\n"
        "De Lijn, stockees dans\n"
        "un SAM materiel et\n"
        "impossibles a extraire.\n"
        "\n"
        "Tables des stations :\n"
        "zoobab/mobib-extractor.\n"
        "Structures des champs :\n"
        "metrodroid/metrodroid\n"
        "(GPL-3.0).\n"
        "\n"
        "flipper-mobib v0.1-fr\n"
        "GPL-3.0-or-later\n"
        "\n"
        "Version FR par Th3rd\n"
        "github.com/Th3rdMan\n"
        "/flipper-mobib\n"
        "\n"
        "D'apres i12bp8\n"
        "github.com/i12bp8\n"
        "/flipper-mobib\n"
        "\n"
        "Sans lien avec STIB/\n"
        "MIVB, SNCB/NMBS,\n"
        "De Lijn ou TEC.");

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
