# Heritage game-loop handoff

## Reviewer verification (Claude, 2026-11-16) — supersedes "Nothing was compiled or run" below

- **Build:** `BlankEditor Win64 Development` succeeded on Codex's code as written, no errors or warnings.
- **Self-test:** headless run printed `HERITAGE SELFTEST: PASS (20/20)`.
- **Not verified:** nothing has been played with real input. The HUD, Map and Archive screens, keyboard navigation, film look and all timing/tuning values have never been seen on screen; the self-test drives logic directly and runs without rendering.
- **Also changed by the reviewer:** `r.DefaultFeature.MotionBlur=False` in `Config/DefaultEngine.ini`.

## Validation status

**Nothing was compiled or run.** No UnrealHeaderTool, C++ build, Unreal runtime, self-test, PIE session, or real-input playtest was launched. No `Build.bat`, UnrealBuildTool, `dotnet`, `UnrealEditor`, or `UnrealEditor-Cmd` invocation was attempted. There is no build output or self-test output to report. Compilation and runtime acceptance remain with the separate reviewer.

Only source/header inspection and text-based consistency checks were performed. Include paths were resolved against the installed engine and Enhanced Input headers; generated-header ordering, delimiter balance, declaration/definition correspondence, persistent UObject references, and localization-key consistency were inspected. These checks are not C++ compilation, UHT validation, lint execution, or gameplay tests. No standalone project lint command/configuration was found.

All writes were confined to `Source/Blank/Heritage/` and this document. No assets, maps, Blueprints, `.ini`, module build files, project files, dependencies, or Git metadata were created or changed.

## Implemented in the requested sequence

| Step | Source implementation |
| --- | --- |
| 1 | Era/mode/screen/scene types, snapshot/take/edit records, fact tags, `State.Dropped`, `Verb.Film`, flow skeleton, prop freeze/capture/restore, and state-driven prop refresh. Reactions emit facts; the grab interface has a default-empty `OnGrabbed` hook. |
| 2 | Reality at `(0,0,50)` and Film at `(0,4000,50)`, shared room shell, era-specific contents, main light access, static canisters/shards, room-local teleport, era post-process. **Motion blur amount is explicitly overridden to zero in both eras.** |
| 3 | Code-authored eight-scene Reel 1; carried-forward canonical state, restored key frame, locked later scenes, timed hard cuts, fast forward/rewind, Play/Pause, one retained pause layer, rollback, fact deduplication, imaging, immutable uploads, caption-only audio and light choices. |
| 4 | Runtime Enhanced Input additions; centralized flow gates for movement, look, camera, verbs, and Reality-only memento scanning. Space pauses playback but jumps while paused. E/Q start/resume transport and handle completion/cancellation. Held objects are released before canonical restoration or travel. `DashPushSpeed` remains a velocity-change push. |
| 5 | Canvas filmstrip/status colours/leaf notch, mode and caption, missing-frame plate, damaged-area pedestal plate, scratches/cut flash, Map schematic/rack, Notebook, Workbench fact/audio/light rows, upload confirmation, one-line objectives, scan feedback and action log. Navigation state is entirely in `AHeritageHUD`, the explicitly permitted alternative to a separate UI controller. |
| 6 | Upload knowledge enables the Reality portrait's 60 cm / 0.6 s slide. Its key becomes visible and grabbable after scanning; the key opens the door and sets `SLICE COMPLETE`. A `ReelFound` leaf upload reveals Reel 2, which is collectable but cannot be entered. |
| 7 | Replaced the old interaction self-test with 20 numbered direct-logic checks and retained its command-line flag, console command, summary format, and exit-code behaviour. Wrote this handoff. **The checks have not been executed.** |

## Behaviour and navigation

- Reality's five mementos are Reel 1, rusted kettle, portrait, vase shards, and hidden key. Reel 2 is a knowledge reward, not a sixth memento. Film copies, books, and debris are not mementos. `heritage.RequireScanToInteract` defaults to `1` and applies only in Reality; the gate remains limited to mementos for both grab and use.
- First successful scan opens the Archive once. First reel collection opens the Map once. Both close with M or N. Space/E/Q, verbs, movement, look and reset are ignored while a screen is open. Screen transitions clear held transport keys; paused Film physics is frozen while the screen is open.
- Map: Up/Down selects, Enter confirms. In Film the first row leaves the reel; selecting owned Reel 1 closes the Map and continues the current visit.
- Archive opens on the Notebook tab bar. Left/Right changes tabs there; Down or Enter enters the rows. Up returns to the tab bar. Notebook selection shows the saved note and state at capture.
- Workbench: select Frame 5 or Frame 3 and press Enter. When editing is unavailable, the reason is displayed. Up/Down moves between recorded facts, Audio, Light, and Upload. Enter toggles facts; Left/Right cycles Audio/Light. Upload requires a second Enter after the immutable-upload warning. Backspace cancels that confirmation, backs out of an editor, returns to the tab bar, then closes.
- Key upload requires imaging and exactly `FireDoused` plus `KeyTaken`. Audio and Light do not affect correctness. Leaf upload accepts exactly one recorded fact; only `ReelFound` gives the reward. Both uploaded edits are immutable.
- The single pause layer survives leaving/re-entering the reel. Re-entering starts playback at Scene 1. Returning to the layer's scene and pausing restores it. Rollback removes the current layer's evidence, prunes unavailable facts from unuploaded compositions, and recalculates imaging. Previously uploaded frames remain locked.

## Deliberate differences and reasons

1. **Build/run steps are deferred entirely**, as explicitly required by the current task; they supersede the older plan's compile-at-each-milestone instructions.
2. **When a saved pause layer is restored, its fact records move to the new pause-session ID.** The plan increments that ID on each Pause but also expects resume → pause → rollback to undo that layer's imaging. Keeping the old fact-session IDs would leave imaging behind after the layer was rolled back. Evidence from other independent sessions is preserved.
3. **All movable Film books and vase fragments are registered with the director**, in addition to the nine named story actors. The vase still emits physical fragments, but they are preallocated, hidden until breakage, and retained rather than destroyed after a lifespan. This makes playback, screen suspension, and rollback cover the complete mutable Film room instead of leaving stray debris or moved books behind.
4. **Door use hides/freezes its key instead of destroying it.** This preserves the actor for snapshot restoration. The interactor already releases hidden held actors. Door visuals now snap from `Open`, as specified for snapshot-driven state; the obsolete door-animation duration and old landing-poll self-test settings were removed.
5. **Room construction occurs just after `Super::StartPlay()`**, still within game-mode `StartPlay`, with deferred room spawning to set the era before `Build()`. Spawned props therefore finish `BeginPlay()` before director initialization freezes them, so their original physics capability is remembered correctly.
6. **The portrait centre moves 12 cm into the room** (local X `-375`, previously `-387`), with the key at local X `-385`, rotated 90 degrees and supported on a small shelf. This gives the key physical space behind the panel and keeps it reachable when revealed. Reel data uses the same positions.
7. **Opening a screen releases carried objects and ends a dash/camera mode**, then freezes paused Film props. This prevents a live physics handle or pending interaction from changing the working layer behind the screen. Closing restores movement in falling mode so the movement component reacquires its floor.
8. **Heat refresh uses a synchronous geometric hearth query** rather than relying on cached overlap events after a teleport. It reconciles previously tracked heat with the current hearth contents, including snapshot rewinds. There is an additional refresh after all actors are restored so cross-actor visuals, attachments, and heat see the final scene state.
9. The self-test has **20 checks rather than the suggested 16**, adding explicit motion-blur, transport/presentation, screen suspension, and side-prop rollback coverage. This is source coverage only; there are no results yet.

No extra playable reel, save/load, real sound assets, AI, mouse UI, or asset pipeline was added. ASCII film-mode symbols are used as allowed by the plan.

## Engine API inspection

Context7's official Unreal documentation was consulted, followed by the installed UE 5.8 headers as ground truth. No signature mismatch with the plan was identified in the APIs inspected. That is not a compiler result.

| Header under `C:/Program Files/Epic Games/UE_5.8/Engine/` | Exact API used / observation |
| --- | --- |
| `Source/Runtime/Engine/Classes/Engine/Scene.h` | `FPostProcessSettings::ColorSaturation` and `ColorGain` are `FVector4`; `FilmGrainIntensity`, `VignetteIntensity`, and `MotionBlurAmount` are floats. Corresponding flags are `bOverride_ColorSaturation`, `bOverride_ColorGain`, `bOverride_FilmGrainIntensity`, `bOverride_VignetteIntensity`, `bOverride_MotionBlurAmount`. All are assigned explicitly when applying an era. |
| `Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputComponent.h` | `BindAction` accepts typed handler payload arguments; direction and navigation integers use these overloads. Enhanced Input headers live under the engine plugin, rather than `Engine/Source`. Runtime actions/context remain `UPROPERTY()` references. |
| `Source/Runtime/Engine/Classes/GameFramework/HUD.h` | Existing `DrawText`, `DrawRect`, `DrawLine`, plus `GetTextSize` for fitting/wrapping. Rendering stays on `AHUD`/Canvas. |
| `Source/Runtime/Engine/Classes/GameFramework/PlayerController.h` | `ProjectWorldLocationToScreen(FVector, FVector2D&, bool)` positions the damaged-area plate. |
| `Source/Runtime/Engine/Classes/Engine/World.h` and `GameFramework/Actor.h` | Typed `SpawnActor`, `SpawnActorDeferred`, then `FinishSpawning(const FTransform&, ...)` for the era-set room builder. |
| `Source/Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h` | `DisableMovement()` and `SetMovementMode(EMovementMode, uint8 = 0)` suspend and restore player movement around screens. |
| `Source/Runtime/Engine/Classes/Components/BoxComponent.h` | `GetUnscaledBoxExtent` for synchronous hearth checks after snapshot transforms. |
| `Source/Runtime/GameplayTags/Classes/GameplayTagContainer.h` | `HasAllExact`, `IsEmpty`, and `ToStringSimple` support exact fact-set validation and Notebook state text. |
| `Source/Runtime/Core/Public/GenericPlatform/GenericPlatformMisc.h` | Existing `RequestExitWithStatus(bool, uint8, const TCHAR* = nullptr)` exit behaviour retained. |

All new snapshot/scene/fact/edit records contain values only, not persistent UObject pointers. UObject references retained by actors/subsystems/input are `UPROPERTY()` `TObjectPtr` fields/containers or existing weak pointers. Every reflected header places its `.generated.h` after its other includes.

## Reviewer validation still owed

The reviewer must compile/UHT-process the project and run the new self-test. On a fresh level with default scan gating, the entry points remain `-HeritageSelfTest` and the PIE console command `Heritage.SelfTest`. The command-line path requests exit status 0 only if all 20 checks succeed, otherwise 1. The console path does not request exit. The self-test intentionally consumes slice state; reset before rerunning or playing manually.

The authored checks cover initialization, scan/collection tutorial screens, era transport/frozen props, canonical rewind, missing-frame clamp, verb gating, facts/imaging/deduplication, saved layer, rollback, incorrect/correct/immutable upload, rewarded leaf, Reality preservation, portrait/key/door completion, Reel 2 refusal, objectives/logs, both motion-blur overrides, transport/audio/light, UI suspension, and letter/vase rollback.

After that, the full manual checklist in `HERITAGE_GameLoop_Plan.md` section 7 remains owed. In particular, verify actual key reach/visibility, held-object release around menu transitions, UI upload cancellation, regular-speed and held E/Q playback, pause jumping, wrong leaf upload without a reward on a separate reset, and the complete optional-reel path. No visual or real-input assessment has been made.

## Values needing human tuning

| Area | Current values / source |
| --- | --- |
| Playback | `FrameInterval = 1.5 s`, `FastRate = 4` on the director. |
| Era look | Reality saturation `0.55`; Film saturation `0.25`, gain `(1.08,1.0,0.8,1.0)`, grain `0.6`, vignette `0.8`; camera blend weight `1`; motion blur override `0` in both. Flow properties expose the look values. |
| Comfort effects | HUD cut flash `0.09 s`, opacity `0.22`; four scratches at opacity `0.08`. Both effects remain unassessed for comfort; their strength/count can be reduced to zero. |
| Lighting | Room main light `5000`, cold-dark multiplier `0.35` and blue `(0.35,0.5,1)`, daylight multiplier `1.2`. Firelight uses the room's warm colour. |
| Portrait/key | Slide `60 cm / 0.6 s`; portrait local `(-375,150,150)`, shelf/key placement as described above. |
| Interaction retained | Reach `220 cm`, max height `190 cm`, ground requirement, scan dwell `1 s`, throw `1100 cm/s`, dash `1600 cm/s` for `0.25 s`, cooldown `0.8 s`, `DashPushSpeed = 500 cm/s`, impact magnitude `800`, vase break threshold `650`. |
| HUD | Margin `24`, row height `24`, film cell height `32`, screen footer reserve `210`, damage plate `100 x 100`. Inspect small viewports and long fact/Notebook text; the layout has not been rendered. |
| Authored content | Scene transforms live in `HeritageReelData.cpp`. If changing the corresponding room layout/prop dimensions, update the authored keys too; these are code-authored content, not inferred from moved actors at runtime. |

## File manifest

Created:

- `Source/Blank/Heritage/HeritageGameFlowSubsystem.h`
- `Source/Blank/Heritage/HeritageGameFlowSubsystem.cpp`
- `Source/Blank/Heritage/HeritageFilmDirector.h`
- `Source/Blank/Heritage/HeritageFilmDirector.cpp`
- `Source/Blank/Heritage/HeritageReelData.h`
- `Source/Blank/Heritage/HeritageReelData.cpp`
- `Docs/HERITAGE_GameLoop_Handoff.md`

Edited (all source paths are under `Source/Blank/Heritage/`):

- `HeritageTags.h`, `HeritageTags.cpp`, `HeritageTypes.h`, `HeritageInterfaces.h`
- `HeritageObjectComponent.cpp`, `HeritageArchiveSubsystem.cpp`
- `HeritageStatics.h`, `HeritageStatics.cpp`
- `HeritageProp.h`, `HeritageProp.cpp`, `HeritageProps.h`, `HeritageProps.cpp`
- `HeritageDemoRoom.h`, `HeritageDemoRoom.cpp`, `HeritageGameMode.h`, `HeritageGameMode.cpp`
- `HeritageCharacter.h`, `HeritageCharacter.cpp`, `HeritageInteractorComponent.h`, `HeritageInteractorComponent.cpp`
- `HeritageHUD.h`, `HeritageHUD.cpp`, `HeritageSelfTest.cpp`
