#pragma once
// =============================================================================
// Sphere.h — Procedural UV-sphere geometry generator
//
// CG concept — Procedural geometry:
//   Instead of loading a model file, we compute sphere vertex positions from
//   the parametric sphere equations at runtime. Every planet, the Sun, and
//   the Moon will reuse this function with different radii.
//
// UV-sphere parameterisation:
//   Divide the sphere surface into a latitude/longitude grid (stacks × slices).
//   For each grid point (i, j):
//     phi   = i * π / stacks          (0 at north pole, π at south pole)
//     theta = j * 2π / slices
//     pos   = (sin(phi)*cos(theta), cos(phi), sin(phi)*sin(theta)) * radius
//     normal = pos / radius            (unit outward normal)
// =============================================================================

#include "render/Mesh.h"

namespace render {

/// Creates a UV-sphere mesh centred at the origin.
/// @param radius  Distance from origin to surface (default 1.0)
/// @param stacks  Number of horizontal latitude rings; higher = smoother poles
/// @param slices  Number of vertical longitude segments; higher = smoother silhouette
Mesh createSphere(float radius = 1.0f, int stacks = 32, int slices = 32);

} // namespace render
