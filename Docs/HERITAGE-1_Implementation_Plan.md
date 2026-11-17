# HERITAGE-1 — Level Interaction Prototype (关卡交互原型)

Implementation plan for Codex. Target: Unreal Engine **5.8**, **C++ only**, project `Blank.uproject` (module `Blank`, API macro `BLANK_API`).

Revision: v2 (self-reviewed, see §11 for what changed from the first draft).

## 1. Sources and what they ask for

| Source | What it says | How this demo answers |
|---|---|---|
| Jira HERITAGE-1 | Explore how the player interacts with objects in a level. One level, one idea, no full flow. Study *player abilities* and *level elements*. Ideas: photo, grab/drop/throw, dash (bump into someone, knock things over), jump (onto a table). | One greybox room; all four abilities implemented as generic verbs. |
| GDD §3.3.4 (meeting 2026-11-09) | Drop fixed menu options. Player has general verbs (take/put down, use/throw/move); the *scene* decides the response. Same verb gives different results by object and state — full kettle near fireplace can douse it, empty kettle cannot. Implement player verbs and object responses **separately** for reuse. | The core architecture (§4): verbs emit *stimuli*, objects own their *reactions*. The kettle/fireplace example is the main puzzle. |
| GDD §1.1 / §1.3 | First-person investigation puzzle adventure. Observe → infer → act + verify → understand. Design for habitual thinking (key implies locked door). Anticipate player actions and give feedback (a vase can be smashed). | First-person character; key/door chain; breakable vase; every action produces visible feedback. |
| GDD §3.1.3 / Words | `Tab` takes out the camera; dwell ~1 s on an object captures a **Memento** into the Archive. Movement is WASD / Space / Ctrl. | Camera Mode exactly as specified. |
| GDD §3.1.1 | Scanning turns an object from non-interactable to interactable (a mechanic marked "needs prototype testing"). | Behind a console variable, default off, so the team can A/B it. |
| GDD §1.4.1 | Game code detects player behaviour and records it against the Film Segment. | An action log records every verb/result; shown on HUD. This is the hook the later film-editing work will consume. |
| GDD §3.1.1 | In film, `E` = fast-forward, `Q` = rewind, `Space` = pause. | Not in scope, but `E`/`Q` are left **unbound** so they stay free. |

**Out of scope** (do not build): Play/Pause film mode, film reels/segments/frames, Archive editing UI (scene/audio/light), Map, upload, reality-vs-film level switching, save/load, real art, audio, NPC AI, animation.

## 2. The demo: "The Study"

One room, one idea: **a small set of general verbs × objects that react by their own rules, with more than one way to solve each step.**

Main chain (habitual-thinking puzzle):

1. The exit **door** is locked. A **key** is visible inside the lit **fireplace**. Grabbing it fails: "Too hot to touch."
2. A **kettle** sits on top of a tall **cabinet**, out of reach from the floor. Three valid ways to get it:
   - **Jump** onto the **table** next to the cabinet and grab it.
   - **Throw** a **book** at it to knock it down.
   - **Dash** into the cabinet; the impact shakes loose whatever is on top.
3. Use the kettle on the fire while empty: "The kettle is empty." Use it on the **basin** → it fills.
4. Use the full kettle on the fireplace → fire goes out, key cools.
5. Grab the key, use it on the door → door opens → "DEMO COMPLETE".

Side reactions (leaf content, same verbs, different responses):

- **Vase** on a pedestal: breaks on a hard enough impact (thrown, hit by a thrown object, or dashed into). Breaking reveals a **film reel**.
- **Butler** (static NPC stand-in) holds a **letter**. Dashing into the butler or hitting it with a thrown object makes it stagger and drop the letter. Pouring water on it gives a text reaction only.
- **Portrait** on the wall: cannot be grabbed, only photographed.

Mementos to photograph (5): key, film reel, letter, kettle, portrait. HUD shows `Mementos n/5`.

## 3. Hard constraints for Codex

1. **No binary assets.** Do not create or edit `.uasset` / `.umap`. No Blueprints, no Input Action assets, no UMG widget assets. Everything is C++ plus `.ini`.
2. Geometry uses engine meshes only: `/Engine/BasicShapes/Cube`, `Sphere`, `Cylinder`, `Cone`, `Plane`, coloured through a dynamic instance of `/Engine/BasicShapes/BasicShapeMaterial` (vector parameter `Color`).
3. Input Actions and the Mapping Context are created at runtime with `NewObject` and held in `UPROPERTY()` members (GC safety).
4. HUD is an `AHUD` subclass drawing on `Canvas`. English strings only (default engine font has no CJK glyphs); wrap user-facing text in `NSLOCTEXT("Heritage", ...)`.
5. All new code lives in `Source/Blank/Heritage/` (`Public` / `Private` split not required; keep the module flat under that folder). Class prefix `Heritage`. Log category `LogHeritage`.
6. Do not touch `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, or the `Plugins` list in `Blank.uproject`.
7. **Engine source is ground truth.** UE 5.8 is recent; when any API signature is uncertain, read the header under `C:\Program Files\Epic Games\UE_5.8\Engine\Source\` instead of guessing. Do not use deprecated APIs if the header marks them.
8. Tunable numbers are `UPROPERTY(EditAnywhere, Category="Heritage")` with the defaults given here. No magic numbers in function bodies.
9. The Unreal Editor must be closed while building from the command line. The project sits in a OneDrive folder; if a build fails on a locked file, retry once before investigating.
10. Each milestone must compile with zero errors and zero new warnings before moving on.

Build command (run from PowerShell):

```
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" BlankEditor Win64 Development -Project="C:\Users\Richard\OneDrive\Desktop\Unreal\Blank\Blank.uproject" -WaitMutex
```

## 4. Architecture

Two halves that never reference each other's concrete classes:

```
Player verbs (AHeritageCharacter + UHeritageInteractorComponent)
        │  emits FHeritageStimulus (Impact / Water / Use / Photo)
        ▼
UHeritageObjectComponent on any actor  ──►  actor's own reaction code
        │
        └──►  UHeritageArchiveSubsystem (action log + scanned mementos)  ──►  AHeritageHUD
```

### 4.1 Gameplay tags (native, `HeritageTags.h/.cpp`)

Declare with `UE_DECLARE_GAMEPLAY_TAG_EXTERN` / `UE_DEFINE_GAMEPLAY_TAG`. Add `"GameplayTags"` to `PublicDependencyModuleNames`.

- Stimuli: `Heritage.Stimulus.Impact`, `Heritage.Stimulus.Water`, `Heritage.Stimulus.Use`, `Heritage.Stimulus.Photo`
- States: `Heritage.State.Lit`, `Heritage.State.Hot`, `Heritage.State.Filled`, `Heritage.State.Broken`, `Heritage.State.Locked`, `Heritage.State.Open`, `Heritage.State.Scanned`
- Verbs (for the log): `Heritage.Verb.Grab`, `Drop`, `Throw`, `Use`, `Dash`, `Jump`, `Photo`

### 4.2 `FHeritageStimulus` (`HeritageTypes.h`)

```cpp
USTRUCT(BlueprintType)
struct FHeritageStimulus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FGameplayTag Type;
    UPROPERTY(BlueprintReadWrite) float Magnitude = 0.f;      // Impact: approx. speed change in cm/s
    UPROPERTY(BlueprintReadWrite) TObjectPtr<AActor> Instigator; // the player pawn, if any
    UPROPERTY(BlueprintReadWrite) TObjectPtr<AActor> Source;     // the object that delivered it (thrown book, kettle)
    UPROPERTY(BlueprintReadWrite) FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite) FVector Direction = FVector::ZeroVector;
};
```

Also here: `FHeritageActionRecord { float Time; FGameplayTag Verb; FText Object; FText Target; FText Result; }` and `FHeritageArchiveEntry { FText Name; FText Note; FGameplayTagContainer StateAtCapture; float Time; }`.

### 4.3 `UHeritageObjectComponent` (one component, deliberately)

`UActorComponent`. This is the *only* thing the player code knows about a level object.

- `FText DisplayName`
- `bool bGrabbable`, `bool bMemento`, `FText MementoNote`
- `FGameplayTagContainer StateTags` with `AddState / RemoveState / HasState`, broadcasting `OnStateChanged`
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeritageStimulus, const FHeritageStimulus&, Stimulus)` → `OnStimulus` (BlueprintAssignable, for designers later)
- `bool ReceiveStimulus(const FHeritageStimulus&)`: broadcast `OnStimulus`, then if the owner implements `IHeritageReactive` call `HandleStimulus` and return its result
- `bool CanBeGrabbed(FText& OutReason) const`: false if `!bGrabbable`, if `Hot`, or if scan-gating is on and not `Scanned`; owner may veto through `IHeritageReactive::CanBeGrabbed`
- `static UHeritageObjectComponent* Find(const AActor*)`

Interfaces (`HeritageInterfaces.h`, plain C++ `UINTERFACE`s, not BlueprintNativeEvent — keep it simple):

- `IHeritageReactive`: `virtual bool HandleStimulus(const FHeritageStimulus&)`, `virtual bool CanBeGrabbed(FText& OutReason) const { return true; }`
- `IHeritageUsable` (things that do something when *held* and used): `virtual bool UseOn(AActor* Target, AActor* User, FText& OutResult)`

Helper `UHeritageStatics` (BlueprintFunctionLibrary): `SendStimulus(AActor* Target, const FHeritageStimulus&)`, `LogAction(WorldContext, Verb, Object, Target, Result)`, `Toast(WorldContext, FText)`.

Console variable in `HeritageStatics.cpp`: `heritage.RequireScanToInteract` (int, default `0`).

### 4.4 `UHeritageArchiveSubsystem` (`UWorldSubsystem`)

- `TArray<FHeritageActionRecord> ActionLog`, `LogAction(...)` (also `UE_LOG(LogHeritage, Log, ...)`), `OnActionLogged`
- `TArray<FHeritageArchiveEntry> Entries`, `bool ScanMemento(UHeritageObjectComponent*)` (adds entry with a copy of current `StateTags`, adds `Scanned`, returns false if already scanned), `OnMementoScanned`
- `int32 GetTotalMementos()`: number of object components in the world with `bMemento`. The reel and letter exist from the start (hidden / attached, see §5), so this is always 5.
- `FText CurrentToast; float ToastExpireTime;` + `Toast(FText, Duration=2.5)`
- `bool bDemoComplete`

### 4.5 Player

**`AHeritageCharacter : ACharacter`**

- First-person `UCameraComponent` at relative `(0,0,64)`, `bUsePawnControlRotation = true`. No mesh.
- `UPhysicsHandleComponent`, `UHeritageInteractorComponent`.
- Movement defaults: `MaxWalkSpeed 450`, `JumpZVelocity 500`, `AirControl 0.35`, `NavAgentProps.bCanCrouch = true`.
- Runtime input (created in constructor or `PostInitializeComponents`, stored in `UPROPERTY()`):

| Action | Type | Keys | Notes |
|---|---|---|---|
| Move | Axis2D | W/S/A/D | W: swizzle YXZ; S: swizzle + negate; A: negate; D: none |
| Look | Axis2D | Mouse2D | negate Y |
| Jump | Bool | Space | `Started` → `Jump()`, `Completed` → `StopJumping()`; log `Verb.Jump` on `OnJumped` |
| Crouch | Bool | LeftControl | hold |
| Dash | Bool | LeftShift | |
| Grab/Drop | Bool | LeftMouseButton | |
| Throw | Bool | RightMouseButton | |
| Use | Bool | F | |
| Camera Mode | Bool | Tab | toggle |
| Reset | Bool | R | reopen current level |

  Add the mapping context in `PawnClientRestart` (and `BeginPlay` fallback) through `UEnhancedInputLocalPlayerSubsystem`. `E` and `Q` stay unbound.

- **Dash**: `DashSpeed 1600`, `DashDuration 0.25`, `DashCooldown 0.8`, `DashImpactMagnitude 800`, `DashPushImpulse 60000`. Direction = last movement input, else camera forward flattened. On start: zero `GroundFriction` and `BrakingDecelerationWalking`, `LaunchCharacter(Dir*DashSpeed, true, false)`; restore both on end. While dashing, each tick do a multi sphere sweep (capsule radius + 15) from the capsule centre 60 cm along the dash direction by object types `WorldStatic`, `WorldDynamic`, `PhysicsBody`, `Pawn`, keeping only actors that have a `UHeritageObjectComponent` or a simulating primitive; for each *new* such actor this dash: add impulse to simulating components, and `SendStimulus(Impact, DashImpactMagnitude, Source = self)`. Not allowed in Camera Mode. Log `Verb.Dash` with the first thing hit (or "nothing").

**`UHeritageInteractorComponent`** (on the character; owns all object verbs)

- **Focus**: each tick, line trace from camera `FocusDistance 400` on `ECC_Visibility`, ignoring owner and held actor. Focused actor = hit actor if it has a `UHeritageObjectComponent`. In Camera Mode use `ScanDistance 800`.
- **Reach rule** (`CanReach(Actor, OutReason)`, used by Grab and Use): hit distance ≤ `ReachDistance 220`, else "Too far."; target's bounds centre Z ≤ feet Z + `MaxReachHeight 190`, else "Too high to reach."; character `IsMovingOnGround()`, else no action. Feet Z = actor Z − capsule half-height. The prompt shows the reason instead of the verb when the rule fails, so the player learns *why*.
- **Grab** (LMB, nothing held): `CanReach` then `CanBeGrabbed` → on refusal `Toast(reason)` and log. On success: `PhysicsHandle->GrabComponentAtLocationWithRotation(root primitive, NAME_None, COM, rotation)`, set that primitive to ignore `ECC_Pawn`, remember it. Each tick: `SetTargetLocationAndRotation(Camera + Forward*HoldDistance(110) + Up*(-15), yaw-only rotation)`. If the held body ends up > `BreakDistance 220` from the target (stuck behind geometry), auto-drop.
- If the held actor is destroyed or hidden while held (the door consumes the key), release the handle and clear the held pointer that tick. Hold it as a `TWeakObjectPtr`.
- **Drop** (LMB, holding): release; restore pawn collision after `0.3 s` timer.
- **Throw** (RMB, holding): release, `AddImpulse(CameraForward * ThrowSpeed(1100), NAME_None, /*bVelChange*/ true)`; restore pawn collision after `0.3 s`.
- **Use** (F):
  - Holding something that implements `IHeritageUsable` → `UseOn(Target)` where `Target` = focused actor, else the nearest `UHeritageObjectComponent` owner within `UseAssistRadius 160` of a point 100 cm ahead of the camera. Toast and log the returned result text.
  - Holding something not usable → Toast "Nothing happens."
  - Empty hands → send `Stimulus.Use` to the focused actor.
- **Camera Mode** (Tab): interpolate FOV `90 → 55` (`FInterpTo`, speed 10). While active, Grab/Throw/Use/Dash are ignored; a held object is dropped on entering. If the focused object has `bMemento` and is not `Scanned`, accumulate `ScanProgress`; at `ScanHoldTime 1.0` call `ArchiveSubsystem->ScanMemento`, send `Stimulus.Photo`, Toast "Captured: <name>", start a `0.15 s` white flash. Progress resets when focus changes.
- Exposes read-only state for the HUD: `GetFocused()`, `GetHeld()`, `IsCameraMode()`, `GetScanProgress01()`, `GetPromptText()`, `GetFlashAlpha()`.
- `GetPromptText()` builds the context prompt, e.g. `[LMB] Grab Kettle`, `[F] Use Kettle on Fireplace`, `[LMB] Drop  [RMB] Throw`, `Hold to capture…`.

**`AHeritageHUD : AHUD`** — `DrawHUD()` only, using `GEngine->GetMediumFont()` / `GetLargeFont()`:
crosshair dot; prompt under crosshair; `Holding: <name> (<state>)`; toast (centre-lower, fades); objective checklist top-left (Get the kettle / Fill it / Put out the fire / Take the key / Open the door — ticked from world state); `Mementos n/5`; last 5 action-log lines bottom-left; controls legend top-right; Camera Mode viewfinder (four corner brackets, scan progress bar, flash overlay); `DEMO COMPLETE — press R to reset` banner.

**`AHeritageGameMode : AGameModeBase`** — sets `DefaultPawnClass`, `HUDClass`. In `StartPlay()` (before `Super`): spawn `AHeritageDemoRoom` at the origin. After `Super::StartPlay()`: teleport every player pawn to the room's `GetPlayerSpawnTransform()` and set control rotation. Do not rely on a `PlayerStart`.

## 5. Level objects

Base **`AHeritageProp : AActor`** — `UStaticMeshComponent` root (simulating physics, `SetNotifyRigidBodyCollision(true)`, CCD on), `UHeritageObjectComponent`. Helper `InitShape(EHeritageShape, FVector SizeCm, FLinearColor, float MassKg)` (engine basic shapes are 100 cm → scale = Size/100; `SetMassOverrideInKg`). `SetColor()` via cached `UMaterialInstanceDynamic`.

Impact detection lives here once: `OnComponentHit` → `Speed = NormalImpulse.Size() / Mass`; if `Speed >= MinImpactSpeed (300)` and not within `0.1 s` of the last report: send `Impact(Speed, Source = this, Instigator = last thrower)` to **self** and to **OtherActor**. Skip while held.

| Class | Shape / look | Behaviour |
|---|---|---|
| `AHeritageProp` (book ×3) | small box, dark red, 0.6 kg | Grabbable, no reaction. Ammunition. |
| `AHeritageBreakable` (vase) | cylinder 25×25×45, teal, 2 kg, on a static pedestal | `IHeritageReactive`. Impact ≥ `BreakThreshold 650` (a drop from carry height lands at ≈ 450, a throw at ≈ 1100) → add `Broken`, hide and disable self, spawn 6 small cube shards with random impulse, un-hide and detach the hidden **film reel** (`AHeritageProp`, flat yellow cylinder, `bMemento`) at its location. Log "Vase shattered". |
| `AHeritageKettle` | cylinder 22×22×25 + cone spout (attached mesh), grey, 1.5 kg | `IHeritageUsable`, `bMemento`. `UseOn(Basin)` → add `Filled`, tint blue, "Filled the kettle." `UseOn(anything)` while `Filled` → send `Water` stimulus to target, remove `Filled`, tint grey, "Poured water on <target>." While empty and target is not the basin → return false with "The kettle is empty." |
| `AHeritageBasin` | static box 80×50×85, light blue top | Static (no physics). Marker only; the kettle recognises it by class. |
| `AHeritageFireplace` | static stone box frame with an open front, orange emissive cube + flickering `UPointLightComponent` | `IHeritageReactive`. Starts `Lit`. Has a `UBoxComponent` "hearth" volume; any `UHeritageObjectComponent` owner overlapping it while `Lit` gets `Hot`, and loses it on exit or when the fire dies. `Water` while `Lit` → remove `Lit`, hide flame, light off, clear `Hot` from contents, "The fire hisses out." `Water` while unlit → "The ashes are already wet." |
| Key (`AHeritageKey : AHeritageProp`) | small flat box 12×4×2, gold, 0.2 kg | `bMemento`, `bGrabbable`. Starts inside the hearth, so it is `Hot` → grab refused with "Too hot to touch." `IHeritageUsable::UseOn(Door)` → forwards a `Use` stimulus with `Source = key`. |
| `AHeritageDoor` | static frame + door panel box 100×8×210, brown | `IHeritageReactive`. Starts `Locked`. `Use` with `Source` being an `AHeritageKey` → remove `Locked`, add `Open`, rotate the panel 100° over 0.8 s (tick lerp), destroy the key, set `bDemoComplete`. `Use` otherwise → "Locked. There is a keyhole." Impact → "The door rattles." |
| `AHeritageCabinet` | static box 90×45×230, dark wood | `IHeritageReactive`. Static. Has a `UBoxComponent` over its top surface. Impact ≥ `ShakeThreshold 400` → every simulating prop overlapping the top volume gets a velocity-change impulse of `180 cm/s` away from the wall plus `120` up. "The cabinet shudders." |
| `AHeritageNPC` (butler) | capsule-ish: cylinder body 45×45×150 + sphere head, black; static, blocks pawn | `IHeritageReactive`. Holds the **letter** (`AHeritageProp`, flat white box, `bMemento`, physics off, attached at hand height, `bGrabbable = false` while held). Impact ≥ `StaggerThreshold 400` → tilt 12° for 0.5 s then recover (tick), detach letter, enable its physics, `bGrabbable = true`, "The butler stumbles and drops a letter." Further impacts → stagger only. `Water` → "The butler glares at you, dripping." |
| `AHeritagePortrait` | thin box on wall 80×4×110, with a lighter inner box | `bMemento`, not grabbable. `Use` → "A stern family, painted long ago." |
| Table | static box top 160×80×6 at height 75, four legs | Plain static geometry (no component needed). The jump platform. |

Memento notes: one sentence each, neutral placeholder text (narrative is still "讨论中" in the GDD — do not invent lore).

### `AHeritageDemoRoom`

Builds everything in `BeginPlay` (or an explicit `Build()` called by the game mode). Room interior **800 (X) × 600 (Y) × 350 (Z)** cm centred on the origin, floor top at Z = 0. All positions in cm, room-local:

| Element | Position (X, Y, Z of base centre) | Notes |
|---|---|---|
| Floor | 0, 0, −10 | box 800×600×20, grey |
| Ceiling | 0, 0, 350 | box 800×600×20 |
| Walls | ±400 on X, ±300 on Y | 20 thick, 350 high, off-white |
| Player spawn | −300, 0, 100 | facing +X |
| Fireplace | 380, 0, 0 | on the +X wall, opening faces −X; key on the hearth floor |
| Door | 0, −295, 0 | on the −Y wall |
| Cabinet | −150, 275, 0 | against the +Y wall; kettle on top, centred, 15 cm from the wall |
| Table | −30, 240, 0 | long side along X, touching the cabinet's side |
| Basin | 250, 270, 0 | +Y wall |
| Pedestal + vase | 200, −200, 0 | pedestal box 40×40×90 |
| Butler + letter | −100, −150, 0 | facing +X |
| Portrait | −395, 150, 150 | on the −X wall |
| Books ×3 | on the table | stacked loosely |
| Lights | point light at 0, 0, 320 (intensity ~5000 cd-equivalent, attenuation 1200, warm); fire light at hearth | room must be readable with the fire out |

Spawn the room actor at world `(0, 0, 50)` so its floor does not z-fight with the template map's floor.

Why the kettle is out of reach (see the reach rule in §4.5): cabinet top is at 230, kettle centre ≈ 242. Feet on the floor → max reach height 190 → refused with "Too high to reach." Feet on the table top (Z = 81) → 271 → allowed. Jumping does not help because grabbing requires being on the ground. Trace distance alone would not gate this: from the floor the straight-line distance to the kettle can be under 200 cm.

Config changes (`Config/DefaultEngine.ini`, section `[/Script/EngineSettings.GameMapsSettings]`):

```
GameDefaultMap=/Engine/Maps/Templates/Template_Default
EditorStartupMap=/Engine/Maps/Templates/Template_Default
GlobalDefaultGameMode=/Script/Blank.HeritageGameMode
```

(`Template_Default.umap` exists in this engine install. It provides sky and a floor; the room is self-contained regardless.)

## 6. Self-test

`HeritageSelfTest.cpp`: function `RunHeritageSelfTest(UWorld*)`, exposed as console command `Heritage.SelfTest` (`FAutoConsoleCommandWithWorld`), and auto-run 1 s after `StartPlay` when the command line has `-HeritageSelfTest`, after which it calls `FPlatformMisc::RequestExitWithStatus(false, FailCount == 0 ? 0 : 1)`.

It drives **object logic directly** (no input simulation) and checks, in order:

0. `CanReach(kettle)` is false with the pawn on the floor beside the cabinet; true after teleporting the pawn onto the table top and letting it land (wait a few frames).
1. Key has `Hot`; `CanBeGrabbed` is false.
2. Kettle `UseOn(Fireplace)` while empty returns false; fireplace still `Lit`.
3. Kettle `UseOn(Basin)` → kettle has `Filled`.
4. Kettle `UseOn(Fireplace)` → fireplace not `Lit`; kettle not `Filled`; key not `Hot`; `CanBeGrabbed` true.
5. Door `Use` stimulus without key → still `Locked`.
6. Impact 300 on vase → not `Broken`. Impact 1000 → `Broken`; film reel visible and has physics.
7. Impact 1000 on butler → letter detached and `bGrabbable`.
8. Impact 1000 on cabinet → kettle Z velocity > 0 (re-place the kettle on top first).
9. `ScanMemento(portrait)` → true, entry count 1, `Scanned` set; second call → false.
10. Key `UseOn(Door)` → door has `Open`, `bDemoComplete` true.
11. Action log is non-empty.

Output one line per check (`[PASS]` / `[FAIL] <what>`) and a final `HERITAGE SELFTEST: PASS (12/12)` or `FAIL (n/12)` at `Display` verbosity.

Headless run:

```
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\Richard\OneDrive\Desktop\Unreal\Blank\Blank.uproject" -game -nullrhi -nosound -unattended -nosplash -HeritageSelfTest -log -stdout
```

If the headless run cannot be made to work in a reasonable time (e.g. `-nullrhi` crash), say so in the hand-off notes and leave `Heritage.SelfTest` as an in-PIE console command. Do not fake the result.

## 7. Files

```
Source/Blank/Blank.Build.cs                     (edit: add "GameplayTags"; add "PhysicsCore" only if the linker asks for it)
Config/DefaultEngine.ini                        (edit: three lines in GameMapsSettings)
Source/Blank/Heritage/HeritageLog.h/.cpp
Source/Blank/Heritage/HeritageTags.h/.cpp
Source/Blank/Heritage/HeritageTypes.h
Source/Blank/Heritage/HeritageInterfaces.h/.cpp
Source/Blank/Heritage/HeritageObjectComponent.h/.cpp
Source/Blank/Heritage/HeritageStatics.h/.cpp
Source/Blank/Heritage/HeritageArchiveSubsystem.h/.cpp
Source/Blank/Heritage/HeritageInteractorComponent.h/.cpp
Source/Blank/Heritage/HeritageCharacter.h/.cpp
Source/Blank/Heritage/HeritageHUD.h/.cpp
Source/Blank/Heritage/HeritageGameMode.h/.cpp
Source/Blank/Heritage/HeritageProp.h/.cpp
Source/Blank/Heritage/HeritageProps.h/.cpp      (Breakable, Kettle, Basin, Fireplace, Key, Door, Cabinet, NPC, Portrait)
Source/Blank/Heritage/HeritageDemoRoom.h/.cpp
Source/Blank/Heritage/HeritageSelfTest.cpp
Docs/HERITAGE-1_Handoff.md                      (written by Codex at the end, see §9)
```

## 8. Milestones

Build after each one. Stop and report if a milestone cannot be made to compile.

| # | Deliverable | Done when |
|---|---|---|
| M1 | Build.cs, log, tags, types, interfaces, `UHeritageObjectComponent`, statics, archive subsystem | Compiles. |
| M2 | Character with runtime input (move/look/jump/crouch/reset), game mode, ini changes, empty `AHeritageDemoRoom` that builds floor/walls/ceiling/lights, HUD with crosshair + controls legend | Compiles; game mode spawns room and places the player inside. |
| M3 | `AHeritageProp`, interactor focus/grab/drop/throw, impact detection, books + table, HUD prompt/held/toast/log | Compiles. |
| M4 | Dash; all reactive props (§5); full room layout; objectives on HUD; demo-complete flow | Compiles. |
| M5 | Camera Mode, memento scanning, viewfinder HUD, `heritage.RequireScanToInteract` | Compiles. |
| M6 | Self-test + headless run | `HERITAGE SELFTEST: PASS (12/12)` in the log, or an honest report of what failed. |

## 9. Hand-off (Codex writes `Docs/HERITAGE-1_Handoff.md`)

- What was built, per milestone; anything skipped or changed from this plan and why.
- Final build output tail and the self-test output, copied verbatim.
- Every place an engine API differed from what this plan assumed.
- Tuning values that were guessed and need a human in PIE (reach, throw speed, break thresholds, dash).
- Explicit statement that nothing was play-tested with real input (Codex cannot press keys), so the manual checklist below is still owed.

## 10. Manual acceptance checklist (human, in PIE)

1. Spawn inside the room; WASD/mouse/Space/Ctrl work; `R` resets.
2. Key in fire: grab refused with "Too hot to touch."
3. Kettle from the floor: prompt reads "Too high to reach", also mid-jump. Standing on the table: it can be grabbed.
4. Fresh reset: throwing a book knocks the kettle down. Fresh reset: dashing into the cabinet knocks it down.
5. Empty kettle on fire → "The kettle is empty." Basin fills it. Full kettle douses the fire. Key can be grabbed. Key opens door. Banner shows.
6. Vase survives a gentle drop, breaks when thrown or dashed into; reel appears.
7. Dash into butler → letter drops. Water on butler → text only.
8. `Tab` narrows FOV, shows viewfinder; holding on each of the 5 mementos for 1 s captures it; counter reaches 5/5.
9. `heritage.RequireScanToInteract 1` then reset: kettle/key refuse grab until scanned.
10. Action log lists each of the above as it happens. Carried objects do not shove or launch the player.

## 11. What changed in revision (v1 → v2)

- **Assets**: v1 assumed Input Action / map assets. Codex cannot author `.uasset`, so input is built at runtime and the room is spawned from C++ on an engine template map.
- **Fewer moving parts**: v1 had separate Interactable, Reactive and Memento components. Merged into one `UHeritageObjectComponent`, per the GDD rule "subtract on framework, add on level content".
- **Key bindings**: v1 used `E` for interact. Moved to `F` because the GDD reserves `E`/`Q` for film fast-forward/rewind.
- **Kettle reach**: v1 relied on trace distance and cabinet height. Working the numbers showed the player could reach it from the floor, and occlusion tricks (a lip on the cabinet) also blocked the view from the table. Replaced with an explicit reach rule: max height above the feet, grounded only, with the reason shown in the prompt. Deterministic, tunable, and it tells the player what to solve.
- **Vase threshold**: raised from 500 to 650 so setting the vase down or dropping it does not break it.
- **Held object lifetime**: the door destroys the key while it is in the player's hand; the interactor now has to tolerate that.
- **More than one solution**: added throw-to-knock-down and dash-to-shake for the kettle so the demo actually shows the "general verbs, scene responds" idea rather than a single scripted answer.
- **Hot key generalised**: v1 hard-coded "key is hot while fire lit". Now the hearth volume marks *anything* inside it `Hot`, so the rule is the fireplace's, not the key's.
- **Verification**: added the direct-logic self-test and headless run, since Codex cannot play the game; added the requirement to report rather than paper over a failed headless run.
- **Scan-gating**: the GDD marks "scan to make interactable" as untested, so it is a console variable (default off) rather than baked in.
- **Scope cuts**: removed a v1 sketch of Play/Pause mode and an Archive screen; both belong to later tickets.
- **Lore**: memento text is neutral placeholder; the narrative section of the GDD is still under discussion.

## 12. Open points for the team

1. The project is not a git repository. Recommend `git init` with an Unreal `.gitignore` before Codex starts so its diff can be reviewed; otherwise Codex must be run with its skip-git-check option.
2. `Space` is both jump (this prototype) and film pause (GDD §3.1.1). Not a conflict yet; it will be once film mode exists.
3. Whether scan-gating should default on is a design call to make after trying both.
