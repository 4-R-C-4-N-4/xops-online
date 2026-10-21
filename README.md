# XOPS ONLINE

Browser multiplayer deathmatch built on OpenXOPS (BSD) via the OpenXOPS-web
port. Work in progress: see docs/ONLINE-PLAN.md and PROGRESS.md.
Play: https://4-r-c-4-n-4.github.io/xops-online/

Multiplayer (experimental): in the "Multiplayer deathmatch" panel, pick a map,
enter a name and press Host; send the room code (or Copy link) to a friend,
who enters it and presses Join. "Online (WebRTC)" connects browsers directly
(signaling over public Nostr relays, no server of ours); "Same-browser test"
connects two windows of one browser. Keep the host's window visible: browsers
pause hidden tabs. Players behind strict NATs may not connect yet (no TURN).

Unofficial fan project; not affiliated with the original X operations.

---

# OpenXOPS-web (unofficial fork)

A work-in-progress port of OpenXOPS 1.080 to the browser (WebAssembly) and Linux.
**The browser preview includes an original free-data starter mission.**
Click Start game with no file to play STARTER. The bundled pack is an early
CC0 prototype: generated models, map textures, font/HUD and placeholder skins.
An optional player-supplied `xops096.zip` adds the classic missions locally.
Japanese text uses the public-domain Shinonome font.

## Play the browser preview

**Live:** https://4-r-c-4-n-4.github.io/OpenXOPS-web/

Listed in the portfolio's Games section; opens in a new tab. Engine updates
build and deploy automatically from this repo. See [deployment details](docs/DEPLOYMENT.md).

See **[Running and browser quickstart](docs/RUNNING.md)** for controls, verified
behaviour and known limitations. This runs the game, not the triangle smoke test.

```sh
cd ~/Work/OpenXOPS-web
source ~/programs/emsdk/emsdk_env.sh
bash tools/build-playable-web.sh
python3 -m http.server 8766 --bind 127.0.0.1 --directory build-web
```

Open http://127.0.0.1:8766/ and click **Start game** without selecting a file.
Enter skips the opening; select **STARTER** and left-click its briefing to begin.
**UFO FIELD** is first in the main mission list. Survive
three alien waves in a fenced pasture with hay and static cow cover. Start with
an 8-round scoped LEVER 30-30 (64 damage); select empty slot 2 to loot dropped
laser weapons. Scope: right mouse in Modern, Left Shift in Classic. Kill all
20 aliens to win. Clean lever-rifle headshots kill every alien in one hit.
Full-height hay and standing-height cows block laser fire; four-second reload
breaks separate the waves. [Mission design and engine hooks](docs/MISSION-UFO-FIELD.md).
If you want classic content instead, select your original `xops096.zip` before
starting. A remembered classic ZIP is used until **Forget saved ZIP** removes
it. Player archives stay local and are not uploaded.

The preview uses the original OpenGL renderer via Emscripten legacy-GL
compatibility. The experimental shader rewrite is separate and does not block
this build. Native developer builds still use stub graphics.

- Architecture, sequence diagrams and where to extend: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- Goals: [docs/GOALS.md](docs/GOALS.md)
- Scope and plan: [docs/SCOPE-OF-WORK.md](docs/SCOPE-OF-WORK.md)
- Status: [PROGRESS.md](PROGRESS.md)
- Licensing and data: [NOTICE.md](NOTICE.md). No original X operations data is
  included. The default CC0 starter pack is playable but unfinished; it is not
  a replacement for every classic mission, skin, effect or sound.

`src/` holds UTF-8 source. Tag `upstream-1.080` preserves the original CP932
import. The historical upstream README follows; its Windows-only instructions
and statement that source is not on GitHub describe upstream, not this fork.

---

<img width="800" height="320" alt="Image" src="https://github.com/user-attachments/assets/a076a462-b7c9-4da3-8900-70d5d4a86b6a" />

# OpenXOPS

OpenXOPS is free, open-source, offline FPS(First-person shooter) game. 
It is based on [X operations](https://rucsgames.net/se/xops/), a Japanese freeware FPS released by Team MITEI in 2003.

OpenXOPS is primarily written in C/C++ and is developed for Windows.

## Official Website

https://openxops.net/en/

## Development environment

* Programming language : C/C++
* IDE : Microsoft Visual Studio 2019
* Add SDK : Microsoft DirectX SDK (June 2010)

Used technology...
* 3D/2D Graphics : DirectX or OpenGL
* Input : WindowsAPI or DirectInput
* Sound : ezds.dll or DirectSound
* File : C standard library (stdio.h)
* Physics engine : Original
* AI engine : Original

**Game engine is not used.**

## License

OpenXOPS is New BSD License.

## Source Code

OpenXOPS uses Subversion (SVN) as its official version control system.

**The source code is not hosted on GitHub.**
Please visit the official OpenXOPS website for the source code
and Subversion repository.

https://openxops.net/en/developer.php

---

OpenXOPSは、2003年にチームMITEIが発表した日本製フリーFPS「[X operations](https://rucsgames.net/se/xops/)」（本家XOPS）をもとに開発されている、無料のオープンソース・オフラインFPSです。

主にC/C++で開発され、Windowsで動作します。

## 公式Webサイト

https://openxops.net/

## 開発言語や環境など

* 開発言語 ： C/C++
* 開発環境 ： Microsoft Visual Studio 2019
* 使用SDK ： Microsoft DirectX SDK (June 2010)

一般的に「ゲームエンジン」と呼ばれるものは一切使用せず、全てフルスクラッチで書き上げています。
* 3D/2D描画関係 ： DirectX または OpenGL
* 入力関係 ： DirectInput または WindowsAPI
* サウンド関係 ： 本家XOPS付属の ezds.dll または DirectSound　（※）
* ファイル読み込み関係 ： C言語標準関数
* 物理エンジン ： 独自
* AI関係 ： 独自

## ライセンス

ライセンスは修正BSDライセンスを適用しています。

## ソースコード

OpenXOPSの公式ソースコード管理にはSubversion（SVN）を使用しています。

**GitHub上ではソースコードを公開していません。**
ソースコードおよびSubversionリポジトリについては、
OpenXOPS公式Webサイトをご覧ください。

https://openxops.net/developer.php
