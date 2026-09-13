# IMPLEMENTATION_PLAN.md — DaDungeon (UE 5.8, C++-first)

Small dungeon, starter map `Lvl_FirstPerson` as blockout. No new level art.

## 0. Pre-reqs

- Enable plugins in `DaDungeon.uproject`:
  ```json
  { "Name": "CommonUI", "Enabled": true },
  { "Name": "ModelViewViewModel", "Enabled": true }
  ```
  Both ship with 5.8 but `EnabledByDefault: false` (`Engine/Plugins/Runtime/CommonUI/CommonUI.uplugin`, `Engine/Plugins/Runtime/ModelViewViewModel/ModelViewViewModel.uplugin` — Beta).
- `DaDungeon.Build.cs`: add `CommonUI`, `CommonInput`, `ModelViewViewModel`, `UMG`.
- Delete or ignore `Source/DaDungeon/Variant_*` — template cruft.
- Folder layout:
  ```
  Source/DaDungeon/
    Player/  Stats/  Interaction/  Enemy/  Trigger/  Save/  UI/
  Content/Data/  Content/UI/
  ```

## 1. Day 1 — Foundations

**Goal:** `APlayerCharacter : ADaDungeonCharacter` with 3 components, BP child.

- Create `APlayerCharacter` (C++) owning `UStatsComponent`, `UProgressionComponent`, `UInteractionComponent` via `CreateDefaultSubobject`.
- Expose `UPROPERTY(EditAnywhere)` only for mesh/camera tuning — keep logic in C++.
- Create `BP_PlayerCharacter` child for mesh/camera.
- Update `DefaultEngine.ini:16` `GlobalDefaultGameMode` to new `ADaDungeonGameMode` spawning `APlayerCharacter`.
- Verify Enhanced Input still routes (`IA_Move`, `IA_Look`, `IA_Jump`).

**Done when:** Play in `Lvl_FirstPerson` moves as before.

## 2. Day 2 — Stats + Interaction

**Stats**
- `EStatType { Health, Mana, Gold, XP, Level }` + `TMap<EStatType,float>` in `UStatsComponent`.
- API: `GetStat`, `SetStat`, `ModifyStat(Amount)`. `DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatChanged, EStatType, Type, float, Old, float, New)`.
- `UPROPERTY(BlueprintAssignable)`.

**Interactable**
- `UINTERFACE(MinimalAPI, BlueprintType)` / `IInteractable` with `BlueprintNativeEvent void Interact(APlayerCharacter*)`.
- `AInteractable : AActor, IInteractable` — `UStaticMeshComponent`, `UBoxComponent` overlap, `UPROPERTY(EditAnywhere) EStatType StatType; float Amount; FText Prompt`.
- `UInteractionComponent` — trace/overlap query for `IInteractable`, bind `IA_Interact` (new action + `IMC_Default` mapping, key `E`).
- 3 BPs: `BP_GoldChest (+50 Gold)`, `BP_HealthShrine (+25 Health)`, `BP_DamageTrap (-20 Health)` — only set mesh + StatType/Amount.

**Done when:** `BP_HealthShrine → Interact → Health +25 → OnStatChanged → log` no `if Gold` branching.

## 3. Day 3 — Enemy Architecture

- `UPrimaryDataAsset` defs: `UEnemyDefinition`, `UWeaponDefinition`, `UArmorDefinition`, `UEnemyVisualDefinition` (mesh, `TSubclassOf<UAnimInstance>`, montages).
- Row: `DA_Bow`, `DA_LightArmour`, `DA_WarriorVisuals`, `DA_KnightVisuals`, `DA_RangedWarrior` / `DA_DefensiveKnight` (both Bow+LightArmour, differ only by `TSubclassOf<UEnemyCombatStrategy>`).
- `AEnemy : ACharacter` with `UStatsComponent`, `UEnemyLoadoutComponent`, `UEnemyCombatComponent` + `OnDied` delegate.
- `UEnemyCombatStrategy : UObject` `UCLASS(Abstract, Blueprintable, BlueprintType)` with `BlueprintNativeEvent DecideAction` — `UAggressiveStrategy`, `UDefensiveStrategy` (C++).
- `FEnemyFactory` — plain struct in `Enemy/Factory/`:
  ```cpp
  struct FEnemyFactory {
    static AEnemy* CreateEnemy(UWorld*, const UEnemyDefinition*, FTransform);
  };
  ```
  Spawns `AEnemy`, applies stats/loadout, `NewObject<Strategy>(Enemy)` stored in `UEnemyCombatComponent`.
- `BP_Enemy` child for mesh/anim only.
- Optional isolated: `BP_AggressiveStrategy : UAggressiveStrategy`.

**Done when:** `DA_RangedWarrior → Factory → BP_Enemy + Aggressive` and swapping strategy field changes behavior with no code change.

## 4. Day 4 — Events, Triggers, Progression

- `AEnemy::OnDied` → listeners: `UProgressionComponent` (if needed), `UTrigger`.
- `UTrigger : UObject` or `AActor` with `TArray<UTriggerCondition*> Conditions` + `OnTriggered`. `UTriggerCondition : UObject` with `IsSatisfied()`.
  - `UEnemyKilledCondition` — counts `OnDied` events.
  - `UHasStatCondition` — reads `UStatsComponent::GetStat` on each re-eval (no tick).
  - Door: `ADoor` owns `UTrigger`; `OnTriggered → Open`.
  - Example: `Kill 3 + Have 50 Gold → Open`.
- `UProgressionComponent` — binds `OnStatChanged` XP, reads `DT_XPLevels` (`FTableRowBase` row `FXPLevelRow { int32 RequiredXP }`), calls `SetStat(Level, N)` → `OnLevelUp`.
- Create `DT_XPLevels` with 5 rows (0/100/250/500/900).

**Done when:** kills + gold open door; XP gain → level up without polling.

## 5. Day 5 — CommonUI + MVVM (highest risk)

- `UGameUISubsystem : ULocalPlayerSubsystem` — `Initialize(ULocalPlayer*)` creates `UCommonUILayout` root.
- Root with 4 layers: `HUD` (persistent), `Menu` (`CommonActivatableWidgetStack`), `Modal` (blocking), `Notification` (transient queue).
- `UPlayerViewModel : UMVVMViewModelBase` (ModelViewViewModel plugin) — `FieldNotify` props for Health/Mana/Gold/XP/Level, binds `OnStatChanged`/`OnLevelUp`.
- Widgets:
  - `WBP_HUD` (CommonUI + MVVM binding, no direct player ref)
  - `WBP_Pause`, `WBP_Stats` (menu stack push/pop)
  - `WBP_Confirm`, `WBP_LevelUp` (modal)
  - `WBP_Notification` (queue: +50 Gold, Level Up, Door Unlocked)
- Input: CommonUI input routing — modal takes priority, pause suspends game input.

**Done when:** HUD reflects stats via ViewModel, pause → stats → back, level-up modal blocks, notifications queue.

## 6. Day 6 — Save + Buffer

- `USaveSubsystem : UGameInstanceSubsystem` + `UDaDungeonSaveGame : USaveGame` (`Level`, `TMap<EStatType,float>` or individual `UPROPERTY(SaveGame)` fields).
- API: `Save()`, `Load()` via `UGameplayStatics::SaveGameToSlot/LoadGameFromSlot`.
- Restore: on `Load`, apply to `UStatsComponent` on spawned player.
- Use remaining time as buffer for Day 5 overflow.

**Done when:** quit → relaunch → stats/level/gold restored.

## 7. Day 7 — Review (no features)

- Reread `ARCHITECTURE.md` vs shipped code. Fix drift: remove casts, tighten ownership, ensure BP/Data boundaries.
- Test additive creation: new enemy (`DA_*`), new interactable (`BP_*` + `IInteractable`), new condition, new UI screen — each should be ≤ few lines of C++ or data-only.

## 8. Verification

- Build: `UnrealBuildTool` Development Editor.
- Play: interact chain, enemy kill → XP → level → notification, trigger door, pause/menu/modal layers, save/load.
- Packaging not required — editor PIE is sufficient for hand-off.

## 9. Non-goals

Audio, VFX, complex AI, dialogue, crafting, multiplayer, GAS, World Partition, inventory, achievements.
