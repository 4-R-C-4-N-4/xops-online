//! @file gamemain.h
//! @brief ゲームメイン処理のヘッダー

//--------------------------------------------------------------------------------
// 
// OpenXOPS
// Copyright (c) 2014-2023, OpenXOPS Project / [-_-;](mikan) All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright notice, 
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice, 
//   this list of conditions and the following disclaimer in the documentation 
//   and/or other materials provided with the distribution.
// * Neither the name of the OpenXOPS Project nor the names of its contributors 
//   may be used to endorse or promote products derived from this software 
//   without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL OpenXOPS Project BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//--------------------------------------------------------------------------------

#ifndef GAMEMAIN_H
#define GAMEMAIN_H

#define MAINMENU_X (GameConfig.GetScreenWidth() - 360)	//!< メニューの表示 X座標（左上基準）
#define MAINMENU_Y (GameConfig.GetScreenHeight() - 340)	//!< メニューの表示 Y座標（〃）
#define TOTAL_MENUITEMS 8								//!< メニュー1画面に表示するミッション数
#define MAINMENU_H (TOTAL_MENUITEMS+2)*30 + 25			//!< メニューの表示サイズ・高さ
#define MAINMENU_OPTION_X 10							//!< メニューの表示 オプション項目リンクのX座標（左上基準）
#define MAINMENU_OPTION_Y (MAINMENU_Y+MAINMENU_H - 16)	//!< メニューの表示 オプション項目リンクのY座標（〃）

#define INPUT_ARROWKEYS_ANGLE DegreeToRadian(4)		//!< 方向キーでの回転角度
#define INPUT_F1NUMKEYS_ANGLE DegreeToRadian(2)		//!< 三人称視点でのテンキーの回転角度

#define HUDA_WEAPON_POSX (GameConfig.GetScreenWidth() - 255)	//!< 武器情報を描画する領域・X座標
#define HUDA_WEAPON_POSY (GameConfig.GetScreenHeight() - 98)	//!< 武器情報を描画する領域・Y座標
#define HUDA_WEAPON_SIZEW 8										//!< 武器情報を描画する領域・横サイズ（32ピクセルの配置個数）
#define HUDA_WEAPON_SIZEH 3										//!< 武器情報を描画する領域・縦サイズ（32ピクセルの配置個数）

#define VIEW_HEIGHT 19.0f							//!< 視点の高さ
#define VIEW_DIST 0.1f								//!< 中心から視点までの距離
#define VIEW_F1MODE_ANGLE DegreeToRadian(-22.5f)	//!< 三人称視点での視点角度（初期）
#define VIEW_F1MODE_DIST 14.0f						//!< 三人称視点での視点距離（最大）
#define VIEW_FREECAMERA_SCALE 1.5f					//!< フリーカメラでの移動速度（高速モードは×2）
#define VIEWANGLE_NORMAL DegreeToRadian(65)			//!< 標準的な視野角

#define TOTAL_EVENTLINE 3				//!< イベントのライン数
#define TOTAL_EVENTENTRYPOINT_0 -100	//!< ライン 0 の開始識別番号
#define TOTAL_EVENTENTRYPOINT_1 -110	//!< ライン 1 の開始識別番号
#define TOTAL_EVENTENTRYPOINT_2 -120	//!< ライン 2 の開始識別番号

#define TOTAL_EVENTENT_SHOWMESSEC 5.0f		//!< イベントメッセージを表示する秒数

#ifndef H_LAYERLEVEL
 #define H_LAYERLEVEL 3		//!< Select include file.
#endif
#include "main.h"
#include "humancommand.h"	//[XOPS-ONLINE]

#ifdef ENABLE_MENUOPTIONS
 #define OPTIONS_P1_W 340															//!< オプション画面 No.1　幅
 #define OPTIONS_P1_H (30*6)														//!< オプション画面 No.1　高さ
 #define OPTIONS_P1_X (GameConfig.GetScreenWidth() - 360)							//!< オプション画面 No.1　X座標
 #define OPTIONS_P1_Y (MAINMENU_Y+MAINMENU_H - OPTIONS_P1_H - 24)					//!< オプション画面 No.1　Y座標
 #define OPTIONS_P1_DATAS 3															//!< オプション画面 No.1　リンクテキストの数
 #define OPTIONS_P2_W 600															//!< オプション画面 No.2　幅
 #define OPTIONS_P2_H 360															//!< オプション画面 No.2　高さ
 #define OPTIONS_P2_X ((GameConfig.GetScreenWidth() - OPTIONS_P2_W)/2)				//!< オプション画面 No.2　X座標
 #define OPTIONS_P2_Y (105 + (GameConfig.GetScreenHeight()-105 - OPTIONS_P2_H)/2)	//!< オプション画面 No.2　Y座標
 #define OPTIONS_P2_DATAS 32														//!< オプション画面 No.2　リンクテキストの数
 #define OPTIONS_P3_W 600															//!< オプション画面 No.3　幅
 #define OPTIONS_P3_H 320															//!< オプション画面 No.3　高さ
 #define OPTIONS_P3_X ((GameConfig.GetScreenWidth() - OPTIONS_P3_W)/2)				//!< オプション画面 No.3　X座標
 #define OPTIONS_P3_Y (105 + (GameConfig.GetScreenHeight()-105 - OPTIONS_P3_H)/2)	//!< オプション画面 No.3　Y座標
 #define OPTIONS_P3_DATAS 1															//!< オプション画面 No.3　リンクテキストの数
#if (GRAPHIC_ENGINE == 0)||(GRAPHIC_ENGINE == 1)||(GRAPHIC_ENGINE == 9)	//[OpenXOPS-web] GLES: full licence text (the short list is an OpenGL 1.1 flicker workaround)
 #define OPTIONS_LICENSELINE 26											//!< ライセンス文の行数
#elif GRAPHIC_ENGINE == 8
 #define OPTIONS_LICENSELINE 6											//!< ライセンス文の行数
#endif	//GRAPHIC_ENGINE
#endif	//ENABLE_MENUOPTIONS

#ifdef ENABLE_DEBUGCONSOLE
 #define MAX_CONSOLELEN 78		//!< デバック用コンソールの文字数（行）
 #define MAX_CONSOLELINES 12	//!< デバック用コンソールの行数
 #define CONSOLE_PROMPT ">"		//!< デバック用コンソールのプロンプト
#endif

int InitGame(WindowControl *WindowCtrl, int mode, char *MIFpath);
void CleanupGame();

//! ゲームの状態を受け渡しする構造体
typedef struct
{
	int selectmission_id;	//!< 選択されたミッション
	char mifpath[MAX_PATH];	//!< MIFファイルのパス
	bool missioncomplete;	//!< ミッション完了
	unsigned int framecnt;	//!< フレーム数
	int fire;		//!< 射撃回数
	float ontarget;	//!< 命中数
	int kill;		//!< 倒した敵の数	
	int headshot;	//!< 敵の頭部に命中した数
} GameInfo;

#ifdef ENABLE_MENUOPTIONS
//! メニューリンクテキスト構造体
typedef struct
{
	bool enable;	//!< リンクテキストとしての有効化フラグ	
	int pos_x;		//!< X座標
	int pos_y;		//!< Y座標
	int text_w;		//!< 文字の幅
	int text_h;		//!< 文字の高さ
	int text_defaultcolor;		//!< 標準の文字色
	int text_cursorcolor;		//!< 選択時の文字色
	char text[32];				//!< テキスト
} MenuLinkTextData;
#endif	//ENABLE_MENUOPTIONS

#ifdef ENABLE_DEBUGCONSOLE
//! コンソールを管理する構造体
typedef struct
{
	int color;							//!< 色
	char textdata[MAX_CONSOLELEN+1];	//!< 文字列
} ConsoleData;
#endif

//! @brief オープニング画面管理クラス
//! @details オープニング画面を管理します。
class opening : public D3Dscene
{
	float add_camera_x;			//!< カメラ座標 Xの増減量
	float add_camera_y;			//!< カメラ座標 Yの増減量
	float add_camera_z;			//!< カメラ座標 Zの増減量
	float add_camera_rx;		//!< カメラ回転角度 RXの増減量
	float add_camera_ry;		//!< カメラ回転角度 RYの増減量
	//int opening_banner;		//!< オープニングで表示するテクスチャID
	void Render3D();
	void Render2D();

public:
	opening();
	~opening();
	int Create();
	int Recovery();
	void Input();
	void Process();
	void Destroy();
};

//! @brief メニュー画面管理クラス
//! @details メニュー画面を管理します。
class mainmenu : public D3Dscene
{
	char demopath[MAX_PATH];		//!< デモファイルのパス
	int modescreen;					//!< 表示画面
	int mainmenu_mouseX;			//!< メニュー画面マウスX座標
	int mainmenu_mouseY;			//!< メニュー画面マウスY座標
	int mainmenu_mouseY_old;		//!< メニュー画面マウスY座標（前回のフレーム）
	class ScrollbarObject scrollbar_official;	//!< 標準ミッションのスクロールバー
	class ScrollbarObject scrollbar_addon;		//!< addonのスクロールバー
	int gametitle;					//!< ゲームタイトル画像
#ifdef ENABLE_MENUOPTIONS
	MenuLinkTextData Options_p1LinkTextData[OPTIONS_P1_DATAS];		//!< オプション画面 No.1　リンクテキストデータ
	MenuLinkTextData Options_p2LinkTextData[OPTIONS_P2_DATAS];		//!< オプション画面 No.2　リンクテキストデータ
	MenuLinkTextData Options_p3LinkTextData[OPTIONS_P3_DATAS];		//!< オプション画面 No.3　リンクテキストデータ
	class ScrollbarObject scrollbar_license;		//!< ライセンス文のスクロールバー
	int Options_p2LinkTextID;						//!< オプション画面 No.2　押されたリンクテキストのID
	bool Options_FullscreenFlag_old;				//!< オプション画面 No.2　フルスクリーンフラグ
	int Options_KeyConfigID;						//!< オプション画面 No.2　キー設定ID
	int Options_KeyConfigCnt;						//!< オプション画面 No.2　キー設定カウント（フレーム数）
	const char *licenseinfo[OPTIONS_LICENSELINE];	//!< ライセンス文
	char *licenseinfobuf;							//!< 表示するライセンス文を保持するバッファー
#endif	//ENABLE_MENUOPTIONS

	void Render3D();
	void Render2D();
#ifdef ENABLE_MENUOPTIONS
	void CreateOptions();
	void InputOptions();
	void ProcessOptions();
	void RenderOptions();
#endif	//ENABLE_MENUOPTIONS

public:
	mainmenu();
	~mainmenu();
	int Create();
	int Recovery();
	void Input();
	void Process();
	void Destroy();
};

//! @brief ブリーフィング画面管理クラス
//! @details ブリーフィング画面を管理します。
class briefing : public D2Dscene
{
	bool TwoTexture;	//!< ブリーフィング画像を2枚使用
	int TextureA;		//!< ブリーフィング画像A
	int TextureB;		//!< ブリーフィング画像B
	void Render2D();

public:
	briefing();
	~briefing();
	int Create();
	int Recovery();
	void Destroy();
};

//! @brief メインゲーム画面管理クラス
//! @details メインゲーム画面を管理します。
class maingame : public D3Dscene
{
	//class EventControl Event[TOTAL_EVENTLINE];	//!< イベント制御クラス
	int SkyNumber;			//!< 背景空番号
	bool DarkScreenFlag;	//!< 画面を暗く
	bool FogFlag;			//!< フォグ有効
	float mouse_rx;				//!< マウスによる水平軸角度
	float mouse_ry;				//!< マウスによる垂直軸角度
	float view_rx;				//!< マウス角度とカメラ角度の差（水平軸）
	float view_ry;				//!< マウス角度とカメラ角度の差（垂直軸）
	float add_camera_rx;		//!< カメラ回転角度 RXの増減量
	float add_camera_ry;		//!< カメラ回転角度 RYの増減量
	bool ShowInfo_Debugmode;	//!< 座標などを表示するデバックモード
	bool Camera_Debugmode;		//!< カメラデバックモード
	bool tag;					//!< オブジェクトのタグを表示
	bool radar;					//!< 簡易レーダー表示
	bool wireframe;				//!< マップをワイヤーフレーム表示
	bool nomodel;				//!< モデル描画フラグ
	bool CenterLine;			//!< 3D空間に中心線を表示
	bool Camera_Blind;			//!< 目隠し描画
	bool Camera_F1mode;			//!< カメラF1モード
	int Camera_F2mode;			//!< カメラF2モード
	bool Camera_HOMEmode;		//!< カメラHOMEモード
	bool Cmd_F5;				//!< 裏技F5モード
	int InvincibleID;			//!< 無敵な人の判定
	bool PlayerAI;				//!< プレイヤー操作をAIに委ねる
	bool AIstop;				//!< AI処理を停止する
	bool AINoFight;				//!< AIが非戦闘化する（戦わない）
	int AIdebuginfoID;			//!< AIのデバック情報表示
	int start_framecnt;			//!< メインゲーム開始時のカウント
	int end_framecnt;			//!< メインゲーム終了のカウント
	bool EventStop;				//!< イベント処理を停止する
	bool EventSkip[TOTAL_EVENTLINE];	//!< イベント処理をスキップする
	int GameSpeed;				//!< ゲームの実行速度（1以上の整数値）
	int message_id;				//!< 表示中のイベントメッセージ番号
	int message_cnt;			//!< 表示中のイベントメッセージカウント
	bool redflash_flag;			//!< レッドフラッシュ描画フラグ
	int time;		//!< timer
	int time_input;					//!< 入力取得の処理時間
	int time_process_object;		//!< 基本オブジェクトの処理時間
	int time_process_ai;			//!< AIの処理時間
	int time_process_event;			//!< イベントの処理時間
	int time_sound;					//!< サウンドの処理時間
	int time_render;				//!< 描画の処理時間
	GameInfo MainGameInfo;			//!< リザルト用管理クラス
	bool CheckInputControl(int CheckKey, int mode);
	HumanCommand LastPlayerCommand;	//!< [XOPS-ONLINE] this tick's local command (sent to the host by clients)
	void InputPlayer(human *myHuman, int mouse_x, int mouse_y, float MouseSensitivity);
	void InputViewCamera(int mouse_x, int mouse_y, float MouseSensitivity);
	void Render3D();
	void Render2D();
	bool GetRadarPos(float in_x, float in_y, float in_z, int RadarPosX, int RadarPosY, int RadarSize, float RadarWorldR, int *out_x, int *out_y, float *local_y, bool check);
	void RenderRadar();

#ifdef ENABLE_DEBUGCONSOLE
	bool Show_Console;						//!< デバック用コンソールを表示
	ConsoleData *InfoConsoleData;			//!< デバック用コンソールデータ（表示済み）
	TextBoxObject ConsoleTextBoxControl;	//!< デバック用コンソールデータ（入力中）
	char NewCommand[MAX_CONSOLELEN];		//!< 新たに入力された未処理のコマンド
	int ScreenShot;		//!< SSを撮影する
	void AddInfoConsole(int color, const char *str);
	bool GetCommandNum(const char *cmd, int *num);
	void InputConsole();
	void ProcessConsole();
	void RenderConsole();
#endif

public:
	maingame();
	~maingame();
	void SetShowInfoFlag(bool flag);
	int GetGameSpeed();
	int Create();
	int Recovery();
	void Input();
	void Process();
	void Sound();
	void Destroy();
};

//! @brief リザルト画面管理クラス
//! @details リザルト（結果表示）画面を管理します。
class result : public D2Dscene
{
	void Render2D();

public:
	result();
	~result();
};

void InitScreen(WindowControl *WindowCtrl, opening *Opening, mainmenu *MainMenu, briefing *Briefing, maingame *MainGame, result *Result);
void ProcessScreen(WindowControl *WindowCtrl, opening *Opening, mainmenu *MainMenu, briefing *Briefing, maingame *MainGame, result *Result, unsigned int framecnt);
int ChangeWindowMode(WindowControl *WindowCtrl, D3DGraphics *d3dg, InputControl *inputCtrl, scene *RecoveryScene, bool fullscreen);

#endif