# 🚇 flipper-mobib FR – Lecteur de cartes MOBIB pour Flipper Zero

![Version](https://img.shields.io/badge/Version-0.1--fr-blue)
![License](https://img.shields.io/badge/License-GPL--3.0-teal)
![Firmware](https://img.shields.io/badge/Firmware-Momentum_mntm--012-orange)
[![Author: Th3rd](https://img.shields.io/badge/github-Th3rdMan-181717?logo=github)](https://github.com/Th3rdMan)
[![Based on: flipper-mobib](https://img.shields.io/badge/based_on-i12bp8%2Fflipper--mobib-555)](https://github.com/i12bp8/flipper-mobib)

**flipper-mobib FR** est une application [Flipper Zero](https://flipperzero.one/) qui lit et décode les cartes de transport belges **MOBIB** — la carte sans contact Calypso / ISO 14443-B partagée par la **STIB/MIVB**, la **SNCB/NMBS**, **De Lijn** et le **TEC**.  
Il suffit de poser la carte contre le dos du Flipper : l'application lit tous les fichiers accessibles, décode les informations utiles (titulaire, abonnements, derniers trajets) et sauvegarde la lecture sur la carte SD.

> Construit sur la base de [flipper-mobib](https://github.com/i12bp8/flipper-mobib) par **i12bp8**, traduit en français, réorganisé et enrichi.

<p align="center"><img src="docs/screenshots/menu.png" width="384" alt="Menu principal"></p>

---

## 🔍 Fonctionnalités

- 🇫🇷 **Interface entièrement en français**  
  Menus, écrans de lecture, sections de la carte, messages et écran « A propos ».  
  Dates au format `JJ/MM/AAAA`, prix en `EUR`. Les polices du Flipper étant limitées à l'ASCII, les textes sont écrits sans accents.

- 📋 **Résumé affiché dès la lecture**  
  Après un scan, l'essentiel apparaît immédiatement, sans passer par les menus :  
  titulaire, commune, date de naissance, validité de la carte, fin de l'abonnement en cours, dernier voyage.

- 📮 **Code postal → commune**  
  Le code postal du titulaire est traduit en nom de commune (`1090 Jette`) grâce à une base de **1187 codes postaux belges** (bpost).  
  La base est livrée comme fichier d'assets (`files/postal_codes.txt`) : installée sur la carte SD au premier lancement, elle ne consomme pas de RAM.

- 💾 **Sauvegardes nommées `NOM Prenom`**  
  Chaque lecture est enregistrée dans `/ext/apps_data/mobib/dumps/` sous le nom du titulaire (`DUPONT Jean.mobibdump`), avec un suffixe numérique si le nom existe déjà (`DUPONT Jean 2`).  
  Les cartes anonymes gardent le format `<PUPI>_<date>`.

- 🗑️ **Suppression des sauvegardes**  
  Depuis le menu de la carte, avec écran de confirmation. Retour automatique à la liste des sauvegardes.

- 🚌 **Trajets lisibles**  
  Chaque validation est présentée clairement :

  | Ligne | Exemple |
  |-------|---------|
  | Date - heure | `02/10/2026 - 17:26` |
  | Mode + ligne | `Bus 71`, `Tram 92`, `Metro 2` |
  | Lieu | `Arret : Flagey` / `Station : Simonis` |
  | Correspondance | `depart a 16:32` (heure de la première validation du trajet) |

- 🗺️ **Arrêts STIB à jour (2026)**  
  Table régénérée depuis le GTFS officiel STIB-MIVB : **72 lignes** (bus, tram, métro) et **3405 arrêts**, contre 49 lignes de bus en 2009.  
  Le script [`tools/gtfs_stops.py`](tools/gtfs_stops.py) permet de la régénérer à tout moment.

- 🎫 **Abonnements corrigés**  
  L'unité de durée utilisée par les cartes actuelles est interprétée en **mois** (et non en années), ce qui donne des durées et des dates de fin cohérentes.

- 🔎 **Sections détaillées**  
  Titulaire, Abonnements, Trajets, Enregistrements bruts, FCI et Analyse avancée (applications Calypso secondaires) restent accessibles depuis le menu de la carte.

---

## 🧾 Exemple de résumé

Données fictives :

```
=== CARTE MOBIB ===

DUPONT Jean
1000 Bruxelles
Ne le 15/03/1990

Reseau MOBIB
Carte n. 1A2B3C4D
Valable jusqu'au 12/05/2029

Abonnement 31/10/2026
Dernier voyage :
  Place Reine Astrid
02/10 17:26 - Tram 19

1 abonnement
4 trajets
```

- **Titulaire** : nom, commune (code postal + nom), date de naissance (`Ne le` / `Nee le`). Chaque ligne n'apparaît que si la carte contient l'information.
- **Abonnement** : date de fin estimée de l'abonnement le plus récent (date d'achat + durée − 1 jour). Les tickets Jump ne sont pas comptés. `(echu)` est ajouté si la date est dépassée.
- **Dernier voyage** : le nom de l'arrêt n'apparaît que s'il est reconnu ; un nom long passe à la ligne suivante.

---

## 🎯 Objectif

flipper-mobib FR est conçu pour **lire ses propres cartes** et comprendre ce qu'elles contiennent, dans un cadre de curiosité, d'interopérabilité ou de recherche.

L'application est **en lecture seule** par conception :

- pas d'écriture ni de modification des abonnements ;
- pas de recharge ni de modification des compteurs de voyages ;
- pas de déchiffrement des authentifiants de la carte ;
- pas de lecture des fichiers protégés par une session sécurisée.

Toute écriture Calypso exige une session authentifiée avec les clés des opérateurs, stockées dans un module matériel (SAM) et impossibles à extraire de la carte.

---

## 📊 Sources de données

| Donnée | Source | Validation |
|--------|--------|------------|
| Arrêts bus / tram / métro (ligne + numéro STIB) | GTFS STIB-MIVB, [data.belgianmobility.io](https://data.belgianmobility.io), 03/10/2026 | 97 % des couples (ligne, arrêt) communs avec la table 2009 portent le même nom |
| Stations de métro (zone / sous-zone / station) | [zoobab/mobib-extractor](https://github.com/zoobab/mobib-extractor) (2009) | inchangée |
| Codes postaux → communes | bpost / NGI-IGN, [Open Data Wallonie-Bruxelles](https://www.odwb.be/explore/dataset/postal-codes-belgium/), 05/09/2025 | 1187 codes ; noms FR, sinon NL / DE |
| Structure des champs (titulaire, trajets, abonnements) | [metrodroid](https://github.com/metrodroid/metrodroid) | vérifiée sur des cartes réelles 2024-2026 |

Points vérifiés sur des cartes réelles :

- le nom est stocké `PRENOM` + séparateur + `NOM` ;
- le numéro d'arrêt bus / tram occupe **16 bits** (metrodroid n'en lit que 12) : sans cela, les arrêts au-delà de 4095 étaient introuvables ;
- le code transporteur `0` correspond au **métro** ;
- l'unité de durée `3` des abonnements correspond à des **mois** (abonnement de 49 EUR = 1, abonnement annuel = 12).

---

## ⚠️ Limites connues

- La carte ne conserve que les **3 ou 4 dernières validations**, pas l'historique complet.
- Seules les **validations** sont enregistrées : impossible de savoir où le voyageur est descendu.
- La table des **stations de métro** date de 2009 et utilise une numérotation propre à la carte, absente du GTFS : certaines stations s'affichent sous forme de numéro.
- La **date de fin d'abonnement** est calculée (date d'achat + durée) et non lue sur la carte.
- Seuls 4 noms de tarifs sont connus ; les autres s'affichent `Tarif 0x…`.
- Pas d'accents à l'écran : les polices intégrées du Flipper sont limitées à l'ASCII.

---

## 📦 Installation

Firmware cible : **Momentum** (testé sur `mntm-012`, API 87.1).

**Compilation avec [`ufbt`](https://github.com/flipperdevices/flipperzero-ufbt) :**
```bash
git clone https://github.com/Th3rdMan/flipper-mobib.git
cd flipper-mobib
pip install ufbt
ufbt update --hw-target=f7 --url=https://up.momentum-fw.dev/builds/firmware/mntm-012/flipper-z-f7-sdk-mntm-012.zip
ufbt              # compile dist/mobib.fap
ufbt launch       # compile, envoie sur le Flipper connecté et lance l'application
```

Le fichier `dist/mobib.fap` peut aussi être copié manuellement dans `/ext/apps/NFC/` sur la carte SD.

**Mise à jour de la table des arrêts :**
```bash
# gtfs/ = archive GTFS STIB-MIVB décompressée
python tools/gtfs_stops.py gtfs application/calypso/calypso_bus_table.inc nouvelle_table.inc
```

**Mise à jour des codes postaux :**
```bash
# export CSV (séparateur ;) de https://www.odwb.be/explore/dataset/postal-codes-belgium/
python tools/postal_codes.py postal_codes.csv files/postal_codes.txt --report
```

---

## 🧭 Utilisation

1. Ouvrir **Apps → NFC → MOBIB**.

2. Choisir **Lire une carte** et poser la carte MOBIB à plat contre le dos du Flipper.

3. Le **Résumé** s'affiche automatiquement ; la lecture est sauvegardée sous `NOM Prenom`.

4. **Retour** ouvre le menu de la carte : Résumé, Titulaire, Abonnements, Trajets, Enregistrements, FCI, Analyse avancée.

5. **Sauvegardes** (menu principal) permet de rouvrir une lecture précédente sans la carte.

6. **Supprimer la sauvegarde** (en bas du menu de la carte) efface le fichier après confirmation.

---

## ✍️ Auteur & crédits

**Th3rd**  
👁️‍🗨️ [https://github.com/Th3rdMan](https://github.com/Th3rdMan)

Basé sur [**flipper-mobib**](https://github.com/i12bp8/flipper-mobib) par [**i12bp8**](https://github.com/i12bp8) — merci pour les bases posées. Sa documentation d'origine (en anglais) est conservée dans [`docs/README.upstream.md`](docs/README.upstream.md).

Remerciements également à :

- [**metrodroid**](https://github.com/metrodroid/metrodroid) — structures des champs MOBIB / Calypso (GPL-3.0) ;
- [**zoobab/mobib-extractor**](https://github.com/zoobab/mobib-extractor) — table des stations de métro ;
- **STIB-MIVB** — données des arrêts, © STIB-MIVB, licence [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) ;
- **bpost** / **NGI-IGN** — liste des codes postaux belges, via [Open Data Wallonie-Bruxelles](https://www.odwb.be/explore/dataset/postal-codes-belgium/).

Sans lien avec la STIB/MIVB, la SNCB/NMBS, De Lijn, le TEC ou Calypso Networks Association. À utiliser uniquement sur ses propres cartes, dans le respect des lois sur la RFID et les données personnelles.

---

> 📘 Projet libre sous licence **GPL-3.0-or-later**. Contributions, suggestions et pull requests bienvenues.
