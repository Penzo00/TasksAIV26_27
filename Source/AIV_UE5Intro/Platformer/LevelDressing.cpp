#include "LevelDressing.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------
// ASplineFence
// ---------------------------------------------------------------------

ASplineFence::ASplineFence()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	RootComponent = Spline;

	Posts = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Posts"));
	Posts->SetupAttachment(Spline);
	RailInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rails"));
	RailInstances->SetupAttachment(Spline);

	if (Cylinder.Succeeded())
	{
		Posts->SetStaticMesh(Cylinder.Object);
	}
	if (Cube.Succeeded())
	{
		RailInstances->SetStaticMesh(Cube.Object);
	}
}

void ASplineFence::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildFence();
}

void ASplineFence::RebuildFence()
{
	Posts->ClearInstances();
	RailInstances->ClearInstances();
	if (PostMesh)
	{
		Posts->SetStaticMesh(PostMesh);
	}
	if (Material)
	{
		Posts->SetMaterial(0, Material);
		RailInstances->SetMaterial(0, Material);
	}
	const ECollisionEnabled::Type Collision = bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;
	Posts->SetCollisionEnabled(Collision);
	RailInstances->SetCollisionEnabled(Collision);

	// "Per lunghezza" (UE16): non un paletto per punto della spline, ma uno
	// ogni Spacing cm, qualunque sia la forma della spline.
	const float Length = Spline->GetSplineLength();
	const int32 Segments = FMath::Max(1, FMath::RoundToInt(Length / Spacing));
	const float Step = Length / Segments;
	TArray<FVector> Points;
	for (int32 i = 0; i <= Segments; ++i)
	{
		const FVector P = Spline->GetLocationAtDistanceAlongSpline(Step * i, ESplineCoordinateSpace::Local);
		Points.Add(P);
		// il cilindro è alto 100 cm con il pivot al centro
		const FTransform PostXf(FRotator::ZeroRotator, P + FVector(0.f, 0.f, PostSize.Z * 0.5f), PostSize / 100.f);
		Posts->AddInstance(PostXf, false);
	}

	if (Rails <= 0)
	{
		return;
	}
	for (int32 i = 0; i + 1 < Points.Num(); ++i)
	{
		const FVector A = Points[i];
		const FVector B = Points[i + 1];
		const FVector Mid = (A + B) * 0.5f;
		const FVector Dir = B - A;
		const float Span = Dir.Size();
		const FRotator Rot = Dir.Rotation();
		for (int32 r = 0; r < Rails; ++r)
		{
			const float Z = PostSize.Z * (r + 1) / (Rails + 1);
			const FTransform RailXf(Rot, Mid + FVector(0.f, 0.f, Z), FVector(Span / 100.f, RailThickness / 100.f, RailThickness / 100.f));
			RailInstances->AddInstance(RailXf, false);
		}
	}
}

// ---------------------------------------------------------------------
// AScatterArea
// ---------------------------------------------------------------------

AScatterArea::AScatterArea()
{
	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->SetBoxExtent(AreaExtent);
	RootComponent = Area;

	Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Instances"));
	Instances->SetupAttachment(Area);
}

void AScatterArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void AScatterArea::Rebuild()
{
	Area->SetBoxExtent(AreaExtent);
	Instances->ClearInstances();
	Instances->SetStaticMesh(Mesh);
	Instances->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (CullDistance > 0.f)
	{
		Instances->SetCullDistances(static_cast<int32>(CullDistance * 0.8f), static_cast<int32>(CullDistance));
	}
	if (!Mesh)
	{
		return;
	}

	FRandomStream Random(Seed);
	TArray<FVector> Placed;
	const FTransform Frame = GetActorTransform();
	UWorld* World = GetWorld();
	int32 Attempts = 0;
	while (Placed.Num() < Count && Attempts < Count * 20)
	{
		++Attempts;
		const FVector Local(Random.FRandRange(-AreaExtent.X, AreaExtent.X), Random.FRandRange(-AreaExtent.Y, AreaExtent.Y), 0.f);
		FVector WorldPos = Frame.TransformPosition(Local);

		// distanza minima dalle altre istanze (come il raggio del foliage)
		bool bTooClose = false;
		for (const FVector& Other : Placed)
		{
			if (FVector::DistSquared2D(Other, WorldPos) < FMath::Square(MinDistance))
			{
				bTooClose = true;
				break;
			}
		}
		if (bTooClose)
		{
			continue;
		}

		if (bAlignToGround && World)
		{
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ScatterTrace), false, this);
			const FVector Start = WorldPos + FVector(0.f, 0.f, AreaExtent.Z);
			const FVector End = WorldPos - FVector(0.f, 0.f, AreaExtent.Z);
			if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
			{
				continue;   // niente terreno qui sotto
			}
			WorldPos = Hit.ImpactPoint;
		}
		Placed.Add(WorldPos);

		const float Scale = Random.FRandRange(MinScale, MaxScale);
		const FRotator Rot(0.f, Random.FRandRange(0.f, 360.f), 0.f);
		Instances->AddInstance(FTransform(Rot, WorldPos, FVector(Scale)), true);   // in coordinate mondo
	}
}

// ---------------------------------------------------------------------
// AFlickeringLight
// ---------------------------------------------------------------------

AFlickeringLight::AFlickeringLight()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = SceneRoot;
	// la luce è figlia della radice, così il tremolio di posizione è relativo all'actor
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(SceneRoot);
	Light->SetMobility(EComponentMobility::Movable);
}

void AFlickeringLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLightSettings();
}

void AFlickeringLight::ApplyLightSettings()
{
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(BaseIntensity);
	Light->SetUseTemperature(true);
	Light->SetTemperature(Temperature);
	Light->SetAttenuationRadius(AttenuationRadius);
	Light->SetSourceRadius(SourceRadius);
	Light->SetCastShadows(bCastShadows);
}

void AFlickeringLight::BeginPlay()
{
	Super::BeginPlay();
	ApplyLightSettings();
	Seed = FMath::FRand() * 100.f;
}

void AFlickeringLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds * FlickerSpeed;

	// Due "ottave" di rumore di Perlin: una lenta (il fuoco che respira) e
	// una veloce (le fiammelle). Il risultato va da circa -1 a 1.
	const float Slow = FMath::PerlinNoise1D(Clock * 0.35f + Seed);
	const float Fast = FMath::PerlinNoise1D(Clock * 1.7f + Seed * 2.f);
	const float Noise = FMath::Clamp(Slow * 0.7f + Fast * 0.5f, -1.f, 1.f);
	const float Factor = 1.f + Noise * FlickerAmount;
	Light->SetIntensity(BaseIntensity * Factor);

	if (PositionJitter > 0.f)
	{
		const FVector Offset(FMath::PerlinNoise1D(Clock * 0.9f + Seed + 11.f),
			FMath::PerlinNoise1D(Clock * 0.9f + Seed + 23.f),
			FMath::PerlinNoise1D(Clock * 0.9f + Seed + 37.f) * 0.5f);
		Light->SetRelativeLocation(Offset * PositionJitter);
	}

	for (AActor* Glow : GlowActors)
	{
		if (!Glow)
		{
			continue;
		}
		TArray<UMeshComponent*> Meshes;
		Glow->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* MeshComp : Meshes)
		{
			MeshComp->SetScalarParameterValueOnMaterials(GlowParameter, GlowBaseValue * Factor);
		}
	}
}
