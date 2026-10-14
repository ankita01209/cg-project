# Project Audit

## 1. Project Layout & CMake
- **File Tree:** Standard `src/`, `assets/`, `libs/` structure.
- **CMake Source Listing:** Explicitly lists every `.cpp` file under `add_executable`. No `GLOB` usage.
- **Generator:** Cached generator is "Visual Studio 17 2022" (from `implementation_plan.md` instructions).
- **Shaders Runtime:** `CMakeLists.txt` defines `ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"` macro, so the executable finds absolute paths accurately.

## 2. OpenGL Context & Instancing
- **Context:** OpenGL 3.3 Core (configured safely through `glad/`).
- **Instancing:** GL 3.3 natively supports `glDrawElementsInstanced` and `glVertexAttribDivisor`. We will utilize these for the procedural asteroid belt.

## 3. Celestial Data & Simulation
- **Key Fields:** `radius`, `orbitDistance`, `orbitalSpeed`, `rotationSpeed`, `currentOrbitAngle`, `position`.
- **World Position Compute:** Circular XZ orbit computed as `x = cos(angle) * dist`, `z = sin(angle) * dist`.
- **Moon Parenting:** Moon is parented dynamically in `getMoonModelMatrix()` by composing `EarthWorld * MoonOrbit * MoonSpinScale`.
- **Actual Values:**
  - Sun (r = 1.5, orb = 0.0) | Earth (r = 0.17, orb = 8.5)
  - Mars (r = 0.12, orb = 11.0) | Jupiter (r = 0.55, orb = 15.0)
  - Moon (r = 0.3 local [0.05 world], orb = 12.0 local [2.0 world])

## 4. Simulation Delta Time
- `main.cpp` calculates `rawDelta` as the difference between frames.
- This is multiplied by `inputManager.getTimeScale()` and fed as `deltaTime` to `solarSystem.update(deltaTime)`. We must inject this exact scaled parameter into new logic.

## 5. Camera Architecture
- **Modes:** Orbit-driven using target vector, distance scalar, yaw, and pitch.
- **Tour Pathing:** `InputManager.cpp` uses a `smoothstep(t)` polynomial ease-in/ease-out to interpolate from previous camera position to new planet frames softly.

## 6. Input & Tour State
- **InputManager Tracking:** Manages `TourState` (Off, Transitioning, Dwelling).
- **Tour Hooks:** Exposes `startTourTransition()` and `cancelTour()` mechanisms ensuring a clean state.

## 7. Renderer Structure
- **Shader setup:** Pushes Matrix/Lighting uniforms down natively per mesh (`basic.frag`).
- **Sun Execution:** Checks index `i==0`, toggling `uIsEmissive=1` discarding Blinn-Phong diffuse shading.
- **Blend States:** Employs explicit sequential transparent draw stacks without depth writes matching back-to-front ordering.
- **Orbit paths:** `GL_LINE_STRIP` iterations computed externally and fed directly to VBOs per frame.

## 8. UI Overlay Design
- **Existing panels:** Centrally controlled in `UIOverlay::renderPanel`. Utilizes `ImGui::BeginTabBar("MainTabs")`.
- **Injection point:** We will inject "SOLAR OBSERVATORY" as a top-level tab item matching the styling structure of `Focus Info` and `Compare`.

## 9. Risks and Integration Strategy
- **Mode Switching Risk:** Simultaneous active states (Tour vs Mission vs Free).
  - *Mitigation:* Create central `requestMode()` proxy inside `InputManager` immediately calling `cancelTour()` or `m_camera.abortMission()` restoring fallback camera locks.
- **Performance Risk:** Belt allocation.
  - *Mitigation:* `glBufferSubData` bound instance drawing ensures 300 instances costs identically essentially 1 draw call preventing draw starvation.
- **Time Sync Risk:** Missions traveling on scaled delta.
  - *Mitigation:* Strict pipeline injection below `rawDelta * timeScale` in the main sim loop.
