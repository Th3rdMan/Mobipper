# Architecture

The application is structured around the standard Flipper GUI building
blocks: a `ViewDispatcher` driving a `SceneManager`, with a small set of
reusable view modules allocated once at startup.

```
                ┌────────────────────┐
                │   mobib_app_main   │   FAP entry point
                └─────────┬──────────┘
                          │ allocates
                ┌─────────▼──────────┐
                │      MobibApp      │   global app state
                └──┬───────────────┬─┘
                   │ owns          │ owns
        ┌──────────▼─────┐  ┌──────▼────────────┐
        │ ViewDispatcher │  │   SceneManager    │
        └──────────┬─────┘  └──────┬────────────┘
                   │ hosts         │ runs
        ┌──────────▼──────────┐    │
        │ Submenu / Widget /  │◄───┘
        │ Popup / TextBox …   │
        └─────────────────────┘
```

## Source organisation

| Path                              | Responsibility                                      |
|-----------------------------------|-----------------------------------------------------|
| `application/mobib_app.{c,h}`     | App lifecycle, view registration, scene tables      |
| `application/scenes/scenes.h`     | Scene id enum + handler tables                      |
| `application/scenes/scenes_config.h` | X-macro list of scenes (single source of truth)  |
| `application/scenes/scene_*.c`    | One scene per file (`on_enter`/`on_event`/`on_exit`)|

New scenes are added by:

1. Appending an `ADD_SCENE(...)` line in `scenes_config.h`.
2. Creating `scene_<name>.c` with the three handler functions.
3. Listing the new source in `application.fam`.

## Planned milestones

1. **M0 — skeleton (this commit).** App builds, launches, shows a Start
   menu (`Read MOBIB`, `About`) and handles back navigation cleanly.
2. **M1 — card detection.** Open the NFC HAL, poll for ISO 14443-B,
   recognise a MOBIB ATQB and surface the UID/PUPI on screen.
3. **M2 — raw dump.** Select the Calypso AID (`1TIC.ICA`), enumerate the
   well-known SFIs and persist a binary dump to the SD card.
4. **M3 — decoder.** Parse Environment / Holder / Contracts / Event Log
   per the Intercode v2 layout used on Belgian networks.
5. **M4 — UI.** Trip list, contract details, balance/expiry, JSON export.
