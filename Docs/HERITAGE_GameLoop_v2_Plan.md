# Heritage loop v2 — no map, two missing frames, per-frame submit, mouse UI

## Context

Playtest feedback on the full-loop slice (`Source/Blank/Heritage/`, UE 5.8, C++ only):

- The Map adds a step without adding anything. Scanning a reel should give it to you, and one button should take you into it.
- "Damaged area" vs "missing frame" is a distinction without a difference: both need the player to act something out. Make both plain missing frames.
- Frame 3 (fill the kettle) should be submittable on its own; the Archive should default to the frame you are working on.
- The UI is hard to read and operate. Make it obvious, using only rectangles and text (no sprites).

Decisions from the user:

- **Playback over missing frames:** a missing frame does not hard-stop the reel. It plays as a gap lasting a few seconds, proportional to the number of actions that frame needs, then continues. The reel **blocks** only when it reaches a missing frame while an *earlier* missing frame is still unrestored.
- **Frames 6–8 are always visible.** The reel never shows the key going behind the portrait; the last frame is the butler standing in front of the portrait.
- **Portrait in Reality always works** (pure knowledge gate). Restoring frames is not required to finish.
- **Checklist stays** for composing a frame. **Audio/Light, vase + Reel 2, Notebook** all stay.
- **Mouse-clickable buttons** in the Archive; keys remain as shortcuts.

Workflow: I write the Codex-facing plan to `Docs/HERITAGE_GameLoop_v2_Plan.md`, dispatch Codex (CLI, workspace-write, builds forbidden to avoid the dotnet pop-ups), then act as reviewer: build, run the headless self-test, read the diff (`git diff` now works), fix or send back.

## Design

### Reel 1 (8 frames, authored in `HeritageReelData.cpp`)

| # | Kind | Content |
|---|---|---|
| 1 | Readable | Fire lit, key in hearth, butler at door, kettle on table |
| 2 | Readable | Butler at table with kettle |
| 3 | **Missing** — answer `{KettleFilled}` (1 action) | Restored: butler at basin, kettle `Filled` |
| 4 | Readable | Butler at fireplace with full kettle |
| 5 | **Missing** — answer `{FireDoused, KeyTaken}` (2 actions) | Restored: fire out, empty kettle on hearth, butler holds key |
| 6 | Readable | Butler leaves the fireplace carrying the key (fire out) |
| 7 | Readable | Butler crossing the room |
| 8 | Readable, final | Butler stands in front of the closed portrait |

Frames after a gap must carry complete state themselves (frame 4 sets the full kettle; frame 6 sets fire out, kettle, key), since an unrestored frame contributes nothing.

Data model change (`HeritageTypes.h`): drop `bHasDamagedArea`, `bLockedUntilKeyUploaded`, `KeySceneIndex`, `LeafSceneIndex`, `KeyAnswer`, `RestoredKeyScene` from the segment. Each `FHeritageSceneDef` gets `FGameplayTagContainer Answer` and `TArray<FHeritageActorSnapshot> Restored`; `Kind` is `Readable` or `Missing`. Director keeps one `FHeritageFrameEdit` per missing frame (indexed by scene), replacing `KeyEdit`/`LeafEdit` and every `bool bKeyFrame` parameter with `int32 Frame`.

### Film director (`HeritageFilmDirector.h/.cpp`)

- **Gap playback:** on an unrestored missing frame, hold for `GapSecondsPerAction (2.0) × Answer.Num()` (divided by the transport rate), then step on. World during the gap = previous frame's state with the butler hidden. Expose `GetGapProgress01()`.
- **Blocking:** `IsBlocked()` — current frame is missing and unrestored *and* an earlier missing frame is unrestored. Playback stops there until the earlier one is uploaded. Replaces `LastPlayableScene()` clamping; all frames are reachable otherwise.
- **Target frame:** `GetTargetFrame()` = current frame if it is missing and unrestored, else the next unrestored missing frame after it, else none. Facts recorded while paused are stamped with the target frame. Pausing on frame 2 or 3 targets 3; on 4 or 5 targets 5.
- **Imaging per frame:** `IsImaged(Frame)` = facts stamped for that frame contain its `Answer`. Toast "Frame N imaged." on transition.
- **Checklist per frame:** `GetFactsFor(Frame)` (distinct facts stamped for it). `ToggleFact/CycleAudio/CycleLight/CanEdit/Upload` take `Frame`.
- **Upload(Frame):** refused if an earlier missing frame is unrestored ("Restore frame 3 first."); otherwise chosen set must equal `Answer` exactly ("The frames do not fit together."). Success locks the edit, the frame plays from `Restored` with the chosen light and audio caption.
- Pause layer, rollback (now also un-images the target frame), freeze/restore, screen suspension: unchanged.
- Vase: breaking it while paused reveals the reel and **immediately** calls `Flow->UnlockLeafReward()` ("You know where to look now."). `ReelFound`/`VaseBroken` are still recorded and show up as decoys in the checklist.

### Flow (`HeritageGameFlowSubsystem.h/.cpp`)

- Remove `EHeritageScreen::Map`, `bOpenedMap`, the `M` action, `bKeySceneUploaded`, `IsKeySceneUploaded()`.
- **Scan grants the reel:** `ScanMemento` on an `AHeritageReelCanister` calls `CollectReel`. Canister `Use` no longer collects; on an owned reel it enters the reel (same as the button). Reel 2 likewise: scan to own, entering is refused as unreadable.
- **Enter / leave:** `ToggleReel()` — Reality with Reel 1 owned → `EnterFilm`; Film → `LeaveFilm`. Bound to **Enter** when no screen is open, and to a button in the Archive.
- First scan still auto-opens the Archive (now landing on the Reel page with the ENTER REEL button when a reel was just scanned).
- Objectives rewritten: scan something → scan the reel → "Enter the reel [Enter]" → watch → "A frame is missing. Pause [Space] and act out what happened." → "Frame N imaged. Open the Archive [N] and upload." → after frame 8 has been shown or all frames restored: "Leave the reel [Enter]. Use what you saw." → "Open the door."

### Props (`HeritageProps.cpp`)

- Portrait in Reality: `Use` always slides it aside and reveals the key (remove the upload gate). Key still needs scanning before grabbing.
- Film portrait is never opened by reel data.

### UI (`HeritageHUD.h/.cpp`, `HeritageCharacter.cpp`) — rectangles and text only

Mouse support, implemented by hand rather than `AHUD` hit boxes: while a screen is open, show the cursor (`bShowMouseCursor`, `FInputModeGameAndUI` without mouse lock; restore `FInputModeGameOnly` on close); each drawn button registers `{Rect, Id, bEnabled}` in a per-frame array; LMB `Started` while a screen is open hit-tests it; hover highlights. Codex must verify the input-mode and mouse-position APIs in the 5.8 headers.

Archive screen (N), one obvious layout:

- Top bar buttons: `REEL` | `NOTEBOOK` | `CLOSE`, plus `ENTER REEL` / `LEAVE REEL` (hidden when no reel is owned).
- **Reel page:** large clickable filmstrip, each cell showing its number and a status word (`MISSING` red, `IMAGED` amber, `RESTORED` green, readable grey), current frame outlined. Below it, the selected frame's panel: title ("Frame 5 — MISSING"), a one-line status/instruction (the `CanEdit` reason, or "Tick the actions that belong in this frame."), the checklist as clickable checkbox rows, `Audio ◂ ▸` and `Light ◂ ▸` arrow buttons, and a big `UPLOAD FRAME 5` button (greyed with the reason when not allowed). Upload opens a centred confirm box with `UPLOAD` / `CANCEL` buttons.
- **Default selection:** opening the Archive in Film selects the director's target frame; otherwise the first unrestored missing frame.
- **Notebook page:** as now, with clickable rows.
- Keyboard still works: arrows, Enter, Backspace, N.

In-world HUD:

- Objective in a backed banner at top centre; era badge top-left.
- Replace the one-line control list with a short row of key-cap chips (bordered box with the key, then the action), only those valid right now: e.g. Reality `[Tab] Camera  [N] Archive  [Enter] Enter reel`; Film Play `[Space] Pause  [E] >>  [Q] <<  [Enter] Leave`; Film Pause `[E] Resume  [Backspace] Roll back  [N] Upload frame 5`.
- Film: mode badge; filmstrip with the same status colours; on a gap, a "FRAME MISSING" plate with a draining progress bar; when blocked, "Restore frame 3 first"; while paused, "REC > FRAME 5  1/2 actions" turning to "FRAME 5 IMAGED — [N] to upload".
- Text over the world always sits on a translucent backing rect; headings use the large engine font.

### Controls after the change

`M` unbound. `Enter` = enter/leave reel (no screen) or confirm (screen). LMB = click when a screen is open. Everything else as before.

### Self-test (`HeritageSelfTest.cpp`) — rewrite to match

Keep the flag, command, output format and exit code. Checks to cover: scan grants Reel 1 with no pickup and no Map; `ToggleReel` enters/leaves; frame 3 gap lasts `1 × GapSecondsPerAction` and frame 5 `2 ×` (drive via an exposed step/advance-time function); playback passes frame 3 unrestored, then **blocks** at frame 5; target frame is 3 when paused on 2–3 and 5 on 4–5; filling the kettle images frame 3 only; frame 5 upload refused while 3 is unrestored; frame 3 wrong/extra checklist refused, exact accepted and locked; after that playback passes the frame 5 gap and reaches frame 8 with the butler at the portrait and the film portrait closed; frame 5 imaging, upload, restored state, light/audio; rollback un-images the target frame; vase break unlocks Reel 2 at once; Reality portrait opens with **no** frames restored (fresh-state check done before any upload); key → door → slice complete; motion blur still zero; screen open freezes physics.

## Files

Edited: `HeritageTypes.h`, `HeritageTags.*` (if tags change), `HeritageReelData.*`, `HeritageFilmDirector.*`, `HeritageGameFlowSubsystem.*`, `HeritageProps.*`, `HeritageArchiveSubsystem.*` (scan → reel hook), `HeritageCharacter.*`, `HeritageInteractorComponent.cpp` (verb gating while clicking), `HeritageHUD.*`, `HeritageSelfTest.cpp`, `HeritageDemoRoom.cpp` (if canister setup changes).
New docs: `Docs/HERITAGE_GameLoop_v2_Plan.md` (Codex brief, expanded from this), `Docs/HERITAGE_GameLoop_v2_Handoff.md` (Codex).
No `.ini`, `Build.cs`, `.uproject` or asset changes.

## Steps

1. Write `Docs/HERITAGE_GameLoop_v2_Plan.md` from this design.
2. Dispatch Codex via `codex exec -s workspace-write` with the same guardrails as last time: C++ only, stay in `Source/Blank/Heritage/` and `Docs/`, check 5.8 headers, **never run Build.bat / dotnet / UnrealEditor**, no git commits.
3. Review: `git diff` against the brief; build `BlankEditor Win64 Development`; run the headless self-test; fix small issues myself, send larger ones back to Codex with `codex exec resume`.
4. Report what is verified vs unseen. Commit/push only if asked.

## Verification

- Build: `Build.bat BlankEditor Win64 Development -Project=...\Blank.uproject -WaitMutex` → `Result: Succeeded`, no warnings.
- Headless: `UnrealEditor-Cmd.exe Blank.uproject -game -nullrhi -nosound -unattended -nosplash -HeritageSelfTest -log -stdout` → `HERITAGE SELFTEST: PASS (n/n)`.
- Not verifiable by me: how the mouse UI, chips and gap bar look and feel. Manual pass by the user in PIE: scan reel → Archive opens with ENTER REEL → click it → watch the frame 3 gap pass and the reel block at frame 5 → rewind, pause at 3, fill kettle, N, tick, upload → frame 5 the same way → watch to frame 8 → leave → portrait → key → door.
