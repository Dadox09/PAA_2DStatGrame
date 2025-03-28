#include "HUD_Game.h"
#include "Unit.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

// Aggiorna tutte le healthbar delle unità del giocatore
void UHUD_Game::UpdateAllPlayerHealthBars(const TArray<AUnit*>& Units)
{
    // Controllo sicurezza sul numero massimo di unità
    if (Units.Num() > 4) return;

    // Array per gestire dinamicamente i componenti UI
    TArray<UProgressBar*> HealthBars = { HealthBar1, HealthBar2 };
    TArray<UTextBlock*> HPTexts = { HP_Text1, HP_Text2 };
    TArray<UTextBlock*> NameTexts = { UnitName_Text1, UnitName_Text2 };

    // Aggiorna ogni healthbar con i valori dell'unità corrispondente
    for (int32 i = 0; i < Units.Num(); i++)
    {
        if (HealthBars[i] && HPTexts[i] && NameTexts[i] && Units[i])
        {
            // Calcola la percentuale di vita
            float HealthPercentage = (float)Units[i]->Health / (float)Units[i]->MaxHealth;
            HealthBars[i]->SetPercent(HealthPercentage);

            // Formatta il testo HP (es. "50/100")
            HPTexts[i]->SetText(FText::FromString(
                FString::FromInt(Units[i]->Health) + "/" +
                FString::FromInt(Units[i]->MaxHealth)));

            // Imposta il nome dell'unità
            NameTexts[i]->SetText(FText::FromString(Units[i]->UnitName));

            // Mostra gli elementi UI
            HealthBars[i]->SetVisibility(ESlateVisibility::Visible);
            HPTexts[i]->SetVisibility(ESlateVisibility::Visible);
            NameTexts[i]->SetVisibility(ESlateVisibility::Visible);
        }
    }

    // Nasconde le healthbar non utilizzate
    for (int32 i = Units.Num(); i < 2; i++)
    {
        if (HealthBars[i]) HealthBars[i]->SetVisibility(ESlateVisibility::Collapsed);
        if (HPTexts[i]) HPTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
        if (NameTexts[i]) NameTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// Versione per le unità nemiche (stessa logica della funzione precedente)
void UHUD_Game::UpdateAllAIHealthBars(const TArray<AUnit*>& Units)
{
    if (Units.Num() > 4) return;

    TArray<UProgressBar*> HealthBars = { HealthBar3, HealthBar4 };
    TArray<UTextBlock*> HPTexts = { HP_Text3, HP_Text4 };
    TArray<UTextBlock*> NameTexts = { UnitName_Text3, UnitName_Text4 };

    for (int32 i = 0; i < Units.Num(); i++)
    {
        if (HealthBars[i] && HPTexts[i] && NameTexts[i] && Units[i])
        {
            float HealthPercentage = (float)Units[i]->Health / (float)Units[i]->MaxHealth;
            HealthBars[i]->SetPercent(HealthPercentage);
            HPTexts[i]->SetText(FText::FromString(
                FString::FromInt(Units[i]->Health) + "/" +
                FString::FromInt(Units[i]->MaxHealth)));
            NameTexts[i]->SetText(FText::FromString(Units[i]->UnitName));

            HealthBars[i]->SetVisibility(ESlateVisibility::Visible);
            HPTexts[i]->SetVisibility(ESlateVisibility::Visible);
            NameTexts[i]->SetVisibility(ESlateVisibility::Visible);
        }
    }

    for (int32 i = Units.Num(); i < 2; i++)
    {
        if (HealthBars[i]) HealthBars[i]->SetVisibility(ESlateVisibility::Collapsed);
        if (HPTexts[i]) HPTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
        if (NameTexts[i]) NameTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// Imposta il testo di Game Over
void UHUD_Game::SetGameOverText(FText text)
{
    if (GameOver_Text)
    {
        GameOver_Text->SetText(text);
    }
}

// Imposta il testo del turno corrente
void UHUD_Game::SetTurnText(FText text)
{
    if (Turn_Text)
    {
        Turn_Text->SetText(text);
    }
}

// Aggiunge una mossa alla cronologia
void UHUD_Game::AddMoveToHistory(const FString& UnitName, const FString& Action, const FString& Target)
{
    // Crea un nuovo record di mossa
    FMoveRecord NewMove;
    NewMove.UnitName = UnitName;
    NewMove.Action = Action;
    NewMove.Target = Target;
    NewMove.FormattedText = UnitName + Action + Target;
    NewMove.Timestamp = GetWorld()->GetTimeSeconds();

    // Aggiunge in testa alla cronologia (ultime mosse prima)
    MoveHistory.Insert(NewMove, 0);

    // Mantiene solo le ultime MaxMoveHistorySize mosse
    if (MoveHistory.Num() > MaxMoveHistorySize)
    {
        MoveHistory.RemoveAt(MaxMoveHistorySize, MoveHistory.Num() - MaxMoveHistorySize);
    }

    // Aggiorna la visualizzazione
    UpdateMoveHistoryDisplay();
}

// Aggiorna la visualizzazione della cronologia mosse
void UHUD_Game::UpdateMoveHistoryDisplay()
{
    TArray<UTextBlock*> MoveTextBlocks = { MoveText_1, MoveText_2, MoveText_3, MoveText_4, MoveText_5 };

    for (int32 i = 0; i < MoveTextBlocks.Num(); i++)
    {
        if (MoveTextBlocks[i])
        {
            if (i < MoveHistory.Num())
            {
                // Mostra la mossa se esiste
                MoveTextBlocks[i]->SetVisibility(ESlateVisibility::Visible);
                MoveTextBlocks[i]->SetText(FText::FromString(MoveHistory[i].FormattedText));
            }
            else
            {
                // Nasconde i TextBlock non utilizzati
                MoveTextBlocks[i]->SetVisibility(ESlateVisibility::Hidden);
            }
        }
    }
}

// Funzione helper per aggiungere una mossa alla cronologia
void UHUD_Game::SetExecutionText(FString from, FString Move, FString to) {
    AddMoveToHistory(from, Move, to);
}
