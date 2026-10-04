# Journal des versions

Toutes les évolutions notables de Mobipper. Les versions suivent le format `major.minor` des applications Flipper.

## v0.3 — 2026-10-04

### Changements
- Les sauvegardes s'affichent avec l'icône de Mobipper dans « Sauvegardes », au lieu du « ? » des fichiers de type inconnu.
- README : capture des sauvegardes et GIF de démonstration mis à jour.

## v0.2 — 2026-10-04

### Ajouts
- **Voyages restants** des titres à voyages (Jump 1 / 10 voyages) : affichés dans « Abonnements » et, s'il en reste, dans le résumé. Compteurs lus comme metrodroid (fichier commun SFI 19, sinon SFI 0A à 0D).
- **Une sauvegarde par carte** : rescanner une carte déjà enregistrée met à jour sa sauvegarde au lieu de créer « NOM Prenom 2 » (reconnaissance par numéro de carte).
- **Stations de métro** : ajout d'Elisabeth (ligne 2/6) et du second code de Gare du Midi ; lignes renommées selon le réseau actuel (1, 2, 5, 6, prémétro).
- Compilation automatique (GitHub Actions) pour Momentum et le firmware officiel, avec les `.fap` joints à chaque release.
- GIF de démonstration dans le README.

### Changements
- Identifiant de l'app : `mobib` devient `mobipper` (pour laisser `mobib` au projet d'origine). Les sauvegardes de la v0.1 sont déplacées automatiquement de `/ext/apps_data/mobib/dumps` vers `/ext/apps_data/mobipper/dumps` au premier lancement.
- Auteurs de l'app : « Th3rd & i12bp8 », lien vers le dépôt dans le manifeste.

## v0.1 — 2026-10-03

Première version de Mobipper, basée sur [flipper-mobib](https://github.com/i12bp8/flipper-mobib) de i12bp8.

### Ajouts
- Interface entièrement en français.
- Sauvegardes nommées « NOM Prenom », menu « Sauvegardes » et suppression avec confirmation.
- Résumé affiché juste après le scan : titulaire, code postal et commune (base de 1 187 codes postaux belges), date de naissance, validité de la carte, fin d'abonnement, dernière utilisation.
- Trajets lisibles : date et heure, mode et ligne, arrêt ou station, correspondance.
- Table des arrêts STIB régénérée depuis le GTFS 2026 (3 405 arrêts), numéro d'arrêt lu sur 16 bits.
- Nouvelle icône, nom « Mobipper », crédits dans « A propos ».

### Corrections
- Plus de blocage sur un écran blanc en ouvrant « Sauvegardes » (navigateur de fichiers intégré à l'app).
- Unité de durée 3 = mois (et non années) ; transporteur 0 = métro.
