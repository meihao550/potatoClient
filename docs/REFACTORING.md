# リファクタリングのまとめ（dev ブランチ）

> English summary: what the `dev` branch changed compared to `main`, why, and what to test in game.
> Every commit builds with MSVC (VS 2026) at `/W4` with no warnings; CI builds every push.

`main` との差分を、レビューしやすいように目的ごとにまとめたものです。コミットは 1 つの変更ごとに分けてあるので、気になるところはコミット単位で読めます（`git log main..dev`）。

## 1. ビルドと配布

| 変更 | 理由 |
|---|---|
| GitHub Actions の CI（`.github/workflows/build.yml`） | push のたびに MSVC でビルドを確かめる。成果物（`client.dll` / `injector.exe`）もダウンロードできる |
| `Memory.cpp` に `<string>` を追加 | 新しい MSVC では他のヘッダー経由で入らなくなり、ビルドが通らなかった |
| C++ 版インジェクタ `injector.exe` | Python がない環境でも注入できるように。Python 版も残している |
| README のビルド手順を A（Visual Studio）/ B（Docker）/ C（ビルドしない）に整理 | VS 2019 前提の手順では、2022 / 2026 の人がビルドできなかった |
| 警告レベル `/W3` → `/W4` | 出た 2 件を直し、今は警告 0 |
| nlohmann/json v3.12.0 を追加 | 設定ファイル用。3.11 系は CMake 4 で configure できない |

## 2. 安全性（落ちない・壊さない）

| 変更 | 場所 | 何が起きていたか |
|---|---|---|
| 初期化の結果を確認 | `dllmain.cpp` | 描画フックが失敗すると WndProc が入らず、End が効かないまま永久にアンロードできなかった |
| 共有フラグを atomic に | `core/InputFocus.h`, `Module.h`, `Menu.cpp`, `Backends.h` | メニューの開閉・モジュールの ON/OFF などを複数スレッドがロックなしで読み書きしていた |
| Xray の設定画面をロック内で描画 | `Xray.cpp` | `m_status`（std::string）を別スレッドが書き換え中に読むと落ちる可能性があった |
| `InFlight::Guard` とアンロード時の待機 | `core/InFlight.h`, `Hooks.cpp` | `Sleep(200)` で「たぶん終わった」と見なしていた。フック内にスレッドが残ったまま DLL を解放すると落ちる |
| WndProc を外す前に確認 | `gui/Input.cpp` | 後から別のオーバーレイ（Discord など）が差し込んでいると、そのつながりを壊していた。外せないときは DLL を残す |
| WndProc の元の関数を先に保存 | `gui/Input.cpp` | 差し替え直後に来たメッセージが null を呼ぶ可能性があった |
| コンソールの × と Ctrl+C を無効化 | `core/Logger.cpp` | 閉じるとゲームごと終了していた |
| オフハンド用関数の探索を排他に | `sdk/PlayerItems.cpp` | 2 つのスレッドが同時に探すと、片方が null を受け取っていた。失敗すると二度と探さなかった |
| ItemStack / インベントリの読み取りを安全に | `sdk/Actor.h`, `PlayerItems.cpp` | 空のスロットで `name()` を呼ぶと null を読んでいた |
| `findSig` を exe の範囲内に | `core/Memory.cpp` | 最後の領域で exe の外まで読んでいた。複数一致も警告するようにした |
| 描画初期化の途中失敗を片付ける | `render/*Backend.cpp`, `Renderer.cpp` | 失敗すると作りかけのリソースが解放されていなかった |

## 3. 構造（他のクライアントで一般的な形に）

| 変更 | 内容 |
|---|---|
| 型付きフック `Hooks::create<Fn>` | detour と元の関数ポインタの型が合わないとコンパイルエラーになる。`reinterpret_cast` が 7 か所消えた |
| `GameObject` 基底 | 4 か所にコピーされていた `at<T>()` を 1 つに |
| `Actor::refs()` | 位置・当たり判定・向きをまとめて取る。null チェックの重複が 8 か所から消えた |
| `Memory::scanOrLog` / `rva` | シグネチャ探索とログの書式を統一 |
| 直書きの数値を `Offsets.h` へ | `hit + 0x41`、オフハンド探索のバイト列、Xray のビット |
| `Setting`（bool / float / 色） | モジュールの設定をメンバーとして宣言し、メニュー描画と保存を共通化。値は atomic |
| `Config`（JSON） | ON/OFF・キー・設定を `%LOCALAPPDATA%\PotatoClient\config.json` に保存。壊れたファイルは無視 |
| カテゴリとタブ | メニューを「移動 / 表示 / プレイヤー」に分けた |
| tick の優先度 | 「Fly を Speed の後ろに登録する」という順番の工夫を、`setTickPriority(1)` に置き換えた |
| コマンドの `usage` と `help <名前>` | 使い方を各コマンドが持つ。`help` は 1 行 1 コマンド |
| `Args` / `Require` | `atoi`（"abc" が 0 になる）をやめ、前提条件の確認とメッセージを共通化 |
| `runOnServerThenClient` | dupe と enchant の「サーバー → クライアント」の反映を共通化 |
| `render/Overlay` | D3D11 / D3D12 で重複していた ImGui のフレーム処理を共通化 |
| 名前の統一 | 旧名 LearnClient を PotatoClient に |

## 4. 動作が変わったところ

- 描画フックが失敗したときは、待たずにすぐアンロードする。
- アンロード時、フックの終了を最大 3 秒待つ。終わらないときや WndProc を外せないときは、DLL を解放せずに残る（ゲームを再起動すると消える）。
- 設定が保存され、次の注入で戻る。ON だったモジュールは自動で ON になる。
- `dupe abc` や `enchant sharpness x` のような数字でない引数は、0 として扱わず使い方を表示する。
- `enchant` の関数が見つからないときは、「付けられません」ではなく「見つかりません」と表示する。
- tick フックが入らなかったときは、Fly / Speed / Aimbox / Inventory / AutoTotem をメニューで無効表示にする。

## 5. ゲームで確認してほしいこと

CI と手元のビルドでは、コンパイルが通ることまでしか確かめられていません。

1. 注入 → Insert でメニュー → 各タブの各モジュールを ON/OFF → End → もう一度注入。ゲームが落ちないこと
2. 設定（Fly の速さなど）とキー割り当てを変えてメニューを閉じ、End → 再注入。戻っていること
3. `config.json` を壊す（中身を `{` だけにする）→ 注入。デフォルトで起動すること
4. `help` / `help tp` / `tp ~ ~10 ~` / `vclip 5` / `up` / `dupe` / `dupe abc` / `enchant all`
5. Xray を ON にして設定を何度か変える → OFF。ブロックが元に戻ること
6. Discord のオーバーレイなどが動いている状態で End（WndProc を外せない場合の動き）

## 6. やっていないこと

- **表示文字列を `gui/Text.h` に集める**: 約 100 個の文字列を全ファイルで書き換える必要がある。英語 UI を作るときにまとめて行う方がよい。
- **MinHook のバージョン固定**: 今も `master` を取得している。CI で不具合が出たら固定する。
- **ゲームの実機での動作確認**: 上の 5. をお願いします。
