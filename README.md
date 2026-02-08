# ZeBomber

An Unreal Engine 5.7 aerial combat game featuring first-person fighter gameplay with bombs and rockets.

---

## Fighter Pawn Setup

The **FighterPawn** is a first-person cockpit-view pawn. The camera sits at the nose of an invisible airplane. The player flies with WASD, aims rockets with the mouse, and drops bombs with Space.

### Controls

| Key | Action |
|-----|--------|
| **W** | Tip nose down (dive) |
| **S** | Tip nose up (climb) |
| **A** | Turn left |
| **D** | Turn right |
| **Mouse** | Aim rocket crosshair (white) |
| **Left Click** | Fire rockets (hold for auto-fire) |
| **Right Click (hold)** | Free-look camera (look around while flying) |
| **Space** | Drop bomb |
| **Mouse Scroll Up/Down** | Zoom radar in/out |

### Crosshairs

- **White + crosshair** — Follows the mouse cursor. Rockets fire toward this point.
- **Red circle crosshair** — Predicted bomb impact point. Automatically calculated from current speed, altitude, direction, and gravity.

### Free-Look Camera

Hold **Right Mouse Button** to look around independently of flight direction. Release to snap back to forward view.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Free Look Max Yaw** | Maximum left/right look angle (degrees) | 90 |
| **Free Look Max Pitch** | Maximum up/down look angle (degrees) | 80 |
| **Free Look Return Speed** | How fast camera returns to forward when RMB released | 5.0 |
| **Aim Sensitivity** | Base mouse sensitivity for both aiming and free-look | 2.5 |
| **Free Look Sensitivity Multiplier** | Multiplies base sensitivity for free-look only | 3.0 |

#### Free-Look Tuning
- **Faster free-look:** Increase `Free Look Sensitivity Multiplier` (e.g., 5.0 for very fast)
- **Slower free-look:** Decrease `Free Look Sensitivity Multiplier` (e.g., 1.0 for same as aiming)
- **Wider look angles:** Increase `Free Look Max Yaw` and `Free Look Max Pitch`
- **Quicker snap-back:** Increase `Free Look Return Speed`

### Radar System

A square radar display in the top-right corner shows enemies within range. The radar rotates with the player's heading so "up" on radar is always forward.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Radar World Range** | How far the radar can detect enemies (units) | 8000 |
| **Radar Radius** | Visual size of radar on screen (pixels) | 120 |
| **Radar Zoom** | Current zoom level (1.0 = default, lower = zoomed in) | 1.0 |
| **Radar Zoom Step** | How much zoom changes per scroll | 0.15 |
| **Radar Zoom Min/Max** | Zoom limits | 0.2 / 3.0 |

#### Radar Elements
- **Blue triangle** — Player (fighter) at center, pointing forward
- **Red diamond dots** — Tanks on the ground
- **Yellow circle dots** — Helicopters (with vertical height bar below)
- **Square border** — Outer radar boundary (zooms with range)
- **Concentric rings** — Distance markers (66% and 33% of range)

#### Radar Controls
- **Mouse Scroll Up** — Zoom in (see nearby enemies in more detail)
- **Mouse Scroll Down** — Zoom out (see farther enemies)

---

## Blueprint Configuration

All parameters are editable in the **BP_Fighter** Blueprint Details panel. Select the root component to see them.

### Flight Parameters

Found under the **Flight** category in the Details panel.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Min Speed** | Stall speed floor — airplane won't go slower than this | 800 |
| **Max Speed** | Top speed ceiling | 3000 |
| **Default Speed** | Cruising speed when flying level | 1500 |
| **Speed Change Rate** | How much diving/climbing affects speed (units/sec²) | 400 |
| **Pitch Rate** | How fast the nose goes up/down (degrees/sec) — higher = more agile | 12 |
| **Pitch Inertia** | Sluggishness of pitch response (0 = instant, 0.95 = heavy bomber) | 0.92 |
| **Yaw Rate** | How fast the airplane turns left/right (degrees/sec) — higher = more agile | 15 |
| **Yaw Inertia** | Sluggishness of turning (0 = instant, 0.95 = heavy bomber) | 0.90 |
| **Roll Rate** | How fast the airplane banks when turning (degrees/sec) | 20 |
| **Max Roll Angle** | Maximum visual bank angle when turning (degrees) | 30 |
| **Max Pitch Angle** | Maximum nose up/down angle (degrees) — limits how steep you can fly | 45 |
| **Leveling Speed** | How fast the airplane auto-levels at minimum altitude (degrees/sec) | 15 |
| **Min Altitude** | Floor altitude — airplane cannot fly below this | 500 |
| **Start Altitude** | Altitude where the airplane spawns at the start of the game | 5000 |

#### Tuning Tips

- **More agile airplane:** Increase `Pitch Rate` and `Yaw Rate`, decrease `Pitch Inertia` and `Yaw Inertia` (closer to 0).
- **Heavy bomber feel:** Decrease `Pitch Rate` and `Yaw Rate`, increase `Pitch Inertia` and `Yaw Inertia` (closer to 0.95).
- **Limit steep angles:** Lower `Max Pitch Angle` (e.g., 20 degrees prevents extreme dives/climbs).
- **Faster gameplay:** Increase `Min Speed` and `Default Speed`.

### Camera Parameters

Found under the **Camera** category.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Camera Offset** | Position offset from pawn origin (X = forward/back, Y = left/right, Z = up/down) | (0, 0, 0) |
| **Camera Pitch Offset** | Tilt the default view (negative = look down for better ground visibility) | 0 |

#### Tuning Tips

- Set `Camera Pitch Offset` to **-5 to -15** degrees to angle the view slightly downward for better ground target visibility.
- Adjust `Camera Offset` Z value (e.g., 50) to raise the viewpoint slightly above the airplane nose.

### Rocket Parameters

Found under the **Rocket** category.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Rocket Class** | Blueprint class for the rocket projectile | None (must assign) |
| **Rocket Cooldown** | Seconds between shots | 0.15 |
| **Rocket Spawn Offset** | Where rockets spawn relative to pawn origin (local space) | (300, 0, -50) |
| **Crosshair Max Distance** | Maximum raycast distance for mouse aiming (units) | 50000 |

### Bombing Parameters

Found under the **Bombing** category.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Bomb Class** | Blueprint class for the bomb projectile | None (must assign) |
| **Bomb Drop Speed** | Additional speed added to the bomb on release (units/sec) | 0 |
| **Bomb Cooldown** | Seconds between bomb drops | 0.5 |
| **Bomb Spawn Offset** | Where bombs spawn relative to pawn origin (local space) | (0, 0, -100) |
| **Bomb Gravity** | Gravity for impact prediction — should match Project Settings gravity | 980 |
| **Bomb Drop Sound** | Sound played when a bomb is released | None (optional) |

---

## Game Mode Setup

To use the Fighter pawn in a level:

1. Create a **Game Mode** Blueprint based on `GameModeBase`
2. Set **Default Pawn Class** → `BP_Fighter`
3. Set **Player Controller Class** → `FighterPlayerController`
4. Set **HUD Class** → `FighterHUD`
5. In your level's **World Settings**, set **GameMode Override** to your new Game Mode

---

## Input Setup

### Input Actions (all Digital / bool)

Create these in Content Browser → **Input** → **Input Action**:

- `IA_PitchDown` — W key
- `IA_PitchUp` — S key
- `IA_TurnLeft` — A key
- `IA_TurnRight` — D key
- `IA_DropBomb` — Space
- `IA_FireRocket` — Left Mouse Button

### Input Mapping Context

Create `IMC_Fighter` in Content Browser → **Input** → **Input Mapping Context**, and map each Input Action to its key.

Assign `IMC_Fighter` and all six Input Actions in the **BP_Fighter** Blueprint under the **Input** category.

---

## C++ Source Files

| File | Description |
|------|-------------|
| `FighterPawn.h/.cpp` | First-person cockpit pawn with flight physics, weapons, and bomb impact prediction |
| `FighterHUD.h/.cpp` | Draws red bomb crosshair and white rocket crosshair on screen |
| `FighterPlayerController.h/.cpp` | Configures mouse input for precise aiming (hidden OS cursor) |
| `BomberPawn.h/.cpp` | Original third-person bomber pawn (legacy) |
| `BombProjectile.h/.cpp` | Bomb projectile with physics and explosion |
| `RocketProjectile.h/.cpp` | Rocket projectile with straight-line flight |
