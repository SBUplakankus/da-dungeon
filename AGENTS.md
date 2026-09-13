# AGENTS.md — DaDungeon (UE 5.8)

> Root layout: `README.md` (overview) + `AGENTS.md` (this file) + `Docs/` (`ARCHITECTURE.md`, `IMPLEMENTATION_PLAN.md`, `UE5_Systems_Sandbox_Handoff.md`) + `DaDungeon/` (engine project). Keep root tidy — new prose docs go in `Docs/`, never loose at root.
> License: CC BY-NC-ND 4.0 (see `LICENSE`). Applies to original code/docs only — tutorial/course-derived code stays owned by its authors; do not strip license notices or relicense third-party code.

## Project layout — non-obvious
- Engine project lives in `DaDungeon/` subfolder, not repo root: `DaDungeon/DaDungeon.uproject:1` (`EngineAssociation: 5.8`). All `**/Binaries/`, `**/Intermediate/`, `**/Saved/`, `**/DerivedDataCache/` patterns in `.gitignore:4` match both root and subfolder — do not commit them.
- Solution files are `DaDungeon/DaDungeon.sln` + `DaDungeon.slnx` — open from inside `DaDungeon/`, not root.
- Source module is single: `DaDungeon/Source/DaDungeon/DaDungeon.Build.cs:5` (`DaDungeon` Runtime). New code goes `Source/DaDungeon/Public/` + `Source/DaDungeon/Private/` (see `Public/Subsystems/FrontendUISubsystem.h:25`, `Public/PlayerCharacter.h:17`). `Variant_Horror/` and `Variant_Shooter/` under `Source/DaDungeon/` are template cruft — `Docs/IMPLEMENTATION_PLAN.md:14` says delete/ignore; do not extend them.
- Content/config: `DaDungeon/Content/`, `DaDungeon/Config/DefaultEngine.ini:16` (still points `GlobalDefaultGameMode` at `BP_FirstPersonGameMode` — update to `ADaDungeonGameMode` per plan Day 1), `DaDungeon/Config/DefaultGame.ini:5`.

## Build & run — no npm/test harness
- No automated tests, lint, formatter, or CI in repo. Verification is PIE in Unreal Editor.
- Build: open `DaDungeon/DaDungeon.sln(x)` in Visual Studio/Rider and build `DaDungeonEditor Win64 Development`, or `UnrealBuildTool DaDungeonEditor Win64 Development -Project="DaDungeon/DaDungeon.uproject"`. Package not required (`Docs/IMPLEMENTATION_PLAN.md:112`).
- Editor startup map: `DaDungeon/Config/DefaultEngine.ini:7` `EditorStartupMap=/Game/DaDungeon/Levels/MainMenuLvl`.

## Module & plugin dependencies — must fix before touching UI/MVVM
- Enabled plugins `DaDungeon/DaDungeon.uproject:19-46`: `CommonUI`, `StateTree`, `GameplayStateTree`, `MVVMToolset`, `ModelViewViewModelPreview` (Beta name for `ModelViewViewModel` in `Docs/IMPLEMENTATION_PLAN.md:8`), `FlatNodes`, `ModelingToolsEditorMode` (Editor only).
- `DaDungeon/Source/DaDungeon/DaDungeon.Build.cs:11` currently lists `Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, StateTreeModule, GameplayStateTreeModule, GameplayTags, UMG, Slate` — **missing** `CommonUI`, `CommonInput`, `ModelViewViewModel` that `Docs/IMPLEMENTATION_PLAN.md:13` requires. Adding any `CommonUI`/`MVVM` include without updating `Build.cs` will fail to compile — add `CommonUI`, `CommonInput`, `ModelViewViewModel` there first.
- `DaDungeon.uproject:12` `AdditionalDependencies` also omits `CommonUI` — keep in sync with `Build.cs`.
- Public include hack `DaDungeon.Build.cs:27` adds `Variant_*` folders to `PublicIncludePaths` — remove when deleting variants.

## Architecture — read before coding
- `ARCHITECTURE.md:1` is the running construction journal (who creates/owns/lifetime per type). Day 7 reviews it vs shipped code — update it the day you add a type (Component/Interface/Factory/Strategy/DataAsset/DataTable/Subsystem/ViewModel/Delegate).
- `Docs/UE5_Systems_Sandbox_Handoff.md:1` + `Docs/IMPLEMENTATION_PLAN.md:1` are the full spec and 7-day schedule. Trust them over stale comments, but trust executable config (`.uproject`, `.Build.cs`, `.ini`) over prose when they conflict.
- Enforced boundary `ARCHITECTURE.md:204`:
  - **C++**: actors, components, interfaces, delegates, subsystems, factory, strategies, viewmodels.
  - **Data**: `DA_*` (`UPrimaryDataAsset`), `DT_XPLevels` (`FTableRowBase`) — tuning only, no behavior.
  - **Blueprint**: `BP_PlayerCharacter`, `BP_Enemy`, `BP_GoldChest/Shrine/Trap`, `WBP_*`, anim BPs — asset assignment/layout only, minimal logic.
- Communication order `ARCHITECTURE.md:216`: `Direct ref -> Interface (IInteractable) -> Delegate (OnStatChanged/OnDied/OnLevelUp) -> Subsystem`. No global bus, no `Cast<>` chains, no widget->gameplay writes. `Stats -> OnChanged -> ViewModel -> widget`.

## Key types & gotchas
- **Player**: `APlayerCharacter : ADaDungeonCharacter` owns `UStatsComponent`, `UProgressionComponent`, `UInteractionComponent` via `CreateDefaultSubobject` in ctor. C++ is abstract base (`DaDungeonCharacter.h:21` `UCLASS(abstract)`); `BP_PlayerCharacter` is the concrete child. `Source/DaDungeon/Public/PlayerCharacter.h:17` is currently a stub — flesh it out per plan.
- **Stats**: `EStatType {Health,Mana,Gold,XP,Level}` + `TMap<EStatType,float>` in `UStatsComponent`; `GetStat/SetStat/ModifyStat`; `DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatChanged, ...)` `BlueprintAssignable`. No `if Gold` branching in interactables — generic `ModifyStat`.
- **IInteractable**: `UINTERFACE(MinimalAPI, BlueprintType)` + `UFUNCTION(BlueprintNativeEvent) Interact(APlayerCharacter*)` (`ARCHITECTURE.md:62`). Implemented by `AInteractable` (owns `UStaticMeshComponent` + `UBoxComponent` + `StatType/Amount/Prompt`) and any door/actor. `UInteractionComponent` binds `IA_Interact` (new action, `IMC_Default`, key `E`) -> trace -> `IInteractable::Interact`.
- **Enemy**: `AEnemy : ACharacter` + `UStatsComponent`/`UEnemyLoadoutComponent`/`UEnemyCombatComponent` + `OnDied`. Definitions are `UEnemyDefinition : UPrimaryDataAsset` (and `UWeaponDefinition`/`UArmorDefinition`/`UEnemyVisualDefinition`) — factory `FEnemyFactory` (plain C++ struct, no `UCLASS`, `ARCHITECTURE.md:85`) knows `AEnemy` base; definition does NOT carry `TSubclassOf<AEnemy>`. Strategy is `UEnemyCombatStrategy : UObject` `UCLASS(Abstract, Blueprintable, BlueprintType)` with `BlueprintNativeEvent DecideAction`; instances via `NewObject<Strategy>(Enemy)` outer=`AEnemy` owned by `UEnemyCombatComponent` (`UPROPERTY()`). Two C++ concretes `UAggressiveStrategy`/`UDefensiveStrategy`; optional isolated `BP_AggressiveStrategy : UAggressiveStrategy` is not wired to any `DA_*`.
- **Trigger**: `UTrigger` + `TArray<UTriggerCondition*>` (`UEnemyKilledCondition` counts `OnDied`, `UHasStatCondition` reads `GetStat` on re-eval) -> `ADoor::OnTriggered -> Open`. Event-driven re-eval, no tick (`Docs/UE5_Systems_Sandbox_Handoff.md:281`).
- **Progression**: `Level` lives only in `UStatsComponent`. `UProgressionComponent` listens `OnStatChanged(XP)` -> `DT_XPLevels` rows `0/100/250/500/900` (`ARCHITECTURE.md:142`) -> `SetStat(Level,N)` -> `OnLevelUp`. No duplicate `Level` field.
- **Save**: `USaveSubsystem : UGameInstanceSubsystem` (`Initialize`/`Deinitialize`, `GetSubsystem<USaveSubsystem>()`, `USaveGame` + `SaveGameToSlot`). Do not make everything a `GameInstanceSubsystem` service locator (`ARCHITECTURE.md:200`).
- **UI**: `UFrontendUISubsystem : UGameInstanceSubsystem` (`Public/Subsystems/FrontendUISubsystem.h:25`) — diverges from handoff spec `ULocalPlayerSubsystem`. Uses `UWidget_PrimaryLayout : UCommonUserWidget` (`Public/Widgets/Widget_PrimaryLayout.h:15`) with 4 `UCommonActivatableWidgetContainerBase` stacks keyed by `FrontendGameplayTags` (`Public/FrontendGameplayTags.h:10` `Frontend_WidgetStack_Modal/GameMenu/GameHud/Frontend`). Navigation via `RegisterWidgetStack`/`FindWidgetStackByTag` + async `PushSoftWidgetToStackAsync` (`Private/Subsystems/FrontendUISubsystem.cpp:44` via `AssetManager` `RequestAsyncLoad`). ViewModel `UPlayerViewModel : UMVVMViewModelBase` (plugin `ModelViewViewModel`) with `FieldNotify`, created by UI subsystem, injected via `SetViewModel` — never let widgets reference `APlayerCharacter` directly.

## Conventions to preserve
- Update `ARCHITECTURE.md` 2-3 lines per new type before moving on; check `ARCHITECTURE.md:224` Day 7 checklist (no `Cast<>` chains, additive `DA_*`/`BP_*`/`UTriggerCondition` creation without code changes).
- Keep C++/Data/BP separation strict — content designers tune `DA_*`/`DT_*` without touching C++.
