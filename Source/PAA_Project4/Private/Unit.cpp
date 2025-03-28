#include "Unit.h"
#include "GridGenerator.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

// Costruttore della classe Unit
AUnit::AUnit()
{
    // Inizializzazione delle proprietà di default
    MovementRange = 0;    // Range di movimento iniziale
    AttackRange = 0;      // Range di attacco iniziale
    Health = 0;           // Salute iniziale
    MaxHealth = 0;        // Salute massima
    MinDamage = 0;        // Danno minimo
    MaxDamage = 0;        // Danno massimo
    IsEnemy = false;      // Nemico o alleato
    UnitType = 0;         // Tipo di unità

    // Creazione del componente mesh
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Caricamento della mesh di default (un piano)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
    if (MeshAsset.Succeeded())
    {
        MeshComponent->SetStaticMesh(MeshAsset.Object);
    }

    // Abilita il tick per l'update frame
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AUnit::BeginPlay()
{
    Super::BeginPlay();

}

// Logica di movimento dell'unità
void AUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsMoving && PathWaypoints.Num() > 0)
    {
        // Calcola la direzione verso il prossimo waypoint
        FVector CurrentLocation = GetActorLocation();
        FVector NextWaypoint = PathWaypoints[0];
        FVector Direction = (NextWaypoint - CurrentLocation).GetSafeNormal();

        // Calcola la distanza da percorrere in questo frame
        float DistanceToMove = MovementSpeed * DeltaTime;
        float RemainingDistance = FVector::Dist(CurrentLocation, NextWaypoint);

        if (DistanceToMove >= RemainingDistance)
        {
            // Se siamo arrivati al waypoint, rimuovilo dalla lista
            SetActorLocation(NextWaypoint);
            PathWaypoints.RemoveAt(0);

            if (PathWaypoints.Num() == 0)
            {
                // Fine del movimento
                bIsMoving = false;
                UE_LOG(LogTemp, Warning, TEXT("Movement completed."));
            }
        }
        else
        {
            // Muovi l'unità verso il waypoint
            FVector NewLocation = CurrentLocation + Direction * DistanceToMove;
            SetActorLocation(NewLocation);
        }
    }
}

// Verifica se l'unità è viva
bool AUnit::IsAlive() const
{
    return Health > 0;
}

// Ottiene la posizione sulla griglia
FVector2D AUnit::GetGridPosition() const
{
    if (GridGenerator)
    {
        return GridGenerator->GetGridPositionFromWorld(GetActorLocation());
    }
    return FVector2D(0, 0);
}

// Logica di attacco
void AUnit::Attack(AUnit* Target)
{
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("No enemy to attack!"));
        return;
    }

    // Calcola il danno casuale tra min e max
    int32 Damage = FMath::RandRange(MinDamage, MaxDamage);
    Target->Health -= Damage;
    UE_LOG(LogTemp, Warning, TEXT("%s attacca %s per %d danni!"), *GetName(), *Target->GetName(), Damage);

    // Logica speciale per Sniper
    if (this->UnitType == 1) {
        int32 selfDamage = rand() % 4;

        if (Target->UnitType == 1 || (Target->UnitType == 0 && FVector::Dist(GetActorLocation(), Target->GetActorLocation()) <= 1.0f))
        {
            this->Health -= selfDamage;
            DrawDebugString(
                GetWorld(),
                this->GetActorLocation(),
                FString::Printf(TEXT("-%d HP Self Damage"), selfDamage),
                nullptr,
                FColor::White,
                2.0f,
                true,
                1.5f
            );
        }

        // Controllo morte dell'attaccante
        if (this->Health <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s died!"), *this->GetName());
            this->Destroy();
        }
    }

    // Mostra il danno inflitto
    FVector TextLocation = Target->GetActorLocation();
    TextLocation.Z += 100.0f;
    DrawDebugString(GetWorld(), TextLocation, FString::Printf(TEXT("-%d HP"), Damage),
        nullptr, FColor::White, 2.0f, true, 1.5f);

    // Controllo morte del bersaglio
    if (Target->Health <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s died!"), *Target->GetName());
        Target->Destroy();
    }
}

// Muove l'unità a una nuova posizione sulla griglia
bool AUnit::MoveTo(FVector2D NewPosition)
{
    if (!GridGenerator) return false;

    FVector2D CurrentPosition = GetGridPosition();

    // Controlla se la destinazione è nel range
    if (FMath::Abs(CurrentPosition.X - NewPosition.X) + FMath::Abs(CurrentPosition.Y - NewPosition.Y) > MovementRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("Destinazione fuori range!"));
        return false;
    }

    // Controlla se la cella è valida
    if (!IsCellValid(NewPosition))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cella occupata o ostacolo!"));
        return false;
    }

    // Trova il percorso con A*
    TArray<FGridNode*> PathNodes = GridGenerator->AStarPathfind(CurrentPosition, NewPosition, MovementRange);
    if (PathNodes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Percorso non trovato!"));
        return false;
    }

    // Converti i nodi in waypoints 3D
    PathWaypoints.Empty();
    for (FGridNode* Node : PathNodes)
    {
        FVector Waypoint = GridGenerator->GetWorldLocationFromGrid(Node->Position);
        Waypoint.Z = GetActorLocation().Z;
        PathWaypoints.Add(Waypoint);
    }

    // Pulizia memoria
    for (FGridNode* Node : PathNodes)
    {
        delete Node;
    }

    // Avvia il movimento
    bIsMoving = true;
    return true;
}

// Verifica se una cella è valida per il movimento
bool AUnit::IsCellValid(FVector2D CellPosition) const
{
    if (!GridGenerator) return false;

    // Controlla i bordi della griglia
    if (CellPosition.X < 0 || CellPosition.Y < 0 ||
        CellPosition.X >= GridGenerator->GridSize || CellPosition.Y >= GridGenerator->GridSize)
    {
        return false;
    }

    // Controlla la presenza di ostacoli
    if (GridGenerator->ObstaclePositions.Contains(CellPosition))
    {
        return false;
    }

    return true;
}

// Imposta il generatore di griglia
void AUnit::SetGridGenerator(AGridGenerator* GridGeneratorRef)
{
    GridGenerator = GridGeneratorRef;
}
