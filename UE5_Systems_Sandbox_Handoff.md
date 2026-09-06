# UE5 C++ Systems Sandbox — One-Week Handoff (Trimmed)

## Goal

Build a tiny 5–10 minute dungeon whose gameplay is intentionally simple.

The purpose of the project is not gameplay.

The purpose is to relearn Unreal Engine through a C++-first architecture and specifically experiment with:

- C++ → Blueprint inheritance
- Components
- Interfaces
- Delegates/events
- Factories
- Strategy pattern
- Data Assets
- Data Tables
- Subsystems
- CommonUI
- MVVM
- UI layering
- Object ownership/lifetime
- Runtime construction
- C++/Blueprint/Data boundaries

**Cut from the original scope:** Inventory system, Achievement system. Both taught the same Data → Component/System → Event lesson already covered elsewhere (Stats, Progression). Removing them removes duplicate work, not duplicate learning.

## Core Philosophy

**C++**
- Architecture, systems, rules/contracts, runtime objects, construction, communication

**Data Assets / Data Tables**
- Content definitions, configuration, tuning, references to assets

**Blueprint**
- Visual/content configuration, asset assignment, widget layouts, animation blueprints, concrete subclasses where useful

Blueprint should contain minimal gameplay logic.

---

## 1. The Game

A tiny dungeon consisting of a few sections. The player:

1. Walks around using default Unreal movement.
2. Interacts with objects.
3. Gains/loses stats.
4. Encounters a few enemies.
5. Satisfies generic conditions.
6. Opens doors into subsequent sections.
7. Eventually completes/extracts.

There is no concern for polished gameplay.

**Explicitly out of scope:** Audio, VFX, complex combat, complex AI, dialogue, crafting, multiplayer, networking, World Partition, advanced animation systems, Gameplay Ability System, inventory, achievements.

The goal is systems construction and communication.

---

## 2. High-Level Architecture

```
                         GAME INSTANCE
                              │
                        Save Subsystem


                         LOCAL PLAYER
                              │
                       UI Subsystem
                              │
                 ┌────────────┼────────────┐
                 │            │            │
                HUD         Menus        Modals
                 │                         │
                 │                    Notifications
                 │
                 ▼
               MVVM
                 │
                 ▼
              CommonUI


                           WORLD
                             │
              ┌──────────────┼──────────────┐
              │              │              │
          GameMode       GameState        Player
                                             │
              ┌──────────────┼──────────────┐
              │              │              │
            Stats       Progression    Interaction
          Component      Component      Component
                                             │
                                             ▼
                                       Interactable
                                             │
                                          Events
                                             │
              ┌──────────────────────────────┴─────────────┐
              │                                            │
      (feeds Progression)                              Triggers


                          ENEMIES
                             │
                       Enemy Factory
                             │
                       Enemy Definition
                             │
                             ▼
                           Enemy
                             │
              ┌──────────────┼──────────────┐
              │              │              │
           Stats          Loadout       Combat Strategy
          Component       Component       UObject
```

---

## 3. Player

Create a C++ player character:

```
APlayerCharacter
    ↓
BP_PlayerCharacter
```

The C++ class owns the important runtime architecture.

Components:

```
APlayerCharacter
├── UStatsComponent
├── UProgressionComponent
└── UInteractionComponent
```

Blueprint handles primarily mesh, camera configuration if necessary, and visual/editor configuration. Use Unreal's default movement/input as much as possible.

---

## 4. Stats System

Create one generic stats component.

**UStatsComponent** — Stats for the project: Health, Mana, Gold, XP, Level.

Do not create five unrelated systems. Keep the stat type as simple as possible — an enum and an internal map/array is enough:

```cpp
enum class EStatType
{
    Health,
    Mana,
    Gold,
    XP,
    Level
};
```

You're learning construction, not designing the perfect RPG stat framework. The component should expose a generic API conceptually like:

```
GetStat(StatType)
SetStat(StatType, Value)
ModifyStat(StatType, Amount)
```

`Amount > 0` → increase, `Amount < 0` → decrease.

Stat changes should generate an event/delegate:

```
StatsComponent
    │
    └── OnStatChanged
```

Other systems can observe this without the stats component knowing about them.

---

## 5. Interactable System

Create a C++ base interactable:

```
AInteractable
```

Three concrete examples — one positive, one positive-alt, one negative — enough to prove the pattern without redundant content:

```
BP_GoldChest     → Stat = Gold,   Amount = +50
BP_HealthShrine  → Stat = Health, Amount = +25
BP_DamageTrap    → Stat = Health, Amount = -20
```

The base class owns the interaction architecture. The interactable should **not** contain special-case logic such as `if Gold`, `if Health`. It requests a generic stat modification from the stat system.

**Blueprint properties:** The C++ base class exposes properties allowing Blueprint/Data configuration of visual asset/mesh, interaction information, stat type, amount, and any simple presentation values. Blueprint concrete classes should mostly configure appearance.

---

## 6. Interaction Interface

Create an Unreal interface representing something that can be interacted with.

```
IInteractable
    Interact(Player)
```

The player interaction system should interact through the interface rather than knowing concrete types:

```
Player
   │
   ▼
"I found something interactable"
   │
   ▼
IInteractable
```

Rather than `if Chest`, `if Shrine`, `if Trap`, `if Door`. This is primarily an exercise in communication through contracts.

---

## 7. Trigger / Condition System

Create a generic trigger system.

```
UTrigger
    └── Conditions[]
```

A trigger is satisfied when all configured conditions are satisfied.

**Example — Door Trigger:**

```
Conditions:
    Kill 3 enemies    ✓   (EnemyKilledCondition)
    Have 50 gold      ✓   (HasStatCondition)

All satisfied → Door opens
```

Two condition types implemented, enough to demonstrate the architecture:

```
EnemyKilledCondition
HasStatCondition
```

The door itself should not know how individual conditions work.

```
Gameplay events
      ↓
Conditions
      ↓
Trigger
      ↓
Door
```

Prefer event-driven communication over unnecessary polling. Note: `HasStatCondition` is a standing predicate rather than a one-shot event — on any relevant gameplay event, the trigger re-evaluates all its conditions' current state rather than waiting for a dedicated "condition satisfied" signal.

---

## 8. Enemy System

This is the main design-pattern exercise.

```
AEnemy
├── UStatsComponent
├── UEnemyLoadoutComponent
└── UEnemyCombatComponent
```

The enemy itself should not become a giant class.

---

## 9. Enemy Definitions

Create:

```
UEnemyDefinition : UPrimaryDataAsset
```

It describes an enemy. With only one weapon and one armor definition in scope, the two example enemies differ by **strategy only** — this is deliberately the cleanest possible demonstration that swapping a single data field changes enemy behavior without touching code.

**Important:** the Data Asset does not reference `BP_Enemy`. The factory already knows the base class it spawns (`AEnemy`) — having the definition also carry an Enemy Class field would mean you're testing `TSubclassOf<AEnemy>` resolution as much as the factory pattern itself, for no real benefit at this stage. `AEnemy → BP_Enemy` still exists, but purely for mesh/animation configuration, same role Blueprint plays everywhere else in this project.

```
DA_RangedWarrior
    Weapon:           DA_Bow
    Armour:           DA_LightArmour
    Combat Strategy:  UAggressiveStrategy
    Stats:            WarriorStats
    Visual Definition: WarriorVisuals

DA_DefensiveKnight
    Weapon:           DA_Bow
    Armour:           DA_LightArmour
    Combat Strategy:  UDefensiveStrategy
    Stats:            KnightStats
    Visual Definition: KnightVisuals
```

The Data Asset answers *"What is this enemy?"* It should not contain complicated runtime behaviour.

---

## 10. Enemy Factory

Create a C++ factory:

```
FEnemyFactory
```

Its job: construct/configure an `AEnemy` from a `UEnemyDefinition`. The factory itself knows the base class it spawns — it doesn't ask the definition what to spawn.

```
EnemyDefinition
      │
      ▼
EnemyFactory
      │
      ├── Spawn AEnemy
      ├── Apply stats
      ├── Configure loadout
      ├── Create strategy
      └── Apply visual definition
```

The factory is C++. Do not create Blueprint factories for this exercise.

**Make this your one deliberate plain C++ class.** Everything else in this project is an Unreal type (`UObject`, `AActor`, `UActorComponent`, `UDataAsset`, a Subsystem, an Interface, a Delegate). `FEnemyFactory` doesn't need to be any of those — it just needs to be callable. Building it as an ordinary C++ class/struct (no `UCLASS`, no reflection) is itself part of the lesson: not everything in a UE project has to be an Unreal type, and knowing when to reach for one versus when plain C++ is enough is a real design decision, not a default.

---

## 11. Enemy Strategy Pattern

Combat strategy is separate from weapon/loadout.

```
Enemy
├── Loadout
│   ├── Weapon
│   └── Armour
│
└── Combat Strategy
    ├── Aggressive
    └── Defensive
```

Create:

```
UEnemyCombatStrategy
```

as an abstract UObject-based strategy, with two concrete **C++** subclasses:

```
UAggressiveStrategy
UDefensiveStrategy
```

Both enemy definitions reference these C++ strategies directly — that's the main chain, and it should work end-to-end without Blueprint involved at all. Two strategies are enough to prove the swap works cleanly — a third ("Balanced") would add volume without adding a new lesson. The strategy decides what the enemy should do; it does not need sophisticated AI.

```
Enemy
   ↓
Combat Component
   ↓
Strategy
   ↓
Decision
```

The strategy should not own the actual enemy combat execution.

---

## 12. Blueprint Strategy Extension (isolated experiment)

Both production strategies stay C++ (Section 11). As a **separate, standalone experiment** — not wired into either enemy definition or the Definition of Done — make one Blueprint subclass:

```
UAggressiveStrategy
        ↓
BP_AggressiveStrategy
```

Build it, confirm it satisfies the same interface as its C++ siblings, then set it aside. Do not create a `BP_DefensiveStrategy` counterpart and do not swap either enemy definition over to it — the point is to run the experiment once, not to maintain a third strategy variant alongside the two you already have.

The construction chain this exercises:

```
C++ abstract UObject
        ↓
Blueprint subclass
        ↓
Does it satisfy the same interface as a C++ subclass?
```

The real question this answers isn't "can Blueprint extend this" — it's *"when should a C++ class be designed for Blueprint extension in the first place?"* That's a more interesting and more transferable lesson than the mechanical chain from the original plan (Data Asset → Factory → Enemy), which this project already demonstrates plenty with the C++-only path.

---

## 13. Enemy Loadout

Keep weapon/armour deliberately simple — one of each is sufficient, since the enemy definitions above already demonstrate variation through strategy rather than gear.

```
UWeaponDefinition
UArmorDefinition
```

```
DA_Bow
    Damage
    Range
    AttackCooldown

DA_LightArmour
    Defence
```

Do not build a complete equipment framework.

---

## 14. Enemy Visuals

Visuals should be data/configuration rather than hardcoded in C++.

```
DA_WarriorVisuals
    Mesh
    AnimationClass
    AttackAnimation
    DeathAnimation
```

C++ defines what is configurable. Data selects the actual assets. Blueprint can provide concrete visual configuration where appropriate.

---

## 15. Enemy Events

Enemy should expose useful events, e.g. `OnDamaged`, `OnDied`.

```
Enemy
  │
  └── OnDied
        │
        ├── Progression
        └── Trigger conditions
```

The enemy should not directly call those systems. This demonstrates the Observer/event pattern.

---

## 16. Data Tables

Use at least one Data Table deliberately.

**XPLevels**

| Level | RequiredXP |
|-------|-----------|
| 1 | 0 |
| 2 | 100 |
| 3 | 250 |
| 4 | 500 |
| 5 | 900 |

C++ defines the row struct. The Data Table contains the actual values. This is primarily an exercise in understanding Data Asset vs Data Table. Do not turn the Data Table into a general-purpose database.

---

## 17. Progression

Create a minimal progression system as its own component on the player:

```
APlayerCharacter
├── UStatsComponent
├── UProgressionComponent   ← reads/modifies UStatsComponent
└── UInteractionComponent
```

**Level lives in exactly one place: `UStatsComponent`.** `UProgressionComponent` holds no duplicate state — it listens to `StatsComponent::OnStatChanged` for XP changes, checks the result against the `XPLevels` Data Table, and if the threshold is crossed, calls `StatsComponent::SetStat(Level, NewLevel)`. That write fires `OnStatChanged` normally, same as any other stat change. Progression is pure computation sitting on top of Stats, not a second source of truth for the player's level.

```
StatsComponent::OnStatChanged (XP)
      ↓
ProgressionComponent checks XPLevels table
      ↓
StatsComponent::SetStat(Level, N)
      ↓
OnLevelUp
```

This is also the first place two components on the *same* actor talk to each other via delegate, rather than component-to-subsystem or actor-to-actor — worth calling out in the architecture journal as a distinct communication shape from everything else in the project.

Other systems can observe `OnLevelUp`:

```
OnLevelUp
    ├── HUD updates
    └── Notification appears
```

No sophisticated RPG system is required.

---

## 18. Save System

Create a minimal save system/subsystem. Persist:

```
Level
Stats
Gold
```

This exists primarily to exercise subsystem lifetime, SaveGame objects, serialization, and restoring runtime state. Do not build a sophisticated save architecture.

---

## 19. UI Architecture

Use CommonUI for game-level UI. Use UMG MVVM for UI state/data. Use Slate only if there's a specific reason to drop down to it.

```
UI Root
│
├── HUD Layer
├── Menu Layer
├── Modal Layer
└── Notification Layer
```

Four layers total — no dedicated Achievement layer. Reward feedback (gold gained, level up, door unlocked) now routes entirely through the Notification Layer.

Create a player-local UI system/subsystem:

```
UGameUISubsystem : ULocalPlayerSubsystem
```

It owns/coordinates the UI layers.

---

## 20. HUD Layer

Persistent UI: Health, Mana, Gold, XP, Level.

```
StatsComponent
      ↓
PlayerViewModel
      ↓
MVVM
      ↓
HUD Widget
```

The HUD should not repeatedly reach into the player and manually pull values.

---

## 21. Menu Layer

Screens: **Pause** and **Stats**. With inventory cut, these two screens are enough to demonstrate CommonUI navigation/activation without a screen that has nothing left to show.

```
Game
 ↓
Pause
 ↓
Stats
 ↓
Back
 ↓
Pause
 ↓
Back
 ↓
Game
```

The menu layer owns this navigation rather than individual widgets manually managing viewport Z-order.

---

## 22. Modal Layer

For blocking UI: **Confirmation** and **Level Up**. ("Error" dropped — nothing in this sandbox produces a real failure state worth modeling.)

```
Pause
    ↓
Confirm Dialog
    ↓
Close
    ↓
Pause
```

The modal should take input priority while active.

---

## 23. Notification Layer

Generic transient notifications:

```
+50 Gold
-20 Health
+100 XP
Door Unlocked
Level Up
```

Gameplay should communicate an event/request. The notification system decides how it gets presented.

---

## 24. MVVM

ViewModels sit between gameplay and widgets.

```
StatsComponent
      │
      ▼
PlayerViewModel
      │
      ▼
HUD
```

The ViewModel should be a presentation adapter. Do not turn ViewModels into gameplay systems.

**Bad:**
```
PlayerViewModel
    ├── Combat
    ├── Quests
    └── SaveGame
```

**Good:**
```
Gameplay systems
      ↓
ViewModel
      ↓
UI
```

---

## 25. Subsystems

Use Subsystems only where lifetime/scope makes sense.

```
ULocalPlayerSubsystem
    Game UI

UGameInstanceSubsystem
    Save system
```

Do not use a subsystem simply because you need a globally accessible object. Ask: *"What lifetime does this system belong to?"*

**Bad — the temptation by Day 6, once everything "just needs to be reachable somewhere":**
```
UGameInstanceSubsystem
    ├── Stats
    ├── Progression
    ├── EnemyManager
    ├── TriggerManager
    ├── InteractionManager
    └── EverythingElse
```

**Good:**
```
Stats           → lives on the Player (Component)
Progression     → lives on the Player (Component), reads/modifies Stats via delegate — not a subsystem, not duplicate state
EnemyManager    → probably doesn't need to exist — enemies can be self-sufficient actors
TriggerManager  → each UTrigger is its own object; no manager required
Save            → GameInstanceSubsystem (this one actually is global-lifetime)
UI              → LocalPlayerSubsystem (this one actually is player-scoped)
```

A subsystem is the right answer only when the lifetime genuinely matches Game Instance or Local Player scope — not whenever a class needs to be found from multiple places. Avoid turning subsystems into a giant service locator.

---

## 26. Communication Rules

One of the primary goals of the project.

Prefer:

```
Direct reference
      ↓
Interface
      ↓
Delegate/Event
      ↓
Subsystem
```

depending on the relationship. Avoid unnecessary global event buses, unnecessary casting, widgets directly manipulating gameplay systems, and gameplay directly manipulating widgets.

**GOOD:**
```
Enemy
 ↓ OnDied
Progression System
 ↓
XP gained → Level up
 ↓
UI System
 ↓
Notification Layer
```

**Not:**
```
Enemy
 ↓
NotificationWidget->Show()
```

---

## 27. Construction Rules

For every class, answer:

- Who creates it? Who owns it? What is its lifetime?
- Where is it configured?
- Does Blueprint need access?
- Does it need to be a UObject? Could it simply be a normal C++ class?
- Should another system know about it directly?

These questions are more important than blindly applying design patterns.

---

## 28. Architecture Journal

Not another game feature — a single running file, `ARCHITECTURE.md`, kept alongside the project.

The day you introduce something, write 2–3 lines answering the Construction Rules questions for it, while the reasoning is still fresh:

```
UStatsComponent

Why Component?
- Belongs to Player
- Reusable
- Lifetime follows Player

Why not Subsystem?
- Not global
- State belongs to the individual Player
```

Do this for at least: Component, Interface, Factory, Strategy, Data Asset, Data Table, Subsystem, ViewModel, Delegate.

This isn't overhead — it's the artifact Day 7 actually reviews against. Instead of a vibes-based "does this feel right" pass, Day 7 becomes: reread each journal entry, then check whether the shipped code still matches what you claimed on the day you wrote it. Drift between the two is usually exactly where the architecture review should focus. By the end of the week you'll also have a personal "how I structure UE5" reference, rather than just a finished toy project.

---

## 29. C++ / Data / Blueprint Boundary

**C++:** Architecture, components, actors, interfaces, delegates, factories, strategies, subsystems, ViewModels, runtime state, communication, APIs, rules.

**Data Assets:** Enemy definitions, weapon/armour definitions, visual definitions, configuration.

**Data Tables:** Tabular values, XP curves/levels, similar homogeneous data.

**Blueprint:** Mesh assignment, animation assignment, visual composition, widget layouts, concrete visual variants, asset configuration, small presentation-only behaviour.

Blueprint should have minimal gameplay logic.

---

## 30. One-Week Schedule

*Journal habit: on each day below, write the `ARCHITECTURE.md` entry for whatever you built that day, before moving on. It takes minutes and Day 7 depends on it.*

**Day 1 — Unreal foundations**
Project setup, C++ module, character, components, Blueprint-derived classes, properties, basic asset references.
*Goal: Understand Unreal's object/component/class model.*

**Day 2 — Interaction + Stats**
Stats Component, Interaction Component, Interactable base class, Interactable interface, generic stat modification, delegates.
*Goal: Understand C++ object communication.*

**Day 3 — Enemy architecture**
Enemy, Enemy Definition, weapon/armour definitions, Enemy Loadout, Strategy base class, two concrete C++ strategies, Enemy Factory (the project's one deliberate plain C++ class), visual definitions. Run the isolated BP strategy experiment (Section 12) if time allows.
*Goal: Understand Data → Factory → Runtime Object → Strategy.*

**Day 4 — Events + Conditions**
Enemy death events, Trigger system, two conditions, door progression, XP/level progression.
*Goal: Understand event-driven communication.*

**Day 5 — CommonUI + MVVM**
UI subsystem, UI root, HUD layer, Menu layer, Modal layer, Notification layer, ViewModels.
*Goal: Understand the UI architecture. Budget the most slack here — CommonUI input routing is the most likely source of overrun.*

**Day 6 — Save + buffer**
Save subsystem, save/load runtime state. Lighter now that achievements are cut — treat remaining time as buffer for Day 5 overflow or Day 7 prep.
*Goal: Understand subsystem lifetime.*

**Day 7 — Architecture review**
Do not add features. Instead:
- Reread `ARCHITECTURE.md` entry by entry, and check each claim against what actually shipped — drift between the two is where to focus
- Remove unnecessary dependencies
- Identify inappropriate casts
- Check ownership, UObject lifetimes, subsystem responsibilities
- Check Blueprint / Data Asset responsibilities
- Test adding a new enemy, a new interactable, a new UI screen, a new condition

The final question: *"How much existing code needs to change when I add a new piece of content?"* Ideally: very little.

---

## 31. Definition of Done

**Enemy**
```
DA_RangedWarrior → Enemy Factory → AEnemy (BP_Enemy for mesh/anim) → Bow + Light Armour + Aggressive Strategy
```
Another enemy can be created by changing only the Combat Strategy field.

**Interaction**
```
BP_HealthShrine → Interact → Health +25 → StatChanged → HUD updates
```

**Conditions**
```
Kill enemies + Have enough gold → Door → Opens
```

**UI**
HUD, Menu, Modal, Notification all coexist as separate layers with appropriate input/navigation behaviour.

**Events**
```
Enemy died → XP → Level up → Notification
```
without the enemy directly depending on those systems.

**Persistence**
Save and reload: Stats, Level, Gold.

---

## 32. The Actual Learning Objective

At the end of the week, you should be comfortable looking at a requirement like *"I need a new enemy variant"* and immediately thinking:

- Is this a new class? No. It's probably a Data Asset.
- Does it need new behaviour? If yes → Strategy.
- Does it need a new runtime object? Factory.
- Does it need persistent state? Component/object.
- Does another system need to know about it? Delegate/interface.
- Does the UI need to react? Event → ViewModel → CommonUI.

That is the skill this project is designed to develop. The dungeon is just the excuse.
