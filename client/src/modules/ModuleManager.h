#pragma once
#include "Module.h"
#include <memory>
#include <vector>

namespace ModuleManager {
    void init();
    void shutdown();                 // disables every module
    void onKey(int vk);              // toggles modules bound to vk
    void render();                   // calls onRender of enabled modules(render thread)
    std::vector<std::unique_ptr<Module>>& modules();
}
