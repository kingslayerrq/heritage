#include "HeritageCharacter.h"
#include "HeritageInteractorComponent.h"
#include "HeritageGameFlowSubsystem.h"
#include "HeritageFilmDirector.h"
#include "HeritageHUD.h"
#include "HeritageStatics.h"
#include "HeritageTags.h"
#include "HeritageObjectComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"

AHeritageCharacter::AHeritageCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(GetRootComponent());
    Camera->SetRelativeLocation(CameraOffset);
    Camera->bUsePawnControlRotation = true;
    PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
    Interactor = CreateDefaultSubobject<UHeritageInteractorComponent>(TEXT("Interactor"));
    OverrideInputComponentClass = UEnhancedInputComponent::StaticClass();
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->JumpZVelocity = JumpSpeed;
    GetCharacterMovement()->AirControl = AirControlAmount;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
}
void AHeritageCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    MappingContext = NewObject<UInputMappingContext>(this);
    auto MakeAction = [this](EInputActionValueType Type)
    {
        UInputAction* Action = NewObject<UInputAction>(this); Action->ValueType = Type; return Action;
    };
    MoveAction = MakeAction(EInputActionValueType::Axis2D);
    MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
    LookAction = MakeAction(EInputActionValueType::Axis2D);
    JumpAction = MakeAction(EInputActionValueType::Boolean);
    CrouchAction = MakeAction(EInputActionValueType::Boolean);
    ResetAction = MakeAction(EInputActionValueType::Boolean);
    GrabAction = MakeAction(EInputActionValueType::Boolean);
    ThrowAction = MakeAction(EInputActionValueType::Boolean);
    UseAction = MakeAction(EInputActionValueType::Boolean);
    DashAction = MakeAction(EInputActionValueType::Boolean);
    CameraModeAction = MakeAction(EInputActionValueType::Boolean);
    MapAction = MakeAction(EInputActionValueType::Boolean);
    ArchiveAction = MakeAction(EInputActionValueType::Boolean);
    ForwardAction = MakeAction(EInputActionValueType::Boolean);
    RewindAction = MakeAction(EInputActionValueType::Boolean);
    BackAction = MakeAction(EInputActionValueType::Boolean);
    ConfirmAction = MakeAction(EInputActionValueType::Boolean);
    NavUpAction = MakeAction(EInputActionValueType::Boolean);
    NavDownAction = MakeAction(EInputActionValueType::Boolean);
    NavLeftAction = MakeAction(EInputActionValueType::Boolean);
    NavRightAction = MakeAction(EInputActionValueType::Boolean);
    auto MapMove = [this](FKey Key, bool bSwizzle, bool bNegative)
    {
        auto& Mapping = MappingContext->MapKey(MoveAction, Key);
        if (bSwizzle) { Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(MappingContext)); }
        if (bNegative) { Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext)); }
    };
    MapMove(EKeys::W, true, false); MapMove(EKeys::S, true, true);
    MapMove(EKeys::A, false, true); MapMove(EKeys::D, false, false);
    auto& LookMapping = MappingContext->MapKey(LookAction, EKeys::Mouse2D);
    auto* NegateY = NewObject<UInputModifierNegate>(MappingContext);
    NegateY->bX = false; NegateY->bY = true; NegateY->bZ = false;
    LookMapping.Modifiers.Add(NegateY);
    MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
    MappingContext->MapKey(CrouchAction, EKeys::LeftControl);
    MappingContext->MapKey(ResetAction, EKeys::R);
    MappingContext->MapKey(GrabAction, EKeys::LeftMouseButton);
    MappingContext->MapKey(ThrowAction, EKeys::RightMouseButton);
    MappingContext->MapKey(UseAction, EKeys::F);
    MappingContext->MapKey(DashAction, EKeys::LeftShift);
    MappingContext->MapKey(CameraModeAction, EKeys::Tab);
    MappingContext->MapKey(MapAction, EKeys::M);
    MappingContext->MapKey(ArchiveAction, EKeys::N);
    MappingContext->MapKey(ForwardAction, EKeys::E);
    MappingContext->MapKey(RewindAction, EKeys::Q);
    MappingContext->MapKey(BackAction, EKeys::BackSpace);
    MappingContext->MapKey(ConfirmAction, EKeys::Enter);
    MappingContext->MapKey(NavUpAction, EKeys::Up);
    MappingContext->MapKey(NavDownAction, EKeys::Down);
    MappingContext->MapKey(NavLeftAction, EKeys::Left);
    MappingContext->MapKey(NavRightAction, EKeys::Right);
}
void AHeritageCharacter::BeginPlay()
{
    Super::BeginPlay();
    Camera->SetRelativeLocation(CameraOffset);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->JumpZVelocity = JumpSpeed;
    GetCharacterMovement()->AirControl = AirControlAmount;
    InstallMappingContext();
}
void AHeritageCharacter::PawnClientRestart() { Super::PawnClientRestart(); InstallMappingContext(); }
void AHeritageCharacter::InstallMappingContext()
{
    if (!MappingContext) { return; }
    if (auto* PC = Cast<APlayerController>(GetController()))
    {
        if (auto* LP = PC->GetLocalPlayer())
        {
            if (auto* Input = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Input->RemoveMappingContext(MappingContext);
                Input->AddMappingContext(MappingContext, 0);
                PC->SetInputMode(FInputModeGameOnly());
                PC->bShowMouseCursor = false;
            }
        }
    }
}
void AHeritageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHeritageCharacter::Move);
        Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHeritageCharacter::Look);
        Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AHeritageCharacter::StartJump);
        Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AHeritageCharacter::EndJump);
        Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AHeritageCharacter::StartCrouch);
        Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AHeritageCharacter::EndCrouch);
        Input->BindAction(ResetAction, ETriggerEvent::Started, this, &AHeritageCharacter::ResetLevel);
        Input->BindAction(GrabAction, ETriggerEvent::Started, Interactor.Get(), &UHeritageInteractorComponent::GrabOrDrop);
        Input->BindAction(ThrowAction, ETriggerEvent::Started, Interactor.Get(), &UHeritageInteractorComponent::Throw);
        Input->BindAction(UseAction, ETriggerEvent::Started, Interactor.Get(), &UHeritageInteractorComponent::Use);
        Input->BindAction(DashAction, ETriggerEvent::Started, this, &AHeritageCharacter::Dash);
        Input->BindAction(CameraModeAction, ETriggerEvent::Started, Interactor.Get(), &UHeritageInteractorComponent::ToggleCameraMode);
        Input->BindAction(MapAction, ETriggerEvent::Started, this, &AHeritageCharacter::ToggleMap);
        Input->BindAction(ArchiveAction, ETriggerEvent::Started, this, &AHeritageCharacter::ToggleArchive);
        Input->BindAction(BackAction, ETriggerEvent::Started, this, &AHeritageCharacter::Back);
        Input->BindAction(ConfirmAction, ETriggerEvent::Started, this, &AHeritageCharacter::Confirm);
        Input->BindAction(ForwardAction, ETriggerEvent::Started, this, &AHeritageCharacter::TransportPressed, 1);
        Input->BindAction(ForwardAction, ETriggerEvent::Completed, this, &AHeritageCharacter::TransportReleased, 1);
        Input->BindAction(ForwardAction, ETriggerEvent::Canceled, this, &AHeritageCharacter::TransportReleased, 1);
        Input->BindAction(RewindAction, ETriggerEvent::Started, this, &AHeritageCharacter::TransportPressed, -1);
        Input->BindAction(RewindAction, ETriggerEvent::Completed, this, &AHeritageCharacter::TransportReleased, -1);
        Input->BindAction(RewindAction, ETriggerEvent::Canceled, this, &AHeritageCharacter::TransportReleased, -1);
        Input->BindAction(NavUpAction, ETriggerEvent::Started, this, &AHeritageCharacter::Navigate, 0, -1);
        Input->BindAction(NavDownAction, ETriggerEvent::Started, this, &AHeritageCharacter::Navigate, 0, 1);
        Input->BindAction(NavLeftAction, ETriggerEvent::Started, this, &AHeritageCharacter::Navigate, -1, 0);
        Input->BindAction(NavRightAction, ETriggerEvent::Started, this, &AHeritageCharacter::Navigate, 1, 0);
    }
}
void AHeritageCharacter::Move(const FInputActionValue& Value)
{
    if (!GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanMove()) { return; }
    const FVector2D Axis = Value.Get<FVector2D>();
    const FRotator Yaw(0, GetControlRotation().Yaw, 0);
    AddMovementInput(Yaw.Vector(), Axis.Y);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}
void AHeritageCharacter::Look(const FInputActionValue& Value)
{
    if (!GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanMove()) { return; }
    const FVector2D Axis = Value.Get<FVector2D>(); AddControllerYawInput(Axis.X); AddControllerPitchInput(Axis.Y);
}
void AHeritageCharacter::StartJump()
{
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!Flow->CanMove()) { return; }
    if (Flow->Era == EHeritageEra::Film && Flow->Director && Flow->Director->GetMode() == EHeritageFilmMode::Play)
    {
        ClearTransport(); Flow->Director->Pause(); return;
    }
    Jump();
}
void AHeritageCharacter::EndJump() { StopJumping(); }
void AHeritageCharacter::StartCrouch() { if (GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanMove()) { Crouch(); } }
void AHeritageCharacter::EndCrouch() { UnCrouch(); }
void AHeritageCharacter::ResetLevel() { if (!GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanMove()) { return; } UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true))); }
void AHeritageCharacter::OnJumped_Implementation()
{
    Super::OnJumped_Implementation();
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Jump, FText::GetEmpty(), FText::GetEmpty(), NSLOCTEXT("Heritage", "Jumped", "Jumped."));
}
void AHeritageCharacter::Dash()
{
    if (!GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanUseVerbs()) { return; }
    const float Now = GetWorld()->GetTimeSeconds();
    if (bDashing || Now < NextDashTime || Interactor->IsCameraMode()) { return; }
    DashDirection = GetLastMovementInputVector(); DashDirection.Z = 0.f;
    if (DashDirection.IsNearlyZero()) { DashDirection = Camera->GetForwardVector(); DashDirection.Z = 0.f; }
    DashDirection = DashDirection.GetSafeNormal();
    if (DashDirection.IsNearlyZero()) { DashDirection = GetActorForwardVector(); }
    auto* Movement = GetCharacterMovement(); SavedFriction = Movement->GroundFriction; SavedBraking = Movement->BrakingDecelerationWalking;
    Movement->GroundFriction = 0.f; Movement->BrakingDecelerationWalking = 0.f;
    DashHits.Empty(); FirstDashHit = NSLOCTEXT("Heritage", "Nothing", "nothing");
    bDashing = true; DashEndTime = Now + DashDuration; NextDashTime = Now + DashCooldown;
    LaunchCharacter(DashDirection * DashSpeed, true, false);
}
void AHeritageCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->CanUseVerbs()) { EndDash(); return; }
    if (!bDashing) { return; }
    TArray<FHitResult> Hits;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    Objects.AddObjectTypesToQuery(ECC_PhysicsBody); Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HeritageDash), false, this);
    if (Interactor->GetHeld()) { Params.AddIgnoredActor(Interactor->GetHeld()); }
    GetWorld()->SweepMultiByObjectType(Hits, GetActorLocation(), GetActorLocation() + DashDirection * DashSweepDistance, FQuat::Identity,
        Objects, FCollisionShape::MakeSphere(GetCapsuleComponent()->GetScaledCapsuleRadius() + DashSweepPadding), Params);
    for (const FHitResult& Hit : Hits)
    {
        AActor* Actor = Hit.GetActor(); UPrimitiveComponent* HitBody = Hit.GetComponent();
        if (!IsValid(Actor) || DashHits.Contains(Actor) || (!UHeritageObjectComponent::Find(Actor) && (!HitBody || !HitBody->IsSimulatingPhysics()))) { continue; }
        if (DashHits.IsEmpty()) { FirstDashHit = UHeritageStatics::NameOf(Actor); }
        DashHits.Add(Actor);
        TInlineComponentArray<UPrimitiveComponent*> Bodies(Actor);
        for (auto* Body : Bodies) { if (Body->IsSimulatingPhysics()) { Body->AddImpulse(DashDirection * DashPushSpeed, NAME_None, true); } }
        FHeritageStimulus Impact; Impact.Type = HeritageTags::Stimulus_Impact; Impact.Magnitude = DashImpactMagnitude;
        Impact.Source = this; Impact.Instigator = this; Impact.Location = Hit.ImpactPoint; Impact.Direction = DashDirection;
        UHeritageStatics::SendStimulus(Actor, Impact);
    }
    if (GetWorld()->GetTimeSeconds() >= DashEndTime) { EndDash(); }
}
void AHeritageCharacter::EndDash()
{
    if (!bDashing) { return; }
    bDashing = false; GetCharacterMovement()->GroundFriction = SavedFriction; GetCharacterMovement()->BrakingDecelerationWalking = SavedBraking;
    UHeritageStatics::LogAction(this, HeritageTags::Verb_Dash, FText::GetEmpty(), FirstDashHit, NSLOCTEXT("Heritage", "Dashed", "Dashed."));
}
void AHeritageCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndDash(); Super::EndPlay(EndPlayReason);
}

void AHeritageCharacter::ToggleMap() { GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->ToggleScreen(EHeritageScreen::Map); }
void AHeritageCharacter::ToggleArchive() { GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->ToggleScreen(EHeritageScreen::Archive); }
void AHeritageCharacter::ClearTransport()
{
    bForwardHeld = false; bRewindHeld = false;
    if (auto* Director = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>()->Director.Get()) { Director->SetTransport(false, false); }
}
void AHeritageCharacter::TransportPressed(int32 Direction)
{
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (!Flow->CanMove() || Flow->Era != EHeritageEra::Film || !Flow->Director) { return; }
    if (Direction > 0) { bForwardHeld = true; } else { bRewindHeld = true; }
    if (Flow->Director->GetMode() == EHeritageFilmMode::Pause) { Flow->Director->ResumePlay(Direction); }
    Flow->Director->SetTransport(bForwardHeld, bRewindHeld);
}
void AHeritageCharacter::TransportReleased(int32 Direction)
{
    if (Direction > 0) { bForwardHeld = false; } else { bRewindHeld = false; }
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->CanMove() && Flow->Era == EHeritageEra::Film && Flow->Director) { Flow->Director->SetTransport(bForwardHeld, bRewindHeld); }
}
void AHeritageCharacter::Back()
{
    auto* Flow = GetWorld()->GetSubsystem<UHeritageGameFlowSubsystem>();
    if (Flow->OpenScreen != EHeritageScreen::None)
    {
        if (auto* PC = Cast<APlayerController>(GetController())) { if (auto* HUD = Cast<AHeritageHUD>(PC->GetHUD())) { HUD->Back(); } }
    }
    else if (Flow->Era == EHeritageEra::Film && Flow->Director) { Flow->Director->Rollback(); }
}
void AHeritageCharacter::Confirm()
{
    if (auto* PC = Cast<APlayerController>(GetController())) { if (auto* HUD = Cast<AHeritageHUD>(PC->GetHUD())) { HUD->Confirm(); } }
}
void AHeritageCharacter::Navigate(int32 X, int32 Y)
{
    if (auto* PC = Cast<APlayerController>(GetController())) { if (auto* HUD = Cast<AHeritageHUD>(PC->GetHUD())) { HUD->Navigate(X, Y); } }
}
