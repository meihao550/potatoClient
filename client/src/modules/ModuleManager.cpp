#include "ModuleManager.h"
#include "Fly.h"
#include "Speed.h"
#include "Xray.h"
#include "sdk/PlayerTick.h"
#include "core/Logger.h"
#include "Aimbox.h"
#include "InventoryView.h"

namespace {
    std::vector<std::unique_ptr<Module>> g_modules;

    void onClientTick(Actor& player) {
        for (auto& m : g_modules)
            if (m->isEnabled()) m->onTick(player);
    }
}

std::vector<std::unique_ptr<Module>>& ModuleManager::modules() { return g_modules; }

void ModuleManager::init() {
    g_modules.push_back(std::make_unique<Aimbox>());
    g_modules.push_back(std::make_unique<InventoryView>());
    g_modules.push_back(std::make_unique<Xray>());
    g_modules.push_back(std::make_unique<Speed>());
    g_modules.push_back(std::make_unique<Fly>());   // after Speed: while both are on, Fly wins
    for (auto& m : g_modules)
        LOG("module %-10s available=%d", m->name().c_str(), m->isAvailable());
    PlayerTick::setClientTickListener(&onClientTick);
}

void ModuleManager::shutdown() {
    PlayerTick::setClientTickListener(nullptr);
    for (auto& m : g_modules) m->setEnabled(false);
}

void ModuleManager::onKey(int vk) {
    for (auto& m : g_modules)
        if (m->key() == vk) m->toggle();
}

// レンダーの定義
void ModuleManager::render() {
    for (auto& m : g_modules)
        if (m->isEnabled()) m->onRender();
}