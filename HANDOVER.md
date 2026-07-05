# OoT RL — Project Handover

> **Status (Snapshot M8, 2026-07-05):** M7 ist abgeschlossen — steuerbares Gameplay
> wird per Warp vom Titlescreen betreten (§6), `LeaveHouseEnv` läuft stabil auf
> einem Savestate-Anker, das SB3/PPO-Prototyping-Harness (M8) existiert. Nächste
> Schritte laut Roadmap §7: Härtung (M8.5) → Step-Pfad-Performance &
> Grafik-Konfiguration (M8.6) → Parallelisierung (M9).

---

## 1. Zweck des Projekts

Reinforcement-Learning-Environment für *The Legend of Zelda: Ocarina of Time* (N64),
aufgebaut auf **Ship of Harkinian (SoH)** als headless Renderer. Ziel ist ein
deterministisches, schnelles, parallelisierbares Env mit Gymnasium-API, mit dem
RL-Agenten OoT spielen können.

Target-Hardware: 2× RTX 6000 Ada, ~100 CPU-Cores, ~64–80 parallel envs (M9).

## 2. Stack

| Komponente            | Technologie                                            |
|-----------------------|--------------------------------------------------------|
| Renderer              | Ship of Harkinian 9.2.3 (commit `cb71e22a`), gepatcht  |
| Headless-Output       | EGL + GL FBO + `glGetTexImage` (RGB)                   |
| Snapshot / Reset      | SoH Savestate-Manager (RAM-only, in-process)           |
| Native-Bindings       | pybind11, Modul `oot_rl`                               |
| Python-API            | Gymnasium (`gym.Env`)                                  |
| Build                 | CMake (Top-Level im Repo-Root), Ninja                  |

## 3. Repo-Struktur

```
oot_rl/
├── CMakeLists.txt              # Top-Level — DIESES bauen (NICHT external/Shipwright/)
├── src/
│   ├── smoke_test.cpp          # C++ Test-Executable (CTest: Boot + Behavior-Test)
│   └── python_bindings.cpp     # pybind11-Modul 'oot_rl'
├── external/
│   └── Shipwright/             # SoH submodule (Patches+Overlay zur Build-Zeit appliziert)
│       ├── libultraship/       # innerer submodule (auch gepatcht)
│       └── soh/                # game-specific code
├── overlay/                    # UNSERE eigenen neuen SoH-Files (z.B. soh_api.c,
│   │                           #   FrameCapture.cpp) — werden ins Submodule symlinkt
│   ├── shipwright/             # → external/Shipwright/soh/...
│   └── libultraship/           # → external/Shipwright/libultraship/...
├── patches/                    # NUR minimale Edits an UPSTREAM-Files (re-appliziert)
│   ├── shipwright/             # Patches gegen external/Shipwright/ (4)
│   ├── libultraship/           # Patches gegen libultraship submodule (11)
│   └── README.md               # Patch+Overlay-Workflow im Detail
├── scripts/                    # build.sh, apply_patches.sh, link_overlay.sh,
│                               #   start_dev.sh, export_patches.sh, leave_dev.sh
├── python/
│   ├── oot_rl_gym/leave_house.py    # erstes Trainings-Env (Package-Name ≠ oot_rl,
│   │                                #   sonst schattet es das native Modul)
│   ├── train_leavehouse.py          # M8 SB3/PPO-Prototyping-Harness (throwaway)
│   ├── test_native.py               # Smoke-Test der Bindings
│   ├── test_gym.py / test_warp.py / test_actorcounts.py   # weitere Test-Skripte
│   ├── find_loading_zone.py         # Helper zur Door-Position-Ermittlung
│   └── oot_rl.cpython-*.so          # Build-Output (gitignored, ~680 MB)
├── data/
│   ├── oot.o2r                 # extrahierte Game-Assets (aus ROM)
│   └── file1.sav               # OoT-eigenes Save-File (Boot-Anker)
└── build-cmake/                # CMake-Build-Output (Symlinks für *.o2r dorthin)
```

## 4. Build

### Voraussetzungen

- Ubuntu 24.04 (oder kompatibel), GCC 13+
- CMake, Ninja, ccache
- Dev-Libraries: EGL, SDL2, libpng, libogg, libvorbis, libopus, libopusfile,
  nlohmann_json, libzip, libtinyxml2, libspdlog, libfmt
- Python 3.11 (Conda empfohlen)
- pybind11 (im Python-Env: `pip install pybind11`)
- Originale OoT N64-ROM für die Asset-Extraktion (einmalig — daraus wird `oot.o2r`)

### Setup

```bash
conda create -n oot-rl python=3.11
conda activate oot-rl
pip install pybind11 numpy gymnasium

git submodule update --init --recursive
cmake -S . -B build-cmake
cmake --build build-cmake -j
```

Erster Build dauert lang (~1600 Steps; Shipwright wird vollständig kompiliert).
Wichtig: **Conda-Env muss beim `cmake -S . -B build-cmake` aktiv sein**, sonst
findet das CMakeLists pybind11 nicht.

### Häufige Build-Probleme

**`pybind11 not found` trotz Installation.** Shipwright filtert aktiv
Conda-Pfade aus `CMAKE_PREFIX_PATH` (man sieht `-- Ignoring Conda prefix: ...`
in der CMake-Ausgabe). Das Top-Level-CMakeLists nutzt deshalb
`python -m pybind11 --cmakedir` und setzt `pybind11_DIR` direkt. Falls das doch
scheitert: manuell `-Dpybind11_DIR=$(python -m pybind11 --cmakedir)` an cmake
übergeben.

**`relocation R_X86_64_TPOFF32 against ... recompile with -fPIC`.** Tritt auf,
weil statische Libs in eine `.so` gelinkt werden. Fix ist im Top-Level-CMakeLists
gesetzt (`set(CMAKE_POSITION_INDEPENDENT_CODE ON)`). Wenn der Fehler nach einem
Submodule-Update wieder auftritt: `rm -rf build-cmake && cmake -S . -B build-cmake
&& cmake --build build-cmake`.

**`oot.o2r` fehlt zur Laufzeit.** `GenerateSohOtr`-Target braucht die
Original-ROM. Liegt in `data/`, im `build-cmake/` als Symlink. Nach einem
`rm -rf build-cmake`: vor dem Bauen Symlink in `build-cmake/oot.o2r` wiederherstellen.

## 5. Was funktioniert

### `./build-cmake/smoke_test`  (auch via `ctest --test-dir build-cmake -R smoke_test`)

C++ Test-Executable (CTest-Target). Bootet die Engine mit neutralem Input (steppt
bis `GameState.valid`), captured einen Frame und prüft Save/Load-Determinismus.
Demonstriert M4/M6/M7 (Input, Save/Load, Frame-Capture).

> **Achtung — Boot landet in der Attract-Demo, nicht in steuerbarem Gameplay.**
> Mit neutralem Input lädt die Engine `data/file1.sav` automatisch und wird um
> Frame ~231 in Scene 81 (Kokiri Forest) `valid` — aber das ist die
> Title-Screen-Attract-Demo, nicht agenten-steuerbares Gameplay. `save_state`
> funktioniert dort (Engine ist im GamePlay-Mode), daher "bestehen" die Tests,
> während sie auf der Demo operieren. Siehe §6.

### `python ../python/test_native.py`

Lädt das native Modul, ruft `init()`, demonstriert Save/Load + Frame-Capture
mit zufälligen Aktionen. Funktioniert, weil im erfolgreichen Test ein
`save_state(0)` *nach* dem Erreichen von Gameplay gemacht wurde (in der Test-Datei
manuell).

### Native-Module-API (`import oot_rl as soh`)

```python
# Lifecycle
soh.set_app_path("/abs/path/to/build-cmake")   # MANDATORY before init
soh.set_headless(True)
soh.init()
soh.enable_frame_capture()
soh.shutdown()

# Stepping
soh.step_frame()

# Input
soh.set_input(port=0, buttons=soh.BTN_A, stick_x=80, stick_y=0)
soh.clear_input()
# Button-Konstanten: BTN_A, BTN_B, BTN_START, BTN_Z, BTN_L, BTN_R,
#                    BTN_CUP/CDOWN/CLEFT/CRIGHT, BTN_DUP/DDOWN/DLEFT/DRIGHT

# Game state — ACHTUNG: das Feld heißt scene_id, NICHT scene
gs = soh.get_game_state()
gs.valid                  # bool — true sobald GameState-Struct befüllt
gs.scene_id               # int  — current scene index (-1 falls nicht geladen)
gs.pos_x / pos_y / pos_z  # float coordinates
gs.hp / gs.max_hp

# Snapshot (RAM-only, NICHT persistent across processes)
soh.save_state(slot)      # RuntimeError, wenn ausserhalb GamePlay
soh.load_state(slot)

# Frame
w, h = soh.get_frame_dimensions()   # typisch (640, 480)
frame = soh.get_frame()             # numpy (480, 640, 3) uint8, RGB, top-down
```

## 6. Gameplay-Einstieg (M7-Blocker — GELÖST via Warp)

**Der Weg in steuerbares Gameplay ist der Warp vom Titlescreen**, nicht der
Auto-Boot: `soh.warp_to_entrance(entrance_id, link_age)` (vom Titlescreen aus,
d.h. bevor die Attract-Demo ihre PlayState erzeugt hat — dann nimmt SoHs `Warp()`
den sauberen `GAMEMODE_NORMAL`-Branch). Danach settlen lassen, `save_state(0)`
als Episoden-Anker. Genau so macht es `LeaveHouseEnv._build_anchor()`.

Historischer Kontext (der frühere M7-Blocker, weiterhin wichtig zu wissen):

- Mit neutralem Input lädt die Engine `data/file1.sav` automatisch und läuft um
  Frame ~231 in die **Title-Screen-Attract-Demo** (Scene 81, Hyrule Field) — die
  Engine ist dort im GamePlay-Mode (`save_state` funktioniert), aber der Agent
  steuert Link NICHT.
- **`gs.valid == true` heisst NUR "GameState-Struct ist befüllt", NICHT "Agent
  steuert Link".** Auch die Attract-Demo ist `valid`.
- `SoH_BootToGameplay()` (steppte nur bis `valid`, erreichte damit bestenfalls
  die Demo) ist in M8.5 **entfernt** — es war vom Warp-Pfad abgelöst und `valid`
  ≠ steuerbar. Der Einstieg ist ausschließlich der Warp; wer nur bis `valid`
  steppen will (z.B. der Smoke-Test), macht das mit einer lokalen Schleife.

## 7. Roadmap

> **Projektziel als Kalibrierung:** oot_rl liefert die *Infrastruktur* für
> RL-Forschung an OoT — schnell, deterministisch, parallelisierbar, flexibel
> konfigurierbar. Lern-Strategien (Exploration, Curricula, Demos, Algorithmen)
> sind bewusst NICHT Teil dieser Roadmap; die Infrastruktur muss sie nur
> ermöglichen, ohne sie vorwegzunehmen.
>
> **Leitmetrik (ab M8.6 bei jedem Schritt mitführen):** Env-Steps/s pro Prozess
> und aggregiert, plus "Spielstunden pro Wallclock-Stunde". Compute ist knapp und
> OoT teuer — jede Optimierung wird gegen diese Zahlen belegt, nicht vermutet.

### M7 — Gymnasium-Env, GROBE Variante — ABGESCHLOSSEN

- [x] Steuerbares Gameplay per **Warp** vom Titlescreen (`warp_to_entrance` /
      `warp_to`, Overlay `SoH_Warp.cpp`). Verifiziert: Kind-Link in Link's Haus
      (entrance 0xBB → scene 52), steuerbar. Details §6.
- [x] `save_state(0)` als grober Reset-Anker nach dem Warp; Reset stellt exakt
      wieder her (RNG/Kamera bewusst noch nicht deterministisch → Mx).
- [x] `LeaveHouseEnv` (`python/oot_rl_gym/`) smoke-getestet (`test_gym.py`).
- → Offene Reste wandern: Dialog-Recovery (B+START) in die `SoHEnv`-Basisklasse
  (M8.5); Inventar/Alter/exakte Startzustände ins Authoring-Milestone (Mx).

### M8 — Erstes Training (Prototyping, NICHT optimales Lernen) — WEITGEHEND FERTIG

> Ziel: die Pipeline end-to-end zum Laufen bringen, nicht optimal lernen. Das Env
> bleibt **algorithmus-agnostisch** (`gym.Env`); SB3 ist nur Prototyping-Harness.

- [x] Observation-Wrapper (`ResizeObservation` 84×84, optional Grayscale,
      `VecFrameStack` 4) in `train_leavehouse.py`.
- [x] **SB3 PPO** als Prototyping-Harness (`CnnPolicy`), bewusst throwaway.
- [x] Eval + MP4 der Policy (`--record`).
- [x] Single-process Steps/s als erste Baseline notieren (wird in M8.6 durch die
      Profiling-Zerlegung abgelöst). Skripte: `python/bench/bench_step.py` (Config
      1–3) und `python/bench/bench_env.py` (Config 4), je eigener Prozess.

  **Baseline 2026-07-05** (Ryzen 9 5950X 16C/32T, RTX 3090, headless EGL; je 2000
  Frames nach Warp-Anker, 200 Frames Warmup ausgeklammert; in-game 20 fps ⇒
  Spielstunden/Wallclock-Stunde = fps/20):

  | # | Konfiguration | Frames/s | Steps/s | Spielstd./Wallclock-Std. |
  |---|---|---|---|---|
  | 1 | Engine-Step, Frame-Capture AUS | ~195 | — | ~9.8× |
  | 2 | Engine-Step, Frame-Capture AN, kein get_frame | ~225 | — | ~11.3× |
  | 3 | Engine-Step + get_frame() jeden 4. Frame | ~205 | — | ~10.3× |
  | 4 | `LeaveHouseEnv` end-to-end, Random-Policy (frame_skip=4, get_frame/Step) | ~184 | ~46 | ~9.2× |

  Auffälligkeiten (direkter Input für M8.6):
  - Frame-Capture zu aktivieren kostet **nichts** — der Nicht-Capture-Pfad (Config
    1, ~195 fps) ist reproduzierbar *langsamer* als der Capture-Pfad (Config 2,
    ~225 fps). Der teure Anteil ist nicht die Capture-FBO, sondern der
    **get_frame()-Readback**: Config 3 verliert ggü. Config 2 ~20 fps bei nur jedem
    4. Frame. get_frame ist damit der Haupt-Hebel für M8.6 (Downscale nativ,
    Readback reduzieren/asynchron).
  - Config 4 (~46 Steps/s) liegt unter Config 3 trotz identischer get_frame-Kadenz:
    die Differenz (~205→~184 fps) ist reiner Python-Env-Overhead pro Step
    (Action-Dispatch, `get_game_state`, numpy, Reward). Klein, aber vor M9 messbar.

> **Architektur-Entscheidung (2026-06-13, bestätigt 2026-07-05):** RL-Loop bleibt
> Python/`gym.Env`, NICHT nach C++ portieren. Der Singleton-Constraint (SoH
> process-global) verbietet in-process-Vektorisierung → Parallelität ist so oder
> so multi-process (M9). Heiße Pfade sind schon nativ; was noch nativ werden muss,
> ist der Observation-Pfad (M8.6), nicht der RL-Loop.

### M8.5 — Härtung & API-Konsolidierung (aus Architektur-Review 2026-07-05)

> Alles hier ist JETZT billig und wird mit jedem weiteren Env teurer. Vor dem
> nächsten Env erledigen.

- [x] **Eine rc-Konvention für die gesamte C-API:** 0 = Erfolg, negative
      Fehlercodes. `SoH_GetGameState`/`SoH_WarpTo*` auf 0=Erfolg umgestellt
      (Overlay); "kein PlayState/Player" bleibt ein legitimer Zustand über das
      `valid`-Feld (rc bleibt 0). `SoH_SaveState`/`SoH_LoadState` mappen die
      positiven `SaveStateReturn`-Codes auf negativ (`-rc`, z.B. `-4` =
      `FAIL_WRONG_GAMESTATE`/Dialog offen — patch `0004`). Alle pybind-Bindings
      einheitlich über `check()` → Exceptions; `warp_to`/`warp_to_entrance`
      werfen jetzt statt rc zurückzugeben. Konvention zentral im Header
      dokumentiert.
- [ ] **Actor-Count-Workaround hinter die Savestate-API ziehen:**
      Snapshot/Restore der ActorDB-`numLoaded`-Counter in `SoH_SaveState`/
      `SoH_LoadState` kapseln (pro Slot); `snapshot_actor_counts`/
      `restore_actor_counts` aus der Python-API entfernen. Prinzip: künftige
      Savestate-Lücken-Fixes (RNG, Mx) ebenfalls in der C-Schicht kapseln —
      nie als gepaarte Aufrufe, die jeder Caller kennen muss.
- [ ] **`runtime.py` + `SoHEnv`-Basisklasse:** ein Modul besitzt den
      Prozess-Singleton (init, frame-dims-Polling, shutdown/refcount statt
      `close()` mit globaler Fernwirkung); eine Basisklasse kapselt das
      Anchor-Muster (Boot → Warp → Settle → `save_state`) inkl. Dialog-Recovery
      (B+START) im `reset()`. Konkrete Envs definieren nur noch Task:
      Actions, Reward, Terminierung.
- [ ] **Packaging:** `pyproject.toml` (scikit-build-core), Paket `oot_rl` mit
      der nativen Extension als `oot_rl._native` (löst das
      `oot_rl_gym`-Shadowing-Workaround strukturell), `pip install -e .`.
      Test-Skripte → pytest-Suite mit **Prozess-pro-Test** (Singleton!),
      über CTest aufrufbar.
- [ ] **CI:** Workflow, der `apply_patches.sh` + Build gegen die gepinnten
      Submodule laufen lässt — die Patch-Serie (15 Patches) ist die fragilste
      Stelle des Projekts; Brüche müssen vor dem Merge auffallen.
- [x] **Aufräumen:** `SoH_BootToGameplay()` entfernt (Warp ist der Einstieg,
      true ≠ steuerbar, §6); der Smoke-Test steppt jetzt lokal bis
      `GameState.valid`. `[pybind11-debug]`-Messages aus dem CMakeLists entfernt.

### M8.6 — Step-Pfad-Performance & Grafik-Konfiguration

> Single-Process-Gewinne multiplizieren sich in M9 mit 64–80. Reihenfolge:
> ERST die Profiling-Zerlegung, DANN gezielt optimieren — nicht raten.

- [ ] **Profiling-Zerlegung pro Step:** Zeitanteile Spiellogik / Audio / Raster /
      Readback / Python-Overhead. Etabliert die Leitmetrik-Baseline.
- [ ] **Audio-Synthese im Headless komplett skippen.** Patch `0002` skippt nur
      das SDL-Device — Sequenz-Playback + Mixing (N64-Audio-DSP-Emulation) laufen
      pro Frame weiter und sind im Training reine Verschwendung. Kurzschluss vor
      der Synthese (AudioMgr-Pfad), nicht erst vor der Ausgabe.
- [ ] **`step_frames(n, render_last_only=True)`:** Bei frame_skip=n braucht nur
      der letzte Frame Pixel; OoT-Logik liest (bis auf seltene
      Framebuffer-Effekte — dokumentieren) nie aus dem Framebuffer. Zwischenframes
      ohne Fast3d-Rasterisierung steppen. Reduziert zugleich die
      Python↔C-Roundtrips auf 1 pro Aktion.
- [ ] **Readback-Pfad auf die GPU:** Downscale (+ optional Grayscale) per Blit in
      ein kleines FBO, Readback dann z.B. 84×84×1 (~7 KB) statt synchronem
      `glGetTexImage` über 640×480×3 (~900 KB, `FrameCapture.cpp`). Löst
      Readback-Stall, CPU-Resize und die M9-IPC-Payload in einem. Optional
      zusätzlich PBO-Double-Buffering.
- [ ] **Grafik-Konfiguration pro Prozess** (fix bei `init()`, kein
      Laufzeit-Umschalten nötig) — als ein Config-Struct/Setter-Satz nativ, in
      Python als Env-Konstruktor-Parameter:
      - interne Renderauflösung (N64-nativ 320×240 oder weniger — 640×480
        rastern ist Verschwendung, wenn die Policy 84×84 sieht),
      - Capture-Auflösung (GPU-Downscale-Ziel) und RGB vs. Grayscale,
      - frame_skip nativ (Parameter von `step_frames`),
      - Frame-Capture **komplett aus** für Envs ohne Pixel-Observations — dann
        entfallen Rastern + Readback ganz (State-only-Envs steppen um
        Größenordnungen schneller),
      - Platz für weitere Schalter (Texturfilter, AA, …) im selben Struct.
- [ ] **Flexibler State-Readout als Infrastruktur:** `SoH_GetGameState`
      erweiterbar halten bzw. generische Feld-Accessoren ergänzen, damit Envs
      ihre Observations frei aus RAM-State und/oder Pixeln mischen können —
      welche Observations sinnvoll sind, ist Forschungsfrage, nicht Infrastruktur.

### M9 — Parallelisierung

- [ ] **Worker-Protokoll:** ein SoH-Prozess pro Env (Singleton-Constraint,
      Threads unmöglich); außen ein VecEnv-artiges Interface. **Asynchrones
      Stepping** (EnvPool-Stil: Learner konsumiert fertige Batches, wartet nicht
      auf den langsamsten Env — Resets/Scene-Loads streuen stark; bei 64–80
      Prozessen dominiert sonst der Straggler).
- [ ] **Shared-Memory-Ringpuffer** für Observations (kein Pickling über Pipes);
      mit M8.6-Downscale sind das ~7 KB/Step.
- [ ] **GPU-Zuteilung:** `EGL_PLATFORM_DEVICE_EXT` pro Prozess;
      Sättigungs-Benchmark Envs-pro-GPU **inklusive Learner-Konkurrenz** (der
      Learner braucht dieselben 2 GPUs).
- [ ] **Gegenexperiment Software-GL (llvmpipe)** auf den ~100 Cores: bei
      320×240-Raster und 84×84-Obs ist die Rasterlast winzig — gut möglich, dass
      CPU-Rendering besser skaliert und beide GPUs exklusiv dem Learner gehören.
      Ein Benchmark-Tag, potenziell großer Payoff.
- [ ] **Speicher-Footprint messen** (N × Heap + Assets + GL-Kontext; Assets
      werden pro Prozess geladen) und **CPU-Pinning** der Worker.
- [ ] **Abnahmekriterium:** aggregierte Steps/s mit 64–80 Envs auf der
      Zielhardware, belegt gegen die M8.6-Baseline (Leitmetrik).

### Mx — Sauberes, deterministisches Env-Authoring (Scenario-System) — SPÄTER

> Eigener Milestone. Ziel: zitierfähige, bit-reproduzierbare Env-Startzustände.
> Zusätzliche Motivation (2026-07-05): vollständige Savestates + expliziter
> RNG-Seed sind auch die Voraussetzung für **reproduzierbare Anker-Restores** —
> Infrastruktur, auf der Forschungsideen (z.B. Anker-Archive, Startzustands-
> Curricula) aufsetzen können, ohne sie hier vorwegzunehmen.
> Abnahmekriterium: bit-identischer Replay über N Steps nach `load_state`.

- [ ] **Zwei-Artefakt-Modell:** *Rezept* (deklarativ, portabel: SaveContext/Items
      via Randomizer-Setter + `Sram_InitDebugSave`, Entrance/Warp **oder**
      Cutscene-Einstieg, **expliziter RNG-Seed**) + *Snapshot* (frame-genauer
      SoH-Savestate als Laufzeit-Anker).
- [ ] **Savestate vollständig machen — RNG zuerst:** `rngSeed` ist ein TOTES Feld
      (nie geschrieben/gelesen); `sRandInt`/`sRandFloat` sind `static` in
      `code_800FD970.c` → liegen NICHT im erfassten `gSystemHeap`. Boot deterministisch
      seeden (`Rand_Seed`, statt `osGetTime()` in `z_play.c:539`) + RNG in den
      Savestate aufnehmen (Accessor nötig). Kapselung in `SoH_SaveState`/
      `SoH_LoadState`, wie beim Actor-Count-Fix (M8.5-Prinzip).
- [x] **ActorDB `numLoaded` (dieselbe Gap-Klasse, aber CRASH)** — bereits gelöst:
      Overlay `SoH_ActorCounts.cpp` snapshottet/restored die Counter um save/load.
      Verifiziert (50 Zyklen). Wird in M8.5 in die Savestate-API gekapselt.
- [ ] **RNG-Politik per Switch:** fester globaler Seed (max. Reproduzierbarkeit)
      ODER kontrollierter Pro-Episode-Seed (Generalisierung) — beides möglich.
- [ ] **Cutscene-Starts** (z.B. Frame 0 der Intro — enthält Lern-relevante Infos):
      Cutscenes laufen in der PlayState → `save_state` erlaubt; per Frame-Step exakt
      positionieren.
- [ ] **Kamera-nach-Intro:** KEIN Savestate-Bug — OoT-Save-Semantik. Direkt nach
      der Intro hängt die Kamera oben; nach in-game Save+Load ist sie woanders,
      gesteuert über einen Save-Flag (Lead: `sceneSetupIndex`/`cutsceneIndex` bzw.
      `respawn`/`respawnFlag`-Pfad in `Warping.cpp`). Hier ansetzen, nicht im Snapshot.
- [ ] **Authoring-Tool:** Python-Driver (boot, seed, frame-step, snapshot, Rezept
      als JSON) + SoHs **eigene ImGui-Debug-Windows** im DebugView gesurfaced
      (`debugSaveEditor`, `actorViewer`, `valueViewer`, `colViewer`) statt eigenem
      Inspector. Braucht Modus-Toggle: Authoring = ImGui an (Patch `0006` schaltet
      es im Headless-Pfad ab), Training = ImGui aus.

> **Empfohlene Reihenfolge:** M8-Baseline notieren → M8.5 (Härtung, vor dem
> nächsten Env) → M8.6 (Step-Pfad + Grafik-Config) → M9 (Parallelisierung).
> Mx nach Bedarf, spätestens wenn Reproduzierbarkeit für Experimente gebraucht
> wird — der RNG-Teil ist davon der wichtigste und kann vorgezogen werden.
>
> **Zurückgestellt (Task-Design, nicht Infrastruktur):** Reward-Shaping /
> Tür-Koordinate, Folge-Tasks Kokiri Forest → Mido.

## 8. Technische Pitfalls — unbedingt wissen

### `LUS_APP_BUNDLE_PATH` — der Pfad-Override

libultraship nutzt unter Linux ausschliesslich `/proc/self/exe` zur Bestimmung
des App-Verzeichnisses (`Context.cpp`, `__linux__`-Branch). Im Python-Embedding
zeigt das auf den Python-Interpreter — SoH findet `oot.o2r` dort nicht und kippt
in den interaktiven Asset-Extractor (endloser Splash, 330% CPU, hängender Prozess).

Patch in `patches/libultraship/`: vor `readlink("/proc/self/exe", ...)` wird
`getenv("LUS_APP_BUNDLE_PATH")` geprüft. **Vor jedem `init()` setzen:**
```python
os.environ["LUS_APP_BUNDLE_PATH"] = "/abs/path/to/build-cmake"
# oder:
soh.set_app_path("/abs/path/to/build-cmake")
```

### VAO im OpenGL Core Profile

Vanilla-SoH erzeugt VAO nur auf Apple/GLES. Im EGL-Core-Profile auf Desktop-Linux
braucht's aber ein gebundenes VAO, sonst werden alle `glDrawArrays`/`glDrawElements`
still verworfen → schwarzes FBO. Fix in `patches/libultraship/`,
`gfx_opengl.cpp::Init()`.

### Frame-Orientierung

Im FBO-Pfad ist der Frame **bereits top-down** (`opengl_invertY=true` in Fast3d).
**Kein `np.flipud` nötig.** Direkt fütterbar in CNN-basierte Policies.

### Field-Name-Quirks

- GameState-Feld heisst **`scene_id`**, NICHT `scene` — das Modul-`__repr__`
  täuscht das vor (`GameState(... scene=...)`), aber Attribut-Zugriff geht nur
  über `scene_id`.
- **`gs.valid`** heisst "GameState-Struct ist befüllt", NICHT "im Gameplay-Modus".
  Auch im File-Select gilt `valid=True`.

### Build-Konfiguration

- `set(CMAKE_POSITION_INDEPENDENT_CODE ON)` MUSS oben in Top-Level-CMakeLists,
  vor `add_subdirectory(external/Shipwright)`.
- `pybind11_DIR` per `execute_process(python -m pybind11 --cmakedir)` bestimmen,
  NICHT über `CMAKE_PREFIX_PATH` (Shipwright filtert das, siehe §4).
- `BUILD_AS_LIB=ON` macht libultraship + soh zu statischen `.a`'s. Das pybind11-
  Modul linkt diese in eine `.so`.

### Snapshot-Mechanik

- RAM-only, nicht persistent across processes.
- Funktioniert NUR im "GamePlay"-Mode (nicht in Title, File-Select, Pause,
  Cutscenes, Dialog-Boxen, Item-Screen).
- `load_state` aus einem Dialog heraus failt mit rc=4. Recovery via
  B+START-Drücken reicht für Dialoge/Pause-Menüs, aber **NICHT** für File-Select.

### Save-File vs. Save-State

| Mechanismus              | Typ            | Persistent?   | Anwendung                  |
|--------------------------|----------------|---------------|----------------------------|
| `file1.sav` (in `data/`) | OoT-Save-File  | ja, Disk      | Boot-Anker beim File-Select|
| `save_state(slot)`       | SoH-RAM-State  | nein, in-proc | Episode-Reset (Determinismus) |

### Action-Set für RL

Mit `BTN_A` im Action-Vokabular kann der Agent in Dialog-Boxen geraten und dann
ist Save/Load blockiert (`load_state` failt mit rc=4). **Korrektur (2026-06-13):
auch in Link's Haus (Scene 52) ist A NICHT safe** — beim PPO-Smoke triggerte A
eine Text-Box (textId 0x22a). Endet eine Episode in einem offenen Dialog, failt
der Reset-`load_state`. Mitigation: A aus dem Action-Set nehmen ODER Dialog-Recovery
(B+START) im `reset()` robust machen. (Kokiri Forest = Scene 0x55 = 85, NICHT 81 —
81 ist Hyrule Field.)

## 9. Dev-Workflow

### Patch-Management (Zwei-Eimer-Modell, Details in `patches/README.md`)

- **Eigene neue Files** (z.B. `soh_api.c`, `FrameCapture.cpp`) → liegen in
  `overlay/` im Hauptrepo und werden von `scripts/link_overlay.sh` ins Submodule
  **symlinkt**. Zum Ändern einfach `overlay/` editieren + neu bauen — KEIN
  Dev-Mode, KEINE Patch-Regenerierung.
- **Edits an Upstream-Files** → als `git format-patch` in `patches/`, NICHT direkt
  ins Submodule committen. Nur diese brauchen Dev-Mode (`scripts/start_dev.sh`)
  zum Editieren und `scripts/export_patches.sh` zum Zurückschreiben.
- Bei Submodule-Update auf neueren Upstream: nur die `patches/` einzeln
  re-applien und auf Konflikte prüfen — `overlay/` bleibt aus dem Konfliktpfad.

### Test-Reihenfolge nach Build

1. `ctest --test-dir build-cmake -R smoke_test` (oder `./build-cmake/smoke_test`)
   — wenn das hängt, ist's nicht python-spezifisch.
2. `python ../python/test_native.py` — smoke-test der Bindings.
3. `python ../python/find_loading_zone.py` — Env-Prerequisites (aktuell der Blocker).

### Debug

- Hängende Prozesse: `gdb -batch -p $(pgrep -f <process>) -ex "thread apply all bt"`
- Stack-Trace-Hinweise:
  - `OTRGlobals::RunExtract` = Asset-Extractor (typisch: `oot.o2r` fehlt oder
    falscher App-Path)
  - `BS::thread_pool::worker` an einer CV = idle worker (normal)
  - `Fast3d::Interpreter::RunGuiOnly` = die Engine rendert nur GUI / Splash
- `PYTHONUNBUFFERED=1 python -u ...` für vollständige Logs ohne Buffering.

## 10. Glossar

| Begriff       | Bedeutung                                                              |
|---------------|------------------------------------------------------------------------|
| SoH           | Ship of Harkinian, OoT decompilation + native renderer                 |
| LUS           | libultraship — SoH's Renderer/Audio/Input-Lib, getrennt vom Game-Code  |
| OTR / o2r     | SoH's Game-Asset-Container-Format                                      |
| `oot.o2r`     | aus N64-ROM extrahierte Game-Daten                                     |
| `soh.o2r`     | SoH's eigene Resources                                                 |
| Fast3d        | SoH's N64-RDP-Emulation, der von uns genutzte Renderer-Pfad            |
| FBO           | Framebuffer Object — Off-Screen-Render-Target für headless + capture   |
| Savestate     | RAM-Snapshot des kompletten Game-Zustands (System Heap + Audio + Save Context) |
| Scene         | OoT's Begriff für Raum/Outdoor-Bereich. Identifiziert über `scene_id`. |

---

**Bei Übernahme**: Lies §7 (Roadmap — dort steht, was als nächstes dran ist und
warum) und §8 (Pitfalls) zuerst. Das Trainings-Env läuft end-to-end; die
anstehende Arbeit ist Härtung (M8.5), Step-Pfad-Performance (M8.6) und
Parallelisierung (M9).
