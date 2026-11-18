# Heritage game-loop v2 handoff

## Reviewer verification (Claude, 2026-11-17) — supersedes "Nothing was compiled or run" below

- **Build:** failed once on `HeritageHUD.cpp` (a local `bEditable` shadowed `AActor::bEditable`, error C4458). Renamed to `bFrameEditable`; `BlankEditor Win64 Development` then succeeded with no warnings.
- **Self-test:** headless run printed `HERITAGE SELFTEST: PASS (33/33)`.
- **Read in review:** `HeritageHUD.cpp` in full, and the character and flow diffs (click routing, input-mode switching, objectives).
- **Not verified:** nothing has been seen on screen or played with a mouse. Cursor capture when the Archive opens and closes, button hit areas, the 1280x800 layout scaling and all timing values are unseen.

## Validation status

**Nothing was compiled or run.** No C++ compilation, UnrealHeaderTool, gameplay self-test, editor, PIE session, rendering, or real-input playtest was launched. No Build.bat, UnrealBuildTool, dotnet, UnrealEditor, or UnrealEditor-Cmd invocation was attempted. There are no build or gameplay results to report. The separate reviewer owns compilation and runtime verification.

Only source inspection and text-based consistency checks were performed: local engine-header/API inspection, include-path resolution, generated-header ordering, delimiter balance, declaration/definition correspondence, localization-key consistency, removed-symbol searches, and `git diff --check`. These are not compiler, UHT, lint, or gameplay results. No standalone lint configuration/command was found; the project self-test requires the forbidden Unreal runtime and was not executed.

Writes are limited to `Source/Blank/Heritage/` and this handoff. The existing, untracked v2 plan was read and left unchanged. No assets, maps, Blueprints, configuration, build/module/project files, dependencies, or Git metadata were changed. Git reads used a command-local `safe.directory` override because of the sandbox account's different ownership; no Git configuration was written.

## What changed

- Reel 1 has two ordinary missing frames: frame 3 requires exactly `KettleFilled`; frame 5 requires exactly `FireDoused` and `KeyTaken`. Each scene owns its answer and restored snapshots. The director stores one value-only edit per missing scene, indexed by zero-based scene number.
- Unrestored gaps use `GapSecondsPerAction * Answer.Num()` in reel time. Transport rate scales elapsed reel time. `GetGapProgress01()` is elapsed fraction; the HUD draws `1 - progress` as the draining bar. Large time advances re-evaluate duration after each cut, retaining the remainder. `AdvanceTime()` is shared by Tick and the authored self-test.
- Forward playback/stepping stops at an unrestored missing frame only if an earlier missing frame is unrestored. Rewind can escape the block. `SetScene()` can address all eight scenes; there is no locked-scene clamp.
- Frame 4 supplies its full kettle independently of frame 3. Frame 6 supplies fire out, empty kettle, and the carried key independently of frame 5. Frames 6-8 stay visible in the filmstrip. The final frame keeps the butler visible in front of a closed portrait. No authored Film frame opens the portrait or places the key behind it.
- Facts are stamped with the current/next unrestored missing frame, deduplicated per frame and pause session, and exposed as a per-frame checklist. Imaging is derived from that frame's evidence. An imaging transition posts `Frame N imaged.` and repeats it on the director's next Tick so the originating interaction's toast cannot immediately hide it.
- Exact-set uploads are ordered and immutable. Frame 5 can be imaged/composed before 3, but upload reports `Restore frame 3 first.` until 3 is restored. Audio and light remain independent expressive choices, applied when the restored frame plays.
- The single retained pause layer, snapshot merge/restore, side-prop restoration, transport suspension, and freeze/unfreeze are retained. Rollback removes the current layer's unuploaded evidence and all unuploaded evidence for its target, even if recorded from a different pause scene; this guarantees the target becomes unimaged. Other frames retain independent evidence, and uploaded evidence/edits remain locked.
- Scanning a canister grants ownership immediately. Canisters remain visible; Use on an owned canister attempts entry instead of collecting it. Reel 2 is now a scannable memento and remains unreadable. Breaking the Film vase while recording reveals it in Reality immediately through `UnlockLeafReward()`; both vase facts remain checklist decoys. The reward is knowledge and survives rollback.
- Enter toggles Reel 1 entry/exit when no screen is open. The Map enum, UI, bindings, flags and methods are removed, as are key/leaf frame edits, damaged-area flags, locked-scene data, and every `bKeyFrame` parameter.
- Reality portrait Use has no restoration prerequisite. The existing Reality memento scan gate still applies to the portrait, and the revealed key must still be scanned before grabbing. This follows the instruction to retain scan gating while making restoration optional.
- Objectives follow scan, reel ownership, watching, missing/imaged frames, final-frame knowledge, and the door. Seeing frame 8 or restoring both missing frames enables the leave/use-knowledge objective; opening the Reality portrait also directs the player to the door without requiring film completion.
- The Archive has REEL / NOTEBOOK / CLOSE and ENTER REEL / LEAVE REEL buttons, clickable status cells, per-frame checkboxes, audio/light controls, an upload button with availability reason, and a centered confirmation. Reel 2 has an unreadable entry button once owned. Notebook rows are clickable and retain captured notes/state.
- The HUD uses only rectangles and text, ASCII control symbols, large engine-font headings, backed world text, key-cap chips, a mode badge, gap/block feedback, and per-target recording progress. Readable/missing/imaged/restored cells are grey/red/amber/green; the current Film scene is outlined and the Archive selection is underlined.
- `DashPushSpeed`, the verb/stimulus/reaction split, mementos-only Reality gating, and both zero-motion-blur overrides are preserved.

## Input and screen behavior

LMB has one Enhanced Input `Started` handler in the character. If a screen is open it calls HUD hit testing and returns unconditionally, including when that click closes the screen or enters/leaves the reel. Otherwise it calls the existing grab/drop verb. Throw and other gameplay verbs retain their existing flow gates. Look and movement retain their screen gates.

Opening a screen shows the cursor and applies `FInputModeGameAndUI` with `DoNotLock` and `SetHideCursorDuringCapture(false)`. Closing hides the cursor and applies `FInputModeGameOnly` with capture-mousedown consumption. The installed engine implementation restores viewport focus and high-precision mouse movement for this mode, so there is no deliberate extra-click requirement. Actual mouse capture/release behavior still requires the reviewer's real-input check.

Hit targets are value-only `{X,Y,W,H,Id,bEnabled}` records rebuilt each HUD draw. Drawing and mouse coordinates share the same logical scale. Changes that invalidate controls clear the previous list immediately. The confirmation rebuilds the list with only UPLOAD and CANCEL, preventing clicks through the modal.

Keyboard arrows move focus between visible controls. Moving onto a frame cell selects that frame; moving onto a Notebook row selects that entry. Enter activates the focused control. Left/Right on audio/light controls cycles the corresponding choice directly; Enter on either arrow also applies its direction. Backspace cancels confirmation or closes the Archive; N closes it. M has no binding.

Opening the Archive in Film selects the director's target, falling back to the first unrestored missing frame if there is no forward target. With none remaining it selects the current frame. In Reality it selects the first unrestored missing frame and initially focuses ENTER REEL if owned. The first scan opens the Notebook for an ordinary memento, or the Reel page for a canister. Later manual Archive openings start on the Reel page.

## Implementation choices and deviations

1. `CanUpload(Frame, Why)` is an additional helper. It distinguishes upload ordering from editability so an already-imaged frame 5 can be composed while frame 3 is still missing. Exact-answer rejection stays in `Upload`, allowing the requested wrong/extra composition feedback.
2. Keyboard navigation uses directional button focus instead of the old nested tab/workbench row state. There is no intermediate editor screen to back out of, so Backspace closes directly outside confirmation. The existing audio/light Left/Right shortcut is retained.
3. No edit to `HeritageInteractorComponent.cpp` was needed: its grab/drop, throw, use and camera paths already gate on the flow. The character's exclusive LMB routing supplies the additional UI-click isolation without duplicating those gates.
4. The legacy name `UnlockLeafReward()` and its reward flag remain because the v2 plan explicitly calls that method. There is no leaf frame, leaf edit, damaged area, or leaf upload path remaining.
5. The self-test source contains 33 checks rather than retaining the previous count of 20. Additional cases cover rate-scaled gaps, cross-pause rollback, restored presentation for both frames, canister entry, independent evidence, and screen suspension. Its flag, console command, per-check logging, summary shape and exit-status behavior remain unchanged.
6. The UI skill's web packages/templates were not introduced: they conflict with this task's C++-only Canvas implementation and allowed-write scope. No dependencies were added.

The reviewer-only Workflow, Steps and Verification sections were not executed or rewritten.

## Engine APIs inspected and expectation differences

Context7's official Unreal documentation was consulted first for input modes. The installed UE 5.8 headers and relevant implementation were then read as ground truth. No incompatible signature or struct-member mismatch was found. Relevant details that needed explicit resolution are listed below; none represent compilation evidence.

| Installed engine file (relative to `C:/Program Files/Epic Games/UE_5.8/Engine/`) | Observation / use |
| --- | --- |
| `Source/Runtime/Engine/Classes/GameFramework/PlayerController.h` | `FInputModeGameAndUI::SetLockMouseToViewportBehavior(EMouseLockMode)` and `SetHideCursorDuringCapture(bool)` return `FInputModeGameAndUI&`. Uses `EMouseLockMode::DoNotLock`. `FInputModeGameOnly::SetConsumeCaptureMouseDown(bool)` returns its mode reference. `SetInputMode` accepts `const FInputModeDataBase&`. |
| Same header | `GetMousePosition` has both `(float&, float&) const` and `(double&, double&) const` overloads. HUD explicitly uses floats; it does not assume `FVector2D` component precision. `bShowMouseCursor` is a public reflected bitfield and remains directly assignable as requested. |
| `Source/Runtime/Engine/Private/PlayerController.cpp` | Game-and-UI releases capture, leaves input enabled, and uses `CaptureDuringMouseDown`. With no focus widget specified it preserves focus. Game-only calls `UseHighPrecisionMouseMovement`, `SetUserFocus` and `LockMouseToWidget`, and selects permanent capture. No Slate or UMG dependency was needed to restore viewport focus. |
| `Source/Runtime/Engine/Classes/GameFramework/HUD.h` | `DrawRect(FLinearColor,float,float,float,float)`, `DrawText(...,UFont*,float,bool)`, and `GetTextSize(...,float&,float&,UFont*,float) const` match the rectangle/text implementation. No AHUD hit-box APIs are used. |
| `Source/Runtime/Engine/Classes/Engine/Canvas.h` | `ClipX`/`ClipY` are float clipping dimensions. All UI geometry uses an explicit common scale derived from these values. |
| `Source/Runtime/Engine/Classes/Engine/Engine.h` | `GetMediumFont()` and `GetLargeFont()` return `UFont*`; the latter is used for headings. |
| `Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputComponent.h` | Enhanced Input headers are under the engine plugin, not `Engine/Source`. Typed `BindAction` supports the existing handler and payload pattern. The LMB action retains `ETriggerEvent::Started`; only its handler changes. Runtime input objects retain their existing `UPROPERTY()` ownership. |

Every explicit non-generated include in the Heritage directory was resolved against local project/engine/Enhanced Input headers. Reflected headers retain `.generated.h` as their last include. New persistent fields contain only values; persistent UObject references remain reflected `TObjectPtr` or weak pointers. Declaration/definition review included the native gameplay-tag declaration/definition macros, which are not ordinary C++ function declarations.

## Unexecuted self-test coverage

The source retains `-HeritageSelfTest` and `Heritage.SelfTest`. It still logs numbered `[PASS]` / `[FAIL]` lines and `HERITAGE SELFTEST: PASS|FAIL (n/n)` and requests status 0 only when all expected checks succeed, otherwise 1, for the command-line path. These are output contracts, not observed results.

The 33 authored checks cover:

- Fresh state; scan-to-own and first Archive opening; portrait opening with zero restored frames; revealed-key scan gating; owned-canister Use and both ToggleReel directions; transport into Film, frozen props and verb gates; zero motion blur; ignoring Play facts.
- Frame 3's one-action gap and independent frame 4 state; blocking at 5; accelerated rewind; targets on 2-3 versus 4-5; frame-specific imaging/deduplication; retained pause layer; rollback pruning and cross-pause un-imaging; immediate vase reward and both decoys.
- Imaged/exact frame 5 ordering refusal; frame 5 rollback preserving frame 3 evidence; empty/wrong/extra frame 3 refusals; exact immutable frame 3 upload and restored presentation; two-action frame 5 gap and independent frame 6 state; visible final butler/closed portrait; accelerated forward gap.
- Frame 5 imaging, incomplete refusal, exact immutable upload, restored state/light/audio; Pause physics/verb/fact suspension; Play timer suspension; reversible vase/letter state with persistent knowledge; Reality preservation on exit; scan-owned but unreadable Reel 2; key/door/slice completion.

The direct-logic checks do not exercise actual mouse input, hit-target rendering, cursor capture, font legibility, or keyboard focus movement. Those remain unobserved, along with compilation and all gameplay behavior.

## Values needing tuning

| Area | Current values |
| --- | --- |
| Playback | `FrameInterval = 1.5 s`, `GapSecondsPerAction = 2 s` (frame 3: 2 s; frame 5: 4 s), `FastRate = 4`. Pausing/resuming reapplies the current canonical frame and restarts its interval, as before. |
| Canvas layout | Logical minimum design surface 1280 x 800, uniformly fitted to actual Canvas dimensions. Margin 24; line height 28; film cells 58 high; ordinary buttons 40-42 high; upload 52 high; modal 640 x 210. Checklist and presentation controls occupy 60% / 35% of content width. Notebook displays eight rows per page. Small-window readability and ultrawide layout need visual review. |
| Keyboard focus | Directional candidate score is forward distance plus four times perpendicular distance. Review travel between filmstrip, checklist, presentation, and upload controls. |
| Comfort/readability | Cut flash 0.09 s at 0.22 opacity; four 1-unit scratches at 0.08 opacity; backed text opacity 0.88; screen dim opacity 0.97; three action-log lines. Existing toast fade 0.5 s. |
| Era grading | Reality saturation 0.55; Film saturation 0.25, gain `(1.08,1.0,0.8,1.0)`, grain 0.6, vignette 0.8. Motion blur override stays 0 in both eras. |
| Lighting | Main light 5000; cold-dark multiplier 0.35 and blue `(0.35,0.5,1)`; daylight multiplier 1.2. |
| Content transforms | Authored frame poses are in `HeritageReelData.cpp`; frame 8 butler is room-local `(-300,150,75)`, carried key `(-335,150,100)`, portrait remains at its closed position. Human review is needed for pose clarity and prop spacing. |
| Retained interaction | Portrait slide 60 cm / 0.6 s; reach 220 cm; max reach height 190 cm; scan dwell 1 s; throw 1100 cm/s; dash 1600 cm/s for 0.25 s; cooldown 0.8 s; `DashPushSpeed = 500 cm/s`; vase threshold 650. |

## Files

Created:

- `Docs/HERITAGE_GameLoop_v2_Handoff.md`

Edited (all under `Source/Blank/Heritage/`):

- `HeritageTypes.h`
- `HeritageReelData.cpp`
- `HeritageFilmDirector.h`, `HeritageFilmDirector.cpp`
- `HeritageGameFlowSubsystem.h`, `HeritageGameFlowSubsystem.cpp`
- `HeritageArchiveSubsystem.cpp`
- `HeritageProps.cpp`
- `HeritageDemoRoom.cpp`
- `HeritageCharacter.h`, `HeritageCharacter.cpp`
- `HeritageHUD.h`, `HeritageHUD.cpp`
- `HeritageSelfTest.cpp`

## Locked frames change

Reel 1 now stops on frame 4 until frame 3 is uploaded, then on frame 6 until frame 5 is uploaded. Both uploads open the way to frame 8 and the butler at the closed portrait. Playback, fast-forward, stepping, and direct scene selection share this limit. Rewind still works at either stop, and each reachable gap keeps its authored duration.

Both filmstrips mark locked cells with a dark color and `LOCKED`. At a stop, a backed HUD plate names the frame to restore. The objective directs the player to rewind to that gap, or to upload it if already imaged. Pausing on frame 4 still targets frame 5, so its Archive checklist stays available and its upload button explains why frame 3 must be restored first. The leave objective still requires seeing the true last frame or restoring both gaps. The Reality portrait still opens with zero uploads.

The self-test now contains 37 checks, including both stops, seek and transport limits, targeting and upload order, and playback through frame 8 after both uploads. No deviations from the brief. Nothing was compiled or run for this change.

### Reviewer verification of the locked-frames change (Claude, 2026-11-18)

- Build: `BlankEditor Win64 Development` succeeded on Codex's change as written, no warnings.
- Self-test: headless run printed `HERITAGE SELFTEST: PASS (37/37)`.
- Not verified: the LOCKED cells and the "Restore frame N to see more." plate have not been seen on screen.

## Scan and upload flow change

Scanning either reel grants ownership, hides its canister, and disables collision. The room keeps the actor, and the canister still counts toward the Notebook total. The vase reveals Reel 2 once. Scans leave the Archive closed. Collecting a reel shows the [N] hint, as does scanning an ordinary memento first. In Reality, the Reel 1 entry objective offers both [N] for the Archive and [Enter] to enter directly.

Successful uploads close the Archive and show a restoration toast. In Film, the director discards the retained pause layer and drops held props before showing the uploaded frame in forward Play at 1×. Other frames keep their evidence and draft edits. Uploading in Reality keeps the player there. A refused upload leaves the Archive open with its reason and preserves the current mode.

The self-test now has 46 checks, including collection, upload refusal, restored playback through frame 6, and preservation of work on frame 5. No deviations from the brief. Nothing was compiled or run for this change.


### Reviewer verification of the scan and upload flow change (Claude, 2026-11-18)

- Build: `BlankEditor Win64 Development` succeeded on Codex's change as written, no warnings.
- Self-test: headless run printed `HERITAGE SELFTEST: PASS (46/46)`.
- Not verified: nothing has been seen on screen or played with real input.

