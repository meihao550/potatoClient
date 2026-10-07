# PotatoClient — Minecraft Bedrock クライアント

Horion 風の内部 (DLL) クライアントを一から作って、チートクライアントの仕組みを学ぶためのプロジェクトです。
対象: **Minecraft Bedrock 1.26.52 (GDK 版 / `Minecraft.Windows.exe`)**

> シングルプレイ / 自分のワールド専用で使ってください。公開サーバーや Realms で使うと規約違反・BAN の対象です。
> マルチプレイでも使える機能はありますが、許可されているサーバーでやるようにしましょう。

## How to use

### 1. 必要なもの
- Windows 10 / 11 (x64)
- Minecraft Bedrock **1.26.52**（GDK 版。Microsoft Store / Xbox アプリから入れたもの）
- Visual Studio 2019 Build Tools（「C++ によるデスクトップ開発」をインストール）
- Python 3（64bit 版）
- git（ビルド時に MinHook / Dear ImGui を自動ダウンロードするため）

### 2. ビルド
プロジェクトのフォルダ（`C:\minefolder`）でコマンドを実行します。
```
set CMAKE="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
%CMAKE% -S . -B build -G "Visual Studio 16 2019" -A x64
%CMAKE% --build build --config Release
```
`build\Release\client.dll` ができれば成功です（`client.pdb` はクラッシュ解析用）。
2 回目以降はソースを変えたら `--build` の行だけ実行すれば OK です。

#### Docker でビルドする（任意）
Visual Studio を入れずに DLL だけ作りたいときは、同梱の `Dockerfile` が使えます。Docker を **Windows コンテナ** モードにしておく必要があります（ゲームへの注入はコンテナではできません。ビルド専用です）。
```
docker build -t potatoclient-build .
docker create --name potatoclient-out potatoclient-build
docker cp potatoclient-out:C:\out .\out
docker rm potatoclient-out
```
`out\client.dll` ができます。Windows 10 では `--build-arg WINDOWS_VERSION=ltsc2019` を付けるか `--isolation=hyperv` でビルドしてください。

### 3. インジェクト
1. マイクラを起動し、**シングルプレイのワールドに入る**（Xray のブロック一覧はワールドに入ってから作られます）
2. 別のウィンドウで次を実行
   ```
   python injector\injector.py
   ```
3. 開いたウィンドウの **Inject** ボタンを押す
   - DLL のパスは `build\Release\client.dll` が初期値です。別の DLL を使うときは「参照...」で選びます
4. 「Inject 成功!」と表示され、ゲームの横にコンソール窓が開いて `Injected!` が出れば成功

### 4. 操作
| キー | 動作 |
|---|---|
| Insert | メニュー表示 / 非表示（表示中は視点・移動・クリックがゲームに伝わらない） |
| X（メニューで変更可） | Xray ON/OFF |
| F（メニューで変更可） | Fly ON/OFF |
| Home | コマンド欄を開く（`up` と打って Enter で実行、Esc で閉じる） |
| End | アンロード（ゲームは続行、もう一度 Inject できる） |

メニューの見方:
- **チェックボックス**: モジュールの ON/OFF
- **「キー: X」ボタン**: 押してから好きなキーを押すとキー割り当てを変更。Esc を押すと割り当てなし
- **「設定」**: 開くとモジュールごとの詳細設定が出る

### 5. Xray の使い方
1. Insert でメニューを開き、Xray にチェック（または X キー）
2. 「設定」を開いて表示したいものを選ぶ
   - **表示する鉱石**: ダイヤ、鉄、金などをそれぞれ ON/OFF。「全部ON」「全部OFF」ボタンもあります
   - **追加ブロック**: 名前の一部を入力して「追加」（例: `amethyst` → アメジスト関連が見える）。`x` で削除
   - **光を通す**: ON だと地下の鉱石が暗くならない
3. **設定 → ビデオ → 「スムーズライティング」を一度切り替えて戻す**と、すべてのチャンクに反映されます
   - 描画距離を変えるだけだと、新しく範囲に入ったチャンクしか作り直されず「一部だけ透明」になります
4. OFF にしたときも、同じようにスムーズライティングを切り替えると元の見た目に戻ります

### 6. UP コマンド（地上に出る）
1. ワールド内で **Home** を押すと左上にコマンド欄が出る
2. `up` と打って **Enter**（`.up` や `UP` でも OK）
3. 今いる X/Z の真上で一番高いブロックの上にテレポートし、結果が左上に数秒表示される
   - 水面と葉っぱも「地面」として扱います（海の上なら水面、木の下なら木の上）
   - ネザーでは岩盤の天井が一番上なので、天井の上に出ます
   - `help` でコマンド一覧、Esc で閉じる

### 6.5 移動系（マルチサーバーでも動く）
シングルでは内蔵サーバーの ServerPlayer を `/tp` と同じ経路で動かし、**リモートのサーバーでは LocalPlayer の位置・速度を直接書き換えます**（クライアントは毎 tick 自分の位置をサーバーに送るので、移動チェックの無いサーバーならそのまま通る）。

| コマンド | 動作 |
|---|---|
| `up` | 真上の一番高いブロックの上へ（マルチでも OK） |
| `vclip 10` / `vclip -5` | 真上 / 真下に n ブロック移動（壁抜け） |
| `hclip 5` | 向いている方向に n ブロック移動 |
| `tp 100 64 -20` / `tp ~ ~20 ~` | 座標へ移動（足元の座標。`~` で相対） |

| モジュール | 動作 |
|---|---|
| Fly (F) | WASD で移動、Space で上昇、Shift で下降、何も押さないとその場で停止。「設定」で速さ・アンチキック |
| Speed | WASD を押している間、横の速さを設定値にする |

- キーは GetAsyncKeyState で直接読むので **WASD / Space / Shift 固定**（ゲームのキー設定は見ない）。メニュー・コマンド欄を開いている間とゲームが前面にない間は無視
- Fly を切ると普通に落ちて落下ダメージを受けます。降りてから切る
- サーバーに移動チェック（アンチチート）があると、大きなテレポートや速すぎる移動は引き戻されます。そのときは速さを下げる / 距離を短くして刻む

`dupe` — 手に持っているアイテムを最大数 (64 / 16) まで増やす。`dupe 10` なら 10 個にする
- 道具・防具などスタックできないアイテムは増やせません
- UP と同じく自分のワールド専用です

`enchant` — 手に持っている武器・防具にエンチャント（`enchant sharpness` / `enchant all` / `enchant list`）
- **マルチサーバーでも使える（OP 権限が必要）**: サーバーへ `/enchant @s <名前> <レベル>` をチャットと同じ方法で送る。手持ちのアイテムに付けられるものだけを送るので、`enchant all` でもエラーがあふれない。レベルはサーバーの `/enchant` に合わせて最大レベルまで
- くわしい使い方は **[docs/ENCHANT.md](docs/ENCHANT.md)（エンチャントの付け方 説明書）**

### 6.6 インベントリ系
| モジュール | 動作 |
|---|---|
| Inventory | インベントリ 36 マスとオフハンドを画面右上に表示。選択中のホットバーは黄色。メニューを開いている間はウィンドウを動かせて、マスにカーソルを当てると名前と個数が出る（マルチでも OK） |
| AutoTotem | オフハンドに不死のトーテムがなく、インベントリにあれば自動でオフハンドへ移す。オフハンドに別のものがあれば空きマスへ退避（「設定」で OFF にできる）。**シングルプレイ専用** |

### 7. アンロードと再ビルド
- ゲーム内で **End** を押すとアンロードされ、コンソール窓が閉じます（Xray が ON なら自動で元に戻します）
- インジェクターは DLL のコピー（`client_loaded_<時刻>.dll`）を読み込ませます。なので、**注入中でもビルドできます**。古いコピーは次の Inject のときに自動で削除されます
- 新しいビルドを試すときは End でアンロード → もう一度 Inject

### 8. トラブルシューティング
| 症状 | 対処 |
|---|---|
| 「Minecraft.Windows.exe が起動していません」 | 先にマイクラを起動する |
| 「LoadLibraryW が失敗しました」 | DLL のパスを確認。Release でビルドしたか、64bit か確認 |
| Insert でメニューが出ない | コンソールに `renderer api = D3D12` が出ているか確認。出ていなければワールドを少し動かして描画させる |
| Xray を ON にしてもチェックが入らない | ワールドに入ってから ON にする（コンソールに「ブロック一覧が見つかりません」と出る） |
| Xray が一部のチャンクにしか効かない | スムーズライティングを一度切り替える |
| `up` で「ワールドに入ってから使ってください」 | ポーズ中はプレイヤーの tick が止まるので、ゲームに戻ってから実行。ログに `hook LocalPlayer::normalTick` と `hook ServerPlayer::normalTick` が `MH_OK` で出ているか確認。サーバー (Realms 等) では使えない |
| ゲームがクラッシュした | `build\Release\client.log` の最後の行と、イベントビューアー →「Windows ログ」→「Application」のエラー（`client.dll` の障害オフセット）を確認。`client.pdb` でソースの行と対応づけられます |
| ゲームを更新したら動かない | オフセットが変わっています。下の「ゲームが更新されたら」を参照 |

ログ: `build\Release\client.log`（コンソール窓にも同じ内容が出ます）

## 仕組み

### 1. インジェクション — `injector/injector.py`
`OpenProcess` → `VirtualAllocEx` → `WriteProcessMemory`(DLL パス) → `CreateRemoteThread(LoadLibraryW)`。
ゲーム自身に `LoadLibraryW` を呼ばせて DLL を読み込ませる古典的な手法です。
DLL は毎回タイムスタンプ付きのコピーを注入するので、注入中でも再ビルドできます。

### 2. DLL のエントリ — `client/src/dllmain.cpp`
`DllMain` はローダーロック中なので何もせず、スレッドを起動してそこで初期化します。

### 3. 描画フック (ImGui オーバーレイ) — `client/src/render/`
自前でダミーの D3D12 デバイス/スワップチェーンを作り、COM の vtable から
`IDXGISwapChain::Present`(8) / `ResizeBuffers`(13) / `ID3D12CommandQueue::ExecuteCommandLists`(10) のアドレスを取って
MinHook でフックします。ゲームが毎フレーム Present するたびに ImGui を描きます。

### 4. 入力 — `client/src/gui/Input.cpp`, `GameInputHook.cpp`
- WndProc をサブクラス化してキー入力を拾う（Insert / End / キーバインド）
- この GDK 版はマウス・キーボードを **GameInput v3** で読んでいるので、
  `IGameInputReading::GetMouseState`(14) / `GetKeyState`(13) をフック。メニュー表示中は視点位置を凍結し、キーを「押されていない」にする

### 5. Xray — `client/src/modules/Xray.cpp`, `client/src/sdk/`
関数フックではなく **ゲームのデータを書き換える** 方式です。
- ブロックの種類ごとに `BlockType` オブジェクトがあり、`BlockTypeRegistry` の `std::map` に入っている
- レジストリはシグネチャを使わず、exe の `.data` を走査して「`minecraft:` で始まる名前を持つ std::map」を検証して見つける (`BlockRegistry.cpp`)
- `minecraft:barrier` は元々「見えない・隣の面を隠さない」ブロックなので、表示しないブロックを全部 barrier と同じ値にする
  - `BlockType`: `mRenderLayer=Barrier(14)`, `mIsOpaqueFullBlock=false`, `mTranslucency=1`, `mLightBlock=0`
  - 各 `Block`(状態): キャッシュされた `mIsOpaqueFullBlock`, `mLight`, 遮蔽形状 (`occlusionShapes`) を 0 に
- 元の値は 1 回だけ保存して、OFF で正確に戻す

### 6. UP コマンド — `client/src/commands/`, `client/src/sdk/PlayerTick.cpp`, `Actor.h`
この exe にはクラス名 (RTTI) がほぼ無く、プレイヤーを指す固定のポインタもないので、**コードを手がかり**にします。
- **LocalPlayer の vtable**: LocalPlayer のデストラクタは `lea rax,[vftable]` → `mov [rcx],rax` で vtable を書き込む。この命令列をシグネチャにして、`lea` の rel32 から vtable のアドレスを得る (`Offsets::Sig`)
- **ゲームスレッドに乗る**: vtable の 24 番 = `normalTick` を MinHook でフック。毎 tick `this`（= 今のプレイヤー）が手に入り、しかもゲームのスレッドで動ける。コマンド欄（描画スレッド）からは `PlayerTick::run()` で処理を予約する
- **動かすのはサーバー側のプレイヤー**: シングルプレイでも同じプロセス内でサーバーが動いていて、本当の位置はサーバーが持っている。クライアントの LocalPlayer だけ動かすとサーバーに戻される（最初の版はこれで失敗した）。なので ServerPlayer（コンストラクタの vtable 代入をシグネチャに）の normalTick もフックし、サーバーのスレッドで ServerPlayer を `/tp` と同じように動かす。サーバーがクライアントに移動を伝える
- **一番上のブロックを調べる**: `Actor+0x1C8` → Dimension、`+0xF0` → BlockSource（そのディメンションの「世界」）。BlockSource の仮想関数 `getAboveTopSolidBlock(x, z, 水, 葉)` が「一番上の固体ブロックの 1 つ上の Y」を返す
  - MSVC はオーバーロードした仮想関数を**宣言と逆順**に並べるので、ヘッダーの順番から 1 つずれる (`getBlock` や `getAboveTopSolidBlock`)。逆アセンブルで引数の使い方を見て確認した
- **移動**: ServerPlayer の仮想関数 21 番 `teleportTo(pos, ...)`（/tp と同じ経路）。座標は目の高さ（足元 + 1.62）なので、今の「目の高さ − 足元」を足して渡す
- コマンドは `Command` を継承して `CommandManager::init()` に登録する（モジュールと同じ形）

### 6.5 移動系 — `sdk/Actor.h` (`moveFeetTo`), `modules/Fly.cpp`, `Speed.cpp`, `commands/MoveCommands.cpp`
- `Actor+0x228` → ActorRotationComponent（pitch, yaw, 前回の pitch, yaw）。yaw 0 = +Z 向き、-90 = +X 向き
- **テレポート**: `PlayerTick::runOnSelf` が「位置の持ち主」を選ぶ。内蔵サーバーが動いていれば ServerPlayer の `teleportTo`、なければ LocalPlayer の `StateVector.pos/posPrev` と AABB を同じだけずらす
- **Fly / Speed**: `StateVector.posDelta`（速度、ブロック/tick）。ゲームは tick ごとに「posDelta だけ動く → 重力と空気抵抗をかける」ので、LocalPlayer の normalTick の**直後**に上書きすると次の tick はその値ちょうどで動く。モジュールは `Module::onTick` を実装すれば毎 tick 呼ばれる

### 7. DUPE コマンド — `client/src/commands/DupeCommand.cpp`
- インベントリの中身は `ItemStack` (0x98 バイト) の配列で、個数は `mCount` (+0x22) の 1 バイト
- Actor の仮想関数 77 番 `getCarriedItem()` は「選択中のスロットの ItemStack への参照」を返す（中身: `Actor+0x5B8` の PlayerInventory → 選択スロット番号 (+0x10) → コンテナ (+0xB8) の `getItem(slot)`）。参照なのでそこに書けば本物のスタックが変わる
- 最大スタック数は ItemStack → `WeakPtr<Item>` (+0x08) → Item の `mMaxStackSize` (+0xA8)
- **サーバーとクライアントの両方に同じ値を書く**。サーバー側が本物（保存・設置に使われる）、クライアント側は表示用。サーバーは「変わっていない」と思っているスロットを送り直さないので、片方だけだと表示がずれる

### 8. ENCHANT コマンド — `client/src/sdk/Enchant.cpp`, `commands/EnchantCommand.cpp`
- 文字列 `"commands.enchant.success"` → それを使う `EnchantCommand::execute` → 中で呼ばれる `EnchantUtils::applyEnchant(ItemStackBase&, EnchantmentInstance const&, bool)` を特定し、関数の先頭をシグネチャに
- NBT を自分で作らずゲームの関数に任せるので、付けられる/付けられないのルールも `/enchant` と同じ。サーバー・クライアント両方の手持ちに同じ処理をする（dupe と同じ理由）
- **リモートサーバー**: `sdk/CommandSender.cpp`。文字列 `"CommandRequestPacket"` → `getName` → vtable → それを書き込むコンストラクタと「チャットのコマンドを送る関数」を特定。その関数と同じく、ペイロード（コマンド文字列 / 送信元 = Player / バージョン 0x34）からパケットを作り、`Actor+0x1D8` の Level → 仮想関数 328 `getPacketSender` → 仮想関数 2 `send` で送る。権限チェックはサーバーがするので OP が必要
  - パケットはコマンド文字列のバッファを引き取るが、それは DLL 側の CRT で確保したもの。ゲームに解放させないよう、送ったあとに取り返してからパケットを破棄する
- 説明書: [docs/ENCHANT.md](docs/ENCHANT.md)

### 9. インベントリ / AutoTotem — `client/src/sdk/PlayerItems.cpp`, `modules/InventoryView.cpp`, `modules/AutoTotem.cpp`
- **インベントリ**: `Actor+0x5B8` の PlayerInventory → `+0xB8` の Inventory（Container、36 スロット: 0..8 ホットバー、9..35 その他）。`getCarriedItem`(77) と同じ経路。Container の仮想関数は `getItem`(7) / `setItem`(12) / `removeItem`(14) / `getContainerSize`(20)
- **オフハンド**: インベントリではなく、`ActorEquipment::getHandContainer(EntityContext&)` が返す 2 スロットの「手のコンテナ」の 1 番。この関数にはシグネチャがないので、`Actor::getEquippedTotem`(79) の先頭の `mov rsi,rcx / add rcx,8 / call ...` から呼び先を実行時に読む（`Actor+8` が EntityContext）
- **AutoTotem**: クライアント側のコピーで「オフハンドにトーテムがない & インベントリにある」を判定してから、サーバースレッドで ServerPlayer に対して `setItem`（オフハンドの物を退避）→ `setOffhandSlot`(78) → `removeItem` を呼ぶ。ゲームの関数で動かすので、dupe のような手動の同期はいらない

## ゲームが更新されたら (リバースエンジニアリングの手順)
オフセットは `client/src/sdk/Offsets.h` に集約してあります。
1. ゲームを起動してワールドに入る
2. `python tools\dump_image.py dump\Minecraft.Windows.dump.exe` — 暗号化された exe をメモリから復元して Ghidra/IDA で開ける形にする
3. [LeviLamina](https://github.com/LiteLDev/LeviLamina) の `src/mc/world/level/block/BlockType.h`, `Block.h` で構造体レイアウトを確認（BDS 用だがクライアントとほぼ同じ）
4. 実メモリを読んで、名前などの既知の値がそのオフセットにあるか確かめてから `Offsets.h` を更新
5. UP コマンド用: `LocalPlayer` / `ServerPlayer` の vtable シグネチャがまだ 1 か所だけに一致するか、vtable 番号 (`VIndex`) が合っているかを逆アセンブルで確認
   - プレイヤーは「当たり判定 0.6×1.8 の AABBShapeComponent」をメモリから探し、それを指す Actor (+0x220) から見つけられる

## 構成
```
injector/injector.py          ボタン 1 つの Inject
tools/dump_image.py           実行中のゲームから exe をダンプ
client/src/
  dllmain.cpp                 初期化スレッド / アンロード
  core/                       ログ, パターンスキャン, MinHook ラッパ
  render/                     Present フック, D3D12 / D3D11 の ImGui 描画
  gui/                        メニュー, WndProc, 入力フック
  modules/                    Module 基底, ModuleManager, Xray, Fly, Speed, Aimbox, Inventory, AutoTotem
  commands/                   Command 基底, CommandManager, up / vclip / hclip / tp / dupe / enchant / help
  sdk/                        ゲームの構造体 (オフセット, BlockType, Actor, レジストリ探索, プレイヤーの tick フック)
```
新しい機能は `Module` を継承して `ModuleManager::init()` に登録すればメニューに出ます。
新しいコマンドは `Command` を継承して `CommandManager::init()` に登録すればコマンド欄で使えます。
