#include "Aimbox.h"
#include "core/Util.h"
#include "sdk/ActorList.h"
#include <Windows.h>
#include <imgui.h>
#include <cmath>

/*
 * Aimbox
 * ------
 * World -> screen without the game's matrices: build a camera from our own eye
 * position + yaw/pitch, split (point - eye) into right/up/forward, divide by the
 * forward distance (perspective) and scale by the FOV.
 */

namespace {
    using Util::kDegToRad;

    Vec3 sub(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    Vec3 cross(Vec3 a, Vec3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

    struct Camera {
        Vec3 eye, forward, right, up;
        float focal;     // (screen height / 2) / tan(fov / 2)
        ImVec2 center;
    };

    // yaw 0 = +Z, -90 = +X / positive pitch = looking down
    Camera makeCamera(Vec3 eye, float yawDeg, float pitchDeg, float fovDeg, ImVec2 screen) {
        const float yaw = yawDeg * kDegToRad, pitch = pitchDeg * kDegToRad;
        Camera c;
        c.eye = eye;
        c.forward = { -std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch) };
        c.right = { -std::cos(yaw), 0.0f, -std::sin(yaw) };
        c.up = cross(c.right, c.forward);
        c.focal = (screen.y * 0.5f) / std::tan(fovDeg * 0.5f * kDegToRad);
        c.center = { screen.x * 0.5f, screen.y * 0.5f };
        return c;
    }

    // false = behind the camera
    bool worldToScreen(const Camera& c, Vec3 p, ImVec2& out) {
        const Vec3 d = sub(p, c.eye);
        const float z = dot(d, c.forward);
        if (z < 0.05f) return false;
        out.x = c.center.x + dot(d, c.right) / z * c.focal;
        out.y = c.center.y - dot(d, c.up) / z * c.focal;
        return true;
    }
}

void Aimbox::onTick(Actor& player) {
    const auto me = player.refs();
    if (!me) return;

    Snapshot snap;
    snap.time = GetTickCount64();
    snap.eye = me->state.pos;
    snap.yaw = me->rotation.yaw;
    snap.pitch = me->rotation.pitch;

    const float range = m_range;
    ActorList::get(player, m_actors);
    for (Actor* a : m_actors) {
        if (!a || a == &player) continue;
        AABBShapeComponent* shape = a->aabbShape();
        if (!shape) continue;
        if (m_skipSmall && shape->width < 0.3f) continue;
        const Vec3 d = sub(shape->aabb.min, me->state.pos);
        if (dot(d, d) > range * range) continue;
        snap.boxes.push_back(shape->aabb);
    }

    std::lock_guard lock(m_mutex);
    m_snap = std::move(snap);
}

void Aimbox::onRender() {
    Snapshot snap;
    {
        std::lock_guard lock(m_mutex);
        snap = m_snap;
    }
    if (GetTickCount64() - snap.time > 500) return;   // paused / left the world

    const Camera cam = makeCamera(snap.eye, snap.yaw, snap.pitch, m_fov, ImGui::GetIO().DisplaySize);
    const ImU32 color = m_color.packed();
    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    static const int edges[12][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },   // bottom
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },   // top
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },   // sides
    };
    for (const AABB& b : snap.boxes) {
        const Vec3 corners[8] = {
            { b.min.x, b.min.y, b.min.z }, { b.max.x, b.min.y, b.min.z },
            { b.max.x, b.min.y, b.max.z }, { b.min.x, b.min.y, b.max.z },
            { b.min.x, b.max.y, b.min.z }, { b.max.x, b.max.y, b.min.z },
            { b.max.x, b.max.y, b.max.z }, { b.min.x, b.max.y, b.max.z },
        };
        ImVec2 s[8];
        bool visible = true;
        for (int i = 0; i < 8; i++) visible &= worldToScreen(cam, corners[i], s[i]);
        if (!visible) continue;   // simple version: skip boxes partly behind us
        for (auto& e : edges) draw->AddLine(s[e[0]], s[e[1]], color, 1.5f);
    }
}

void Aimbox::onDisable() {
    std::lock_guard lock(m_mutex);
    m_snap = {};
}
