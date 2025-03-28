#include "Brawler.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABrawler::ABrawler()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (MeshAsset.Succeeded())
    {
        MeshComponent->SetStaticMesh(MeshAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("Material'/Game/Textures/Soldier2_Blue_Mat.Soldier2_Blue_Mat'"));
    if (MaterialAsset.Succeeded())
    {
        MeshComponent->SetMaterial(0, MaterialAsset.Object);
    }

    MeshComponent->SetRelativeLocation(FVector(0, 0, 1.0f));
    MeshComponent->SetWorldScale3D(FVector(1.7f));

    MovementRange = 6;
    AttackRange = 1;
    Health = 40;
	MaxHealth = 40;
    MinDamage = 1;
    MaxDamage = 6;
	IsEnemy = false;
	UnitName = "HP : B";
    UnitType = 0;
}
