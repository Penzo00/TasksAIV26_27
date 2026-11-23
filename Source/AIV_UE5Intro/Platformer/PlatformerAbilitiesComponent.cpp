#include "PlatformerAbilitiesComponent.h"

#include "PlatformerInteractable.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"

UPlatformerAbilitiesComponent::UPlatformerAbilitiesComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	JumpAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Input/Actions/IA_Jump.IA_Jump")));
	InteractAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Input/Actions/IA_Interact.IA_Interact")));
}

UPlatformerAbilitiesComponent* UPlatformerAbilitiesComponent::FindOrAdd(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (UPlatformerAbilitiesComponent* Existing = Actor->FindComponentByClass<UPlatformerAbilitiesComponent>())
	{
		return Existing;
	}
	UPlatformerAbilitiesComponent* NewComp = NewObject<UPlatformerAbilitiesComponent>(Actor, TEXT("PlatformerAbilities"));
	Actor->AddInstanceComponent(NewComp);
	NewComp->RegisterComponent();   // chiama anche BeginPlay, perché il gioco è già partito
	return NewComp;
}

float UPlatformerAbilitiesComponent::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

void UPlatformerAbilitiesComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	Move = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !Move)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// UE13: i parametri del Character Movement. I livelli sono misurati su questi valori.
	if (bApplyMovementTuning)
	{
		Move->JumpZVelocity = JumpZVelocity;
		Move->AirControl = AirControl;
	}
	DefaultAirControl = Move->AirControl;
	DefaultGravityScale = Move->GravityScale;

	// Il nostro tick prima di quello del Character Movement: la velocità
	// del wall run viene usata nello stesso frame.
	Move->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick);

	Character->LandedDelegate.AddDynamic(this, &UPlatformerAbilitiesComponent::HandleLanded);
	Character->MovementModeChangedDelegate.AddDynamic(this, &UPlatformerAbilitiesComponent::HandleMovementModeChanged);

	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->OnAbilityUnlocked.AddDynamic(this, &UPlatformerAbilitiesComponent::HandleAbilityUnlocked);
	}

	TryBindInput();
}

void UPlatformerAbilitiesComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bWallRunning)
	{
		StopWallRun();
	}
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->OnAbilityUnlocked.RemoveDynamic(this, &UPlatformerAbilitiesComponent::HandleAbilityUnlocked);
	}
	Super::EndPlay(EndPlayReason);
}

void UPlatformerAbilitiesComponent::TryBindInput()
{
	if (!Character || BoundInput.IsValid())
	{
		return;
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Character->InputComponent);
	if (!Input)
	{
		return;   // il pawn non è ancora posseduto: si riprova al prossimo tick
	}
	// Enhanced Input esegue TUTTI i binding della stessa azione: quello di
	// BP_Bot2 (Jump) e questo convivono.
	if (UInputAction* Jump = JumpAction.LoadSynchronous())
	{
		Input->BindAction(Jump, ETriggerEvent::Started, this, &UPlatformerAbilitiesComponent::HandleJumpPressed);
	}
	if (UInputAction* Interact = InteractAction.LoadSynchronous())
	{
		Input->BindAction(Interact, ETriggerEvent::Started, this, &UPlatformerAbilitiesComponent::HandleInteractPressed);
	}
	BoundInput = Input;
}

void UPlatformerAbilitiesComponent::HandleAbilityUnlocked(EPlatformerAbility Ability)
{
	if (Ability == EPlatformerAbility::DoubleJump)
	{
		AirJumpsUsed = 0;   // si può usare subito, anche a mezz'aria
	}
}

bool UPlatformerAbilitiesComponent::CanDoubleJump() const
{
	if (bDoubleJumpAlwaysUnlocked)
	{
		return true;
	}
	const UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	return Run && Run->HasAbility(EPlatformerAbility::DoubleJump);
}

bool UPlatformerAbilitiesComponent::CanWallRun() const
{
	if (bWallRunAlwaysUnlocked)
	{
		return true;
	}
	const UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	return Run && Run->HasAbility(EPlatformerAbility::WallRun);
}

// ---------------------------------------------------------------------
// Terra / aria
// ---------------------------------------------------------------------

void UPlatformerAbilitiesComponent::HandleMovementModeChanged(ACharacter* InCharacter, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (!Move)
	{
		return;
	}
	if (Move->IsFalling() && PrevMovementMode == MOVE_Walking)
	{
		// Si è lasciato il terreno: con un salto (velocità verso l'alto) o
		// camminando oltre il bordo (qui scatta il coyote time).
		LeftGroundTime = Now();
		bJumpedSinceGrounded = Move->Velocity.Z > 10.f;
	}
}

void UPlatformerAbilitiesComponent::HandleLanded(const FHitResult& Hit)
{
	if (bWallRunning)
	{
		StopWallRun();
	}
	AirJumpsUsed = 0;
	bJumpedSinceGrounded = false;
	bWallRunSpent = false;
	LastWall.Reset();
	if (Move)
	{
		Move->AirControl = DefaultAirControl;
	}
	// Il jump buffer si esegue al prossimo tick: durante l'atterraggio il
	// Character azzera lo stato del salto.
}

// ---------------------------------------------------------------------
// Il salto in aria
// ---------------------------------------------------------------------

void UPlatformerAbilitiesComponent::HandleJumpPressed()
{
	if (!Character || !Move)
	{
		return;
	}
	if (!Move->IsFalling())
	{
		return;   // a terra salta BP_Bot2 da solo
	}

	// 1) sul muro: wall jump
	if (bWallRunning)
	{
		DoWallJump(WallRunNormal, LastWall.Get());
		return;
	}

	// 2) coyote time: appena caduti da un bordo vale ancora il salto da terra
	if (!bJumpedSinceGrounded && Now() - LeftGroundTime <= CoyoteTime)
	{
		bJumpedSinceGrounded = true;
		Character->LaunchCharacter(FVector(0.f, 0.f, Move->JumpZVelocity), false, true);
		return;
	}

	// 3) accanto a un muro percorribile: wall jump anche senza wall run
	if (CanWallRun())
	{
		FHitResult Wall;
		if (FindWall(Wall))
		{
			DoWallJump(Wall.ImpactNormal, Wall.GetActor());
			return;
		}
	}

	// 4) doppio salto
	if (CanDoubleJump() && AirJumpsUsed < AirJumps)
	{
		DoDoubleJump();
		return;
	}

	// 5) niente da fare adesso: si "prenota" il salto per l'atterraggio
	JumpBufferedUntil = Now() + JumpBufferTime;
}

void UPlatformerAbilitiesComponent::DoDoubleJump()
{
	const FVector Horizontal(Move->Velocity.X, Move->Velocity.Y, 0.f);
	FVector NewHorizontal = Horizontal;

	// Il secondo salto è "controllabile": va dove si sta premendo adesso.
	const FVector Input = Character->GetLastMovementInputVector();
	if (Input.SizeSquared2D() > 0.01f)
	{
		const float Speed = FMath::Max(Horizontal.Size(), DoubleJumpMinHorizontalSpeed);
		const FVector Wanted = Input.GetSafeNormal2D() * Speed;
		NewHorizontal = FMath::Lerp(Horizontal, Wanted, DoubleJumpSteering);
	}

	Character->LaunchCharacter(NewHorizontal + FVector(0.f, 0.f, DoubleJumpZVelocity), true, true);
	Move->AirControl = FMath::Max(Move->AirControl, DoubleJumpAirControl);
	++AirJumpsUsed;
	bJumpedSinceGrounded = true;
	OnDoubleJump.Broadcast();
}

// ---------------------------------------------------------------------
// Wall run e wall jump
// ---------------------------------------------------------------------

bool UPlatformerAbilitiesComponent::IsWallValid(const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit || FMath::Abs(Hit.ImpactNormal.Z) > 0.3f)
	{
		return false;   // niente pavimenti né soffitti
	}
	AActor* WallActor = Hit.GetActor();
	if (WallActor && WallActor == LastWall.Get() && Now() - LastWallJumpTime < SameWallCooldown)
	{
		return false;   // appena saltati via da questo muro
	}
	if (!bRequireWallTag)
	{
		return true;
	}
	const UPrimitiveComponent* Comp = Hit.GetComponent();
	return (WallActor && WallActor->ActorHasTag(WallTag)) || (Comp && Comp->ComponentHasTag(WallTag));
}

bool UPlatformerAbilitiesComponent::FindWall(FHitResult& OutHit) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FVector Forward = FVector(Move->Velocity.X, Move->Velocity.Y, 0.f).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FVector Start = Character->GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlatformerWallTrace), false, Character);
	const FVector Dirs[] = { Right, -Right, Forward };
	bool bFound = false;
	float BestDistance = TNumericLimits<float>::Max();
	for (const FVector& Dir : Dirs)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, Start + Dir * WallTraceDistance, WallTraceChannel, Params)
			&& IsWallValid(Hit) && Hit.Distance < BestDistance)
		{
			BestDistance = Hit.Distance;
			OutHit = Hit;
			bFound = true;
		}
	}
	return bFound;
}

void UPlatformerAbilitiesComponent::StartWallRun(const FHitResult& Hit)
{
	bWallRunning = true;
	WallRunElapsed = 0.f;
	WallRunNormal = FVector(Hit.ImpactNormal.X, Hit.ImpactNormal.Y, 0.f).GetSafeNormal();
	LastWall = Hit.GetActor();

	// La direzione lungo il muro è perpendicolare alla normale; si sceglie il
	// verso in cui si stava già andando.
	WallRunDirection = FVector::CrossProduct(WallRunNormal, FVector::UpVector).GetSafeNormal();
	if (FVector::DotProduct(WallRunDirection, Move->Velocity) < 0.f)
	{
		WallRunDirection = -WallRunDirection;
	}
	WallRunVerticalSpeed = FMath::Max(Move->Velocity.Z, WallRunStartUpSpeed);
	DefaultGravityScale = Move->GravityScale;
	Move->GravityScale = 0.f;   // la "gravità" la gestiamo noi, più dolce
	if (bWallRestoresAirJumps)
	{
		AirJumpsUsed = 0;
	}
	OnWallRunStarted.Broadcast();
}

void UPlatformerAbilitiesComponent::UpdateWallRun(float DeltaTime)
{
	WallRunElapsed += DeltaTime;

	// Il muro c'è ancora? Trace verso il muro.
	FHitResult Hit;
	const FVector Start = Character->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlatformerWallRunTrace), false, Character);
	const bool bWall = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - WallRunNormal * WallTraceDistance, WallTraceChannel, Params)
		&& Hit.bBlockingHit && FMath::Abs(Hit.ImpactNormal.Z) < 0.3f;
	const bool bInput = Character->GetLastMovementInputVector().SizeSquared2D() > 0.01f;

	if (!bWall || !bInput || WallRunElapsed > WallRunMaxTime || !Move->IsFalling())
	{
		StopWallRun();
		return;
	}

	WallRunNormal = FVector(Hit.ImpactNormal.X, Hit.ImpactNormal.Y, 0.f).GetSafeNormal();
	WallRunVerticalSpeed -= WallRunGravity * DeltaTime;
	// lungo il muro + un filo verso il muro per restarci attaccati + la componente verticale
	Move->Velocity = WallRunDirection * WallRunSpeed - WallRunNormal * 60.f + FVector(0.f, 0.f, WallRunVerticalSpeed);
}

void UPlatformerAbilitiesComponent::StopWallRun()
{
	if (!bWallRunning)
	{
		return;
	}
	bWallRunning = false;
	bWallRunSpent = true;
	if (Move)
	{
		Move->GravityScale = DefaultGravityScale;
	}
	OnWallRunEnded.Broadcast();
}

void UPlatformerAbilitiesComponent::DoWallJump(const FVector& WallNormal, AActor* Wall)
{
	const FVector Normal = FVector(WallNormal.X, WallNormal.Y, 0.f).GetSafeNormal();
	const FVector Along = FVector::CrossProduct(Normal, FVector::UpVector).GetSafeNormal();
	const float AlongSpeed = FVector::DotProduct(Move->Velocity, Along) * WallJumpKeepAlongWall;

	StopWallRun();
	const FVector Launch = Normal * WallJumpAwaySpeed + Along * AlongSpeed + FVector(0.f, 0.f, WallJumpUpSpeed);
	Character->LaunchCharacter(Launch, true, true);

	LastWall = Wall;
	LastWallJumpTime = Now();
	bWallRunSpent = false;   // dopo un wall jump ci si può attaccare a un altro muro
	bJumpedSinceGrounded = true;
	if (bWallRestoresAirJumps)
	{
		AirJumpsUsed = 0;
	}
	OnWallJump.Broadcast();
}

// ---------------------------------------------------------------------
// Interazione (UE12)
// ---------------------------------------------------------------------

void UPlatformerAbilitiesComponent::UpdateFocus()
{
	FocusedInteractable.Reset();
	const UWorld* World = GetWorld();
	if (!World || !Character)
	{
		return;
	}
	const FVector Start = Character->GetActorLocation();
	const FVector End = Start + Character->GetActorForwardVector() * InteractRange;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlatformerInteractTrace), false, Character);
	FHitResult Hit;
	if (World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, InteractChannel,
		FCollisionShape::MakeSphere(InteractRadius), Params))
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->GetClass()->ImplementsInterface(UPlatformerInteractable::StaticClass()))
		{
			FocusedInteractable = HitActor;
		}
	}
}

void UPlatformerAbilitiesComponent::HandleInteractPressed()
{
	UpdateFocus();
	if (AActor* Target = FocusedInteractable.Get())
	{
		IPlatformerInteractable::Execute_Interact(Target, Character);
	}
}

// ---------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------

void UPlatformerAbilitiesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Character || !Move)
	{
		return;
	}
	TryBindInput();

	// jump buffer: salto prenotato in aria, eseguito appena si tocca terra
	if (JumpBufferedUntil > 0.f && Move->IsMovingOnGround())
	{
		if (Now() <= JumpBufferedUntil)
		{
			Character->Jump();
		}
		JumpBufferedUntil = -100.f;
	}

	if (bWallRunning)
	{
		UpdateWallRun(DeltaTime);
	}
	else if (!bWallRunSpent && Move->IsFalling() && CanWallRun())
	{
		const FVector Horizontal(Move->Velocity.X, Move->Velocity.Y, 0.f);
		const bool bInput = Character->GetLastMovementInputVector().SizeSquared2D() > 0.01f;
		if (bInput && Horizontal.Size() >= WallRunMinSpeed && Move->Velocity.Z > -WallRunMaxFallSpeed)
		{
			FHitResult Wall;
			// per la corsa servono muri AI LATI (il trace in avanti è solo per il wall jump)
			if (FindWall(Wall) && FMath::Abs(FVector::DotProduct(Wall.ImpactNormal.GetSafeNormal2D(), Horizontal.GetSafeNormal())) < 0.7f)
			{
				StartWallRun(Wall);
			}
		}
	}

	if (Now() >= NextFocusUpdate)
	{
		NextFocusUpdate = Now() + 0.1f;
		UpdateFocus();
	}
}
