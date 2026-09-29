# JM Poison

Fork-derived from Poisoner's Aid by NoahBoddie. This build intentionally targets Skyrim Special Edition 1.5.97.

Pass 1 provides configurable weapon-poison dose progression based on Alchemy skill and configurable perks, with a configurable perk that unlocks an infinite coating.

Default integration uses the existing rank perks in `JM_Sithis_Overhaul.esp`:
- Rank 1 `0x00000800`: +1 dose
- Rank 2 `0x00000801`: +2 doses
- Rank 3 `0x00000802`: +3 doses
- Rank 4 `0x00000803`: +4 doses
- Rank 5 `0x00000804`: infinite coating

All thresholds, bonuses, perk FormIDs, and the finite cap are configurable in `JM_Poison.ini`.

This fork retains the upstream GPL-3.0-or-later license.
