#pragma once
#include "Engine/Engine.h"
#include "Unit.h"
#include "CoreMinimal.h"
#include "HUD_Game.h"
#include "GameFramework/GameModeBase.h"
#include "PAA_GameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnChangedDelegate, bool, IsPlayerTurn);

// Forward declarations per evitare dipendenze circolari
class AUnit;
class AGridGenerator;
class ASniper;
class ABrawler;

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Placement,
    InGame,
    GameOver
};


UCLASS()
class PAA_PROJECT4_API APAA_GameMode : public AGameModeBase
{
    GENERATED_BODY()

protected:
    FVector2D LastHoveredGridCoord;
    bool bHasHoveredTile = false;

public:

    void UpdateTileHighlightUnderMouse();


    void HighlightHoveredTile();


    void DebugLog(const FString& Message, FColor Color, float Duration);

    // Costruttore
    APAA_GameMode();

    UPROPERTY(BlueprintAssignable, Category = "Game Events")
    FOnTurnChangedDelegate OnTurnChanged;

protected:
    virtual void BeginPlay() override;
    void CheckIfTurnComplete();
    bool CanUnitAttackAnyEnemy(AUnit* Unit);


    TMap<AUnit*, bool> PlayerUnitsMoved;
    TMap<AUnit*, bool> PlayerUnitsAttacked;
    int32 PlayerUnitsActedCount;
    bool bReadyForNextTurn;

    


public:

    void OnMovementCompleted(AUnit* Unit);

    TArray<FVector2D> AttackHighlightedCells;
	void ResetMovementHighlights();

    TArray<AUnit*> PendingAIUnits;
    FTimerHandle AIUnitTimerHandle;
    void ProcessNextAIUnit();

	void ShowAttackRange(AUnit* Unit);

    void DebugCollision();

    virtual void Tick(float DeltaTime) override;


    // Funzione per gestire il lancio della moneta
    void CoinFlip();

    // Funzioni per le fasi successive
    UFUNCTION(BlueprintCallable)
    void NextTurn();
    void CheckWinConditions();
    void ExecuteAITurn();
    AUnit* FindNearestPlayerUnit(const FVector& Location);

    UPROPERTY(EditAnywhere, Category = "Unit Spawning")
    TSubclassOf<ASniper> SniperClass;

    UPROPERTY(EditAnywhere, Category = "Unit Spawning")
    TSubclassOf<ABrawler> BrawlerClass;

    // Posizioni predefinite per l'AI
    UPROPERTY(EditAnywhere, Category = "Unit Spawning")
    FVector2D AISniperSpawn = FVector2D(22, 22);

    UPROPERTY(EditAnywhere, Category = "Unit Spawning")
    FVector2D AIBrawlerSpawn = FVector2D(23, 22);

    // Fase di gioco
    EGamePhase CurrentPhase;

    AUnit* SelectedUnit;
    void ShowMovementRange(AUnit* Unit); 
    UPROPERTY()
    TArray<FVector2D> AllowedMovementCells;

    // Variabili per il posizionamento
    bool bIsPlayerTurn; // true se turno giocatore, false se turno AI
    bool bGameStarted;  // true se il gioco è iniziato
    int32 UnitsPlaced;  // Totale unità posizionate (max 4)
    int32 PlayerUnitIndex; // 0: Sniper, 1: Brawler
    int32 AIUnitIndex;     // 0: Sniper, 1: Brawler

    // Riferimento al GridGenerator trovato in BeginPlay
    AGridGenerator* CachedGridGenerator;

    // Array dei tipi di unità per il posizionamento
    TArray<TSubclassOf<AUnit>> PlayerUnitTypes;
    TArray<TSubclassOf<AUnit>> AIUnitTypes;

    // Funzione helper per lo spawning
    void SpawnUnit(TSubclassOf<AUnit> UnitClass, FVector2D GridPosition, AGridGenerator* GridGenerator, bool IsEnemy);


    // Gestisce il posizionamento in base al click (turno giocatore)
    void OnPlaceUnit();

    // Simula il posizionamento dell'AI (chiamato automaticamente nel Tick)
    void SimulateAIPlacement();

    void SelectUnit(AUnit* UnitToSelect);

    // Flag per evitare multipli click
    bool bClickInProgress;

    // Flag per indicare se il turno può essere skippato manualmente (tutte le unità hanno compiuto almeno un'azione)
    bool bReadyForManualSkip;

    UFUNCTION(BlueprintCallable, Category = "Game")
    bool IsPlayerTurn() const { return bIsPlayerTurn; }

    UFUNCTION(BlueprintCallable, Category = "Game")
    EGamePhase GetCurrentPhase() const { return CurrentPhase; }

    /*HUD*/

    UPROPERTY()
    UHUD_Game* HUDGame;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UHUD_Game> HUDGameClass;

    void ShowHUD();

    void ShowEnemyHUD();

	void ShowGameOverHUD(FString text);

	void ShowTurnHUD(FText text);

	void DeselectUnit();

    FString ConvertGridToChessNotation(const FVector2D& GridPosition);

    UPROPERTY()
    TArray<AUnit*> CurrentAttackableUnits;

};
