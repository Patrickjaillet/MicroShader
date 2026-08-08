# TODO.md — audit du 2026-08-05

> **Portée de cet audit** : les trois audits approfondis initialement prévus (moteur
> Rust de golf, coquille C++ Win32 complète, couverture de tests + exactitude de la
> doc) ont été interrompus par une limite de session API avant de produire un
> résultat. Ce document a été reconstitué par une passe directe (grep/lecture ciblée,
> sans sous-agents) — moins exhaustive que prévu, mais chaque point ci-dessous a été
> **vérifié concrètement dans le code**, pas deviné. À relancer avec les agents pour
> compléter si besoin.

---

## Critique

- [x] **L'accessibilité lecteur d'écran ne couvre pas tout, contrairement à ce que dit le README.**
  Corrigé : README mis à jour pour indiquer que la couverture n'est pas encore
  complète pour Diff, Trace, Stats, Keybindings, Command Palette et Minimap.
  (Les enregistrements d'accessibilité manquants dans les 6 fichiers restent
  un travail futur.)

- [x] **Pas de déclaration "DPI-aware" — rendu potentiellement flou/mal dimensionné sur écran HiDPI.**
  Corrigé partiellement : `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`
  ajouté au début de `main()`. La mise en page pixel-fixe reste un chantier futur
  (scaling complet des constantes UI).

---

## Haute priorité

- [x] **Le mode MRT n'est pas couvert par la résolution automatique de collision de noms.**
  Corrigé : `resolve_rename_collisions` prend maintenant un paramètre `mrt_targets` et,
  pour `mrt_targets >= 2`, vérifie les noms de sortie MRT (`o0`/`o1` ou
  `outColor0`/`outColor1`) au lieu du nom mono-cible. Comme `o0`/`o1` sont censés
  apparaître nus dans la source (convention MRT de twigl, pas de renommage
  `gl_FragColor` -> `o0`), la détection ne se déclenche que sur une **déclaration
  locale** du même nom (préfixée d'un mot-clé de type GLSL, via la nouvelle
  `identifier_looks_locally_declared`) — pas sur l'usage nu attendu, pour éviter les
  faux positifs sur tout shader MRT légitime. `rewrite_twigl_shader_mrt` passe
  désormais son vrai `mrt_targets` ; l'API C (`ushader_twigl_rename_collision_warnings`)
  et son unique appelant C++ (`Win32TwiglExportPanel::recompute_preview`) ont été mis à
  jour en conséquence. 5 nouveaux tests Rust couvrent le cas (déclaration locale
  renommée, usage nu non touché, `outColor0`/`outColor1` en mode Classic, non-régression
  du check mono-cible, bout-en-bout via `rewrite_twigl_shader_mrt`) ; build C++ complet
  + `ctest` vérifiés après coup.

- [x] **Couverture de tests quasi nulle sur plusieurs modules de logique pure, faciles à tester.**
  Corrigé : `tests/ui_pure_logic_test.cpp` (nouvelle cible CMake `ui_pure_logic_test`)
  couvre maintenant `fuzzy_match.cpp`, `unified_diff.cpp`, `glsl_format.cpp`,
  `keybindings_storage.cpp`, `recent_files.cpp` (via redirection de `APPDATA` vers un
  dossier scratch, jamais les vraies données utilisateur), `golf_options_convert.cpp`
  et `glsl_token_rules.cpp`/`glsl_syntax_colors.cpp`, sur le même modèle que
  `tests/twigl_golf_collision_test.cpp` (harnais `main()` sans framework).

- [x] **Classes UI avec état, sans aucun test — même classe de risque que les deux régressions Twigl déjà expédiées.**
  Corrigé : `tests/win32_panel_state_test.cpp` (nouvelle cible CMake
  `win32_panel_state_test`) couvre `win32_command_palette.cpp` (filtrage, navigation
  clavier, exécution/fermeture), `win32_diff_view.cpp` (hit-testing, clamp du scroll),
  `win32_trace_view.cpp` (expansion/collapse de ligne, reset sur `set_steps`),
  `win32_document_tab_strip.cpp` (hit-testing complet), `win32_keybindings.cpp`
  (matching, labels, round-trip save/load via `APPDATA` scratch), `win32_stats_panel.cpp`
  (contrat de cycle de vie sans `create()`), `win32_appearance_panel.cpp` (drag de
  slider, reset, checkbox) et `win32_minimap.cpp` (seuils d'affichage). `Win32TraceView`,
  `Win32DiffView` et `Win32CommandPalette` ont chacun reçu quelques petits accesseurs
  `const` réservés aux tests (même précédent que `Win32TwiglExportPanel::current_mode()`
  etc.) pour exposer l'état de sélection/scroll sans dépendre de D2D.

---

## Priorité moyenne

- [x] **Commentaire obsolète : la capture GIF est décrite comme "non implémentée".**
  Corrigé : commentaire dans `capture_viewport_png_action` mis à jour pour
  indiquer que le GIF est implémenté (Phase 45.1).

- [x] **`neyret.rs:258` — `.unwrap()` sur `partial_cmp` pourrait paniquer sur un `NaN`.**
  Corrigé : `.unwrap()` remplacé par `.unwrap_or(std::cmp::Ordering::Equal)`.

- [x] **`gif.rs` : plusieurs `.expect()` internes au dictionnaire LZW (lignes 421, 442, 523) reposent sur un invariant jamais vérifié par un test de propriété/fuzz.**
  Corrigé : deux nouveaux tests dans `rust-core/src/gif.rs` (`mod tests`) utilisent un
  petit générateur pseudo-aléatoire xorshift64 auto-contenu (pas de nouvelle
  dépendance externe), avec seed fixe pour reproductibilité. Le premier
  (`lzw_dictionary_invariant_holds_across_many_randomized_streams_and_code_sizes`)
  fait tourner 500 essais de `lzw_encode`/`lzw_decode` en faisant varier
  `min_code_size` (2 à 8 bits) et la forme du contenu (aléatoire uniforme, séquences
  répétitives forçant la croissance/reset du dictionnaire, motifs répétés de longueur
  variable). Le second
  (`encode_gif_round_trips_losslessly_across_randomized_dimensions_frame_counts_and_palettes`)
  fait la même chose via `encode_gif` bout-en-bout, avec largeur/hauteur/nombre de
  frames/nombre de couleurs (≤256, donc sans perte de quantification) tirés
  aléatoirement, et vérifie l'égalité pixel-à-pixel après décodage.

---

## Notes annexes (déjà connues, pour mémoire — pas de nouvelle action)

- Ambiguïté texte/portée déjà documentée dans `rust-core/src/twigl.rs` (identifiants
  `r`/`m`/`t`/`f`/`b`/`o`/`FC` d'un shader utilisateur, désormais **résolue
  automatiquement** à l'export — voir `roadmap.md`, section "-2"). Reste vrai en
  théorie pour l'import (`unrewrite_twigl_shader`) si le code twigl.app collé contient
  lui-même une ambiguïté de portée — cas non couvert par la résolution automatique
  (qui ne s'applique qu'à l'export), documenté comme limitation inhérente.
- `rust-core/src/inline.rs` protège explicitement `main`/`mainImage` de l'élimination
  de code mort et du renommage — vérifié, ce n'est **pas** un risque (une hypothèse de
  cette session, infirmée par la lecture du code).

---

## Comment continuer cet audit

Les trois angles suivants n'ont pas pu être creusés faute de budget de session :
1. Revue complète du moteur de recherche "Golf harder" (`rust-core/src/search.rs`) —
   objectifs de recherche, respect des bornes de temps/itérations.
2. Fuites de ressources D2D/DirectWrite/GDI+ sur les chemins d'erreur (paires
   create()/destroy() dans tout `src/ui/`).
3. Vérification ligne à ligne de chaque section du README contre le code (seul un
   échantillon — Compare mode, raccourcis clavier, accessibilité — a été vérifié ici).
