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
| **Space** | Drop bomb |

### Crosshairs

- **White + crosshair** — Follows the mouse cursor. Rockets fire toward this point.
- **Red circle crosshair** — Predicted bomb impact point. Automatically calculated from current speed, altitude, direction, and gravity.

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
