#pragma once


#include "CoreMinimal.h"  
#include "Blueprint/UserWidget.h"  
#include "HUD_Game.generated.h"  

class AUnit;

USTRUCT(BlueprintType)
struct FMoveRecord
{
GENERATED_BODY()

UPROPERTY(BlueprintReadWrite, Category = "Move")
FString UnitName;

UPROPERTY(BlueprintReadWrite, Category = "Move")
FString Action;

UPROPERTY(BlueprintReadWrite, Category = "Move")
FString Target;

UPROPERTY(BlueprintReadWrite, Category = "Move")
FString FormattedText;

UPROPERTY(BlueprintReadWrite, Category = "Move")
float Timestamp;
};

UCLASS()
class PAA_PROJECT4_API UHUD_Game : public UUserWidget
{
GENERATED_BODY()

public:

void UpdateAllPlayerHealthBars(const TArray<AUnit*>& Units);

void UpdateAllAIHealthBars(const TArray<AUnit*>& Units);

void SetTurnText(FText text);

void SetGameOverText(FText text);

void SetExecutionText(FString from, FString to, FString Move);

UFUNCTION(BlueprintCallable, Category = "Game HUD")
void AddMoveToHistory(const FString& UnitName, const FString& Action, const FString& Target);

// Funzione per aggiornare la visualizzazione dello storico mosse
UFUNCTION(BlueprintCallable, Category = "Game HUD")
void UpdateMoveHistoryDisplay();

UPROPERTY(EditAnywhere, meta = (BindWidget))
class UProgressBar* HealthBar1;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UProgressBar* HealthBar2;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UProgressBar* HealthBar3;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UProgressBar* HealthBar4;

UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* HP_Text1;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* HP_Text2;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* HP_Text3;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* HP_Text4;

UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* UnitName_Text1;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* UnitName_Text2;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* UnitName_Text3;
UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* UnitName_Text4;


UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* GameOver_Text;

UPROPERTY(EditAnywhere, meta = (BindWidget))
class UTextBlock* Turn_Text;	

UPROPERTY()
TArray<FMoveRecord> MoveHistory;

// Numero massimo di mosse da mantenere nello storico
UPROPERTY(EditAnywhere, Category = "Game HUD")
int32 MaxMoveHistorySize = 5;

// Riferimenti ai widget di testo per le ultime 5 mosse
UPROPERTY(meta = (BindWidget))
class UTextBlock* MoveText_1;

UPROPERTY(meta = (BindWidget))
class UTextBlock* MoveText_2;

UPROPERTY(meta = (BindWidget))
class UTextBlock* MoveText_3;

UPROPERTY(meta = (BindWidget))
class UTextBlock* MoveText_4;

UPROPERTY(meta = (BindWidget))
class UTextBlock* MoveText_5; 

};