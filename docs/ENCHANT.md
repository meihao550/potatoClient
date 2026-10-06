# エンチャントの付け方 — LearnClient 説明書

手に持っている武器・防具・道具に、コマンドでエンチャントを付ける機能の説明書です。

> **自分のワールド（シングルプレイ）専用です。** ゲームに内蔵されたサーバーを直接操作する仕組みなので、他人のサーバーや Realms では動きません（「自分のワールド (シングルプレイ) でのみ使えます」と表示されます）。

---

## 1. 準備

1. マイクラを起動して、**自分のワールドに入る**
2. `python injector\injector.py` → **Inject**（くわしくは README の「インジェクト」）
3. コンソールに次の行が出ていれば準備 OK
   ```
   hook LocalPlayer::normalTick ... -> MH_OK
   hook ServerPlayer::normalTick ... -> MH_OK
   EnchantUtils::applyEnchant at exe+0x....
   ```
   `applyEnchant at exe+0` と出た場合はゲームのバージョンが違います（下の「困ったとき」）。

## 2. 基本の使い方

1. エンチャントしたいアイテム（剣、ツルハシ、防具など）を**手に持つ**
2. **Home** キーでコマンド欄を開く
3. コマンドを打って **Enter**
4. 左上に「○○ に付けました: ダメージ増加 5」のように結果が出る

閉じるときは **Esc**。

## 3. コマンド一覧

| コマンド | やること | 例 |
|---|---|---|
| `enchant <名前>` | そのエンチャントを**最大レベル**で付ける | `enchant sharpness` |
| `enchant <名前> <レベル>` | レベルを指定して付ける（1〜255） | `enchant efficiency 3` |
| `enchant all` | そのアイテムに合う**おすすめセット**をまとめて最大レベルで付ける | `enchant all` |
| `enchant list` | 使える名前の一覧を表示 | `enchant list` |

- 名前は英語の ID（`/enchant` コマンドと同じ）。大文字小文字はどちらでも OK
- 日本語名（`ダメージ増加` など）も使えますが、コマンド欄では日本語入力がうまくいかないことがあるので英語がおすすめ
- 先頭の `.` は付けても付けなくても OK（`.enchant all` でも動きます）

## 4. おすすめの付け方

### 剣
```
enchant all
```
→ 修繕・耐久力・ダメージ増加・ドロップ増加・火属性・ノックバック

個別に付けるなら:
```
enchant sharpness
enchant looting
enchant fire_aspect
enchant unbreaking
enchant mending
```

### ツルハシ・シャベル・斧
```
enchant all
```
→ 修繕・耐久力・効率強化・幸運

**シルクタッチ**が欲しいときは `all` を使わずに個別で（幸運とシルクタッチは両立しません）:
```
enchant silk_touch
enchant efficiency
enchant unbreaking
enchant mending
```

### 防具（ヘルメット・チェストプレート・レギンス・ブーツ）
1 つずつ手に持って `enchant all`。部位に合うものだけが付きます。
- ヘルメット: ダメージ軽減・棘の鎧・水中呼吸・水中採掘
- ブーツ: ダメージ軽減・落下耐性・水中歩行・ソウルスピード
- レギンス: ダメージ軽減・スニーク速度上昇

### 弓
```
enchant all
```
→ 射撃ダメージ増加・パンチ・フレイム・修繕・耐久力（修繕と無限は両立しないので修繕を優先）

無限にしたいときは:
```
enchant infinity
enchant power
enchant punch
enchant flame
```

### トライデント
`enchant all` → 忠誠・水生特効・召雷。激流が欲しいときは `enchant riptide`（忠誠・召雷とは両立しません）。

### クロスボウ
`enchant all` → 拡散・高速装填。貫通が欲しいときは `enchant piercing`（拡散とは両立しません）。

### メイス
`enchant all` → 密度・防具貫通・風爆。

## 5. 名前の一覧

| 名前 | 日本語 | 最大 | 付くもの |
|---|---|---|---|
| `protection` | ダメージ軽減 | 4 | 防具 |
| `fire_protection` | 火炎耐性 | 4 | 防具 |
| `feather_falling` | 落下耐性 | 4 | ブーツ |
| `blast_protection` | 爆発耐性 | 4 | 防具 |
| `projectile_protection` | 飛び道具耐性 | 4 | 防具 |
| `thorns` | 棘の鎧 | 3 | 防具 |
| `respiration` | 水中呼吸 | 3 | ヘルメット |
| `depth_strider` | 水中歩行 | 3 | ブーツ |
| `aqua_affinity` | 水中採掘 | 1 | ヘルメット |
| `sharpness` | ダメージ増加 | 5 | 剣・斧 |
| `smite` | アンデッド特効 | 5 | 剣・斧 |
| `bane_of_arthropods` | 虫特効 | 5 | 剣・斧 |
| `knockback` | ノックバック | 2 | 剣 |
| `fire_aspect` | 火属性 | 2 | 剣 |
| `looting` | ドロップ増加 | 3 | 剣 |
| `efficiency` | 効率強化 | 5 | 道具 |
| `silk_touch` | シルクタッチ | 1 | 道具 |
| `unbreaking` | 耐久力 | 3 | ほぼ全部 |
| `fortune` | 幸運 | 3 | 道具 |
| `power` | 射撃ダメージ増加 | 5 | 弓 |
| `punch` | パンチ | 2 | 弓 |
| `flame` | フレイム | 1 | 弓 |
| `infinity` | 無限 | 1 | 弓 |
| `luck_of_the_sea` | 宝釣り | 3 | 釣り竿 |
| `lure` | 入れ食い | 3 | 釣り竿 |
| `frost_walker` | 氷渡り | 2 | ブーツ |
| `mending` | 修繕 | 1 | ほぼ全部 |
| `binding` | 束縛の呪い | 1 | 防具 |
| `vanishing` | 消滅の呪い | 1 | ほぼ全部 |
| `impaling` | 水生特効 | 5 | トライデント |
| `riptide` | 激流 | 3 | トライデント |
| `loyalty` | 忠誠 | 3 | トライデント |
| `channeling` | 召雷 | 1 | トライデント |
| `multishot` | 拡散 | 1 | クロスボウ |
| `piercing` | 貫通 | 4 | クロスボウ |
| `quick_charge` | 高速装填 | 3 | クロスボウ |
| `soul_speed` | ソウルスピード | 3 | ブーツ |
| `swift_sneak` | スニーク速度上昇 | 3 | レギンス |
| `wind_burst` | 風爆 | 3 | メイス |
| `density` | 密度 | 5 | メイス |
| `breach` | 防具貫通 | 4 | メイス |
| `lunge` | 突進 | 3 | 槍 |

`enchant all` は呪い（束縛・消滅）を付けません。

## 6. 付かないときは（ルール）

エンチャントはゲーム本来のルール（`/enchant` コマンドと同じチェック）で付けています。次の場合は「このアイテムには付けられません」と出ます。

- **アイテムの種類が合わない** — 例: ブーツに `sharpness`、ブロックや食べ物に何か
- **両立しないエンチャントがもう付いている** — 例:
  - `sharpness` / `smite` / `bane_of_arthropods`（どれか 1 つ）
  - `fortune` と `silk_touch`
  - `mending` と `infinity`
  - `protection` 系 4 種（どれか 1 つ）
  - `depth_strider` と `frost_walker`
  - `loyalty` と `riptide`、`channeling` と `riptide`
  - `multishot` と `piercing`
- 何も持っていない → 「エンチャントしたい武器・防具を手に持ってください」

一度付けたエンチャントを外すコマンドはありません。外したいときは砥石を使ってください。

### 最大レベルより上（例: `enchant sharpness 10`）
1〜255 まで指定できますが、ゲーム側のチェックで**付かない／最大レベルにされる可能性があります**（まだ試していません）。うまくいったかは結果のメッセージとアイテムの説明で確認してください。

## 7. 困ったとき

| 症状 | 対処 |
|---|---|
| 「ワールドに入ってから使ってください」 | ポーズ中は使えません。ゲームに戻ってから実行 |
| 「自分のワールド (シングルプレイ) でのみ使えます」 | サーバー・Realms では使えません |
| 「不明なエンチャント」 | つづりを確認。`enchant list` で一覧 |
| 何も起きない／メッセージが出ない | `build\Release\client.log` を確認。`applyEnchant at exe+0` ならシグネチャが合っていません |
| 付いたのに表示が変わらない | ホットバーで一度別のスロットに切り替えて戻す。ワールドに入り直すと確実 |
| ゲームを更新したら動かない | `client/src/sdk/Offsets.h` の `Sig::applyEnchant` を探し直す（下の「仕組み」） |

## 8. 仕組み（学習用）

- エンチャントはアイテムの **NBT**（`ItemStack::mUserData` の中の `ench` リスト）に入っています。自分で NBT を組み立てるのは大変なので、**ゲームの `/enchant` が使っている関数をそのまま呼びます**:
  ```cpp
  bool EnchantUtils::applyEnchant(ItemStackBase& item, EnchantmentInstance const& enchant, bool allowNonVanilla);
  // EnchantmentInstance = { uint8 種類ID; int レベル; }  (8 バイト)
  ```
- **関数の見つけ方**: exe の中から文字列 `"commands.enchant.success"` を探す → それを使っているコード = `EnchantCommand::execute` → その中で「手持ちアイテムをコピー → チェック → 付ける → 手に戻す」の順に関数を呼んでいるので、付ける関数を特定 → 関数の先頭バイトをシグネチャにする (`Offsets::Sig::applyEnchant`)
- **サーバーとクライアントの両方に付ける**: dupe と同じで、本物のインベントリはサーバー側（保存される）、クライアント側は表示用。片方だけだと表示がずれる
- 実装: `client/src/sdk/Enchant.cpp`（ID 表と関数呼び出し）、`client/src/commands/EnchantCommand.cpp`（コマンド）
