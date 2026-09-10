# _ThirdParty — quarantined marketplace content

This folder is gitignored (`/.gitignore:80`). Do not commit its contents.

Currently quarantined (~9.8 GB):
- `ModularSciFiStation/` (2.3 GB)
- `ParagonMinions/` (4.9 GB)
- `ParagonLtBelica/` (2.0 GB)
- `ParagonWraith/` (1.3 GB)
- `ParagonMuriel/` (776 MB)
- `ParagonMaleAnnouncer/` (153 MB)

When `Agora` and `Monolith` finish downloading, move them here as well:
`DaDungeon/Content/Agora/` → `DaDungeon/Content/_ThirdParty/Agora/`
`DaDungeon/Content/Monolith/` → `DaDungeon/Content/_ThirdParty/Monolith/`
(or `ParagonAgora`/`ParagonMonolith` variants).

## Restore

Recreate via Fab / Epic Launcher: **Add to Project → DaDungeon** and choose `_ThirdParty` as target (or move after install). Do not `git clone` large packs.

## After moving — fix redirectors

Moving Content folders outside the Editor breaks UE redirectors. After any move:

1. Open `DaDungeon.uproject` in Unreal Editor (UE 5.8).
2. **Tools → Fix Up Redirectors** on `Content/_ThirdParty/` and `Content/DaDungeon/`.
3. Resave affected maps (`DaDungeon/Levels/DungeonLevel`, `MainMenuLvl`).
4. Verify PIE still loads Modular/Paragon meshes.

`.gitignore` also proactively ignores `Content/Agora/`, `Content/Monolith/`, and top-level `Paragon*`/`ModularSciFiStation/` so a fresh Fab install won't accidentally get committed before you quarantine it.
