#pragma once
// =============================================================================
// RenderStats.h  —  Lightweight per-frame renderer counters
//
// Usage (in main.cpp render loop):
//   1. stats.reset()                            — top of each frame
//   2. stats.recordDraw(mesh.indexCount())      — after glDrawElements
//   3. stats.recordInstancedDraw(n, indexCount) — inside AsteroidBelt::draw
//   4. ++stats.drawCalls                        — for raw glDrawArrays calls
//   5. UIOverlay::updateFrameStats(ms, stats)   — before renderPanel()
//
// Design: plain struct, no virtual functions, no heap allocations.
// =============================================================================

namespace render {

struct RenderStats {
    int drawCalls          = 0; ///< Total draw calls this frame
    int instancedDrawCalls = 0; ///< Subset that used instancing
    int totalInstances     = 0; ///< Total instances drawn across all instanced calls
    int triangleCount      = 0; ///< Total triangles submitted (estimate)

    void reset() noexcept {
        drawCalls = instancedDrawCalls = totalInstances = triangleCount = 0;
    }

    /// Record a non-instanced indexed draw call (glDrawElements / glDrawElementsBaseVertex).
    /// @param indexCount  Index count reported by Mesh::indexCount()
    void recordDraw(int indexCount) noexcept {
        ++drawCalls;
        triangleCount += indexCount / 3;
    }

    /// Record an instanced draw call (glDrawElementsInstanced).
    /// @param instanceCount  Number of instances rendered
    /// @param indexCount     Indices per instance
    void recordInstancedDraw(int instanceCount, int indexCount) noexcept {
        ++drawCalls;
        ++instancedDrawCalls;
        totalInstances += instanceCount;
        triangleCount  += (instanceCount * indexCount) / 3;
    }
};

} // namespace render
