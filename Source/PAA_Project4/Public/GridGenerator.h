#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGenerator.generated.h"

struct FGridNode
{
    FVector2D Position;
    float gCost;
    float hCost;
    float fCost;
    FGridNode* Parent;
    bool bIsObstacle;
};

UCLASS()
class PAA_PROJECT4_API AGridGenerator : public AActor
{
    GENERATED_BODY()
    
public:
    AGridGenerator();

protected:
    virtual void BeginPlay() override;

public:

    void DestroyGrid();

    TArray<FGridNode*> AStarPathfind(const FVector2D& StartPos, const FVector2D& EndPos, int32 MovementRange);


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    UInstancedStaticMeshComponent* MountainInstancedMeshComponent;


    UPROPERTY()
    TMap<FVector2D, int32> TileInstanceMapping;


    FVector GetWorldCenterFromGrid(const FVector2D& GridPosition) const;

    UPROPERTY(VisibleAnywhere, Category = "Grid")
    UInstancedStaticMeshComponent* InstancedMeshComponent;

    bool IsGridConnected(const TArray<TArray<bool>>& ObstacleMap);

    void DFS(const TArray<TArray<bool>>& ObstacleMap, TArray<TArray<bool>>& Visited, int32 X, int32 Y);

    FVector2d GetRandomFreeTile() const;

    virtual void Tick(float DeltaTime) override;

    void CreateGrid();

    // Funzioni helper per conversione coordinate
    FVector2D GetGridPositionFromWorld(const FVector& WorldLocation) const;
    FVector GetWorldLocationFromGrid(const FVector2D& GridPosition) const;

    // Dimensione della griglia: per esempio, 25 per una griglia 25x25
    UPROPERTY(EditAnywhere, Category = "Grid")
    int32 GridSize;

    // Dimensione di ogni tile (in unità Unreal)
    UPROPERTY(EditAnywhere, Category = "Grid")
    float TileSize;

    // Percentuale di ostacoli 
    UPROPERTY(EditAnywhere, Category = "Grid")
    float ObstaclePercentage;

    // Array per memorizzare le posizioni  degli ostacoli
    UPROPERTY()
    TArray<FVector2D> ObstaclePositions;

    // Mesh da usare per i tile normali
    UPROPERTY(EditAnywhere, Category = "Grid")
    UStaticMesh* TileMesh;

    // Mesh da usare per gli ostacoli
    UPROPERTY(EditAnywhere, Category = "Grid")
    UStaticMesh* ObstacleMesh;

    // Componente per istanziare i tile normali
    UPROPERTY(VisibleAnywhere, Category = "Grid")
    UInstancedStaticMeshComponent* TileInstancedMeshComponent;

    // Componente per istanziare gli ostacoli
    UPROPERTY(VisibleAnywhere, Category = "Grid")
    UInstancedStaticMeshComponent* ObstacleInstancedMeshComponent;

    UMaterialInstanceDynamic* MovementHighlightMaterial;
    
    
};
