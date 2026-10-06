#include "ActorList.h"
#include "core/Logger.h"
#include <Windows.h>

// ==========================================================
// このファイルの仕事:
//   ゲームから「ワールドにいるエンティティ全員のリスト」をもらってくる
// ==========================================================

namespace {   // ← この中のものはこのファイルの中だけで使う、という意味

    // ---------------------------------------------------------
    // ゲームが返してくるリストの形
    // std::vector の中身は、実はこの 3 つの住所だけ
    // ---------------------------------------------------------
    struct GameList {
        Actor** first;   // リストの先頭の住所
        Actor** last;    // 最後の要素の「次」の住所
        Actor** end;     // 借りたメモリの終わり(今回は使わない)
    };

    // ---------------------------------------------------------
    // 「ゲームの図書館の返却窓口」= ucrtbase.dll の free 関数を探す
    // (この DLL は静的 CRT なので、自分の free で返すとクラッシュする)
    // ---------------------------------------------------------
    using FreeFunction = void (*)(void* memory);   // 「住所を 1 つ受け取って何も返さない関数」に名前を付けた

    FreeFunction findGameFree() {
        HMODULE ucrt = GetModuleHandleW(L"ucrtbase.dll");   // ゲームが使っている ucrtbase.dll を探す
        if (ucrt == nullptr) return nullptr;                 // 見つからなかった
        void* address = GetProcAddress(ucrt, "free");        // その中の "free" という関数の住所を探す
        return reinterpret_cast<FreeFunction>(address);      // 「ただの住所」を「関数」として扱う
    }

    // ---------------------------------------------------------
    // 番号 (VIndex) が合っているかの確認用ログ (最初の 1 回だけ出す)
    // ---------------------------------------------------------
    void logFunctionAddress(void* level) {
        void** vtable = *static_cast<void***>(level);   // Level の「仮想関数の一覧表」
        void* function = vtable[Offsets::Level::VIndex::getRuntimeActorList];
        uintptr_t base = Memory::moduleBase();
        LOG("Level vtable = exe+%#llx", (unsigned long long)((uintptr_t)vtable - base));
        LOG("getRuntimeActorList = exe+%#llx", (unsigned long long)((uintptr_t)function - base));
    }

    void logResult(const std::vector<Actor*>& list, Actor& player) {
        bool foundMyself = false;
        for (Actor* actor : list) {
            if (actor == &player) foundMyself = true;
        }
        LOG("actor list: %zu actors, contains LocalPlayer = %d", list.size(), foundMyself);
    }
}

std::vector<Actor*> ActorList::get(Actor& player) {
    std::vector<Actor*> result;   // 空のリスト。最後にこれを返す

    // (1) プレイヤーから Level (ワールド全体の管理人) を取り出す
    void* level = player.at<void*>(Offsets::Actor::level);
    if (level == nullptr) return result;

    // (2) 返却窓口を探す。static = 最初の 1 回だけ探して、あとは覚えておく
    static FreeFunction gameFree = findGameFree();
    if (gameFree == nullptr) return result;

    static bool firstTime = true;
    if (firstTime) logFunctionAddress(level);

    // (3) 空の箱を用意して、ゲームの関数に「ここにリストを入れて」と頼む
    //     (値で返す関数は、箱の住所が this の次の隠し引数になる)
    GameList gameList{};
    Memory::callVirtual<GameList*, GameList*>(level, Offsets::Level::VIndex::getRuntimeActorList, &gameList);

    // (4) 中身を 1 体ずつ自分のリストにコピーする
    size_t count = gameList.last - gameList.first;   // 何体いるか
    for (size_t i = 0; i < count; i++) {
        result.push_back(gameList.first[i]);          // push_back = Python の append
    }

    // (5) ゲームから借りたリストは、ゲームの窓口に返す
    if (gameList.first != nullptr) gameFree(gameList.first);

    if (firstTime) {
        logResult(result, player);
        firstTime = false;
    }
    return result;
}
