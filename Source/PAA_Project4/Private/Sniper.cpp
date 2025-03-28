#include "Sniper.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ASniper::ASniper()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (MeshAsset.Succeeded())
    {
        MeshComponent->SetStaticMesh(MeshAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("Material'/Game/Textures/Soldier1_Blue_Mat.Soldier1_Blue_Mat'"));
    if (MaterialAsset.Succeeded())
    {
        MeshComponent->SetMaterial(0, MaterialAsset.Object);
    }

    MeshComponent->SetRelativeLocation(FVector(0, 0, 1.0f));
    MeshComponent->SetWorldScale3D(FVector(1.7f));

    MovementRange = 3;
    AttackRange = 10;
    Health = 20;
	MaxHealth = 20;
    MinDamage = 4;
    MaxDamage = 8;
    IsEnemy = false;
	UnitName = "HP : S";
    UnitType = 1;
}
