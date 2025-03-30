#include "PAA_GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GridGenerator.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Unit.h"
#include "Sniper.h"
#include "Brawler.h"
#include <Engine/DecalActor.h>
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"

APAA_GameMode::APAA_GameMode()
{
    static ConstructorHelpers::FClassFinder<UHUD_Game> HUDGameBPClass(TEXT("/Game/Blueprints/BP_HUD_Game"));
    if (HUDGameBPClass.Class != nullptr)
    {
        HUDGameClass = HUDGameBPClass.Class;
    }

    SelectedUnit = nullptr;

    SniperClass = ASniper::StaticClass();
    BrawlerClass = ABrawler::StaticClass();
    PrimaryActorTick.bCanEverTick = true;

    bIsPlayerTurn = false;
    bGameStarted = false;
    CurrentPhase = EGamePhase::Placement;
    UnitsPlaced = 0;
    PlayerUnitIndex = 0;
    AIUnitIndex = 0;
    bClickInProgress = false;

    // Inizializza gli array dei tipi di unita per il posizionamento
    PlayerUnitTypes.Add(SniperClass);
    PlayerUnitTypes.Add(BrawlerClass);
    AIUnitTypes.Add(SniperClass);
    AIUnitTypes.Add(BrawlerClass);

}

void APAA_GameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!HUDGame)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            // Aggiungi il widget HUD al viewport
            HUDGame = CreateWidget<UHUD_Game>(PC, HUDGameClass); 
            if (HUDGame)
            {
                HUDGame->AddToViewport();
            }
        }
    }

    ShowHUD();
    ShowEnemyHUD();
    DebugCollision();

    PlayerUnitsActedCount = 0;
    bReadyForNextTurn = false;

    // Abilita il cursore del mouse
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = true;
    }

    // Trova e salva il GridGenerator
    for (TActorIterator<AGridGenerator> It(GetWorld()); It; ++It)
    {
        CachedGridGenerator = *It;
        break;
    }
    if (!CachedGridGenerator)
    {
        return;
    }

    // Visuale della camera
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        FVector Center = FVector((CachedGridGenerator->GridSize * CachedGridGenerator->TileSize) / 2.f,
            (CachedGridGenerator->GridSize * CachedGridGenerator->TileSize) / 2.f,
            0.f);
        FVector CameraLocation = Center + FVector(0.f, -50.f, 2800.f);
        FRotator CameraRotation = FRotator(-90.f, 0.f, -90.f);
        FActorSpawnParameters SpawnParams;
        ACameraActor* CameraActor = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraLocation, CameraRotation, SpawnParams);
        if (CameraActor)
        {
            PC->SetViewTarget(CameraActor);
            UE_LOG(LogTemp, Warning, TEXT("Camera impostata al centro della griglia."));
        }
    }
    CoinFlip();
    bGameStarted = true;
}

void APAA_GameMode::UpdateTileHighlightUnderMouse()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    float MouseX, MouseY;
    PC->GetMousePosition(MouseX, MouseY);

    FVector WorldOrigin, WorldDirection;
    if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDirection))
    {
        // Se non troviamo una posizione, ripristina l'ultima evidenza,
        // MA SOLO se la cella non fa parte delle celle di movimento attive.
        if (bHasHoveredTile && CachedGridGenerator->TileInstanceMapping.Contains(LastHoveredGridCoord))
        {
            // Se la cella non è tra quelle di movement range, la ripristiniamo
            if (!AllowedMovementCells.Contains(LastHoveredGridCoord))
            {
                int32 InstanceIndex = CachedGridGenerator->TileInstanceMapping[LastHoveredGridCoord];
                CachedGridGenerator->TileInstancedMeshComponent->SetCustomDataValue(InstanceIndex, 0, 1.0f, true);
            }
            bHasHoveredTile = false;
        }
        return;
    }

    FVector TraceStart = WorldOrigin;
    FVector TraceEnd = WorldOrigin + WorldDirection * 10000.f;
    FHitResult Hit;
    FCollisionQueryParams Params;

    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        // Ottieni la coordinata della griglia dalla posizione dell'impatto
        FVector2D CurrentHoveredGridCoord = CachedGridGenerator->GetGridPositionFromWorld(Hit.Location);

        // Se era già evidenziata una cella diversa, ripristina quella precedente
        if (bHasHoveredTile && LastHoveredGridCoord != CurrentHoveredGridCoord)
        {
            if (CachedGridGenerator->TileInstanceMapping.Contains(LastHoveredGridCoord))
            {
                // Ripristina solo se la cella precedente non appartiene alle celle del movimento
                if (!AllowedMovementCells.Contains(LastHoveredGridCoord))
                {
                    int32 PrevInstanceIndex = CachedGridGenerator->TileInstanceMapping[LastHoveredGridCoord];
                    CachedGridGenerator->TileInstancedMeshComponent->SetCustomDataValue(PrevInstanceIndex, 0, 0.0f, true);
                }
            }
        }

        LastHoveredGridCoord = CurrentHoveredGridCoord;
        bHasHoveredTile = true;

        // Evidenzia la cella corrente se non fa già parte delle celle di movimento (oppure, se vuoi evidenziare sempre l'hover, puoi sovrascriverla)
        if (CachedGridGenerator->TileInstanceMapping.Contains(CurrentHoveredGridCoord))
        {
            int32 InstanceIndex = CachedGridGenerator->TileInstanceMapping[CurrentHoveredGridCoord];

            float NewValue = AllowedMovementCells.Contains(CurrentHoveredGridCoord) ? 2.0f : 2.0f;
            CachedGridGenerator->TileInstancedMeshComponent->SetCustomDataValue(InstanceIndex, 0, NewValue, true);
        }
    }
    else
    {
        // Se il trace non colpisce nulla, rimuovi l'evidenziazione dalla cella precedentemente evidenziata
        if (bHasHoveredTile && CachedGridGenerator->TileInstanceMapping.Contains(LastHoveredGridCoord))
        {
            // Ripristina solo se la cella non fa parte delle celle di movimento
            if (!AllowedMovementCells.Contains(LastHoveredGridCoord))
            {
                int32 InstanceIndex = CachedGridGenerator->TileInstanceMapping[LastHoveredGridCoord];
                CachedGridGenerator->TileInstancedMeshComponent->SetCustomDataValue(InstanceIndex, 0, 1.0f, true);
            }
            bHasHoveredTile = false;
        }
    }
}


void APAA_GameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    if (!PC->IsInputKeyDown(EKeys::LeftMouseButton))
    {
        bClickInProgress = false;
    }

    if (CurrentPhase == EGamePhase::Placement || CurrentPhase == EGamePhase::InGame)
    {
        float MouseX, MouseY;
        PC->GetMousePosition(MouseX, MouseY);

        FVector WorldOrigin, WorldDirection;
        if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDirection))
            return;

        FVector TraceStart = WorldOrigin;
        FVector TraceEnd = WorldOrigin + WorldDirection * 10000.f;
        FHitResult Hit;
        FCollisionQueryParams Params;

        if (CurrentPhase == EGamePhase::Placement || SelectedUnit != nullptr)
        {
            if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
            {
                FVector2D GridCoord = CachedGridGenerator->GetGridPositionFromWorld(Hit.Location);
                FVector TileCenterLocation = CachedGridGenerator->GetWorldLocationFromGrid(GridCoord);
				TileCenterLocation.Z = -10.f;
                UpdateTileHighlightUnderMouse();
            }
        }

        // Gestire il click sinistro del mouse
        if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) && !bClickInProgress)
        {
            bClickInProgress = true;
            if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
            {
                FVector2D ClickedGrid = CachedGridGenerator->GetGridPositionFromWorld(Hit.Location);
                AUnit* ClickedUnit = Cast<AUnit>(Hit.GetActor());

                // Se hai già un'unità selezionata e clicchi su un'altra unità
                if (SelectedUnit && SelectedUnit->IsEnemy == false && ClickedUnit && ClickedUnit != SelectedUnit)
                {
                    // Verifica se l'unità cliccata è nella lista degli attaccabili
                    if (CurrentAttackableUnits.Contains(ClickedUnit))
                    {
                        if (PlayerUnitsAttacked.Contains(SelectedUnit) && PlayerUnitsAttacked[SelectedUnit])
                        {
                            FlushPersistentDebugLines(GetWorld());
                            ResetMovementHighlights();
                            DeselectUnit();
                            return;
                        }

                        SelectedUnit->Attack(ClickedUnit);
                        ShowHUD();
                        ShowEnemyHUD();
                        HUDGame->SetExecutionText(SelectedUnit->UnitName, " attack ", ClickedUnit->UnitName);
                        if (ClickedUnit->Health <= 0)
                        {
                            if (HUDGame)
                            {
                                HUDGame->AddMoveToHistory(ClickedUnit->UnitName, " eliminato da ", SelectedUnit->UnitName);
                            }
                            CheckWinConditions();
                        }

                        FlushPersistentDebugLines(GetWorld());
                        ResetMovementHighlights();
                        PlayerUnitsAttacked.Add(SelectedUnit, true);

                        if (PlayerUnitsMoved.Contains(SelectedUnit) && PlayerUnitsMoved[SelectedUnit])
                        {
                            PlayerUnitsActedCount++;
                        }

                        DeselectUnit();
                        CheckIfTurnComplete();
                    }
                    else
                    {
                        FlushPersistentDebugLines(GetWorld());
                        ResetMovementHighlights();
                        DeselectUnit();
                    }
                }
                else if (ClickedUnit && ClickedUnit->IsEnemy == false && !SelectedUnit && bIsPlayerTurn)
                {
                    bool HasMoved = PlayerUnitsMoved.Contains(ClickedUnit) && PlayerUnitsMoved[ClickedUnit];
                    bool HasAttacked = PlayerUnitsAttacked.Contains(ClickedUnit) && PlayerUnitsAttacked[ClickedUnit];

                    if (HasMoved && HasAttacked)
                    {
                        DebugLog(TEXT("Questa unità ha già completato tutte le sue azioni in questo turno."), FColor::Green, 5.f);

                    }
                    else
                    {
                        FlushPersistentDebugLines(GetWorld());
                        SelectUnit(ClickedUnit); // Seleziona l'unità cliccata
                    }
                }
				else if (ClickedUnit && ClickedUnit->IsEnemy && !SelectedUnit && bIsPlayerTurn)
				{
					DebugLog(TEXT("Unità nemica cliccata."), FColor::Green, 5.f);
                    ResetMovementHighlights();

				}
				// Se hai un'unità selezionata e clicchi su di essa
                else if(SelectedUnit && SelectedUnit == ClickedUnit){
                    DeselectUnit();
					ResetMovementHighlights();
                    FlushPersistentDebugLines(GetWorld());
                }
                // Se hai un'unità selezionata e clicchi su una cella vuota
                else if (SelectedUnit && SelectedUnit->IsEnemy == false && !ClickedUnit && bIsPlayerTurn)
                {
                        if (AllowedMovementCells.Contains(ClickedGrid))
                        {
                            FlushPersistentDebugLines(GetWorld());
                            SelectedUnit->MoveTo(ClickedGrid);

                            float MovementDuration = 0.5f;
                            FTimerHandle MovementTimerHandle; 
                            FTimerDelegate MovementDelegate = FTimerDelegate::CreateUObject(this, &APAA_GameMode::OnMovementCompleted, SelectedUnit);
                            GetWorldTimerManager().SetTimer(MovementTimerHandle, MovementDelegate, MovementDuration, false);

                            ResetMovementHighlights();

                            HUDGame->SetExecutionText(SelectedUnit->UnitName, " -> ", ConvertGridToChessNotation(ClickedGrid));


                            if (PlayerUnitsAttacked.Contains(SelectedUnit))
                            {
                                PlayerUnitsActedCount++;
                            }

                            DeselectUnit();

                        }
                        else
                        {
                            ResetMovementHighlights();
                            DeselectUnit();
                            FlushPersistentDebugLines(GetWorld());
                            DebugLog(TEXT("Cella cliccata non valida per il movimento."), FColor::Green, 5.f);
                            DeselectUnit();
                        }
                }
                else if (CurrentPhase == EGamePhase::Placement)
                {
                    // Posizionamento di una nuova unità
                    OnPlaceUnit();
                }
            }
        }

        if (PC->WasInputKeyJustPressed(EKeys::SpaceBar) && bIsPlayerTurn && CurrentPhase == EGamePhase::InGame)
        {
            if (bReadyForManualSkip)
            {
                DebugLog(TEXT("Turno terminato manualmente dal giocatore."), FColor::Green, 5.f);
                UE_LOG(LogTemp, Warning, TEXT("Turno terminato manualmente dal giocatore."));

                NextTurn();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Devi agire con tutte le tue unità (almeno una azione ciascuna) prima di poter passare il turno."));
                DebugLog(TEXT("Devi agire con tutte le tue unità (almeno una azione ciascuna) prima di poter passare il turno."), FColor::Green, 5.f);
            }
        }

    }
    bool bAnyUnitMoving = false;
    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit && !Unit->IsEnemy && Unit->bIsMoving)
        {
            bAnyUnitMoving = true;
            break;
        }
    }

    if (!bIsPlayerTurn && CurrentPhase == EGamePhase::InGame &&
        PendingAIUnits.Num() == 0 &&
        !GetWorldTimerManager().IsTimerActive(AIUnitTimerHandle) &&
        !bAnyUnitMoving)
    {
        ExecuteAITurn();
    }
}

void APAA_GameMode::OnMovementCompleted(AUnit *Unit)
{
    PlayerUnitsMoved.Add(Unit, true);
    CheckIfTurnComplete();

    UE_LOG(LogTemp, Warning, TEXT("Selected Unit aggiunto ai mossi"));

}


void APAA_GameMode::CheckIfTurnComplete()
{
    int32 TotalPlayerUnits = 0;
    TArray<AUnit*> PlayerUnits;
    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit && !Unit->IsEnemy)
        {
            TotalPlayerUnits++;
            PlayerUnits.Add(Unit);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("CheckIfTurnComplete: %d total player units"), TotalPlayerUnits);

    int32 CompletedUnitsCountAuto = 0;
    int32 CompletedUnitsCountManual = 0;

    for (AUnit* Unit : PlayerUnits)
    {
        bool HasMoved = PlayerUnitsMoved.Contains(Unit) && PlayerUnitsMoved[Unit];
        bool HasAttacked = PlayerUnitsAttacked.Contains(Unit) && PlayerUnitsAttacked[Unit];

        bool UnitCompleteAuto = (HasMoved && HasAttacked) || (HasMoved && !CanUnitAttackAnyEnemy(Unit));
        bool UnitCompleteManual = (HasMoved || HasAttacked);

        if (UnitCompleteAuto)
            CompletedUnitsCountAuto++;

        if (UnitCompleteManual)
            CompletedUnitsCountManual++;

        UE_LOG(LogTemp, Warning, TEXT("Unit %s: Moved=%d, Attacked=%d, AutoComplete=%d, ManualComplete=%d"),
            *Unit->GetName(), HasMoved, HasAttacked, UnitCompleteAuto, UnitCompleteManual);
    }

    bool bAllAuto = (CompletedUnitsCountAuto == TotalPlayerUnits);
    bool bAllManual = (CompletedUnitsCountManual == TotalPlayerUnits);

    bReadyForManualSkip = bAllManual;

    if (bAllAuto)
    {
        UE_LOG(LogTemp, Warning, TEXT("All units have completed all actions automatically. Ending turn."));
        bReadyForNextTurn = true;
        NextTurn();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%d/%d units have fully completed their actions."), CompletedUnitsCountAuto, TotalPlayerUnits);
    }
}

bool APAA_GameMode::CanUnitAttackAnyEnemy(AUnit* Unit)
{
    if (!Unit) return false;

    FVector2D UnitPos = Unit->GetGridPosition();
    UE_LOG(LogTemp, Warning, TEXT("Unit %s at grid position: %s"), *Unit->GetName(), *UnitPos.ToString());

    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* EnemyUnit = *It;
        if (EnemyUnit && EnemyUnit->IsEnemy)
        {
            FVector2D EnemyPos = EnemyUnit->GetGridPosition();
            int32 Distance = FMath::Abs(UnitPos.X - EnemyPos.X) + FMath::Abs(UnitPos.Y - EnemyPos.Y);
            UE_LOG(LogTemp, Warning, TEXT("Checking enemy %s at %s: Distance = %d, AttackRange = %d"),
                *EnemyUnit->GetName(), *EnemyPos.ToString(), Distance, Unit->AttackRange);

            if (Distance <= Unit->AttackRange)
            {
                return true;
            }
        }
    }
    return false;
}


void APAA_GameMode::DeselectUnit()
{
    SelectedUnit = nullptr;
    CurrentAttackableUnits.Empty();
}

void APAA_GameMode::SelectUnit(AUnit* UnitToSelect)
{
    if (!UnitToSelect || !CachedGridGenerator) return;

    if (SelectedUnit != nullptr && SelectedUnit != UnitToSelect)
    {
        FlushPersistentDebugLines(GetWorld());
        ResetMovementHighlights();
    }

    SelectedUnit = UnitToSelect;

    UE_LOG(LogTemp, Warning, TEXT("Unit selected: %s"), *SelectedUnit->GetName());

    bool HasMoved = PlayerUnitsMoved.Contains(SelectedUnit) && PlayerUnitsMoved[SelectedUnit];
    bool HasAttacked = PlayerUnitsAttacked.Contains(SelectedUnit) && PlayerUnitsAttacked[SelectedUnit];

    if (HasMoved && HasAttacked)
    {
        UE_LOG(LogTemp, Warning, TEXT("This unit has already completed all its actions this turn."));
        SelectedUnit = nullptr;
        return;
    }

    if (HasAttacked)
    {
        UE_LOG(LogTemp, Warning, TEXT("This unit has already attacked this turn. It can only move."));
    }

    FVector2D GridPos = SelectedUnit->GetGridPosition();
    FVector CenterLocation = CachedGridGenerator->GetWorldCenterFromGrid(GridPos);

    if (!HasMoved)
    {
        ShowMovementRange(SelectedUnit);
    }

    if (!HasAttacked)
    {
        ShowAttackRange(SelectedUnit);
    }
}


void APAA_GameMode::DebugCollision()
{
    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit)
        {
            FVector UnitLoc = Unit->GetActorLocation();
            DrawDebugBox(GetWorld(), UnitLoc,
                FVector(50.f, 50.f, 50.f),
                FColor::Yellow, false, 5.0f, 0, 15.f);

            UE_LOG(LogTemp, Warning, TEXT("Unit %s is at position %s"),
                *Unit->GetName(), *Unit->GetActorLocation().ToString());
        }
    }
}

void APAA_GameMode::ShowAttackRange(AUnit* Unit)
{
    if (!Unit || !CachedGridGenerator) return;

    CurrentAttackableUnits.Empty(); 

    FVector2D UnitPos = Unit->GetGridPosition();

    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* EnemyUnit = *It;
        if (EnemyUnit && EnemyUnit->IsEnemy)
        {
            FVector2D EnemyPos = EnemyUnit->GetGridPosition();
            int32 Distance = FMath::Abs(UnitPos.X - EnemyPos.X) + FMath::Abs(UnitPos.Y - EnemyPos.Y);

            if (Distance <= Unit->AttackRange)
            {
                // Aggiungi alla lista di unità attaccabili
                CurrentAttackableUnits.Add(EnemyUnit);

                FVector Origin, BoxExtent;
                EnemyUnit->GetActorBounds(false, Origin, BoxExtent);

                FVector DrawLocation = Origin;
                DrawLocation.Z += BoxExtent.Z + 10.0f;

                DrawDebugBox(GetWorld(), DrawLocation,
                    FVector(CachedGridGenerator->TileSize / 2.0f, CachedGridGenerator->TileSize / 2.0f, 5.f),
                    FColor::Red, false, 10.f, 0, 5.f);
            }
        }
    }
}


void APAA_GameMode::ShowMovementRange(AUnit* Unit)
{
    if (!Unit || !CachedGridGenerator) return;

    // Resetta eventuali evidenziazioni precedenti
    ResetMovementHighlights();
    AllowedMovementCells.Empty();

    FVector2D CurrentPos = Unit->GetGridPosition();

    for (int32 x = -Unit->MovementRange; x <= Unit->MovementRange; x++)
    {
        for (int32 y = -Unit->MovementRange; y <= Unit->MovementRange; y++)
        {
            if (FMath::Abs(x) + FMath::Abs(y) <= Unit->MovementRange)
            {
                FVector2D TestPos(CurrentPos.X + x, CurrentPos.Y + y);

                TArray<FGridNode*> Path = CachedGridGenerator->AStarPathfind(CurrentPos, TestPos, Unit->MovementRange);

                if (Path.Num() > 0 && (Path.Num() - 1) <= Unit->MovementRange)
                {
                    AllowedMovementCells.Add(TestPos);

                    if (CachedGridGenerator->TileInstanceMapping.Contains(TestPos))
                    {
                        int32 InstanceIndex = CachedGridGenerator->TileInstanceMapping[TestPos];
                        CachedGridGenerator->TileInstancedMeshComponent->SetCustomDataValue(InstanceIndex, 0, 1.0f, true);
                    }
                }

                for (FGridNode* Node : Path)
                {
                    delete Node;
                }
            }
        }
    }
}

void APAA_GameMode::CoinFlip()
{
    bIsPlayerTurn = FMath::RandBool();
    if (bIsPlayerTurn)
    {
        DebugLog(TEXT("Il giocatore posiziona per primo."), FColor::Green, 5.f);
        HUDGame->SetExecutionText("", "HP WON THE COIN FLIP.", "");
        HUDGame->SetExecutionText("", "HP PLACES FIRST.", "");
        HUDGame->SetExecutionText("", "SPAWN YOUR SNIPER.", "");
    }
    else
    {
        DebugLog(TEXT("AI posiziona per primo."), FColor::Green, 5.f);
        HUDGame->SetExecutionText("", "AI WON THE COIN FLIP.", "");
        HUDGame->SetExecutionText("", "AI PLACES FIRST.", "");
        SimulateAIPlacement();
    }
}

void APAA_GameMode::SpawnUnit(TSubclassOf<AUnit> UnitClass, FVector2D GridPosition, AGridGenerator* GridGenerator, bool IsEnemy)
{
    if (!UnitClass || !GridGenerator) return;
    UWorld* World = GetWorld();
    if (!World) return;

    FVector SpawnLocation = GridGenerator->GetWorldLocationFromGrid(GridPosition);
    SpawnLocation.Z = 1.f;
    FRotator SpawnRotation = FRotator::ZeroRotator;
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AUnit* NewUnit = World->SpawnActor<AUnit>(UnitClass, SpawnLocation, SpawnRotation, SpawnParams);

    FVector2D SpawnCoord = FVector2D(GridPosition.X, GridPosition.Y);


    if (NewUnit)
    {
        NewUnit->IsEnemy = IsEnemy;
        NewUnit->SetGridGenerator(GridGenerator);

        if (IsEnemy)
        {

            UStaticMeshComponent* MeshComp = NewUnit->FindComponentByClass<UStaticMeshComponent>();
            if (MeshComp)
            {
                UMaterial* EnemyMaterial = nullptr;

                if (UnitClass == BrawlerClass)
                {
					NewUnit->UnitName = "AI : B";
                    EnemyMaterial = LoadObject<UMaterial>(nullptr, TEXT("Material'/Game/Textures/Soldier2_Red_Mat.Soldier2_Red_Mat'"));
                }
                else if (UnitClass == SniperClass)
                {
					NewUnit->UnitName = "AI : S";
                    EnemyMaterial = LoadObject<UMaterial>(nullptr, TEXT("Material'/Game/Textures/Soldier1_Red_Mat.Soldier1_Red_Mat'"));
                }

                if (EnemyMaterial)
                {
                    MeshComp->SetMaterial(0, EnemyMaterial);
                }
                
            }
        }

        HUDGame->SetExecutionText(NewUnit->UnitName, " placed in ", ConvertGridToChessNotation(SpawnCoord));
        UE_LOG(LogTemp, Warning, TEXT("Spawnato %s in %s"), *NewUnit->GetName(), *GridPosition.ToString());
    }
}

// Gestisce il posizionamento delle unità da parte del giocatore durante la fase di placement
void APAA_GameMode::OnPlaceUnit()
{
    // Controlla se siamo nella fase di placement e se è il turno del giocatore
    if (CurrentPhase != EGamePhase::Placement || !bIsPlayerTurn) return;


    // Ottieni il controller del giocatore
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    // Ottieni la posizione del mouse e convertila in coordinate di mondo
    float MouseX, MouseY;
    PC->GetMousePosition(MouseX, MouseY);
    FVector WorldOrigin, WorldDirection;
    if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDirection))
        return;

    // Esegui un raycast per determinare la cella cliccata
    FVector TraceStart = WorldOrigin;
    FVector TraceEnd = WorldOrigin + WorldDirection * 10000.f;
    FHitResult Hit;
    FCollisionQueryParams Params;
    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        // Converti la posizione del mondo in coordinate della griglia
        FVector2D GridCoord = CachedGridGenerator->GetGridPositionFromWorld(Hit.Location);

        // Controlla se la posizione è valida
        if (GridCoord.X < 0 || GridCoord.Y < 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Posizione non valida!"));
            DebugLog(TEXT("Non puoi posizionare un'unità qui!"), FColor::Green, 5.f);
            return;
        }

        // Controlla se la cella contiene un ostacolo
        if (CachedGridGenerator->ObstaclePositions.Contains(GridCoord))
        {
            UE_LOG(LogTemp, Warning, TEXT("Non puoi posizionare un'unità su un ostacolo!"));
            DebugLog(TEXT("Non puoi posizionare un'unità su un ostacolo!"), FColor::Green, 5.f);
            return;
        }

        // Spawna l'unità nella posizione selezionata
        SpawnUnit(PlayerUnitTypes[PlayerUnitIndex], GridCoord, CachedGridGenerator, false);
        PlayerUnitIndex++;
        UnitsPlaced++;

        // Controlla se sono state posizionate tutte le unità
        if (UnitsPlaced >= 4)
        {
            // Passa alla fase di gioco
            CurrentPhase = EGamePhase::InGame;
            UE_LOG(LogTemp, Warning, TEXT("Fase di posizionamento terminata."));
            DebugLog(TEXT("Fase di posizionamento terminata."), FColor::Green, 5.f);
        }
        else
        {
            // Passa il turno all'AI
            bIsPlayerTurn = false;
            SimulateAIPlacement();
        }
    }

    // Aggiorna l'HUD per mostrare le unità nemiche e del giocatore
    ShowEnemyHUD();
    ShowHUD();
}

void APAA_GameMode::SimulateAIPlacement()
{

    if (CurrentPhase != EGamePhase::Placement) return;

    // Scegli una cella libera casuale che non contenga ostacoli
    FVector2D AIPos = CachedGridGenerator->GetRandomFreeTile();

    SpawnUnit(AIUnitTypes[AIUnitIndex], AIPos, CachedGridGenerator, true);
    AIUnitIndex++;
    UnitsPlaced++;

    UE_LOG(LogTemp, Warning, TEXT("AI ha posizionato la sua unità in posizione: %s"), *AIPos.ToString());

    if (UnitsPlaced < 4)
    {
        bIsPlayerTurn = true;
        if (PlayerUnitIndex == 0) {
            HUDGame->SetExecutionText("", "SPAWN YOUR SNIPER.", "");
        }
        else {
            HUDGame->SetExecutionText("", "SPAWN YOUR BRAWLER.", "");
        }
    }
    else
    {
        CurrentPhase = EGamePhase::InGame;
        UE_LOG(LogTemp, Warning, TEXT("Fase di posizionamento terminata."));
    }
    ShowHUD();
    ShowEnemyHUD();
}

AUnit* APAA_GameMode::FindNearestPlayerUnit(const FVector& Location)
{
    // Per AI trovo il nemico piu vicino 
    AUnit* NearestPlayerUnit = nullptr;
    float NearestDistance = FLT_MAX;
    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit && (Unit->IsA(ASniper::StaticClass()) || Unit->IsA(ABrawler::StaticClass())))
        {
            float Distance = FVector::Dist(Location, Unit->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestPlayerUnit = Unit;
            }
        }
    }
    return NearestPlayerUnit;
}

void APAA_GameMode::ExecuteAITurn()
{
    UE_LOG(LogTemp, Warning, TEXT("Unità AI da processare: %d"), PendingAIUnits.Num());

    // Raccogliamo tutte le unità del giocatore e dell'AI
    TArray<AUnit*> PlayerUnits;
    TArray<AUnit*> AIUnits;

    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit)
        {
            if (Unit->IsEnemy)
            {
                AIUnits.Add(Unit);
            }
            else
            {
                PlayerUnits.Add(Unit);
            }
        }
    }

    // Se non ci sono unità del giocatore o dell'AI, termina il turno
    if (PlayerUnits.Num() == 0 || AIUnits.Num() == 0)
    {
        NextTurn();
        return;
    }

    // Prepara la coda di unità AI da processare con ritardo
    PendingAIUnits = AIUnits;

    // Inizia a processare la prima unità
    GetWorldTimerManager().SetTimer(AIUnitTimerHandle, this, &APAA_GameMode::ProcessNextAIUnit, 0.1f, false);
}

// Gestisce le azioni dell'AI per ogni unità durante il suo turno
void APAA_GameMode::ProcessNextAIUnit()
{
    UE_LOG(LogTemp, Warning, TEXT("ProcessNextAIUnit avviato. Rimanenti: %d"), PendingAIUnits.Num());

    // Se non ci sono più unità AI da processare, passa al turno successivo
    if (PendingAIUnits.Num() == 0)
    {
        NextTurn();
        return;
    }

    // Prendi la prima unità AI dalla coda e rimuovila
    AUnit* AIUnit = PendingAIUnits[0];
    PendingAIUnits.RemoveAt(0);

    // Ottieni tutte le unità del giocatore
    TArray<AUnit*> PlayerUnits;
    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit && !Unit->IsEnemy)
        {
            PlayerUnits.Add(Unit);
        }
    }

    // Log della selezione dell'unità AI
    UE_LOG(LogTemp, Warning, TEXT("AI seleziona l'unità in posizione %s"), *AIUnit->GetGridPosition().ToString());

    // Controlla le capacità dell'unità
    bool bCanMove = (AIUnit->MovementRange > 0);
    bool bCanAttack = true;
    FVector2D AIUnitPos = AIUnit->GetGridPosition();

    // Cerca un bersaglio in range di attacco
    AUnit* TargetInAttackRange = nullptr;
    for (AUnit* PlayerUnit : PlayerUnits)
    {
        FVector2D PlayerUnitPos = PlayerUnit->GetGridPosition();
        int32 Distance = FMath::Abs(AIUnitPos.X - PlayerUnitPos.X) + FMath::Abs(AIUnitPos.Y - PlayerUnitPos.Y);

        if (Distance <= AIUnit->AttackRange)
        {
            TargetInAttackRange = PlayerUnit;
            break;
        }
    }

    /* STRATEGIA DELL'AI:
    1. Se c'è un bersaglio in range -> attacca direttamente
    2. Se può muoversi -> trova il percorso migliore verso il nemico più vicino
    3. Dopo essersi mosso, verifica se può attaccare
    4. Se non può fare nulla -> passa alla prossima unità */

    // CASO 1: Attacco diretto se il bersaglio è in range
    if (TargetInAttackRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI sceglie di attaccare senza muoversi"));

        // Evidenzia il bersaglio
        DrawDebugBox(GetWorld(), FVector(TargetInAttackRange->GetActorLocation().X, TargetInAttackRange->GetActorLocation().Y, 0),
            FVector(CachedGridGenerator->TileSize / 2.0f, CachedGridGenerator->TileSize / 2.0f, 5.f),
            FColor::Red, false, 2.f, 0, 5.f);

        // Esegui l'attacco
        AIUnit->Attack(TargetInAttackRange);
        ShowHUD();
        ShowEnemyHUD();

        // Aggiorna l'HUD con l'azione
        HUDGame->SetExecutionText(AIUnit->UnitName, " attack ", TargetInAttackRange->UnitName);

        // Controlla se il bersaglio è stato eliminato
        if (TargetInAttackRange->Health <= 0)
        {
            if (HUDGame)
            {
                HUDGame->AddMoveToHistory(TargetInAttackRange->UnitName, " eliminato da ", AIUnit->UnitName);
            }
            CheckWinConditions();
        }
    }
    // CASO 2: Movimento verso il nemico più vicino
    else if (bCanMove)
    {
        AUnit* NearestPlayerUnit = nullptr;
        float NearestDistance = FLT_MAX;

        // Trova l'unità nemica più vicina
        for (AUnit* PlayerUnit : PlayerUnits)
        {
            FVector2D PlayerUnitPos = PlayerUnit->GetGridPosition();
            float Distance = FMath::Sqrt(FMath::Square(AIUnitPos.X - PlayerUnitPos.X) + FMath::Square(AIUnitPos.Y - PlayerUnitPos.Y));
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestPlayerUnit = PlayerUnit;
            }
        }

        if (NearestPlayerUnit)
        {
            // Calcola il percorso migliore usando A*
            FVector2D TargetPos = NearestPlayerUnit->GetGridPosition();
            TArray<FGridNode*> Path = CachedGridGenerator->AStarPathfind(AIUnitPos, TargetPos, AIUnit->MovementRange);

            if (Path.Num() > 0)
            {
                // Determina la posizione migliore in base al movimento disponibile
                int32 Steps = FMath::Min(Path.Num() - 1, AIUnit->MovementRange);
                FVector2D BestMovePosition = Path[Steps]->Position;

                // Controlla se la cella è occupata
                bool bOccupied = false;
                for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
                {
                    AUnit* OtherUnit = *It;
                    if (OtherUnit && OtherUnit != AIUnit && OtherUnit->GetGridPosition() == BestMovePosition)
                    {
                        bOccupied = true;
                        break;
                    }
                }

                // Se occupata, prova con la cella precedente
                if (bOccupied && Steps > 0)
                {
                    BestMovePosition = Path[Steps - 1]->Position;
                }

                // Pulizia della memoria
                for (FGridNode* Node : Path)
                {
                    delete Node;
                }

                // Esegui il movimento
                if (AIUnit->MoveTo(BestMovePosition))
                {
                    // Aggiorna l'HUD con il movimento
                    HUDGame->SetExecutionText(AIUnit->UnitName, " -> ", ConvertGridToChessNotation(BestMovePosition));
                    UE_LOG(LogTemp, Warning, TEXT("AI muove l'unità da %s a %s"), *AIUnitPos.ToString(), *BestMovePosition.ToString());

                    // Verifica se dopo il movimento può attaccare
                    AUnit* NewTargetInRange = nullptr;
                    for (AUnit* PlayerUnit : PlayerUnits)
                    {
                        FVector2D PlayerUnitPos = PlayerUnit->GetGridPosition();
                        int32 Distance = FMath::Abs(AIUnitPos.X - PlayerUnitPos.X) + FMath::Abs(AIUnitPos.Y - PlayerUnitPos.Y);
                        if (Distance <= AIUnit->AttackRange)
                        {
                            NewTargetInRange = PlayerUnit;
                            break;
                        }
                    }

                    // Se può attaccare dopo essersi mosso
                    if (NewTargetInRange)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("AI attacca dopo il movimento"));
                        DrawDebugBox(GetWorld(), FVector(NewTargetInRange->GetActorLocation().X, NewTargetInRange->GetActorLocation().Y, 0),
                            FVector(CachedGridGenerator->TileSize / 2.0f, CachedGridGenerator->TileSize / 2.0f, 5.f),
                            FColor::Red, false, 2.f, 0, 5.f);

                        AIUnit->Attack(NewTargetInRange);
                        ShowHUD();
                        ShowEnemyHUD();
                        HUDGame->SetExecutionText(AIUnit->UnitName, " attack ", NewTargetInRange->UnitName);

                        if (NewTargetInRange->Health <= 0)
                        {
                            if (HUDGame)
                            {
                                HUDGame->AddMoveToHistory(NewTargetInRange->UnitName, " eliminato da ", AIUnit->UnitName);
                            }
                            CheckWinConditions();
                        }
                    }
                }
            }
        }
    }

    // Programma il processamento della prossima unità AI
    GetWorldTimerManager().SetTimer(AIUnitTimerHandle, this, &APAA_GameMode::ProcessNextAIUnit, 1.0f, false);
}
// Gestisce il passaggio di turno tra giocatore e AI
void APAA_GameMode::NextTurn()
{
    // Non permettere il cambio turno durante la fase di posizionamento
    if (CurrentPhase == EGamePhase::Placement) {
        return;
    }

    // Aggiorna l'interfaccia e resetta le selezioni
    ShowHUD();
    ShowEnemyHUD();
    SelectedUnit = nullptr;
    ResetMovementHighlights();

    // Inverte il turno
    bIsPlayerTurn = !bIsPlayerTurn;

    if (bIsPlayerTurn)
    {
        // Setup per il turno del giocatore
        ShowTurnHUD(FText::FromString("Player"));

        // Resetta gli stati delle azioni delle unità
        PlayerUnitsMoved.Empty();
        PlayerUnitsAttacked.Empty();
        PlayerUnitsActedCount = 0;
        bReadyForNextTurn = false;

        // Log di debug
        UE_LOG(LogTemp, Warning, TEXT("Turno del giocatore!"));
        DebugLog(TEXT("Turno del giocatore!"), FColor::Green, 5.f);
    }
    else
    {
        // Setup per il turno dell'AI
        ShowTurnHUD(FText::FromString("AI"));

        // Log di debug
        UE_LOG(LogTemp, Warning, TEXT("Turno dell'AI!"));
        DebugLog(TEXT("Turno dell'AI!"), FColor::Green, 5.f);
    }
}


void APAA_GameMode::CheckWinConditions()
{
    bool bPlayerUnitsAlive = false;
    bool bAIUnitsAlive = false;

    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit && !Unit->IsEnemy && Unit->IsAlive()) 
        {
            bPlayerUnitsAlive = true;
            break;
        }
    }

    // Controlla se ci sono unità dell'IA vive
    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit && Unit->IsEnemy && Unit->IsAlive())
        {
            bAIUnitsAlive = true;
            break;
        }
    }

    if (!bPlayerUnitsAlive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hai perso!"));
        CachedGridGenerator->DestroyGrid();
		ShowGameOverHUD("You Lost.");
        return;
    }

    if (!bAIUnitsAlive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hai vinto!"));
        CachedGridGenerator->DestroyGrid();
		ShowGameOverHUD("You Won.");
        return;
    }
}

void APAA_GameMode::ResetMovementHighlights()
{
    for (FVector2D Cell : AllowedMovementCells)
    {
        if (CachedGridGenerator->TileInstanceMapping.Contains(Cell))
        {
            int32 InstanceIndex = CachedGridGenerator->TileInstanceMapping[Cell];
            CachedGridGenerator->TileInstancedMeshComponent->SetCustomDataValue(InstanceIndex, 0, 0.0f, true);
        }
    }
    AllowedMovementCells.Empty();
}

void APAA_GameMode::DebugLog(const FString& Message, FColor Color , float Duration)
{
    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

}

void APAA_GameMode::ShowHUD()
{
    if (!HUDGame)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            HUDGame = CreateWidget<UHUD_Game>(PC, HUDGameClass);
            if (HUDGame)
            {
                HUDGame->AddToViewport();
            }
        }
    }

    // Prendi tutte le unit amiche
    TArray<AUnit*> PlayerUnits;
    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit && !Unit->IsEnemy && Unit->IsAlive())
        {
            PlayerUnits.Add(Unit);
        }
    }
    HUDGame->UpdateAllPlayerHealthBars(PlayerUnits);
}



void APAA_GameMode::ShowEnemyHUD()
{
    if (!HUDGame)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            HUDGame = CreateWidget<UHUD_Game>(PC, HUDGameClass);
            if (HUDGame)
            {
                HUDGame->AddToViewport();
            }
        }
    }

    // Prendi tutte le unit nemiche

    TArray<AUnit*> AIUnits;

    for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
    {
        AUnit* Unit = *It;
        if (Unit && Unit->IsEnemy && Unit->IsAlive())
        {
            AIUnits.Add(Unit);
        }
    }

    HUDGame->UpdateAllAIHealthBars(AIUnits);
}


void APAA_GameMode::ShowGameOverHUD(FString text) {

    // Assicurati che l'HUD sia visibile
    if (!HUDGame)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            // Aggiungo il widget HUD al viewport
            HUDGame = CreateWidget<UHUD_Game>(PC, HUDGameClass);
            if (HUDGame)
            {
                HUDGame->AddToViewport();
            }
        }
    }
	HUDGame->SetGameOverText(FText::FromString(text));
}

void APAA_GameMode::ShowTurnHUD(FText text) {
	if (!HUDGame)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			// Aggiungo il widget HUD al viewport
			HUDGame = CreateWidget<UHUD_Game>(PC, HUDGameClass);
			if (HUDGame)
			{
				HUDGame->AddToViewport();
			}
		}
	}
	HUDGame->SetTurnText(text);
}


FString APAA_GameMode::ConvertGridToChessNotation(const FVector2D& GridPosition)
{
    int32 X = FMath::RoundToInt(GridPosition.X);
    int32 Y = FMath::RoundToInt(GridPosition.Y);

    X = FMath::Clamp(X, 0, 24);
    TCHAR Letter = 'A' + X;

    Y = FMath::Clamp(Y, 0, 24);
    int32 Number = Y + 1;

    // Combiniamo lettera e numero
    return FString::Printf(TEXT("%c%d"), Letter, Number);
}