# ZeBomber

An Unreal Engine 5.7 aerial combat game. Defend your base from waves of tanks and helicopters using bombs and rockets from a first-person fighter cockpit.

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
| **Q** | Slide left (lateral adjustment) |
| **E** | Slide right (lateral adjustment) |
| **Mouse** | Aim rocket crosshair (white) |
| **Left Click** | Fire rockets (hold for auto-fire) |
| **Right Click (hold)** | Free-look camera (look around while flying) |
| **Space** | Drop bomb |
| **[** | Radar zoom in |
| **]** | Radar zoom out |
| **Mouse Scroll Up/Down** | Adjust fighter speed |
| **/** | Toggle jet HUD on/off |
| **ESC** | Pause game (show instructions + quit option) |
| **X** | Quit game (only when paused) |

### Game Flow

1. **Instructions screen** — Game starts showing built-in instructions. Press **Space** to begin.
2. **Wave gameplay** — Enemies spawn and advance toward the base at (0,0,0). Destroy all enemies to complete the wave.
3. **Wave end screen** — Shows wave duration, kill stats, and base HP. Press **Space** for the next wave.
4. **Pause** — Press **ESC** to pause. Shows instructions + "Press X to close the game". Press **ESC** again to resume.
5. **Game Over** — If base HP reaches 0, the game over screen appears. Press **Space** to restart.

### Base Defense

- Enemies stop at their line of fire and shoot the base at variable intervals.
- Each hit reduces **Base HP** by 1 and flashes the screen red.
- Base HP is shown in the top-left score panel. Turns red when below 25%.
- If Base HP reaches 0, you lose.
- Only **bombs** destroy tanks. **Rockets** destroy helicopters.

### Wave System

- Waves are managed by `TankWaveSpawner` and `HeliWaveSpawner` actors in the level.
- Spawners wait for the FighterPawn to trigger each wave (no auto-spawning).
- Each wave adds more enemies (configurable per-spawner).
- Kill counters show **X/Y** format (destroyed/total in current wave).

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

### Slide Movement (Q/E Keys)

Use **Q** and **E** keys for subtle lateral sliding adjustments while maintaining forward flight. This simulates rudder/aileron trim for precise bomb targeting.

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Slide Speed** | Lateral movement speed when holding Q/E (units/sec) | 300 |
| **Slide Decay Rate** | How quickly slide movement decays when keys released (units/sec²) | 800 |

#### How Slide Movement Works
- **Q key** slides left relative to aircraft orientation
- **E key** slides right relative to aircraft orientation  
- Slide velocity decays smoothly when keys are released
- Movement combines with normal forward flight
- Provides fine positioning control without affecting heading

#### Tuning Tips
- **Faster sliding:** Increase `Slide Speed` (e.g., 500 for quick adjustments)
- **Slower sliding:** Decrease `Slide Speed` (e.g., 150 for subtle movements)
- **Quicker stop:** Increase `Slide Decay Rate` (e.g., 1200 for rapid deceleration)
- **Gentler stop:** Decrease `Slide Decay Rate` (e.g., 400 for smooth coasting)

### Speed Control (Mouse Wheel)

Use **mouse wheel up/down** to adjust the fighter's default cruising speed. This affects your base speed that can be modified by pitching (diving/climbing).

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Speed Step** | How much speed changes per mouse wheel scroll (units/sec) | 100 |

#### How Speed Control Works
- **Mouse wheel up** increases default cruising speed
- **Mouse wheel down** decreases default cruising speed  
- Speed is clamped between `Min Speed` and `Max Speed` limits
- Pitching (W/S keys) still modifies speed around this new default
- Changes persist until you adjust again or restart the game

#### Tuning Tips
- **Faster adjustments:** Increase `Speed Step` (e.g., 200 for quick changes)
- **Finer control:** Decrease `Speed Step` (e.g., 50 for precise adjustments)
- **Speed limits:** Adjust `Min Speed` and `Max Speed` in Flight Parameters

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
- **[** — Zoom in (see nearby enemies in more detail)
- **]** — Zoom out (see farther enemies)

### Jet HUD (Pitch Ladder)

A compact green fighter jet HUD overlay at screen center showing aircraft orientation relative to the ground. Press **/** to toggle it on/off. The current state is shown in the bottom-right settings panel.

#### Elements
- **Horizon line** — Tilts with roll, moves with pitch. Shows where level flight is.
- **Pitch ladder** — ±10° and ±20° lines. Solid above horizon, dashed below. Degree labels on the right.
- **Aircraft symbol** — Fixed wings + center dot at screen center. Compare against the horizon to see your pitch/roll.
- **Heading** — "HDG 270" text above the ladder showing compass heading.
- **Speed** — Shown in knots to the left of the ladder.
- **Altitude** — Shown in feet to the right of the ladder.

#### Parameters (HUD|JetHUD category)

| Parameter | Description | Default |
|-----------|-------------|---------|
| **Jet HUD Color** | Main overlay color (classic green) | (0, 1, 0.2, 0.7) |
| **Jet HUD Dim Color** | Secondary/below-horizon color | (0, 1, 0.2, 0.35) |
| **Jet HUD Thickness** | Line thickness | 1.5 |
| **Horizon Line Width** | Width of horizon line from center (px) | 100 |
| **Pitch Ladder Width** | Width of pitch lines (px) | 60 |
| **Pitch Pixels Per Degree** | Spacing between pitch lines | 4 |
| **Pitch Ladder Step** | Degrees between lines | 10 |
| **Pitch Ladder Range** | Max degrees shown (±) | 20 |

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
| **Slide Speed** | Lateral movement speed when holding Q/E (units/sec) | 300 |
| **Slide Decay Rate** | How quickly slide movement decays when keys released (units/sec²) | 800 |
| **Speed Step** | How much speed changes per mouse wheel scroll (units/sec) | 100 |

#### Tuning Tips

- **More agile airplane:** Increase `Pitch Rate` and `Yaw Rate`, decrease `Pitch Inertia` and `Yaw Inertia` (closer to 0).
- **Heavy bomber feel:** Decrease `Pitch Rate` and `Yaw Rate`, increase `Pitch Inertia` and `Yaw Inertia` (closer to 0.95).
- **Limit steep angles:** Lower `Max Pitch Angle` (e.g., 20 degrees prevents extreme dives/climbs).
- **Faster gameplay:** Increase `Min Speed` and `Default Speed`.
- **Slide adjustments:** Modify `Slide Speed` for lateral movement rate, `Slide Decay Rate` for how quickly sliding stops.
- **Speed control:** Adjust `Speed Step` for mouse wheel sensitivity, `Min Speed`/`Max Speed` for overall speed range.

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
- `IA_SlideLeft` — Q key
- `IA_SlideRight` — E key
- `IA_RadarZoomIn` — [ key
- `IA_RadarZoomOut` — ] key
- `IA_SpeedUp` — Mouse Wheel Up
- `IA_SpeedDown` — Mouse Wheel Down
- `IA_DropBomb` — Space
- `IA_FireRocket` — Left Mouse Button

### Input Mapping Context

Create `IMC_Fighter` in Content Browser → **Input** → **Input Mapping Context**, and map each Input Action to its key.

Assign `IMC_Fighter` and all twelve Input Actions in the **BP_Fighter** Blueprint under the **Input** category.

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
