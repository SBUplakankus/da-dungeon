# ARCHITECTURE.md — DaDungeon

Running journal. One entry per pattern on the day it lands. 2–3 lines answering the Construction Rules before moving on. Day 7 reviews this file against shipped code — drift is the review.

> **How to use:** fill the template under each heading when you create the type. Keep it to why / owner / lifetime / boundary. No essay.

---

## 1. Component — `UStatsComponent`

**Why Component?**
- State belongs to an individual actor (Player, Enemy).
- Lifetime follows owner.
- Reusable across actor types.

**Why not Subsystem?**
- Not global. No cross-actor coordination needed.

**Construction**
- Who creates: `APlayerCharacter` / `AEnemy` via `CreateDefaultSubobject` in ctor.
- Who owns: Owning actor (outer).
- Lifetime: actor lifetime.
- Configured: C++ defaults + optional Data Table tuning.
- Needs UObject: yes — `UActorComponent` for replication/reflection/editor.

---

## 2. Component — `UProgressionComponent`

**Why Component?**
- Computation over `UStatsComponent` on same actor.
- No duplicate `Level` state — reads/writes `Stats::Level`.

**Communication shape**
- Listens to `UStatsComponent::OnStatChanged` (XP) → checks `DT_XPLevels` → `SetStat(Level, N)` → `OnLevelUp`.

**Construction**
- Owner: `APlayerCharacter`.
- Lifetime: actor.
- Boundary: pure calculation, no UI/save calls.

---

## 3. Component — `UInteractionComponent`

**Why Component?**
- Player-scoped input + trace for `IInteractable`.

**Construction**
- Owner: `APlayerCharacter`.
- Binds `IA_Interact` (Enhanced Input) → line/sphere trace → `IInteractable::Interact`.

---

## 4. Interface — `IInteractable`

**Why Interface?**
- Decouples interactor from concrete types (`BP_GoldChest`, `BP_HealthShrine`, `BP_DamageTrap`, `ADoor`).
- C++ contract: `Interact(APlayerCharacter*)`.

**UE 5.8.2**
- `UINTERFACE(MinimalAPI, BlueprintType)` + `class IInteractable` with `GENERATED_BODY()` + `UFUNCTION(BlueprintNativeEvent)`.

**Construction**
- Not owned. Implemented by `AInteractable` and any actor needing interaction.

---

## 5. Delegate — `OnStatChanged / OnDied / OnLevelUp`

**Why Delegate?**
- Observer pattern without direct dependency.
- `Stats` doesn't know HUD/Progression; `Enemy` doesn't know XP/Triggers.

**UE 5.8.2**
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatChanged, EStatType, Type, float, OldValue, float, NewValue);`
- `BlueprintAssignable` where UI needs it.

**Construction**
- Owned by broadcaster (`UStatsComponent`, `AEnemy`).
- Listeners bind in `BeginPlay` / `NativeConstruct`, unbind on destroy.

---

## 6. Factory — `FEnemyFactory` (plain C++)

**Why plain C++ (no UCLASS)?**
- Stateless service — just `Spawn` + `Configure`. No reflection, GC, or tick needed.
- Lesson: not everything must be a `UObject`.

**Construction**
- Who creates: caller (`AGameMode` / spawner) constructs on stack.
- Signature: `AEnemy* CreateEnemy(UWorld*, const UEnemyDefinition*, FTransform)`.
- Knows base class `AEnemy` — definition does not carry `TSubclassOf<AEnemy>`.

---

## 7. Strategy — `UEnemyCombatStrategy`

**Why UObject abstract?**
- Needs `UCLASS` to allow `BP_AggressiveStrategy` experiment and Data Asset reference (`TSubclassOf<UEnemyCombatStrategy>` in definition).
- Two concrete C++: `UAggressiveStrategy`, `UDefensiveStrategy`.

**UE 5.8.2**
- `UCLASS(Abstract, Blueprintable, BlueprintType)` + `UFUNCTION(BlueprintNativeEvent)` decision method.
- Instance created via `NewObject<Strategy>(Enemy)` — outer is `AEnemy`, GC-safe. Owned by `UEnemyCombatComponent` (`UPROPERTY()`).

**Why not Strategy as enum?**
- Swapping behavior requires new class, not `switch`.

---

## 8. Data Asset — `UEnemyDefinition : UPrimaryDataAsset`

**Why Data Asset?**
- Answers "what is this enemy" — weapon/armour refs, strategy class, stat row, visuals.
- Content designers tune without touching C++.

**Boundary**
- No runtime behavior. Factory interprets it.

**Construction**
- Created in editor (`Content/Data/DA_RangedWarrior`). Referenced by spawner. `UPrimaryDataAsset` for Asset Manager if needed.

---

## 9. Data Asset — `UWeaponDefinition / UArmorDefinition / UEnemyVisualDefinition`

Same rationale as above. C++ defines fields (`Damage`, `Range`, `Defence`, `Mesh`, `AnimClass`), Data picks assets.

---

## 10. Data Table — `DT_XPLevels`

**Why Data Table over Data Asset?**
- Homogeneous tabular data — one row per level. Cheaper than N assets.

**UE 5.8.2**
- `USTRUCT(BlueprintType) : public FTableRowBase { RequiredXP }`.
- `UDataTable*` on `UProgressionComponent` or `UGameInstance` — loaded via `ConstructorHelpers` or `UPROPERTY(EditDefaultsOnly)`.

**Rows**
| Level | RequiredXP |
|------:|-----------:|
| 1 | 0 |
| 2 | 100 |
| 3 | 250 |
| 4 | 500 |
| 5 | 900 |

---

## 11. Subsystem — `USaveSubsystem : UGameInstanceSubsystem`

**Why GameInstanceSubsystem?**
- Lifetime = game instance (global, survives map travel). Exactly matches save scope.

**UE 5.8.2**
- `UCLASS() : public UGameInstanceSubsystem` — `Initialize`/`Deinitialize` lifecycle. Access via `GetGameInstance()->GetSubsystem<USaveSubsystem>()`.

**Construction**
- No actor owner. Serializes `Level/Stats/Gold` via `USaveGame` + `UGameplayStatics::SaveGameToSlot`.

---

## 12. Subsystem — `UGameUISubsystem : ULocalPlayerSubsystem`

**Why LocalPlayerSubsystem?**
- UI is per-player, not global. Correct scoping for splitscreen/local player.

**UE 5.8.2**
- `UCLASS() : public ULocalPlayerSubsystem` — `Initialize(ULocalPlayer*)` gives player context.

**Owns**
- `UCommonUILayout`-style root with 4 layers: HUD / Menu / Modal / Notification. Menu layer owns `CommonActivatableWidget` stack navigation.

---

## 13. ViewModel — `UPlayerViewModel`

**Why ViewModel?**
- Presentation adapter between gameplay (`UStatsComponent`) and widgets. Widgets never touch gameplay directly.

**UE 5.8.2**
- `UMG Viewmodel` plugin (`ModelViewViewModel.uplugin`, Beta in 5.8) — `UViewModel` or `UMVVMViewModelBase`. Expose `UFUNCTION` + `FieldNotify` for bindings. Created by `UGameUISubsystem`, injected into `UUserWidget` via `SetViewModel`.

**Boundary**
- No combat/save logic. Only forwards `OnStatChanged` → `NotifyFieldValueChanged`.

---

## 14. Object Ownership Summary

| Object | Created by | Outer / Owner | Lifetime |
|--------|------------|---------------|----------|
| `APlayerCharacter` | `GameMode` spawn | `UWorld` | Level |
| `UStatsComponent` | `APlayerCharacter` ctor | Player | Player |
| `AEnemy` | `FEnemyFactory` | `UWorld` | Until death/destroy |
| `UEnemyCombatStrategy` instance | Factory `NewObject` | `AEnemy` | Enemy |
| `UTrigger` / Conditions | Placed actor or `ADoor` subobject | Door / World | Door/Level |
| `USaveSubsystem` | Engine | `GameInstance` | App |
| `UGameUISubsystem` | Engine | `LocalPlayer` | Player session |

---

## 15. C++ / Data / Blueprint Boundary (enforced)

- **C++:** actors, components, interfaces, delegates, subsystems, factory, strategies, viewmodels.
- **Data:** `DA_*`, `DT_XPLevels` — tuning only.
- **Blueprint:** `BP_PlayerCharacter`, `BP_Enemy`, `BP_GoldChest/Shrine/Trap`, `WBP_*` layouts, anim BPs. Minimal logic — mostly asset assignment.

---

## 16. Communication Rules

```
Direct ref → Interface → Delegate → Subsystem
```

No global bus. No `Cast<>` chains. No widget → gameplay writes. Gameplay → `OnChanged` → ViewModel → widget.

---

## Day 7 Checklist (use this)

- [ ] Each entry above still matches shipped code
- [ ] No `UGameInstanceSubsystem` service-locator creep
- [ ] No widget reaching into `APlayerCharacter` directly
- [ ] Adding new enemy = new `DA_*` only
- [ ] Adding new interactable = new `BP_*` with `IInteractable` only
- [ ] Adding new condition = new `UTriggerCondition` subclass only
