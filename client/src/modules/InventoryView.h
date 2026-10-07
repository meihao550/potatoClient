#pragma once
#include "Module.h"
#include <mutex>
#include <string>

// Shows the 36 inventory slots + off hand in an overlay window.
// Read on the game thread (onTick), drawn on the render thread (onRender) from a copy.
class InventoryView : public Module {
public:
    InventoryView() : Module("Inventory", "インベントリの中身を画面に表示", Category::Player, 0) {
        addSettings({ &m_cellWidth, &m_showOffhand });
    }
    void onTick(Actor& player) override;
    void onRender() override;
    void onDisable() override;

private:
    struct Slot {
        std::string name;   // "minecraft:" removed, empty = nothing
        int count = 0;
    };
    struct Snapshot {
        unsigned long long time = 0;   // GetTickCount64 when read
        Slot slots[36];
        Slot offhand;
        int selected = 0;
    };
    std::mutex m_mutex;
    Snapshot m_snap;

    FloatSetting m_cellWidth{ "cellWidth", "マスの大きさ", 64.0f, 40.0f, 120.0f, "%.0f px" };
    BoolSetting m_showOffhand{ "showOffhand", "オフハンドも表示", true,
                               "メニューを開いている間はウィンドウを動かせる / マスにカーソルで名前" };
};
