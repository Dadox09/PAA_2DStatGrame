#pragma once

#include "CoreMinimal.h"
#include "HUD_Game.h"
#include "GameFramework/Actor.h"
#include "Unit.generated.h"

class AGridGenerator;

UCLASS()
class PAA_PROJECT4_API AUnit : public APawn
{
    GENERATED_BODY()

public:
    AUnit();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    // Variabili per il movimento fluido
    FVector TargetWorldLocation;
    FVector2D TargetGridPosition;
    float MovementSpeed = 400.0f; 

public:

    bool bIsMoving = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    bool IsEnemy;

    // Ottieni la posizione corrente nella griglia
    UFUNCTION(BlueprintCallable, Category = "Unit")
    FVector2D GetGridPosition() const;

    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool MoveTo(FVector2D NewPosition);

	void Attack(AUnit* Target);

    // Verifica se una cella è valida per il movimento
    UFUNCTION(BlueprintCallable, Category = "Unit")
    bool IsCellValid(FVector2D CellPosition) const;

    // Imposta il generatore di griglia
    void SetGridGenerator(AGridGenerator* GridGenerator);

    bool IsAlive() const;

    // Proprietà dell'unità
    
    int32 UnitType;

    UPROPERTY()
    UHUD_Game* HUDGame;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FString UnitName; // Nome dell'unità

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MovementRange; // Quante celle può muoversi l'unità

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 AttackRange; // Range di attacco

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 Health; // Punti vita

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MaxHealth; // Max Punti vita

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MinDamage; // Danno minimo

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MaxDamage; // Danno massimo

    // Componente mesh dell'unità
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;


private:
    // Riferimento al generatore di griglia
    AGridGenerator* GridGenerator;

    TArray<FVector> PathWaypoints;
};