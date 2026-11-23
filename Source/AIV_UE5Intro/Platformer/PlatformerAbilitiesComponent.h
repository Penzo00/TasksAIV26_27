// Le abilità di movimento di Bot2, scritte come COMPONENTE (lezione 13):
// non serve toccare BP_Bot2, la GameMode lo aggiunge al personaggio quando nasce.
//
//  * Tuning del Character Movement (UE13): Jump Z Velocity e Air Control.
//  * Forgiveness (UE14): coyote time e jump buffer.
//  * Doppio salto sbloccabile, con il secondo salto CONTROLLABILE: in aria
//    si sceglie la nuova direzione con lo stick/WASD.
//  * Wall run: correndo in aria accanto a un muro con il tag "WallRun" ci si
//    "appiccica" e lo si percorre per un po'.
//  * Wall jump: salto contro il muro, che spinge via dal muro e verso l'alto.
//  * Interazione (UE12): tasto E verso gli oggetti con l'interfaccia
//    IPlatformerInteractable (leve, cartelli...).
//
// Il salto "normale" resta quello di BP_Bot2 (IA_Jump -> Jump): il componente
// ascolta lo STESSO input (Enhanced Input esegue tutti i binding di
// un'azione) e interviene solo quando il personaggio è in aria.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "PlatformerRunSubsystem.h"
#include "PlatformerAbilitiesComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UInputAction;
class UEnhancedInputComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlatformerMoveEvent);

UCLASS(ClassGroup = (Platformer), meta = (BlueprintSpawnableComponent))
class AIV_UE5INTRO_API UPlatformerAbilitiesComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlatformerAbilitiesComponent();

	/** Restituisce il componente dell'actor, creandolo se manca. */
	static UPlatformerAbilitiesComponent* FindOrAdd(AActor* Actor);

	// ------------------------------------------------------------ Tuning (UE13)

	/** Se vero, all'avvio imposta Jump Z Velocity e Air Control qui sotto (i livelli sono misurati con questi valori). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Tuning")
	bool bApplyMovementTuning = true;

	/** Forza del salto (cm/s). Con 650: salto alto ~2,1 m, lungo ~4 m camminando e ~6 m correndo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Tuning", meta = (EditCondition = "bApplyMovementTuning", ClampMin = "100.0"))
	float JumpZVelocity = 650.f;

	/** Quanto si può sterzare in aria (0 = niente, 1 = come a terra). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Tuning", meta = (EditCondition = "bApplyMovementTuning", ClampMin = "0.0", ClampMax = "1.0"))
	float AirControl = 0.4f;

	// ------------------------------------------------------------ Forgiveness (UE14)

	/** Coyote time: secondi in cui si può ancora saltare dopo essere caduti da un bordo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Forgiveness", meta = (ClampMin = "0.0"))
	float CoyoteTime = 0.15f;

	/** Jump buffer: un salto premuto poco prima di atterrare parte appena si tocca terra. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Forgiveness", meta = (ClampMin = "0.0"))
	float JumpBufferTime = 0.15f;

	// ------------------------------------------------------------ Doppio salto

	/** Se vero, il doppio salto è disponibile anche senza averlo sbloccato (per i test). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Doppio salto")
	bool bDoubleJumpAlwaysUnlocked = false;

	/** Quanti salti in più si possono fare in aria. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Doppio salto", meta = (ClampMin = "1"))
	int32 AirJumps = 1;

	/** Spinta verso l'alto del secondo salto (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Doppio salto", meta = (ClampMin = "0.0"))
	float DoubleJumpZVelocity = 600.f;

	/** Quanto il secondo salto segue la direzione premuta: 0 = mantiene la direzione di prima, 1 = gira del tutto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Doppio salto", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DoubleJumpSteering = 1.f;

	/** Velocità orizzontale minima data dal secondo salto se si preme una direzione (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Doppio salto", meta = (ClampMin = "0.0"))
	float DoubleJumpMinHorizontalSpeed = 350.f;

	/** Air Control dopo il secondo salto, fino all'atterraggio: il salto si "pilota" meglio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Doppio salto", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DoubleJumpAirControl = 0.8f;

	// ------------------------------------------------------------ Wall run

	/** Se vero, il wall run e il wall jump sono disponibili senza sblocco (default: sì). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run")
	bool bWallRunAlwaysUnlocked = true;

	/** Se vero, si corre e si salta solo sui muri con il tag qui sotto (consigliato: niente scorciatoie). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run")
	bool bRequireWallTag = true;

	/** Tag dell'actor (o del componente) che rende un muro "percorribile". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run")
	FName WallTag = FName(TEXT("WallRun"));

	/** Canale del trace che cerca il muro ai lati. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run")
	TEnumAsByte<ECollisionChannel> WallTraceChannel = ECC_Visibility;

	/** Distanza del trace laterale, dal centro della capsula (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run", meta = (ClampMin = "10.0"))
	float WallTraceDistance = 80.f;

	/** Velocità orizzontale minima per attaccarsi al muro (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run", meta = (ClampMin = "0.0"))
	float WallRunMinSpeed = 200.f;

	/** Velocità lungo il muro durante la corsa (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run", meta = (ClampMin = "0.0"))
	float WallRunSpeed = 650.f;

	/** Spinta verso l'alto all'inizio della corsa sul muro (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run")
	float WallRunStartUpSpeed = 220.f;

	/** "Gravità" durante la corsa sul muro (cm/s²): più bassa = si resta su più a lungo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run", meta = (ClampMin = "0.0"))
	float WallRunGravity = 380.f;

	/** Durata massima di una corsa sul muro (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run", meta = (ClampMin = "0.1"))
	float WallRunMaxTime = 1.5f;

	/** Se si cade più veloci di così non ci si attacca al muro (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall run", meta = (ClampMin = "0.0"))
	float WallRunMaxFallSpeed = 900.f;

	// ------------------------------------------------------------ Wall jump

	/** Spinta via dal muro (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall jump", meta = (ClampMin = "0.0"))
	float WallJumpAwaySpeed = 600.f;

	/** Spinta verso l'alto (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall jump", meta = (ClampMin = "0.0"))
	float WallJumpUpSpeed = 700.f;

	/** Quanta velocità lungo il muro si conserva saltando (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall jump", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallJumpKeepAlongWall = 0.5f;

	/** Dopo un wall jump, per quanti secondi non ci si riattacca allo STESSO muro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall jump", meta = (ClampMin = "0.0"))
	float SameWallCooldown = 0.45f;

	/** Se vero, toccare un muro (wall run o wall jump) restituisce il doppio salto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Wall jump")
	bool bWallRestoresAirJumps = true;

	// ------------------------------------------------------------ Input e interazione

	/** L'azione di salto (la stessa di BP_Bot2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Input")
	TSoftObjectPtr<UInputAction> JumpAction;

	/** L'azione di interazione (tasto E). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Input")
	TSoftObjectPtr<UInputAction> InteractAction;

	/** Raggio entro cui si può interagire (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Interazione", meta = (ClampMin = "10.0"))
	float InteractRange = 260.f;

	/** Raggio della Sphere Trace di interazione (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Interazione", meta = (ClampMin = "1.0"))
	float InteractRadius = 45.f;

	/** Canale della trace di interazione (nel progetto: InteractionTrace). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita|Interazione")
	TEnumAsByte<ECollisionChannel> InteractChannel = ECC_GameTraceChannel5;

	// ------------------------------------------------------------ Eventi e stato

	UPROPERTY(BlueprintAssignable, Category = "Abilita")
	FOnPlatformerMoveEvent OnDoubleJump;

	UPROPERTY(BlueprintAssignable, Category = "Abilita")
	FOnPlatformerMoveEvent OnWallJump;

	UPROPERTY(BlueprintAssignable, Category = "Abilita")
	FOnPlatformerMoveEvent OnWallRunStarted;

	UPROPERTY(BlueprintAssignable, Category = "Abilita")
	FOnPlatformerMoveEvent OnWallRunEnded;

	UFUNCTION(BlueprintPure, Category = "Abilita")
	bool IsWallRunning() const { return bWallRunning; }

	UFUNCTION(BlueprintPure, Category = "Abilita")
	bool CanDoubleJump() const;

	UFUNCTION(BlueprintPure, Category = "Abilita")
	bool CanWallRun() const;

	/** L'oggetto con cui si interagirebbe premendo E adesso (per l'HUD). */
	UFUNCTION(BlueprintPure, Category = "Abilita")
	AActor* GetFocusedInteractable() const { return FocusedInteractable.Get(); }

	/** Il salto in aria (coyote, wall jump o doppio salto). Lo chiama l'input, ma si può chiamare anche da Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Abilita")
	void HandleJumpPressed();

	/** Interagisce con l'oggetto davanti (tasto E). */
	UFUNCTION(BlueprintCallable, Category = "Abilita")
	void HandleInteractPressed();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	UFUNCTION()
	void HandleMovementModeChanged(ACharacter* InCharacter, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	UFUNCTION()
	void HandleAbilityUnlocked(EPlatformerAbility Ability);

	void TryBindInput();
	bool FindWall(FHitResult& OutHit) const;
	bool IsWallValid(const FHitResult& Hit) const;
	void StartWallRun(const FHitResult& Hit);
	void UpdateWallRun(float DeltaTime);
	void StopWallRun();
	void DoWallJump(const FVector& WallNormal, AActor* Wall);
	void DoDoubleJump();
	void UpdateFocus();
	float Now() const;

	UPROPERTY()
	TObjectPtr<ACharacter> Character;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> Move;

	TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
	TWeakObjectPtr<AActor> FocusedInteractable;
	TWeakObjectPtr<AActor> LastWall;

	float DefaultAirControl = 0.05f;
	float DefaultGravityScale = 1.f;
	int32 AirJumpsUsed = 0;
	bool bJumpedSinceGrounded = false;
	float LeftGroundTime = -100.f;
	float JumpBufferedUntil = -100.f;
	float LastWallJumpTime = -100.f;
	float NextFocusUpdate = 0.f;

	bool bWallRunning = false;
	bool bWallRunSpent = false;   // una corsa sul muro per salto: si ricarica atterrando o con un wall jump
	float WallRunElapsed = 0.f;
	float WallRunVerticalSpeed = 0.f;
	FVector WallRunNormal = FVector::ZeroVector;
	FVector WallRunDirection = FVector::ZeroVector;
};
