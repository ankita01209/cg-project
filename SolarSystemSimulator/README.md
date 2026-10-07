# Solar System Simulator

A modular C++17 / OpenGL 3.3 Core solar system renderer.  
Dependencies are fetched automatically by CMake — no manual installs needed beyond the tools listed below.

---

## Prerequisites (one-time machine setup)

| Tool | Where to get it |
|---|---|
| **CMake ≥ 3.20** | https://cmake.org/download/ |
| **Visual Studio 2022** (with "Desktop development with C++" workload) | https://visualstudio.microsoft.com/downloads/ |
| **Git** (so CMake FetchContent can clone GLFW/GLM) | https://git-scm.com/download/win |

> **MinGW / MSYS2 users:** Replace `-G "Visual Studio 17 2022" -A x64` with `-G "Ninja"` and ensure `ninja` and `gcc`/`clang` are on your PATH.

---

## Build (Windows, Visual Studio)

Open a **Developer PowerShell for VS 2022** (or any terminal that has `cmake` on PATH), then:

```powershell
# 1 — Configure (downloads GLFW & GLM automatically on first run)
cmake -S SolarSystemSimulator -B build -G "Visual Studio 17 2022" -A x64

# 2 — Build (Release)
cmake --build build --config Release

# 3 — Run
.\build\Release\SolarSystemSimulator.exe
```

**Expected output:**
```
[OK] OpenGL 3.3 loaded — scaffold build successful!
```
The program opens an 800×600 window, prints the message, and exits — this is intentional for Phase 1.

---

## Project structure

```
SolarSystemSimulator/
├── CMakeLists.txt          ← Build system entry point
├── README.md
├── assets/
│   └── shaders/            ← GLSL shaders (added in later phases)
├── libs/
│   └── glad/               ← Vendored GLAD (OpenGL 3.3 Core loader)
└── src/
    ├── main.cpp
    ├── core/               ← Engine core utilities
    ├── render/             ← Renderer classes
    ├── scene/              ← Scene graph / planet hierarchy
    ├── camera/             ← Camera & projection
    ├── input/              ← Keyboard / mouse handling
    └── ui/                 ← HUD / ImGui integration (future)
```
