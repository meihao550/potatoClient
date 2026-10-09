#pragma once

/*
 * Config file
 * -----------
 * %LOCALAPPDATA%\PotatoClient\config.json (the exact path is written to client.log).
 * For every module: on/off, key bind, its registered Settings and any extra state:
 *
 *   { "version": 1,
 *     "modules": { "Fly": { "enabled": false, "key": 70, "settings": { "speed": 1.0, ... } }, ... } }
 *
 * Modules are matched by name and settings by id, so adding a module or a setting keeps
 * old files working. Anything missing, unknown or of the wrong type keeps its default.
 */
namespace Config {
    void load();   // applies the file to ModuleManager::modules(); a missing / broken file is ignored
    void save();   // writes the current state (via a temp file, so a crash never leaves half a file)
}
