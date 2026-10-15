# 🌌 Interactive 3D Solar System Simulator & Celestial Orrery

[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg?style=flat-square&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![OpenGL 3.3 Core](https://img.shields.io/badge/Graphics-OpenGL%203.3%20Core-red.svg?style=flat-square&logo=opengl)](https://www.opengl.org/)
[![GLFW 3.4](https://img.shields.io/badge/Windowing-GLFW%203.4-green.svg?style=flat-square)](https://www.glfw.org/)
[![GLM 1.0.1](https://img.shields.io/badge/Math-GLM%201.0.1-orange.svg?style=flat-square)](https://github.com/g-truc/glm)
[![Dear ImGui 1.90.1](https://img.shields.io/badge/GUI-Dear%20ImGui%201.90.1-purple.svg?style=flat-square)](https://github.com/ocornut/imgui)
[![CMake 3.20+](https://img.shields.io/badge/Build-CMake%20%E2%89%A5%203.20-brightgreen.svg?style=flat-square&logo=cmake)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](LICENSE)

An interactive, scientifically calibrated 3D Solar System Simulator and Celestial Orrery developed in **modern C++17** and **OpenGL 3.3 Core Profile**. Features real-time hierarchical orbital mechanics, photorealistic surface texturing, hardware-instanced asteroid belts, scientifically differentiated minor planets, dynamic comet tails, multi-pass HDR Bloom post-processing, an atmospheric Fresnel scattering model, and an educational cinematic tour mode.

---

## 📸 Table of Contents

- [Overview & Architecture](#-overview--architecture)
- [Key Features](#-key-features)
  - [1. Celestial Orrery & Hierarchy](#1-celestial-orrery--hierarchy)
  - [2. Minor Planets & Scientifically Differentiated Asteroids](#2-minor-planets--scientifically-differentiated-asteroids)
  - [3. Hardware-Instanced Asteroid Belt](#3-hardware-instanced-asteroid-belt)
  - [4. Dynamic Comets & Solar Wind Tails](#4-dynamic-comets--solar-wind-tails)
  - [5. Visuals & Post-Processing Pipeline](#5-visuals--post-processing-pipeline)
  - [6. Camera System & Cinematic Tour](#6-camera-system--cinematic-tour)
  - [7. Interactive HUD & Observatory GUI](#7-interactive-hud--observatory-gui)
- [Computer Graphics Techniques](#-computer-graphics-techniques)
- [Project Directory Structure](#-project-directory-structure)
- [Controls & Keybindings](#-controls--keybindings)
- [Prerequisites & Dependencies](#-prerequisites--dependencies)
- [Build & Run Instructions](#-build--run-instructions)
  - [Windows (Visual Studio 2022)](#windows-visual-studio-2022)
  - [Windows (MinGW-w64 / MSYS2 / Ninja)](#windows-mingw-w64--msys2--ninja)
  - [Linux / macOS](#linux--macos)
- [Shader Pipeline Details](#-shader-pipeline-details)
- [Educational Telemetry & Astronomical Data](#-educational-telemetry--astronomical-data)
- [Troubleshooting & FAQ](#-troubleshooting--faq)
- [License & Acknowledgments](#-license--acknowledgments)

---

## 🔭 Overview & Architecture

The simulator is built with a clean separation of concerns across core systems:

```
                      ┌─────────────────────────────────────────┐
                      │              main.cpp                   │
                      │     (Render Loop & State Manager)       │
                      └────┬───────────┬───────────┬────────────┘
                           │           │           │
         ┌─────────────────▼┐    ┌─────▼─────┐    ┌▼──────────────────┐
         │  scene::         │    │  camera:: │    │  render::         │
         │  SolarSystem     │    │  Camera   │    │  Bloom / Shader / │
         │  CelestialBody   │    │           │    │  Sphere / Mesh /  │
         │  AsteroidBelt    │    └─────┬─────┘    │  Texture          │
         └─────────┬────────┘          │          └─────────┬─────────┘
                   │                   │                    │
         ┌─────────▼───────────────────▼────────────────────▼─────────┐
         │                      OpenGL 3.3 Core                       │
         │        (VAO / VBO / FBO / Textures / GLSL Shaders)         │
         └─────────────────────────────┬──────────────────────────────┘
                                       │
                      ┌────────────────▼────────────────┐
                      │           ui::UIOverlay         │
                      │    (Dear ImGui Observatory HUD) │
                      └─────────────────────────────────┘
```

- **Core Engine**: Encapsulated GLFW window creation, OpenGL context initialization, input dispatching, and high-precision frame timing (`deltaSeconds`).
- **Scene Graph**: Manages hierarchical transformations, where child bodies (moons) evaluate their spatial positions relative to parent bodies (planets) dynamically per frame via model matrix stacks.
- **Rendering Pipeline**: Multi-pass rendering pipeline combining an opaque pass (Blinn-Phong lit spheres), an instanced pass (GPU asteroid field), a sorted transparent pass (planetary atmospheres, rings, corona glow, comet tails), and an HDR Bloom post-processing pipeline.
- **Observatory UI**: Full Dear ImGui integration offering planetary telemetry, orbit path toggles, speed sliders, educational facts, and diagnostic performance graphs.

---

## 🌟 Key Features

### 1. Celestial Orrery & Hierarchy
- **The Sun + 8 Major Planets**: Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune.
- **14 Natural Satellites (Moons)** parented dynamically with local orbital radii and velocities:
  - **Earth**: The Moon
  - **Mars**: Phobos, Deimos
  - **Jupiter**: Galilean Moons (Io, Europa, Ganymede, Callisto)
  - **Saturn**: Titan, Enceladus, Rhea, Iapetus
  - **Uranus**: Titania, Miranda
  - **Neptune**: Triton
- **Hierarchical Transform Stack**: Moons compute their world matrices via:
  $$\mathbf{M}_{\text{moon}} = \mathbf{T}(\mathbf{P}_{\text{parent}}) \cdot \mathbf{T}(\mathbf{P}_{\text{local orbit}}) \cdot \mathbf{R}(\theta_{\text{spin}}) \cdot \mathbf{S}(r_{\text{moon}})$$
- **Compressive Scale Modeling**: Astronomical distances and radii are scaled with calibrated logarithmic compression to allow simultaneous viewport observation while strictly maintaining relative orbital hierarchies.

### 2. Minor Planets & Scientifically Differentiated Asteroids
Rather than using generic grey placeholders, the simulator features 6 major minor planets modeled after authentic NASA and ESO spacecraft observations:

| Minor Planet | Classification | Scientific Real-Life Features | Geometric Modeling | Material Reflectance |
| :--- | :--- | :--- | :--- | :--- |
| **1 Ceres** | Dwarf Planet | Dark brownish-clay crust; prominent bright white sodium carbonate salt deposits in *Occator Crater* (*Cerealia Facula*) and the *Ahuna Mons* cryovolcano. | Hydrostatic Sphere | Balanced matte (`spec=0.20`, `shininess=20.0`) |
| **4 Vesta** | Differentiated Protoplanet | High-albedo basaltic crust (albedo ~0.42); colossal 500 km *Rheasilvia* south-polar impact basin with a 22 km central peak; equatorial *Divalia Fossae* troughs. | Oblate Ellipsoid (`1.08 × 0.88 × 1.04`) | Bright Basaltic (`spec=0.35`, `shininess=32.0`) |
| **2 Pallas** | B-type Asteroid | Distinct slate-blue / bluish-grey carbonaceous tone; heavily battered "golf-ball" cratered crust; steep **34.8° inclination** to the ecliptic plane. | Battered Sphere (`1.04 × 0.96 × 1.02`) | Diffuse Carbonaceous (`spec=0.18`, `shininess=16.0`) |
| **10 Hygiea** | C-type Dwarf Planet Candidate | Ultra-dark graphite/soot primitive carbonaceous surface (albedo ~0.07); remarkably spherical under self-gravity. | Pristine Sphere | Absorptive Matte (`spec=0.04`, `shininess=8.0`) |
| **433 Eros** | Near-Earth Asteroid | Warm golden-tan / silicate ochre coloration; *Himeros* saddle depression; smooth ponded regolith flats; visited by *NEAR Shoemaker*. | Peanut Geometry (`2.20 × 0.95 × 1.05`, 25° tilt) | Stony Silicate (`spec=0.22`, `shininess=24.0`) |
| **16 Psyche** | M-type Metallic Asteroid | Metallic nickel-iron protoplanetary core stripped by ancient collisions; *Widmanstätten* crystalline reflection; high radar albedo. | Irregular Core (`1.12 × 0.92 × 1.05`) | Polished Metal (`spec=0.85`, `shininess=64.0`) |

### 3. Hardware-Instanced Asteroid Belt
- Over **1,200 procedural asteroids** rendered in a single draw call via `glDrawElementsInstanced`.
- Per-instance attributes streamed via VBOs with `glVertexAttribDivisor(..., 1)`:
  - `vec3 aInstanceOffset`: Orbit radius (semi-major axis between Mars and Jupiter), vertical offset, and current orbital angle.
  - `vec3 aInstanceColor`: Unique basaltic, silicate, and carbonaceous mineral tones.
  - `vec3 aInstanceScale`: Randomized non-uniform irregular rocky scales.
  - `vec3 aInstanceRotation`: Independent rotation speeds, tumbling axes, and phase offsets.
- Vertex shader dynamically resolves real-time circular orbit kinematics on the GPU.

### 4. Dynamic Comets & Solar Wind Tails
- Highly eccentric elliptical orbits (e.g., **1P/Halley** with $e = 0.967$ and $i = 18.0^\circ$, plus long-period comets).
- **Proximity-Adaptive Tails**: Tail length and brightness dynamically scale as the nucleus approaches perihelion according to solar radiation pressure:
  $$\vec{D}_{\text{tail}} = \text{normalize}(\mathbf{P}_{\text{comet}} - \mathbf{P}_{\text{Sun}})$$
- Tail quads dynamically orient and billboard away from the Sun, blending additively with depth masking disabled.

### 5. Visuals & Post-Processing Pipeline
- **Blinn-Phong Lighting Model**: Per-fragment shading evaluating ambient baseline, Lambertian diffuse, and specular highlights from the Sun's world-space position.
- **Multi-Pass HDR Bloom**:
  1. **Bright-Pass Filter**: Extracts high-luminance fragments ($L > 0.8$) into an off-screen floating-point FBO.
  2. **Separable Two-Pass Gaussian Blur**: Ping-pongs horizontal and vertical 5-tap Gaussian convolutions across downsampled textures for high performance.
  3. **Composite & Tone Mapping**: Additively blends the blurred bloom texture over the scene using Reinhard exposure tone-mapping:
     $$\mathbf{C}_{\text{final}} = \mathbf{1} - e^{-\mathbf{C}_{\text{HDR}} \cdot \gamma}$$
- **Atmospheric Fresnel Rim Scattering**: Two-pass outer shell for Earth with a custom shader calculating view-dependent rim glow:
  $$I_{\text{rim}} = (1.0 - \max(\vec{N} \cdot \vec{V}, 0.0))^3$$
- **Planetary Rings**: Double-sided, alpha-blended geometric discs for Saturn with authentic 26.7° axial tilt.
- **Interactive Rim Highlighting**: Focused bodies display a gentle additive Fresnel selection shell when manually targeted.

### 6. Camera System & Cinematic Tour
- **Spherical Orbit Camera**: Smooth yaw, pitch, and zoom distances centered on any celestial target or world origin.
- **Cinematic Tour Mode (`T`)**:
  - Automatically navigates through 15 curated stops: Sun $\rightarrow$ 4 Inner Terrestrial Planets $\rightarrow$ 6 Major Asteroids $\rightarrow$ 4 Gas & Ice Giants.
  - Dynamic sunlit hemisphere alignment: Computes optimal viewing yaw ($\theta_{\text{view}} = \text{atan2}(x, z) + 20^\circ$) to always frame planets from their illuminated side.
  - Smoothstep polynomial easing ($s(t) = 3t^2 - 2t^3$) over 2.0-second camera transitions followed by 4.8-second dwell times.
  - Synchronized educational fun facts overlay during dwell periods.

### 7. Interactive HUD & Observatory GUI
- **Observatory Telemetry**: Inspect exact world coordinates, visual radius, semi-major axis, orbital period, orbital velocity, and axial tilt.
- **Simulation Time Control**: Interactive speed multiplier slider ranging from $0.1\times$ to $10.0\times$ speed, with single-key pause/play (`Space`) and speed reset (`R`).
- **Visibility Layers**: Granular toggles for Planet Orbit Rings, Moon Orbit Paths, Asteroid Belts, Comet Tails, Atmosphere Shells, and Surface Textures.
- **Real-Time Performance Diagnostics (`F3`)**: Framerate (FPS), frame time (ms), active draw calls per frame, rendered triangle count, and camera coordinates.
- **Diagnostic Logging (`F9`)**: Automated 120-frame spatial transform CSV export for mathematical trajectory validation.

---

## 💻 Computer Graphics Techniques

| CG Principle | Project Implementation | Primary Source File |
| :--- | :--- | :--- |
| **Coordinate Spaces & Projections** | Model $\rightarrow$ World $\rightarrow$ View $\rightarrow$ Clip $\rightarrow$ NDC $\rightarrow$ Viewport pipeline using GLM matrices. | [`Camera.cpp`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/src/camera/Camera.cpp) |
| **Hierarchical Scene Graph** | Dynamic composition of parent-child affine transformations for multi-moon orbital dynamics. | [`SolarSystem.cpp`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/src/scene/SolarSystem.cpp) |
| **Blinn-Phong Shading** | Half-vector $\vec{H} = \frac{\vec{L} + \vec{V}}{\|\vec{L} + \vec{V}\|}$ per-fragment specular shading with material roughness parameters. | [`basic.frag`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/assets/shaders/basic.frag) |
| **Hardware Instancing** | `glDrawElementsInstanced` drawing 1,200+ distinct meshes in 1 draw call using vertex attribute divisors. | [`AsteroidBelt.cpp`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/src/scene/AsteroidBelt.cpp) |
| **HDR Bloom Post-Processing** | Dual-FBO ping-pong pipeline with bright extraction, two-pass separable Gaussian blur, and additive compositing. | [`Bloom.cpp`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/src/render/Bloom.cpp) |
| **Atmospheric Fresnel Glow** | Rim-brightening inverse dot-product shader rendering planetary exospheres. | [`atmosphere.frag`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/assets/shaders/atmosphere.frag) |
| **Alpha Blending & Sorting** | Controlled `glDepthMask(GL_FALSE)` pass with additive (`GL_SRC_ALPHA, GL_ONE`) and standard alpha blending. | [`main.cpp`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/src/main.cpp) |
| **Procedural UV Sphere Mesh** | Parametric generation of sphere vertices, normals, and equirectangular UV texture coordinates. | [`Sphere.cpp`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/src/render/Sphere.cpp) |
| **Trilinear Texture Filtering** | Equirectangular planetary texture projection with `GL_REPEAT` (U), `GL_CLAMP_TO_EDGE` (V), and `GL_LINEAR_MIPMAP_LINEAR`. | [`Texture.cpp`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/src/render/Texture.cpp) |
| **Camera Easing Functions** | Hermite cubic polynomial ($3t^2 - 2t^3$) smoothstep interpolation for cinematic transitions. | [`InputManager.cpp`](file:///c:/Users/Ankita/OneDrive/Desktop/cg-project/SolarSystemSimulator/src/input/InputManager.cpp) |

---

## 📂 Project Directory Structure

```
SolarSystemSimulator/
├── assets/
│   ├── shaders/
│   │   ├── basic.vert              # Opaque planet & asteroid vertex shader (instanced/standard)
│   │   ├── basic.frag              # Blinn-Phong lighting, textures & specular materials
│   │   ├── unlit.vert              # Unlit orbits, billboard comet tails & starfield vertex
│   │   ├── unlit.frag              # Color & alpha blending for unlit geometries
│   │   ├── atmosphere.vert         # Atmosphere shell vertex shader
│   │   ├── atmosphere.frag         # Fresnel rim scattering shader
│   │   ├── fullscreen.vert         # Full-screen quad vertex shader for post-processing
│   │   ├── bloom_brightness.frag   # High-luminance extraction filter
│   │   ├── bloom_blur.frag         # Two-pass horizontal/vertical Gaussian blur
│   │   └── bloom_composite.frag    # HDR bloom additive blend and tone-mapping
│   └── textures/
│       └── planets/                # 2048x1024 high-resolution equirectangular texture maps
│           ├── sun.jpg, mercury.jpg, venus.jpg, earth.jpg, mars.jpg
│           ├── jupiter.jpg, saturn.jpg, uranus.jpg, neptune.jpg, moon.jpg
│           ├── ceres.jpg, vesta.jpg, pallas.jpg, hygiea.jpg, eros.jpg, psyche.jpg
│           └── README.md
├── libs/
│   ├── glad/                       # Vendored OpenGL 3.3 Core Profile loader
│   │   ├── include/glad/glad.h
│   │   └── src/glad.c
│   └── stb/                        # Vendored stb_image image loading library
│       ├── stb_image.h
│       └── stb_image.cpp
├── src/
│   ├── main.cpp                    # Application entry point, main loop & render orchestration
│   ├── camera/
│   │   ├── Camera.h / .cpp         # Spherical orbit camera, view/projection matrix math
│   ├── core/
│   │   ├── Window.h / .cpp         # GLFW window wrapper & OpenGL context setup
│   │   └── Shader.h / .cpp         # GLSL shader program compilation, linking & uniform setters
│   ├── input/
│   │   ├── InputManager.h / .cpp   # Keyboard/mouse dispatch, tour state machine & smoothstep
│   ├── render/
│   │   ├── Bloom.h / .cpp          # Multi-pass HDR Bloom FBO & blur pipeline
│   │   ├── Mesh.h / .cpp           # VAO/VBO/EBO abstraction for geometric primitives
│   │   ├── RenderStats.h           # Draw call, triangle, and frame diagnostic trackers
│   │   ├── Sphere.h / .cpp         # Parametric UV sphere procedural geometry generator
│   │   └── Texture.h / .cpp        # 2D texture loader, trilinear mipmapping & GL binding
│   ├── scene/
│   │   ├── Asteroid.h              # Data structure for instanced asteroid parameters
│   │   ├── AsteroidBelt.h / .cpp   # Instanced asteroid belt generator and renderer
│   │   ├── CelestialBody.h / .cpp  # Planetary body data model and model matrix evaluator
│   │   └── SolarSystem.h / .cpp    # Ephemeris database, moon hierarchies, comets & tour stops
│   └── ui/
│       ├── UIOverlay.h / .cpp      # Dear ImGui HUD, telemetry panels, fun facts & diagnostic metrics
│   CMakeLists.txt                  # Build configuration & dependency FetchContent rules
└── README.md                       # Complete project documentation
```

---

## 🎮 Controls & Keybindings

### Mouse Navigation
| Action | Binding | Description |
| :--- | :--- | :--- |
| **Orbit Camera** | `Left Click + Drag` | Rotates the camera in spherical coordinates (Yaw & Pitch) around the focused body. |
| **Pan Camera** | `Right Click + Drag` | Panning translation of the target anchor point across the camera plane. |
| **Zoom View** | `Mouse Scroll Wheel` | Exponential distance zoom towards or away from the focused body (clamped). |

### Keyboard Shortcuts
| Key | Function | Detailed Behavior |
| :--- | :--- | :--- |
| **`1` – `8`** | **Focus Planet** | Center and track: 1=Mercury, 2=Venus, 3=Earth, 4=Mars, 5=Jupiter, 6=Saturn, 7=Uranus, 8=Neptune. |
| **`0`** | **Focus Sun** | Centers camera tracking directly onto the Sun at the coordinate origin $(0, 0, 0)$. |
| **`Esc`** | **Overview Mode** | Resets camera target to the solar system center and restores free overview framing. |
| **`Space`** | **Pause / Resume** | Freezes simulation time progression; camera orbiting remains active. |
| **`+` / `-`** | **Adjust Speed** | Multiplies simulation time progression by $1.25\times$ or $0.8\times$ (range $0.1\times$ to $10.0\times$). |
| **`R`** | **Reset Speed** | Restores simulation time multiplier back to normal $1.0\times$ speed. |
| **`T`** | **Cinematic Tour** | Toggles automated educational tour across the Sun, all planets, and major asteroids. |
| **`F3`** | **Metrics Overlay** | Toggles real-time performance telemetry (FPS, frame time, draw calls, triangles). |
| **`F9`** | **Export CSV Log** | Captures 120 consecutive frames of planetary transforms to `transform_log.csv`. |

---

## 📦 Prerequisites & Dependencies

Dependencies are managed automatically through **CMake `FetchContent`** — no manual library installation is required beyond a C++17 compiler and CMake:

| Dependency | Version | Purpose | Retrieval Method |
| :--- | :--- | :--- | :--- |
| **CMake** | $\ge 3.20$ | Build automation and dependency orchestration | System Pre-requisite |
| **C++ Compiler** | C++17 | MSVC 2022 (Windows), GCC $\ge 9$, or Clang $\ge 10$ | System Pre-requisite |
| **GLFW** | 3.4 | Window management and OpenGL 3.3 context creation | Automated `FetchContent` |
| **GLM** | 1.0.1 | Header-only mathematics library (vectors, matrices, quaternions) | Automated `FetchContent` |
| **Dear ImGui** | 1.90.1 | Immediate-mode graphical user interface | Automated `FetchContent` |
| **GLAD** | 3.3 Core | OpenGL 3.3 Core function pointer loader | Vendored in `libs/glad/` |
| **stb_image** | 2.29 | Lightweight image loading library | Vendored in `libs/stb/` |

---

## 🔨 Build & Run Instructions

### Windows (Visual Studio 2022)

Open a **Developer PowerShell for VS 2022** or standard terminal with CMake in PATH:

```powershell
# 1. Clone repository
git clone https://github.com/ankita01209/cg-project.git
cd cg-project

# 2. Configure project (downloads GLFW, GLM, ImGui automatically)
cmake -S SolarSystemSimulator -B build -G "Visual Studio 17 2022" -A x64

# 3. Compile in Release mode
cmake --build build --config Release

# 4. Launch executable
.\build\Release\SolarSystemSimulator.exe
```

### Windows (MinGW-w64 / MSYS2 / Ninja)

```powershell
# Configure using Ninja generator and GCC/Clang
cmake -S SolarSystemSimulator -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build

# Launch
.\build\SolarSystemSimulator.exe
```

### Linux / macOS

```bash
# Install system development dependencies (Ubuntu / Debian example)
sudo apt update
sudo apt install build-essential cmake git libx11-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev libgl1-mesa-dev

# Configure and compile
cmake -S SolarSystemSimulator -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Launch
./build/SolarSystemSimulator
```

> [!NOTE]
> `ASSETS_DIR` is defined at compile time by CMake as an absolute path to `SolarSystemSimulator/assets/`. You can execute `SolarSystemSimulator.exe` from any working directory without texture or shader path resolution errors.

---

## 🎨 Shader Pipeline Details

### 1. `basic.vert` & `basic.frag` (Planets, Moons, Major Asteroids)
- Computes world-space position `vFragPos`, normal vector `vNormal`, and texture coordinates `vTexCoord`.
- Supports instanced translation offsets and colors for the asteroid belt via `uIsAsteroid`.
- Blinn-Phong lighting:
  - Ambient component boosted to $0.28$ to ensure texture visibility in deep space.
  - Per-fragment diffuse scattering from the Sun's position.
  - Configurable specular highlight governed by `uSpecularStrength` and `uShininess` (e.g. high metallic sheen on 16 Psyche vs. matte absorption on 10 Hygiea).
  - Additive Fresnel rim highlight `pow(1.0 - dot(N, V), 3.0)` for currently targeted celestial bodies.

### 2. `atmosphere.vert` & `atmosphere.frag` (Atmospheric Glow)
- Renders an inflated sphere shell ($1.15\times$ planet radius) surrounding Earth.
- Computes viewing angle dot product between normal and camera view vector.
- Additive blend function `glBlendFunc(GL_SRC_ALPHA, GL_ONE)` creates an atmospheric scattering glow at the planet's limb.

### 3. `bloom_*.frag` (HDR Post-Processing Pipeline)
- **Brightness Extraction**: Isolates fragments exceeding luminance threshold ($0.2126R + 0.7152G + 0.0722B > 0.8$).
- **Two-Pass Gaussian Blur**: Separates 2D convolution into a horizontal pass followed by a vertical pass with 5 Gaussian weights:
  $$W = [0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216]$$
- **Additive Composite**: Combines the original scene buffer with the blurred bloom texture using exposure tone mapping.

---

## 📊 Educational Telemetry & Astronomical Data

All planetary parameters are curated based on NASA planetary fact sheets:

| Body | Visual Radius | Orbit Semi-Major Axis | Orbital Speed (rad/s) | Rotation Speed (rad/s) | Axial Tilt / Inclination | Key Feature |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Sun** | 1.65 wu | 0.0 wu | 0.00 | 0.20 | 7.25° | Emissive G-type main-sequence star |
| **Mercury** | 0.15 wu | 4.0 wu | 1.50 | 0.30 | 0.03° | Caloris basin; highest orbital eccentricity |
| **Venus** | 0.22 wu | 6.0 wu | 1.00 | -0.10 | 177.3° | Retrograde axial spin; runaway greenhouse effect |
| **Earth** | 0.24 wu | 8.5 wu | 0.80 | 1.50 | 23.44° | Liquid water oceans; nitrogen-oxygen atmosphere |
| **Mars** | 0.18 wu | 11.0 wu | 0.60 | 1.40 | 25.19° | Olympus Mons; Valles Marineris canyon system |
| **1 Ceres** | 0.085 wu | 12.4 wu | 0.52 | 1.10 | 10.6° | Largest asteroid belt body; water cryovolcanism |
| **4 Vesta** | 0.070 wu | 12.85 wu | 0.50 | 1.40 | 7.1° | 500 km Rheasilvia basin; basaltic protoplanet |
| **2 Pallas** | 0.065 wu | 13.30 wu | 0.48 | 1.20 | 34.8° | Steep orbital inclination; golf-ball surface |
| **10 Hygiea** | 0.060 wu | 13.75 wu | 0.45 | 0.90 | 3.8° | Extremely dark (albedo 0.07); spherical candidate |
| **433 Eros** | 0.045 wu | 10.40 wu | 0.65 | 1.80 | 10.8° | Peanut-shaped Mars-crosser; NEAR Shoemaker site |
| **16 Psyche** | 0.055 wu | 14.20 wu | 0.43 | 1.50 | 3.1° | Exposed nickel-iron core; NASA Psyche target |
| **Jupiter** | 0.70 wu | 15.0 wu | 0.40 | 3.00 | 3.13° | Great Red Spot storm; 95 natural moons |
| **Saturn** | 0.58 wu | 20.0 wu | 0.25 | 2.80 | 26.73° | Spectacular ring system (ice & rock fragments) |
| **Uranus** | 0.40 wu | 26.0 wu | 0.15 | -1.80 | 97.77° | Tilted sideways on orbital plane; methane ice |
| **Neptune** | 0.38 wu | 32.0 wu | 0.10 | 1.70 | 28.32° | Fastest supersonic winds exceeding 2,100 km/h |

---

## ❓ Troubleshooting & FAQ

<details>
<summary><b>1. Textures or shaders fail to load at runtime</b></summary>

Ensure your CMake configuration properly passes `ASSETS_DIR`. The project embeds the absolute assets path at compile time. If assets are moved after building, re-run `cmake --build build` to update the compile definitions.
</details>

<details>
<summary><b>2. Black screen or OpenGL 3.3 initialization error</b></summary>

Verify that your GPU drivers support **OpenGL 3.3 Core Profile** or higher. On laptops with dual GPUs (integrated Intel/AMD + dedicated NVIDIA/AMD), configure your graphics control panel to run the executable on the high-performance GPU.
</details>

<details>
<summary><b>3. CMake Generator error: "could not find any instance of Visual Studio"</b></summary>

If building on Windows without Visual Studio IDE installed, use the Ninja generator with MinGW:
```powershell
cmake -S SolarSystemSimulator -B build -G "Ninja"
```
Ensure `ninja` and `gcc`/`g++` are in your system `PATH`.
</details>

<details>
<summary><b>4. ImGui interface text is blurry or scaling issues</b></summary>

Dear ImGui automatically monitors the GLFW framebuffer size and scales coordinate inputs. If using high-DPI Windows display scaling (125%, 150%), the interface automatically adapts viewport bounds.
</details>

---

## 📄 License & Acknowledgments

- **License**: This project is licensed under the [MIT License](LICENSE).
- **Libraries**:
  - [GLFW](https://www.glfw.org/) (Windowing & input)
  - [GLAD](https://glad.dav1d.de/) (OpenGL 3.3 Core loader)
  - [GLM](https://github.com/g-truc/glm) (OpenGL Mathematics)
  - [Dear ImGui](https://github.com/ocornut/imgui) (Immediate mode GUI)
  - [stb_image](https://github.com/nothings/stb) (Public domain image loader)
- **Imagery & Ephemeris Data**:
  - Planetary textures and orbital specifications based on publicly released datasets from **NASA Jet Propulsion Laboratory (JPL)**, **NASA Dawn Mission**, **NEAR Shoemaker**, and the **European Southern Observatory (ESO / VLT SPHERE)**.