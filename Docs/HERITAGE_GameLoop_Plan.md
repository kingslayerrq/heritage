# Heritage — Full Game Loop Vertical Slice

Implementation plan for Codex. Builds on the HERITAGE-1 interaction prototype already in `Source/Blank/Heritage/` (read that code and `Docs/HERITAGE-1_Implementation_Plan.md` first; its hard constraints in §3 still apply: C++ and `.ini` only, no `.uasset`/`.umap`, engine basic shapes, Canvas HUD, runtime-created input, engine headers are ground truth).

## 1. The loop being built (GDD §1.4.1, §3.1, §3.3)

```
[Reality] explore the aged room, scan mementos with the camera
   → pick up a film reel
[UI] Map + film rack → choose the reel → enter it
[Film] Play Mode: watch the past as a bystander (step-frame playback, E forward, Q rewind, Space pause)
       Pause Mode: the scene becomes interactable; act out what you think happened
[Code] player behaviour is detected and recorded against the Film Segment ("imaging")
[UI] Archive → Workbench: compose the missing frame (Scene / Audio / Light) → upload
       key frame must be the correct answer; uploaded frames are locked
   → reel restored → later frames become readable → new knowledge unlocks an object in Reality
[Reality] use that knowledge → progress; a leaf frame optionally reveals a second reel
```

GDD rules this slice must respect:

- Investigating the past never changes Reality. It only changes what the player (and the catalogue) *knows*.
- Play Mode and Pause Mode keep separate scene state. What the player does while paused lives in its own layer and does not alter playback. The player can roll that layer back.
- Key Scene: only the correct reconstruction uploads. Leaf Scene: optional, enriches, may reward.
- Scanning an unknown object in Reality is what makes it interactable.
- Reduce text; feedback through shape, colour and short prompts.

Out of scope: more than one room, more than one playable reel, real art/audio assets, save/load, NPC AI, mouse-driven UI (keyboard only), localisation beyond `NSLOCTEXT`.

## 2. The slice, concretely

One room, "The Study", existing twice in the same world: **Reality** (now, abandoned) and **Film** (the past, Reel 1). No level streaming: two `AHeritageDemoRoom` instances, Reality at world `(0, 0, 50)`, Film at `(0, 4000, 50)`; entering a reel teleports the player to the same room-local position in the other room.

### Reel 1 — "The Study, an evening" (one Film Segment, bound to the key, 8 Scenes)

| # | Kind | Canonical content |
|---|---|---|
| 1 | Readable | Fire lit, key glinting in the hearth. Butler stands inside the door. Kettle on the table. Vase intact on its pedestal. |
| 2 | Readable | Butler at the table, kettle at his hand. |
| 3 | Readable, **Damaged Area (leaf)** over the pedestal | Butler at the basin, kettle `Filled`. |
| 4 | Readable | Butler in front of the fireplace with the full kettle. |
| 5 | **Missing Frame (key)** | Unknown until restored. Restored content: fire out, kettle empty on the hearth floor, butler holding the key. |
| 6 | Locked until 5 is uploaded | Butler walking to the portrait with the key. |
| 7 | Locked until 5 is uploaded | Butler at the portrait; portrait swung aside; key behind it. |
| 8 | Locked until 5 is uploaded | Portrait back in place, butler gone, door closed. |

**Key answer** for Scene 5: the facts `FireDoused` and `KeyTaken`, exactly those two.
**Leaf** for Scene 3: any one recorded fact may be uploaded; only `ReelFound` (the player broke the vase while paused and saw a reel inside) gives a reward.

### Reality room (same layout, aged)

- Fireplace cold (not `Lit`), no butler, no letter, no books on the table except one.
- "Rusted kettle" on the floor near the basin. Vase is already broken: a static "Vase shards" prop on the pedestal.
- **Reel 1 canister** on the table (new prop, §4.4).
- **Reel 2 canister** hidden among the shards; invisible and non-interactable until the leaf reward unlocks it.
- Portrait on the wall. Until Scene 5 is uploaded, Use gives the old flavour line. After upload, Use swings it aside (slide 60 cm along the wall over 0.6 s) and reveals a **key** behind it.
- Door locked. Key opens it → `SLICE COMPLETE`.
- Reality mementos (scannable): Reel 1 canister, rusted kettle, portrait, vase shards, key. Film-room copies are **not** mementos.
- Scan-gating is **on in Reality**: a memento cannot be grabbed or used until scanned. Never applied in Film. `heritage.RequireScanToInteract` now defaults to `1` and means "in Reality".

### Player-visible flow and objective text (HUD, one line at a time)

1. "Take out the camera [Tab] and scan something of value." (first scan auto-opens the Archive once, GDD Stage 2)
2. "Scan the film reel on the table, then pick it up [F]." (pickup auto-opens the Map once, GDD Stage 3)
3. "Open the Map [M] and enter the reel."
4. "Watch. [Space] pause · [E] forward · [Q] rewind."
5. "A frame is missing. Pause and act out what happened." (shown when playback reaches Scene 5 unrestored)
6. "Imaged. Open the Archive [N], compose the frame and upload."
7. "Leave the reel [M]. Use what you learned."
8. "Open the door."

## 3. Controls

Existing bindings stay. Changes and additions (runtime Enhanced Input, same pattern as now):

| Key | Reality | Film · Play Mode | Film · Pause Mode | UI open |
|---|---|---|---|---|
| WASD / mouse | move, look | move, look (bystander) | move, look | — |
| Space | jump | **pause** | jump | — |
| E | — | hold: fast-forward ×4 | tap: resume Play | — |
| Q | — | hold: rewind ×4 | tap: resume Play, rewinding | — |
| LMB / RMB / F / Shift | grab·drop / throw / use / dash | disabled | as Reality | — |
| Tab | Camera Mode | disabled | disabled | — |
| Backspace | — | — | **roll back** this pause layer | back / cancel |
| M | open Map | open Map | open Map | close |
| N | open Archive | open Archive | open Archive | close |
| Arrow keys | — | — | — | navigate |
| Enter | — | — | — | confirm |
| R | reset level | reset level | reset level | — |

`Space` is pause only while playing, so jumping still works while paused; `E`/`Q` double as "resume". This reconciles the GDD's film keys with its movement keys. While a UI screen is open, movement, look and all verbs are ignored and playback is frozen.

## 4. Architecture

Keep the existing split (verbs → stimuli → object reactions). Add three things: a **flow subsystem** (where am I, what do I own, what is unlocked), a **film director** (timeline, modes, snapshots, takes), and **facts** (what the player demonstrably did).

### 4.1 New tags and types

Tags: `Heritage.Fact.FireDoused`, `.KeyTaken`, `.VaseBroken`, `.ReelFound`, `.ButlerShoved`, `.KettleFilled`. `Heritage.Verb.Film` (for log lines about transport/upload).

`HeritageTypes.h` additions:

```cpp
UENUM() enum class EHeritageEra : uint8 { Reality, Film };
UENUM() enum class EHeritageFilmMode : uint8 { Play, Pause };
UENUM() enum class EHeritageSceneKind : uint8 { Readable, MissingKey };
UENUM() enum class EHeritageScreen : uint8 { None, Map, Archive };

USTRUCT() struct FHeritageActorSnapshot   // one film actor at one moment
{ FName Id; FTransform RoomLocal; FGameplayTagContainer State; bool bHidden = false; };

USTRUCT() struct FHeritageSceneDef
{ EHeritageSceneKind Kind; bool bLockedUntilKeyUploaded = false; bool bHasDamagedArea = false;
  FText Caption; TArray<FHeritageActorSnapshot> Keys; };   // only actors that differ from Scene 1 need a key

USTRUCT() struct FHeritageFactRecord { FGameplayTag Fact; FText Text; int32 SceneIndex = 0; int32 PauseSession = 0; };

USTRUCT() struct FHeritageFrameEdit
{ TArray<FGameplayTag> ChosenFacts; int32 AudioChoice = 0; int32 LightChoice = 0; bool bUploaded = false; };

USTRUCT() struct FHeritageSegmentDef
{ FName ReelId; FText Title; TArray<FHeritageSceneDef> Scenes; int32 KeySceneIndex; int32 LeafSceneIndex;
  FGameplayTagContainer KeyAnswer; TArray<FHeritageActorSnapshot> RestoredKeyScene; };
```

Reel content is authored in code in `HeritageReelData.cpp` (`FHeritageSegmentDef BuildReel1()`), using room-local coordinates from the existing room layout. A scene's effective state for an actor = the latest key at or before that scene (carry forward), so most scenes need only the butler and kettle.

### 4.2 `AHeritageProp` additions (small, in the base class)

- `FName FilmId` (set by the room builder for actors the director controls: `Butler`, `Kettle`, `Key`, `Vase`, `Fireplace`, `Door`, `Portrait`, `Letter`, `VaseReel`).
- `bool bPhysicsProp` — remembered at BeginPlay: does this prop simulate when free.
- `void SetFrozen(bool)` — frozen: physics off, velocity zeroed; unfrozen: physics restored if `bPhysicsProp`.
- `virtual void RefreshFromState()` — make visuals and derived data match `Object->StateTags`. Overrides: kettle colour from `Filled`; fireplace flame/light from `Lit` (and re-run heat); breakable hidden + reel shown when `Broken`, intact otherwise; door panel angle from `Open` (snap, no animation); portrait slid aside from a new `Heritage.State.Open`; NPC letter attached unless a new `Heritage.State.Dropped` is on the letter.
- `FHeritageActorSnapshot Capture(const FTransform& Room) const` / `void Restore(const FHeritageActorSnapshot&, const FTransform& Room)` — transform (teleport physics), hidden flag, state tags, then `RefreshFromState()`.

Existing reaction code must set state tags *before* visuals so `RefreshFromState()` is the single source of truth; refactor reactions to call it rather than duplicating visual changes.

### 4.3 Facts

`UHeritageStatics::RecordFact(WorldContext, FGameplayTag Fact, FText Text)` → forwards to the film director if the player is in Film and paused; otherwise ignored (facts only come from acting in the past). Emit from the object reactions, not from player code:

- Fireplace doused by water → `FireDoused` ("Doused the fire with the kettle.")
- Key grabbed (interactor notifies the grabbed actor through a new `IHeritageReactive::OnGrabbed(AActor* By)` default-empty hook; the key records the fact only if it is not `Hot`) → `KeyTaken` ("Took the key from the hearth.")
- Kettle filled → `KettleFilled`; vase broken → `VaseBroken`; vase reel revealed → `ReelFound` ("A film reel was hidden in the vase."); butler staggered → `ButlerShoved`.

A fact is recorded once per pause session (no duplicates).

### 4.4 New props (`HeritageProps.h/.cpp`)

- `AHeritageReelCanister` — flat cylinder 22×22×6, dark metal with a coloured top disc; static; `bMemento`; `FName ReelId`. `Use` (empty hands) → `Flow->CollectReel(ReelId)`, hide self, toast "Added to the film rack."
- `AHeritageShards` — a few small static cubes on the pedestal; `bMemento`; not grabbable.
- Portrait: add the Reality behaviour in §2 (only when `Flow->IsKeySceneUploaded()` and era is Reality); it owns a hidden `AHeritageKey` it reveals.

### 4.5 `UHeritageGameFlowSubsystem` (`UWorldSubsystem`)

Owns global slice state. No UI drawing, no input handling.

- `EHeritageEra Era`; `AHeritageDemoRoom* RealityRoom, *FilmRoom`; `AHeritageFilmDirector* Director`.
- Rack: `TArray<FName> OwnedReels`; `CollectReel(FName)`; `bool OwnsReel(FName)`.
- `EnterFilm(FName ReelId)` (only Reel 1 is playable; Reel 2 → toast "This reel cannot be read yet."): remember era, teleport the pawn to the same room-local transform in the Film room, set film post-process, `Director->BeginVisit()`. `LeaveFilm()`: `Director->EndVisit()`, teleport back, Reality post-process.
- `EHeritageScreen OpenScreen`; `ToggleScreen(EHeritageScreen)`; `CloseScreen()`.
- `bool CanUseVerbs() const` (no screen open, and not Film+Play); `bool CanUseCamera() const` (Reality, no screen); `bool CanMove() const` (no screen).
- `bool RequiresScanHere() const` (cvar on and era Reality).
- Unlock flags: `bKeySceneUploaded`, `bLeafRewardUnlocked` (reveals the Reel 2 canister in Reality), `bSliceComplete`.
- Tutorial: `int32 ObjectiveStage` (1–8, advanced by events: first scan, reel collected, film entered, missing frame reached, imaged, uploaded, back in Reality with knowledge, door open) and `FText GetObjectiveText()`. One-shot auto-open flags for Archive (first scan) and Map (first reel).
- Post-process: applied to the character camera's `PostProcessSettings` with blend weight 1. Reality: saturation ≈ 0.55. Film: saturation ≈ 0.25, warm `ColorGain` tint, `FilmGrainIntensity` ≈ 0.6, `VignetteIntensity` ≈ 0.8. Values as `UPROPERTY`s. Verify the exact `FPostProcessSettings` member and override-flag names in the 5.8 header.

The interactor and character must ask the flow subsystem (`CanUseVerbs`, `CanUseCamera`, `CanMove`, `RequiresScanHere`) instead of deciding themselves. `UHeritageObjectComponent::CanBeGrabbed` uses `RequiresScanHere()`; the interactor's Use path applies the same scan gate to mementos in Reality with the same "Scan this first with the camera." reason.

### 4.6 `AHeritageFilmDirector` (actor, spawned by the game mode, owns Reel 1 in the Film room)

State: `FHeritageSegmentDef Segment`; `TMap<FName, AHeritageProp*> Actors` (registered by the Film room after it builds); `int32 SceneIndex` (0-based internally, shown 1-based); `EHeritageFilmMode Mode`; `float FrameTimer`; `int32 PlayDirection` (+1/−1); `float Rate` (1 or 4); take data: `TArray<FHeritageFactRecord> TakeFacts`, `int32 PauseSession`, `bool bImaged`; edits: `FHeritageFrameEdit KeyEdit, LeafEdit`; pause layer: `bool bHasLayer; int32 LayerScene; TArray<FHeritageActorSnapshot> Layer`.

Deterministic API (used by input, HUD and the self-test alike):

- `BeginVisit()` → `SetScene(0)`, Play Mode. `EndVisit()` → if paused, save the layer; freeze everything.
- `int32 LastPlayableScene() const` — the key scene index if not uploaded (playback cannot pass a missing frame), else the last scene.
- `SetScene(int32)` — clamp to `[0, LastPlayableScene()]`; apply the canonical state: for every registered actor, restore the carried-forward snapshot, `SetFrozen(true)`. If the scene is the unrestored key scene, apply Scene 4's world state but hide the butler and set `IsOnMissingFrame()`. If the key frame is uploaded, Scene 5 uses `RestoredKeyScene`, and the room light uses `KeyEdit.LightChoice`.
- `StepScene(int32 Delta)`; `Tick`: in Play Mode with no screen open, accumulate `FrameTimer`; every `FrameInterval (1.5 s) / Rate` call `StepScene(PlayDirection)`. Reaching either end stops there (stays in Play Mode, timer idle). The step is a hard cut, like a slideshow (GDD "定格").
- `SetTransport(bool bForwardHeld, bool bRewindHeld)` — Play Mode: forward held → +1 at ×4; rewind held → −1 at ×4; neither → +1 at ×1.
- `Pause()` — Mode = Pause, `++PauseSession`. If `bHasLayer && LayerScene == SceneIndex`, restore the layer; otherwise start from the canonical state already applied. Then `SetFrozen(false)` on all props. If on the unrestored missing frame, the working state is Scene 4's (the last known moment). Toast once per visit: "You act in the butler's stead."
- `ResumePlay(int32 Direction)` — capture the layer (`bHasLayer = true`, `LayerScene = SceneIndex`), Mode = Play, re-apply canonical `SetScene(SceneIndex)`.
- `Rollback()` — paused only: discard the layer, remove facts whose `PauseSession` is the current one, recompute `bImaged`, re-apply canonical state and unfreeze. Toast "Rolled back."
- `RecordFact(...)` — paused only; dedupe within the session; then `bImaged = TakeFacts contains all of Segment.KeyAnswer`; on the transition to imaged: toast "Imaging successful." and notify the flow subsystem.
- Archive API: `TArray<FHeritageFactRecord> GetDistinctFacts()`; `ToggleFact(bool bKeyFrame, FGameplayTag)`; `CycleAudio/CycleLight(bool bKeyFrame, int32 Delta)`; `bool CanEdit(bool bKeyFrame, FText& Why)` (key: needs `bImaged`; leaf: needs ≥ 1 fact; neither if already uploaded); `bool Upload(bool bKeyFrame, FText& OutResult)`.
  - Key upload succeeds only if `ChosenFacts` as a set equals `KeyAnswer`. Failure: "The frames do not fit together." and nothing is consumed. Success: `bUploaded = true`, flow `bKeySceneUploaded = true`, "Frame restored. The rest of the reel is readable."
  - Leaf upload needs exactly one chosen fact. Always succeeds. If it is `ReelFound`: flow `bLeafRewardUnlocked = true`, "You know where to look now." Otherwise: "Your account is on the reel."
  - Uploaded frames are immutable.
- Audio choices (caption only, no sound assets): Silence / Hiss of steam / Crash / Low laughter. Light choices: Firelight (warm) / Cold dark (dim blue) / Daylight (bright neutral). The chosen audio caption is shown on the HUD while that scene is on screen during playback; the chosen light is applied to the Film room's main light for that scene.
- Read-only getters for the HUD: scene count, per-scene status (`Readable`, `Missing`, `Restored`, `Locked`, plus damaged-area and leaf-uploaded flags), current index, mode, rate/direction, `bImaged`, whether recording (paused), current caption.

### 4.7 Room builder

`AHeritageDemoRoom` gets `EHeritageEra Era` (set through deferred spawn before `Build()`), and builds the shared shell plus era-specific contents (§2). The Film room registers its controlled actors with the director by `FilmId`. Film room layout change from HERITAGE-1: the kettle starts on the table (Scene 1), not on the cabinet; books stay on the table and one goes on the cabinet top so the shake reaction still shows. Give each room a main light it exposes (`GetMainLight()`) so the director can apply light choices.

Game mode `StartPlay`: spawn both rooms, spawn the director, hand all three to the flow subsystem, place the player in the Reality room, apply Reality post-process.

### 4.8 Input and character

Add actions: `Map (M)`, `Archive (N)`, `Forward (E)`, `Rewind (Q)`, `Back (Backspace)`, `Confirm (Enter)`, `NavUp/Down/Left/Right (arrow keys)`. Route them in the character to a small `UHeritageUIController` object (or directly to the HUD's screen logic; choose one place and keep all screen navigation state there, not scattered):

- Space: `Flow` in Film + Play → `Director->Pause()`; otherwise jump.
- E / Q: in Film + Play → transport held state (bind `Started` and `Completed`); in Film + Pause → on `Started`, `ResumePlay(+1 / −1)`.
- Backspace: screen open → back; Film + Pause → `Rollback()`.
- `Move`/`Look`/verbs early-out on `!Flow->CanMove()` / `!Flow->CanUseVerbs()`. Entering Play Mode drops whatever is held.

### 4.9 HUD (Canvas, keyboard-driven)

Always: objective line (from the flow subsystem), toast, prompt, action log (keep), era badge top-left (`REALITY` / `FILM · Reel 1`).

Film overlay: a filmstrip along the bottom — one cell per scene, numbered, coloured by status (readable / current / missing with a hatched or red cell / locked dark / restored green), a small notch on the leaf scene; mode label `PLAY ▸`, `PLAY ▸▸ x4`, `◂◂ REWIND x4`, or `PAUSE ● REC`; the scene caption; on the unrestored missing frame a large "FRAME MISSING" plate; a few random thin vertical scratch lines and a brief dark flash on each scene step to sell the film look. Use only glyphs the default engine font has; fall back to ASCII (`>`, `>>`, `<<`, `||`, `*REC`) if unsure.

Map screen (M): left panel, a top-down schematic of the Study (rectangle with door, fireplace, table marked) and the current era; right panel, the film rack: owned reels with status text ("Reel 1 — 4 of 8 frames readable, 1 missing", "Reel 2 — unreadable"). Up/Down selects, Enter: in Reality → enter the selected reel; in Film → the list has a first entry "Leave the reel (return to Reality)".

Archive screen (N), two tabs switched with Left/Right when focus is on the tab bar (keep the navigation model simple and documented in a footer line):

- **Notebook**: scanned mementos with their note and state at capture (existing archive entries).
- **Workbench**: the filmstrip again; two editable rows, "Frame 5 (missing)" and "Frame 3 (damaged area)". Selecting one opens its editor: a checklist of the distinct recorded facts (Enter toggles), an Audio row and a Light row (Left/Right cycle), and an Upload button. Upload asks once "Uploaded frames cannot be changed. Upload? [Enter] yes  [Backspace] no" (GDD Stage 2). Show `CanEdit`'s reason when editing is not available.

Screen open ⇒ dim the world with a translucent black quad.

### 4.10 Self-test (replace the current one; keep the `-HeritageSelfTest` flag, console command and exit-code behaviour)

Drive logic directly through the flow subsystem, director and props. No input simulation. Suggested checks (number them; print `[PASS]`/`[FAIL]` per check and `HERITAGE SELFTEST: PASS (n/n)`):

1. Starts in Reality; both rooms and the director exist; rack empty.
2. Reality scan gate: Reel 1 canister Use is refused before scan, works after `ScanMemento`; rack has Reel 1.
3. `EnterFilm(Reel1)` → era Film, pawn inside the Film room bounds, Scene 1, Play Mode, all film props frozen.
4. Canonical state: `SetScene(2)` (third scene) → kettle has `Filled`; `SetScene(0)` → not `Filled`, fireplace `Lit`, key `Hot`.
5. Playback cannot pass the missing frame: stepping forward 20 times ends on the key scene index; `IsOnMissingFrame()` true; butler hidden.
6. Play Mode refuses verbs (`CanUseVerbs()` false); Pause allows them and unfreezes the kettle.
7. Facts are ignored in Play Mode; in Pause, dousing the fire with the filled kettle records `FireDoused`; grabbing the cooled key (call the grab hook) records `KeyTaken`; `bImaged` becomes true.
8. Pause layer: `ResumePlay(+1)` restores canonical state (fire `Lit` again at Scene 4); `Pause()` at the same scene restores the layer (fire out).
9. Rollback: clears the layer and the current session's facts; `bImaged` false; redo the two actions → imaged again.
10. Wrong composition (only `FireDoused`, or with an extra fact) → `Upload(key)` fails, nothing locked.
11. Correct composition → upload succeeds; `LastPlayableScene()` is the last scene; a second upload attempt is refused; Scene 5 canonical now shows the fire out.
12. Leaf: pause, break the vase (Impact 1000) → `VaseBroken` and `ReelFound` recorded; choose `ReelFound`, upload leaf → `bLeafRewardUnlocked`.
13. `LeaveFilm()` → era Reality, pawn in the Reality room; Reality fireplace still cold, Reality vase shards unchanged (the past did not change the present).
14. Reality portrait: Use → reveals the key; scan it; key opens the Reality door → `bSliceComplete`.
15. Reel 2 canister is visible and collectable; `EnterFilm(Reel2)` is refused.
16. Objective stage reached 8 or beyond; action log non-empty.

## 5. Files

New: `HeritageGameFlowSubsystem.h/.cpp`, `HeritageFilmDirector.h/.cpp`, `HeritageReelData.h/.cpp`, optionally `HeritageUIController.h/.cpp`.
Edited: `HeritageTags`, `HeritageTypes.h`, `HeritageInterfaces.h`, `HeritageObjectComponent`, `HeritageStatics`, `HeritageProp`, `HeritageProps`, `HeritageDemoRoom`, `HeritageGameMode`, `HeritageCharacter`, `HeritageInteractorComponent`, `HeritageHUD`, `HeritageSelfTest.cpp`.
No changes to `Blank.Build.cs`, `.ini`, or anything outside `Source/Blank/Heritage/` and `Docs/`.

Recent reviewer changes already in the code that must be kept: `DashPushSpeed` (velocity-change dash push), scan gate limited to mementos.

## 6. Order of work

1. Types, tags, flow subsystem skeleton, prop additions (`FilmId`, freeze, `RefreshFromState`, capture/restore), reaction refactor.
2. Two-era room builder, new props, game mode wiring, era teleport and post-process.
3. Film director: reel data, canonical scenes, Play/Pause, transport, pause layer, rollback, facts, imaging.
4. Input routing and gating in character/interactor.
5. HUD: film overlay, Map, Archive (Notebook + Workbench), upload flow, objectives.
6. Unlocks in Reality (portrait/key/door, Reel 2), slice-complete.
7. Self-test rewrite. Hand-off file `Docs/HERITAGE_GameLoop_Handoff.md`: what was built, deviations and why, every engine API that differed from assumptions, values that need tuning, and a plain statement of what was and was not compiled or run.

## 7. Manual acceptance (human, in the editor — not Codex)

1. Reality: cannot pick up the reel before scanning it; first scan opens the Archive; picking up the reel opens the Map.
2. Enter Reel 1: look changes (sepia, grain); frames step like a slideshow; E/Q/Space behave per §3; verbs do nothing in Play Mode.
3. Playback halts at "FRAME MISSING"; later cells are dark.
4. Pause at Scene 4, pour the kettle on the fire, take the key → "Imaging successful." Resume → the fire is lit again; pause again at the same scene → your version is back; Backspace rolls it back.
5. Archive: wrong fact set is refused; correct set uploads after the confirmation; frame turns green; reel plays to Scene 8 showing where the key was hidden, with the chosen light and audio caption on Scene 5.
6. Leave the reel: Reality is unchanged. Portrait now swings aside; scan and take the key; open the door → SLICE COMPLETE.
7. Optional: break the vase while paused, upload the leaf with "A film reel was hidden in the vase." → Reel 2 appears in the shards in Reality and can be collected but not read.
8. R resets everything.

## 8. Open design points (decisions taken here, for the team to confirm)

- `Space` pauses only in Play Mode; `E`/`Q` resume. The GDD gives Space to both jump and pause.
- The key frame's "correct answer" is a set of recorded facts chosen in the Workbench; Audio and Light are free expression and never block an upload.
- One pause layer per segment is kept (the most recent scene paused on), not one per scene.
- The player acts beside a frozen butler rather than visibly replacing him (GDD §3.3.2's Animus idea is only a toast line here).
- Knowledge unlocks are hard gates (the portrait does nothing until the frame is uploaded), not Outer-Wilds-style soft knowledge.
- Archive is on `N` and Map on `M`; the GDD does not name keys for them.
