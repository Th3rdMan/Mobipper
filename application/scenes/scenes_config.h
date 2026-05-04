/*
 * X-macro list of every scene in the application. This file is intentionally
 * header-guard-less: it is included multiple times with different definitions
 * of `ADD_SCENE` to generate enums, handler tables and forward declarations.
 *
 * To add a new scene:
 *   1. Append a line below.
 *   2. Create `scene_<name>.c` implementing the three handlers.
 *   3. List the new source in `application.fam`.
 */

ADD_SCENE(mobib, start, Start)
ADD_SCENE(mobib, scan,  Scan)
ADD_SCENE(mobib, about, About)
