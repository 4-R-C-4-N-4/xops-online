//! @file gamemain.cpp
//! @brief ゲームメイン処理

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
// ※メニュー内のライセンス文章は mainmenu::CreateOptions() 内で定義されている。

#include "gamemain.h"
#include "net/session.h"	//[XOPS-ONLINE]
extern xnet::Session g_netSession;	//[XOPS-ONLINE] net/exports.cpp
#include "platform/vfs.h"

D3DGraphics d3dg;			//!< 描画クラス
InputControl inputCtrl;		//!< 入力取得クラス
SoundControl SoundCtrl;		//!< サウンド再生クラス

//! ゲームの設定値
ParameterInfo GameParamInfo;

//! リソース管理
ResourceManager Resource;

SoundManager GameSound;		//!< ゲーム効果音再生クラス

BlockDataInterface BlockData;		//!< ブロックデータ管理クラス
PointDataInterface PointData;		//!< ポイントデータ管理クラス
MIFInterface MIFdata;				//!< MIFコントロール
AddonList GameAddon;				//!< addonのリスト
Collision CollD;					//!< 当たり判定管理クラス
ObjectManager ObjMgr;				//!< オブジェクト管理クラス
AIcontrol HumanAI[MAX_HUMAN];		//!< AI管理クラス

GameInfo GameInfoData;				//!< ゲームの状態

EventControl Event[TOTAL_EVENTLINE];	//!< イベント制御クラス


//! @brief 基本的な初期化処理
int InitGame(WindowControl *WindowCtrl, int mode, char *MIFpath)
{
	if( WindowCtrl == NULL ){ return 1; }

	//D3DGraphicsクラス初期化
	if( d3dg.InitD3D(WindowCtrl, "data\\char.dds", GameConfig.GetFullscreenFlag(), GameConfig.GetLanguage()) ){
		WindowCtrl->ErrorInfo("Direct3Dの作成に失敗しました");
		return 1;
	}

	//InputControlクラス初期化
	if( inputCtrl.InitInput(WindowCtrl) ){
		WindowCtrl->ErrorInfo("Input initialization error");
		return 1;
	}

	//SoundControlクラス初期化
	if( SoundCtrl.InitSound(WindowCtrl) ){
		WindowCtrl->ErrorInfo("dll open failed");
		return 1;
	}

	//設定値を初期化
	GameParamInfo.InitInfo();

	//リソースの初期設定
	Resource.SetParameterInfo(&GameParamInfo);
	Resource.SetD3DGraphics(&d3dg);
	Resource.SetSoundControl(&SoundCtrl);

	//リソースを初期化
	Resource.LoadHumanModel();
	Resource.LoadWeaponModelTexture();
	Resource.LoadWeaponSound();
	Resource.LoadSmallObjectModelTexture();
	Resource.LoadSmallObjectSound();
	Resource.LoadScopeTexture();
	Resource.LoadBulletModelTexture();
	Resource.LoadBulletSound();
	Resource.LoadEffectTexture();

	//入力処理設定
	inputCtrl.SetMouseLimitFlag( GameConfig.GetMouseLimitFlag() );

	//効果音初期化
	float volume;
	if( GameConfig.GetSoundFlag() == false ){
		volume = 0.0f;
	}
	else{
#ifdef ENABLE_ADDCONFIG
		volume = GameConfig.GetVolume();
#else
		volume = 1.0f;
#endif
	}
	GameSound.SetClass(&SoundCtrl, &Resource, &GameParamInfo);
	SoundCtrl.SetVolume(volume);

	//オブジェクトマネージャー初期化
	ObjMgr.SetClass(&GameParamInfo, &d3dg, &Resource, &BlockData, &PointData, &CollD, &GameSound, &MIFdata);

	if( (mode != 0)&&(MIFpath != NULL) ){
		//※ミッションを直接起動するならば、addonリストは不要であるため作成しない。

		//起動するミッション設定
		GameInfoData.selectmission_id = -1;
		strcpy(GameInfoData.mifpath, MIFpath);
		GameState.SetStateMode(mode);
		if( mode == 2 ){
			//mifファイルを読み込み
			if( MIFdata.LoadFiledata(MIFpath, true) != 0 ){
				WindowCtrl->ErrorInfo("briefing data open failed");
				return 1;
			}
		}
	}
	else{
		//addonリスト作成
		char str[16];
		sprintf(str, "%s\\", ADDONDIR);
		GameAddon.LoadFiledata(str);

		GameInfoData.selectmission_id = 0;
		strcpy(GameInfoData.mifpath, "");
	}

	return 0;
}

//! @brief リソースをリセットする
//! @return 成功：0　失敗：-1　待機：1
//! @attention 通常は、描画処理に失敗した場合に限り呼び出してください。
int ResetGame(WindowControl *WindowCtrl)
{
	if( WindowCtrl == NULL ){ return -1; }

	//リストを正しく解放するため、予め呼ぶ。
	Resource.CleanupHumanTexture();

	int ReturnCode = d3dg.ResetD3D(WindowCtrl);

	if( ReturnCode == 0 ){
		//リソースを初期化
		Resource.LoadHumanModel();
		Resource.LoadWeaponModelTexture();
		Resource.LoadSmallObjectModelTexture();
		Resource.LoadScopeTexture();
		Resource.LoadBulletModelTexture();
		Resource.LoadEffectTexture();

		//WindowCtrl->ErrorInfo("Recovery...");
		return 0;
	}
	if( ReturnCode == 1 ){
		return 1;
	}
	//if( ReturnCode == 2 ){
		//WindowCtrl->ErrorInfo("Resetに失敗しました");
		//WindowCtrl->CloseWindow();
		return -1;
	//}
}

//! @brief 基本的な解放処理
void CleanupGame()
{
	//リソースを解放
	Resource.DestroyResource();

	//設定値を解放
	GameParamInfo.DestroyInfo();


	//SoundControlクラス解放
	SoundCtrl.DestroySound();

	//InputControlクラス解放
	inputCtrl.DestroyInput();

	//D3DGraphicsクラス解放
	d3dg.DestroyD3D();
}

//! @brief コンストラクタ
opening::opening()
{
	add_camera_x = 0.0f;
	add_camera_y = 0.0f;
	add_camera_z = 0.0f;
	add_camera_rx = 0.0f;
	add_camera_ry = 0.0f;
}

//! @brief ディストラクタ
opening::~opening()
{}

int opening::Create()
{
	int blockflag, pointflag;

	//ブロックデータ読み込み
	blockflag = BlockData.LoadFiledata("data\\map10\\temp.bd1");

	//ポイントデータ読み込み
	pointflag = PointData.LoadFiledata("data\\map10\\op.pd1");

	//ファイル読み込みでエラーがあったら中断
	if( (blockflag != 0)||(pointflag != 0) ){
		//2bit : point data open failed
		//1bit : block data open failed
		return pointflag*2 + blockflag;
	}

	//ブロックデータ初期化
	BlockData.CalculationBlockdata(false);
	d3dg->LoadMapdata(&BlockData, "data\\map10\\");
	CollD.InitCollision(&BlockData);

	//ポイントデータ初期化
	ObjMgr.LoadPointData();
	ObjMgr.SetPlayerID(MAX_HUMAN-1);	//実在しない人をプレイヤーに（銃声のサウンド再生対策）

	//追加の当たり判定設定
	ObjMgr.SetAddCollisionFlag(false);

	//AI設定
	for(int i=0; i<MAX_HUMAN; i++){
		HumanAI[i].SetClass(&ObjMgr, i, &BlockData, &PointData, &GameParamInfo, &CollD, GameSound);
		HumanAI[i].Init();
	}

	//背景空読み込み
	Resource.LoadSkyModelTexture(1);

	//opening_banner = d3dg->LoadTexture("banner.png", true, false);

	//サウンド初期化
	GameSound->InitWorldSound();

	//マウスカーソルを中央へ移動
	inputCtrl->MoveMouseCenter();
	framecnt = 0;

	camera_x = -5.0f;
	camera_y = 58.0f;
	camera_z = 29.0f;
	camera_rx = DegreeToRadian(206);
	camera_ry = DegreeToRadian(12);
	add_camera_x = 0.0f;
	add_camera_y = 0.0f;
	add_camera_z = 0.0f;
	add_camera_rx = 0.0f;
	add_camera_ry = 0.0f;

	GameState->NextState();
	return 0;
}

int opening::Recovery()
{
	//ブロックデータ初期化
	d3dg->LoadMapdata(&BlockData, "data\\map10\\");

	//ポイントデータ初期化
	ObjMgr.Recovery();

	//背景空初期化
	Resource.CleanupSkyModelTexture();
	Resource.LoadSkyModelTexture(1);

	//opening_banner = d3dg->LoadTexture("banner.png", true, false);

	return 0;
}

void opening::Input()
{
	inputCtrl->GetInputState(false);

	if( inputCtrl->CheckKeyDown(GetEscKeycode()) ){
		GameState->PushBackSpaceKey();
	}
	else if( inputCtrl->CheckMouseButtonUpL() ){
		GameState->PushMouseButton();
	}
	else if( inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x0F)) ){		// [ENTER]
		//[ENTER]を押しても、マウスをクリックしたことにする。
		GameState->PushMouseButton();
	}
}

//! @todo カメラの移動を滑らかにする
void opening::Process()
{
	//オブジェクトマネージャーを実行
	ObjMgr.Process(-1, false, false);

	//AIを実行
	for(int i=0; i<MAX_HUMAN; i++){
		HumanAI[i].Process();
	}

	//カメラワークを求める（座標）
	if( framecnt < (int)(4.0f*GAMEFPS) ){
		add_camera_z = 0.0f;
		add_camera_y = 0.0f;
	}
	else if( framecnt < (int)(5.0f*GAMEFPS) ){
		add_camera_z += (0.08f - add_camera_z) / 5.0f;
		add_camera_y += (-0.05f - add_camera_y) / 5.0f;
	}
	else{
		add_camera_z = 0.08f;
		add_camera_y = -0.05f;
	}
	camera_x += add_camera_x;
	camera_y += add_camera_y;
	camera_z += add_camera_z;

	//カメラワークを求める（回転）
	if( framecnt < (int)(2.6f*GAMEFPS) ){
		add_camera_rx = 0.0f;
		add_camera_ry = 0.0f;
	}
	else if( framecnt < (int)(3.6f*GAMEFPS) ){
		add_camera_rx += (DegreeToRadian(0.9f) - add_camera_rx) / 5.0f;
		add_camera_ry += (DegreeToRadian(-0.6f) - add_camera_ry) / 5.0f;
	}
	else if( framecnt < (int)(5.0f*GAMEFPS) ){
		add_camera_rx = DegreeToRadian(0.9f);
		add_camera_ry = DegreeToRadian(-0.6f);
	}
	else{
		add_camera_rx *= 0.8f;
		add_camera_ry *= 0.8f;
	}
	camera_rx += add_camera_rx;
	camera_ry += add_camera_ry;

	//16秒経ったら終了
	if( framecnt >= 16*((int)GAMEFPS) ){
		GameState->PushMouseButton();
	}

	framecnt += 1;
}

void opening::Render3D()
{
	int skymodel, skytexture;

	//フォグとカメラを設定
	d3dg->SetFog(true, 1);
	d3dg->SetCamera(camera_x, camera_y, camera_z, camera_rx, camera_ry, VIEWANGLE_NORMAL);

	//カメラ座標に背景空を描画
	d3dg->SetWorldTransform(camera_x, camera_y, camera_z, 0.0f, 0.0f, 2.0f);
	Resource.GetSkyModelTexture(&skymodel, &skytexture);
	d3dg->RenderModel(skymodel, skytexture, false, false, false);

	//Zバッファを初期化
	d3dg->ResetZbuffer();

	//マップを描画
	d3dg->ResetWorldTransform();
	d3dg->RenderMapdata(false);

	//オブジェクトを描画
	ObjMgr.Render(camera_x, camera_y, camera_z, camera_rx, camera_ry, 0, false);
}

void opening::Render2D()
{
	float effect = 0.0f;

	//float scaling_x = (float)GameConfig.GetScreenWidth() / 640;
	float scaling_y = (float)GameConfig.GetScreenHeight() / 480;
	int swidth = (int)((float)GameConfig.GetScreenWidth() / GameConfig.GetScreenHeight() * 480);

	//ブラックアウト設定
	if( framecnt < (int)(2.0f*GAMEFPS) ){
		effect = GetEffectAlpha(framecnt, 1.0f, 2.0f, 0.0f, true);
	}
	if( ((int)(2.0f*GAMEFPS) <= framecnt)&&(framecnt < (int)(11.0f*GAMEFPS)) ){
		effect = 0.0f;
	}
	if( ((int)(11.0f*GAMEFPS) <= framecnt)&&(framecnt < (int)(15.0f*GAMEFPS)) ){
		effect = GetEffectAlpha(framecnt, 1.0f, 4.0f, 11.0f, false);
	}
	if( (int)(15.0f*GAMEFPS) <= framecnt ){
		effect = 1.0f;
	}
	d3dg->Draw2DBox(0, 0, GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,effect));

	//上下の黒縁描画
	d3dg->Draw2DBox(0, 0, GameConfig.GetScreenWidth(), (int)(scaling_y * 40), d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
	d3dg->Draw2DBox(0, GameConfig.GetScreenHeight() - (int)(scaling_y * 40), GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));

	//プロジェクト名
	if( ((int)(0.5f*GAMEFPS) < framecnt)&&(framecnt < (int)(4.0f*GAMEFPS)) ){
		char str[32];
		float effectA = 1.0f;
		sprintf(str, "%s project", GAMENAME);
		if( framecnt < (int)(1.5f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 0.5f, false); }
		if( framecnt > (int)(3.0f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 3.0f, true); }
		d3dg->Draw2DTextureFontTextCenterScaling(0, 340, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,effectA), 22, 22);
	}

	//スタッフ名・その１
	if( ((int)(4.5f*GAMEFPS) < framecnt)&&(framecnt < (int)(8.5f*GAMEFPS)) ){
		float effectA = 1.0f;
		if( framecnt < (int)(5.5f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 4.5f, false); }
		if( framecnt > (int)(7.5f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 7.5f, true); }
		d3dg->Draw2DTextureFontTextScaling(60, 150, "ORIGINAL", d3dg->GetColorCode(1.0f,1.0f,1.0f,effectA), 20, 20);
	}
	if( ((int)(5.0f*GAMEFPS) < framecnt)&&(framecnt < (int)(9.0f*GAMEFPS)) ){
		float effectA = 1.0f;
		if( framecnt < (int)(6.0f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 5.0f, false); }
		if( framecnt > (int)(8.0f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 8.0f, true); }
		d3dg->Draw2DTextureFontTextScaling(100, 180, "nine-two", d3dg->GetColorCode(1.0f,1.0f,1.0f,effectA), 20, 20);
		d3dg->Draw2DTextureFontTextScaling(100, 210, "TENNKUU", d3dg->GetColorCode(1.0f,1.0f,1.0f,effectA), 20, 20);
	}

	//スタッフ名・その２
	if( ((int)(7.0f*GAMEFPS) < framecnt)&&(framecnt < (int)(11.0f*GAMEFPS)) ){
		float effectA = 1.0f;
		if( framecnt < (int)(8.0f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 7.0f, false); }
		if( framecnt > (int)(10.0f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 10.0f, true); }
		d3dg->Draw2DTextureFontTextScaling(swidth - 310, 300, "REMAKE", d3dg->GetColorCode(1.0f,1.0f,1.0f,effectA), 20, 20);
	}
	if( ((int)(7.5f*GAMEFPS) < framecnt)&&(framecnt < (int)(11.5f*GAMEFPS)) ){
		float effectA = 1.0f;
		if( framecnt < (int)(8.5f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 7.5f, false); }
		if( framecnt > (int)(10.5f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 10.5f, true); }
		d3dg->Draw2DTextureFontTextScaling(swidth - 270, 330, "[-_-;](mikan)", d3dg->GetColorCode(1.0f,1.0f,1.0f,effectA), 20, 20);
		//d3dg->Draw2DTextureScaling(swidth - 230, 360, opening_banner, 200, 40, effectA);
	}

	//ゲーム名
	if( (int)(12.0f*GAMEFPS) <= framecnt ){	//framecnt < (int)(16.0f*GAMEFPS)
		char str[32];
		float effectA = 1.0f;
		sprintf(str, GAMENAME);
		if( framecnt < (int)(13.0f*GAMEFPS) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 1.0f, 12.0f, false); }
		if( ((int)(15.0f*GAMEFPS) < framecnt)&&(framecnt < (int)(15.5f*GAMEFPS)) ){ effectA = GetEffectAlpha(framecnt, 1.0f, 0.5f, 15.0f, true); }
		if( framecnt >= (int)(15.5f*GAMEFPS) ){ effectA = 0.0f; }
		d3dg->Draw2DTextureFontTextCenterScaling(0, (480-11)/2, str, d3dg->GetColorCode(1.0f,0.0f,0.0f,effectA), 22, 22);
	}
}

void opening::Destroy()
{
	//ブロックデータ解放
	d3dg->CleanupMapdata();

	//オブジェクトマネージャー解放
	ObjMgr.Cleanup();

	//背景空解放
	Resource.CleanupSkyModelTexture();

	//d3dg->CleanupTexture(opening_banner);

	GameState->NextState();
}

//! @brief コンストラクタ
mainmenu::mainmenu()
{
	demopath[0] = '\0';
	modescreen = 0;
	mainmenu_mouseX = 0;
	mainmenu_mouseY = 0;
	mainmenu_mouseY_old = mainmenu_mouseY;
	gametitle = -1;

#ifdef ENABLE_MENUOPTIONS
	for( int i=0; i<OPTIONS_P1_DATAS; i++ ){
		Options_p1LinkTextData[i].enable = false;
	}
	for( int i=0; i<OPTIONS_P2_DATAS; i++ ){
		Options_p2LinkTextData[i].enable = false;
	}
	for( int i=0; i<OPTIONS_P3_DATAS; i++ ){
		Options_p3LinkTextData[i].enable = false;
	}
	Options_p2LinkTextID = -1;
	Options_FullscreenFlag_old = GameConfig.GetFullscreenFlag();
	Options_KeyConfigID = -1;
	Options_KeyConfigCnt = 0;
	licenseinfo[0] = "";
	licenseinfobuf = new char[100*18];
	for( int i=0; i<OPTIONS_LICENSELINE; i++ ){
		licenseinfobuf[i] = '\0';
	}
#endif
}

//! @brief ディストラクタ
mainmenu::~mainmenu()
{
#ifdef ENABLE_MENUOPTIONS
	if( licenseinfobuf != NULL ){
		delete [] licenseinfobuf;
		licenseinfobuf = NULL;
	}
#endif
}

int mainmenu::Create()
{
	char bdata[MAX_PATH];
	char pdata[MAX_PATH];
	int blockflag, pointflag;

	//デモを決定し読み込む
	switch( GetRand(6) ){
		case 0:
			strcpy(demopath, "data\\map2\\");
			break;
		case 1:
			strcpy(demopath, "data\\map4\\");
			break;
		case 2:
			strcpy(demopath, "data\\map5\\");
			break;
		case 3:
			strcpy(demopath, "data\\map7\\");
			break;
		case 4:
			strcpy(demopath, "data\\map8\\");
			break;
		case 5:
			strcpy(demopath, "data\\map16\\");
			break;
	}
	// OpenXOPS-web: data packs may lack the classic demo maps (free pack).
	// Try the random pick first, then the other classic demos, then the
	// pack-provided data\menu\ demo (docs/MISSIONS.md).
	{
		const char *demos[] = { demopath, "data\\map2\\", "data\\map4\\", "data\\map5\\", "data\\map7\\", "data\\map8\\", "data\\map16\\", "data\\menu\\" };
		char picked[MAX_PATH];
		strcpy(picked, demopath);
		for(int i=0; i<(int)(sizeof(demos)/sizeof(demos[0])); i++){
			strcpy(bdata, demos[i]);
			strcat(bdata, "temp.bd1");
			strcpy(pdata, demos[i]);
			strcat(pdata, "demo.pd1");
			FILE *bf = vfs_fopen(bdata, "rb");
			FILE *pf = vfs_fopen(pdata, "rb");
			const bool ok = (bf != NULL) && (pf != NULL);
			if( bf != NULL ){ fclose(bf); }
			if( pf != NULL ){ fclose(pf); }
			if( ok ){ strcpy(picked, demos[i]); break; }
		}
		strcpy(demopath, picked);
	}
	strcpy(bdata, demopath);
	strcat(bdata, "temp.bd1");
	strcpy(pdata, demopath);
	strcat(pdata, "demo.pd1");

	//ブロックデータ読み込み
	blockflag = BlockData.LoadFiledata(bdata);

	//ポイントデータ読み込み
	pointflag = PointData.LoadFiledata(pdata);

	//ファイル読み込みでエラーがあったら中断
	if( (blockflag != 0)||(pointflag != 0) ){
		//2bit : point data open failed
		//1bit : block data open failed
		return pointflag*2 + blockflag;
	}

	//ブロックデータ初期化
	BlockData.CalculationBlockdata(false);
	d3dg->LoadMapdata(&BlockData, demopath);
	CollD.InitCollision(&BlockData);

	//ポイントデータ初期化
	ObjMgr.LoadPointData();

	//追加の当たり判定設定
	ObjMgr.SetAddCollisionFlag(false);

	//AI設定
	for(int i=0; i<MAX_HUMAN; i++){
		HumanAI[i].SetClass(&ObjMgr, i, &BlockData, &PointData, &GameParamInfo, &CollD, GameSound);
		HumanAI[i].Init();
	}

	modescreen = 0;

	gametitle = d3dg->LoadTexture("data\\title.dds", false, false);

	//サウンド初期化
	GameSound->InitWorldSound();

	mainmenu_mouseX = GameConfig.GetScreenWidth()/2;
	mainmenu_mouseY = GameConfig.GetScreenHeight()/2;
	mainmenu_mouseY_old = mainmenu_mouseY;

	//ミッションのスクロールバーの設定
	scrollbar_official.Create(MAINMENU_X+341, MAINMENU_Y+1, 19, MAINMENU_H-1-24, GameParamInfo.GetOfficialMissionCount(), TOTAL_MENUITEMS, scrollbar_official.GetScrollItem());
	scrollbar_official.SetColor(d3dg->GetColorCode(0.5f,0.5f,0.5f,0.5f), d3dg->GetColorCode(0.6f,0.6f,0.25f,1.0f), d3dg->GetColorCode(0.8f,0.8f,0.25f,1.0f),
		d3dg->GetColorCode(0.4f,0.67f,0.57f,1.0f), d3dg->GetColorCode(0.38f,0.77f,0.64f,1.0f), d3dg->GetColorCode(0.6f,0.3f,0.25f,1.0f), d3dg->GetColorCode(0.8f,0.3f,0.25f,1.0f));
	scrollbar_addon.Create(MAINMENU_X+341, MAINMENU_Y+1, 19, MAINMENU_H-1-24, GameAddon.GetTotaldatas(), TOTAL_MENUITEMS, scrollbar_addon.GetScrollItem());
	scrollbar_addon.SetColor(d3dg->GetColorCode(0.5f,0.5f,0.5f,0.5f), d3dg->GetColorCode(0.6f,0.6f,0.25f,1.0f), d3dg->GetColorCode(0.8f,0.8f,0.25f,1.0f),
		d3dg->GetColorCode(0.4f,0.67f,0.57f,1.0f), d3dg->GetColorCode(0.38f,0.77f,0.64f,1.0f), d3dg->GetColorCode(0.6f,0.3f,0.25f,1.0f), d3dg->GetColorCode(0.8f,0.3f,0.25f,1.0f));

#ifdef ENABLE_MENUOPTIONS
	CreateOptions();
#endif

	inputCtrl->MoveMouseCenter();
	framecnt = 0;

	GameState->NextState();
	return 0;
}

int mainmenu::Recovery()
{
	//ブロックデータ初期化
	d3dg->LoadMapdata(&BlockData, demopath);

	//ポイントデータ初期化
	ObjMgr.Recovery();

	gametitle = d3dg->LoadTexture("data\\title.dds", false, false);

	return 0;
}

void mainmenu::Input()
{
	inputCtrl->GetInputState(false);

	//マウス座標を取得
	inputCtrl->GetMouseMovement(&mainmenu_mouseX, &mainmenu_mouseY);
	//if( mainmenu_mouseX < 0 ){ mainmenu_mouseX = 0; }
	//if( mainmenu_mouseX > GameConfig.GetScreenWidth()-1 ){ mainmenu_mouseX = GameConfig.GetScreenWidth()-1; }
	//if( mainmenu_mouseY < 0 ){ mainmenu_mouseY = 0; }
	//if( mainmenu_mouseY > GameConfig.GetScreenHeight()-1 ){ mainmenu_mouseY = GameConfig.GetScreenHeight()-1; }

	if( modescreen == 0 ){
		//スクロールバーを処理し情報取得
		int scrollitems;
		int totalmission;
		if( GameInfoData.selectmission_id >= 0 ){
			scrollbar_official.Input(mainmenu_mouseX, mainmenu_mouseY, inputCtrl->CheckMouseButtonNowL(), mainmenu_mouseY - mainmenu_mouseY_old);
			scrollitems = scrollbar_official.GetScrollItem();
			totalmission = GameParamInfo.GetOfficialMissionCount();
		}
		else{
			scrollbar_addon.Input(mainmenu_mouseX, mainmenu_mouseY, inputCtrl->CheckMouseButtonNowL(), mainmenu_mouseY - mainmenu_mouseY_old);
			scrollitems = scrollbar_addon.GetScrollItem();
			totalmission = GameAddon.GetTotaldatas();
		}

		//ミッション選択
		if( inputCtrl->CheckMouseButtonUpL() ){
			for(int i=0; i<TOTAL_MENUITEMS; i++){
				char name[32];
				strcpy(name, "");
				if( GameInfoData.selectmission_id >= 0 ){
					GameParamInfo.GetOfficialMission(scrollitems + i, name, NULL, NULL, NULL, NULL, NULL);

					if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+(signed)strlen(name)*20))&&(MAINMENU_Y+30 + i*30 < mainmenu_mouseY)&&(mainmenu_mouseY < MAINMENU_Y+30 + i*30 + 26) ){
						GameInfoData.selectmission_id = scrollitems + i;
						strcpy(GameInfoData.mifpath, "");
						GameState->PushMouseButton();
					}
				}
				else{
					strcpy(name, GameAddon.GetMissionName(scrollitems + i));

					if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+(signed)strlen(name)*20))&&(MAINMENU_Y+30 + i*30 < mainmenu_mouseY)&&(mainmenu_mouseY < MAINMENU_Y+30 + i*30 + 26) ){
						GameInfoData.selectmission_id = -1;
						strcpy(GameInfoData.mifpath, ADDONDIR);
						strcat(GameInfoData.mifpath, "\\");
						strcat(GameInfoData.mifpath, GameAddon.GetFileName(scrollitems + i));
						GameState->PushMouseButton();
					}
				}
			}
		}

		if( inputCtrl->CheckMouseButtonUpL() ){
			// UP
			if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+340))&&(MAINMENU_Y < mainmenu_mouseY)&&(mainmenu_mouseY < MAINMENU_Y+30) ){
				if( GameInfoData.selectmission_id >= 0 ){
					scrollbar_official.ScrollUP();
				}
				else{
					scrollbar_addon.ScrollUP();
				}
			}

			// DOWN
			if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+340))&&((MAINMENU_Y+MAINMENU_H-55) < mainmenu_mouseY)&&(mainmenu_mouseY < (MAINMENU_Y+MAINMENU_H-55+30)) ){
				if( GameInfoData.selectmission_id >= 0 ){
					scrollbar_official.ScrollDOWN();
				}
				else{
					scrollbar_addon.ScrollDOWN();
				}
			}
		}

		//標準ミッションとアドオンリストの切り替え
		if( inputCtrl->CheckMouseButtonUpL() ){
			if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+340))&&((MAINMENU_Y+MAINMENU_H-25) < mainmenu_mouseY)&&(mainmenu_mouseY < (MAINMENU_Y+MAINMENU_H-2)) ){
				if( GameInfoData.selectmission_id >= 0 ){
					if( GameAddon.GetTotaldatas() > 0 ){
						GameInfoData.selectmission_id = -1;
					}
				}
				else{
					GameInfoData.selectmission_id = 0;
				}
			}
		}

#ifdef ENABLE_MENUOPTIONS
		//オプション画面切り替え
		if( inputCtrl->CheckMouseButtonUpL() ){
			if( (MAINMENU_OPTION_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_OPTION_X+120))&&(MAINMENU_OPTION_Y < mainmenu_mouseY)&&(mainmenu_mouseY < MAINMENU_OPTION_Y+16) ){
				modescreen = 1;
			}
		}
#endif

		//ESCキーを処理
		if( inputCtrl->CheckKeyDown(GetEscKeycode()) ){
			GameState->PushBackSpaceKey();
		}
	}
	else{
#ifdef ENABLE_MENUOPTIONS
		InputOptions();
#endif
	}

	mainmenu_mouseY_old = mainmenu_mouseY;
}

void mainmenu::Process()
{
	//オブジェクトマネージャーを実行
	ObjMgr.Process(-1, true, false);

	//AIを実行
	for(int i=0; i<MAX_HUMAN; i++){
		HumanAI[i].Process();
	}

	//カメラ位置を計算
	human *myHuman = ObjMgr.GetPlayerHumanObject();
	myHuman->GetPosData(&camera_x, &camera_y, &camera_z, NULL);
	camera_x -= 4.0f;
	camera_y += 22.0f;
	camera_z -= 12.0f;
	camera_rx = DegreeToRadian(45);
	camera_ry = DegreeToRadian(-25);

	if( modescreen != 0 ){
#ifdef ENABLE_MENUOPTIONS
		ProcessOptions();
#endif
	}

	framecnt += 1;
}

void mainmenu::Render3D()
{
	//フォグとカメラを設定
	d3dg->SetFog(true, 0);
	d3dg->SetCamera(camera_x, camera_y, camera_z, camera_rx, camera_ry, VIEWANGLE_NORMAL);

	//Zバッファを初期化
	d3dg->ResetZbuffer();

	//マップを描画
	d3dg->ResetWorldTransform();
	d3dg->RenderMapdata(false);

	//オブジェクトを描画
	ObjMgr.Render(camera_x, camera_y, camera_z, camera_rx, camera_ry, 0, false);
}

void mainmenu::Render2D()
{
	int color;
	float effect;

	int swidth = (int)((float)GameConfig.GetScreenWidth() / GameConfig.GetScreenHeight() * 480);

	//ゲームのバージョン情報表示
	d3dg->Draw2DTextureFontTextScaling(swidth - 118+1, 75+1, GAMEVERSION, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 18, 22);
	d3dg->Draw2DTextureFontTextScaling(swidth - 118, 75, GAMEVERSION, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 18, 22);

	if( modescreen == 0 ){
		//スクロールバーの情報を取得
		int scrollitems;
		int totalmission;
		bool scrolling;
		if( GameInfoData.selectmission_id >= 0 ){
			scrollitems = scrollbar_official.GetScrollItem();
			totalmission = GameParamInfo.GetOfficialMissionCount();
			scrolling = scrollbar_official.CheckScrolling();
		}
		else{
			scrollitems = scrollbar_addon.GetScrollItem();
			totalmission = GameAddon.GetTotaldatas();
			scrolling = scrollbar_addon.CheckScrolling();
		}

		//メニューエリア描画
		if( GameAddon.GetTotaldatas() > 0 ){	//addonがあれば
			d3dg->Draw2DBox(MAINMENU_X-1, MAINMENU_Y, MAINMENU_X+360, MAINMENU_Y+MAINMENU_H+1, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));
		}
		else{
			d3dg->Draw2DBox(MAINMENU_X-1, MAINMENU_Y, MAINMENU_X+360, MAINMENU_Y+MAINMENU_H-24, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));
		}

		//スクロールバー描画
		if( GameInfoData.selectmission_id >= 0 ){
			scrollbar_official.Draw(d3dg);
		}
		else{
			scrollbar_addon.Draw(d3dg);
		}

		//'< UP >'表示
		if( scrollitems > 0 ){
			d3dg->Draw2DTextureFontText(MAINMENU_X+1, MAINMENU_Y+1, "<  UP  >", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 25, 26);

			//文字の色を設定
			if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+340))&&(MAINMENU_Y < mainmenu_mouseY)&&(mainmenu_mouseY < MAINMENU_Y+30)&&(scrolling == false) ){
				color = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f);
			}
			else{
				color = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
			}

			//文字を表示
			d3dg->Draw2DTextureFontText(MAINMENU_X, MAINMENU_Y, "<  UP  >", color, 25, 26);
		}
		else{
			d3dg->Draw2DTextureFontText(MAINMENU_X+1, MAINMENU_Y+1, "<  UP  >", d3dg->GetColorCode(0.6f,0.6f,0.6f,1.0f), 25, 26);
		}

		//'< DOWN >'表示
		if( scrollitems < (totalmission - TOTAL_MENUITEMS) ){
			d3dg->Draw2DTextureFontText(MAINMENU_X+1, MAINMENU_Y+MAINMENU_H-55+1, "< DOWN >", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 25, 26);

			//文字の色を設定
			if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+340))&&((MAINMENU_Y+MAINMENU_H-55) < mainmenu_mouseY)&&(mainmenu_mouseY < (MAINMENU_Y+MAINMENU_H-55+30))&&(scrolling == false) ){
				color = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f);
			}
			else{
				color = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
			}

			//文字を表示
			d3dg->Draw2DTextureFontText(MAINMENU_X, MAINMENU_Y+MAINMENU_H-55, "< DOWN >", color, 25, 26);
		}
		else{
			d3dg->Draw2DTextureFontText(MAINMENU_X+1, MAINMENU_Y+MAINMENU_H-55+1, "< DOWN >", d3dg->GetColorCode(0.6f,0.6f,0.6f,1.0f), 25, 26);
		}

		//標準ミッションとaddon切り替え
		if( GameInfoData.selectmission_id >= 0 ){
			//addonがあれば
			if( GameAddon.GetTotaldatas() > 0 ){
				//文字の色を設定
				if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+340))&&((MAINMENU_Y+MAINMENU_H-25) < mainmenu_mouseY)&&(mainmenu_mouseY < (MAINMENU_Y+MAINMENU_H-2))&&(scrolling == false) ){
					color = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f);
				}
				else{
					color = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
				}

				//文字を表示
				d3dg->Draw2DTextureFontText(MAINMENU_X+1, MAINMENU_Y+MAINMENU_H-25+1, "ADD-ON MISSIONS >>", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 17, 22);
				d3dg->Draw2DTextureFontText(MAINMENU_X, MAINMENU_Y+MAINMENU_H-25, "ADD-ON MISSIONS >>", color, 17, 22);
			}
		}
		else{
			//addonがあれば
			//if( GameAddon.GetTotaldatas() > 0 ){
				//文字の色を設定
				if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+340))&&((MAINMENU_Y+MAINMENU_H-25) < mainmenu_mouseY)&&(mainmenu_mouseY < (MAINMENU_Y+MAINMENU_H-2))&&(scrolling == false) ){
					color = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f);
				}
				else{
					color = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
				}

				//文字を表示
				d3dg->Draw2DTextureFontText(MAINMENU_X+1, MAINMENU_Y+MAINMENU_H-25+1, "<< STANDARD MISSIONS", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 17, 22);
				d3dg->Draw2DTextureFontText(MAINMENU_X, MAINMENU_Y+MAINMENU_H-25, "<< STANDARD MISSIONS", color, 17, 22);
			//}
		}

		//ミッション名を表示
		for(int i=0; i<TOTAL_MENUITEMS; i++){
			char name[32];
			strcpy(name, "");

			//ミッション名を取得
			if( GameInfoData.selectmission_id >= 0 ){
				GameParamInfo.GetOfficialMission(scrollitems + i, name, NULL, NULL, NULL, NULL, NULL);
			}
			else{
				strcpy(name, GameAddon.GetMissionName(scrollitems + i));
			}

			//文字の色を設定
			if( (MAINMENU_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_X+(signed)strlen(name)*20))&&(MAINMENU_Y+30 + i*30 < mainmenu_mouseY)&&(mainmenu_mouseY < MAINMENU_Y+30 + i*30 + 26)&&(scrolling == false) ){
				color = d3dg->GetColorCode(1.0f,0.6f,0.6f,1.0f);
			}
			else{
				color = d3dg->GetColorCode(0.6f,0.6f,1.0f,1.0f);
			}

			//文字を表示
			d3dg->Draw2DTextureFontText(MAINMENU_X+1, MAINMENU_Y+30+1 + i*30, name, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 20, 26);
			d3dg->Draw2DTextureFontText(MAINMENU_X, MAINMENU_Y+30 + i*30, name, color, 20, 26);
		}

#ifdef ENABLE_MENUOPTIONS
		//オプション項目リンク
		d3dg->Draw2DBox(MAINMENU_OPTION_X, MAINMENU_OPTION_Y, MAINMENU_OPTION_X+132+2, MAINMENU_OPTION_Y+16+2, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));

		//文字の色を設定
		if( (MAINMENU_OPTION_X < mainmenu_mouseX)&&(mainmenu_mouseX < (MAINMENU_OPTION_X+132+2))&&(MAINMENU_OPTION_Y < mainmenu_mouseY)&&(mainmenu_mouseY < MAINMENU_OPTION_Y+16)&&(scrolling == false) ){
			color = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f);
		}
		else{
			color = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
		}

		//文字を表示
		d3dg->Draw2DTextureFontText(MAINMENU_OPTION_X+1, MAINMENU_OPTION_Y+1, "< OPTIONS >", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(MAINMENU_OPTION_X, MAINMENU_OPTION_Y, "< OPTIONS >", color, 12, 16);
#endif	//ENABLE_MENUOPTIONS
	}
	else{
#ifdef ENABLE_MENUOPTIONS
		RenderOptions();
#endif
	}

	//マウスカーソル表示（赤線）
	d3dg->Draw2DBox(0, mainmenu_mouseY-1, GameConfig.GetScreenWidth(), mainmenu_mouseY+1, d3dg->GetColorCode(1.0f,0.0f,0.0f,0.5f));
	d3dg->Draw2DBox(mainmenu_mouseX-1, 0, mainmenu_mouseX+1, GameConfig.GetScreenHeight(), d3dg->GetColorCode(1.0f,0.0f,0.0f,0.5f));
	d3dg->Draw2DLine(0, mainmenu_mouseY, GameConfig.GetScreenWidth(), mainmenu_mouseY, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
	d3dg->Draw2DLine(mainmenu_mouseX, 0, mainmenu_mouseX, GameConfig.GetScreenHeight(), d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));

	//ゲームのロゴマーク描画
	d3dg->Draw2DTextureScaling(20, 25, gametitle, 480, 80, 1.0f);

	//ブラックアウト設定
	if( framecnt < (int)(2.0f*GAMEFPS) ){
		effect = GetEffectAlpha(framecnt, 1.0f, 2.0f, 0.0f, true);
	}
	else{
		effect = 0.0f;
	}
	d3dg->Draw2DBox(0, 0, GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,effect));
}

#ifdef ENABLE_MENUOPTIONS
//! @brief オプション画面の初期化処理
void mainmenu::CreateOptions()
{
	//変数初期化
	Options_FullscreenFlag_old = GameConfig.GetFullscreenFlag();
	Options_KeyConfigID = -1;
	Options_KeyConfigCnt = 0;


	//No.1画面　リンクテキスト
	Options_p1LinkTextData[0].enable = true;
	Options_p1LinkTextData[0].pos_x = 0;
	Options_p1LinkTextData[0].pos_y = 30;
	Options_p1LinkTextData[0].text_w = 20;
	Options_p1LinkTextData[0].text_h = 26;
	Options_p1LinkTextData[0].text_defaultcolor = d3dg->GetColorCode(0.6f,0.6f,1.0f,1.0f);
	Options_p1LinkTextData[0].text_cursorcolor = d3dg->GetColorCode(1.0f,0.6f,0.6f,1.0f);
	strcpy(Options_p1LinkTextData[0].text, "CONFIG");
	Options_p1LinkTextData[1].enable = true;
	Options_p1LinkTextData[1].pos_x = 0;
	Options_p1LinkTextData[1].pos_y = 90;
	Options_p1LinkTextData[1].text_w = 20;
	Options_p1LinkTextData[1].text_h = 26;
	Options_p1LinkTextData[1].text_defaultcolor = d3dg->GetColorCode(0.6f,0.6f,1.0f,1.0f);
	Options_p1LinkTextData[1].text_cursorcolor = d3dg->GetColorCode(1.0f,0.6f,0.6f,1.0f);
	strcpy(Options_p1LinkTextData[1].text, "SOFTWARE LICENSE");
	Options_p1LinkTextData[2].enable = true;
	Options_p1LinkTextData[2].pos_x = 0;
	Options_p1LinkTextData[2].pos_y = 150;
	Options_p1LinkTextData[2].text_w = 20;
	Options_p1LinkTextData[2].text_h = 26;
	Options_p1LinkTextData[2].text_defaultcolor = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
	Options_p1LinkTextData[2].text_cursorcolor = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f);
	strcpy(Options_p1LinkTextData[2].text, "< EXIT >");


	//No.2画面　リンクテキスト
	strcpy(Options_p2LinkTextData[0].text, "< DEFAULT >");
	strcpy(Options_p2LinkTextData[1].text, "< SAVE >");
	strcpy(Options_p2LinkTextData[2].text, "< CANCEL >");
	for(int i=0; i<3; i++){
		Options_p2LinkTextData[i].enable = true;
		Options_p2LinkTextData[i].pos_x = 350;
		Options_p2LinkTextData[i].pos_y = 280 + i*30;
		Options_p2LinkTextData[i].text_w = 12;
		Options_p2LinkTextData[i].text_h = 16;
		Options_p2LinkTextData[i].text_defaultcolor = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
		Options_p2LinkTextData[i].text_cursorcolor = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f);
	}

	Options_p2LinkTextData[3].pos_x = 350+20;
	Options_p2LinkTextData[3].pos_y = 20;
	strcpy(Options_p2LinkTextData[3].text, "[<<]");
	Options_p2LinkTextData[4].pos_x = 350+120;
	Options_p2LinkTextData[4].pos_y = 20;
	strcpy(Options_p2LinkTextData[4].text, "[>>]");
	Options_p2LinkTextData[5].pos_x = 350+20;
	Options_p2LinkTextData[5].pos_y = 70;
	strcpy(Options_p2LinkTextData[5].text, "[<<]");
	Options_p2LinkTextData[6].pos_x = 350+120;
	Options_p2LinkTextData[6].pos_y = 70;
	strcpy(Options_p2LinkTextData[6].text, "[>>]");
	for(int i=0; i<4; i++){
		Options_p2LinkTextData[3 + i].enable = true;
		Options_p2LinkTextData[3 + i].text_w = 12;
		Options_p2LinkTextData[3 + i].text_h = 16;
		Options_p2LinkTextData[3 + i].text_defaultcolor = d3dg->GetColorCode(0.6f,0.6f,1.0f,1.0f);
		Options_p2LinkTextData[3 + i].text_cursorcolor = d3dg->GetColorCode(1.0f,0.6f,0.6f,1.0f);
	}

	Options_p2LinkTextData[7].pos_x = 350;
	Options_p2LinkTextData[7].pos_y = 100;
	strcpy(Options_p2LinkTextData[7].text, "[ ] FULL SCREEN");
	Options_p2LinkTextData[8].pos_x = 350;
	Options_p2LinkTextData[8].pos_y = 120;
	strcpy(Options_p2LinkTextData[8].text, "[ ] WINDOW");
	Options_p2LinkTextData[9].pos_x = 350;
	Options_p2LinkTextData[9].pos_y = 160;
	strcpy(Options_p2LinkTextData[9].text, "[ ] ENABLE SOUND");
	Options_p2LinkTextData[10].pos_x = 350;
	Options_p2LinkTextData[10].pos_y = 180;
	strcpy(Options_p2LinkTextData[10].text, "[ ] ENABLE BLOOD");
	Options_p2LinkTextData[11].pos_x = 350;
	Options_p2LinkTextData[11].pos_y = 200;
	strcpy(Options_p2LinkTextData[11].text, "[ ] INVERT MOUSE");
	Options_p2LinkTextData[12].pos_x = 350;
	Options_p2LinkTextData[12].pos_y = 220;
	strcpy(Options_p2LinkTextData[12].text, "[ ] FRAME SKIP");
	Options_p2LinkTextData[13].pos_x = 350;
	Options_p2LinkTextData[13].pos_y = 240;
	strcpy(Options_p2LinkTextData[13].text, "[ ] ANOTHER GUNSIGHT");
	for(int i=0; i<7; i++){
		Options_p2LinkTextData[7 + i].enable = true;
		Options_p2LinkTextData[7 + i].text_w = 12;
		Options_p2LinkTextData[7 + i].text_h = 16;
		Options_p2LinkTextData[7 + i].text_defaultcolor = d3dg->GetColorCode(0.6f,0.6f,1.0f,1.0f);
		Options_p2LinkTextData[7 + i].text_cursorcolor = d3dg->GetColorCode(1.0f,0.6f,0.6f,1.0f);
	}

	for(int i=0; i<18; i++){
		Options_p2LinkTextData[14 + i].enable = true;
		Options_p2LinkTextData[14 + i].pos_x = 170;
		Options_p2LinkTextData[14 + i].pos_y = i*20;
		Options_p2LinkTextData[14 + i].text_w = 12;
		Options_p2LinkTextData[14 + i].text_h = 16;
		Options_p2LinkTextData[14 + i].text_defaultcolor = d3dg->GetColorCode(0.6f,0.6f,1.0f,1.0f);
		Options_p2LinkTextData[14 + i].text_cursorcolor = d3dg->GetColorCode(1.0f,0.6f,0.6f,1.0f);
		strcpy(Options_p2LinkTextData[14 + i].text, "[ ]");
	}

	Options_p3LinkTextData[0].enable = true;
	Options_p3LinkTextData[0].pos_x = (OPTIONS_P3_W - 160)/2;
	Options_p3LinkTextData[0].pos_y = OPTIONS_P3_H - 35;
	Options_p3LinkTextData[0].text_w = 20;
	Options_p3LinkTextData[0].text_h = 26;
	Options_p3LinkTextData[0].text_defaultcolor = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
	Options_p3LinkTextData[0].text_cursorcolor = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f);
	strcpy(Options_p3LinkTextData[0].text, "< EXIT >");

	int maxline = 18;

	//韓国語であれば表示行数を減らす
	if( GameConfig.GetLanguage() == 1 ){
		maxline = 14;
	}

	scrollbar_license.Create(OPTIONS_P3_X + OPTIONS_P3_W - 20, OPTIONS_P3_Y+55, 20, 18*12, OPTIONS_LICENSELINE, maxline, 0);
	scrollbar_license.SetColor(d3dg->GetColorCode(0.5f,0.5f,0.5f,0.5f), d3dg->GetColorCode(0.6f,0.6f,0.25f,1.0f), d3dg->GetColorCode(0.8f,0.8f,0.25f,1.0f),
		d3dg->GetColorCode(0.4f,0.67f,0.57f,1.0f), d3dg->GetColorCode(0.38f,0.77f,0.64f,1.0f), d3dg->GetColorCode(0.6f,0.3f,0.25f,1.0f), d3dg->GetColorCode(0.8f,0.3f,0.25f,1.0f));

#if (GRAPHIC_ENGINE == 0)||(GRAPHIC_ENGINE == 1)||(GRAPHIC_ENGINE == 9)	//[OpenXOPS-web] GLES: full licence text (the short list is an OpenGL 1.1 flicker workaround)
	//ライセンス文章
	licenseinfo[0] = "";
	licenseinfo[1] = "OpenXOPS";
	licenseinfo[2] = "Copyright (c) 2014-2023, OpenXOPS Project / [-_-;](mikan) All rights reserved.";
	licenseinfo[3] = "";
	licenseinfo[4] = "Redistribution and use in source and binary forms, with or without";
	licenseinfo[5] = "modification, are permitted provided that the following conditions are met:";
	licenseinfo[6] = "* Redistributions of source code must retain the above copyright notice, ";
	licenseinfo[7] = "  this list of conditions and the following disclaimer.";
	licenseinfo[8] = "* Redistributions in binary form must reproduce the above copyright notice, ";
	licenseinfo[9] = "  this list of conditions and the following disclaimer in the documentation ";
	licenseinfo[10] = "  and/or other materials provided with the distribution.";
	licenseinfo[11] = "* Neither the name of the OpenXOPS Project nor the names of its contributors ";
	licenseinfo[12] = "  may be used to endorse or promote products derived from this software ";
	licenseinfo[13] = "  without specific prior written permission.";
	licenseinfo[14] = "";
	licenseinfo[15] = "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND";
	licenseinfo[16] = "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED";
	licenseinfo[17] = "WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE";
	licenseinfo[18] = "DISCLAIMED. IN NO EVENT SHALL OpenXOPS Project BE LIABLE FOR ANY";
	licenseinfo[19] = "DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES";
	licenseinfo[20] = "(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;";
	licenseinfo[21] = "LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND";
	licenseinfo[22] = "ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT";
	licenseinfo[23] = "(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS";
	licenseinfo[24] = "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";
	licenseinfo[25] = "";
	//行数追加時 "OPTIONS_LICENSELINE"定数 要変更
#elif GRAPHIC_ENGINE == 8
	licenseinfo[0] = "";
	licenseinfo[1] = "OpenXOPS";
	licenseinfo[2] = "Copyright (c) 2014-2023, OpenXOPS Project / [-_-;](mikan) All rights reserved.";
	licenseinfo[3] = "";
	licenseinfo[4] = " -- See the document.";
	licenseinfo[5] = "";
	//※OpenGLの場合、スクロール必要な分量を出すとチラつくので注意。
#endif	//GRAPHIC_ENGINE
}

//! @brief オプション画面の入力処理
void mainmenu::InputOptions()
{
	Options_p2LinkTextID = -1;

	if( modescreen == 1 ){
		//リンクテキスト判定処理
		int ButtonID = -1;
		for(int i=0; i<OPTIONS_P1_DATAS; i++){
			if( Options_p1LinkTextData[i].enable == true ){
				if( inputCtrl->CheckMouseButtonUpL() ){
					if( ((OPTIONS_P1_X + Options_p1LinkTextData[i].pos_x) < mainmenu_mouseX)&&(mainmenu_mouseX < (OPTIONS_P1_X + Options_p1LinkTextData[i].pos_x + (signed)strlen(Options_p1LinkTextData[i].text)*Options_p1LinkTextData[i].text_w))
						&&((OPTIONS_P1_Y + Options_p1LinkTextData[i].pos_y) < mainmenu_mouseY)&&(mainmenu_mouseY < OPTIONS_P1_Y + Options_p1LinkTextData[i].pos_y + Options_p1LinkTextData[i].text_h)
					){
						ButtonID = i;
					}
				}
			}
		}
		if( ButtonID == 0 ){
			modescreen = 2;
		}
		if( ButtonID == 1 ){
			modescreen = 3;

			//ライセンス文のスクロールバーを一番上にセット
			scrollbar_license.SetScrollItem(0);
			scrollbar_license.Input(0, 0, false, 0);
		}
		if( ButtonID == 2 ){
			modescreen = 0;
		}

		if( inputCtrl->CheckKeyDown(GetEscKeycode()) ){		//ESCキーを処理
			modescreen = 0;
		}
	}

	else if( modescreen == 2 ){
		//キー設定用の入力キー判定処理
		if( (Options_KeyConfigID != -1)&&(Options_KeyConfigCnt > 0) ){
			int keycode = inputCtrl->CheckInputAll();
			if( keycode != -1 ){
				GameConfig.SetKeycode(Options_KeyConfigID, keycode);

				Options_KeyConfigID = -1;
				Options_KeyConfigCnt = 0;

				return;		//クリック時に他のリンクテキストを反応させないよう処理終了
			}
		}

		//リンクテキスト判定処理
		for(int i=0; i<OPTIONS_P2_DATAS; i++){
			if( Options_p2LinkTextData[i].enable == true ){
				if( inputCtrl->CheckMouseButtonUpL() ){
					if( ((OPTIONS_P2_X + Options_p2LinkTextData[i].pos_x) < mainmenu_mouseX)&&(mainmenu_mouseX < (OPTIONS_P2_X + Options_p2LinkTextData[i].pos_x + (signed)strlen(Options_p2LinkTextData[i].text)*Options_p2LinkTextData[i].text_w))
						&&((OPTIONS_P2_Y + Options_p2LinkTextData[i].pos_y) < mainmenu_mouseY)&&(mainmenu_mouseY < OPTIONS_P2_Y + Options_p2LinkTextData[i].pos_y + Options_p2LinkTextData[i].text_h)
					){
						Options_p2LinkTextID = i;
					}
				}
			}
		}

		if( inputCtrl->CheckKeyDown(GetEscKeycode()) ){		//ESCキーを処理
			//modescreen = 1;
			Options_p2LinkTextID = 2;
		}
	}

	else if( modescreen == 3 ){
		scrollbar_license.Input(mainmenu_mouseX, mainmenu_mouseY, inputCtrl->CheckMouseButtonNowL(), mainmenu_mouseY - mainmenu_mouseY_old);

		//リンクテキスト判定処理
		int ButtonID = -1;
		for(int i=0; i<OPTIONS_P3_DATAS; i++){
			if( Options_p3LinkTextData[i].enable == true ){
				if( inputCtrl->CheckMouseButtonUpL() ){
					if( ((OPTIONS_P3_X + Options_p3LinkTextData[i].pos_x) < mainmenu_mouseX)&&(mainmenu_mouseX < (OPTIONS_P3_X + Options_p3LinkTextData[i].pos_x + (signed)strlen(Options_p3LinkTextData[i].text)*Options_p3LinkTextData[i].text_w))
						&&((OPTIONS_P3_Y + Options_p3LinkTextData[i].pos_y) < mainmenu_mouseY)&&(mainmenu_mouseY < (OPTIONS_P3_Y + Options_p3LinkTextData[i].pos_y + Options_p3LinkTextData[i].text_h))
					){
						ButtonID = i;
					}
				}
			}
		}
		if( ButtonID == 0 ){
			modescreen = 1;
		}

		if( inputCtrl->CheckKeyDown(GetEscKeycode()) ){		//ESCキーを処理
			modescreen = 1;
		}
	}
}

//! @brief オプション画面のメイン処理
void mainmenu::ProcessOptions()
{
	if( modescreen == 2 ){

		//Default
		if( Options_p2LinkTextID == 0 ){
			//デフォルトの値読み込み
			GameConfig.SetDefaultConfig();
		}

		//Save
		if( Options_p2LinkTextID == 1 ){
			//ウィンドウ・フルスクリーン切り替え
			if( Options_FullscreenFlag_old != GameConfig.GetFullscreenFlag() ){
				if( ChangeWindowMode(WindowCtrl, d3dg, inputCtrl, this, GameConfig.GetFullscreenFlag()) != 0 ){
					//失敗したら設定を戻す
					GameConfig.SetFullscreenFlag(Options_FullscreenFlag_old);
				}
				else{
					//成功したら設定を反映する
					Options_FullscreenFlag_old = GameConfig.GetFullscreenFlag();
				}
			}

			//効果音再設定
			float volume;
			if( GameConfig.GetSoundFlag() == false ){
				volume = 0.0f;
			}
			else{
				volume = 1.0f;
			}
			SoundCtrl.SetVolume(volume);

			//設定ファイル書き込み
			if( GameConfig.SaveFile("config.dat") == 1 ){
				//MainWindow.ErrorInfo("config data save failed");
				//return 1;
			}

			Options_KeyConfigID = -1;
			Options_KeyConfigCnt = 0;

			modescreen = 1;
		}

		//Cancel
		if( Options_p2LinkTextID == 2 ){
			//設定ファイル読み直す
			if( GameConfig.LoadFile("config.dat") == 1 ){
				//MainWindow.ErrorInfo("config data open failed");
				//return 1;
			}

			Options_FullscreenFlag_old = GameConfig.GetFullscreenFlag();
			Options_KeyConfigID = -1;
			Options_KeyConfigCnt = 0;

			modescreen = 1;
		}

		if( Options_p2LinkTextID == 3 ){
			int param = GameConfig.GetMouseSensitivity();
			param -= 1;
			if( param < 10 ){ param = 10; }
			GameConfig.SetMouseSensitivity(param);
		}
		if( Options_p2LinkTextID == 4 ){
			int param = GameConfig.GetMouseSensitivity();
			param += 1;
			if( param > 50 ){ param = 50; }
			GameConfig.SetMouseSensitivity(param);
		}
		if( Options_p2LinkTextID == 5 ){
			int param = GameConfig.GetBrightness();
			param -= 1;
			if( param < 0 ){ param = 0; }
			GameConfig.SetBrightness(param);
		}
		if( Options_p2LinkTextID == 6 ){
			int param = GameConfig.GetBrightness();
			param += 1;
			if( param > 10 ){ param = 10; }
			GameConfig.SetBrightness(param);
		}

		if( Options_p2LinkTextID == 7 ){
			GameConfig.SetFullscreenFlag(true);
		}
		if( Options_p2LinkTextID == 8 ){
			GameConfig.SetFullscreenFlag(false);
		}
		if( Options_p2LinkTextID == 9 ){
			if( GameConfig.GetSoundFlag() == true ){ GameConfig.SetSoundFlag(false); }
			else { GameConfig.SetSoundFlag(true); }
		}
		if( Options_p2LinkTextID == 10 ){
			if( GameConfig.GetBloodFlag() == true ){ GameConfig.SetBloodFlag(false); }
			else { GameConfig.SetBloodFlag(true); }
		}
		if( Options_p2LinkTextID == 11 ){
			if( GameConfig.GetInvertMouseFlag() == true ){ GameConfig.SetInvertMouseFlag(false); }
			else { GameConfig.SetInvertMouseFlag(true); }
		}
		if( Options_p2LinkTextID == 12 ){
			if( GameConfig.GetFrameskipFlag() == true ){ GameConfig.SetFrameskipFlag(false); }
			else { GameConfig.SetFrameskipFlag(true); }
		}
		if( Options_p2LinkTextID == 13 ){
			if( GameConfig.GetAnotherGunsightFlag() == true ){ GameConfig.SetAnotherGunsightFlag(false); }
			else { GameConfig.SetAnotherGunsightFlag(true); }
		}

		//キー設定用　カウントセット
		for(int i=0; i<18; i++){
			if( Options_p2LinkTextID == (14 + i) ){
				Options_KeyConfigID = i;
				Options_KeyConfigCnt = (int)(GAMEFPS*5);
			}
		}

		//キー設定用　カウントダウン
		if( (Options_KeyConfigID != -1)&&(Options_KeyConfigCnt > 0) ){
			Options_KeyConfigCnt -= 1;
		}
		else{
			Options_KeyConfigID = -1;
			Options_KeyConfigCnt = 0;
		}
	}
}

//! @brief オプション画面の描画処理
void mainmenu::RenderOptions()
{
	if( modescreen == 1 ){
		int color;

		//エリア描画
		d3dg->Draw2DBox(OPTIONS_P1_X, OPTIONS_P1_Y, OPTIONS_P1_X + OPTIONS_P1_W, OPTIONS_P1_Y + OPTIONS_P1_H, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));

		for(int i=0; i<OPTIONS_P1_DATAS; i++){
			if( Options_p1LinkTextData[i].enable == true ){
				//文字の色を設定
				if( ((OPTIONS_P1_X + Options_p1LinkTextData[i].pos_x) < mainmenu_mouseX)&&(mainmenu_mouseX < (OPTIONS_P1_X + Options_p1LinkTextData[i].pos_x + (signed)strlen(Options_p1LinkTextData[i].text)*Options_p1LinkTextData[i].text_w))
					&&((OPTIONS_P1_Y + Options_p1LinkTextData[i].pos_y) < mainmenu_mouseY)&&(mainmenu_mouseY < (OPTIONS_P1_Y + Options_p1LinkTextData[i].pos_y + Options_p1LinkTextData[i].text_h))
				){
					color = Options_p1LinkTextData[i].text_cursorcolor;
				}
				else{
					color = Options_p1LinkTextData[i].text_defaultcolor;
				}

				//文字を表示
				d3dg->Draw2DTextureFontText(OPTIONS_P1_X + Options_p1LinkTextData[i].pos_x +1, OPTIONS_P1_Y + Options_p1LinkTextData[i].pos_y +1, Options_p1LinkTextData[i].text,
					d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), Options_p1LinkTextData[i].text_w, Options_p1LinkTextData[i].text_h);
				d3dg->Draw2DTextureFontText(OPTIONS_P1_X + Options_p1LinkTextData[i].pos_x, OPTIONS_P1_Y + Options_p1LinkTextData[i].pos_y, Options_p1LinkTextData[i].text,
					color, Options_p1LinkTextData[i].text_w, Options_p1LinkTextData[i].text_h);
			}
			else{
				d3dg->Draw2DTextureFontText(OPTIONS_P1_X + Options_p1LinkTextData[i].pos_x, OPTIONS_P1_Y + Options_p1LinkTextData[i].pos_y, Options_p1LinkTextData[i].text,
					d3dg->GetColorCode(0.6f,0.6f,0.6f,1.0f), Options_p1LinkTextData[i].text_w, Options_p1LinkTextData[i].text_h);
			}
		}
	}

	if( modescreen == 2 ){
		int color;
		char str[32];

		//エリア描画
		d3dg->Draw2DBox(OPTIONS_P2_X, OPTIONS_P2_Y, OPTIONS_P2_X + OPTIONS_P2_W, OPTIONS_P2_Y + OPTIONS_P2_H, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));

		//マウス感度
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 350 +1, OPTIONS_P2_Y + 0 +1, "MOUSE SENSITIVITY", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 350, OPTIONS_P2_Y + 0, "MOUSE SENSITIVITY", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		sprintf(str, "%02d", GameConfig.GetMouseSensitivity());
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 350 + 85 +1, OPTIONS_P2_Y + 20 +1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 350 + 85, OPTIONS_P2_Y + 20, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);

		//画面明るさ
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 350 +1, OPTIONS_P2_Y + 50 +1, "BRIGHTNESS", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 350, OPTIONS_P2_Y + 50, "BRIGHTNESS", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		sprintf(str, "%02d", GameConfig.GetBrightness());
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 350 + 85 +1, OPTIONS_P2_Y + 70 +1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 350 + 85, OPTIONS_P2_Y + 70, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);

		//操作キー一覧
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 0 +1, "TURN UP", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 0, "TURN UP", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 20 +1, "TURN DOWN", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 20, "TURN DOWN", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 40 +1, "TURN LEFT", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 40, "TURN LEFT", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 60 +1, "TURN RIGHT", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 60, "TURN RIGHT", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 80 +1, "MOVE FORWARD", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 80, "MOVE FORWARD", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 100 +1, "MOVE BACKWARD", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 100, "MOVE BACKWARD", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 120 +1, "MOVE LEFT", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 120, "MOVE LEFT", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 140 +1, "MOVE RIGHT", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 140, "MOVE RIGHT", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 160 +1, "WALK", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 160, "WALK", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 180 +1, "JUMP", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 180, "JUMP", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 200 +1, "RELOAD", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 200, "RELOAD", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 220 +1, "DROP WEAPON", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 220, "DROP WEAPON", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 240 +1, "ZOOM", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 240, "ZOOM", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 260 +1, "FIRE MODE", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 260, "FIRE MODE", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 280 +1, "SWITCH WEAPON", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 280, "SWITCH WEAPON", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 300 +1, "WEAPON 1", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 300, "WEAPON 1", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 320 +1, "WEAPON 2", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 320, "WEAPON 2", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0 +1, OPTIONS_P2_Y + 340 +1, "FIRE", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 12, 16);
		d3dg->Draw2DTextureFontText(OPTIONS_P2_X + 0, OPTIONS_P2_Y + 340, "FIRE", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 12, 16);
		for(int i=0; i<18; i++){
			if( Options_KeyConfigID == i ){
				sprintf(Options_p2LinkTextData[14 + i].text, "[(%d)]", (int)((float)Options_KeyConfigCnt/GAMEFPS)+1 );
			}
			else{
				sprintf(Options_p2LinkTextData[14 + i].text, "[%s]", GameConfig.GetOriginalkeycodeString( GameConfig.GetKeycode(i) ) );
			}
		}

		//項目名設定
		if( GameConfig.GetFullscreenFlag() == true ){
			Options_p2LinkTextData[7].text[1] = '+';
			Options_p2LinkTextData[8].text[1] = ' ';
		}
		else{
			Options_p2LinkTextData[7].text[1] = ' ';
			Options_p2LinkTextData[8].text[1] = '+';
		}
		if( GameConfig.GetSoundFlag() == true ){ Options_p2LinkTextData[9].text[1] = '*'; }
		else{ Options_p2LinkTextData[9].text[1] = ' '; }
		if( GameConfig.GetBloodFlag() == true ){ Options_p2LinkTextData[10].text[1] = '*'; }
		else{ Options_p2LinkTextData[10].text[1] = ' '; }
		if( GameConfig.GetInvertMouseFlag() == true ){ Options_p2LinkTextData[11].text[1] = '*'; }
		else{ Options_p2LinkTextData[11].text[1] = ' '; }
		if( GameConfig.GetFrameskipFlag() == true ){ Options_p2LinkTextData[12].text[1] = '*'; }
		else{ Options_p2LinkTextData[12].text[1] = ' '; }
		if( GameConfig.GetAnotherGunsightFlag() == true ){ Options_p2LinkTextData[13].text[1] = '*'; }
		else{ Options_p2LinkTextData[13].text[1] = ' '; }

		for(int i=0; i<OPTIONS_P2_DATAS; i++){
			if( Options_p2LinkTextData[i].enable == true ){
				//文字の色を設定
				if( ((OPTIONS_P2_X + Options_p2LinkTextData[i].pos_x) < mainmenu_mouseX)&&(mainmenu_mouseX < (OPTIONS_P2_X + Options_p2LinkTextData[i].pos_x + (signed)strlen(Options_p2LinkTextData[i].text)*Options_p2LinkTextData[i].text_w))
					&&((OPTIONS_P2_Y + Options_p2LinkTextData[i].pos_y) < mainmenu_mouseY)&&(mainmenu_mouseY < (OPTIONS_P2_Y + Options_p2LinkTextData[i].pos_y + Options_p2LinkTextData[i].text_h))
				){
					color = Options_p2LinkTextData[i].text_cursorcolor;
				}
				else{
					color = Options_p2LinkTextData[i].text_defaultcolor;
				}

				//文字を表示
				d3dg->Draw2DTextureFontText(OPTIONS_P2_X + Options_p2LinkTextData[i].pos_x +1, OPTIONS_P2_Y + Options_p2LinkTextData[i].pos_y +1, Options_p2LinkTextData[i].text,
					d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), Options_p2LinkTextData[i].text_w, Options_p2LinkTextData[i].text_h);
				d3dg->Draw2DTextureFontText(OPTIONS_P2_X + Options_p2LinkTextData[i].pos_x, OPTIONS_P2_Y + Options_p2LinkTextData[i].pos_y, Options_p2LinkTextData[i].text,
					color, Options_p2LinkTextData[i].text_w, Options_p2LinkTextData[i].text_h);
			}
			else{
				d3dg->Draw2DTextureFontText(OPTIONS_P2_X + Options_p2LinkTextData[i].pos_x, OPTIONS_P2_Y + Options_p2LinkTextData[i].pos_y, Options_p2LinkTextData[i].text,
					d3dg->GetColorCode(0.6f,0.6f,0.6f,1.0f), Options_p2LinkTextData[i].text_w, Options_p2LinkTextData[i].text_h);
			}
		}
	}

	if( modescreen == 3 ){
		int color;
		int maxline = 18;

		//韓国語であれば表示行数を減らす
		if( GameConfig.GetLanguage() == 1 ){
			maxline = 14;
		}

		//エリア描画
		d3dg->Draw2DBox(OPTIONS_P3_X, OPTIONS_P3_Y, OPTIONS_P3_X + OPTIONS_P3_W, OPTIONS_P3_Y + OPTIONS_P3_H, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));

		d3dg->Draw2DTextureFontText(OPTIONS_P3_X + 0 +1, OPTIONS_P3_Y + 15 +1, "SOFTWARE LICENSE", d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 18, 24);
		d3dg->Draw2DTextureFontText(OPTIONS_P3_X + 0, OPTIONS_P3_Y + 15, "SOFTWARE LICENSE", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 18, 24);

		d3dg->Draw2DBox(OPTIONS_P3_X + 0, OPTIONS_P3_Y + 55, OPTIONS_P3_X + OPTIONS_P3_W-20, OPTIONS_P3_Y + 55 + 18*12, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.7f));

		//ライセンス文描画
		int scrollline = scrollbar_license.GetScrollItem();
		strcpy(licenseinfobuf, "");
#if OPTIONS_LICENSELINE > maxline
		for(int i=0; i<maxline; i++){
#else
		for(int i=0; i<OPTIONS_LICENSELINE; i++){
#endif
			strcat(licenseinfobuf, licenseinfo[scrollline + i]);
			strcat(licenseinfobuf, "\n");
		}
		d3dg->Draw2DMSSmallFontText(OPTIONS_P3_X + 5, OPTIONS_P3_Y+55, licenseinfobuf, d3dg->GetColorCode(0.8f,0.8f,0.8f,1.0f));

		//スクロールバー描画
		scrollbar_license.Draw(d3dg);

		for(int i=0; i<OPTIONS_P3_DATAS; i++){
			if( Options_p3LinkTextData[i].enable == true ){
				//文字の色を設定
				if( ((OPTIONS_P3_X + Options_p3LinkTextData[i].pos_x) < mainmenu_mouseX)&&(mainmenu_mouseX < (OPTIONS_P3_X + Options_p3LinkTextData[i].pos_x + (signed)strlen(Options_p3LinkTextData[i].text)*Options_p3LinkTextData[i].text_w))
					&&((OPTIONS_P3_Y + Options_p3LinkTextData[i].pos_y) < mainmenu_mouseY)&&(mainmenu_mouseY < (OPTIONS_P3_Y + Options_p3LinkTextData[i].pos_y + Options_p3LinkTextData[i].text_h))
				){
					color = Options_p3LinkTextData[i].text_cursorcolor;
				}
				else{
					color = Options_p3LinkTextData[i].text_defaultcolor;
				}

				//文字を表示
				d3dg->Draw2DTextureFontText(OPTIONS_P3_X + Options_p3LinkTextData[i].pos_x +1, OPTIONS_P3_Y + Options_p3LinkTextData[i].pos_y +1, Options_p3LinkTextData[i].text,
					d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), Options_p3LinkTextData[i].text_w, Options_p3LinkTextData[i].text_h);
				d3dg->Draw2DTextureFontText(OPTIONS_P3_X + Options_p3LinkTextData[i].pos_x, OPTIONS_P3_Y + Options_p3LinkTextData[i].pos_y, Options_p3LinkTextData[i].text,
					color, Options_p3LinkTextData[i].text_w, Options_p3LinkTextData[i].text_h);
			}
			else{
				d3dg->Draw2DTextureFontText(OPTIONS_P3_X + Options_p3LinkTextData[i].pos_x, OPTIONS_P3_Y + Options_p3LinkTextData[i].pos_y, Options_p3LinkTextData[i].text,
					d3dg->GetColorCode(0.6f,0.6f,0.6f,1.0f), Options_p3LinkTextData[i].text_w, Options_p3LinkTextData[i].text_h);
			}
		}
	}
}
#endif	//ENABLE_MENUOPTIONS

void mainmenu::Destroy()
{
	//ブロックデータ解放
	d3dg->CleanupMapdata();

	//オブジェクトマネージャー解放
	ObjMgr.Cleanup();

	//背景空解放
	Resource.CleanupSkyModelTexture();

	d3dg->CleanupTexture(gametitle);

	GameState->NextState();
}

//! @brief コンストラクタ
briefing::briefing()
{
	TwoTexture = false;
	TextureA = -1;
	TextureB = -1;
}

//! @brief ディストラクタ
briefing::~briefing()
{}

int briefing::Create()
{
	char path[MAX_PATH];
	char pdata[MAX_PATH];
	char PictureA[MAX_PATH];
	char PictureB[MAX_PATH];

	//背景画像を取得
	gametitle = d3dg->LoadTexture("data\\title.dds", false, false);

	//mifファイルのファイルパス取得
	if( GameInfoData.selectmission_id >= 0 ){
		//[OpenXOPS-web] main-list free missions may be .mif files
		GameParamInfo.GetOfficialMissionBriefingPath(GameInfoData.selectmission_id, path);
	}
	else{
		strcpy(path, GameInfoData.mifpath);
	}

	//mifファイルを読み込み
	if( MIFdata.LoadFiledata(path, true) != 0 ){
		//briefing data open failed
		return 1;
	}

	//ブリーフィング画像のファイルパス取得
	MIFdata.GetPicturefilePath(PictureA, PictureB);

	//ブリーフィング画像読み込み
	if( strcmp(PictureB, "!") == 0 ){
		TwoTexture = false;
		TextureA = d3dg->LoadTexture(PictureA, true, false);
		TextureB = -1;
	}
	else{
		TwoTexture = true;
		TextureA = d3dg->LoadTexture(PictureA, true, false);
		TextureB = d3dg->LoadTexture(PictureB, true, false);
	}

	//マウスカーソルを中央へ移動
	inputCtrl->MoveMouseCenter();
	framecnt = 0;

	GameState->NextState();
	return 0;
}

int briefing::Recovery()
{
	char PictureA[MAX_PATH];
	char PictureB[MAX_PATH];

	//背景画像を取得
	gametitle = d3dg->LoadTexture("data\\title.dds", false, false);

	//ブリーフィング画像のファイルパス取得
	MIFdata.GetPicturefilePath(PictureA, PictureB);

	//ブリーフィング画像読み込み
	if( strcmp(PictureB, "!") == 0 ){
		TwoTexture = false;
		TextureA = d3dg->LoadTexture(PictureA, true, false);
		TextureB = -1;
	}
	else{
		TwoTexture = true;
		TextureA = d3dg->LoadTexture(PictureA, true, false);
		TextureB = d3dg->LoadTexture(PictureB, true, false);
	}

	return 0;
}

void briefing::Render2D()
{
	float effectA = GetEffectAlphaLoop(framecnt, 0.8f, 0.7f, true);
	float effectB = GetEffectAlphaLoop(framecnt, 0.8f, 1.0f, true);
	int effectB_sizeW = (int)( (float)(framecnt%((int)(GAMEFPS*1.0f))) * 0.2f + 18 );
	int effectB_sizeH = (int)( (float)(framecnt%((int)(GAMEFPS*1.0f))) * 1.0f + 26 );

	//メモ：背景画像の描画は、自動的に行われる。

	//固定文字表示
	d3dg->Draw2DTextureFontTextCenterScaling(0, 30, "BRIEFING", d3dg->GetColorCode(1.0f,1.0f,0.0f,effectA), 60, 42);
	d3dg->Draw2DTextureFontText(GameConfig.GetScreenWidth() - 210 - effectB_sizeW*20/2, GameConfig.GetScreenHeight() - 37 - effectB_sizeH/2,
								"LEFT CLICK TO BEGIN", d3dg->GetColorCode(1.0f,1.0f,1.0f,effectB), effectB_sizeW, effectB_sizeH);
	d3dg->Draw2DTextureFontText(GameConfig.GetScreenWidth() - 210 - 18*20/2, GameConfig.GetScreenHeight() - 37 - 26/2, "LEFT CLICK TO BEGIN", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 18, 26);

	//ブリーフィング画像描画
	if( TwoTexture == false ){
		if( TextureA == -1 ){ d3dg->Draw2DBoxScaling(40, 180, 40+160, 180+150, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f)); }
		else{ d3dg->Draw2DTextureScaling(40, 180, TextureA, 160, 150, 1.0f); }
	}
	else{
		if( TextureA == -1 ){ d3dg->Draw2DBoxScaling(40, 130, 40+160, 130+150, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f)); }
		else{ d3dg->Draw2DTextureScaling(40, 130, TextureA, 160, 150, 1.0f); }

		if( TextureB == -1 ){ d3dg->Draw2DBoxScaling(40, 300, 40+160, 300+150, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f)); }
		else{ d3dg->Draw2DTextureScaling(40, 300, TextureB, 160, 150, 1.0f); }
	}

	//ミッション名を取得・表示
	char mname[64];
	if( MIFdata.GetFiletype() == false ){
		GameParamInfo.GetOfficialMission(GameInfoData.selectmission_id, NULL, mname, NULL, NULL, NULL, NULL);
	}
	else{
		strcpy(mname, MIFdata.GetMissionFullname());
	}
	d3dg->Draw2DTextureFontTextCenterScaling(0, 90, mname, d3dg->GetColorCode(1.0f,0.5f,0.0f,1.0f), 18, 25);

	//ミッション説明を表示
	d3dg->Draw2DMSFontTextScaling(230, 180, MIFdata.GetBriefingText(), d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f));
}

void briefing::Destroy()
{
	//ブリーフィング画像を解放
	d3dg->CleanupTexture(TextureA);
	d3dg->CleanupTexture(TextureB);

	//背景画像を解放
	d3dg->CleanupTexture(gametitle);

	GameState->NextState();
}

//! @brief コンストラクタ
maingame::maingame()
{
	SkyNumber = 0;
	DarkScreenFlag = false;
	FogFlag = true;
	mouse_rx = 0.0f;
	mouse_ry = 0.0f;
	view_rx = 0.0f;
	view_ry = 0.0f;
	add_camera_rx = 0.0f;
	add_camera_ry = 0.0f;
	ShowInfo_Debugmode = false;
	Camera_Debugmode = false;
	tag = false;
	radar = false;
	wireframe = false;
	nomodel = false;
	CenterLine = false;
	Camera_Blind = true;
	Camera_F1mode = false;
	Camera_F2mode = 0;
	Camera_HOMEmode = GameConfig.GetShowArmFlag();
	Cmd_F5 = false;
	InvincibleID = -1;
	PlayerAI = false;
	AIstop = false;
	AINoFight = false;
	AIdebuginfoID = -1;
	start_framecnt = 0;
	end_framecnt = 0;
	EventStop = false;
	for(int i=0; i<TOTAL_EVENTLINE; i++){ EventSkip[i] = false; }
	GameSpeed = 1;
	message_id = -1;
	message_cnt = 0;
	redflash_flag = false;
	time = 0;
	time_input = 0;
	time_process_object = 0;
	time_process_ai = 0;
	time_process_event = 0;
	time_sound = 0;
	time_render = 0;
	memcpy(&MainGameInfo, &GameInfoData, sizeof(GameInfo));

#ifdef ENABLE_DEBUGCONSOLE
	Show_Console = false;
	InfoConsoleData = NULL;
	NewCommand[0] = '\0';
	ScreenShot = 0;
#endif
}

//! @brief ディストラクタ
maingame::~maingame()
{}

//! @brief 座標などを表示するデバックモードのフラグ設定
void maingame::SetShowInfoFlag(bool flag)
{
	ShowInfo_Debugmode = flag;
}

//! @brief ゲームの実行速度を取得
int maingame::GetGameSpeed()
{
	return GameSpeed;
}

int maingame::Create()
{
	memcpy(&MainGameInfo, &GameInfoData, sizeof(GameInfo));
	char path[MAX_PATH];
	char bdata[MAX_PATH];
	char pdata[MAX_PATH];
	char pdata2[MAX_PATH];
	int blockflag, pointflag;
	bool collisionflag;

	//.bd1と.pd1のファイルパスを求める
	if( MIFdata.GetFiletype() == false ){
		GameParamInfo.GetOfficialMission(MainGameInfo.selectmission_id, NULL, NULL, path, pdata2, &collisionflag, &DarkScreenFlag);

		strcpy(bdata, path);
		strcat(bdata, OFFICIALMISSION_BD1);
		strcpy(pdata, path);
		strcat(pdata, pdata2);
		strcat(pdata, ".pd1");
	}
	else{
		MIFdata.GetDatafilePath(bdata, pdata);
		collisionflag = MIFdata.GetCollisionFlag();
		DarkScreenFlag = MIFdata.GetScreenFlag();

		GetFileDirectory(bdata, path);
	}

	//追加小物を読み込む
	for(int i=0; i<MAX_ADDSMALLOBJECT; i++){
		Resource.LoadAddSmallObject(i, MIFdata.GetAddSmallobjectModelPath(i), MIFdata.GetAddSmallobjectTexturePath(i), MIFdata.GetAddSmallobjectSoundPath(i));
	}

	//ブロックデータ読み込み
	blockflag = BlockData.LoadFiledata(bdata);

	//ポイントデータ読み込み
	pointflag = PointData.LoadFiledata(pdata);

	//ファイル読み込みでエラーがあったら中断
	if( (blockflag != 0)||(pointflag != 0) ){
		//2bit : point data open failed
		//1bit : block data open failed
		return pointflag*2 + blockflag;
	}

	//ブロックデータ初期化
	BlockData.CalculationBlockdata(DarkScreenFlag);
	d3dg->LoadMapdata(&BlockData, path);
	CollD.InitCollision(&BlockData);

	//ポイントデータ初期化
	ObjMgr.LoadPointData();

	//追加の当たり判定設定
	ObjMgr.SetAddCollisionFlag(collisionflag);

	//AI設定
	for(int i=0; i<MAX_HUMAN; i++){
		HumanAI[i].SetClass(&ObjMgr, i, &BlockData, &PointData, &GameParamInfo, &CollD, GameSound);
		HumanAI[i].Init();
	}


	//背景空読み込み
	SkyNumber = MIFdata.GetSkynumber();
	Resource.LoadSkyModelTexture(SkyNumber);

	//サウンド初期化
	GameSound->InitWorldSound();

	//イベント初期化
	for(int i=0; i<TOTAL_EVENTLINE; i++){
		Event[i].SetClass(&PointData, &ObjMgr);
	}
	Event[0].Reset(TOTAL_EVENTENTRYPOINT_0);
	Event[1].Reset(TOTAL_EVENTENTRYPOINT_1);
	Event[2].Reset(TOTAL_EVENTENTRYPOINT_2);

	//[XOPS-ONLINE] an armed host/client session takes over this mission
	g_netSession.Attach(&ObjMgr, &PointData);
	if( g_netSession.IsActive() ){ ShowInfo_Debugmode = false; }	//direct-launch debug overlay is for mission testing, not matches

	//プレイヤーの向きを取得
	ObjMgr.GetPlayerHumanObject()->GetRxRy(&mouse_rx, &mouse_ry);

	FogFlag = true;
	view_rx = 0.0f;
	view_ry = 0.0f;
	add_camera_rx = 0.0f;
	add_camera_ry = 0.0f;
	Camera_Debugmode = false;
	tag = false;
	radar = false;
	wireframe = false;
	nomodel = false;
	CenterLine = false;
	Camera_Blind = true;
	Camera_F1mode = false;
	InvincibleID = -1;
	PlayerAI = false;
	AIstop = false;
	AINoFight = false;
	AIdebuginfoID = -1;
	framecnt = 0;
	start_framecnt = 0;
	end_framecnt = 0;
	EventStop = false;
	for(int i=0; i<TOTAL_EVENTLINE; i++){ EventSkip[i] = false; }
	GameSpeed = 1;
	message_id = -1;
	message_cnt = 0;
	redflash_flag = false;
	MainGameInfo.missioncomplete = false;
	MainGameInfo.fire = 0;
	MainGameInfo.ontarget = 0;
	MainGameInfo.kill = 0;
	MainGameInfo.headshot = 0;

#ifdef ENABLE_DEBUGCONSOLE
	Show_Console = false;
	ScreenShot = 0;

	//コンソール用初期化
	InfoConsoleData = new ConsoleData [MAX_CONSOLELINES];
	for(int i=0; i<MAX_CONSOLELINES; i++){
		InfoConsoleData[i].color = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
		InfoConsoleData[i].textdata[0] = '\0';
	}
	AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Game Debug Console.");
	AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "        Command list >help");
	ConsoleTextBoxControl.InitTextBox("", MAX_CONSOLELEN - (int)strlen(CONSOLE_PROMPT), 0x01|0x04|0x20);
#endif

	//マウスカーソルを中央へ移動
	inputCtrl->MoveMouseCenter();

	GameState->NextState();
	return 0;
}

int maingame::Recovery()
{
	char path[MAX_PATH];
	char bdata[MAX_PATH];	//ダミー
	char pdata[MAX_PATH];	//ダミー

	//.bd1と.pd1のファイルパスを求める
	if( MIFdata.GetFiletype() == false ){
		GameParamInfo.GetOfficialMission(MainGameInfo.selectmission_id, NULL, NULL, path, NULL, NULL, NULL);
	}
	else{
		MIFdata.GetDatafilePath(bdata, pdata);
		GetFileDirectory(bdata, path);
	}

	//追加小物を解放⇒読み込む
	for(int i=0; i<MAX_ADDSMALLOBJECT; i++){
		Resource.CleanupAddSmallObject(i);
		Resource.LoadAddSmallObject(i, MIFdata.GetAddSmallobjectModelPath(i), MIFdata.GetAddSmallobjectTexturePath(i), MIFdata.GetAddSmallobjectSoundPath(i));
	}

	//ブロックデータ初期化
	d3dg->LoadMapdata(&BlockData, path);

	//ポイントデータ初期化
	ObjMgr.Recovery();

	//背景空初期化
	Resource.CleanupSkyModelTexture();
	Resource.LoadSkyModelTexture(SkyNumber);

	return 0;
}

//! @brief 特定操作の入力をチェック
bool maingame::CheckInputControl(int CheckKey, int mode)
{
	int KeyCode = OriginalkeycodeToDinputdef(GameConfig.GetKeycode(CheckKey));
	if( KeyCode == -1 ){
		if( mode == 0 ){
			return inputCtrl->CheckMouseButtonNowL();
		}
		if( mode == 1 ){
			return inputCtrl->CheckMouseButtonDownL();
		}
		//if( mode == 2 ){
			return inputCtrl->CheckMouseButtonUpL();
		//}
	}
	if( KeyCode == -2 ){
		if( mode == 0 ){
			return inputCtrl->CheckMouseButtonNowR();
		}
		if( mode == 1 ){
			return inputCtrl->CheckMouseButtonDownR();
		}
		//if( mode == 2 ){
			return inputCtrl->CheckMouseButtonUpR();
		//}
	}
	if( KeyCode == -3 ){
		int CodeL, CodeR;
		GetDoubleKeyCode(0, &CodeL, &CodeR);
		if( mode == 0 ){
			if( inputCtrl->CheckKeyNow(CodeL) ){
				return true;
			}
			return inputCtrl->CheckKeyNow(CodeR);
		}
		if( mode == 1 ){
			if( inputCtrl->CheckKeyDown(CodeL) ){
				return true;
			}
			return inputCtrl->CheckKeyDown(CodeR);
		}
		//if( mode == 2 ){
			if( inputCtrl->CheckKeyUp(CodeL) ){
				return true;
			}
			return inputCtrl->CheckKeyUp(CodeR);
		//}
	}
	if( KeyCode == -4 ){
		int CodeL, CodeR;
		GetDoubleKeyCode(1, &CodeL, &CodeR);
		if( mode == 0 ){
			if( inputCtrl->CheckKeyNow(CodeL) ){
				return true;
			}
			return inputCtrl->CheckKeyNow(CodeR);
		}
		if( mode == 1 ){
			if( inputCtrl->CheckKeyDown(CodeL) ){
				return true;
			}
			return inputCtrl->CheckKeyDown(CodeR);
		}
		//if( mode == 2 ){
			if( inputCtrl->CheckKeyUp(CodeL) ){
				return true;
			}
			return inputCtrl->CheckKeyUp(CodeR);
		//}
	}

	if( mode == 0 ){
		return inputCtrl->CheckKeyNow( OriginalkeycodeToDinputdef(GameConfig.GetKeycode(CheckKey)) );
	}
	if( mode == 1 ){
		return inputCtrl->CheckKeyDown( OriginalkeycodeToDinputdef(GameConfig.GetKeycode(CheckKey)) );
	}
	//if( mode == 2 ){
		return inputCtrl->CheckKeyUp( OriginalkeycodeToDinputdef(GameConfig.GetKeycode(CheckKey)) );
	//}
}

void maingame::Input()
{
	time = GetTimeMS();

	//プレイヤーのクラスを取得
	human *myHuman = ObjMgr.GetPlayerHumanObject();

	//キー入力を取得
	inputCtrl->GetInputState(true);
	inputCtrl->MoveMouseCenter();

	//マウスの移動量取得
	int x, y;
	float MouseSensitivity;
	inputCtrl->GetMouseMovement(&x, &y);

	//視点の移動量計算
	ScopeParameter sparam;
	GameParamInfo.GetScopeParam(myHuman->GetScopeMode(), &sparam);
	MouseSensitivity = DegreeToRadian(1) * sparam.MouseRange * GameConfig.GetMouseSensitivity();

	//マウス反転（オプション設定）が有効ならば、反転する。
	if( GameConfig.GetInvertMouseFlag() == true ){
		y *= -1;
	}

	if( inputCtrl->CheckKeyDown(GetEscKeycode()) ){					//ゲーム終了操作かチェック
		GameState->PushBackSpaceKey();
		GameSpeed = 1;
	}
	else if( inputCtrl->CheckKeyDown( GetFunctionKeycode(12) ) ){	//リセット操作かチェック
		GameState->PushF12Key();
		GameSpeed = 1;
	}

	//画面のUI変更操作かチェック
	if( inputCtrl->CheckKeyDown( GetFunctionKeycode(2) ) ){
		if( Camera_F2mode == 2 ){
			Camera_F2mode = 0;
		}
		else{
			Camera_F2mode += 1;
		}
	}

#ifdef ENABLE_DEBUGCONSOLE
	if( Show_Console == false ){
#endif

		if( Camera_Debugmode == false ){	//通常モードならば
			if( PlayerAI == false ){
				InputPlayer(myHuman, x, y, MouseSensitivity);
			}
		}
		else{								//デバックモードならば
			InputViewCamera(x, y, MouseSensitivity);
		}

#ifdef ENABLE_DEBUGCONSOLE
	}

	//デバック用コンソールの表示操作かチェック
	if( inputCtrl->CheckKeyDown( GetFunctionKeycode(11) ) ){
		if( Show_Console == false ){
			Show_Console = true;

			ConsoleTextBoxControl.SetTextBoxStr("");
		}
		else{
			Show_Console = false;
		}
	}

	if( Show_Console == true ){
		InputConsole();
	}
#endif

	time_input = GetTimeMS() - time;
}

//! @brief プレイヤー操作系の入力処理
//! @param myHuman プレイヤーのクラス
//! @param mouse_x マウスのX座標
//! @param mouse_y マウスのY座標 
//! @param MouseSensitivity 視点の移動量計算
void maingame::InputPlayer(human *myHuman, int mouse_x, int mouse_y, float MouseSensitivity)
{
	if( myHuman == NULL ){ return; }

	int PlayerID = ObjMgr.GetPlayerID();

	if( myHuman->GetHP() > 0 ){

		//マウスによる向きを計算
		mouse_rx += mouse_x * MouseSensitivity;
		mouse_ry -= mouse_y * MouseSensitivity;

		//キー操作による向きを計算
		if(      (CheckInputControl(KEY_TURNUP, 0) == true)&&(CheckInputControl(KEY_TURNDOWN, 0) == false) ){ add_camera_ry += (INPUT_ARROWKEYS_ANGLE - add_camera_ry)*0.2f; }			// 標準：[↑]
		else if( (CheckInputControl(KEY_TURNDOWN, 0) == true)&&(CheckInputControl(KEY_TURNUP, 0) == false) ){ add_camera_ry += (INPUT_ARROWKEYS_ANGLE*-1 - add_camera_ry)*0.2f; }		// 標準：[↓]
		else { add_camera_ry = 0.0f; }
		if(      (CheckInputControl(KEY_TURNLEFT, 0) == true)&&(CheckInputControl(KEY_TURNRIGHT, 0) == false) ){ add_camera_rx += (INPUT_ARROWKEYS_ANGLE*-1 - add_camera_rx)*0.2f; }	// 標準：[←]
		else if( (CheckInputControl(KEY_TURNRIGHT, 0) == true)&&(CheckInputControl(KEY_TURNLEFT, 0) == false) ){ add_camera_rx += (INPUT_ARROWKEYS_ANGLE - add_camera_rx)*0.2f; }		// 標準：[→]
		else { add_camera_rx = 0.0f; }
		mouse_rx += add_camera_rx;
		mouse_ry += add_camera_ry;

		if( mouse_ry > DegreeToRadian(70) ) mouse_ry = DegreeToRadian(70);
		if( mouse_ry < DegreeToRadian(-70) ) mouse_ry = DegreeToRadian(-70);


		//[XOPS-ONLINE] keyboard/mouse -> HumanCommand -> ObjectManager::ApplyHumanCommand
		//(network peers drive their humans through the same function)
		HumanCommand cmd;
		cmd.held = 0;
		cmd.pressed = 0;
		cmd.rx = mouse_rx;
		cmd.ry = mouse_ry;
		static const struct { int key; unsigned short bit; } bindings[] = {
			{KEY_MOVEFORWARD, HCMD_FORWARD}, {KEY_MOVEBACKWARD, HCMD_BACK}, {KEY_MOVELEFT, HCMD_LEFT}, {KEY_MOVERIGHT, HCMD_RIGHT},
			{KEY_WALK, HCMD_WALK}, {KEY_JUMP, HCMD_JUMP}, {KEY_SHOT, HCMD_FIRE}, {KEY_RELOAD, HCMD_RELOAD},
			{KEY_SWITCHWEAPON, HCMD_SWITCH}, {KEY_WEAPON1, HCMD_WEAPON1}, {KEY_WEAPON2, HCMD_WEAPON2},
			{KEY_DROPWEAPON, HCMD_DROP}, {KEY_ZOOMSCOPE, HCMD_SCOPE}, {KEY_SHOTMODE, HCMD_SHOTMODE},
		};
		for(unsigned int i=0; i<sizeof(bindings)/sizeof(bindings[0]); i++){
			if( CheckInputControl(bindings[i].key, 0) ){ cmd.held |= bindings[i].bit; }
			if( CheckInputControl(bindings[i].key, 1) ){ cmd.pressed |= bindings[i].bit; }
		}
		LastPlayerCommand = cmd;

		//[XOPS-ONLINE] clients aim locally and send the command; the host moves and shoots
		if( (g_netSession.GetRole() == xnet::Session::ROLE_CLIENT)&&g_netSession.IsActive() ){
			myHuman->SetRxRy(cmd.rx, cmd.ry);
			g_netSession.ClientSendCommand(cmd);
		}
		//弾の発射に成功すれば
		else if( ObjMgr.ApplyHumanCommand(PlayerID, &cmd) == 1 ){
			//スコアに加算
			MainGameInfo.fire += 1;

			//プレイヤーの向きを取得（反動）
			ObjMgr.GetPlayerHumanObject()->GetRxRy(&mouse_rx, &mouse_ry);
		}

	}

	//カメラ表示モード変更操作かチェック
	if( inputCtrl->CheckKeyDown( GetFunctionKeycode(1) ) ){
		if( Camera_F1mode == false ){
			Camera_F1mode = true;
			view_rx = 0.0f;
			view_ry = VIEW_F1MODE_ANGLE;
		}
		else{
			Camera_F1mode = false;
			view_rx = 0.0f;
			view_ry = 0.0f;
		}
	}

	//カメラ操作
	if( Camera_F1mode == true ){
		if( inputCtrl->CheckKeyNow( OriginalkeycodeToDinputdef(0x0C) ) ){	//NUM8
			view_ry -= INPUT_F1NUMKEYS_ANGLE;
		}
		if( inputCtrl->CheckKeyNow( OriginalkeycodeToDinputdef(0x09) ) ){	//NUM5
			view_ry += INPUT_F1NUMKEYS_ANGLE;
		}
		if( inputCtrl->CheckKeyNow( OriginalkeycodeToDinputdef(0x08) ) ){	//NUM4
			view_rx -= INPUT_F1NUMKEYS_ANGLE;
		}
		if( inputCtrl->CheckKeyNow( OriginalkeycodeToDinputdef(0x0A) ) ){	//NUM6
			view_rx += INPUT_F1NUMKEYS_ANGLE;
		}
	}

	//　ここまで通常操作系
	//
	//　ここから裏技系

	//裏技・上昇の操作かチェック
	if( (inputCtrl->CheckKeyNow( GetFunctionKeycode(5) ))&&(inputCtrl->CheckKeyNow(OriginalkeycodeToDinputdef(0x0F))) ){	// F5 + [ENTER]
		Cmd_F5 = true;
	}
	else{
		Cmd_F5 = false;
	}

	if( myHuman->GetHP() > 0 ){

		//裏技・弾追加の操作かチェック
		if( inputCtrl->CheckKeyNow( GetFunctionKeycode(6) ) ){
			if( inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x0F)) ){		// [ENTER]
				ObjMgr.CheatAddBullet(PlayerID);
			}
		}

		//裏技・武器変更の操作かチェック
		if( inputCtrl->CheckKeyNow( GetFunctionKeycode(7) ) ){
			if( inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x02)) ){		// [←]
				int id_param = myHuman->GetMainWeaponTypeNO();

				//次の武器番号を計算
				if( id_param >= TOTAL_PARAMETERINFO_WEAPON-1 ){ id_param = 0; }
				else{ id_param += 1; }

				ObjMgr.CheatNewWeapon(PlayerID, id_param);
			}
			if( inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x03)) ){		// [→]
				int id_param = myHuman->GetMainWeaponTypeNO();

				//次の武器番号を計算
				if( id_param <= 0 ){ id_param = TOTAL_PARAMETERINFO_WEAPON-1; }
				else{ id_param -= 1; }

				ObjMgr.CheatNewWeapon(PlayerID, id_param);
			}
		}

	}

	//裏技・人変更の操作かチェック
	if( inputCtrl->CheckKeyNow( GetFunctionKeycode(8) ) ){
		int Player_HumanID;

		if( inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x02)) ){		// [←]
			//現在のプレイヤー番号を取得
			Player_HumanID = ObjMgr.GetPlayerID();

			//次の人を計算
			Player_HumanID += 1;
			if( Player_HumanID >= MAX_HUMAN ){ Player_HumanID = 0; }

			//対象プレイヤー番号を適用
			ObjMgr.SetPlayerID(Player_HumanID);

			//プレイヤーの向きを取得
			ObjMgr.GetPlayerHumanObject()->GetRxRy(&mouse_rx, &mouse_ry);

			//F1モード時にカメラの向きを再設定
			if( Camera_F1mode == true ){
				camera_rx = DegreeToRadian(90);
			}
		}
		if( inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x03)) ){		// [→]
			//現在のプレイヤー番号を取得
			Player_HumanID = ObjMgr.GetPlayerID();

			//次の人を計算
			Player_HumanID -= 1;
			if( Player_HumanID < 0 ){ Player_HumanID = MAX_HUMAN-1; }

			//対象プレイヤー番号を適用
			ObjMgr.SetPlayerID(Player_HumanID);

			//プレイヤーの向きを取得
			ObjMgr.GetPlayerHumanObject()->GetRxRy(&mouse_rx, &mouse_ry);

			//F1モード時にカメラの向きを再設定
			if( Camera_F1mode == true ){
				camera_rx = DegreeToRadian(90);
			}
		}
	}

	//裏技・人追加の操作かチェック
	if( inputCtrl->CheckKeyNow( GetFunctionKeycode(9) ) ){
		if( (inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x00)))||(inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x01))) ){		// [↑]・[↓]
			float x, y, z, r;
			int param, dataid, team; 
			int selectweapon;
			weapon *weapon[TOTAL_HAVEWEAPON];
			int weapon_paramid[TOTAL_HAVEWEAPON];
			for(int i=0; i<TOTAL_HAVEWEAPON; i++){
				weapon[i] = NULL;
				weapon_paramid[i] = 0;
			}
			int id;

			//プレイヤーの座標や武器を取得
			myHuman->GetPosData(&x, &y, &z, &r);
			myHuman->GetParamData(&param, &dataid, NULL, &team);
			myHuman->GetWeapon(&selectweapon, weapon, NULL, NULL);
			for(int i=0; i<TOTAL_HAVEWEAPON; i++){
				if( weapon[i] != NULL ){
					weapon[i]->GetParamData(&weapon_paramid[i], NULL, NULL);
				}
			}

			//プレイヤーの目の前の座標を取得
			x += cosf(r*-1 + (float)M_PI/2)*10.0f;
			y += 5.0f;
			z += sinf(r*-1 + (float)M_PI/2)*10.0f;

			//人を追加
			id = ObjMgr.AddHumanIndex(x, y, z, r, param, team, weapon_paramid, 0, false);
			if( id >= 0 ){
				ObjMgr.ChangeHaveWeapon(id, selectweapon);

				//AIを設定
				HumanAI[id].Init();
				if( inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x00)) ){		// [↑]
					HumanAI[id].SetHoldTracking(PlayerID);
				}
				if( inputCtrl->CheckKeyDown(OriginalkeycodeToDinputdef(0x01)) ){		// [↓]
					HumanAI[id].SetHoldWait(x, z, r);
				}
			}
		}
	}

	//裏技・腕描画の操作かチェック
	if( inputCtrl->CheckKeyDown( GetHomeKeycode() ) ){
		if( Camera_HOMEmode == false ){
			Camera_HOMEmode = true;
		}
		else{
			Camera_HOMEmode = false;
		}
	}
}

//! @brief フリーカメラ操作系の入力処理
//! @param mouse_x マウスのX座標
//! @param mouse_y マウスのY座標 
//! @param MouseSensitivity 視点の移動量計算
void maingame::InputViewCamera(int mouse_x, int mouse_y, float MouseSensitivity)
{
	//マウス移動をカメラの向きとして適用
	camera_rx -= mouse_x * MouseSensitivity;
	camera_ry -= mouse_y * MouseSensitivity;

	//キー操作による向きを計算
	if( CheckInputControl(KEY_TURNUP, 0) ){ camera_ry += INPUT_ARROWKEYS_ANGLE; }			// 標準：[↑]
	if( CheckInputControl(KEY_TURNDOWN, 0) ){ camera_ry -= INPUT_ARROWKEYS_ANGLE; }			// 標準：[↓]
	if( CheckInputControl(KEY_TURNLEFT, 0) ){ camera_rx += INPUT_ARROWKEYS_ANGLE; }			// 標準：[←]
	if( CheckInputControl(KEY_TURNRIGHT, 0) ){ camera_rx -= INPUT_ARROWKEYS_ANGLE; }		// 標準：[→]

	if( camera_ry > DegreeToRadian(70) ) camera_ry = DegreeToRadian(70);
	if( camera_ry < DegreeToRadian(-70) ) camera_ry = DegreeToRadian(-70);

	//移動量決定
	float dist = VIEW_FREECAMERA_SCALE;
	if( CheckInputControl(KEY_SHOT, 0) ){
		dist *= 2;
	}

	//キー操作によりカメラ座標を計算
	if( CheckInputControl(KEY_MOVEFORWARD, 0) ){
		camera_x += cosf(camera_rx)*cosf(camera_ry)*dist;
		camera_y += sinf(camera_ry)*dist;
		camera_z += sinf(camera_rx)*cosf(camera_ry)*dist;
	}
	if( CheckInputControl(KEY_MOVEBACKWARD, 0) ){
		camera_x -= cosf(camera_rx)*cosf(camera_ry)*dist;
		camera_y -= sinf(camera_ry)*dist;
		camera_z -= sinf(camera_rx)*cosf(camera_ry)*dist;
	}
	if( CheckInputControl(KEY_MOVELEFT, 0) ){
		camera_x += cosf(camera_rx + (float)M_PI/2)*dist;
		camera_z += sinf(camera_rx + (float)M_PI/2)*dist;
	}
	if( CheckInputControl(KEY_MOVERIGHT, 0) ){
		camera_x += cosf(camera_rx - (float)M_PI/2)*dist;
		camera_z += sinf(camera_rx - (float)M_PI/2)*dist;
	}
}

void maingame::Process()
{
	//プレイヤーのクラスを取得
	human *myHuman = ObjMgr.GetPlayerHumanObject();

	float ontarget;
	int kill, headshot;

	//-----------------------------------

	time = GetTimeMS();

	//オブジェクトマネージャーを実行
	int cmdF5id;
	if( Cmd_F5 == true ){
		cmdF5id = ObjMgr.GetPlayerID();
	}
	else{
		cmdF5id = -1;
	}
	//[XOPS-ONLINE] host applies peer commands before the tick and replicates after it;
	//clients apply the newest snapshot and events before their (damage-free) tick
	bool netHost = g_netSession.IsActive() && (g_netSession.GetRole() == xnet::Session::ROLE_HOST);
	bool netClient = g_netSession.IsActive() && (g_netSession.GetRole() == xnet::Session::ROLE_CLIENT);
	if( netHost ){ g_netSession.HostPreTick(); }
	if( netClient ){ g_netSession.ClientApply(); }
	ObjMgr.Process(cmdF5id, false, DarkScreenFlag);
	if( netHost ){ g_netSession.HostPostTick(); }

	//プレイヤーの戦歴を加算
	ObjMgr.GetHumanShotInfo( ObjMgr.GetPlayerID(), &ontarget, &kill, &headshot);
	MainGameInfo.ontarget += ontarget;
	MainGameInfo.kill += kill;
	MainGameInfo.headshot += headshot;

	time_process_object = GetTimeMS() - time;

	//-----------------------------------

	time = GetTimeMS();
	//[OpenXOPS-web] humans deployed by a wave event start with fresh AI
	for(int i=0; i<MAX_HUMAN; i++){
		if( ObjMgr.TakeAIInitFlag(i) == true ){ HumanAI[i].Init(); }
	}

	if( (AIstop == false)&&(netClient == false) ){	//[XOPS-ONLINE] clients run no AI
		int PlayerID = ObjMgr.GetPlayerID();
		for(int i=0; i<MAX_HUMAN; i++){
			if( (PlayerAI == false)&&(i == PlayerID) ){ continue; }
			if( ObjMgr.GetNetControlled(i) == true ){ continue; }	//[XOPS-ONLINE] driven by a network peer
			//AIを実行
			HumanAI[i].Process();
		}
	}
	time_process_ai = GetTimeMS() - time;

	//-----------------------------------

	//ミッションが終了していなければ、
	if( (end_framecnt == 0)&&(g_netSession.IsActive() == false) ){	//[XOPS-ONLINE] deathmatch has no win/lose
		int check = ObjMgr.CheckGameOverorComplete();

		//ゲームクリアー判定
		if( check == 1 ){
			end_framecnt += 1;
			MainGameInfo.missioncomplete = true;
		}

		//ゲームオーバー判定
		if( check == 2 ){
			end_framecnt += 1;
			MainGameInfo.missioncomplete = false;
		}
	}

	//-----------------------------------

	time = GetTimeMS();
	if( (EventStop == false)&&(g_netSession.IsActive() == false) ){	//[XOPS-ONLINE] no mission events in deathmatch
		//イベント実行
		for(int i=0; i<TOTAL_EVENTLINE; i++){
			if( end_framecnt == 0 ){
				EventParameter EventParam;

				//イベントの実行状況を構造体に格納
				EventParam.endcnt				 = end_framecnt;
				EventParam.complete				 = MainGameInfo.missioncomplete;
				EventParam.MessageID			 = message_id;
				EventParam.SetMessageID			 = false;

				//イベントポイントを実行
				Event[i].ProcessEventPoint(EventSkip[i], &EventParam);

				//構造体から各変数に戻す
				end_framecnt						 = EventParam.endcnt;
				MainGameInfo.missioncomplete		 = EventParam.complete;
				message_id							 = EventParam.MessageID;

				//イベントスキップフラグを戻す
				EventSkip[i] = false;

				//イベントメッセージが再セットされていたら、カウントを戻す。
				if( EventParam.SetMessageID == true ){
					message_cnt = 0;
				}
			}
		}
	}

	if( (message_id != -1)&&(message_cnt < (int)(TOTAL_EVENTENT_SHOWMESSEC*GAMEFPS)) ){
		message_cnt += 1;
	}
	else{
		message_id = -1;
		message_cnt = 0;
	}
	time_process_event = GetTimeMS() - time;

	//-----------------------------------

	float x, y, z;
	myHuman->GetPosData(&x, &y, &z, NULL);

	if( PlayerAI == true ){
		//プレイヤーの向きを取得
		ObjMgr.GetPlayerHumanObject()->GetRxRy(&mouse_rx, &mouse_ry);
	}

	//カメラワークを求める
	if( Camera_Debugmode == true ){
		//
	}
	else if( myHuman->GetDeadFlag() == true ){
		float camera_ry2 = camera_ry;

		//camera_ry2を -PI～PI の間に正規化
		camera_ry2 = AngleNormalization(camera_ry2);

		float ry = camera_ry2*0.95f + DegreeToRadian(-89.0f)*0.05f;						// 19/20 + 1/20
		float r = 3.12f;

		camera_rx += DegreeToRadian(1.0f);
		camera_ry = ry;
		camera_x = x + cosf(camera_rx)*r;
		camera_y = y + 33.3f;
		camera_z = z + sinf(camera_rx)*r;
	}
	else if( myHuman->GetDeadMotionFlag() == true ){	//myHuman->GetDeadFlag() == false
		//死亡モーション中はカメラ座標を変更しない
	}
	else if( Camera_F1mode == true ){
		float crx, cry;
		float ccx, ccy, ccz;

		//カメラの注視点を計算
		crx = camera_rx*0.8f + (view_rx + mouse_rx*-1 + (float)M_PI/2)*0.2f;	// 8 : 2
		cry = camera_ry*0.8f + (view_ry + mouse_ry)*0.2f - (float)M_PI/2;		// 8 : 2
		ccx = x - cosf(crx)*cosf(cry)*3.0f;
		ccy = y + HUMAN_HEIGHT-0.5f + sinf(cry*-1)*2.5f;
		ccz = z - sinf(crx)*cosf(cry)*3.0f;

		//注視点からカメラまでの当たり判定
		cry += (float)M_PI/2;
		float dist;
		if( CollD.CheckALLBlockIntersectRay(ccx, ccy, ccz, cosf(crx)*cosf(cry)*-1, sinf(cry*-1), sinf(crx)*cosf(cry)*-1, NULL, NULL, &dist, VIEW_F1MODE_DIST) == true ){
			dist -= 1.0f;
		}
		else{
			dist = VIEW_F1MODE_DIST;
		}

		//カメラ座標を再計算
		camera_x = ccx - cosf(crx)*cosf(cry)*(dist);
		camera_y = ccy + sinf(cry*-1)*dist;
		camera_z = ccz - sinf(crx)*cosf(cry)*(dist);
		camera_rx = crx;
		camera_ry = cry;
	}
	else{
		float crx = view_rx + mouse_rx*-1 + (float)M_PI/2;
		float cry = view_ry + mouse_ry;

		camera_x = x + cosf(crx)*cosf(cry)*VIEW_DIST;
		camera_y = y + VIEW_HEIGHT + sinf(cry)*VIEW_DIST;
		camera_z = z + sinf(crx)*cosf(cry)*VIEW_DIST;
		camera_rx = crx;
		camera_ry = cry;
	}

	//ダメージを受けていれば、レッドフラッシュを描画する
	redflash_flag = myHuman->CheckHit(NULL);

	//-----------------------------------

#ifdef ENABLE_DEBUGCONSOLE
	//デバック用コンソールを表示しており、かつミッション終了のカウントが始まっていなければ～
	if( (Show_Console == true)&&(end_framecnt == 0) ){
		//デバック用コンソールのメイン処理
		ProcessConsole();
	}
#endif

	//-----------------------------------

	framecnt += 1;
	if( start_framecnt < (int)(2.0f*GAMEFPS) ){	//ミッション開始中なら
		start_framecnt += 1;
	}
	if( end_framecnt == 1 ){					//ミッション終了直後ならば
		MainGameInfo.framecnt = framecnt;
		memcpy(&GameInfoData, &MainGameInfo, sizeof(GameInfo));		//全てコピー
		end_framecnt += 1;
	}
	else if( end_framecnt > 0 ){				//ミッション終了中ならば
		if( end_framecnt < (int)(4.0f*GAMEFPS) ){
			end_framecnt += 1;
		}
		else{
			GameInfoData.fire = MainGameInfo.fire;			//射撃回数
			GameInfoData.ontarget = MainGameInfo.ontarget;	//命中数
			GameInfoData.kill = MainGameInfo.kill;			//倒した敵の数
			GameInfoData.headshot = MainGameInfo.headshot;	//敵の頭部に命中した数

			GameState->PushMouseButton();
			GameSpeed = 1;
		}
	}
}

void maingame::Sound()
{
	time = GetTimeMS();

	//プレイヤーのチーム番号取得
	int teamid;
	human *myHuman = ObjMgr.GetPlayerHumanObject();
	myHuman->GetParamData(NULL, NULL, NULL, &teamid);

	//サウンドを再生
	GameSound->PlayWorldSound(camera_x, camera_y, camera_z, camera_rx, teamid);

	time_sound = GetTimeMS() - time;
}

void maingame::Render3D()
{
	time = GetTimeMS();


	int skymodel, skytexture;
	human *myHuman = ObjMgr.GetPlayerHumanObject();

	//フォグとカメラを設定
	d3dg->SetFog(FogFlag, SkyNumber);
	if( (Camera_F1mode == false)&&(Camera_Debugmode == false) ){
		ScopeParameter sparam;
		int scopemode = myHuman->GetScopeMode();
		GameParamInfo.GetScopeParam(scopemode, &sparam);

		d3dg->SetCamera(camera_x, camera_y, camera_z, camera_rx, camera_ry, sparam.ViewAngle);
	}
	else{
		ScopeParameter sparam;
		GameParamInfo.GetScopeParam(0, &sparam);

		d3dg->SetCamera(camera_x, camera_y, camera_z, camera_rx, camera_ry, sparam.ViewAngle);
	}

	//カメラ座標に背景空を描画
	d3dg->SetWorldTransform(camera_x, camera_y, camera_z, 0.0f, 0.0f, 2.0f);
	Resource.GetSkyModelTexture(&skymodel, &skytexture);
	d3dg->RenderModel(skymodel, skytexture, DarkScreenFlag, false, false);

	//Zバッファを初期化
	d3dg->ResetZbuffer();

	if( CenterLine == true ){
		//中心線表示（デバック用）
		d3dg->RenderCenterline();
	}

	//マップを描画
	d3dg->ResetWorldTransform();
	d3dg->RenderMapdata(wireframe);

	//プレイヤーの描画有無の決定
	int DrawPlayer = 0;
	if( (Camera_F1mode == false)&&(Camera_Debugmode == false)&&(myHuman->GetHP() > 0) ){
		if( Camera_HOMEmode == false ){
			DrawPlayer = 1;
		}
		else{
			DrawPlayer = 2;
		}
	}
	//オブジェクトを描画
	ObjMgr.Render(camera_x, camera_y, camera_z, camera_rx, camera_ry, DrawPlayer, nomodel);

	//AIデバック情報表示
	if( AIdebuginfoID != -1 ){
		if( (0 <= AIdebuginfoID)&&(AIdebuginfoID < MAX_HUMAN) ){
			float posx, posy, posz, rx;
			int EnemyID;
			float mposx, mposz;
			int movemode;
			ObjMgr.GetHumanObject(AIdebuginfoID)->GetPosData(&posx, &posy, &posz, &rx);
			EnemyID = HumanAI[AIdebuginfoID].GetEnemyHumanID();
			HumanAI[AIdebuginfoID].GetMoveTargetPos(&mposx, &mposz, &movemode);

			d3dg->ResetWorldTransform();

			//本人
			d3dg->Renderline(posx+10.0f, posy, posz, posx-10.0f, posy, posz, d3dg->GetColorCode(0.0f,0.0f,1.0f,1.0f));
			d3dg->Renderline(posx, posy+10.0f, posz, posx, posy-10.0f, posz, d3dg->GetColorCode(0.0f,0.0f,1.0f,1.0f));
			d3dg->Renderline(posx, posy, posz+10.0f, posx, posy, posz-10.0f, d3dg->GetColorCode(0.0f,0.0f,1.0f,1.0f));

			//移動先
			d3dg->Renderline(mposx+10.0f, posy, mposz, mposx-10.0f, posy, mposz, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
			d3dg->Renderline(mposx, 5000.0f, mposz, mposx, -500.0f,mposz, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
			d3dg->Renderline(mposx, posy, mposz+10.0f, mposx, posy, mposz-10.0f, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));

			if( EnemyID != -1 ){
				ObjMgr.GetHumanObject(EnemyID)->GetPosData(&posx, &posy, &posz, &rx);

				//攻撃対象
				d3dg->Renderline(posx+3.0f, posy, posz+3.0f, posx+3.0f, posy, posz-3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx+3.0f, posy, posz-3.0f, posx-3.0f, posy, posz-3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx-3.0f, posy, posz-3.0f, posx-3.0f, posy, posz+3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx-3.0f, posy, posz+3.0f, posx+3.0f, posy, posz+3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx+3.0f, posy+HUMAN_HEIGHT, posz+3.0f, posx+3.0f, posy+HUMAN_HEIGHT, posz-3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx+3.0f, posy+HUMAN_HEIGHT, posz-3.0f, posx-3.0f, posy+HUMAN_HEIGHT, posz-3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx-3.0f, posy+HUMAN_HEIGHT, posz-3.0f, posx-3.0f, posy+HUMAN_HEIGHT, posz+3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx-3.0f, posy+HUMAN_HEIGHT, posz+3.0f, posx+3.0f, posy+HUMAN_HEIGHT, posz+3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx+3.0f, posy, posz+3.0f, posx+3.0f, posy+HUMAN_HEIGHT, posz+3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx+3.0f, posy, posz-3.0f, posx+3.0f, posy+HUMAN_HEIGHT, posz-3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx-3.0f, posy, posz-3.0f, posx-3.0f, posy+HUMAN_HEIGHT, posz-3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
				d3dg->Renderline(posx-3.0f, posy, posz+3.0f, posx-3.0f, posy+HUMAN_HEIGHT, posz+3.0f, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
			}
		}
	}
}

void maingame::Render2D()
{
	char str[256];
	unsigned char stru[256];
	float fps = GetFps(10);
	float effect;

	human *myHuman = ObjMgr.GetPlayerHumanObject();

	int selectweapon;
	weapon *weapon[TOTAL_HAVEWEAPON];
	int weapon_paramid[TOTAL_HAVEWEAPON];
	int lnbslist[TOTAL_HAVEWEAPON];
	int nbslist[TOTAL_HAVEWEAPON];
	WeaponParameter weapon_paramdata;
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		weapon[i] = NULL;
		weapon_paramid[i] = 0;
		lnbslist[i] = 0;
		nbslist[i] = 0;
	}
	int lnbs = 0;
	int nbs = 0;
	int reloadcnt = 0;
	int selectweaponcnt = 0;
	int weaponmodel, weapontexture;
	char weaponname[24+1];
	int hp;
	int param_scopemode;
	int param_WeaponP;
	int param_pellet;
	int ErrorRange;

	//各種設定やゲーム情報を取得
	myHuman->GetWeapon(&selectweapon, weapon, lnbslist, nbslist);
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		if( selectweapon == i ){
			//メイン武器
			//武器を持っているならその弾数、未所持ならば未所持時の弾数を取得
			if( weapon[i] != NULL ){
				weapon[i]->GetParamData(&weapon_paramid[i], &lnbs, &nbs);
			}
			else{
				lnbs = lnbslist[i];
				nbs = nbslist[i];
			}
		}
		else{
			//サブ武器　サブ武器は武器の種類のみ
			if( weapon[i] != NULL ){
				weapon[i]->GetParamData(&weapon_paramid[i], NULL, NULL);
			}
		}
	}
	reloadcnt = myHuman->GetWeaponReloadCnt();
	selectweaponcnt = myHuman->GetChangeHaveWeaponCnt() + myHuman->GetChangeWeaponIDCnt();
	GameParamInfo.GetWeapon(weapon_paramid[selectweapon], &weapon_paramdata);
	//strcpy(weaponname, weapon_paramdata.name);
	strcpy(str, weapon_paramdata.name);
	str[sizeof(weaponname)-1] = '\0';
	strcpy(weaponname, str);
	hp = myHuman->GetHP();
	param_scopemode = weapon_paramdata.scopemode;
	param_WeaponP = weapon_paramdata.WeaponP;
	param_pellet = weapon_paramdata.pellet;
	ErrorRange = myHuman->GetGunsightErrorRange();

	float human_x, human_y, human_z, human_rx;
	myHuman->GetPosData(&human_x, &human_y, &human_z, &human_rx);

	//HPによる色の決定
	int statecolor;
	if( hp >= 100 ){
		statecolor = d3dg->GetColorCode(0.0f,1.0f,0.0f,1.0f);
	}
	else if( hp >= 50 ){
		statecolor = d3dg->GetColorCode(1.0f/50*(100-hp),1.0f,0.0f,1.0f);
	}
	else if( hp > 0 ){
		statecolor = d3dg->GetColorCode(1.0f,1.0f/50*hp,0.0f,1.0f);
	}
	else{
		statecolor = d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f);
	}

	//レッドフラッシュ描画
	if( (redflash_flag == true)&&(Camera_Debugmode == false) ){
		d3dg->Draw2DBox(0, 0, GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), d3dg->GetColorCode(1.0f,0.0f,0.0f,0.5f));
		redflash_flag = false;
	}

	//スコープ描画
	if( (Camera_F1mode == false)&&(Camera_Debugmode == false)&&(myHuman->GetScopeMode() != 0) ){
		ScopeParameter sparam;
		GameParamInfo.GetScopeParam(myHuman->GetScopeMode(), &sparam);

		if( ((float)GameConfig.GetScreenWidth() / GameConfig.GetScreenHeight()) > 1.5f ){
			int swidth = (int)((float)GameConfig.GetScreenHeight() * 1.333f)+1;
			d3dg->Draw2DTexture((GameConfig.GetScreenWidth()-swidth)/2, 0, Resource.GetScopeTexture(), swidth, GameConfig.GetScreenHeight(), 1.0f);
			d3dg->Draw2DBox(0, 0, (GameConfig.GetScreenWidth()-swidth)/2, GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DBox(GameConfig.GetScreenWidth() - (GameConfig.GetScreenWidth()-swidth)/2, 0, GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
		}
		else{
			d3dg->Draw2DTexture(0, 0, Resource.GetScopeTexture(), GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), 1.0f);
		}

		if( sparam.ScopeGunsight == 1 ){
			d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2-49, GameConfig.GetScreenHeight()/2, GameConfig.GetScreenWidth()/2-4, GameConfig.GetScreenHeight()/2, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2+4, GameConfig.GetScreenHeight()/2, GameConfig.GetScreenWidth()/2+49, GameConfig.GetScreenHeight()/2, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2-49, GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2-4, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2+4, GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2+49, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DBox(GameConfig.GetScreenWidth()/2-50, GameConfig.GetScreenHeight()/2-1, GameConfig.GetScreenWidth()/2+50, GameConfig.GetScreenHeight()/2+1, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));
			d3dg->Draw2DBox(GameConfig.GetScreenWidth()/2-1, GameConfig.GetScreenHeight()/2-50, GameConfig.GetScreenWidth()/2+1, GameConfig.GetScreenHeight()/2+50, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));
		}
		if( sparam.ScopeGunsight == 2 ){
			int w;
			if( ((float)GameConfig.GetScreenWidth() / GameConfig.GetScreenHeight()) > 1.5f ){
				int swidth = (int)((float)GameConfig.GetScreenHeight() * 1.333f)+1;
				w = 140 + (GameConfig.GetScreenWidth()-swidth)/2;
			}
			else{
				w = 140;
			}
			d3dg->Draw2DLine(w, GameConfig.GetScreenHeight()/2, GameConfig.GetScreenWidth() -w, GameConfig.GetScreenHeight()/2, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2, 60, GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight() -60, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DBox(w, GameConfig.GetScreenHeight()/2-1, GameConfig.GetScreenWidth() -w, GameConfig.GetScreenHeight()/2+1, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));
			d3dg->Draw2DBox(GameConfig.GetScreenWidth()/2-1, 60, GameConfig.GetScreenWidth()/2+1, GameConfig.GetScreenHeight() -60, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.5f));
		}
	}

	//目隠し描画
	if( (Camera_Blind == true)&&(Camera_Debugmode == false)&&(hp > 0) ){

		int scopemode = myHuman->GetScopeMode();
		float adddist = 1.2f;
		ScopeParameter sparam;

		//スコープによる視野角を決定
		GameParamInfo.GetScopeParam(scopemode, &sparam);
		float addang = sparam.ViewAngle / 4;

		//上
		if( CollD.CheckALLBlockInside(camera_x + cosf(camera_rx)*cosf(camera_ry + addang) * adddist, camera_y + sinf(camera_ry + addang) * adddist, camera_z + sinf(camera_rx)*cosf(camera_ry + addang) * adddist) == true ){
			d3dg->Draw2DBox(0, 0, GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight()/2, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
		}

		//下
		if( CollD.CheckALLBlockInside(camera_x + cosf(camera_rx)*cosf(camera_ry - addang) * adddist, camera_y + sinf(camera_ry - addang) * adddist, camera_z + sinf(camera_rx)*cosf(camera_ry - addang) * adddist) == true ){
			d3dg->Draw2DBox(0, GameConfig.GetScreenHeight()/2, GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
		}

		//左
		if( CollD.CheckALLBlockInside(camera_x + cosf(camera_rx + addang)*cosf(camera_ry) * adddist, camera_y + sinf(camera_ry) * adddist, camera_z + sinf(camera_rx + addang)*cosf(camera_ry) * adddist) == true ){
			d3dg->Draw2DBox(0, 0, GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
		}

		//右
		if( CollD.CheckALLBlockInside(camera_x + cosf(camera_rx - addang)*cosf(camera_ry) * adddist, camera_y + sinf(camera_ry) * adddist, camera_z + sinf(camera_rx - addang)*cosf(camera_ry) * adddist) == true ){
			d3dg->Draw2DBox(GameConfig.GetScreenWidth()/2, 0, GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
		}
	}

#ifdef ENABLE_DEBUGCONSOLE
	//デバック用・ゲーム情報の表示
	if( ShowInfo_Debugmode == true ){
		float move_x, move_y, move_z;
		int keyflag;
		int block_id;
		myHuman->GetMovePos(&move_x, &move_y, &move_z);
		keyflag = myHuman->GetMoveFlag(false);
		myHuman->GetUnderBlock(&block_id, NULL);

		//テクスチャフォントによる表示（半角英数字と記号のみ）
		sprintf(str, "frame:%d   time %02d:%02d", framecnt, framecnt/(int)GAMEFPS/60, framecnt/(int)GAMEFPS%60);
		d3dg->Draw2DTextureDebugFontText(10+1, 10+1, str, d3dg->GetColorCode(0.1f,0.1f,0.1f,1.0f));
		d3dg->Draw2DTextureDebugFontText(10, 10, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f));
		sprintf(str, "camera x:%.2f y:%.2f z:%.2f rx:%.2f ry:%.2f", camera_x, camera_y, camera_z, camera_rx, camera_ry);
		d3dg->Draw2DTextureDebugFontText(10+1, 30+1, str, d3dg->GetColorCode(0.1f,0.1f,0.1f,1.0f));
		d3dg->Draw2DTextureDebugFontText(10, 30, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f));
		sprintf(str, "human[%d] x:%.2f y:%.2f z:%.2f rx:%.2f HP:%d MoveKey:0x%02X", ObjMgr.GetPlayerID(), human_x, human_y, human_z, human_rx, hp, keyflag);
		d3dg->Draw2DTextureDebugFontText(10+1, 50+1, str, d3dg->GetColorCode(0.1f,0.1f,0.1f,1.0f));
		d3dg->Draw2DTextureDebugFontText(10, 50, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f));
		sprintf(str, "          move_x:%.2f move_y:%.2f move_z:%.2f BlockID:%d", move_x, move_y, move_z, block_id);
		d3dg->Draw2DTextureDebugFontText(10+1, 70+1, str, d3dg->GetColorCode(0.1f,0.1f,0.1f,1.0f));
		d3dg->Draw2DTextureDebugFontText(10, 70, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f));
		sprintf(str, "Input:%02dms Object:%02dms AI:%02dms Event:%02dms Sound:%02dms Render:%02dms", time_input, time_process_object, time_process_ai, time_process_event, time_sound, time_render);
		d3dg->Draw2DTextureDebugFontText(10+1, 90+1, str, d3dg->GetColorCode(0.1f,0.1f,0.1f,1.0f));
		d3dg->Draw2DTextureDebugFontText(10, 90, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f));
	}
#endif

	//ゲーム実行速度の表示
	//int speed = (int)(fps / (1000.0f/GAMEFRAMEMS) * 100);
	//sprintf(str, "PROCESSING SPEED %d%%", speed);
	sprintf(str, "fps:%.2f", fps);
	d3dg->Draw2DTextureFontText(GameConfig.GetScreenWidth() - strlen(str)*10 - 5 +1, 5+1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 10, 14);
	d3dg->Draw2DTextureFontText(GameConfig.GetScreenWidth() - strlen(str)*10 - 5, 5, str, d3dg->GetColorCode(1.0f,0.5f,0.0f,1.0f), 10, 14);

	//HUD表示・モードA
	if( Camera_F2mode == 0 ){
		//左下エリア描画
		//"ｳｴｴｴｴｴｴｵ"
		stru[0] = 0xB3;		stru[1] = 0xB4;		stru[2] = 0xB4;		stru[3] = 0xB4;		stru[4] = 0xB4;
		stru[5] = 0xB4;		stru[6] = 0xB4;		stru[7] = 0xB5;		stru[8] = '\0';
		d3dg->Draw2DTextureFontText(15, GameConfig.GetScreenHeight() - 105, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32);
		//"ﾃﾄﾄﾄﾄﾄﾄﾅ"
		for(int i=0; stru[i] != 0x00; i++){ stru[i] += 0x10; }
		d3dg->Draw2DTextureFontText(15, GameConfig.GetScreenHeight() - 105 +32, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32);
		//"ｳｴｴｶｷｷｷｸｹ"
		stru[0] = 0xB3;		stru[1] = 0xB4;		stru[2] = 0xB4;		stru[3] = 0xB6;		stru[4] = 0xB7;
		stru[5] = 0xB7;		stru[6] = 0xB7;		stru[7] = 0xB8;		stru[8] = 0xB9;		stru[9] = '\0';
		d3dg->Draw2DTextureFontText(15, GameConfig.GetScreenHeight() - 55, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32);
		//"ﾃﾄﾄﾆﾇﾇﾇﾈﾉ"
		for(int i=0; stru[i] != 0x00; i++){ stru[i] += 0x10; }
		d3dg->Draw2DTextureFontText(15, GameConfig.GetScreenHeight() - 55 +32, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32);

		//右下エリア用文字コード設定
		stru[0] = 0xB0;//'ｰ';
		for(int i=1; i<HUDA_WEAPON_SIZEW-1; i++){
			stru[i] = 0xB1;//'ｱ';
		}
		stru[HUDA_WEAPON_SIZEW-1] = 0xB2;//'ｲ';
		stru[HUDA_WEAPON_SIZEW] = '\0';

		//右下エリア描画
		d3dg->Draw2DTextureFontText(HUDA_WEAPON_POSX, HUDA_WEAPON_POSY, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32);
		for(int i=0; i<HUDA_WEAPON_SIZEW; i++){ stru[i] += 0x10; }
		for(int i=1; i<HUDA_WEAPON_SIZEH-1; i++){
			d3dg->Draw2DTextureFontText(HUDA_WEAPON_POSX, HUDA_WEAPON_POSY + 32*i, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32);
		}
		for(int i=0; i<HUDA_WEAPON_SIZEW; i++){ stru[i] += 0x10; }
		d3dg->Draw2DTextureFontText(HUDA_WEAPON_POSX, HUDA_WEAPON_POSY + 32*(HUDA_WEAPON_SIZEH-1), (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32);

		//武器の弾数表示
		sprintf((char*)stru, "A%d B%d", lnbs, (nbs - lnbs));
		for(int i=0; i<(int)strlen((char*)stru); i++){
			if( stru[i] == 'A' ){ stru[i] = 0xBB; }	//'ｻ'
			if( stru[i] == 'B' ){ stru[i] = 0xBA; }	//'ｺ'
		}
		d3dg->Draw2DTextureFontText(25, GameConfig.GetScreenHeight() - 96, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 23, 24);

		//HP表示
		if( hp >= 80 ){
			d3dg->Draw2DTextureFontText(23, GameConfig.GetScreenHeight() - 45, "STATE", statecolor, 18, 24);
			d3dg->Draw2DTextureFontText(155, GameConfig.GetScreenHeight() - 45, "FINE", statecolor, 18, 24);
		}
		else if( hp >= 40 ){
			d3dg->Draw2DTextureFontText(23, GameConfig.GetScreenHeight() - 45, "STATE", statecolor, 18, 24);
			d3dg->Draw2DTextureFontText(135, GameConfig.GetScreenHeight() - 45, "CAUTION", statecolor, 18, 24);
		}
		else if( hp > 0 ){
			d3dg->Draw2DTextureFontText(23, GameConfig.GetScreenHeight() - 45, "STATE", statecolor, 18, 24);
			d3dg->Draw2DTextureFontText(140, GameConfig.GetScreenHeight() - 45, "DANGER", statecolor, 18, 24);
		}
		else{
			d3dg->Draw2DTextureFontText(23, GameConfig.GetScreenHeight() - 45, "STATE", statecolor, 18, 24);
			d3dg->Draw2DTextureFontText(155, GameConfig.GetScreenHeight() - 45, "DEAD", statecolor, 18, 24);
		}

		//武器名表示
#ifdef ENABLE_WEAPONNAME_SCALING
		int w, h;
		d3dg->GetFontTextSize(weaponname, 16, 20, 14, &w, &h);
		d3dg->Draw2DTextureFontText(HUDA_WEAPON_POSX + 9, HUDA_WEAPON_POSY + 4 + (20 - h)/2, weaponname, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), w, h);
#else
		d3dg->Draw2DTextureFontText(HUDA_WEAPON_POSX + 9, HUDA_WEAPON_POSY + 4, weaponname, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 16, 20);
#endif
	}

	//HUD表示・モードB
	if( Camera_F2mode == 1 ){
		//画面周りの線
		d3dg->Draw2DLine(0, 0, GameConfig.GetScreenWidth()-1, 0, statecolor);
		d3dg->Draw2DLine(GameConfig.GetScreenWidth()-1, 0, GameConfig.GetScreenWidth()-1, GameConfig.GetScreenHeight()-1, statecolor);
		d3dg->Draw2DLine(0, 0, 0, GameConfig.GetScreenHeight()-1, statecolor);
		d3dg->Draw2DLine(0, GameConfig.GetScreenHeight()-1, GameConfig.GetScreenWidth()-1, GameConfig.GetScreenHeight()-1, statecolor);

		//武器名表示
#ifdef ENABLE_WEAPONNAME_SCALING
		int w, h;
		d3dg->GetFontTextSize(weaponname, 16, 20, 14, &w, &h);

		d3dg->Draw2DBox(8, GameConfig.GetScreenHeight() - 32, 227, GameConfig.GetScreenHeight() - 7, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.3f));
		d3dg->Draw2DTextureFontText(10, GameConfig.GetScreenHeight() - 30 + (20 - h)/2, weaponname, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), w, h);
#else
		d3dg->Draw2DBox(8, GameConfig.GetScreenHeight() - 32, 227, GameConfig.GetScreenHeight() - 7, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.3f));
		d3dg->Draw2DTextureFontText(10, GameConfig.GetScreenHeight() - 30, weaponname, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 16, 20);
#endif
	}

	//レーダー描画
	if( radar == true ){
		RenderRadar();
	}

	//イベントメッセージ表示
	if( (message_id != -1)&&(message_cnt < (int)(TOTAL_EVENTENT_SHOWMESSEC*GAMEFPS)) ){
		char messtr[MAX_POINTMESSAGEBYTE];
		PointData.GetMessageText(messtr, message_id);
		float effectA = 1.0f;
		if( message_cnt < (int)(0.2f*GAMEFPS) ){ effectA = GetEffectAlpha(message_cnt, 1.0f, 0.2f, 0.0f, false); }
		if( (int)((TOTAL_EVENTENT_SHOWMESSEC-0.2f)*GAMEFPS) < message_cnt ){ effectA = GetEffectAlpha(message_cnt, 1.0f, 0.2f, (TOTAL_EVENTENT_SHOWMESSEC - 0.2f), true); }
		d3dg->Draw2DMSFontTextCenterScaling(0 +1, 340 +1, GameConfig.GetScreenWidth(), 140, messtr, d3dg->GetColorCode(0.1f,0.1f,0.1f,effectA));
		d3dg->Draw2DMSFontTextCenterScaling(0, 340, GameConfig.GetScreenWidth(), 140, messtr, d3dg->GetColorCode(1.0f,1.0f,1.0f,effectA));
	}

#ifdef ENABLE_DEBUGCONSOLE
	//デバック用・ゲーム情報の表示
	if( ShowInfo_Debugmode == true ){
		ObjMgr.RenderLog(HUDA_WEAPON_POSX, HUDA_WEAPON_POSY-60);
	}
#endif

	//リロード表示
	if( reloadcnt > 0 ){
		d3dg->Draw2DTextureFontTextCenterScaling(3, 300+3, "RELOADING", d3dg->GetColorCode(0.2f,0.2f,0.2f,1.0f), 32, 34);
		d3dg->Draw2DTextureFontTextCenterScaling(0, 300, "RELOADING", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 32, 34);
	}

	//武器切り替え表示
	if( selectweaponcnt > 0 ){
		d3dg->Draw2DTextureFontTextCenterScaling(3, 300+3, "CHANGING", d3dg->GetColorCode(0.2f,0.2f,0.2f,1.0f), 32, 34);
		d3dg->Draw2DTextureFontTextCenterScaling(0, 300, "CHANGING", d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 32, 34);
	}

	//照準表示
	if( (Camera_F1mode == false)&&(param_WeaponP != 2)&&(param_pellet > 0) ){
		if( (weapon[selectweapon] != NULL) ){
			int scopemode = myHuman->GetScopeMode();
			ScopeParameter sparam;
			GameParamInfo.GetScopeParam(param_scopemode, &sparam);

			if( ( (scopemode == 0)&&(sparam.NoScopeGunsight == true) )||( (scopemode != 0)&&(sparam.ScopeGunsight == 0) ) ){
				if( GameConfig.GetAnotherGunsightFlag() ){	//オプション型
					//照準の透明度
					float alpha = 1.0f - (float)ErrorRange/40.0f;
					if( alpha < 0.0f ){ alpha = 0.0f; }

					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2, GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2+4, d3dg->GetColorCode(1.0f,0.0f,0.0f,0.5f));
					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2-15, GameConfig.GetScreenHeight()/2+15, GameConfig.GetScreenWidth()/2-19, GameConfig.GetScreenHeight()/2+19, d3dg->GetColorCode(1.0f,0.0f,0.0f,0.5f));
					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2+15, GameConfig.GetScreenHeight()/2+15, GameConfig.GetScreenWidth()/2+19, GameConfig.GetScreenHeight()/2+19, d3dg->GetColorCode(1.0f,0.0f,0.0f,0.5f));
					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2-4, GameConfig.GetScreenHeight()/2+4, GameConfig.GetScreenWidth()/2+4, GameConfig.GetScreenHeight()/2+4, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));

					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2-4 - ErrorRange, GameConfig.GetScreenHeight()/2-4 - ErrorRange/2, GameConfig.GetScreenWidth()/2-4 - ErrorRange, GameConfig.GetScreenHeight()/2+4 + ErrorRange/2, d3dg->GetColorCode(1.0f,0.0f,0.0f,alpha));
					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2+4 + ErrorRange, GameConfig.GetScreenHeight()/2-4 - ErrorRange/2, GameConfig.GetScreenWidth()/2+4 + ErrorRange, GameConfig.GetScreenHeight()/2+4 + ErrorRange/2, d3dg->GetColorCode(1.0f,0.0f,0.0f,alpha));
				}
				else{										//標準型
					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2-13, GameConfig.GetScreenHeight()/2, GameConfig.GetScreenWidth()/2-3, GameConfig.GetScreenHeight()/2, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2+13, GameConfig.GetScreenHeight()/2, GameConfig.GetScreenWidth()/2+3, GameConfig.GetScreenHeight()/2, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2-13, GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2-3, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));
					d3dg->Draw2DLine(GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2+13, GameConfig.GetScreenWidth()/2, GameConfig.GetScreenHeight()/2+3, d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f));

					stru[0] = 0xBD;		stru[1] = '\0';	//"ｽ"
					d3dg->Draw2DTextureFontTextCenter(ErrorRange * -1, GameConfig.GetScreenHeight()/2 - 16, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32);
					stru[0] = 0xBE;		stru[1] = '\0';	//"ｾ"
					d3dg->Draw2DTextureFontTextCenter(ErrorRange, GameConfig.GetScreenHeight()/2 - 16, (char*)stru, d3dg->GetColorCode(1.0f,1.0f,1.0f,0.5f), 32, 32); 
				}
			}
		}
	}

	if( tag == true ){
		int color;
		if( ObjMgr.GetObjectInfoTag(camera_x, camera_y, camera_z, camera_rx, camera_ry, &color, str) == true ){
			d3dg->Draw2DTextureFontTextCenterScaling(1, 270 +1 , str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f), 14, 18);
			d3dg->Draw2DTextureFontTextCenterScaling(0, 270, str, color, 14, 18);
		}
	}

#ifdef ENABLE_DEBUGCONSOLE
	//AIデバック情報表示
	if( AIdebuginfoID != -1 ){
		if( (0 <= AIdebuginfoID)&&(AIdebuginfoID < MAX_HUMAN) ){
			float posx, posy, posz, rx;
			int hp;
			char modestr[32];
			int EnemyID;
			int AIlevel;
			float mposx, mposz;
			int movemode;
			pointdata ppdata;
			ObjMgr.GetHumanObject(AIdebuginfoID)->GetPosData(&posx, &posy, &posz, &rx);
			hp = ObjMgr.GetHumanObject(AIdebuginfoID)->GetHP();
			HumanAI[AIdebuginfoID].GetBattleMode(NULL, modestr);
			EnemyID = HumanAI[AIdebuginfoID].GetEnemyHumanID();
			AIlevel = HumanAI[AIdebuginfoID].GetAIlevel();
			HumanAI[AIdebuginfoID].GetMoveTargetPos(&mposx, &mposz, &movemode);
			HumanAI[AIdebuginfoID].GetPathPointData(&ppdata);

			sprintf(str, "AI debug info [ID:%d]", AIdebuginfoID);
			d3dg->Draw2DTextureDebugFontText(20 +1, 130 +1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DTextureDebugFontText(20, 130, str, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
			sprintf(str, "(X:%.2f Y:%.2f Z:%.2f RX:%.2f HP:%d)", posx, posy, posz, rx, hp);
			d3dg->Draw2DTextureDebugFontText(20 +1, 150 +1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DTextureDebugFontText(20, 150, str, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
			sprintf(str, "Mode:%s  TargetEnemyID:%d  AIlevel:%d", modestr, EnemyID, AIlevel);
			d3dg->Draw2DTextureDebugFontText(20 +1, 170 +1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DTextureDebugFontText(20, 170, str, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
			sprintf(str, "PointPath:[%d][%d][%d][%d]", ppdata.p1, ppdata.p2, ppdata.p3, ppdata.p4);
			d3dg->Draw2DTextureDebugFontText(20 +1, 190 +1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DTextureDebugFontText(20, 190, str, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
			sprintf(str, "MovePosX:%.2f  MovePosZ:%.2f  (MoveMode:%d)", mposx, mposz, movemode);
			d3dg->Draw2DTextureDebugFontText(20 +1, 210 +1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DTextureDebugFontText(20, 210, str, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
		}
	}
#endif


	//-----------------------------------

	//Zバッファを初期化
	d3dg->ResetZbuffer();


	//HUD表示・モードA
	if( Camera_F2mode == 0 ){
		int notselectweapon = selectweapon + 1;
		if( notselectweapon == TOTAL_HAVEWEAPON ){ notselectweapon = 0; }

		//（3D描画）カメラ座標を暫定設定
		ScopeParameter sparam;
		GameParamInfo.GetScopeParam(0, &sparam);
		d3dg->SetCamera(camera_x, camera_y, camera_z, camera_rx, camera_ry, sparam.ViewAngle);

		//（3D描画）所持している武器モデルの描画・メイン武器
		GameParamInfo.GetWeapon(weapon_paramid[selectweapon], &weapon_paramdata);
		Resource.GetWeaponModelTexture(weapon_paramid[selectweapon], &weaponmodel, &weapontexture);
		d3dg->SetWorldTransformPlayerWeapon(true, camera_rx, camera_ry, DegreeToRadian(framecnt*2), weapon_paramdata.size);
		d3dg->RenderModel(weaponmodel, weapontexture, false, true, nomodel);

		//（3D描画）所持している武器モデルの描画・サブ武器
		GameParamInfo.GetWeapon(weapon_paramid[notselectweapon], &weapon_paramdata);
		Resource.GetWeaponModelTexture(weapon_paramid[notselectweapon], &weaponmodel, &weapontexture);
		d3dg->SetWorldTransformPlayerWeapon(false, camera_rx, camera_ry, 0.0f, weapon_paramdata.size);
		d3dg->RenderModel(weaponmodel, weapontexture, false, true, nomodel);
	}

	//-----------------------------------

#ifdef ENABLE_DEBUGCONSOLE
	if( Show_Console == true ){
		if( ScreenShot > 0 ){
			ScreenShot += 1;
		}
		else{
			RenderConsole();
		}
	}
#endif

	//-----------------------------------


	//スタート時と終了時のブラックアウト設定
	if( start_framecnt < (int)(2.0f*GAMEFPS) ){
		effect = GetEffectAlpha(start_framecnt, 1.0f, 2.0f, 0.0f, true);
	}
	else if( end_framecnt > 0 ){
		effect = GetEffectAlpha(end_framecnt, 1.0f, 3.0f + 0.5f, 0.0f, false);
	}
	else{
		effect = 0.0f;
	}
	d3dg->Draw2DBox(0, 0, GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight(), d3dg->GetColorCode(0.0f,0.0f,0.0f,effect));

	//終了時の文字表示
	if( end_framecnt > 0 ){
		if( end_framecnt < (int)(1.0f*GAMEFPS) ){
			effect = GetEffectAlpha(end_framecnt, 1.0f, 1.0f, 0.0f, false);
		}
		else if( end_framecnt > (int)(3.0f*GAMEFPS) ){
			effect = GetEffectAlpha(end_framecnt, 1.0f, 1.0f, 3.0f, true);
		}
		else{
			effect = 1.0f;
		}

		if( GameInfoData.missioncomplete == true ){
			d3dg->Draw2DTextureFontTextCenterScaling(0, 240 + 10, "objective complete", d3dg->GetColorCode(1.0f,0.5f,0.0f,effect), 28, 32);
		}
		else{
			d3dg->Draw2DTextureFontTextCenterScaling(0, 240 + 10, "mission failure", d3dg->GetColorCode(1.0f,0.0f,0.0f,effect), 28, 32);
		}
	}


	time_render = GetTimeMS() - time;
}

//! @brief レーダーに描画する座標に変換
//! @param in_x 空間 X座標
//! @param in_y 空間 Y座標
//! @param in_z 空間 Z座標
//! @param RadarPosX レーダーの描画 X座標（左上基準）
//! @param RadarPosY レーダーの描画 Y座標（左上基準）
//! @param RadarSize レーダーの描画サイズ
//! @param RadarWorldR レーダーにポイントする距離
//! @param out_x 2D X座標 を受け取るポインタ
//! @param out_y 2D X座標 を受け取るポインタ
//! @param local_y ローカル Y座標 を受け取るポインタ（NULL可）
//! @param check 計算の省略
//! @return 成功：true　失敗：false
//! @attention checkフラグを有効にすると、レーダーから外れることが明らかになった時点で計算を終了し、falseを返します。
bool maingame::GetRadarPos(float in_x, float in_y, float in_z, int RadarPosX, int RadarPosY, int RadarSize, float RadarWorldR, int *out_x, int *out_y, float *local_y, bool check)
{
	bool outf = false;
	float x, y, z, r, rx;
	float x2, z2, r2, rx2;

	//カメラとの距離を計算
	x = in_x - camera_x;
	y = in_y - camera_y;
	z = in_z - camera_z;

	//近ければ処理する
	if( (check == false) || ((fabsf(x) < RadarWorldR*2)&&(fabsf(z) < RadarWorldR*2)&&(fabsf(y) < 80.0f)) ){
		//角度を距離を計算
		rx = atan2f(z, x);
		r = sqrtf(x*x + z*z);

		//カメラ基準の座標を再計算
		rx2 = (rx - camera_rx)*-1 - (float)M_PI/2;
		x2 = cosf(rx2) * r;
		z2 = sinf(rx2) * r;

		//収まるか判定
		if( (check == false) || ((fabsf(x2) < RadarWorldR)&&(fabsf(z2) < RadarWorldR)) ){
			//描画座標を計算
			r2 = r / RadarWorldR * (RadarSize/2);
			*out_x = (int)(RadarPosX+RadarSize/2 + cosf(rx2) * r2);
			*out_y = (int)(RadarPosY+RadarSize/2 + sinf(rx2) * r2);
			if( local_y != NULL ){ *local_y = y; }
			outf = true;
		}
	}

	return outf;
}

//! @brief 簡易レーダー表示
void maingame::RenderRadar()
{
	int RadarSize = 200;							//レーダーの描画サイズ
	int RadarPosX = GameConfig.GetScreenWidth() - RadarSize - 10;	//レーダーの描画 X座標（左上基準）
	int RadarPosY = 110;							//レーダーの描画 Y座標（左上基準）
	float RadarWorldR = 300.0f;						//レーダーにポイントする距離

	float ecr = DISTANCE_CHECKPOINT / RadarWorldR * (RadarSize/2);

	//下地と囲い
	d3dg->Draw2DBox(RadarPosX, RadarPosY, RadarPosX+RadarSize, RadarPosY+RadarSize, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.6f));
	d3dg->Draw2DLine(RadarPosX, RadarPosY, RadarPosX+RadarSize, RadarPosY, d3dg->GetColorCode(0.0f,0.8f,0.0f,1.0f));
	d3dg->Draw2DLine(RadarPosX+RadarSize, RadarPosY, RadarPosX+RadarSize, RadarPosY+RadarSize, d3dg->GetColorCode(0.0f,0.8f,0.0f,1.0f));
	d3dg->Draw2DLine(RadarPosX+RadarSize, RadarPosY+RadarSize, RadarPosX, RadarPosY+RadarSize, d3dg->GetColorCode(0.0f,0.8f,0.0f,1.0f));
	d3dg->Draw2DLine(RadarPosX, RadarPosY+RadarSize, RadarPosX, RadarPosY, d3dg->GetColorCode(0.0f,0.8f,0.0f,1.0f));

	//マップを描画
	int bs = BlockData.GetTotaldatas();
	for(int i=0; i< bs; i++){
		blockdata bdata;
		int vid[4];
		int bvx[4], bvy[4];

		//ブロックのデータを取得
		BlockData.Getdata(&bdata, i);

		//描画候補のブロックを検出（粗削り）
		if( CollD.CheckBlockAABB(i, camera_x-RadarWorldR*2, camera_y-1.0f, camera_z-RadarWorldR*2, camera_x+RadarWorldR*2, camera_y+1.0f, camera_z+RadarWorldR*2) == true ){

			//各面ごとに処理する
			for(int j=0; j<6; j++){
				//登れない斜面か判定　※地面や階段などの傾斜を除外する
				float angle = acosf(bdata.material[j].vy);
				if( (HUMAN_MAPCOLLISION_SLOPEANGLE < angle)&&(angle < DegreeToRadian(120)) ){

					//ブロック頂点データの関連付けを取得
					blockdataface(j, &(vid[0]), NULL);
					
					//4頂点を計算
					for(int k=0; k<4; k++){
						GetRadarPos(bdata.x[ vid[k] ], bdata.y[ vid[k] ], bdata.z[ vid[k] ], RadarPosX, RadarPosY, RadarSize, RadarWorldR, &(bvx[k]), &(bvy[k]), NULL, false);
					}

					int line_x1, line_y1, line_x2, line_y2;

					//レーダーの四角形に収まるように描画する
					if( Get2DLineInBox(bvx[0], bvy[0], bvx[1], bvy[1], RadarPosX, RadarPosY, RadarPosX+RadarSize, RadarPosY+RadarSize, &line_x1, &line_y1, &line_x2, &line_y2) == true ){
						d3dg->Draw2DLine(line_x1, line_y1, line_x2, line_y2, d3dg->GetColorCode(0.8f,0.8f,0.8f,1.0f));
					}
					if( Get2DLineInBox(bvx[1], bvy[1], bvx[2], bvy[2], RadarPosX, RadarPosY, RadarPosX+RadarSize, RadarPosY+RadarSize, &line_x1, &line_y1, &line_x2, &line_y2) == true ){
						d3dg->Draw2DLine(line_x1, line_y1, line_x2, line_y2, d3dg->GetColorCode(0.8f,0.8f,0.8f,1.0f));
					}
					if( Get2DLineInBox(bvx[2], bvy[2], bvx[3], bvy[3], RadarPosX, RadarPosY, RadarPosX+RadarSize, RadarPosY+RadarSize, &line_x1, &line_y1, &line_x2, &line_y2) == true ){
						d3dg->Draw2DLine(line_x1, line_y1, line_x2, line_y2, d3dg->GetColorCode(0.8f,0.8f,0.8f,1.0f));
					}
					if( Get2DLineInBox(bvx[3], bvy[3], bvx[0], bvy[0], RadarPosX, RadarPosY, RadarPosX+RadarSize, RadarPosY+RadarSize, &line_x1, &line_y1, &line_x2, &line_y2) == true ){
						d3dg->Draw2DLine(line_x1, line_y1, line_x2, line_y2, d3dg->GetColorCode(0.8f,0.8f,0.8f,1.0f));
					}
				}
			}
		}
	}

	//イベントの到着ポイントを表示
	if( EventStop == false ){
		for(int i=0; i<TOTAL_EVENTLINE; i++){
			signed short int p4 = Event[i].GetNextP4();
			pointdata data;
			if( PointData.SearchPointdata(&data, 0x08, 0, 0, 0, p4, 0) != 0 ){
				float y;
				int x_2d, y_2d;
				float alpha;

				if( (data.p1 == 13)||(data.p1 == 16) ){
					data.y += VIEW_HEIGHT;

					if( GetRadarPos(data.x, data.y, data.z, RadarPosX, RadarPosY, RadarSize, RadarWorldR, &x_2d, &y_2d, &y, true) == true ){
						//高さによる透明度
						if( (fabsf(y) < 40.0f) ){
							alpha = 1.0f;
						}
						else{
							alpha = 0.5f;
						}

						//マーカー描画
						d3dg->Draw2DCycle(x_2d, y_2d, (int)ecr, d3dg->GetColorCode(1.0f,0.5f,0.0f,alpha));
					}
				}
			}
		}
	}

	//プレイヤーの情報を取得
	int PlayerID = ObjMgr.GetPlayerID();
	int myteamid;
	ObjMgr.GetHumanObject(PlayerID)->GetParamData(NULL, NULL, NULL, &myteamid);

	//人を描画
	for(int i=0; i<MAX_HUMAN; i++){
		human* thuman;
		float tx, ty, tz;
		int tteamid;
		float y;
		int x_2d, y_2d;
		float alpha;
		int color;

		//人のオブジェクトを取得
		thuman = ObjMgr.GetHumanObject(i);

		//使われていない人や死体は無視する
		if( thuman->GetEnableFlag() == false ){ continue; }
		if( thuman->GetDeadFlag() == true ){ continue; }

		//人の情報を取得
		thuman->GetPosData(&tx, &ty, &tz, NULL);
		thuman->GetParamData(NULL, NULL, NULL, &tteamid);
		ty += VIEW_HEIGHT;

		if( GetRadarPos(tx, ty, tz, RadarPosX, RadarPosY, RadarSize, RadarWorldR, &x_2d, &y_2d, &y, true) == true ){
			//高さによる透明度
			if( (fabsf(y) < 40.0f) ){
				alpha = 1.0f;
			}
			else{
				alpha = 0.5f;
			}

			//マーカーの色を決定
			if( PlayerID == i ){ color = d3dg->GetColorCode(1.0f,1.0f,0.0f,alpha); }				//プレイヤー自身
			else if( tteamid == myteamid ){ color = d3dg->GetColorCode(0.0f,0.5f,1.0f,alpha); }	//味方
			else{ color = d3dg->GetColorCode(1.0f,0.0f,0.5f,alpha); }								//敵

			//マーカー描画
			d3dg->Draw2DBox(x_2d-3, y_2d-3, x_2d+3, y_2d+3, color);
		}
	}
}

#ifdef ENABLE_DEBUGCONSOLE

//! @brief デバック用コンソールに新たな文字列を追加
//! @param color 文字の色
//! @param str 追加する文字列のポインタ
//! @attention 新しい文字列は常に下から追加されます。
//! @attention 表示可能行数（定数：MAX_CONSOLELINES）を上回る場合、最初の行（1行目）を削除し1行ずつずらした上で、一番下の行に追加します。
void maingame::AddInfoConsole(int color, const char *str)
{
	for(int i=0; i<MAX_CONSOLELINES; i++){
		if( InfoConsoleData[i].textdata[0] == NULL ){
			InfoConsoleData[i].color = color;
			strcpy(InfoConsoleData[i].textdata, str);
			return;
		}
	}

	for(int i=1; i<MAX_CONSOLELINES; i++){
		memcpy(&(InfoConsoleData[i-1]), &(InfoConsoleData[i]), sizeof(ConsoleData));
	}
	InfoConsoleData[MAX_CONSOLELINES-1].color = color;
	strcpy(InfoConsoleData[MAX_CONSOLELINES-1].textdata, str);
}

//! @brief コマンドの判定および引数（整数値）を取得
//! @param cmd 判定するコマンド文字のポインタ
//! @param num 与えられた引数を受け取るポインタ
//! @return 取得：true　判定外：false
bool maingame::GetCommandNum(const char *cmd, int *num)
{
	if( cmd == NULL ){ return false; }
	if( num == NULL ){ return false; }

	//コマンド名を調べる
	if( memcmp(NewCommand, cmd, strlen(cmd)) != 0 ){ return false; }
	if( NewCommand[strlen(cmd)] != ' ' ){ return false; }

	//「コマンド名_X」分の文字数に達しているかどうか
	if( strlen(cmd)+2 > strlen(NewCommand) ){ return false; }

	//数字が与えられているか
	for(int i=(int)strlen(cmd)+1; NewCommand[i] != '\0'; i++){
		if( ((NewCommand[i] < '0')||('9' < NewCommand[i]))&&(NewCommand[i] != '+')&&(NewCommand[i] != '-') ){ return false; }
	}

	//与えられた数字を調べる
	*num = atoi(&(NewCommand[ strlen(cmd)+1 ]));
	return true;
}

//! @brief デバック用コンソールの入力処理
void maingame::InputConsole()
{
	char str[MAX_CONSOLELEN];
	NewCommand[0] = '\0';

	if( ConsoleTextBoxControl.ProcessTextBox(inputCtrl) == true ){
		//[ENTER]
		//コンソールに追加し、未処理コマンドとして登録
		strcpy(str, CONSOLE_PROMPT);
		strcat(str, ConsoleTextBoxControl.GetTextBoxStr());
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		strcpy(NewCommand, ConsoleTextBoxControl.GetTextBoxStr());

		ConsoleTextBoxControl.SetTextBoxStr("");
	}
}

//! @brief デバック用コンソールのメイン処理
void maingame::ProcessConsole()
{
	char str[MAX_CONSOLELEN];
	int id;

	//コマンドリスト
	if( strcmp(NewCommand, "help") == 0 ){
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "help          human        result          event            skip <NUM>");
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "config        mif          bd1    pd1      resinfo          info");
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "view          center       map    model    aiinfo <NUM>     tag");
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "radar         inmap        sky <NUM>       dark     fog     teleport <NUM>");
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "player <NUM>  revive       kill <NUM>      treat <NUM>      nodamage <NUM>");
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "newobj <NUM>  break <NUM>  delhuman <NUM>  delweapon <NUM>  delobj <NUM>");
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "nofight       caution      ailevel <NUM>   ff       bot     stop      estop");
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "speed         window       ss              clear            ver       exit");
	}

	//人の統計情報
	if( strcmp(NewCommand, "human") == 0 ){
		int alivemyfriend = 0;
		int myfriend = 0;
		int aliveenemy = 0;
		int enemy = 0;
		int myteamid;

		//プレイヤーのチーム番号を取得
		ObjMgr.GetPlayerHumanObject()->GetParamData(NULL, NULL, NULL, &myteamid);

		for(int i=0; i<MAX_HUMAN; i++){
			int teamid;
			bool deadflag;
			human *thuman = ObjMgr.GetHumanObject(i);
			if( thuman->GetEnableFlag() == true ){
				//死亡状態とチーム番号を取得
				deadflag = thuman->GetDeadFlag();
				thuman->GetParamData(NULL, NULL, NULL, &teamid);

				//カウント
				if( teamid == myteamid ){
					myfriend += 1;
					if( deadflag == false ){ alivemyfriend += 1; }
				}
				else{
					enemy += 1;
					if( deadflag == false ){ aliveenemy += 1; }
				}
			}
		}

		sprintf(str, "Friend:%d/%d  Enemy:%d/%d  Total:%d/%d",
			alivemyfriend, myfriend, aliveenemy, enemy, alivemyfriend + aliveenemy, myfriend + enemy);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
	}

	//暫定リザルト表示
	if( strcmp(NewCommand, "result") == 0 ){
		int intontarget;
		float rate;

		//命中率計算
		intontarget = (int)floor(MainGameInfo.ontarget);
		if( MainGameInfo.fire == 0 ){
			rate = 0.0f;
		}
		else{
			rate = (float)intontarget / MainGameInfo.fire * 100;
		}

		sprintf(str, "Time %02d:%02d  /  Fired %d  /  On target %d(%0.2f)", framecnt/(int)GAMEFPS/60, framecnt/(int)GAMEFPS%60, MainGameInfo.fire, intontarget, MainGameInfo.ontarget);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		sprintf(str, "AR rate %.1f%%  /  Kill %d  /  HS %d", rate, MainGameInfo.kill, MainGameInfo.headshot);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
	}

	//イベントタスク表示
	if( strcmp(NewCommand, "event") == 0 ){
		for(int i=0; i<TOTAL_EVENTLINE; i++){
			signed short int p4 = Event[i].GetNextP4();
			pointdata data;
			if( PointData.SearchPointdata(&data, 0x08, 0, 0, 0, p4, 0) == 0 ){
				sprintf(str, "Event %d   No task.", i);
			}
			else{
				sprintf(str, "Event %d   [%d][%d][%d][%d]", i, data.p1, data.p2, data.p3, data.p4);
			}
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}
	}

	//イベントタスクスキップ
	if( GetCommandNum("skip", &id) == true ){
		if( (0 <= id)&&(id < TOTAL_EVENTLINE) ){
			signed short int p4 = Event[id].GetNextP4();
			pointdata data;
			if( PointData.SearchPointdata(&data, 0x08, 0, 0, 0, p4, 0) == 0 ){
				sprintf(str, "Event %d   No task.", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
			else{
				EventSkip[id] = true;

				sprintf(str, "Skipped event [%d].", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
				sprintf(str, " ([%d][%d][%d][%d])", data.p1, data.p2, data.p3, data.p4);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}

	//configの情報
	if( strcmp(NewCommand, "config") == 0 ){
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "[Basic Config]");

		//キーコード表示
		char str2[8];
		char str3[MAX_CONSOLELEN];
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), " Keycode : ");
		for(int i=0; i*10<TOTAL_ControlKey; i++){
			strcpy(str, " ");
			for(int j=0; j<10; j++){
				if( i*10+j >= TOTAL_ControlKey ){ break; }
				sprintf(str2, " 0x%02X", GameConfig.GetKeycode(i*10+j));
				strcat(str, str2);
			}
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}

		strcpy(str, " ");

		//マウス感度
		sprintf(str3, "MouseSensitivity : %2d           ", GameConfig.GetMouseSensitivity());
		strcat(str, str3);

		//画面表示モード
		if( GameConfig.GetFullscreenFlag() == false ){
			strcat(str, "FullscreenFlag : false (window)");
		}
		else{
			strcat(str, "FullscreenFlag : true (fullscreen)");
		}

		//表示
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		strcpy(str, " ");

		//効果音設定
		if( GameConfig.GetSoundFlag() == false ){
			strcat(str, "SoundFlag : false (OFF)         ");
		}
		else{
			strcat(str, "SoundFlag : true (ON)           ");
		}

		//出血設定
		if( GameConfig.GetBloodFlag() == false ){
			strcat(str, "BloodFlag : false (OFF)");
		}
		else{
			strcat(str, "BloodFlag : true (ON)");
		}

		//表示
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		strcpy(str, " ");

		//画面の明るさ設定
		sprintf(str3, "Brightness : %2d                 ", GameConfig.GetBrightness());
		strcat(str, str3);

		//マウス反転設定
		if( GameConfig.GetInvertMouseFlag() == false ){
			strcat(str, "InvertMouseFlag : false (OFF)");
		}
		else{
			strcat(str, "InvertMouseFlag : true (ON)");
		}

		//表示
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		strcpy(str, " ");

		//フレームスキップ設定
		if( GameConfig.GetFrameskipFlag() == false ){
			strcat(str, "FrameskipFlag : false (OFF)     ");
		}
		else{
			strcat(str, "FrameskipFlag : true (ON)       ");
		}

		//別の照準を使用設定
		if( GameConfig.GetAnotherGunsightFlag() == false ){
			strcat(str, "AnotherGunsightFlag : false (OFF)");
		}
		else{
			strcat(str, "AnotherGunsightFlag : true (ON)");
		}

		//表示
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		strcpy(str, " ");

		//プレイヤー名表示
		char namestr[MAX_PLAYERNAME+1];
		GameConfig.GetPlayerName(namestr);
		strcat(str, "PlayerName : ");
		strcat(str, namestr);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "[Add Config]");

		strcpy(str, " ");

		//言語設定
		sprintf(str3, "Language : %d     ", GameConfig.GetLanguage());
		strcat(str, str3);

		//画面解像度
		sprintf(str3, "ScreenSize : %dx%d     ", GameConfig.GetScreenWidth(), GameConfig.GetScreenHeight());
		strcat(str, str3);

		//音量設定
		sprintf(str3, "MasterVolume : %d%%", (int)(GameConfig.GetVolume()*100));
		strcat(str, str3);

		//表示
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
	}

	//MIFの情報表示
	if( strcmp(NewCommand, "mif") == 0 ){
		bool AddonFlag;
		int MissionID = GameInfoData.selectmission_id;
		char str2[MAX_PATH];
		char str3[MAX_PATH];
		bool collisionflag, screenflag;

		if( GameInfoData.selectmission_id >= 0 ){ AddonFlag = false; }
		else{ AddonFlag = true; }

		//ヘッダー
		if( AddonFlag == true ){ sprintf(str, "[Addon Mission]   (MissionID:%d)", MissionID); }
		else{ sprintf(str, "[Standard Mission]   (MissionID:%d)", MissionID); }
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//MIFファイルパス
		strcpy(str, "MIFpath : ");
		strcpy(str2, GameInfoData.mifpath);
		str2[(MAX_CONSOLELEN - strlen(str) - 1)] = '\0';
		strcat(str, str2);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//ミッション識別名
		strcpy(str, "Name : ");
		if( AddonFlag == true ){ strcpy(str2, MIFdata.GetMissionName()); }
		else{ GameParamInfo.GetOfficialMission(MissionID, str2, NULL, NULL, NULL, NULL, NULL); }
		str2[(MAX_CONSOLELEN - strlen(str) - 1)] = '\0';
		strcat(str, str2);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//ミッション正式名称
		strcpy(str, "FullName : ");
		if( AddonFlag == true ){ strcpy(str2, MIFdata.GetMissionFullname()); }
		else{ GameParamInfo.GetOfficialMission(MissionID, NULL, str2, NULL, NULL, NULL, NULL); }
		str2[(MAX_CONSOLELEN - strlen(str) - 1)] = '\0';
		strcat(str, str2);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//ブロックデータファイル
		strcpy(str, "BD1file : ");
		if( AddonFlag == true ){
			MIFdata.GetDatafilePath(str2, str3);
			str2[(MAX_CONSOLELEN - strlen(str) - 1)] = '\0';
			strcat(str, str2);
		}
		else{
			GameParamInfo.GetOfficialMission(MissionID, NULL, NULL, str2, NULL, NULL, NULL);
			str2[(MAX_CONSOLELEN - strlen(str) - 8 - 1)] = '\0';
			strcat(str, str2);
			strcat(str, OFFICIALMISSION_BD1);
		}
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//ポイントデータファイル
		strcpy(str, "PD1file : ");
		if( AddonFlag == true ){
			MIFdata.GetDatafilePath(str3, str2);
			str2[(MAX_CONSOLELEN - strlen(str) - 1)] = '\0';
			strcat(str, str2);
		}
		else{
			GameParamInfo.GetOfficialMission(MissionID, NULL, NULL, str2, str3, NULL, NULL);
			strcat(str2, str3);
			str2[(MAX_CONSOLELEN - strlen(str) - 4 - 1)] = '\0';
			strcat(str, str2);
			strcat(str, ".pd1");
		}
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//追加小物情報ファイル
		strcpy(str, "AddOBJfile : ");
		strcpy(str2, MIFdata.GetAddSmallobjectFile());
		str2[(MAX_CONSOLELEN - strlen(str) - 1)] = '\0';
		strcat(str, str2);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//各設定値とFlag
		if( AddonFlag == true ){
			collisionflag = MIFdata.GetCollisionFlag();
			screenflag = MIFdata.GetScreenFlag();
		}
		else{
			GameParamInfo.GetOfficialMission(MissionID, NULL, NULL, NULL, NULL, &collisionflag, &screenflag);
		}
		sprintf(str, "Sky:%d    CollisionFlag:%d    DarkScreenFlag:%d", MIFdata.GetSkynumber(), (int)collisionflag, (int)screenflag);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
	}

	//ブロックデータの情報
	if( strcmp(NewCommand, "bd1") == 0 ){
		bool AddonFlag;
		int MissionID = GameInfoData.selectmission_id;
		char str2[MAX_PATH];
		char str3[MAX_PATH];
		char fname1[_MAX_PATH];
		char fname2[_MAX_PATH];
		char flagstr1[4];
		char flagstr2[4];

		if( GameInfoData.selectmission_id >= 0 ){ AddonFlag = false; }
		else{ AddonFlag = true; }

		//ファイル名表示
		strcpy(str, "Filename : ");
		if( AddonFlag == true ){
			MIFdata.GetDatafilePath(str2, str3);
			str2[(MAX_CONSOLELEN - strlen(str) - 1)] = '\0';
			strcat(str, str2);
		}
		else{
			GameParamInfo.GetOfficialMission(MissionID, NULL, NULL, str2, NULL, NULL, NULL);
			str2[(MAX_CONSOLELEN - strlen(str) - 8 - 1)] = '\0';
			strcat(str, str2); 
			strcat(str, OFFICIALMISSION_BD1);
		}
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//合計ブロック数表示
		sprintf(str, "TotalBlocks : %d", BlockData.GetTotaldatas());
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//マップテクスチャの読み込み状況表示
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Texture : ");
		for(int i=0; i<(TOTAL_BLOCKTEXTURE/2); i++){
			//テクスチャIDを取得
			BlockData.GetTexture(fname1, i);
			BlockData.GetTexture(fname2, i + (TOTAL_BLOCKTEXTURE/2));
			
			//テクスチャが読み込まれているか判定
			if( d3dg->GetMapTextureID(i) == -1 ){ strcpy(flagstr1, "NG"); }
			else{ strcpy(flagstr1, "OK"); }
			if( d3dg->GetMapTextureID(i + (TOTAL_BLOCKTEXTURE/2)) == -1 ){ strcpy(flagstr2, "NG"); }
			else{ strcpy(flagstr2, "OK"); }

			//表示
			sprintf(str, " %02d_%s %-31s %02d_%s %s", i, flagstr1, fname1, i + (TOTAL_BLOCKTEXTURE/2), flagstr2, fname2);
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}
	}

	//ポイントデータの情報
	if( strcmp(NewCommand, "pd1") == 0 ){
		bool AddonFlag;
		int MissionID = GameInfoData.selectmission_id;
		char str2[MAX_PATH];
		char str3[MAX_PATH];
		pointdata pdata;
		int TotalPoints = PointData.GetTotaldatas();
		int HumanPoints = 0;
		int WeaponPoints = 0;
		int OjectPoints = 0;
		int HumaninfoPoints = 0;
		int PathPoints = 0;
		int EventPoints = 0;
		int info_paramid1 = 0;
		int info_paramid2 = 0;
		int info_textureid1 = 0;
		int info_textureid2 = 0;
		char fname1[_MAX_PATH];
		char fname2[_MAX_PATH];
		char flagstr1[4];
		char flagstr2[4];

		if( GameInfoData.selectmission_id >= 0 ){ AddonFlag = false; }
		else{ AddonFlag = true; }

		//ファイル名表示
		strcpy(str, "Filename : ");
		if( AddonFlag == true ){
			MIFdata.GetDatafilePath(str3, str2);
			str2[(MAX_CONSOLELEN - strlen(str) - 1)] = '\0';
			strcat(str, str2);
		}
		else{
			GameParamInfo.GetOfficialMission(MissionID, NULL, NULL, str2, str3, NULL, NULL);
			strcat(str2, str3);
			str2[(MAX_CONSOLELEN - strlen(str) - 4 - 1)] = '\0';
			strcat(str, str2);
			strcat(str, ".pd1");
		}
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//合計ポイント数表示
		sprintf(str, "TotalPoints : %d", TotalPoints);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//各種ポイント数を数えて表示
		for(int i=0; i<TotalPoints; i++){
			if( PointData.Getdata(&pdata, i) != 0 ){ continue; }

			if( (pdata.p1 == POINT_P1TYPE_HUMAN)||(pdata.p1 == POINT_P1TYPE_HUMAN2) ){ HumanPoints += 1; }
			if( (pdata.p1 == POINT_P1TYPE_WEAPON)||(pdata.p1 == POINT_P1TYPE_RAND_WEAPON) ){ WeaponPoints += 1; }
			if( pdata.p1 == POINT_P1TYPE_SMALLOBJ ){ OjectPoints += 1; }
			if( pdata.p1 == POINT_P1TYPE_HUMANINFO ){ HumaninfoPoints += 1; }
			if( (pdata.p1 == POINT_P1TYPE_AIPATH)||(pdata.p1 == POINT_P1TYPE_RAND_AIPATH) ){ PathPoints += 1; }
			if( (10 <= pdata.p1)&&(pdata.p1 <= 19) ){ EventPoints += 1; }
		}
		sprintf(str, "HumanPoints : %-3d       WeaponPoints : %-3d    SmallOjectPoints : %-3d", HumanPoints, WeaponPoints, OjectPoints);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		sprintf(str, "HumaninfoPoints : %-3d   AIpathPoints : %-3d    EventPoints : %-3d", HumaninfoPoints, PathPoints, EventPoints);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//無効なポイント数を逆算で求める
		sprintf(str, "InvalidPoints : %d", TotalPoints - (HumanPoints + WeaponPoints + OjectPoints + HumaninfoPoints + PathPoints + EventPoints));
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//人のテクスチャ数を取得
		sprintf(str, "HumanTextures : %d/%d", Resource.GetHumanTextures(), MAX_LOADHUMANTEXTURE);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//人情報テクスチャーの読み込み状況表示
		for(int i=0; i<(MAX_LOADHUMANTEXTURE/2); i++){
			//テクスチャIDを取得
			Resource.GetHumanTextureInfo(i, &info_paramid1, &info_textureid1);
			Resource.GetHumanTextureInfo(i + (MAX_LOADHUMANTEXTURE/2), &info_paramid2, &info_textureid2);

			if( GameParamInfo.GetHumanTexturePath(info_paramid1, fname1) == 1 ){ strcpy(fname1, ""); }
			if( GameParamInfo.GetHumanTexturePath(info_paramid2, fname2) == 1 ){ strcpy(fname2, ""); }

			//テクスチャが読み込まれているか判定
			if( info_textureid1 == -1 ){ strcpy(flagstr1, "NG"); }
			else{ strcpy(flagstr1, "OK"); }
			if( info_textureid2 == -1 ){ strcpy(flagstr2, "NG"); }
			else{ strcpy(flagstr2, "OK"); }

			//表示
			sprintf(str, " %02d_%s %-31s %02d_%s %s", i, flagstr1, fname1, i + (MAX_LOADHUMANTEXTURE/2), flagstr2, fname2);
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}
	}

	//リソース情報表示
	if( strcmp(NewCommand, "resinfo") == 0 ){
		int human = 0;
		int weapon = 0;
		int smallobject = 0;
		int bullet = 0;
		int grenade = 0;
		int effect = 0;

		//グラフィック
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "[Graphics]");
		sprintf(str, " model : %2d/%2d        texture : %2d/%2d", d3dg->GetTotalModels(), MAX_MODEL, d3dg->GetTotalTextures(), MAX_TEXTURE);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//サウンド
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "[Sound]");
		sprintf(str, " sound : %2d/%2d", SoundCtrl.GetTotalSounds(), MAX_LOADSOUND);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//データファイル
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "[Datafile]");
		sprintf(str, " blocks : %3d/%3d     points : %3d/%3d", BlockData.GetTotaldatas(), MAX_BLOCKS, PointData.GetTotaldatas(), MAX_POINTS);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);

		//オブジェクト数
		ObjMgr.GetTotalObjects(&human, &weapon, &smallobject, &bullet, &grenade, &effect);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "[Object]");
		sprintf(str, " human : %3d/%3d      weapon : %3d/%3d      smallobject : %2d/%2d", human, MAX_HUMAN, weapon, MAX_WEAPON, smallobject, MAX_SMALLOBJECT);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		sprintf(str, " bullet : %3d/%3d     grenade : %3d/%3d     effect : %3d/%3d", bullet, MAX_BULLET, grenade, MAX_GRENADE, effect, MAX_EFFECT);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		sprintf(str, " sound : %3d/%3d", GameSound->GetTotalSoundList(), MAX_SOUNDMGR_LIST);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
	}

	//デバック用文字の表示
	if( strcmp(NewCommand, "info") == 0 ){
		if( ShowInfo_Debugmode == false ){
			ShowInfo_Debugmode = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Show Debug information.");
		}
		else{
			ShowInfo_Debugmode = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Hide Debug information.");
		}
	}

	//フリーカメラ切り替え
	if( strcmp(NewCommand, "view") == 0 ){
		if( Camera_Debugmode == false ){
			Camera_Debugmode = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Change FreeView mode.");
		}
		else{
			Camera_Debugmode = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Change PlayerView mode.");
		}

		//デバックモード開始時のカメラ設定
		if( Camera_Debugmode == true ){
			camera_x = 100.0f;
			camera_y = 100.0f;
			camera_z = -100.0f;
			camera_rx = DegreeToRadian(135);
			camera_ry = DegreeToRadian(-40);
		}
	}

	//3Dの中心線表示
	if( strcmp(NewCommand, "center") == 0 ){
		if( CenterLine == false ){
			CenterLine = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Show World center position.");
		}
		else{
			CenterLine = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Hide World center position.");
		}
	}

	//マップをワイヤーフレーム化
	if( strcmp(NewCommand, "map") == 0 ){
		if( wireframe == false ){
			wireframe = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Draw map on the Wire frame.");
		}
		else{
			wireframe = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Draw map on the Normal.");
		}
	}

	//モデル描画フラグ切り替え
	if( strcmp(NewCommand, "model") == 0 ){
		if( nomodel == false ){
			nomodel = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Disable render model.");
		}
		else{
			nomodel = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Enable render model.");
		}
	}

	//AI情報表示
	if( GetCommandNum("aiinfo", &id) == true ){
		if( (0 <= id)&&(id < MAX_HUMAN) ){
			if( AIdebuginfoID == id ){
				//同じ番号が指定されたらなら、無効化
				AIdebuginfoID = -1;
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Disable AI Debug information.");
			}
			else{
				//新たに人を指定
				AIdebuginfoID = id;
				sprintf(str, "Enable AI Debug information. Human[%d].", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}
	else if( strcmp(NewCommand, "aiinfo") == 0 ){
		if( AIdebuginfoID != -1 ){
			//人が指定されていたら、無効化
			AIdebuginfoID = -1;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Disable AI Debug information.");
		}
	}

	//オブジェクトのタグを表示
	if( strcmp(NewCommand, "tag") == 0 ){
		if( tag == false ){
			tag = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Show object information.");
		}
		else{
			tag = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Hide object information.");
		}
	}

	//レーダーを描画
	if( strcmp(NewCommand, "radar") == 0 ){
		if( radar == false ){
			radar = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Show Radar.");
		}
		else{
			radar = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Hide Radar.");
		}
	}

	//目隠しを描画
	if( strcmp(NewCommand, "inmap") == 0 ){
		if( Camera_Blind == false ){
			Camera_Blind = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Blindfold in map.");
		}
		else{
			Camera_Blind = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Not blindfold in map.");
		}
	}

	//背景空の変更
	if( GetCommandNum("sky", &id) == true ){
		if( (0 <= id)&&(id <= 5) ){
			SkyNumber = id;

			//リソース再構築
			Resource.CleanupSkyModelTexture();
			Resource.LoadSkyModelTexture(SkyNumber);

			sprintf(str, "Select SkyNumber %d.", id);
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}
	}

	//画面を暗く
	if( strcmp(NewCommand, "dark") == 0 ){
		char path[MAX_PATH];
		char bdata[MAX_PATH];
		char pdata[MAX_PATH];

		//フラグ切り替え
		if( DarkScreenFlag == false ){
			DarkScreenFlag = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Enable DarkScreen Flag.");
		}
		else{
			DarkScreenFlag = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Disable DarkScreen Flag.");
		}

		//.bd1のファイルパスを求める
		if( MIFdata.GetFiletype() == false ){
			GameParamInfo.GetOfficialMission(MainGameInfo.selectmission_id, NULL, NULL, path, NULL, NULL, NULL);
		}
		else{
			MIFdata.GetDatafilePath(bdata, pdata);
			GetFileDirectory(bdata, path);
		}

		//ブロックデータ初期化
		BlockData.CalculationBlockdata(DarkScreenFlag);
		d3dg->CleanupMapdata();
		d3dg->LoadMapdata(&BlockData, path);
	}

	//フォグ切り替え
	if( strcmp(NewCommand, "fog") == 0 ){
		//フラグ切り替え
		if( FogFlag == false ){
			FogFlag = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Enable Fog Flag.");
		}
		else{
			FogFlag = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Disable Fog Flag.");
		}
	}

	//プレイヤー移動
	if( GetCommandNum("teleport", &id) == true ){
		if( (0 <= id)&&(id < MAX_HUMAN)&&(id != ObjMgr.GetPlayerID()) ){
			float x, y, z, rx;
			human *thuman = ObjMgr.GetHumanObject(id);

			//ターゲットの座標取得
			thuman->GetPosData(&x, &y, &z, NULL);

			//プレイヤー移動
			ObjMgr.GetPlayerHumanObject()->GetPosData(NULL, NULL ,NULL, &rx);
			ObjMgr.GetPlayerHumanObject()->SetPosData(x + 5.0f, y + 5.0f, z + 5.0f, rx);

			sprintf(str, "Teleported player to Human[%d].", id);
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}
	}

	//プレイヤー切り替え
	if( GetCommandNum("player", &id) == true ){
		if( (0 <= id)&&(id < MAX_HUMAN)&&(id != ObjMgr.GetPlayerID()) ){
			//対象プレイヤー番号を適用
			ObjMgr.SetPlayerID(id);

			//プレイヤーの向きを取得
			ObjMgr.GetPlayerHumanObject()->GetRxRy(&mouse_rx, &mouse_ry);

			//F1モード時にカメラの向きを再設定
			if( Camera_F1mode == true ){
				camera_rx = DegreeToRadian(90);
			}

			sprintf(str, "Changed player to Human[%d].", id);
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}
	}

	//全ての死者を蘇生する
	if( strcmp(NewCommand, "revive") == 0 ){
		for(int i=0; i<MAX_HUMAN; i++){
			ObjMgr.HumanResuscitation(i);
		}
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "All human has resuscitation.");
	}

	//回復する
	if( GetCommandNum("treat", &id) == true ){
		if( (0 <= id)&&(id < MAX_HUMAN) ){
			int param, hp;
			HumanParameter data;
			human *thuman = ObjMgr.GetHumanObject(id);
			
			//初期時のHPを取得
			thuman->GetParamData(&param, NULL, NULL, NULL);
			GameParamInfo.GetHuman(param, &data);
			hp = data.hp;

			//回復
			if( thuman->SetHP(hp) == true ){
				sprintf(str, "Set the HP:%d to Human[%d].", data.hp, id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}

	//無敵化
	if( GetCommandNum("nodamage", &id) == true ){
		if( (0 <= id)&&(id < MAX_HUMAN) ){
			if( InvincibleID == id ){
				//同じ番号が指定されたらなら、無効化
				InvincibleID = -1;
				ObjMgr.GetHumanObject(id)->SetInvincibleFlag(false);
				sprintf(str, "Not invincible Human[%d].", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
			else{
				//既に誰かが指定されていたら、既に指定されている人を無効化
				if( InvincibleID != -1 ){
					ObjMgr.GetHumanObject(InvincibleID)->SetInvincibleFlag(false);
					sprintf(str, "Not invincible Human[%d].", InvincibleID);
					AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
				}

				//新たに無敵状態の人を指定
				InvincibleID = id;
				ObjMgr.GetHumanObject(id)->SetInvincibleFlag(true);
				sprintf(str, "Invincible Human[%d].", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}
	else if( strcmp(NewCommand, "nodamage") == 0 ){
		if( InvincibleID != -1 ){
			//人が指定されていたら、無効化
			ObjMgr.GetHumanObject(InvincibleID)->SetInvincibleFlag(false);
			sprintf(str, "Not invincible Human[%d].", InvincibleID);
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			InvincibleID = -1;
		}
	}

	//殺害
	if( GetCommandNum("kill", &id) == true ){
		if( (0 <= id)&&(id < MAX_HUMAN) ){
			human *thuman = ObjMgr.GetHumanObject(id);
			if( thuman->GetEnableFlag() == true ){
				if( thuman->SetHP(0) == true ){
					sprintf(str, "Killed Human[%d].", id);
					AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
				}
			}
		}
	}

	//小物を新規配置
	if( GetCommandNum("newobj", &id) == true ){
		if( (0 <= id)&&(id < TOTAL_PARAMETERINFO_SMALLOBJECT) ){
			int dataid = ObjMgr.AddSmallObjectIndex(camera_x + cosf(camera_rx)*20.0f, camera_y, camera_z + sinf(camera_rx)*20.0f, camera_rx*-1, id, true);
			if( dataid != -1 ){
				sprintf(str, "Add SmallObject[%d].", dataid);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}

	//小物を破壊
	if( GetCommandNum("break", &id) == true ){
		if( (0 <= id)&&(id < MAX_SMALLOBJECT) ){
			smallobject *tsmallobject = ObjMgr.GetSmallObject(id);
			if( tsmallobject->GetEnableFlag() == true ){
				tsmallobject->Destruction();
				sprintf(str, "Broke SmallObject[%d].", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}

	//人を削除
	if( GetCommandNum("delhuman", &id) == true ){
		if( (0 <= id)&&(id < MAX_HUMAN) ){
			human *thuman = ObjMgr.GetHumanObject(id);
			if( thuman->GetEnableFlag() == true ){

				//人が持っている武器もすべて無効にする
				int dummy;
				class weapon *weaponlist[TOTAL_HAVEWEAPON];
				int lnbs[TOTAL_HAVEWEAPON];
				int nbs[TOTAL_HAVEWEAPON];
				thuman->GetWeapon(&dummy, weaponlist, NULL, NULL);
				for(int i=0; i<TOTAL_HAVEWEAPON; i++){
					if( weaponlist[i] != NULL ){
						weaponlist[i]->SetEnableFlag(false);
						weaponlist[i] = NULL;
					}
					lnbs[i] = 0;
					nbs[i] = 0;
				}
				thuman->SetWeapon(weaponlist, lnbs, nbs);

				thuman->SetEnableFlag(false);
				sprintf(str, "Delete human[%d].", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}

	//武器を削除
	if( GetCommandNum("delweapon", &id) == true ){
		if( (0 <= id)&&(id < MAX_WEAPON) ){
			weapon *tweapon = ObjMgr.GetWeaponObject(id);
			if( tweapon->GetEnableFlag() == true ){
				
				//人がその武器を持っているなら、関連付けを外す（捨てさせる）
				int dummy;
				class weapon *weaponlist[TOTAL_HAVEWEAPON];
				int lnbs[TOTAL_HAVEWEAPON];
				int nbs[TOTAL_HAVEWEAPON];
				for(int i=0; i<MAX_HUMAN; i++){
					human *thuman = ObjMgr.GetHumanObject(i);
					if( thuman->GetEnableFlag() == true ){
						thuman->GetWeapon(&dummy, weaponlist, lnbs, nbs);
						for(int j=0; j<TOTAL_HAVEWEAPON; j++){
							if( weaponlist[j] == tweapon ){
								weaponlist[j] = NULL;
							}
						}
						thuman->SetWeapon(weaponlist, lnbs, nbs);
					}
				}

				tweapon->SetEnableFlag(false);
				sprintf(str, "Delete weapon[%d].", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}

	//小物を削除
	if( GetCommandNum("delobj", &id) == true ){
		if( (0 <= id)&&(id < MAX_SMALLOBJECT) ){
			smallobject *tsmallobject = ObjMgr.GetSmallObject(id);
			if( tsmallobject->GetEnableFlag() == true ){
				tsmallobject->SetEnableFlag(false);
				sprintf(str, "Delete SmallObject[%d].", id);
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
			}
		}
	}

	//FF（同士討ち）有効化
	if( strcmp(NewCommand, "ff") == 0 ){
		if( ObjMgr.GetFriendlyFireFlag(0) == false ){	//とりあえず No.0 で判定
			for(int i=0; i<MAX_HUMAN; i++){
				ObjMgr.SetFriendlyFireFlag(i, true);
			}
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Enable Friendly Fire.");
		}
		else{
			for(int i=0; i<MAX_HUMAN; i++){
				ObjMgr.SetFriendlyFireFlag(i, false);
			}
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Disable Friendly Fire.");
		}
	}

	//プレイヤー操作をAI化
	if( strcmp(NewCommand, "bot") == 0 ){
		if( PlayerAI == false ){
			PlayerAI = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Player is AI control. (bot)");
		}
		else{
			PlayerAI = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Player is User control.");
		}
	}

	//AIを非戦闘化させる
	if( strcmp(NewCommand, "nofight") == 0 ){
		if( AINoFight == false ){
			AINoFight = true;
			for(int i=0; i<MAX_HUMAN; i++){
				HumanAI[i].SetNoFightFlag(true);
			}
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "AI had no Fight.");
		}
		else{
			AINoFight = false;
			for(int i=0; i<MAX_HUMAN; i++){
				HumanAI[i].SetNoFightFlag(false);
			}
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "AI had Fight.");
		}
	}

	//AIを警戒させる
	if( strcmp(NewCommand, "caution") == 0 ){
		for(int i=0; i<MAX_HUMAN; i++){
			HumanAI[i].SetCautionMode();
		}
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Set cautious AI.");
	}

	//AIレベル設定
	if( GetCommandNum("ailevel", &id) == true ){
		if( id == -1 ){
			//AIレベルをデフォルト値へ戻す
			for(int i=0; i<MAX_HUMAN; i++){
				HumanAI[i].SetAIlevel(-1);
			}
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Set all human AI-level to default.");
		}
		if( (0 <= id)&&(id < TOTAL_PARAMETERINFO_AILEVEL) ){
			//AIレベルを指定値へ上書き
			for(int i=0; i<MAX_HUMAN; i++){
				HumanAI[i].SetAIlevel(id);
			}
			sprintf(str, "Set all human to AI-level [%d].", id);
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}
	}
	else if( strcmp(NewCommand, "ailevel") == 0 ){
		//AIレベルをデフォルト値へ戻す
		for(int i=0; i<MAX_HUMAN; i++){
			HumanAI[i].SetAIlevel(-1);
		}
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Set all human AI-level to default.");
	}

	//AIの処理を停止
	if( strcmp(NewCommand, "stop") == 0 ){
		if( AIstop == false ){
			AIstop = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Stopped AI control.");
		}
		else{
			AIstop = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Started AI control.");
		}
	}

	/*
	//ミッション達成
	if( strcmp(NewCommand, "comp") == 0 ){
		end_framecnt += 1;
		MainGameInfo.missioncomplete = true;
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Mission complete.");
	}

	//ミッション失敗
	if( strcmp(NewCommand, "fail") == 0 ){
		end_framecnt += 1;
		MainGameInfo.missioncomplete = false;
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Mission fail.");
	}
	*/

	//イベントポイントの処理を停止
	if( strcmp(NewCommand, "estop") == 0 ){
		if( EventStop == false ){
			EventStop = true;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Stopped Event chains.");
		}
		else{
			EventStop = false;
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Started Event chains.");
		}
	}

	//バージョン情報取得
	if( strcmp(NewCommand, "ver") == 0 ){
		sprintf(str, "%s   Version:%s", GAMENAME, GAMEVERSION);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		sprintf(str, " Graphics : %s", GRAPHICS_CORE);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		sprintf(str, " Sound : %s", SOUND_CORE);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		sprintf(str, " Input : %s", INPUT_CORE);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
	}

	//ゲームの実行速度
	if( strcmp(NewCommand, "speed") == 0 ){
		if( GameSpeed == 1 ){ GameSpeed = 2; }
		else if( GameSpeed == 2 ){ GameSpeed = 4; }
		else{ GameSpeed = 1; }

		sprintf(str, "Set GameSpeed x%d", GameSpeed);
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
	}

	//ウィンドウ・フルスクリーン切り替え
	if( strcmp(NewCommand, "window") == 0 ){
		//現在の表示モード取得
		bool flag = d3dg->GetFullScreenFlag();

		if( flag == false ){ flag = true; }
		else{ flag = false; }

		if( ChangeWindowMode(WindowCtrl, d3dg, inputCtrl, this, flag) != 0 ){
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "[Error] Change failed.");
		}
		else{
			if( flag == true ){
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Changed FullScreen mode.");
			}
			else{
				AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Changed Window mode.");
			}
		}
	}

	//スクリーンショットを撮影
	//　※コンソール画面を削除するため、撮影を2フレーム遅らせる。
	if( ScreenShot == 3 ){
		char fname[256];

		//ファイル名を決定
		GetTimeName(fname);
		strcat(fname, ".bmp");

		//撮影・保存
		if( d3dg->SaveScreenShot(fname) == true ){
			sprintf(str, "Saved Screenshot  (File:%s)", fname);
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), str);
		}
		else{
			AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "[Error] Save failed.");
		}

		ScreenShot = 0;
	}
	else{
		if( strcmp(NewCommand, "ss") == 0 ){
			ScreenShot = 1;
		}
	}

	//コンソールをクリア
	if( strcmp(NewCommand, "clear") == 0 ){
		for(int i=0; i<MAX_CONSOLELINES; i++){
			InfoConsoleData[i].color = d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f);
			InfoConsoleData[i].textdata[0] = '\0';
		}
	}

	//コンソールを閉じる
	if( strcmp(NewCommand, "exit") == 0 ){
		AddInfoConsole(d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), "Closed debug console.");
		Show_Console = false;
	}

#ifdef _DEBUG
	//リセット操作
	if( strcmp(NewCommand, "f12") == 0 ){
		GameState->PushF12Key();
		GameSpeed = 1;
	}
#endif

	if( strcmp(NewCommand, "mikan") == 0 ){
		AddInfoConsole(d3dg->GetColorCode(1.0f,0.5f,0.0f,1.0f), "Hello! I'm [-_-;](mikan).");
		AddInfoConsole(d3dg->GetColorCode(1.0f,0.5f,0.0f,1.0f), "Thank you for playing OpenXOPS!");
	}
}

//! @brief デバック用コンソールの表示処理
void maingame::RenderConsole()
{
	//下地
	d3dg->Draw2DBox(0, 0, GameConfig.GetScreenWidth(), (MAX_CONSOLELINES+1)*17 + 5 + 5, d3dg->GetColorCode(0.0f,0.0f,0.0f,0.75f));

	//表示中の文字
	for(int i=0; i<MAX_CONSOLELINES; i++){
		if( InfoConsoleData[i].textdata[0] != NULL ){
			d3dg->Draw2DTextureDebugFontText(5+1, i*17+5+1, InfoConsoleData[i].textdata, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
			d3dg->Draw2DTextureDebugFontText(5, i*17+5, InfoConsoleData[i].textdata, InfoConsoleData[i].color);
		}
	}

	//入力中の文字
	int cnt;
	char str[MAX_CONSOLELEN];
	for(cnt=0; cnt<MAX_CONSOLELINES; cnt++){
		if( InfoConsoleData[cnt].textdata[0] == NULL ){ break; }
	}
	strcpy(str, CONSOLE_PROMPT);
	strcat(str, ConsoleTextBoxControl.GetTextBoxStr());
	d3dg->Draw2DTextureDebugFontText(5+1, cnt*17+5+1, str, d3dg->GetColorCode(0.0f,0.0f,0.0f,1.0f));
	d3dg->Draw2DTextureDebugFontText(5, cnt*17+5, str, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
}

#endif

void maingame::Destroy()
{
	g_netSession.Stop();	//[XOPS-ONLINE] leaving the mission ends the match for this peer

	//ブロックデータ解放
	d3dg->CleanupMapdata();

	//オブジェクトマネージャー解放
	ObjMgr.Cleanup();

	//追加小物を解放
	for(int i=0; i<MAX_ADDSMALLOBJECT; i++){
		Resource.CleanupAddSmallObject(i);
	}

	//背景空解放
	Resource.CleanupSkyModelTexture();

#ifdef ENABLE_DEBUGCONSOLE
	//コンソール用データを解放
	if( InfoConsoleData != NULL ){ delete [] InfoConsoleData; }
	ConsoleTextBoxControl.DestroyTextBox();
#endif

	GameState->NextState();
}

//! @brief コンストラクタ
result::result()
{}

//! @brief ディストラクタ
result::~result()
{}

//! @brief リザルト画面の2D描画部分
void result::Render2D()
{
	char mname[64];
	char str[32];
	float effectA = GetEffectAlphaLoop(framecnt, 1.0f, 0.8f, true);
	int intontarget;
	float rate;

	//命中率計算
	intontarget = (int)floor(GameInfoData.ontarget);
	if( GameInfoData.fire == 0 ){
		rate = 0.0f;
	}
	else{
		rate = (float)intontarget / GameInfoData.fire * 100;
	}

	//メモ：背景画像の描画は、自動的に行われる。

	//固定文字表示
	d3dg->Draw2DTextureFontTextCenterScaling(0, 40, "RESULT", d3dg->GetColorCode(1.0f,0.0f,1.0f,effectA), 50, 42);

	//ミッション名を取得し表示
	if( MIFdata.GetFiletype() == false ){
		GameParamInfo.GetOfficialMission(GameInfoData.selectmission_id, NULL, mname, NULL, NULL, NULL, NULL);
	}
	else{
		strcpy(mname, MIFdata.GetMissionFullname());
	}
	d3dg->Draw2DTextureFontTextCenterScaling(0, 100, mname, d3dg->GetColorCode(0.5f,0.5f,1.0f,1.0f), 18, 25);

	//ミッションクリアーの有無
	if( GameInfoData.missioncomplete == true ){
		d3dg->Draw2DTextureFontTextCenterScaling(0, 150, "mission successful", d3dg->GetColorCode(0.0f,1.0f,0.0f,1.0f), 24, 32);
	}
	else{
		d3dg->Draw2DTextureFontTextCenterScaling(0, 150, "mission failure", d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f), 24, 32);
	}

	//結果表示
	sprintf(str, "Time  %dmin %dsec", GameInfoData.framecnt/(int)GAMEFPS/60, GameInfoData.framecnt/(int)GAMEFPS%60);
	d3dg->Draw2DTextureFontTextCenterScaling(0, 210, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 20, 32);
	sprintf(str, "Rounds fired  %d", GameInfoData.fire);
	d3dg->Draw2DTextureFontTextCenterScaling(0, 260, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 20, 32);
	sprintf(str, "Rounds on target  %d", intontarget);
	d3dg->Draw2DTextureFontTextCenterScaling(0, 310, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 20, 32);
	sprintf(str, "Accuracy rate  %.1f%%", rate);
	d3dg->Draw2DTextureFontTextCenterScaling(0, 360, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 20, 32);
	sprintf(str, "Kill  %d / HeadShot  %d", GameInfoData.kill, GameInfoData.headshot);
	d3dg->Draw2DTextureFontTextCenterScaling(0, 410, str, d3dg->GetColorCode(1.0f,1.0f,1.0f,1.0f), 20, 32);
}

//! @brief screen派生クラスの初期化（クラスの設定）
void InitScreen(WindowControl *WindowCtrl, opening *Opening, mainmenu *MainMenu, briefing *Briefing, maingame *MainGame, result *Result)
{
	if( WindowCtrl == NULL ){ return; }
	if( Opening == NULL ){ return; }
	if( MainMenu == NULL ){ return; }
	if( Briefing == NULL ){ return; }
	if( MainGame == NULL ){ return; }
	if( Result == NULL ){ return; }

	Opening->SetClass(&GameState, WindowCtrl, &d3dg, &inputCtrl, &GameSound);
	MainMenu->SetClass(&GameState, WindowCtrl, &d3dg, &inputCtrl, &GameSound);
	Briefing->SetClass(&GameState, WindowCtrl, &d3dg, &inputCtrl);
	MainGame->SetClass(&GameState, WindowCtrl, &d3dg, &inputCtrl, &GameSound);
	Result->SetClass(&GameState, WindowCtrl, &d3dg, &inputCtrl);
}

//! @brief screen派生クラスの実行
void ProcessScreen(WindowControl *WindowCtrl, opening *Opening, mainmenu *MainMenu, briefing *Briefing, maingame *MainGame, result *Result, unsigned int framecnt)
{
	if( WindowCtrl == NULL ){ return; }
	if( Opening == NULL ){ return; }
	if( MainMenu == NULL ){ return; }
	if( Briefing == NULL ){ return; }
	if( MainGame == NULL ){ return; }
	if( Result == NULL ){ return; }

	int error;

	switch(GameState.GetState()){
		//オープニング初期化
		case STATE_CREATE_OPENING:
			error = Opening->Create();
			if( error != 0 ){
				if( (error&0x0001) != 0 ){
					WindowCtrl->ErrorInfo("block data open failed");
				}
				if( (error&0x0002) != 0 ){
					WindowCtrl->ErrorInfo("point data open failed");
				}
				//WindowCtrl->CloseWindow();
				GameState.PushBackSpaceKey();
			}
			break;

		//オープニング実行
		case STATE_NOW_OPENING:
			Opening->Input();
			Opening->Process();
			Opening->Sound();
			if( (GameConfig.GetFrameskipFlag() == false)||(framecnt%2 == 0) ){
				if( Opening->RenderMain() == true ){
					int ReturnCode;
					ReturnCode = ResetGame(WindowCtrl);
					if( ReturnCode == 2 ){
						WindowCtrl->ErrorInfo("Resetに失敗しました");
						WindowCtrl->CloseWindow();
					}
					else if( ReturnCode == 0 ){
						Opening->Recovery();

						//現在の画面を再スタートさせる
						//GameState.PushF12Key();
					}
				}
			}
			break;

		//オープニング廃棄
		case STATE_DESTROY_OPENING:
			Opening->Destroy();
			break;

		//メニュー初期化
		case STATE_CREATE_MENU:
			error = MainMenu->Create();
			if( error != 0 ){
				if( (error&0x0001) != 0 ){
					WindowCtrl->ErrorInfo("block data open failed");
				}
				if( (error&0x0002) != 0 ){
					WindowCtrl->ErrorInfo("point data open failed");
				}
				WindowCtrl->CloseWindow();
			}
			break;

		//メニュー実行
		case STATE_NOW_MENU:
			MainMenu->Input();
			MainMenu->Process();
			MainMenu->Sound();
			if( (GameConfig.GetFrameskipFlag() == false)||(framecnt%2 == 0) ){
				if( MainMenu->RenderMain() == true ){
					int ReturnCode;
					ReturnCode = ResetGame(WindowCtrl);
					if( ReturnCode == 2 ){
						WindowCtrl->ErrorInfo("Resetに失敗しました");
						WindowCtrl->CloseWindow();
					}
					else if( ReturnCode == 0 ){
						MainMenu->Recovery();

						//現在の画面を再スタートさせる
						//GameState.PushF12Key();
					}
				}
			}
			break;

		//メニュー廃棄
		case STATE_DESTROY_MENU:
			MainMenu->Destroy();
			break;

		//ブリーフィング初期化
		case STATE_CREATE_BRIEFING:
			error = Briefing->Create();
			if( error == 1 ){
				WindowCtrl->ErrorInfo("briefing data open failed");
				WindowCtrl->CloseWindow();
			}
			break;

		//ブリーフィング実行
		case STATE_NOW_BRIEFING:
			Briefing->Input();
			Briefing->Process();
			if( (GameConfig.GetFrameskipFlag() == false)||(framecnt%2 == 0) ){
				if( Briefing->RenderMain() == true ){
					int ReturnCode;
					ReturnCode = ResetGame(WindowCtrl);
					if( ReturnCode == 2 ){
						WindowCtrl->ErrorInfo("Resetに失敗しました");
						WindowCtrl->CloseWindow();
					}
					else if( ReturnCode == 0 ){
						Briefing->Recovery();

						//現在の画面を再スタートさせる
						//GameState.PushF12Key();
					}
				}
			}
			break;

		//ブリーフィング廃棄
		case STATE_DESTROY_BRIEFING:
			Briefing->Destroy();
			break;

		//メインゲーム初期化
		case STATE_CREATE_MAINGAME:
			error = MainGame->Create();
			if( error != 0 ){
				if( (error&0x0001) != 0 ){
					WindowCtrl->ErrorInfo("block data open failed");
				}
				if( (error&0x0002) != 0 ){
					WindowCtrl->ErrorInfo("point data open failed");
				}
				WindowCtrl->CloseWindow();
			}
			break;

		//メインゲーム実行
		case STATE_NOW_MAINGAME:
			for(int i=0; i<MainGame->GetGameSpeed(); i++){
				MainGame->Input();
				MainGame->Process();
				MainGame->Sound();
			}
			if( (GameConfig.GetFrameskipFlag() == false)||(framecnt%2 == 0) ){
				if( MainGame->RenderMain() == true ){
					int ReturnCode;
					ReturnCode = ResetGame(WindowCtrl);
					if( ReturnCode == 2 ){
						WindowCtrl->ErrorInfo("Resetに失敗しました");
						WindowCtrl->CloseWindow();
					}
					else if( ReturnCode == 0 ){
						MainGame->Recovery();

						//現在の画面を再スタートさせる
						//GameState.PushF12Key();
					}
				}
			}
			break;

		//メインゲーム廃棄
		case STATE_DESTROY_MAINGAME:
			MainGame->Destroy();
			break;

		//リザルト初期化
		case STATE_CREATE_RESULT:
			error = Result->Create();
			break;

		//リザルト実行
		case STATE_NOW_RESULT:
			Result->Input();
			Result->Process();
			if( (GameConfig.GetFrameskipFlag() == false)||(framecnt%2 == 0) ){
				if( Result->RenderMain() == true ){
					int ReturnCode;
					ReturnCode = ResetGame(WindowCtrl);
					if( ReturnCode == 2 ){
						WindowCtrl->ErrorInfo("Resetに失敗しました");
						WindowCtrl->CloseWindow();
					}
					else if( ReturnCode == 0 ){
						Result->Recovery();

						//現在の画面を再スタートさせる
						//GameState.PushF12Key();
					}
				}
			}
			break;

		//リザルト廃棄
		case STATE_DESTROY_RESULT:
			Result->Destroy();
			break;

		//ゲーム終了
		case STATE_EXIT:
			WindowCtrl->CloseWindow();
			break;

		default:
			break;
	}
}

//! @brief ウィンドウ・フルスクリーン切り替え
//! @return 成功：0　失敗（ゲーム続行）：1　失敗（ゲーム終了）：2
int ChangeWindowMode(WindowControl *WindowCtrl, D3DGraphics *d3dg, InputControl *inputCtrl, scene *RecoveryScene, bool fullscreen)
{
	if( WindowCtrl == NULL ){ return 2; }
	if( d3dg == NULL ){ return 2; }
	if( inputCtrl == NULL ){ return 2; }
	if( RecoveryScene == NULL ){ return 2; }

	int returncode = 0;
	bool ErrorFlag = false;

	//切り替え処理（初回）
	WindowCtrl->ChangeWindowMode(fullscreen);
	d3dg->SetFullScreenFlag(fullscreen);
	if( ResetGame(WindowCtrl) != 0 ){
		if( fullscreen == false ){
			//フルスクリーン⇒ウィンドウが失敗したら、エラーとして終了。
			WindowCtrl->ErrorInfo("Resetに失敗しました");
			WindowCtrl->CloseWindow();
			ErrorFlag = true;
			returncode = 2;
		}
		else{
			//ウィンドウ⇒フルスクリーンが失敗したら、ウィンドウモードへ戻してゲーム続行を試みる。
			//　（GPUが指定解像度のフルスクリーンに対応してないとか・・？）
			fullscreen = false;

			WindowCtrl->ChangeWindowMode(fullscreen);
			d3dg->SetFullScreenFlag(fullscreen);
			if( ResetGame(WindowCtrl) != 0 ){
				//戻しても失敗するなら、エラーとして終了。
				WindowCtrl->ErrorInfo("Resetに失敗しました");
				WindowCtrl->CloseWindow();
				ErrorFlag = true;
				returncode = 2;
			}
			else{
				returncode = 1;
			}
		}
	}

	//切り替えに成功したら、回復などの後処理。
	if( ErrorFlag == false ){
		RecoveryScene->Recovery();

		//キー入力を取得
		//　※ディスプレイ解像度の変化によるマウスの移動分を捨てる
		inputCtrl->GetInputState(true);
		inputCtrl->MoveMouseCenter();
	}

	return returncode;
}