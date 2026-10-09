#include "Config.h"
#include "ModuleManager.h"
#include "core/Logger.h"
#include <Windows.h>
#include <ShlObj.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace fs = std::filesystem;
using nlohmann::json;

namespace {
    constexpr int kVersion = 1;

    // %LOCALAPPDATA%\PotatoClient\config.json (empty if the folder can't be found)
    fs::path configPath() {
        PWSTR folder = nullptr;
        fs::path path;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder)))
            path = fs::path(folder) / L"PotatoClient" / L"config.json";
        CoTaskMemFree(folder);
        return path;
    }

    // For the log, which is UTF-8 (paths can contain a Japanese user name)
    std::string utf8(const fs::path& path) {
        const std::wstring& w = path.native();
        const int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
        std::string out(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), out.data(), size, nullptr, nullptr);
        return out;
    }

    void apply(Module& module, const json& entry) {
        if (auto settings = entry.find("settings"); settings != entry.end() && settings->is_object()) {
            for (Setting* s : module.settings())
                if (auto value = settings->find(s->id()); value != settings->end()) s->load(*value);
        }
        if (auto extra = entry.find("extra"); extra != entry.end() && extra->is_object()) module.loadExtra(*extra);
        if (auto key = entry.find("key"); key != entry.end() && key->is_number_integer()) {
            const int vk = key->get<int>();
            if (vk >= 0 && vk <= 0xFF) module.setKey(vk);
        }
        // Last, so onEnable already sees the loaded settings
        if (auto enabled = entry.find("enabled"); enabled != entry.end() && enabled->is_boolean() && enabled->get<bool>())
            module.setEnabled(true);
    }
}

void Config::load() {
    const fs::path path = configPath();
    if (path.empty()) return;
    std::ifstream file(path);
    if (!file) {
        LOG("config: no file yet (%s)", utf8(path).c_str());
        return;
    }
    try {
        const json root = json::parse(file, nullptr, false);   // false = no exception on bad JSON
        const auto modules = root.is_object() ? root.find("modules") : root.end();
        if (root.is_discarded() || modules == root.end() || !modules->is_object()) {
            LOG("config: %s is not a valid config, using defaults", utf8(path).c_str());
            return;
        }
        for (auto& m : ModuleManager::modules())
            if (auto entry = modules->find(m->name()); entry != modules->end() && entry->is_object())
                apply(*m, *entry);
        LOG("config: loaded %s", utf8(path).c_str());
    } catch (const std::exception& e) {
        LOG("config: could not be read (%s), using defaults", e.what());
    }
}

void Config::save() {
    const fs::path path = configPath();
    if (path.empty()) return;
    try {
        json modules = json::object();
        for (auto& m : ModuleManager::modules()) {
            json settings = json::object();
            for (Setting* s : m->settings()) s->save(settings[s->id()]);
            json extra = json::object();
            m->saveExtra(extra);

            json entry = { { "enabled", m->isEnabled() }, { "key", m->key() }, { "settings", settings } };
            if (!extra.empty()) entry["extra"] = extra;
            modules[m->name()] = entry;
        }
        const json root = { { "version", kVersion }, { "modules", modules } };

        fs::create_directories(path.parent_path());
        const fs::path temp = fs::path(path).replace_extension(L".tmp");
        {
            std::ofstream file(temp, std::ios::trunc);
            if (!(file << root.dump(2))) {
                LOG("config: could not write %s", utf8(temp).c_str());
                return;
            }
        }
        fs::rename(temp, path);   // replaces the old file in one step
        LOG("config: saved %s", utf8(path).c_str());
    } catch (const std::exception& e) {
        LOG("config: could not be saved (%s)", e.what());
    }
}
