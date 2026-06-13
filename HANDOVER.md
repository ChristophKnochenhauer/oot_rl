# OoT RL — Project Handover

> **Status (Snapshot Ende M7 Baustein 2):** Native Bindings funktionieren end-to-end
> (Build, Init, Frame-Capture, Save/Load). Erste Env-Klasse `LeaveHouseEnv` ist
> geschrieben. Offener Blocker für M7 Baustein 3: der Boot landet in der
> Title-Attract-Demo (Scene 81), nicht in agenten-steuerbarem Gameplay — und
> `gs.valid` unterscheidet die beiden nicht. Siehe §6.

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
│   ├── oot_rl/envs/leave_house.py   # erstes Trainings-Env (WIP)
│   ├── find_loading_zone.py         # Helper zur Door-Position-Ermittlung (WIP)
│   ├── test_native.py               # Smoke-Test der Bindings
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

## 6. Aktueller Blocker (was als nächstes zu tun ist)

**Boot landet in der Attract-Demo, nicht in agenten-steuerbarem Gameplay.**

Mit neutralem Input lädt die Engine `data/file1.sav` automatisch und wird um
Frame ~231 in **Scene 81 (Kokiri Forest)** `valid`. Aber die wild umher-
teleportierenden Link-Positionen zeigen: das ist die **Title-Screen-Attract-
Demo**, kein steuerbares Gameplay. `save_state`/`load_state` funktionieren dort
(Engine ist im GamePlay-Mode), daher "bestehen" smoke_test und `test_native.py`,
während sie auf der Demo operieren.

> **Wichtig:** `gs.valid == true` heisst NUR "GameState-Struct ist befüllt",
> NICHT "Agent steuert Link". Das ist der Kern des Blockers.

### Was NICHT mehr gilt (korrigiert ggü. älteren Notizen)

- Der Boot bleibt **nicht** im File-Select hängen — er läuft in die Attract-Demo.
- Die alte `A/START/A/A`-Button-Sequenz aus `hello.cpp` macht es **schlechter**:
  START während der laufenden Demo kickt RAUS in den File-Select, wo `valid` nie
  wahr wird. Deshalb steppt der native `SoH_BootToGameplay()` (in `soh_api.c`) nur
  bis `valid` und versucht bewusst NICHT, Demo von echtem Save-Load zu unterscheiden.

### Was zu tun ist

1. **Echtes Gameplay von der Attract-Demo unterscheiden.** Das ist der offene
   Kern. Kandidaten: Demo-/Title-State-Flag im Game-Context lesen (statt nur
   `valid`), oder eine Bedingung finden, unter der `file1.sav` als
   *steuerbarer* Spielstand geladen wird (nicht als Demo). Scene-Id allein
   reicht nicht — die Demo läuft selbst in Scene 81.

2. **Snapshot erst nach echtem Gameplay.** SoH-Savestates sind RAM-only und nach
   jedem Prozess-Start leer. Sobald (1) gelöst ist: einmalig `soh.save_state(0)`
   als Reset-Anker, dann erst in die Episode-Loop. (Aktuell ruft
   `find_loading_zone.py` direkt `load_state(0)` auf einen leeren Slot → rc=4.)

3. **`SoH_BootToGameplay()` durch Python exponieren.** Existiert nativ in
   `soh_api.c`, ist aber noch nicht im pybind11-Modul gebunden — sobald (1)
   geklärt ist, hier die verifizierte Sequenz kapseln, damit C++ und Python
   exakt denselben Boot teilen.

## 7. Roadmap

### M7 Baustein 3 — Gymnasium-Env, GROBE Variante (aktueller Fokus)

> Bewusste Entscheidung (2026-06-13): jetzt reicht **grobes** Env-Bauen. Der
> wissenschaftlich saubere, voll-deterministische Weg ist ein eigener späterer
> Milestone (siehe unten). Hier nur so viel Determinismus wie nötig, um Envs zum
> Laufen zu bringen.

- [ ] Steuerbares Gameplay per **Warp** betreten statt auf Attract-Demo zu hoffen:
      `SoH_WarpTo(entranceId, roomNum, x, y, z, rotY)` (dünner Forward auf
      `Warp()`/`WarpPoint` aus `Enhancements/Warping.cpp`) in `soh_api.c` + pybind.
      Evtl. `WarpPoint.bootToPoint` nutzen → umgeht die Demo-Frage ganz.
- [ ] `save_state(0)` als groben Reset-Anker nach dem Warp (RNG/Kamera noch nicht
      garantiert deterministisch — bewusst akzeptiert in dieser Phase).
- [ ] `LeaveHouseEnv` auf diesem Warp-Anker smoke-testen (zufällige Policy)
- [ ] `LeaveHouseEnv.reset` mit Dialog-Recovery (B+START für Menü-Aussteuerung)

### Mx — Sauberes, deterministisches Env-Authoring (Scenario-System) — SPÄTER

> Eigener Milestone. Ziel: zitierfähige, bit-reproduzierbare Env-Startzustände
> für wissenschaftliches RL. Design aus der Discussion 2026-06-13:

- [ ] **Zwei-Artefakt-Modell:** *Rezept* (deklarativ, portabel: SaveContext/Items
      via Randomizer-Setter + `Sram_InitDebugSave`, Entrance/Warp **oder**
      Cutscene-Einstieg, **expliziter RNG-Seed**) + *Snapshot* (frame-genauer
      SoH-Savestate als Laufzeit-Anker).
- [ ] **Savestate vollständig machen — RNG zuerst:** `rngSeed` ist ein TOTES Feld
      (nie geschrieben/gelesen); `sRandInt`/`sRandFloat` sind `static` in
      `code_800FD970.c` → liegen NICHT im erfassten `gSystemHeap`. Boot deterministisch
      seeden (`Rand_Seed`, statt `osGetTime()` in `z_play.c:539`) + RNG in den
      Savestate aufnehmen (Accessor nötig).
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

### M8 — Erstes Training

- [ ] PPO oder DQN drumherum bauen. Optionen: **CleanRL** (single-file,
      transparent) oder **Stable-Baselines3** (battery-included). User-Präferenz
      steht aus.
- [ ] Frame-Stack / Resize / Grayscale via `gymnasium.wrappers`
- [ ] Erstes Training auf "Verlasse Link's Haus" — sollte in Minuten lernbar sein
- [ ] Folge-Tasks: Kokiri Forest → Mido (komplexere Navigation, NPC-Dialog
      Vermeidung)

### M9 — Parallelisierung

- [ ] **Multi-Process-Vector-Env** — SoH ist ein process-globaler Singleton,
      Parallelisierung NUR über Prozesse, nicht Threads
- [ ] Multi-GPU EGL via `EGL_PLATFORM_DEVICE_EXT` (Device-Index-Selection pro
      Prozess)
- [ ] Target: 64–80 parallel envs auf der Ziel-Hardware

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

Mit `BTN_A` im Action-Vokabular kann der Agent in Dialog-Boxen geraten (in
NPC-haltigen Szenen) und dann ist Save/Load blockiert. In Link's Haus (Scene 52)
sind keine NPCs → A safe. In Kokiri Forest (Scene 81) sind viele Kinder → A
entweder rausnehmen oder Dialog-Recovery robust machen.

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

**Bei Übernahme**: Lies §6 zuerst. Der Pfad zum laufenden Trainings-Env ist
mechanisch — Boot-Sequenz portieren, snapshot, dann fliegen die Episoden. Alles
darunter ist erledigt und gut getestet.
