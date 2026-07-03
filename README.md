# Quel che resta

> A zombie survival prototype — inspired by The Walking Dead, built like Rust/ARK/Project Zomboid.

## What is this

Personal project started from a simple frustration: there's no real open-world multiplayer survival game set in a Walking Dead-style world. This is an attempt to build one.

The long-term vision is a PvPvE survival MMO — factions, base building, raiding, city exploration, roleplay — simulating what life during a zombie apocalypse would actually feel like socially and mechanically.

This repository contains two working prototypes of the core gameplay loop.

---

## Prototypes

### Godot 4 (`/godot`)

First-person survival prototype built in Godot 4 (GDScript).

**Implemented:**
- First-person controller with head bob, sprint, crouch (stealth)
- **Noise system** — every action has a sound radius. Gunshot = 48m, bat = 6m, running = 11m. Zombies investigate the source of noise, not the player directly
- Zombie AI with 3 states (wander → investigate → chase) and 3 variants (Walker, Runner, Brute)
- NavMesh pathfinding baked at runtime
- Full day/night cycle with dynamic sun, moon, and fog
- Night hordes that spawn from the map edge and converge on the player's last known position
- Stealth: crouching reduces detection radius to 45%. Flashlight increases it by 70% at night
- Scavenging (crates, abandoned cars), barricade placement, hunger system
- Procedural audio synthesized at runtime (gunshot, zombie groans, heartbeat under 30% HP, ambience)
- Permadeath with record saved to disk

### Unreal Engine 5 (`/ue5`)

Port of the same design to UE5 in C++ (~1900 lines).

**Same feature set, different implementation:**
- Uses `UAISenseConfig_Hearing` + `ReportNoiseEvent` — UE5's native hearing system — instead of a custom signal bus. The noise radius is passed directly to the engine's AI perception system
- Procedural audio synthesized in C++ at runtime (same synthesis recipes as the Godot version, zero audio assets)
- `NavigationAgent` pathfinding with straight-line fallback if no NavMesh is present
- Weapon viewmodels with animations (swing, recoil, muzzle flash) built entirely in code
- SaveGame record persistence
- Dynamic fog (ExponentialHeightFog) that thickens at night
- World generation: roads, abandoned cars, buildings, trees, camps — all procedural

---

## The core design idea

The noise system is the signature mechanic. Every decision is a tradeoff between effectiveness and noise:

- Pistol kills in one shot but makes noise in a 48m radius
- Bat is silent but requires getting close
- Running is faster but attracts nearby zombies
- Crouching makes you nearly invisible but very slow
- The flashlight helps you see at night but makes you visible from further away

This mirrors the logic of the show: the most dangerous thing you can do is make noise.

---

## Stack

| | Godot version | UE5 version |
|---|---|---|
| Language | GDScript | C++ |
| Engine | Godot 4.3 | Unreal Engine 5.8 |
| AI | Custom state machine + NavMesh | AIPerception (native UE hearing) |
| Audio | Procedural synthesis (Python → runtime WAV) | Procedural synthesis (C++ runtime PCM) |
| Persistence | Godot file API | UE5 SaveGame |

---

## Status

Early prototype. Greybox geometry (capsules and boxes), no character models or textures yet. The gameplay loop works — design validation is the current goal before moving to assets.

Looking for collaborators (programmers and 3D artists). See [r/INAT post](#) if interested.

---

## Author

Niko — student, C++/Java/PHP background, first game project.
