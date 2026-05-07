# 2D Strategic Game

A turn-based strategy game built with **Unreal Engine 5** in C++, developed as a university project for the Advanced Algorithms Programming course (PAA).

## Scope

Two factions — Player and AI — compete on a procedurally generated grid. Players place units, then alternate turns moving and attacking until one side is eliminated.

**Core loop:**
1. **Placement phase** — Player and AI place units on the grid; turn order decided by coin flip.
2. **In-game phase** — Each turn, all units can move and/or attack; turn ends when all actions are consumed or manually skipped.
3. **Game over** — Win condition checked after every action.

## Architecture

| Component | Role |
|---|---|
| `AGridGenerator` | Builds the NxN tile grid, spawns obstacles procedurally, exposes A* pathfinding |
| `AUnit` (base) | Pawn with grid position, HP, move/attack range, smooth interpolated movement |
| `ASniper` / `ABrawler` | Concrete unit types with distinct stats |
| `APAA_GameMode` | Central game controller — phase FSM (`Placement → InGame → GameOver`), turn management, AI execution, HUD wiring |
| `UHUD_Game` | UMG widget layer for unit info, turn display, game over screen |

**Key algorithms:**
- A* pathfinding with obstacle-aware grid nodes (`FGridNode`)
- DFS connectivity check ensures obstacle placement never splits the grid
- Randomized damage rolls per attack (`MinDamage`–`MaxDamage`)
- Rule-based AI: finds nearest player unit, moves toward it, attacks if in range

## Stack

- Unreal Engine 5 — C++
- UMG (Unreal Motion Graphics) for UI
- Instanced Static Meshes for grid rendering

---

## Author

**Davide Rizzo** — Computer Engineer, MSc candidate in AI & Human-Centered Computing @ Università di Genova.

[Portfolio](https://daviderizzo.dev) · [GitHub](https://github.com/Dadox09) · [LinkedIn](https://linkedin.com/in/davide-rizzo-it)
