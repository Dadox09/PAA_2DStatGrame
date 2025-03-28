
#include "GridGenerator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h" 


AGridGenerator::AGridGenerator()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    GridSize = 25;               // Griglia 25x25
    TileSize = 110.0f;
    ObstaclePercentage = 0.2f;   // 20% di ostacoli

    TileInstancedMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileInstancedMeshComponent"));
    TileInstancedMeshComponent->SetupAttachment(RootComponent);

    ObstacleInstancedMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ObstacleInstancedMeshComponent"));
    ObstacleInstancedMeshComponent->SetupAttachment(RootComponent);

    // Uso una mesh Plane per i tile, pi� adatta ad un materiale tipo grass
    static ConstructorHelpers::FObjectFinder<UStaticMesh> TileMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (TileMeshAsset.Succeeded())
    {
        TileMesh = TileMeshAsset.Object;
        TileInstancedMeshComponent->SetStaticMesh(TileMesh);
    }
    TileInstancedMeshComponent->NumCustomDataFloats = 1; 


    static ConstructorHelpers::FObjectFinder<UStaticMesh> ObstacleMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (ObstacleMeshAsset.Succeeded())
    {
        ObstacleMesh = ObstacleMeshAsset.Object;
        ObstacleInstancedMeshComponent->SetStaticMesh(ObstacleMesh);
    }
    //Carica i materiali di default
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TileMaterialAsset(TEXT("/Game/StarterContent/Materials/M_Ground_Grass.M_Ground_Grass"));

    MountainInstancedMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MountainInstancedMeshComponent"));
    MountainInstancedMeshComponent->SetupAttachment(RootComponent);


    //Assegna i materiali di default
    if (TileMaterialAsset.Succeeded())
    {
        TileInstancedMeshComponent->SetMaterial(0, TileMaterialAsset.Object);
    }
    if (ObstacleMeshAsset.Succeeded())
    {
        ObstacleMesh = ObstacleMeshAsset.Object;
        ObstacleInstancedMeshComponent->SetStaticMesh(ObstacleMesh);
        MountainInstancedMeshComponent->SetStaticMesh(ObstacleMesh);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObstacleMaterialAsset(TEXT("Material'/Game/Materials/M_ObstacleBase.M_ObstacleBase'"));
    if (ObstacleMaterialAsset.Succeeded())
    {
        ObstacleInstancedMeshComponent->SetMaterial(0, ObstacleMaterialAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MountainMaterialAsset(TEXT("Material'/Game/Materials/M_Mountain.M_Mountain'"));
    if (MountainMaterialAsset.Succeeded())
    {
        MountainInstancedMeshComponent->SetMaterial(0, MountainMaterialAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UTexture2D> ObstacleTextureAsset(TEXT("Texture2D'/Game/Textures/Mountain.Mountain'"));

    if (ObstacleTextureAsset.Succeeded())
    {
        UMaterialInstanceDynamic* ObstacleMaterial = UMaterialInstanceDynamic::Create(
            ObstacleInstancedMeshComponent->GetMaterial(0),
            this
        );
        if (ObstacleMaterial)
        {
            ObstacleMaterial->SetTextureParameterValue(FName("TextureParam"), ObstacleTextureAsset.Object);
            ObstacleInstancedMeshComponent->SetMaterial(0, ObstacleMaterial);
        }
    }
}


void AGridGenerator::DestroyGrid()
{
    if (TileInstancedMeshComponent)
    {
        TileInstancedMeshComponent->ClearInstances();
    }

    if (ObstacleInstancedMeshComponent)
    {
        ObstacleInstancedMeshComponent->ClearInstances();
    }

    if (MountainInstancedMeshComponent)
    {
        MountainInstancedMeshComponent->ClearInstances();
    }

    ObstaclePositions.Empty();
    TileInstanceMapping.Empty();


    UE_LOG(LogTemp, Warning, TEXT("Griglia completamente distrutta!"));
}


void AGridGenerator::BeginPlay()
{
    Super::BeginPlay();
    CreateGrid();
}

void AGridGenerator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}


void AGridGenerator::CreateGrid()
{
    ObstaclePositions.Empty();
    UE_LOG(LogTemp, Warning, TEXT("Creazione della griglia in corso..."));

    int32 TotalCells = GridSize * GridSize;
    int32 MaxObstacleCount = FMath::RoundToInt(TotalCells * ObstaclePercentage);

    TArray<TArray<bool>> ObstacleMap;
    ObstacleMap.SetNum(GridSize);
    UE_LOG(LogTemp, Warning, TEXT("Numero totale di ostacoli previsti: %d"), MaxObstacleCount);
    UE_LOG(LogTemp, Warning, TEXT("Numero di ostacoli effettivamente aggiunti: %d"), ObstaclePositions.Num());
    for (int32 i = 0; i < GridSize; ++i)
    {
        ObstacleMap[i].SetNum(GridSize);
        for (int32 j = 0; j < GridSize; ++j)
        {
            ObstacleMap[i][j] = false;  // Inizialmente non ci sono ostacoli
        }
    }

    // Seleziona casualmente le posizioni per gli ostacoli
    int32 PlacedObstacles = 0;
    while (PlacedObstacles < MaxObstacleCount)
    {
        int32 ObstacleX = FMath::RandRange(0, GridSize - 1);
        int32 ObstacleY = FMath::RandRange(0, GridSize - 1);

        if (!ObstacleMap[ObstacleX][ObstacleY])
        {
            ObstacleMap[ObstacleX][ObstacleY] = true;

            // Verifica che la griglia rimanga connessa
            if (IsGridConnected(ObstacleMap))
            {
                FVector2D Pos(ObstacleX, ObstacleY);
                ObstaclePositions.Add(Pos);
                PlacedObstacles++;
                UE_LOG(LogTemp, Warning, TEXT("Ostacolo aggiunto in posizione: (%d, %d)"), ObstacleX, ObstacleY);
            }
            else
            {
                // Se rende la griglia sconnessa, rimuovi l'ostacolo
                ObstacleMap[ObstacleX][ObstacleY] = false;
            }
        }
    }

    FVector GridOrigin = FVector::ZeroVector;

    for (int32 y = 0; y < GridSize; ++y) {
        for (int32 x = 0; x < GridSize; ++x) {
            FVector TileLocation = GridOrigin + FVector(x * TileSize, y * TileSize, 0);
            FVector2D GridPos(x, y);

            FTransform InstanceTransform;
            InstanceTransform.SetLocation(TileLocation);

            bool isObstacle = ObstaclePositions.Contains(GridPos);
            if (isObstacle) {
                UE_LOG(LogTemp, Warning, TEXT("Creando ostacolo in: (%f, %f)"), GridPos.X, GridPos.Y);

                // Scegli casualmente il tipo di ostacolo
                if (FMath::RandBool()) {
                    if (ObstacleInstancedMeshComponent) {
                        ObstacleInstancedMeshComponent->AddInstance(InstanceTransform);
                    }
                }
                else {
                    if (MountainInstancedMeshComponent) {
                        MountainInstancedMeshComponent->AddInstance(InstanceTransform);
                    }
                }
            }
            else {
                if (TileInstancedMeshComponent) {
                    int32 InstanceIndex = TileInstancedMeshComponent->AddInstance(InstanceTransform);
                    TileInstanceMapping.Add(GridPos, InstanceIndex);
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Numero di ostacoli effettivamente aggiunti: %d"), ObstaclePositions.Num());

}
bool AGridGenerator::IsGridConnected(const TArray<TArray<bool>>& ObstacleMap)
{
    // Trova una cella libera da cui partire
    FIntPoint Start(-1, -1);
    for (int32 y = 0; y < GridSize; ++y)
    {
        for (int32 x = 0; x < GridSize; ++x)
        {
            FVector2D Cell(x, y);
            if (!ObstacleMap[x][y])
            {
                Start = FIntPoint(x, y);
                break;
            }
        }
        if (Start.X != -1)
        {
            break;
        }
    }
    if (Start.X == -1)
    {
        return true;
    }

    const int32 TotalCells = GridSize * GridSize;
    TArray<bool> Visited;
    Visited.Init(false, TotalCells);
    auto Index = [GridSize = this->GridSize](int32 x, int32 y) -> int32 {
        return y * GridSize + x;
        };

    TQueue<FIntPoint> Queue;
    Queue.Enqueue(Start);
    Visited[Index(Start.X, Start.Y)] = true;

    int32 FreeCount = 0;
    int32 TotalFree = 0;
    for (int32 y = 0; y < GridSize; ++y)
    {
        for (int32 x = 0; x < GridSize; ++x)
        {
            if (!ObstacleMap[x][y])
            {
                TotalFree++;
            }
        }
    }

    while (!Queue.IsEmpty())
    {
        FIntPoint Current;
        Queue.Dequeue(Current);
        FreeCount++;

        TArray<FIntPoint> Neighbors;
        Neighbors.Add(FIntPoint(Current.X + 1, Current.Y));
        Neighbors.Add(FIntPoint(Current.X - 1, Current.Y));
        Neighbors.Add(FIntPoint(Current.X, Current.Y + 1));
        Neighbors.Add(FIntPoint(Current.X, Current.Y - 1));

        for (const FIntPoint& Neighbor : Neighbors)
        {
            if (Neighbor.X >= 0 && Neighbor.X < GridSize && Neighbor.Y >= 0 && Neighbor.Y < GridSize)
            {
                if (!ObstacleMap[Neighbor.X][Neighbor.Y])
                {
                    int32 Idx = Index(Neighbor.X, Neighbor.Y);
                    if (!Visited[Idx])
                    {
                        Visited[Idx] = true;
                        Queue.Enqueue(Neighbor);
                    }
                }
            }
        }
    }
    return (FreeCount == TotalFree);
}

void AGridGenerator::DFS(const TArray<TArray<bool>>& ObstacleMap, TArray<TArray<bool>>& Visited, int32 X, int32 Y)
{
    static const int32 dx[] = { 1, -1, 0, 0 };
    static const int32 dy[] = { 0, 0, 1, -1 };

    Visited[X][Y] = true;

    for (int32 i = 0; i < 4; ++i)
    {
        int32 NewX = X + dx[i];
        int32 NewY = Y + dy[i];

        if (NewX >= 0 && NewX < GridSize && NewY >= 0 && NewY < GridSize)
        {
            if (!ObstacleMap[NewX][NewY] && !Visited[NewX][NewY])
            {
                DFS(ObstacleMap, Visited, NewX, NewY);
            }
        }
    }
}

FVector2D AGridGenerator::GetGridPositionFromWorld(const FVector& WorldLocation) const
{
    FVector GridOrigin = FVector::ZeroVector;
    FVector RelativeLocation = WorldLocation - GridOrigin;
    return FVector2D(
        FMath::FloorToInt(RelativeLocation.X / TileSize),
        FMath::FloorToInt(RelativeLocation.Y / TileSize)
    );
}

FVector AGridGenerator::GetWorldLocationFromGrid(const FVector2D& GridPosition) const
{
    FVector GridOrigin = FVector::ZeroVector;
    return GridOrigin + FVector(GridPosition.X * TileSize, GridPosition.Y * TileSize, 50.f);
}

FVector AGridGenerator::GetWorldCenterFromGrid(const FVector2D& GridPosition) const
{
    FVector CornerPos = GetWorldLocationFromGrid(GridPosition);
    return CornerPos + FVector(TileSize, TileSize, 0.0f);
}

FVector2D AGridGenerator::GetRandomFreeTile() const
{
    FVector2D FreeTile;
    do {
        int32 X = FMath::RandRange(0, GridSize - 1);
        int32 Y = FMath::RandRange(0, GridSize - 1);
        FreeTile = FVector2D(X, Y);
    } while (ObstaclePositions.Contains(FreeTile));
    return FreeTile;
}

TArray<FGridNode*> AGridGenerator::AStarPathfind(const FVector2D& StartPos, const FVector2D& EndPos, int32 MovementRange)
{
    TArray<FGridNode*> OpenList;
    TArray<FGridNode*> ClosedList;

    // Crea nodo iniziale
    FGridNode* StartNode = new FGridNode;
    StartNode->Position = StartPos;
    StartNode->gCost = 0;
    StartNode->hCost = FMath::Abs(StartPos.X - EndPos.X) + FMath::Abs(StartPos.Y - EndPos.Y);
    StartNode->fCost = StartNode->gCost + StartNode->hCost;
    StartNode->Parent = nullptr;
    StartNode->bIsObstacle = false; 

    OpenList.Add(StartNode);

    while (OpenList.Num() > 0)
    {
        // Seleziona il nodo con fCost minore
        FGridNode* CurrentNode = OpenList[0];
        for (FGridNode* Node : OpenList)
        {
            if (Node->fCost < CurrentNode->fCost ||
                (Node->fCost == CurrentNode->fCost && Node->hCost < CurrentNode->hCost))
            {
                CurrentNode = Node;
            }
        }

        // Se abbiamo raggiunto la destinazione, ricostruiamo il percorso
        if (CurrentNode->Position == EndPos)
        {
            TArray<FGridNode*> Path;
            FGridNode* Node = CurrentNode;
            while (Node != nullptr)
            {
                Path.Insert(Node, 0); // Inserisce all'inizio dell'array
                Node = Node->Parent;
            }
            return Path;
        }

        OpenList.Remove(CurrentNode);
        ClosedList.Add(CurrentNode);

        // Per ogni vicino (4 direzioni: su, giù, destra, sinistra)
        TArray<FVector2D> Directions = {
            FVector2D(0, 1),
            FVector2D(0, -1),
            FVector2D(1, 0),
            FVector2D(-1, 0)
        };

        for (const FVector2D& Dir : Directions)
        {
            FVector2D NeighborPos = CurrentNode->Position + Dir;

            // Verifica che il vicino sia all'interno dei limiti della griglia
            if (NeighborPos.X < 0 || NeighborPos.Y < 0 || NeighborPos.X >= GridSize || NeighborPos.Y >= GridSize)
                continue;

            // Verifica che il vicino non contenga un ostacolo
            if (ObstaclePositions.Contains(NeighborPos))
                continue;

            // Crea il nodo per il vicino
            FGridNode* NeighborNode = new FGridNode;
            NeighborNode->Position = NeighborPos;
            NeighborNode->bIsObstacle = false;

            // Se il nodo è già nella ClosedList, salta
            bool bInClosedList = false;
            for (FGridNode* ClosedNode : ClosedList)
            {
                if (ClosedNode->Position == NeighborPos)
                {
                    bInClosedList = true;
                    break;
                }
            }
            if (bInClosedList)
            {
                delete NeighborNode;
                continue;
            }

            float NewGCost = CurrentNode->gCost + 1;
            bool bInOpenList = false;
            for (FGridNode* OpenNode : OpenList)
            {
                if (OpenNode->Position == NeighborPos)
                {
                    bInOpenList = true;
                    // Se troviamo un percorso migliore, aggiorniamo i costi
                    if (NewGCost < OpenNode->gCost)
                    {
                        OpenNode->gCost = NewGCost;
                        OpenNode->fCost = NewGCost + OpenNode->hCost;
                        OpenNode->Parent = CurrentNode;
                    }
                    break;
                }
            }

            if (!bInOpenList)
            {
                NeighborNode->gCost = NewGCost;
                NeighborNode->hCost = FMath::Abs(NeighborPos.X - EndPos.X) + FMath::Abs(NeighborPos.Y - EndPos.Y);
                NeighborNode->fCost = NeighborNode->gCost + NeighborNode->hCost;
                NeighborNode->Parent = CurrentNode;
                OpenList.Add(NeighborNode);
            }
        }
    }

    // Se non viene trovato un percorso, restituisce un array vuoto
    return TArray<FGridNode*>();
}
