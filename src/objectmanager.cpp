//! @file objectmanager.cpp
//! @brief ObjectManagerクラスの定義

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

#include "objectmanager.h"

//! @brief コンストラクタ
ObjectManager::ObjectManager()
{
	StagedTotal = 0;	//[OpenXOPS-web]
	for(int i=0; i<MAX_HUMAN; i++){ Human_NetControlled[i] = false; }	//[XOPS-ONLINE]
	NetPuppet = false;	//[XOPS-ONLINE]
	ForceHumanSlot = -1;	//[XOPS-ONLINE]
	for(int i=0; i<MAX_HUMAN; i++){ Human_NetMove[i] = 0; }	//[XOPS-ONLINE]
	for(int i=0; i<MAX_BULLET; i++){ Bullet_LaserBolt[i] = false; }
	for(int i=0; i<MAX_HUMAN; i++){ StagedPending[i] = false; Human_NeedAIInit[i] = false; }
	HumanIndex = new human[MAX_HUMAN];
	WeaponIndex = new weapon[MAX_WEAPON];
	SmallObjectIndex = new smallobject[MAX_SMALLOBJECT];
	BulletIndex = new bullet[MAX_BULLET];
	GrenadeIndex = new grenade[MAX_GRENADE];
	EffectIndex = new effect[MAX_EFFECT];

	framecnt = 0;
	Human_ontarget = new float[MAX_HUMAN];
	Human_kill = new int[MAX_HUMAN];
	Human_headshot = new int[MAX_HUMAN];
	Human_ShotFlag = new bool[MAX_HUMAN];
	Human_YakkyouCnt = new int[MAX_HUMAN];
	BulletObj_HumanIndex = new BulletObjectHumanIndex[MAX_BULLET];
	AddCollisionFlag = false;
	FriendlyFire = new bool[MAX_HUMAN];
	Player_HumanID = 0;
	Human_FrameTextureRefresh = new bool[MAX_HUMAN];

	//[OpenXOPS-web] Give the per-human arrays the values LoadPointData() and the
	//statistics reset (around line 2714) would set. Upstream leaves them uninitialised;
	//when a map fails to load, Process() still reads Human_ShotFlag[] (UBSan:
	//"load of value 190, which is not a valid value for type 'bool'").
	for(int i=0; i<MAX_HUMAN; i++){
		Human_ontarget[i] = 0.0f;
		Human_kill[i] = 0;
		Human_headshot[i] = 0;
		Human_ShotFlag[i] = false;
		Human_YakkyouCnt[i] = -1;
		FriendlyFire[i] = false;
		Human_FrameTextureRefresh[i] = false;
	}

	GameParamInfo = NULL;
	d3dg = NULL;
	Resource = NULL;
	BlockData = NULL;
	PointData = NULL;
	CollD = NULL;
	GameSound = NULL;
	MIFdata = NULL;

	ObjectLog = new ObjectManagerLog;
}

//! @brief ディストラクタ
ObjectManager::~ObjectManager()
{
	Cleanup();

	if( HumanIndex != NULL ){ delete [] HumanIndex; }
	if( WeaponIndex != NULL ){ delete [] WeaponIndex; }
	if( SmallObjectIndex != NULL ){ delete [] SmallObjectIndex; }
	if( BulletIndex != NULL ){ delete [] BulletIndex; }
	if( GrenadeIndex != NULL ){ delete [] GrenadeIndex; }
	if( EffectIndex != NULL ){ delete [] EffectIndex; }

	if( Human_ontarget != NULL ){ delete [] Human_ontarget; }
	if( Human_kill != NULL ){ delete [] Human_kill; }
	if( Human_headshot != NULL ){ delete [] Human_headshot; }
	if( Human_ShotFlag != NULL ){ delete [] Human_ShotFlag; }
	if( Human_YakkyouCnt != NULL ){ delete [] Human_YakkyouCnt; }
	if( BulletObj_HumanIndex != NULL ){ delete [] BulletObj_HumanIndex; }
	if( FriendlyFire != NULL ){ delete [] FriendlyFire; }
	if( Human_FrameTextureRefresh != NULL ){ delete [] Human_FrameTextureRefresh; }

	if( ObjectLog != NULL ){ delete ObjectLog; }
}

//! @brief 参照するクラスを設定
//! @param in_GameParamInfo ゲーム設定データ管理クラス
//! @param in_d3dg 描画処理クラス
//! @param in_Resource リソース管理クラス
//! @param in_BlockData ブロックデータ管理クラス
//! @param in_PointData ポイントデータ管理クラス
//! @param in_CollD 当たり判定処理クラス
//! @param in_GameSound 効果音再生クラス
//! @param in_MIFdata MIFコントロールクラス
//! @attention この関数で設定を行わないと、クラス自体が正しく機能しません。
void ObjectManager::SetClass(ParameterInfo *in_GameParamInfo, D3DGraphics *in_d3dg, ResourceManager *in_Resource, BlockDataInterface *in_BlockData, PointDataInterface *in_PointData, Collision *in_CollD, SoundManager *in_GameSound, MIFInterface *in_MIFdata)
{
	GameParamInfo = in_GameParamInfo;
	d3dg = in_d3dg;
	Resource = in_Resource;
	BlockData = in_BlockData;
	PointData = in_PointData;
	CollD = in_CollD;
	GameSound = in_GameSound;
	MIFdata = in_MIFdata;

	for(int i=0; i<MAX_HUMAN; i++){
		HumanIndex[i].SetParameterInfoClass(GameParamInfo);
	}
	for(int i=0; i<MAX_WEAPON; i++){
		WeaponIndex[i].SetParameterInfoClass(GameParamInfo);
	}
	for(int i=0; i<MAX_SMALLOBJECT; i++){
		SmallObjectIndex[i].SetParameterInfoClass(GameParamInfo);
		SmallObjectIndex[i].SetMIFInterfaceClass(MIFdata);
	}
	for(int i=0; i<MAX_GRENADE; i++){
		GrenadeIndex[i].SetParameterInfoClass(GameParamInfo);
	}

	int bulletmodel, bullettexture;
	BulletParameter data;

	if( Resource->GetBulletModelTexture(0, &bulletmodel, &bullettexture) == 0 ){
		if( GameParamInfo->GetBullet(0, &data) == 0 ){
			for(int i=0; i<MAX_BULLET; i++){
				BulletIndex[i].SetModel(bulletmodel, data.size);
				BulletIndex[i].SetTexture(bullettexture);
			}
		}
	}

	if( Resource->GetBulletModelTexture(1, &bulletmodel, &bullettexture) == 0 ){
		if( GameParamInfo->GetBullet(1, &data) == 0 ){
			for(int i=0; i<MAX_GRENADE; i++){
				GrenadeIndex[i].SetModel(bulletmodel, data.size);
				GrenadeIndex[i].SetTexture(bullettexture);
			}
		}
	}

	ObjectLog->SetClass(d3dg);
}

//! @brief 人追加
//! @param data 人のポイントデータ　（pointdata構造体）
//! @param infodata 参照する人情報のポイントデータ　（〃）
//! @param RandomFlag ランダム補正を有効にする
//! @return 成功：データ番号（0以上）　失敗：-1
//! @attention 無効な人の種類番号が指定された場合は 通称：謎人間 が登場します。テクスチャはマップテクスチャ0番が使用され、HPは 0 が指定（＝即死）されます。
int ObjectManager::AddHumanIndex(pointdata data, pointdata infodata, bool RandomFlag)
{
	int GetHumanFlag;
	HumanParameter HumanParam;
	GetHumanFlag = GameParamInfo->GetHuman(infodata.p2, &HumanParam);
	int Humanindexid = -1;
	int Weaponindexid = -1;
	class weapon *Weapon[TOTAL_HAVEWEAPON];
	int lnbslist[TOTAL_HAVEWEAPON];
	int nbslist[TOTAL_HAVEWEAPON];
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		Weapon[i] = NULL;
		lnbslist[i] = 0;
		nbslist[i] = 0;
	}

#ifndef ENABLE_BUG_HUMANWEAPON
	if( GetHumanFlag != 0 ){
		return -1;
	}
#endif

	//人のモデル番号を取得
	int upmodel[TOTAL_UPMODE];
	int armmodel[TOTAL_ARMMODE];
	int legmodel;
	int walkmodel[TOTAL_WALKMODE];
	int runmodel[TOTAL_RUNMODE];
	Resource->GetHumanModel(upmodel, armmodel, &legmodel, walkmodel, runmodel);

	float px = data.x;
	float py = data.y;
	float pz = data.z;
	float rx = data.r;
	if( RandomFlag == true ){
		px += 0.001f*GetRand(100*2) - 0.1f;
		pz += 0.001f*GetRand(100*2) - 0.1f;
		rx += DegreeToRadian(0.01f)*GetRand(80*2) - DegreeToRadian(0.8f);
	}

	for(int j=0; j<MAX_HUMAN; j++){
		if( HumanIndex[j].GetEnableFlag() == false ){
			//初期化する
			HumanIndex[j].SetPosData(px, py, pz, rx);
			HumanIndex[j].SetParamData(infodata.p2, data.id, data.p4, infodata.p3, true);
			if( GetHumanFlag == 0 ){
				int id = Resource->GetHumanTexture(infodata.p2);
				if( id == -1 ){
					//テクスチャが取得できなければ、リフレッシュフラグを有効にして毎フレーム取得
					Human_FrameTextureRefresh[j] = true;
				}

				HumanIndex[j].SetTexture(id);
				HumanIndex[j].SetModel(upmodel[ HumanParam.model ], armmodel, legmodel, walkmodel, runmodel);
			}
			else{
#ifdef ENABLE_BUG_HUMANWEAPON
				HumanIndex[j].SetTexture(d3dg->GetMapTextureID(0));
				HumanIndex[j].SetModel(upmodel[0], armmodel, legmodel, walkmodel, runmodel);
#endif
			}
			HumanIndex[j].SetEnableFlag(true);
			Humanindexid = j;
			break;
		}
	}

	//初期化できなかったらエラーとして返す
	if( Humanindexid == -1 ){
		return -1;
	}

	//人の設定データが正しく読めていれば～
	if( GetHumanFlag == 0 ){
		//武器Bを仮想武器として追加
		Weaponindexid = AddVisualWeaponIndex(HumanParam.Weapon[0], true);
		if( Weaponindexid != -1 ){
			//成功すれば配列に記録
			Weapon[0] = &WeaponIndex[Weaponindexid];
		}
		if( data.p1 == POINT_P1TYPE_HUMAN ){
			//武器Aの仮想武器として追加
			Weaponindexid = AddVisualWeaponIndex(HumanParam.Weapon[1], true);
			if( Weaponindexid != -1 ){
				//成功すれば配列に記録
				Weapon[1] = &WeaponIndex[Weaponindexid];
			}
		}

		//人に持たせる
		HumanIndex[Humanindexid].SetWeapon(Weapon, lnbslist, nbslist);
	}

	//発砲フラグを初期化
	Human_ShotFlag[Humanindexid] = false;
	Human_YakkyouCnt[Humanindexid] = -1;

	//プレイヤーならば、番号を記録
	if( ( (data.p1 == POINT_P1TYPE_HUMAN)||(data.p1 == POINT_P1TYPE_HUMAN2) )&&(data.p4 == 0) ){
		Player_HumanID = Humanindexid;
	}

	return Humanindexid;
}

//! @brief 人追加（ゲーム中用）
//! @param px X座標
//! @param py Y座標
//! @param pz Z座標
//! @param rx X軸向き
//! @param paramID 種類番号
//! @param TeamID チーム番号
//! @param WeaponID 武器種類番号の配列（要素数：TOTAL_HAVEWEAPON）
//! @param param4 第4パラメーター
//! @param RandomFlag ランダム補正を有効にする
//! @return 成功：データ番号（0以上）　失敗：-1
//! @attention 無効な人の種類番号が指定された場合は 通称：謎人間 が登場します。テクスチャはマップテクスチャ0番が使用され、HPは 0 が指定（＝即死）されます。
int ObjectManager::AddHumanIndex(float px, float py, float pz, float rx, int paramID, int TeamID, int WeaponID[], signed short int param4, bool RandomFlag)
{
	int GetHumanFlag;
	HumanParameter HumanParam;
	GetHumanFlag = GameParamInfo->GetHuman(paramID, &HumanParam);
	int Humanindexid = -1;
	int Weaponindexid = -1;
	class weapon *Weapon[TOTAL_HAVEWEAPON];
	int lnbslist[TOTAL_HAVEWEAPON];
	int nbslist[TOTAL_HAVEWEAPON];
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		Weapon[i] = NULL;
		lnbslist[i] = 0;
		nbslist[i] = 0;
	}

#ifndef ENABLE_BUG_HUMANWEAPON
	if( GetHumanFlag != 0 ){
		return -1;
	}
#endif

	//人のモデル番号を取得
	int upmodel[TOTAL_UPMODE];
	int armmodel[TOTAL_ARMMODE];
	int legmodel;
	int walkmodel[TOTAL_WALKMODE];
	int runmodel[TOTAL_RUNMODE];
	Resource->GetHumanModel(upmodel, armmodel, &legmodel, walkmodel, runmodel);

	if( RandomFlag == true ){
		px += 0.001f*GetRand(100*2) - 0.1f;
		pz += 0.001f*GetRand(100*2) - 0.1f;
		rx += DegreeToRadian(0.01f)*GetRand(80*2) - DegreeToRadian(0.8f);
	}

	for(int i=0; i<MAX_HUMAN; i++){
		if( (ForceHumanSlot >= 0)&&(i != ForceHumanSlot) ){ continue; }	//[XOPS-ONLINE]
		if( HumanIndex[i].GetEnableFlag() == false ){
			//初期化する
			HumanIndex[i].SetPosData(px, py, pz, rx);
			HumanIndex[i].SetParamData(paramID, -1, param4, TeamID, true);
			if( GetHumanFlag == 0 ){
				int id = Resource->GetHumanTexture(paramID);
				if( id == -1 ){
					//テクスチャが取得できなければ、リフレッシュフラグを有効にして毎フレーム取得
					Human_FrameTextureRefresh[i] = true;
				}

				HumanIndex[i].SetTexture(id);
				HumanIndex[i].SetModel(upmodel[ HumanParam.model ], armmodel, legmodel, walkmodel, runmodel);
			}
			else{
#ifdef ENABLE_BUG_HUMANWEAPON
				HumanIndex[i].SetTexture(d3dg->GetMapTextureID(0));
				HumanIndex[i].SetModel(upmodel[0], armmodel, legmodel, walkmodel, runmodel);
#endif
			}
			HumanIndex[i].SetEnableFlag(true);
			Humanindexid = i;
			break;
		}
	}

	//初期化できなかったらエラーとして返す
	if( Humanindexid == -1 ){
		return -1;
	}

	if( GetHumanFlag == 0 ){
		//仮想武器を追加
		for(int i=0; i<TOTAL_HAVEWEAPON; i++){
			Weaponindexid = AddVisualWeaponIndex(WeaponID[i], true);
			if( Weaponindexid != -1 ){
				//成功すれば配列に記録
				Weapon[i] = &WeaponIndex[Weaponindexid];
			}
		}

		//人に持たせる
		HumanIndex[Humanindexid].SetWeapon(Weapon, lnbslist, nbslist);
	}

	//ログ関係の処理
	int player_teamid;
	HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &player_teamid);
	ObjectLog->AddHuman(Humanindexid, TeamID, player_teamid);

	return Humanindexid;
}

//! @brief 武器追加
//! @param data 武器のポイントデータ　（pointdata構造体）
//! @return 成功：データ番号（0以上）　失敗：-1
int ObjectManager::AddWeaponIndex(pointdata data)
{
	//武器番号と弾数を仮登録
	int WeaponID = data.p2;
	int nbs = PointParamToInt(data.p3);

	//ランダムな武器ならば
	if( data.p1 == POINT_P1TYPE_RAND_WEAPON ){
		WeaponParameter WeaponParam;

		if( GetRand(2) == 0 ){
			if( GameParamInfo->GetWeapon(data.p2, &WeaponParam) == 1 ){ return -1; }
			nbs = (int)WeaponParam.nbsmax * TOTAL_WEAPON_AUTOBULLET;		//弾数上書き
		}
		else{
			if( GameParamInfo->GetWeapon(data.p3, &WeaponParam) == 1 ){ return -1; }
			WeaponID = data.p3;						//武器番号上書き
			nbs = (int)WeaponParam.nbsmax * TOTAL_WEAPON_AUTOBULLET;		//弾数上書き
		}
	}

	//NONEならば失敗
	if( WeaponID == ID_WEAPON_NONE ){ return -1; }

	//モデルとテクスチャを取得
	int model, texture;
	if( Resource->GetWeaponModelTexture(WeaponID, &model, &texture) == 1 ){
		return -1;
	}

	//設定値を取得
	WeaponParameter WeaponParam;
	if( GameParamInfo->GetWeapon(WeaponID, &WeaponParam) == 1 ){ return -1; }

	for(int i=0; i<MAX_WEAPON; i++){
		if( WeaponIndex[i].GetEnableFlag() == false ){
			//初期化する
			WeaponIndex[i].SetPosData(data.x, data.y, data.z, data.r);
			WeaponIndex[i].SetParamData(WeaponID, 0, nbs, true);
			WeaponIndex[i].SetModel(model, WeaponParam.size);
			WeaponIndex[i].SetTexture(texture);
			WeaponIndex[i].SetEnableFlag(true);
			WeaponIndex[i].RunReload();
			return i;
		}
	}
	return -1;
}

//! @brief 仮想武器追加
//! @param WeaponID 武器の種類番号
//! @param loadbullet 弾をロードする
//! @return 成功：データ番号（0以上）　失敗：-1
//! @attention 人が最初から所持している武器を、武器データ（オブジェクト）として追加処理するための関数です。
//! @attention 武器単体として配置する場合は、AddWeaponIndex()関数を使用してください。
int ObjectManager::AddVisualWeaponIndex(int WeaponID, bool loadbullet)
{
	//NONEならば失敗
	if( WeaponID == ID_WEAPON_NONE ){ return -1; }

	//情報を取得
	WeaponParameter WeaponParam;
	if( GameParamInfo->GetWeapon(WeaponID, &WeaponParam) == 1 ){ return -1; }

	//モデルとテクスチャを取得
	int model, texture;
	if( Resource->GetWeaponModelTexture(WeaponID, &model, &texture) == 1 ){
		return -1;
	}

	for(int i=0; i<MAX_WEAPON; i++){
		if( WeaponIndex[i].GetEnableFlag() == false ){
			//初期化
			WeaponIndex[i].SetPosData(0.0f, 0.0f, 0.0f, 0.0f);
			if( loadbullet == false ){
				WeaponIndex[i].SetParamData(WeaponID, 0, 0, true);
			}
			else{
				//[OpenXOPS-web] thrown items carry no reserve
				WeaponIndex[i].SetParamData(WeaponID, WeaponParam.nbsmax, WeaponParam.throwaway ? WeaponParam.nbsmax : WeaponParam.nbsmax * TOTAL_WEAPON_AUTOBULLET, true);
			}
			WeaponIndex[i].SetModel(model, WeaponParam.size);
			WeaponIndex[i].SetTexture(texture);
			WeaponIndex[i].SetEnableFlag(true);
			return i;
		}
	}
	return -1;
}

//! @brief 小物追加
//! @param data 小物のポイントデータ　（pointdata構造体）
//! @return 成功：データ番号（0以上）　失敗：-1
int ObjectManager::AddSmallObjectIndex(pointdata data)
{
	//モデルとテクスチャを取得
	int model, texture;
	if( Resource->GetSmallObjectModelTexture(data.p2, &model, &texture) == 1 ){
		return -1;
	}

	for(int i=0; i<MAX_SMALLOBJECT; i++){
		if( SmallObjectIndex[i].GetEnableFlag() == false ){
			//初期化
			SmallObjectIndex[i].SetPosData(data.x, data.y, data.z, data.r);
			SmallObjectIndex[i].SetParamData(data.p2, data.p4, true);
			SmallObjectIndex[i].SetModel(model, SMALLOBJECT_SCALE);
			SmallObjectIndex[i].SetTexture(texture);
			SmallObjectIndex[i].SetEnableFlag(true);

			//位置修正フラグが有効ならば、マップと判定
			if( data.p3 !=0 ){
				SmallObjectIndex[i].CollisionMap(CollD);
			}
			return i;
		}
	}
	return -1;
}

//! @brief 小物追加（ゲーム中用）
//! @param px X座標
//! @param py Y座標
//! @param pz Z座標
//! @param rx X軸向き
//! @param paramID 種類番号
//! @param MapColl 位置修正フラグ
//! @return 成功：データ番号（0以上）　失敗：-1
int ObjectManager::AddSmallObjectIndex(float px, float py, float pz, float rx, int paramID, bool MapColl)
{
	//モデルとテクスチャを取得
	int model, texture;
	if( Resource->GetSmallObjectModelTexture(paramID, &model, &texture) == 1 ){
		return -1;
	}

	for(int i=0; i<MAX_SMALLOBJECT; i++){
		if( SmallObjectIndex[i].GetEnableFlag() == false ){
			//初期化
			SmallObjectIndex[i].SetPosData(px, py, pz, rx);
			SmallObjectIndex[i].SetParamData(paramID, 0, true);
			SmallObjectIndex[i].SetModel(model, SMALLOBJECT_SCALE);
			SmallObjectIndex[i].SetTexture(texture);
			SmallObjectIndex[i].SetEnableFlag(true);

			//位置修正フラグが有効ならば、マップと判定
			if( MapColl == true ){
				SmallObjectIndex[i].CollisionMap(CollD);
			}

			//ログ関係の処理
			ObjectLog->AddSmallObject(i);

			return i;
		}
	}
	return -1;
}

//! @brief エフェクト追加
//! @param pos_x X座標
//! @param pos_y Y座標
//! @param pos_z Z座標
//! @param move_x X軸移動量
//! @param move_y Y軸移動量
//! @param move_z Z軸移動量
//! @param addmove_y Y軸移動量の加算量
//! @param rotation 回転角度
//! @param addrotation 回転角度の加算量
//! @param size 描画倍率
//! @param addsize 描画倍率の加算量
//! @param texture テクスチャのデータ番号
//! @param brightness 明るさ
//! @param addbrightness 明るさの加算量
//! @param alpha 透明度
//! @param addalpha 透明度の加算量
//! @param settype エフェクトの種類　（Effect_Type を組み合せる）
//! @param count 描画フレーム数
//! @return 成功：データ番号（0以上）　失敗：-1
int ObjectManager::AddEffect(float pos_x, float pos_y, float pos_z, float move_x, float move_y, float move_z, float addmove_y, float rotation, float addrotation, float size, float addsize, int texture, int brightness, int addbrightness, float alpha, float addalpha, int settype, int count)
{
	for(int i=0; i<MAX_EFFECT; i++){
		if( EffectIndex[i].GetEnableFlag() == false ){
			EffectIndex[i].SetPosData(pos_x, pos_y, pos_z, 0.0f);
			EffectIndex[i].SetParamData(move_x, move_y, move_z, addmove_y, rotation, addrotation, size, addsize, count, settype);
			EffectIndex[i].SetTexture(texture, brightness, addbrightness, alpha, addalpha);
			EffectIndex[i].SetEnableFlag(true);
			return i;
		}
	}

	return -1;
}

//! @brief （ビルボードでない）エフェクト追加
//! @param id ブロック番号
//! @param face 面番号
//! @param pos_x X座標
//! @param pos_y Y座標
//! @param pos_z Z座標
//! @param size 描画倍率
//! @param rotation 回転角度
//! @param texture テクスチャのデータ番号
//! @param brightness 明るさ
//! @param addbrightness 明るさの加算量
//! @param alpha 透明度
//! @param addalpha 透明度の加算量
//! @param count 描画フレーム数
//! @return 成功：データ番号（0以上）　失敗：-1
int ObjectManager::AddMapEffect(int id, int face, float pos_x, float pos_y, float pos_z, float size, float rotation, int texture, int brightness, int addbrightness, float alpha, float addalpha, int count)
{
	blockdata data;
	float vx, vy, vz;
	float rx, ry;

	//ブロックの面の情報を取得
	BlockData->Getdata(&data, id);
	vx = data.material[face].vx;
	vy = data.material[face].vy;
	vz = data.material[face].vz;

	//角度を求める
	rx = atan2f(vz, vx)*-1 + (float)M_PI;
	ry = atan2f(vy, sqrtf(vx*vx + vz*vz))*-1;

	//エフェクト作成
	for(int i=0; i<MAX_EFFECT; i++){
		if( EffectIndex[i].GetEnableFlag() == false ){
			EffectIndex[i].SetPosData(pos_x, pos_y, pos_z, 0.0f);
			EffectIndex[i].SetParamData(0.0f, 0.0f, 0.0f, 0.0f, rotation, 0.0f, size, 0.0f, count, EFFECT_NOBILLBOARD);
			EffectIndex[i].SetRxRy(rx, ry);
			EffectIndex[i].SetTexture(texture, brightness, addbrightness, alpha, addalpha);
			EffectIndex[i].SetEnableFlag(true);
			return i;
		}
	}

	return -1;
}

//! @brief 出血させる
//! @param x X座標
//! @param y Y座標
//! @param z Z座標
//! @param damage ダメージ
//! @param flowing 流血の有無
void ObjectManager::SetHumanBlood(float x, float y, float z, int damage, bool flowing)
{
	int addtype;

	if( flowing == false ){
		addtype = EFFECT_NORMAL;
	}
	else{
		addtype = EFFECT_COLLIDEMAP;
	}

	if( GameConfig.GetBloodFlag() == true ){
		AddEffect(x, y, z, 0.0f, 0.0f, 0.0f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, 10.0f, 0.2f, Resource->GetEffectBloodTexture(), 10, 0, 0.5f, -0.04f, addtype, (int)(GAMEFPS * 0.4f));
		if( flowing == true ){
			for(int i=0; i<(damage/10); i++){
				float rx = DegreeToRadian(10)*GetRand(36);
				AddEffect(x + cosf(rx)*1.0f, y + (float)(GetRand(20)-10)/10, z + sinf(rx)*1.0f, cosf(rx)*0.5f, GetRand(5)*0.1f + 0.5f, sinf(rx)*0.5f, -0.09f, DegreeToRadian(10)*GetRand(36), 0.0f, GetRand(40)*0.1f+0.1f, 0.1f, Resource->GetEffectBloodTexture(), 10, 0, 1.0f, 0.0f, addtype, (int)(GAMEFPS * 0.5f));
			}
		}
	}
}

//! @brief 人同士の当たり判定
//! @param in_humanA 対象の人オブジェクトA
//! @param in_humanB 対象の人オブジェクトB
//! @return 当たっている：true　当たっていない：false
//! @warning in_humanAとin_humanBで区別はありません。
//! @warning in_humanAとin_humanBでの組み合せは、1フレーム間に1度だけ実行してください。
//! @attention 両クラスは自動的にAddPosOrder()を用いて、お互いを押し合います。
bool ObjectManager::CollideHuman(human *in_humanA, human *in_humanB)
{
	if( in_humanA == NULL ){ return false; }
	if( in_humanB == NULL ){ return false; }

	float h1_x, h1_y, h1_z;
	float h2_x, h2_y, h2_z;
	float angle, length;

	//初期化されていないか、死亡して入れば判定しない
	if( in_humanA->GetEnableFlag() == false ){ return false; }
	if( in_humanB->GetEnableFlag() == false ){ return false; }
	if( in_humanA->GetHP() <= 0 ){ return false; }
	if( in_humanB->GetHP() <= 0 ){ return false; }

	//お互いの座標を取得
	in_humanA->GetPosData(&h1_x, &h1_y, &h1_z, NULL);
	in_humanB->GetPosData(&h2_x, &h2_y, &h2_z, NULL);

	//円柱の当たり判定
	if( CollideCylinder(h1_x, h1_y, h1_z, HUMAN_HUMANCOLLISION_R, HUMAN_HEIGHT-0.5f, h2_x, h2_y, h2_z, HUMAN_HUMANCOLLISION_R, HUMAN_HEIGHT-0.5f, &angle, &length) == true ){
		//めり込んだ分だけ押し出す
		in_humanA->AddPosOrder(angle, 0.0f, length/2);
		in_humanB->AddPosOrder(angle + (float)M_PI, 0.0f, length/2);
		return true;
	}

	return false;
}

//! @brief 弾の当たり判定と処理
//! @param in_bullet 対象の弾オブジェクト
//! @return 当たった：true　当たっていない：false
//! @attention 判定を行う対象は「マップ」「人（頭・上半身・下半身）」「小物」です。
//! @attention 判定に限らず、ダメージ計算や効果音再生まで一貫して行います。
bool ObjectManager::CollideBullet(bullet *in_bullet)
{
	if( in_bullet == NULL ){ return false; }

	//使用されていない弾丸ならば、処理せずに返す。
	if( in_bullet->GetEnableFlag() == false ){ return false; }

	int objectID;
	float bx, by, bz;
	float brx, bry;
	int attacks;
	int penetration;
	float speed;
	int teamid;
	int humanid;
	float ontargetcnt;
	float vx, vy, vz;
	bool CollideFlag = false;
	int mapflag = 0;		//0：ブロックなし、1：ブロックあり未判定、2：ブロックあり判定済み

	float Dist;

	//弾オブジェクトのデータ番号を取得
	objectID = GetBulletObjectID(in_bullet);

	//弾丸の座標を取得し、ベクトルを算出。
	in_bullet->GetPosData(&bx, &by, &bz, &brx, &bry);
	in_bullet->GetParamData(&attacks, &penetration, &speed, &teamid, &humanid, &ontargetcnt);
	vx = cosf(brx)*cosf(bry);
	vy = sinf(bry);
	vz = sinf(brx)*cosf(bry);

	//マップとの当たり判定（弾道上にブロックがあるか）
	if( CollD->CheckALLBlockIntersectRay(bx, by, bz, vx, vy, vz, NULL, NULL, &Dist, speed) == true ){
		mapflag = 1;
	}

	// BULLET_SPEEDSCALE定数 の分解能を算出
	int maxcnt = (int)roundf(speed / BULLET_SPEEDSCALE);

	// BULLET_SPEEDSCALE定数 の分解能で当たり判定
	for(int cnt=0; cnt<maxcnt; cnt++){
		//貫通力が残っていなければ弾は無効にする（判定 1回目）
		if( penetration < 0 ){
			in_bullet->SetEnableFlag(false);
			mapflag = 0;
			break;
		}

		float bvx, bvy, bvz;
		bvx = bx + vx * BULLET_SPEEDSCALE * (cnt+1);
		bvy = by + vy * BULLET_SPEEDSCALE * (cnt+1);
		bvz = bz + vz * BULLET_SPEEDSCALE * (cnt+1);

		//人との当たり判定
		for(int i=0; i<MAX_HUMAN; i++){
			//その人自身が発砲した弾なら処理しない。
			if( i == humanid ){ continue; }

			//使用されていないか、死体ならば処理しない。
			if( HumanIndex[i].GetEnableFlag() == false ){ continue; }
			//if( HumanIndex[i].GetDeadFlag() == true ){ continue; }
			if( HumanIndex[i].GetHP() <= 0 ){ continue; }

			//既に当たった人なら、処理しない。
			if( BulletObj_HumanIndex[objectID].GetIndexFlag(i) == true ){ continue; }

			//座標を取得
			float ox, oy, oz;
			int h_teamid;
			HumanIndex[i].GetPosData(&ox, &oy, &oz, NULL);
			HumanIndex[i].GetParamData(NULL, NULL, NULL, &h_teamid);

			if( FriendlyFire[i] == false ){
				//同じチーム番号（味方）なら処理しない
				if( h_teamid == teamid ){ continue; }
			}

			//頭の当たり判定
			if( CollideCylinderInside(ox, oy + HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H, oz, HUMAN_BULLETCOLLISION_HEAD_R, HUMAN_BULLETCOLLISION_HEAD_H, bvx, bvy, bvz) == true ){
				//人に当たった処理
				HitBulletHuman(i, 0, bvx, bvy, bvz, brx, attacks, humanid, ontargetcnt);

				//対人判定用リスト設定（一度初期化し、最後に当たった人だけ設定）
				BulletObj_HumanIndex[objectID].Init();
				BulletObj_HumanIndex[objectID].SetIndexFlag(i);

				//攻撃力と貫通力を計算
				attacks = (int)((float)attacks * 0.5f);
				penetration -= 1;

				CollideFlag = true;
			}

			//上半身と当たり判定
			if( CollideCylinderInside(ox, oy + HUMAN_BULLETCOLLISION_LEG_H, oz, HUMAN_BULLETCOLLISION_UP_R, HUMAN_BULLETCOLLISION_UP_H, bvx, bvy, bvz) == true ){
				//人に当たった処理
				HitBulletHuman(i, 1, bvx, bvy, bvz, brx, attacks, humanid, ontargetcnt);

				//対人判定用リスト設定（一度初期化し、最後に当たった人だけ設定）
				BulletObj_HumanIndex[objectID].Init();
				BulletObj_HumanIndex[objectID].SetIndexFlag(i);

				//攻撃力と貫通力を計算
				attacks = (int)((float)attacks * 0.6f);
				penetration -= 1;

				CollideFlag = true;
			}

			//足と当たり判定
			if( CollideCylinderInside(ox, oy, oz, HUMAN_BULLETCOLLISION_LEG_R, HUMAN_BULLETCOLLISION_LEG_H, bvx, bvy, bvz) == true ){
				//人に当たった処理
				HitBulletHuman(i, 2, bvx, bvy, bvz, brx, attacks, humanid, ontargetcnt);

				//対人判定用リスト設定（一度初期化し、最後に当たった人だけ設定）
				BulletObj_HumanIndex[objectID].Init();
				BulletObj_HumanIndex[objectID].SetIndexFlag(i);

				//攻撃力と貫通力を計算
				attacks = (int)((float)attacks * 0.7f);
				penetration -= 1;

				CollideFlag = true;
			}
		}

		//貫通力が残っていなければ弾は無効にする（判定 2回目）
		if( penetration < 0 ){
			in_bullet->SetEnableFlag(false);
			mapflag = 0;
			break;
		}

		//小物との当たり判定
		for(int i=0; i<MAX_SMALLOBJECT; i++){
			//使用されていなければ処理しない
			if( SmallObjectIndex[i].GetEnableFlag() == false ){ continue; }

			//座標を取得
			float ox, oy, oz;
			int id;
			float decide;
			SmallObjectIndex[i].GetPosData(&ox, &oy, &oz, NULL);
			SmallObjectIndex[i].GetParamData(&id, NULL);

			//[OpenXOPS-web] Opt-in addon cover follows the visible body, not the
			//legacy ground-level sphere. Disappearing/dead cover no longer blocks.
			const float* hull = MIFdata ? MIFdata->GetAddSmallobjectCover(id - TOTAL_PARAMETERINFO_SMALLOBJECT) : NULL;
			if( hull != NULL ){
				if( SmallObjectIndex[i].GetHP() <= 0 ){ continue; }
				float yaw; SmallObjectIndex[i].GetPosData(NULL,NULL,NULL,&yaw);
				float dx=bvx-ox, dz=bvz-oz;
				float lx=cosf(yaw)*dx-sinf(yaw)*dz;
				float lz=sinf(yaw)*dx+cosf(yaw)*dz;
				if( fabsf(lx)<=hull[0] && bvy>=oy+hull[1] && bvy<=oy+hull[2] && fabsf(lz)<=hull[3] ){
					HitBulletSmallObject(i,bvx,bvy,bvz,(int)floorf(attacks*0.25f),teamid);
					in_bullet->SetEnableFlag(false);
					return true;
				}
				continue;
			}

			//当たり判定の大きさを取得
			if( (TOTAL_PARAMETERINFO_SMALLOBJECT <= id)&&(id <= (TOTAL_PARAMETERINFO_SMALLOBJECT + MAX_ADDSMALLOBJECT -1)) ){
				decide = (float)MIFdata->GetAddSmallobjectDecide(id - TOTAL_PARAMETERINFO_SMALLOBJECT)*SMALLOBJECT_COLLISIONSCALE;
			}
			else{
				SmallObjectParameter Param;
				GameParamInfo->GetSmallObject(id, &Param);
				decide = (float)Param.decide*SMALLOBJECT_COLLISIONSCALE;
			}

			//当たり判定
			if( CollideSphereInside(ox, oy, oz, decide, bvx, bvy, bvz) == true ){
				//小物に当たった処理
				int attack2 = (int)(floorf( (float)attacks * 0.25f ));
				HitBulletSmallObject(i, bvx, bvy, bvz, attack2, teamid);

				//攻撃力を計算
				attacks = (int)(floorf( (float)attacks * 0.7f ));

				CollideFlag = true;
			}
		}

		if( mapflag > 0 ){
			//マップとの当たり判定
			if( CollD->CheckALLBlockInside(bvx, bvy, bvz) == true ){
				//弾がマップに当たった処理
				HitBulletMap(bx + vx*(Dist-1.0f), by + vy*(Dist-1.0f), bz + vz*(Dist-1.0f), teamid);

				//攻撃力と貫通力を計算
				penetration -= 1;
				if( penetration >= 0 ){
					attacks = (int)((float)attacks * 0.6f);
				}

				mapflag = 2;
				CollideFlag = true;
			}
		}
	}

	//弾道上にブロックがあるにも関わらず、ブロックと当たり判定を行っていないならば
	//　（＝薄いブロックがある）
	if( mapflag == 1 ){
		//弾がマップに当たった処理
		HitBulletMap(bx + vx*(Dist-1.0f), by + vy*(Dist-1.0f), bz + vz*(Dist-1.0f), teamid);

		//攻撃力を計算
		if( penetration > 0 ){
			attacks = (int)((float)attacks * 0.75f);
		}
		else{
			attacks = (int)((float)attacks * 0.55f);
		}

		CollideFlag = true;
	}

	//設定を適用（特に攻撃力・貫通力）
	in_bullet->SetParamData(attacks, penetration, speed, teamid, humanid, ontargetcnt, false);

	return CollideFlag;
}

//! @brief 弾がマップに当たった処理
//! @param x 着弾X座標
//! @param y 着弾Y座標
//! @param z 着弾Z座標
//! @param teamID 発射元のチーム番号
void ObjectManager::HitBulletMap(float x, float y, float z, int teamID)
{
	//エフェクト（煙）を描画
	AddEffect(x, y, z, 0.0f, 0.05f, 0.0f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, 3.0f, 0.24f, Resource->GetEffectSmokeTexture(), 20, 0, 0.63f, -0.03f, EFFECT_NORMAL, 20);

	//効果音を再生
	GameSound->HitMap(x, y, z, teamID);
}

//! @brief 弾が人に当たった処理
//! @param HitHuman_id 人の番号
//! @param Hit_id 被弾箇所（頭：0　胴体：1　足：2）
//! @param x 被弾X座標
//! @param y 被弾Y座標
//! @param z 被弾Z座標
//! @param brx 水平角度
//! @param attacks 攻撃力
//! @param Shothuman_id 発射した人の番号
//! @param ontargetcnt 命中時のカウント数
void ObjectManager::HitBulletHuman(int HitHuman_id, int Hit_id, float x, float y, float z, float brx, int attacks, int Shothuman_id, float ontargetcnt)
{
	int Shothuman_TeamID;
	int HitHuman_TeamID;
	int hp_old;
	int damage = 0;
	int paramid;
	HumanParameter Paraminfo;
	bool NotRobot;

	//使用されていないか、死体ならば処理しない。
	if( HumanIndex[HitHuman_id].GetEnableFlag() == false ){ return; }
	if( HumanIndex[HitHuman_id].GetDeadFlag() == true ){ return; }

	//発射元と対象人物のチーム番号取得
	HumanIndex[Shothuman_id].GetParamData(NULL, NULL, NULL, &Shothuman_TeamID);
	HumanIndex[HitHuman_id].GetParamData(NULL, NULL, NULL, &HitHuman_TeamID);

	//ダメージ計算前のHP取得
	hp_old = HumanIndex[HitHuman_id].GetHP();

	//人にダメージと衝撃を与える
	if( NetPuppet == false ){	//[XOPS-ONLINE] the host applies damage; clients only show effects
	if( Hit_id == 0 ){ HumanIndex[HitHuman_id].HitBulletHead(attacks); }
	if( Hit_id == 1 ){ HumanIndex[HitHuman_id].HitBulletUp(attacks); }
	if( Hit_id == 2 ){ HumanIndex[HitHuman_id].HitBulletLeg(attacks); }
	}
	HumanIndex[HitHuman_id].AddPosOrder(brx, 0.0f, 1.0f);

#ifdef ENABLE_BUG_TEAMID
	bool flag = true;

	//チーム番号が負数、かつチーム番号が大きいなら、フラグ無効
	if( (HitHuman_TeamID < 0)&&(Shothuman_TeamID < 0) ){
		if( HitHuman_TeamID < Shothuman_TeamID ){
			flag = false;
		}
	}

	if( flag == true ){
		HumanIndex[HitHuman_id].SetHitFlag(brx*-1 - (float)M_PI/2);
	}
#else
	HumanIndex[HitHuman_id].SetHitFlag(brx*-1 - (float)M_PI/2);
#endif

	//ロボットかどうか判定
	HumanIndex[HitHuman_id].GetParamData(&paramid, NULL, NULL, NULL);
	GameParamInfo->GetHuman(paramid, &Paraminfo);
	if( Paraminfo.type == 1 ){
		NotRobot = false;
	}
	else{
		NotRobot = true;
	}

	//エフェクト（血）を描画
	if( Hit_id == 0 ){ damage = (int)((float)attacks * HUMAN_DAMAGE_HEAD); }
	if( Hit_id == 1 ){ damage = (int)((float)attacks * HUMAN_DAMAGE_UP); }
	if( Hit_id == 2 ){ damage = (int)((float)attacks * HUMAN_DAMAGE_LEG); }
	SetHumanBlood(x, y, z, damage, NotRobot);

	//効果音を再生
	GameSound->HitHuman(x, y, z, Hit_id, Shothuman_TeamID);

	if( (0 <= Shothuman_id)&&(Shothuman_id < MAX_HUMAN) ){
		//弾を発射した人の成果に加算
		Human_ontarget[Shothuman_id] += ontargetcnt;
		if( Hit_id == 0 ){ Human_headshot[Shothuman_id] += 1; }
		if( (hp_old > 0)&&(HumanIndex[HitHuman_id].GetHP() <= 0) ){
			//ダメージ計算前に hp>0 でかつ、計算後に hp <= 0 なら、今回死亡した。
			Human_kill[Shothuman_id] += 1;
		}
	}

	//ログ関係の処理
	if( (hp_old > 0)&&(HumanIndex[HitHuman_id].GetHP() <= 0) ){
		//ダメージ計算前に hp>0 でかつ、計算後に hp <= 0 なら、今回死亡した。

		int player_teamid;
		HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &player_teamid);

		ObjectLog->DiedHuman(Shothuman_id, HitHuman_id, Shothuman_TeamID, HitHuman_TeamID, player_teamid);
	}
}

//! @brief 弾が小物に当たった処理
//! @param HitSmallObject_id 小物の番号
//! @param x 着弾X座標
//! @param y 着弾Y座標
//! @param z 着弾Z座標
//! @param attacks 攻撃力
//! @param teamID 発射元のチーム番号
void ObjectManager::HitBulletSmallObject(int HitSmallObject_id, float x, float y, float z, int attacks, int teamID)
{
	int hp;

	//使用されていなければ処理しない。
	if( SmallObjectIndex[HitSmallObject_id].GetEnableFlag() == false ){ return; }

	//体力がなければ処理しない
	hp = SmallObjectIndex[HitSmallObject_id].GetHP();
	if( hp <= 0 ){ return; }

	//小物にダメージを与える
	SmallObjectIndex[HitSmallObject_id].HitBullet(attacks);

	//エフェクト（煙）を描画
	//AddEffect(x, y, z, 0.01f*(GetRand(101)-50), 0.01f*(GetRand(101)-50), 0.01f*(GetRand(101)-50), 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, 1.0f, 0.1f, Resource->GetEffectSmokeTexture(), 10, 0, 0.625f, -0.047f, EFFECT_NORMAL, 255);
	//AddEffect(x, y, z, 0.01f*(GetRand(101)-50), 0.01f*(GetRand(101)-50), 0.01f*(GetRand(101)-50), 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, 1.0f, 0.1f, Resource->GetEffectSmokeTexture(), 10, 0, 0.625f, -0.047f, EFFECT_NORMAL, 255);
	AddEffect(x, y, z, 0.0f, 0.0f, 0.0f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, 1.0f, 0.1f, Resource->GetEffectSmokeTexture(), 10, 0, 0.625f, -0.047f, EFFECT_NORMAL, 255);
	AddEffect(x, y, z, 0.0f, 0.01f*(GetRand(10)-5), 0.0f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, 1.0f, 0.1f, Resource->GetEffectSmokeTexture(), 10, 0, 0.625f, -0.047f, EFFECT_NORMAL, 255);

	//効果音を再生
	int id;
	SmallObjectIndex[HitSmallObject_id].GetParamData(&id, NULL);
	GameSound->HitSmallObject(x, y, z, id, teamID);

	//ログ関係の処理
	hp = SmallObjectIndex[HitSmallObject_id].GetHP();
	if( hp <= 0 ){
		//ダメージ計算前に hp>0 でかつ、計算後に hp <= 0 なら、今回破壊された。
		ObjectLog->BreakSmallObject(HitSmallObject_id);
	}
}

//! @brief 手榴弾のダメージ判定と処理
//! @param in_grenade 対象の手榴弾オブジェクト
//! @return 当たった：true　当たっていない：false
//! @attention 判定を行う対象は「人」と「小物」です。
//! @attention ダメージ判定に限らず、ダメージ計算や効果音再生まで一貫して行います。
bool ObjectManager::GrenadeExplosion(grenade *in_grenade)
{
	if( in_grenade == NULL ){ return false; }

	bool returnflag = false;

	//座標を取得
	float gx, gy, gz;
	int teamid, humanid;
	in_grenade->GetPosData(&gx, &gy, &gz, NULL, NULL);
	in_grenade->GetParamData(NULL, &teamid, &humanid, NULL);

	//人に爆風の当たり判定
	for(int i=0; i<MAX_HUMAN; i++){
		//初期化されていないか、死体ならば処理しない。
		if( HumanIndex[i].GetEnableFlag() == false ){ continue; }
		if( HumanIndex[i].GetDeadFlag() == true ){ continue; }

		int HitHuman_TeamID;
		float hx, hy, hz;
		float x, y, z, r;

		//人の座標を取得し、距離を計算
		HumanIndex[i].GetPosData(&hx, &hy, &hz, NULL);
		x = hx - gx;
		y = hy - gy;
		z = hz - gz;
		r = sqrtf(x*x + y*y + z*z);

		//100.0より遠ければ計算しない
		if( r > MAX_DAMAGE_GRENADE_DISTANCE + HUMAN_HEIGHT ){ continue; }

		float dummy = 0.0f;
		int total_damage = 0;
		int damage;

		//足元に当たり判定
		y = hy + 2.0f - gy;
		r = sqrtf(x*x + y*y + z*z);
		//ブロックが遮っていなければ　（レイで当たり判定を行い、当たっていなければ）
		if( CollD->CheckALLBlockIntersectRay(gx, gy, gz, x/r, y/r, z/r, NULL, NULL, &dummy, r) == false ){
			//ダメージ量を計算
			damage = HUMAN_DAMAGE_GRENADE_LEG - (int)((float)HUMAN_DAMAGE_GRENADE_LEG/MAX_DAMAGE_GRENADE_DISTANCE * r);
			if( damage > 0 ){
				total_damage += damage;
			}
		}

		//頭に当たり判定
		y = hy + (HUMAN_HEIGHT-2.0f) - gy;
		r = sqrtf(x*x + y*y + z*z);
		//ブロックが遮っていなければ　（レイで当たり判定を行い、当たっていなければ）
		if( CollD->CheckALLBlockIntersectRay(gx, gy, gz, x/r, y/r, z/r, NULL, NULL, &dummy, r) == false ){
			//ダメージ量を計算
			damage = HUMAN_DAMAGE_GRENADE_HEAD - (int)((float)HUMAN_DAMAGE_GRENADE_HEAD/MAX_DAMAGE_GRENADE_DISTANCE * r);
			if( damage > 0 ){
				total_damage += damage;
			}
		}

		if( total_damage > 0 ){
			int hp_old;

			//ダメージ計算前のHP取得
			hp_old = HumanIndex[i].GetHP();

			//ダメージを反映
			if( NetPuppet == false ){ HumanIndex[i].HitGrenadeExplosion(total_damage); }	//[XOPS-ONLINE]

			HumanIndex[i].GetParamData(NULL, NULL, NULL, &HitHuman_TeamID);

			float y2;
			float arx, ary;

			if( (0 <= humanid)&&(humanid < MAX_HUMAN) ){
				//倒していれば、発射した人の成果に加算
				if( HumanIndex[i].GetHP() <= 0 ){ Human_kill[humanid] += 1; }
			}

			//エフェクト（血）を描画
			SetHumanBlood(hx, hy+15.0f, hz, total_damage, false);

			//人と手榴弾の距離を算出
			x = gx - hx;
			y = gy - (hy + 1.0f);
			z = gz - hz;

			//角度を求める
			arx = atan2f(z, x);

			if( sinf(atan2f(y, sqrtf(x*x + z*z))) < 0.0f ){		//上方向に飛ぶなら、角度を計算
				y2 = gy - (hy + HUMAN_HEIGHT);
				ary = atan2f(y2, sqrtf(x*x + z*z)) + (float)M_PI;
			}
			else{		//下方向に飛ぶなら、垂直角度は無効。（爆風で地面にめり込むのを防止）
				ary = 0.0f;
			}

#ifdef ENABLE_BUG_TEAMID
			bool flag = true;

			//チーム番号が負数、かつチーム番号が大きいなら、フラグ無効
			if( (HitHuman_TeamID < 0)&&(teamid < 0) ){
				if( HitHuman_TeamID < teamid ){
					flag = false;
				}
			}

			if( flag == true ){
				HumanIndex[i].SetHitFlag(arx*-1 + (float)M_PI/2);
			}
#else
			HumanIndex[i].SetHitFlag(arx*-1 + (float)M_PI/2);
#endif

			//爆風による風圧
			HumanIndex[i].AddPosOrder(arx, ary, 2.2f/MAX_DAMAGE_GRENADE_DISTANCE * (MAX_DAMAGE_GRENADE_DISTANCE - sqrtf(x*x + y*y + z*z)));

			//ログ関係の処理
			if( (hp_old > 0)&&(HumanIndex[i].GetHP() <= 0) ){
				//ダメージ計算前に hp>0 でかつ、計算後に hp <= 0 なら、今回死亡した。

				int player_teamid;
				HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &player_teamid);

				ObjectLog->DiedHuman(humanid, i, teamid, HitHuman_TeamID, player_teamid);
			}

			returnflag = true;
		}
	}

	//小物に爆風の当たり判定
	for(int i=0; i<MAX_SMALLOBJECT; i++){
		//使用されていれば処理しない
		if( SmallObjectIndex[i].GetEnableFlag() == false ){ continue; }

		float sx, sy, sz;
		float x, y, z, r;

		//小物の座標を取得し、距離を計算
		SmallObjectIndex[i].GetPosData(&sx, &sy, &sz, NULL);
		x = sx - gx;
		y = sy - gy;
		z = sz - gz;
		r = sqrtf(x*x + y*y + z*z);

		//100.0より遠ければ計算しない
		if( r > MAX_DAMAGE_GRENADE_DISTANCE ){ continue; }

		float dummy = 0.0f;

		//ブロックが遮っていなければ　（レイで当たり判定を行い、当たっていなければ）
		if( CollD->CheckALLBlockIntersectRay(gx, gy, gz, x/r, y/r, z/r, NULL, NULL, &dummy, r) == false ){
			int id, damage;
			SmallObjectIndex[i].GetParamData(&id, NULL);

			//ダメージ量を計算し、反映
			damage = SMALLOBJECT_DAMAGE_GRENADE - (int)((float)SMALLOBJECT_DAMAGE_GRENADE/MAX_DAMAGE_GRENADE_DISTANCE * r);
			SmallObjectIndex[i].HitGrenadeExplosion(damage);

			//小物から効果音を発する
			GameSound->HitSmallObject(sx, sy, sz, id, teamid);

			//ログ関係の処理
			if( SmallObjectIndex[i].GetHP() <= 0 ){
				//ダメージ計算前に hp>0 でかつ、計算後に hp <= 0 なら、今回破壊された。
				ObjectLog->BreakSmallObject(i);
			}

			returnflag = true;
		}
	}

	//エフェクト（フラッシュ）の描画
	AddEffect(gx, gy, gz, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 40.0f, 0.0f, Resource->GetEffectMflashTexture(), 0, 0, 0.63f, 0.0f, EFFECT_NORMAL, 2);

	//エフェクト（煙）の描画
	AddEffect(gx, gy, gz, 0.0f, 0.1f, 0.0f, 0.0f,
					DegreeToRadian(10)*GetRand(36), 0.0f, 36.0f, 0.2f, Resource->GetEffectSmokeTexture(), 220, 0, 0.6f, -0.01f, EFFECT_NORMAL, 50);
	AddEffect(gx + ((float)GetRand(7)/10-0.3f), gy + ((float)GetRand(5)/10-0.2f), gz + ((float)GetRand(7)/10-0.3f), (float)GetRand(5)/10-0.2f, (float)GetRand(3)/10, (float)GetRand(5)/10-0.2f, 0.0f,
					DegreeToRadian(10)*GetRand(36), 0.0f, ((float)GetRand(11)+25.0f)*2, 0.3f, Resource->GetEffectSmokeTexture(), 0, 0, 0.47f, -0.004f, EFFECT_NORMAL, 120);
	AddEffect(gx + ((float)GetRand(13)/10-0.6f), gy + ((float)GetRand(11)/10-0.5f), gz + ((float)GetRand(13)/10-0.6f), (float)GetRand(7)/10-0.3f, (float)GetRand(7)/10-0.3f, (float)GetRand(7)/10-0.3f, 0.0f,
					DegreeToRadian(10)*GetRand(36), DegreeToRadian(1), ((float)GetRand(21)+20.0f)*2, 0.2f, Resource->GetEffectSmokeTexture(), 0, 0, 0.4f, -0.004f, EFFECT_NORMAL, 100);
	AddEffect(gx + ((float)GetRand(13)/10-0.6f), gy + ((float)GetRand(11)/10-0.5f), gz + ((float)GetRand(13)/10-0.6f), (float)GetRand(7)/10-0.3f, (float)GetRand(7)/10-0.3f, (float)GetRand(7)/10-0.3f, 0.0f,
					DegreeToRadian(10)*GetRand(36), DegreeToRadian(-1), ((float)GetRand(21)+20.0f)*2, 0.2f, Resource->GetEffectSmokeTexture(), 0, 0, 0.4f, -0.004f, EFFECT_NORMAL, 100);

	//効果音を再生
	GameSound->GrenadeExplosion(gx, gy, gz, teamid);

	return returnflag;
}

//! @brief 倒れた際のエフェクト設定
//! @param in_human 対象の人オブジェクト
void ObjectManager::DeadEffect(human *in_human)
{
	if( in_human == NULL ){ return; }

	int paramid;
	HumanParameter data;
	bool robot;

	in_human->GetParamData(&paramid, NULL, NULL, NULL);

	if( GameParamInfo->GetHuman(paramid, &data) != 0 ){ return; }

	if( data.type == 1 ){
		robot = true;
	}
	else{
		robot = false;
	}

	//もしロボットならば
	if( robot == true ){
		float hx, hy, hz, hrx, hry;

		//各種パラメーターを取得
		in_human->GetPosData(&hx, &hy, &hz, &hrx);
		hry = in_human->GetDeadRy();

		//腰辺りの座標を算出
		hx += cosf(hrx*-1 - (float)M_PI/2) * sinf(hry) * HUMAN_HEIGHT/2;
		hz += sinf(hrx*-1 - (float)M_PI/2) * sinf(hry) * HUMAN_HEIGHT/2;

		//エフェクト（煙）の描画
		float rnd = DegreeToRadian(10)*GetRand(18);
		AddEffect(hx+1.0f, hy+1.0f, hz+1.0f, 0.1f, 0.05f, 0.1f, 0.0f, rnd, 0, GetRand(30)*0.1f+30.0f, 0.1f, Resource->GetEffectSmokeTexture(), 10, 0, 1.0f, -1.0f / (GAMEFPS * 3), EFFECT_NORMAL, (int)GAMEFPS * 3);
		AddEffect(hx+1.0f, hy-1.0f, hz-1.0f, 0.2f, 0.05f, -0.2f, 0.0f, rnd*-1, 0, GetRand(30)*0.1f+30.0f, 0.1f, Resource->GetEffectSmokeTexture(), 10, 0, 1.0f, -1.0f / (GAMEFPS * 3), EFFECT_NORMAL, (int)GAMEFPS * 3);
		AddEffect(hx-1.0f, hy-1.0f, hz-1.0f, -0.3f, 0.05f, -0.3f, 0.0f, rnd, 0, GetRand(30)*0.1f+30.0f, 0.1f, Resource->GetEffectSmokeTexture(), 10, 0, 1.0f, -1.0f / (GAMEFPS * 3), EFFECT_NORMAL, (int)GAMEFPS * 3);
	}
}

//! マップに血が付着するか判定
//! @param in_effect 対象のエフェクトオブジェクト
//! @param id 付着するブロック番号を受け取るポインタ
//! @param face 付着する面番号を受け取るポインタ
//! @param pos_x X座標を受け取るポインタ
//! @param pos_y Y座標を受け取るポインタ
//! @param pos_z Z座標を受け取るポインタ
//! @return 付着する：true　付着しない：false
bool ObjectManager::CollideBlood(effect *in_effect, int *id, int *face, float *pos_x, float *pos_y, float *pos_z)
{
	if( in_effect == NULL ){ return false; }
	if( (id == NULL)||(face == NULL) ){ return false; }
	if( (pos_x == NULL)||(pos_y == NULL)||(pos_z == NULL) ){ return false; }

	//無効なエフェクトならば処理しない
	if( in_effect->GetEnableFlag() == false ){ return false; }
	if( in_effect->GetCollideMapFlag() == false ){ return false; }
	if( in_effect->GetTextureID() != Resource->GetEffectBloodTexture() ){ return false; }

	//血が出ない設定なら処理しない
	if( GameConfig.GetBloodFlag() == false ){ return false; }

	float x, y, z;
	float move_x, move_y, move_z;
	float vx, vy, vz, dist, dist2;

	//エフェクトのパラメーター取得
	in_effect->GetPosData(&x, &y, &z, NULL);
	in_effect->GetMove(&move_x, &move_y, &move_z);

	//エフェクトが移動しないなら処理しない
	if( (move_x == 0.0f)&&(move_y == 0.0f)&&(move_z == 0.0f) ){ return false; }

	//ベクトルを求める
	dist = sqrtf(move_x*move_x + move_y*move_y + move_z*move_z);
	vx = move_x / dist;
	vy = move_y / dist;
	vz = move_z / dist;

	//レイの当たり判定
	if( CollD->CheckALLBlockIntersectRay(x, y, z, vx, vy, vz, id, face, &dist2, dist) == false ){
		return false;
	}

	//マップよりわずかに浮かせる
	dist2 -= 1.0f;

	//付着する座標を求める
	*pos_x = x + vx*dist2;
	*pos_y = y + vy*dist2;
	*pos_z = z + vz*dist2;

	return true;
}

//! @brief 武器を拾う
//! @param in_human 対象の人オブジェクト
//! @param in_weapon 対象の武器オブジェクト
//! @attention 人の種類が ゾンビ の場合、この関数は失敗します。
void ObjectManager::PickupWeapon(human *in_human, weapon *in_weapon)
{
	if( in_human == NULL ){ return; }
	if( in_weapon == NULL ){ return; }

	//無効な人ならば処理しない
	if( in_human->GetEnableFlag() == false ){ return; }
	if( in_human->GetHP() <= 0 ){ return; }

	//人の種類が ゾンビ ならば、失敗
	int paramid;
	HumanParameter Paraminfo;
	in_human->GetParamData(&paramid, NULL, NULL, NULL);
	GameParamInfo->GetHuman(paramid, &Paraminfo);
	if( Paraminfo.type == 2 ){ return; }

	//初期化されている武器で、かつ誰も使っていない武器ならば～
	if( (in_weapon->GetEnableFlag() == true)&&(in_weapon->GetUsingFlag() == false) ){
		float human_x, human_y, human_z;
		float weapon_x, weapon_y, weapon_z;
		float x, z;
		float r;

		//人と武器の座標を取得
		in_human->GetPosData(&human_x, &human_y, &human_z, NULL);
		in_weapon->GetPosData(&weapon_x, &weapon_y, &weapon_z, NULL);

		//高さが範囲内ならば
		if( ((human_y + HUMAN_PICKUPWEAPON_L) <= weapon_y)&&(weapon_y <= (human_y + HUMAN_PICKUPWEAPON_H)) ){
			//距離を計算
			x = human_x - weapon_x;
			z = human_z - weapon_z;
			r = x*x + z*z;
			//距離も範囲内ならば、拾わせる。
			if( r < (HUMAN_PICKUPWEAPON_R * HUMAN_PICKUPWEAPON_R) ){
				in_human->PickupWeapon(in_weapon);
			}
		}
	}
}

//! @brief オブジェクトを解放
void ObjectManager::CleanupPointDataToObject()
{
	StagedTotal = 0; // [OpenXOPS-web] no waves or queued AI initialisations survive teardown.
	for(int i=0; i<MAX_HUMAN; i++){ StagedPending[i] = false; Human_NeedAIInit[i] = false; }
	for(int i=0; i<MAX_HUMAN; i++){
		HumanIndex[i].SetEnableFlag(false);
	}

	for(int i=0; i<MAX_WEAPON; i++){
		WeaponIndex[i].SetEnableFlag(false);
	}

	for(int i=0; i<MAX_SMALLOBJECT; i++){
		SmallObjectIndex[i].SetEnableFlag(false);
	}

	for(int i=0; i<MAX_BULLET; i++){
		BulletIndex[i].SetEnableFlag(false);
	}

	for(int i=0; i<MAX_GRENADE; i++){
		GrenadeIndex[i].SetEnableFlag(false);
	}

	for(int i=0; i<MAX_EFFECT; i++){
		EffectIndex[i].SetEnableFlag(false);
	}


	if( Resource != NULL ){
		Resource->CleanupHumanTexture();
	}
}

//! @brief ポイントデータを元にオブジェクトを配置
void ObjectManager::LoadPointData()
{
	AddCollisionFlag = false;
	Player_HumanID = 0;

	//[OpenXOPS-web] reset staged humans
	StagedTotal = 0;
	for(int i=0; i<MAX_HUMAN; i++){
		StagedPending[i] = false;
		Human_NeedAIInit[i] = false;
	}

	//発砲フラグ初期化
	for(int i=0; i<MAX_HUMAN; i++){
		Human_ShotFlag[i] = false;
		Human_YakkyouCnt[i] = -1;
	}

	//FF（同士討ち）有効化フラグ設定
	for(int i=0; i<MAX_HUMAN; i++){
		FriendlyFire[i] = false;
	}

	//人のテクスチャリフレッシュフラグ初期化
	for(int i=0; i<MAX_HUMAN; i++){
		Human_FrameTextureRefresh[i] = false;
	}

	/*
	//人情報ポイントを探す
	for(int i=0; i<PointData->GetTotaldatas(); i++){
		pointdata data;
		PointData->Getdata(&data, i);

		if( data.p1 == POINT_P1TYPE_HUMANINFO ){
			//人のテクスチャを登録
			Resource->AddHumanTexture(data.p2);
		}
	}
	*/

	//人・武器・小物を探す
	for(int i=0; i<PointData->GetTotaldatas(); i++){
		pointdata data;
		PointData->Getdata(&data, i);

		//人ならば
		if( (data.p1 == POINT_P1TYPE_HUMAN)||(data.p1 == POINT_P1TYPE_HUMAN2) ){
			pointdata humaninfodata;

			//人情報ポイントを探す
			if( PointData->SearchPointdata(&humaninfodata, 0x01 + 0x08, POINT_P1TYPE_HUMANINFO, 0, 0, data.p2, 0) == 0 ){
				//continue;

				//人情報ポイントが見つからなかったら、とりあえず「特殊 黒 A」として追加。（バグの再現）

				HumanParameter HumanParam;
				int Weapon[TOTAL_HAVEWEAPON];
				GameParamInfo->GetHuman(0, &HumanParam);
				for(int j=0; j<TOTAL_HAVEWEAPON; j++){
					Weapon[j] = HumanParam.Weapon[j];
				}

				//人のテクスチャを登録
				Resource->AddHumanTexture(0);

				//人を追加
				if( data.p1 == POINT_P1TYPE_HUMAN2 ){
					Weapon[1] = ID_WEAPON_NONE;
				}

				//プレイヤーならば、番号を記録
				if( (data.p4 == 0)&&(Player_HumanID == 0) ){
					Player_HumanID = AddHumanIndex(data.x, data.y, data.z, data.r, 0, 0, Weapon, data.p4, true);
				}
				else{
					AddHumanIndex(data.x, data.y, data.z, data.r, 0, 0, Weapon, data.p4, true);
				}
			}
			else{
				//人のテクスチャを登録
				Resource->AddHumanTexture(humaninfodata.p2);

				//人として追加
				AddHumanIndex(data, humaninfodata, true);
			}
		}

		//[OpenXOPS-web] Staged human: remember it, preload its texture, spawn later.
		//Group 0 is rejected (p4 == 0 marks the player in upstream code).
		if( (data.p1 == POINT_P1TYPE_STAGED_HUMAN)&&(data.p4 != 0)&&(StagedTotal < MAX_HUMAN) ){
			pointdata humaninfodata;
			if( PointData->SearchPointdata(&humaninfodata, 0x01 + 0x08, POINT_P1TYPE_HUMANINFO, 0, 0, data.p2, 0) != 0 ){
				Resource->AddHumanTexture(humaninfodata.p2);
				StagedPoint[StagedTotal] = data;
				StagedPoint[StagedTotal].p1 = POINT_P1TYPE_HUMAN;
				StagedInfo[StagedTotal] = humaninfodata;
				StagedPending[StagedTotal] = true;
				StagedTotal += 1;
			}
		}

		//武器ならば
		if( (data.p1 == POINT_P1TYPE_WEAPON)||(data.p1 == POINT_P1TYPE_RAND_WEAPON) ){
			AddWeaponIndex(data);
		}

		//小物ならば
		if( data.p1 == POINT_P1TYPE_SMALLOBJ ){
			AddSmallObjectIndex(data);
		}
	}

	//ログ関係の処理
	ObjectLog->ClearLog();
	ObjectLog->InfoLog("Set object...");
}

//! @brief リソースの回復
//! @todo 全てのエフェクトが無効化する。
void ObjectManager::Recovery()
{
	//人のモデル番号を取得
	int upmodel[TOTAL_UPMODE];
	int armmodel[TOTAL_ARMMODE];
	int legmodel;
	int walkmodel[TOTAL_WALKMODE];
	int runmodel[TOTAL_RUNMODE];
	Resource->GetHumanModel(upmodel, armmodel, &legmodel, walkmodel, runmodel);

	//人
	int HumanID;
	int GetHumanFlag;
	HumanParameter HumanParam;
	for(int i=0; i<MAX_HUMAN; i++){
		if( HumanIndex[i].GetEnableFlag() == true ){
			HumanIndex[i].GetParamData(&HumanID, NULL, NULL, NULL);

			GetHumanFlag = GameParamInfo->GetHuman(HumanID, &HumanParam);

#ifndef ENABLE_BUG_HUMANWEAPON
			if( GetHumanFlag != 0 ){
				HumanIndex[i].SetEnableFlag(false);
				continue;
			}
#endif

			if( GetHumanFlag == 0 ){
				//人のテクスチャを登録
				Resource->AddHumanTexture(HumanID);

				int id = Resource->GetHumanTexture(HumanID);
				if( id == -1 ){
					//テクスチャが取得できなければ、リフレッシュフラグを有効にして毎フレーム取得
					Human_FrameTextureRefresh[i] = true;
				}

				HumanIndex[i].SetTexture(id);
				HumanIndex[i].SetModel(upmodel[ HumanParam.model ], armmodel, legmodel, walkmodel, runmodel);
			}
			else{
#ifdef ENABLE_BUG_HUMANWEAPON
				HumanIndex[i].SetTexture(d3dg->GetMapTextureID(0));
				HumanIndex[i].SetModel(upmodel[0], armmodel, legmodel, walkmodel, runmodel);
#endif
			}
		}
	}

	//武器
	int WeaponID;
	int Weaponmodel, Weapontexture;
	for(int i=0; i<MAX_WEAPON; i++){
		if( WeaponIndex[i].GetEnableFlag() == true ){
			//設定値を取得
			WeaponParameter WeaponParam;
			WeaponIndex[i].GetParamData(&WeaponID, NULL, NULL);
			if( Resource->GetWeaponModelTexture(WeaponID, &Weaponmodel, &Weapontexture) == 1 ){ continue; }
			if( GameParamInfo->GetWeapon(WeaponID, &WeaponParam) == 1 ){ continue; }

			//適用
			WeaponIndex[i].SetModel(Weaponmodel, WeaponParam.size);
			WeaponIndex[i].SetTexture(Weapontexture);
		}
	}

	//小物
	int SmallObjectID;
	int SmallObjectmodel, SmallObjecttexture;
	for(int i=0; i<MAX_SMALLOBJECT; i++){
		if( SmallObjectIndex[i].GetEnableFlag() == true ){
			//設定値を取得
			SmallObjectIndex[i].GetParamData(&SmallObjectID, NULL);
			if( Resource->GetSmallObjectModelTexture(SmallObjectID, &SmallObjectmodel, &SmallObjecttexture) == 1 ){ continue; }

			//適用
			SmallObjectIndex[i].SetModel(SmallObjectmodel, SMALLOBJECT_SCALE);
			SmallObjectIndex[i].SetTexture(SmallObjecttexture);
		}
	}

	int bulletmodel, bullettexture;
	BulletParameter data;

	//銃弾適用
	if( Resource->GetBulletModelTexture(0, &bulletmodel, &bullettexture) == 0 ){
		if( GameParamInfo->GetBullet(0, &data) == 0 ){
			for(int i=0; i<MAX_BULLET; i++){
				BulletIndex[i].SetModel(bulletmodel, data.size);
				BulletIndex[i].SetTexture(bullettexture);
			}
		}
	}

	//手榴弾適用
	if( Resource->GetBulletModelTexture(1, &bulletmodel, &bullettexture) == 0 ){
		if( GameParamInfo->GetBullet(1, &data) == 0 ){
			for(int i=0; i<MAX_GRENADE; i++){
				GrenadeIndex[i].SetModel(bulletmodel, data.size);
				GrenadeIndex[i].SetTexture(bullettexture);
			}
		}
	}

	//全てのエフェクトを無効化
	for(int i=0; i<MAX_EFFECT; i++){
		if( EffectIndex[i].GetEnableFlag() == true ){
			EffectIndex[i].SetEnableFlag(false);
		}
	}

	//ログ関係の処理
	ObjectLog->InfoLog("Recovery object...");
}

//! @brief 追加の当たり判定フラグを設定
//! @param flag フラグ
void ObjectManager::SetAddCollisionFlag(bool flag)
{
	AddCollisionFlag = flag;
}

//! @brief FF（同士討ち）有効化フラグを取得
//! @param id データ番号
//! @return フラグ
bool ObjectManager::GetFriendlyFireFlag(int id)
{
	if( (id < 0)||(MAX_HUMAN <= id) ){ return false; }
	return FriendlyFire[id];
}

//! @brief FF（同士討ち）有効化フラグを設定
//! @param id データ番号
//! @param flag フラグ
void ObjectManager::SetFriendlyFireFlag(int id, bool flag)
{
	if( (id < 0)||(MAX_HUMAN <= id) ){ return; }
	FriendlyFire[id] = flag;
}

//! @brief プレイヤー番号を取得
//! @return プレイヤーのデータ番号
int ObjectManager::GetPlayerID()
{
	return Player_HumanID;
}

//! @brief プレイヤー番号を設定
//! @param id プレイヤーのデータ番号
void ObjectManager::SetPlayerID(int id)
{
	Player_HumanID = id;
}

//! @brief 指定したデータ番号のhumanクラスを取得
//! @param id データ番号
//! @return 人オブジェクトのポインタ　（無効なデータ番号で NULL）
human* ObjectManager::GetHumanObject(int id)
{
	if( (id < 0)||(MAX_HUMAN <= id) ){ return NULL; }
	return &(HumanIndex[id]);
}

//! @brief プレイヤーのhumanクラスを取得
//! @return 人オブジェクト（プレイヤー）のポインタ
human* ObjectManager::GetPlayerHumanObject()
{
	return GetHumanObject(Player_HumanID);
}

//! @brief 指定したhumanポインタのデータ番号を取得
//! @param object 人オブジェクトのポインタ
//! @return データ番号　（エラー：-1）
int ObjectManager::GetHumanObjectID(human* object)
{
	if( object == NULL ){ return -1; }

	for(int i=0; i<MAX_HUMAN; i++){
		if( &(HumanIndex[i]) == object ){
			return i;
		}
	}

	return -1;
}

//! @brief 指定したデータ番号のweaponクラスを取得
//! @param id データ番号
//! @return 武器オブジェクトのポインタ　（無効なデータ番号で NULL）
weapon* ObjectManager::GetWeaponObject(int id)
{
	if( (id < 0)||(MAX_WEAPON <= id) ){ return NULL; }
	return &(WeaponIndex[id]);
}

//! @brief 指定したデータ番号のsmallobjectクラスを取得
//! @param id データ番号
//! @return 小物オブジェクトのポインタ　（無効なデータ番号で NULL）
smallobject* ObjectManager::GetSmallObject(int id)
{
	if( (id < 0)||(MAX_SMALLOBJECT <= id) ){ return NULL; }
	return &(SmallObjectIndex[id]);
}

//! @brief 指定したデータ番号のbulletクラスを取得
//! @param id データ番号
//! @return 弾オブジェクトのポインタ　（無効なデータ番号で NULL）
bullet* ObjectManager::GetBulletObject(int id)
{
	if( (id < 0)||(MAX_BULLET <= id) ){ return NULL; }
	return &(BulletIndex[id]);
}

//! @brief 指定したbulletポインタのデータ番号を取得
//! @param object 弾オブジェクトのポインタ
//! @return データ番号　（エラー：-1）
int ObjectManager::GetBulletObjectID(bullet* object)
{
	if( object == NULL ){ return -1; }

	for(int i=0; i<MAX_BULLET; i++){
		if( &(BulletIndex[i]) == object ){
			return i;
		}
	}

	return -1;
}

//! @brief 使用されていないbulletクラスを取得
//! @return 現在未使用の弾オブジェクトのポインタ　（失敗すると NULL）
bullet* ObjectManager::GetNewBulletObject()
{
	for(int i=0; i<MAX_BULLET; i++){
		if( BulletIndex[i].GetEnableFlag() == false ){
			return &(BulletIndex[i]);
		}
	}
	return NULL;
}

//! @brief 使用されていないgrenadeクラスを取得
//! @return 現在未使用の手榴弾オブジェクトのポインタ　（失敗すると NULL）
grenade* ObjectManager::GetNewGrenadeObject()
{
	for(int i=0; i<MAX_GRENADE; i++){
		if( GrenadeIndex[i].GetEnableFlag() == false ){
			return &(GrenadeIndex[i]);
		}
	}
	return NULL;
}

//! @brief 人を検索
//! @param p4 検索対象の識別番号
//! @return 該当したhumanクラスのポインタ　（見つからない場合はNULL）
//! @attention 複数該当する場合、最初に該当したデータを返します。
human* ObjectManager::SearchHuman(signed short int p4)
{
	signed short int humanp4;

	for(int i=0; i<MAX_HUMAN; i++){
		//使われていない人ならば処理しない
		if( HumanIndex[i].GetEnableFlag() == false ){ continue; }

		//第4パラメータを取得
		HumanIndex[i].GetParamData(NULL, NULL, &humanp4, NULL);

		//指定されたp4と一致すれば返す
		if( humanp4 == p4 ){
			return &(HumanIndex[i]);
		}
	}
	return NULL;
}

//! @brief 小物を検索
//! @param p4 検索対象の識別番号
//! @return 該当したsmallobjectクラスのポインタ　（見つからない場合はNULL）
//! @attention 複数該当する場合、最初に該当したデータを返します。
smallobject* ObjectManager::SearchSmallobject(signed short int p4)
{
	signed short int smallobjectp4;

	for(int i=0; i<MAX_SMALLOBJECT; i++){
		//使われていない人ならば処理しない
		//　【破壊積みのオブジェクトも判定するため、無効】
		//if( SmallObjectIndex[i].GetEnableFlag() == false ){ continue; }

		//第4パラメータを取得
		SmallObjectIndex[i].GetParamData(NULL, &smallobjectp4);

		//指定されたp4と一致すれば返す
		if( smallobjectp4 == p4 ){
			return &(SmallObjectIndex[i]);
		}
	}
	return NULL;
}

//! @brief 有効なオブジェクト数を取得
//! @param HumanCnt 人オブジェクトの有効数を受け取るポインタ　（NULL可）
//! @param WeaponCnt 武器オブジェクトの有効数を受け取るポインタ　（NULL可）
//! @param SmallobjectCnt 小物オブジェクトの有効数を受け取るポインタ　（NULL可）
//! @param BulletCnt 弾オブジェクトの有効数を受け取るポインタ　（NULL可）
//! @param GrenadeCnt 手榴弾オブジェクトの有効数を受け取るポインタ　（NULL可）
//! @param EffectCnt エフェクトオブジェクトの有効数を受け取るポインタ　（NULL可）
void ObjectManager::GetTotalObjects(int *HumanCnt, int *WeaponCnt, int *SmallobjectCnt, int *BulletCnt, int *GrenadeCnt, int *EffectCnt)
{
	if( HumanCnt != NULL ){
		*HumanCnt = 0;
		for(int i=0; i<MAX_HUMAN; i++){
			if( HumanIndex[i].GetEnableFlag() == true ){ *HumanCnt += 1; }
		}
	}

	if( WeaponCnt != NULL ){
		*WeaponCnt = 0;
		for(int i=0; i<MAX_WEAPON; i++){
			if( WeaponIndex[i].GetEnableFlag() == true ){ *WeaponCnt += 1; }
		}
	}

	if( SmallobjectCnt != NULL ){
		*SmallobjectCnt = 0;
		for(int i=0; i<MAX_SMALLOBJECT; i++){
			if( SmallObjectIndex[i].GetEnableFlag() == true ){ *SmallobjectCnt += 1; }
		}
	}

	if( BulletCnt != NULL ){
		*BulletCnt = 0;
		for(int i=0; i<MAX_BULLET; i++){
			if( BulletIndex[i].GetEnableFlag() == true ){ *BulletCnt += 1; }
		}
	}

	if( GrenadeCnt != NULL ){
		*GrenadeCnt = 0;
		for(int i=0; i<MAX_GRENADE; i++){
			if( GrenadeIndex[i].GetEnableFlag() == true ){ *GrenadeCnt += 1; }
		}
	}

	if( EffectCnt != NULL ){
		*EffectCnt = 0;
		for(int i=0; i<MAX_EFFECT; i++){
			if( EffectIndex[i].GetEnableFlag() == true ){ *EffectCnt += 1; }
		}
	}
}

//! @brief 前進（走り）を実行
//! @param human_id 人の番号（0～MAX_HUMAN-1）
void ObjectManager::MoveForward(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }

	//オブジェクトにフラグを設定
	HumanIndex[human_id].SetMoveForward();
}

//! @brief 後退を実行
//! @param human_id 人の番号（0～MAX_HUMAN-1）
void ObjectManager::MoveBack(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }

	//オブジェクトにフラグを設定
	HumanIndex[human_id].SetMoveBack();
}

//! @brief 左走りを実行
//! @param human_id 人の番号（0～MAX_HUMAN-1）
void ObjectManager::MoveLeft(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }

	//オブジェクトにフラグを設定
	HumanIndex[human_id].SetMoveLeft();
}

//! @brief 右走りを実行
//! @param human_id 人の番号（0～MAX_HUMAN-1）
void ObjectManager::MoveRight(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }

	//オブジェクトにフラグを設定
	HumanIndex[human_id].SetMoveRight();
}

//! @brief 歩きを実行
//! @param human_id 人の番号（0～MAX_HUMAN-1）
void ObjectManager::MoveWalk(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }

	//オブジェクトにフラグを設定
	HumanIndex[human_id].SetMoveWalk();
}

//! @brief ジャンプ
//! @param human_id 人の番号（0～MAX_HUMAN-1）
void ObjectManager::MoveJump(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }

	HumanIndex[human_id].Jump();
}

//! @brief 発砲
//! @param human_id 発砲する人番号
//! @return 通常弾発射：1　手榴弾発射：2　失敗：0
int ObjectManager::ShotWeapon(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return 0; }

	human *MyHuman = &(HumanIndex[human_id]);

	float pos_x, pos_y, pos_z;
	int teamid;
	float rotation_x, armrotation_y;
	int weapon_paramid;
	int GunsightErrorRange;
	WeaponParameter ParamData;
	bool playerflag;
	bool grenadeflag;

	//人の座標と角度を取得
	MyHuman->GetPosData(&pos_x, &pos_y, &pos_z, NULL);
	MyHuman->GetParamData(NULL, NULL, NULL, &teamid);
	MyHuman->GetRxRy(&rotation_x, &armrotation_y);

	//対象者がプレイヤー自身か判定
	if( MyHuman == &(HumanIndex[Player_HumanID]) ){
		playerflag = true;
	}
	else{
		playerflag = false;
	}

	//弾の発射を要求
	if( MyHuman->ShotWeapon(&weapon_paramid, &GunsightErrorRange) == false ){ return 0; }

	//武器の情報を取得
	if( GameParamInfo->GetWeapon(weapon_paramid, &ParamData) != 0 ){ return 0; }

	//手榴弾か判定
	if( weapon_paramid == ID_WEAPON_GRENADE ){ grenadeflag = true; }
	else{ grenadeflag = false; }

	//誤差の範囲を計算
	int ErrorRange;
	ErrorRange = GunsightErrorRange;
	if( ErrorRange < ParamData.ErrorRangeMIN ){ ErrorRange = ParamData.ErrorRangeMIN; }

	//発射角度（誤差ゼロのベース）を決定
	float rx, ry;
	rx = rotation_x*-1 + (float)M_PI/2;
	ry = armrotation_y;

	//誤差を決定
	if( ErrorRange != 0 ){
		int ErrorRange_x, ErrorRange_y;

		ErrorRange_x = GetRand(ErrorRange*2+1) - ErrorRange;
		ErrorRange_y = GetRand(ErrorRange*2+1) - ErrorRange;

		//発射誤差分　角度に加算
		rx += ErrorRange_x * DegreeToRadian(0.15f);
		ry += ErrorRange_y * DegreeToRadian(0.15f);
	}

	//手榴弾でなければ
	if( grenadeflag == false ){
		class bullet* newbullet;

		//ショットガンなど発射する弾（ペレット）の数分繰り返す
		for(int i=0; i<ParamData.pellet; i++){
			int attacks;
			float ontargetcnt;
			float rx2, ry2;

			//ショットガンなど発射する弾（ペレット）が複数あれば
			if( ParamData.pellet > 1 ){
				//1個の弾あたりの攻撃力と命中時のカウント数を算出
				//　　全弾合わせて、2倍になるようにする。
				attacks = (int)( (float)ParamData.attacks / ((float)ParamData.pellet/2) );
				ontargetcnt = 1.0f / ((float)ParamData.pellet/2);

				//さらに誤差を増やして拡散させる
				int len;
				float a = DegreeToRadian(10) * GetRand(36);
				len = GetRand(5)*2+5;
				rx2 = rx + cosf(a)*len * DegreeToRadian(0.15f);
				ry2 = ry + sinf(a)*len * DegreeToRadian(0.15f);
			}
			else{
				//そのまま攻撃力と命中時のカウント数を反映
				attacks = ParamData.attacks;
				ontargetcnt = 1.0f;

				//発射誤差はそのまま
				rx2 = rx;
				ry2 = ry;
			}

			//発射する未使用のオブジェクトを取得
			newbullet = GetNewBulletObject();
			if( newbullet == NULL ){ return 0; }

			//銃弾を発射
			newbullet->SetPosData(pos_x, pos_y + WEAPONSHOT_HEIGHT, pos_z, rx2, ry2);
			newbullet->SetParamData(attacks, ParamData.penetration, (float)ParamData.speed * BULLET_SPEEDSCALE, teamid, human_id, ontargetcnt, true);
			newbullet->SetEnableFlag(true);

			//[OpenXOPS-web] Per-weapon bolt visuals (pooled bullets are reused, so
			//set the visual on every shot). Type 0 = upstream bullet; missing
			//laser files fall back to it.
			{
				int bt = ParamData.bullettype;
				int bmodel, btexture;
				BulletParameter bdata;
				if( (bt <= 0)||(bt == 1)||(Resource->GetBulletModelTexture(bt, &bmodel, &btexture) != 0)||(bmodel == -1)||(btexture == -1)||(GameParamInfo->GetBullet(bt, &bdata) != 0) ){
					bt = 0;
					Resource->GetBulletModelTexture(0, &bmodel, &btexture);
					GameParamInfo->GetBullet(0, &bdata);
				}
				newbullet->SetModel(bmodel, bdata.size);
				newbullet->SetTexture(btexture);
				Bullet_LaserBolt[ GetBulletObjectID(newbullet) ] = (2 <= ParamData.bullettype)&&(ParamData.bullettype <= 4);
			}

			//対人判定用リスト初期化
			BulletObj_HumanIndex[ GetBulletObjectID(newbullet) ].Init();
		}

		//発砲フラグを設定
		Human_ShotFlag[human_id] = true;
		Human_YakkyouCnt[human_id] = ParamData.yakkyou_delay;
	}
	else{
		class grenade* newgrenade;

		//発射する未使用のオブジェクトを取得
		newgrenade = GetNewGrenadeObject();
		if( newgrenade == NULL ){ return 0; }

		//手榴弾発射
		newgrenade->SetPosData(pos_x, pos_y + WEAPONSHOT_HEIGHT, pos_z, rx, ry);
		newgrenade->SetParamData(8.0f, teamid, human_id, 1.0f, true);
		newgrenade->SetEnableFlag(true);
	}

	if( ParamData.soundvolume > 0 ){
		//銃声を再生
		GameSound->ShotWeapon(pos_x, pos_y + WEAPONSHOT_HEIGHT, pos_z, weapon_paramid, teamid, playerflag);
	}

	if( grenadeflag == true ){
		return 2;
	}
	return 1;
}

//! @brief マズルフラッシュを設定
//! @param humanid 人の番号
//! @attention この関数の呼び出しタイミングを誤ると、銃口に対してマズルフラッシュが合いません。
void ObjectManager::ShotWeaponEffect(int humanid)
{
	//値の範囲をチェック
	if( (humanid < 0)||(MAX_HUMAN <= humanid) ){ return; }

	float pos_x, pos_y, pos_z;
	float rotation_x, armrotation_y;
	int weapon_paramid;
	WeaponParameter ParamData;
	float x, y, z;
	float flashsize, smokesize1, smokesize2;
	float rx, emx, emy, emz;

	//人の座標と角度を取得
	HumanIndex[humanid].GetPosData(&pos_x, &pos_y, &pos_z, NULL);
	HumanIndex[humanid].GetRxRy(&rotation_x, &armrotation_y);

	//武器の情報を取得
	weapon_paramid = HumanIndex[humanid].GetMainWeaponTypeNO();
	if( GameParamInfo->GetWeapon(weapon_paramid, &ParamData) != 0 ){ return; }

	//マズルフラッシュと煙のサイズを決定
	if( ParamData.silencer == true ){ flashsize = 1.2f; }
	else{ flashsize = 0.06f * ParamData.attacks; }
	smokesize1 = 0.04f * ParamData.attacks;
	smokesize2 = 0.032f * ParamData.attacks;

	//煙の移動ベクトルを決定
	rx = rotation_x*-1 + (float)M_PI/2;
	emx = cosf(rx)*cosf(armrotation_y);
	emy = sinf(armrotation_y);
	emz = sinf(rx)*cosf(armrotation_y);

	//行列でエフェクト座標を計算
	d3dg->SetWorldTransformHumanWeapon(pos_x, pos_y + 16.0f, pos_z, ParamData.flashx/10, ParamData.flashy/10, ParamData.flashz/10, rotation_x, armrotation_y*-1, 1.0f);
	d3dg->GetWorldTransformPos(&x, &y, &z);
	d3dg->ResetWorldTransform();

	//マズルフラッシュ描画
	AddEffect(x, y, z, 0.0f, 0.0f, 0.0f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, flashsize, 0.0f, Resource->GetEffectMflashTexture(), 0, 0, 0.63f, 0.0f, EFFECT_NORMAL, 1);

	//行列でエフェクト座標を計算
	d3dg->SetWorldTransformHumanWeapon(pos_x, pos_y + 16.0f, pos_z, ParamData.flashx/10, ParamData.flashy/10, ParamData.flashz/10 - 0.1f, rotation_x, armrotation_y*-1, 1.0f);
	d3dg->GetWorldTransformPos(&x, &y, &z);
	d3dg->ResetWorldTransform();

	//エフェクト（煙）の描画
	AddEffect(x, y, z, emx*0.15f, emy*0.15f + 0.02f, emz*0.15f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, smokesize1, 0.3f, Resource->GetEffectSmokeTexture(), 0, 0, 0.47f, -0.047f, EFFECT_NORMAL, 12);
	AddEffect(x, y, z, emx*0.45f, emy*0.45f, emz*0.45f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, smokesize2, 0.36f, Resource->GetEffectSmokeTexture(), 0, 0, 0.47f, -0.055f, EFFECT_NORMAL, 10);
}

//! @brief 薬莢排出を設定
//! @param humanid 人の番号
//! @attention この関数の呼び出しタイミングを誤ると、銃に対して薬莢排出位置が合いません。
void ObjectManager::ShotWeaponYakkyou(int humanid)
{
	//値の範囲をチェック
	if( (humanid < 0)||(MAX_HUMAN <= humanid) ){ return; }

	float pos_x, pos_y, pos_z;
	float rotation_x, armrotation_y;
	int weapon_paramid;
	WeaponParameter ParamData;
	float x, y, z;
	float yakkyousize;
	float rx;
	float mx, my, mz;

	//人の座標と角度を取得
	HumanIndex[humanid].GetPosData(&pos_x, &pos_y, &pos_z, NULL);
	HumanIndex[humanid].GetRxRy(&rotation_x, &armrotation_y);

	//武器の情報を取得
	weapon_paramid = HumanIndex[humanid].GetMainWeaponTypeNO();
	if( GameParamInfo->GetWeapon(weapon_paramid, &ParamData) != 0 ){ return; }

	//[OpenXOPS-web] Lasers eject no casings; do not allocate invisible effects.
	if( ParamData.yakkyou_size == 0.0f ){ return; }
	//マズルフラッシュと煙のサイズを決定
	if( ParamData.yakkyou_size >= 0.0f ){ yakkyousize = ParamData.yakkyou_size/10; }
	else{ yakkyousize = 0.012f * ParamData.attacks; }

	//武器の情報を取得
	weapon_paramid = HumanIndex[humanid].GetMainWeaponTypeNO();
	if( GameParamInfo->GetWeapon(weapon_paramid, &ParamData) != 0 ){ return; }

	//回転量を決定
	rx = rotation_x*-1 + (float)M_PI/2;

	//薬莢の移動量を決定
	mx = cosf(rx - (float)M_PI/2) * ParamData.yakkyou_sx * 0.12f;
	my = (ParamData.yakkyou_sy + (GetRand(3)-1)) * 0.12f;
	mz = sinf(rx - (float)M_PI/2) * ParamData.yakkyou_sx * 0.12f;

	//行列でエフェクト座標を計算
	d3dg->SetWorldTransformHumanWeapon(pos_x, pos_y + 16.0f, pos_z, ParamData.yakkyou_px/10, ParamData.yakkyou_py/10, ParamData.yakkyou_pz/10, rotation_x, armrotation_y*-1, 1.0f);
	d3dg->GetWorldTransformPos(&x, &y, &z);
	d3dg->ResetWorldTransform();

	//薬莢描画
	AddEffect(x, y, z, mx, my, mz, -0.15f, DegreeToRadian(10)*GetRand(36), DegreeToRadian(GetRand(61)-30), yakkyousize, 0.0f, Resource->GetEffectYakkyouTexture(), 0, 0, 1.0f, 0.0f, EFFECT_NORMAL, (int)(GAMEFPS*0.75f));
}

//! @brief 武器をリロード
//! @param human_id 対象の人番号
//! @return 成功：true　失敗：false
bool ObjectManager::ReloadWeapon(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return false; }

	//無効な人ならば処理しない
	if( HumanIndex[human_id].GetEnableFlag() == false ){ return false; }
	if( HumanIndex[human_id].GetHP() <= 0 ){ return false; }

	//リロードを実行
	if( HumanIndex[human_id].ReloadWeapon() == true ){
		float x, y, z;
		int id;

		//人の座標とチーム番号を取得
		HumanIndex[human_id].GetPosData(&x, &y, &z, NULL);
		HumanIndex[human_id].GetParamData(NULL, NULL, NULL, &id);
		y += 16.0f;

		//音源を配置
		GameSound->ReloadWeapon(x, y, z, id);

		//Human_YakkyouCnt[human_id] = -1;

		return true;
	}

	return false;
}

//! @brief 武器を切り替える（持ち替える）
//! @param human_id 人の番号（0～MAX_HUMAN-1）
//! @param id 持ち替える武器　（-1 で次の武器）
void ObjectManager::ChangeHaveWeapon(int human_id, int id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }

	HumanIndex[human_id].ChangeHaveWeapon(id);
	Human_YakkyouCnt[human_id] = -1;
}

//! @brief 武器の種類切り替え
//! @param human_id 人の番号（0～MAX_HUMAN-1）
//! @return 成功：0　失敗：1
int ObjectManager::ChangeWeaponID(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return 1; }

	//武器の種類を切り替えたら
	if( HumanIndex[human_id].ChangeWeaponID() == 0 ){
		int selectweapon;
		weapon *weapon[TOTAL_HAVEWEAPON];
		int WeaponID;
		int model, texture;
		float WeaponSize;

		//現在の武器を取得
		HumanIndex[human_id].GetWeapon(&selectweapon, weapon, NULL, NULL);
		weapon[selectweapon]->GetParamData(&WeaponID, NULL, NULL);

		//モデルとテクスチャを取得
		if( Resource->GetWeaponModelTexture(WeaponID, &model, &texture) == 1 ){
			return 1;
		}

		//武器のサイズを取得
		WeaponParameter ParamData;
		if( GameParamInfo->GetWeapon(WeaponID, &ParamData) != 0 ){ return 1; }
		WeaponSize = ParamData.size;

		//武器オブジェクトのモデル・テクスチャを変更
		weapon[selectweapon]->SetModel(model, WeaponSize);
		weapon[selectweapon]->SetTexture(texture);

		Human_YakkyouCnt[human_id] = -1;

		return 0;
	}
	return 1;
}

//! @brief 武器を捨てる
//! @param human_id 人の番号（0～MAX_HUMAN-1）
//! @return 成功：true　失敗：false
bool ObjectManager::DumpWeapon(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return false; }

	Human_YakkyouCnt[human_id] = -1;

	return HumanIndex[human_id].DumpWeapon();
}

//! @brief スコープモードを切り替え
//! @param human_id 人の番号（0～MAX_HUMAN-1）
void ObjectManager::ChangeScopeMode(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }

	if( HumanIndex[human_id].GetScopeMode() == 0 ){	//スコープを使用していなければ、スコープを設定
		HumanIndex[human_id].SetEnableScope();
	}
	else{											//使用中なら、解除
		HumanIndex[human_id].SetDisableScope();
	}

	//Human_YakkyouCnt[human_id] = -1;
}

//! @brief 裏技・所持している武器の弾を追加
//! @param human_id 人の番号（0～MAX_HUMAN-1）
//! @return 成功：true　失敗：false
bool ObjectManager::CheatAddBullet(int human_id)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return false; }

	int selectweapon;
	weapon *weapon[TOTAL_HAVEWEAPON];
	int id_param, lnbs, nbs;
	WeaponParameter ParamData;

	//所持している武器を取得
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		weapon[i] = NULL;
	}
	HumanIndex[human_id].GetWeapon(&selectweapon, weapon, NULL, NULL);

	//何かしらの武器を持っていれば
	if( weapon[selectweapon] != NULL ){
		//武器の種類と弾数、武器の設定値を取得
		weapon[selectweapon]->GetParamData(&id_param, &lnbs, &nbs);
		if( GameParamInfo->GetWeapon(id_param, &ParamData) == 0 ){
			//最大弾数分加算し、適用
			nbs += ParamData.nbsmax;
			weapon[selectweapon]->ResetWeaponParam(Resource, id_param, lnbs, nbs);
			return true;
		}
	}

	return false;
}

//! @brief 裏技・所持している武器を変更
//! @param human_id 人の番号（0～MAX_HUMAN-1）
//! @param new_weaponID 新たに設定する武器の種類番号
//! @return 成功：true　失敗：false
bool ObjectManager::CheatNewWeapon(int human_id, int new_weaponID)
{
	//値の範囲をチェック
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return false; }
	if( (new_weaponID < 0)||(TOTAL_PARAMETERINFO_WEAPON <= new_weaponID) ){ return false; }

	human *myHuman = &(HumanIndex[human_id]);

	int selectweapon;
	weapon *weapon[TOTAL_HAVEWEAPON];
	int lnbslist[TOTAL_HAVEWEAPON];
	int nbslist[TOTAL_HAVEWEAPON];
	int lnbs, nbs;

	//所持している武器を取得
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		weapon[i] = NULL;
		lnbslist[i] = 0;
		nbslist[i] = 0;
	}
	myHuman->GetWeapon(&selectweapon, weapon, lnbslist, nbslist);

	//現在武器を所有しておらず、新たに持たせる武器もないなら
	if( (weapon[selectweapon] == NULL)&&(new_weaponID == ID_WEAPON_NONE) ){
		return false;		//処理自体が無意味
	}

	if( weapon[selectweapon] == NULL ){		//武器を所有していなければ
		int dataid = -1;

		//新しい武器を配置
		dataid = AddVisualWeaponIndex(new_weaponID, false);

		//武器が配置できれば
		if( dataid != -1 ){
			//武器設定を適用（主に弾数）
			weapon[selectweapon] = &(WeaponIndex[dataid]);
			weapon[selectweapon]->ResetWeaponParam(Resource, new_weaponID, lnbslist[selectweapon], nbslist[selectweapon]);

			//未所持時の弾数情報は初期化
			lnbslist[selectweapon] = 0;
			nbslist[selectweapon] = 0;

			//武器を持たせる
			myHuman->SetWeapon(weapon, lnbslist, nbslist);

			Human_YakkyouCnt[human_id] = -1;
			return true;
		}
	}
	else{									//武器を所有していれば
		//武器設定を取得（弾数設定を引き継ぐため）
		weapon[selectweapon]->GetParamData(NULL, &lnbs, &nbs);

		if( new_weaponID == ID_WEAPON_NONE ){			//武器を消すなら
			//その武器は削除
			weapon[selectweapon]->SetEnableFlag(false);

			//武器を削除し、弾数を反映
			weapon[selectweapon] = NULL;
			lnbslist[selectweapon] = lnbs;
			nbslist[selectweapon] = nbs;
			myHuman->SetWeapon(weapon, lnbslist, nbslist);

			//スコープモードを解除
			myHuman->SetDisableScope();
		}
		else{											//武器を変更するなら
			//武器設定を適用
			weapon[selectweapon]->ResetWeaponParam(Resource, new_weaponID, lnbs, nbs);
		}

		Human_YakkyouCnt[human_id] = -1;
		return true;
	}

	return false;
}

//! @brief ゾンビの攻撃を受けるか判定
//! @param MyHuman 攻撃する人オブジェクト（ゾンビ側）のポインタ
//! @param EnemyHuman 攻撃を受けた人オブジェクトのポインタ
//! @return 成立：true　不成立：false
//! @warning MyHuman はゾンビ以外も指定できます。<b>ゾンビかどうかは判定しない</b>ため、ゾンビであるか予め確認しておく必要があります。
//! @attention 判定のみを実施します。この判定が成立したら HitZombieAttack()関数 を呼び出してください。 
bool ObjectManager::CheckZombieAttack(human* MyHuman, human* EnemyHuman)
{
	if( MyHuman == NULL ){ return false; }
	if( EnemyHuman == NULL ){ return false; }

	//使用されていないか、死亡していれば処理しない。
	if( MyHuman->GetEnableFlag() == false ){ return false; }
	if( MyHuman->GetHP() <= 0 ){ return false; }
	if( EnemyHuman->GetEnableFlag() == false ){ return false; }
	if( EnemyHuman->GetDeadFlag() == true ){ return false; }

	float mx, my, mz, mrx, tx, ty, tz;
	int mteam, tteam;
	float AttackPoint_x, AttackPoint_y, AttackPoint_z;
	float ax, az;

	MyHuman->GetPosData(&mx, &my, &mz, &mrx);
	MyHuman->GetParamData(NULL, NULL, NULL, &mteam);
	my += VIEW_HEIGHT;
	EnemyHuman->GetPosData(&tx, &ty, &tz, NULL);
	EnemyHuman->GetParamData(NULL, NULL, NULL, &tteam);
	ty += VIEW_HEIGHT;

	//味方ならば処理しない
	if( mteam == tteam ){ return false; }

	//攻撃ポイント（腕の先端）を求める
	AttackPoint_x = mx + cosf(mrx*-1 + (float)M_PI/2) * 2.0f;
	AttackPoint_y = my + VIEW_HEIGHT - 0.5f;
	AttackPoint_z = mz + sinf(mrx*-1 + (float)M_PI/2) * 2.0f;
	ax = AttackPoint_x - tx;
	az = AttackPoint_z - tz;

	//敵（攻撃対象）が攻撃ポイントに触れていれば、当たっている
	if( (ax*ax + az*az) < 3.3f*3.3f ){
		if( (AttackPoint_y >= ty)&&(AttackPoint_y <= (ty + HUMAN_HEIGHT)) ){
			return true;
		}
	}

	return false;
}

//! @brief ゾンビの攻撃を受けた処理
//! @param MyHuman 攻撃する人オブジェクト（ゾンビ側）のポインタ
//! @param EnemyHuman 攻撃を受けた人オブジェクトのポインタ
void ObjectManager::HitZombieAttack(human* MyHuman, human* EnemyHuman)
{
	if( MyHuman == NULL ){ return; }
	if( EnemyHuman == NULL ){ return; }

	//使用されていないか、死亡していれば処理しない。
	if( EnemyHuman->GetEnableFlag() == false ){ return; }
	if( EnemyHuman->GetDeadFlag() == true ){ return; }

	int MyHuman_dataID, MyHuman_TeamID;
	int EnemyHuman_dataID, EnemyHuman_TeamID;
	float mx, my, mz;
	float tx, ty, tz;
	float arx;
	int paramid;
	HumanParameter Paraminfo;
	bool NotRobot;
	int hp_old;

	//ゾンビ側と攻撃を受ける側チーム番号取得
	MyHuman->GetParamData(NULL, &MyHuman_dataID, NULL, &MyHuman_TeamID);
	EnemyHuman->GetParamData(NULL, &EnemyHuman_dataID, NULL, &EnemyHuman_TeamID);

	MyHuman->GetPosData(&mx, &my, &mz, NULL);
	EnemyHuman->GetPosData(&tx, &ty, &tz, NULL);
	ty += VIEW_HEIGHT;

	//敵が攻撃を受けた角度を求める
	arx = atan2f(mz - tz, mx - tx);

	//ロボットかどうか判定
	EnemyHuman->GetParamData(&paramid, NULL, NULL, NULL);
	GameParamInfo->GetHuman(paramid, &Paraminfo);
	if( Paraminfo.type == 1 ){
		NotRobot = false;
	}
	else{
		NotRobot = true;
	}

	//ダメージ計算前のHP取得
	hp_old = EnemyHuman->GetHP();

	//ダメージなどを計算
	if( NetPuppet == false ){ EnemyHuman->HitZombieAttack(); }	//[XOPS-ONLINE]

#ifdef ENABLE_BUG_TEAMID
	bool flag = true;

	//チーム番号が負数、かつチーム番号が大きいなら、フラグ無効
	if( (EnemyHuman_TeamID < 0)&&(MyHuman_TeamID < 0) ){
		if( EnemyHuman_TeamID < MyHuman_TeamID ){
			flag = false;
		}
	}

	if( flag == true ){
		EnemyHuman->SetHitFlag(arx*-1 + (float)M_PI/2);
	}
#else
	EnemyHuman->SetHitFlag(arx*-1 + (float)M_PI/2);
#endif

	//エフェクト（血）を描画
	SetHumanBlood(tx, ty, tz, HUMAN_DAMAGE_ZOMBIEU, NotRobot);

	//効果音を再生
	GameSound->HitHuman(tx, ty, tz, 3, MyHuman_TeamID);

	//ログ関係の処理
	if( (hp_old > 0)&&(EnemyHuman->GetHP() <= 0) ){
		//ダメージ計算前に hp>0 でかつ、計算後に hp <= 0 なら、今回死亡した。

		int player_teamid;
		HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &player_teamid);

		ObjectLog->DiedHuman(MyHuman_dataID, EnemyHuman_dataID, MyHuman_TeamID, EnemyHuman_TeamID, player_teamid);
	}
}

//! @brief 死者を蘇生する
//! @param id 人の番号（0～MAX_HUMAN-1）
//! @return true：成功　false：失敗
//! @warning 手ぶらのまま蘇生します
//! @attention 無効な人番号が指定された場合や、指定した人が生存していた場合、あるいは謎人間に対して実行した場合、この関数は失敗します。
bool ObjectManager::HumanResuscitation(int id)
{
	if( (id < 0)||(MAX_HUMAN <= id) ){ return false; }

	//使用されていないか、生存していれば処理しない。
	if( HumanIndex[id].GetEnableFlag() == false ){ return false; }
	if( HumanIndex[id].GetDeadFlag() == false ){ return false; }

	int id_param, dataid, team;
	signed short int p4;
	HumanIndex[id].GetParamData(&id_param, &dataid, &p4, &team);
	if( (id_param < 0)||( TOTAL_PARAMETERINFO_HUMAN <= id_param) ){ return false; }	//謎人間なら処理しない
	HumanIndex[id].SetParamData(id_param, dataid, p4, team, true);

	//ログ関係の処理
	int player_teamid;
	HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &player_teamid);
	ObjectLog->ReviveHuman(id, team, player_teamid);

	return true;
}

//! @brief ゲームクリアー・ゲームオーバーの判定
//! @return ゲームクリアー：1　ゲームオーバー：2　判定なし：0
//! @attention ゲームクリア―とゲームオーバーが同時に成立する条件では、本家XOPSと同様に「ゲームクリアー」と判定されます。
//! @brief [OpenXOPS-web] Number of staged humans not yet deployed
int ObjectManager::GetStagedCount()
{
	int cnt = 0;
	for(int i=0; i<StagedTotal; i++){
		if( StagedPending[i] == true ){ cnt += 1; }
	}
	return cnt;
}

//! @brief [OpenXOPS-web] Living, enabled humans on a team other than the player's
int ObjectManager::CountLivingHostiles()
{
	int MyTeamid, teamid;
	int cnt = 0;
	HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &MyTeamid);
	for(int i=0; i<MAX_HUMAN; i++){
		if( HumanIndex[i].GetEnableFlag() == false ){ continue; }
		HumanIndex[i].GetParamData(NULL, NULL, NULL, &teamid);
		if( (teamid != MyTeamid)&&(HumanIndex[i].GetHP() > 0) ){ cnt += 1; }
	}
	return cnt;
}

//! @brief [OpenXOPS-web] Bring every staged human of a group into the world
//! @return number of humans deployed
int ObjectManager::DeployStagedGroup(signed short int group)
{
	int cnt = 0;
	for(int i=0; i<StagedTotal; i++){
		if( StagedPending[i] == false ){ continue; }
		if( StagedPoint[i].p4 != group ){ continue; }
		int id = AddHumanIndex(StagedPoint[i], StagedInfo[i], true);
		if( id < 0 ){ continue; } // Keep pending: a failed spawn must not count as a kill.
		StagedPending[i] = false;
		Human_NeedAIInit[id] = true;
		cnt += 1;

		//teleport flash: bright burst plus a rising shimmer (engine effect textures)
		float x = StagedPoint[i].x;
		float y = StagedPoint[i].y;
		float z = StagedPoint[i].z;
		GameSound->Teleport(x, y + 10.0f, z, StagedInfo[i].p3); // [OpenXOPS-web]
		AddEffect(x, y + 10.0f, z, 0.0f, 0.0f, 0.0f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, 24.0f, -0.8f, Resource->GetEffectMflashTexture(), 20, 0, 1.0f, -0.05f, EFFECT_NORMAL, 20);
		AddEffect(x, y + 4.0f, z, 0.0f, 0.12f, 0.0f, 0.0f, DegreeToRadian(10)*GetRand(36), 0.0f, 10.0f, 0.4f, Resource->GetEffectSmokeTexture(), 20, 0, 0.7f, -0.02f, EFFECT_NORMAL, 35);
	}
	return cnt;
}

//! @brief [OpenXOPS-web] Own laser bolt of the player (hidden in first-person view)
bool ObjectManager::IsBulletHiddenInFirstPerson(int id)
{
	if( (id < 0)||(MAX_BULLET <= id) ){ return false; }
	if( (BulletIndex[id].GetEnableFlag() == false)||(Bullet_LaserBolt[id] == false) ){ return false; }
	int shooter = -1;
	BulletIndex[id].GetParamData(NULL, NULL, NULL, NULL, &shooter, NULL);
	return shooter == Player_HumanID;
}

//! @brief [OpenXOPS-web] Read and clear the "AI must re-initialise" flag of a slot
bool ObjectManager::TakeAIInitFlag(int human_id)
{
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return false; }
	bool flag = Human_NeedAIInit[human_id];
	Human_NeedAIInit[human_id] = false;
	return flag;
}

//! @brief [XOPS-ONLINE] Apply one tick of player-style control to a human.
//! @return ShotWeapon() result for this tick (1 bullet, 2 grenade, 0 none)
//! @attention Same calls, in the same order, as the original keyboard path
//!            in maingame::InputPlayer, which now builds a HumanCommand.
int ObjectManager::ApplyHumanCommand(int human_id, const HumanCommand *cmd)
{
	if( (human_id < 0)||(MAX_HUMAN <= human_id)||(cmd == NULL) ){ return 0; }
	human *myHuman = &(HumanIndex[human_id]);
	if( myHuman->GetEnableFlag() == false ){ return 0; }
	if( myHuman->GetHP() <= 0 ){ return 0; }

	int shot = 0;
	myHuman->SetRxRy(cmd->rx, cmd->ry);

	if( cmd->held & HCMD_FORWARD ){ MoveForward(human_id); }
	else if( cmd->held & HCMD_BACK ){ MoveBack(human_id); }
	if( cmd->held & HCMD_LEFT ){ MoveLeft(human_id); }
	else if( cmd->held & HCMD_RIGHT ){ MoveRight(human_id); }
	if( cmd->held & HCMD_WALK ){ MoveWalk(human_id); }
	if( cmd->pressed & HCMD_JUMP ){ MoveJump(human_id); }

	HumanParameter humandata;
	int id_param;
	myHuman->GetParamData(&id_param, NULL, NULL, NULL);
	GameParamInfo->GetHuman(id_param, &humandata);
	bool zombie = (humandata.type == 2);

	//full-auto weapons fire while held; others fire once per press
	bool fire;
	if( (zombie == false)&&(myHuman->GetWeaponBlazingmode() == true) ){ fire = (cmd->held & HCMD_FIRE) != 0; }
	else{ fire = (cmd->pressed & HCMD_FIRE) != 0; }
	if( fire ){
		if( zombie == false ){
			shot = ShotWeapon(human_id);
		}
		else{
			for(int i=0; i<MAX_HUMAN; i++){
				human *EnemyHuman = &(HumanIndex[i]);
				if( CheckZombieAttack(myHuman, EnemyHuman) == true ){
					HitZombieAttack(myHuman, EnemyHuman);
				}
			}
		}
	}

	if( cmd->pressed & HCMD_RELOAD ){ ReloadWeapon(human_id); }
	if( cmd->pressed & HCMD_SWITCH ){ ChangeHaveWeapon(human_id, -1); }
	if( cmd->pressed & HCMD_WEAPON1 ){ ChangeHaveWeapon(human_id, 0); }
	if( cmd->pressed & HCMD_WEAPON2 ){ ChangeHaveWeapon(human_id, 1); }
	if( cmd->pressed & HCMD_SHOTMODE ){ ChangeWeaponID(human_id); }
	if( cmd->pressed & HCMD_DROP ){ DumpWeapon(human_id); }
	if( cmd->pressed & HCMD_SCOPE ){ ChangeScopeMode(human_id); }
	return shot;
}

void ObjectManager::SetNetControlled(int human_id, bool flag)
{
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }
	Human_NetControlled[human_id] = flag;
}

bool ObjectManager::GetNetControlled(int human_id)
{
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return false; }
	return Human_NetControlled[human_id];
}

void ObjectManager::SetNetPuppet(bool flag){ NetPuppet = flag; }
bool ObjectManager::GetNetPuppet(){ return NetPuppet; }

//! @brief [XOPS-ONLINE] AddHumanIndex into exactly this slot (clients mirror host slot ids)
//! @return slot id, or -1 if the slot is in use or the add failed
int ObjectManager::AddHumanIndexAt(int slot, float px, float py, float pz, float rx, int CharacterID, int TeamID, int WeaponID[], signed short int param4)
{
	if( (slot < 0)||(MAX_HUMAN <= slot) ){ return -1; }
	ForceHumanSlot = slot;
	int id = AddHumanIndex(px, py, pz, rx, CharacterID, TeamID, WeaponID, param4, false);
	ForceHumanSlot = -1;
	return id;
}

//! @brief [XOPS-ONLINE] Take a human out of the world, releasing the weapons it holds
void ObjectManager::RemoveHuman(int human_id)
{
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return; }
	human *h = &(HumanIndex[human_id]);
	if( h->GetEnableFlag() == true ){
		int selectweapon;
		weapon *held[TOTAL_HAVEWEAPON];
		for(int i=0; i<TOTAL_HAVEWEAPON; i++){ held[i] = NULL; }
		h->GetWeapon(&selectweapon, held, NULL, NULL);
		for(int i=0; i<TOTAL_HAVEWEAPON; i++){
			if( held[i] != NULL ){ held[i]->SetEnableFlag(false); }
		}
	}
	h->SetEnableFlag(false);
	Human_NetMove[human_id] = 0;
	Human_FrameTextureRefresh[human_id] = false;
}

//! @brief [XOPS-ONLINE] Bring a human back in the same slot with full HP and new weapons
int ObjectManager::RespawnHuman(int human_id, float px, float py, float pz, float rx, int WeaponID[])
{
	if( (human_id < 0)||(MAX_HUMAN <= human_id) ){ return -1; }
	int param, team;
	signed short int p4;
	HumanIndex[human_id].GetParamData(&param, NULL, &p4, &team);
	RemoveHuman(human_id);
	return AddHumanIndexAt(human_id, px, py, pz, rx, param, team, WeaponID, p4);
}

//! @brief [XOPS-ONLINE] Read the replicated state of one human (host side)
bool ObjectManager::GetNetHumanState(int human_id, NetHumanState *out)
{
	if( (human_id < 0)||(MAX_HUMAN <= human_id)||(out == NULL) ){ return false; }
	human *h = &(HumanIndex[human_id]);
	if( h->GetEnableFlag() == false ){ return false; }
	out->id = human_id;
	h->GetPosData(&out->x, &out->y, &out->z, NULL);
	h->GetRxRy(&out->rx, &out->ry);
	out->hp = h->GetHP();
	out->flags = 0;
	if( h->GetDeadFlag() == true ){ out->flags |= NETHUMAN_DEAD; }
	if( h->GetScopeMode() != 0 ){ out->flags |= NETHUMAN_SCOPE; }
	if( h->GetInvincibleFlag() == true ){ out->flags |= NETHUMAN_INVINCIBLE; }
	signed short int p4;
	h->GetParamData(&out->param, NULL, &p4, &out->team);
	int selectweapon;
	weapon *held[TOTAL_HAVEWEAPON];
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){ held[i] = NULL; }
	h->GetWeapon(&selectweapon, held, NULL, NULL);
	out->weapon = NETHUMAN_NOWEAPON;
	if( held[selectweapon] != NULL ){ held[selectweapon]->GetParamData(&out->weapon, NULL, NULL); }
	out->move = h->GetMoveFlag(false);
	return true;
}

//! @brief [XOPS-ONLINE] Apply replicated state to a puppet world (client side)
//! @param keepView true for the local player: its own aim is not overwritten
//! @return 1 if the human was created or respawned in its slot, 0 if updated, -1 on error
int ObjectManager::ApplyNetHumanState(const NetHumanState *in, bool keepView)
{
	if( (in == NULL)||(in->id < 0)||(MAX_HUMAN <= in->id) ){ return -1; }
	human *h = &(HumanIndex[in->id]);
	int created = 0;
	bool alive = (in->hp > 0)&&((in->flags & NETHUMAN_DEAD) == 0);

	int param = -1, team = 0;
	if( h->GetEnableFlag() == true ){ h->GetParamData(&param, NULL, NULL, &team); }
	bool respawned = alive && (h->GetEnableFlag() == true) && (h->GetHP() <= 0);
	if( (h->GetEnableFlag() == false)||(param != in->param)||(team != in->team)||respawned ){
		RemoveHuman(in->id);
		int weapons[TOTAL_HAVEWEAPON];
		for(int i=0; i<TOTAL_HAVEWEAPON; i++){ weapons[i] = ID_WEAPON_NONE; }
		if( in->weapon != NETHUMAN_NOWEAPON ){ weapons[0] = in->weapon; }
		if( AddHumanIndexAt(in->id, in->x, in->y, in->z, in->rx, in->param, in->team, weapons, 0) != in->id ){ return -1; }
		created = 1;
	}

	float ox, oy, oz, orx, ory;
	h->GetPosData(&ox, &oy, &oz, &orx);
	h->GetRxRy(&orx, &ory);
	if( keepView == true ){
		h->SetPosData(in->x, in->y, in->z, orx);
		h->SetRxRy(orx, ory);
	}
	else{
		h->SetPosData(in->x, in->y, in->z, in->rx);
		h->SetRxRy(in->rx, in->ry);
	}

	if( alive == false ){
		if( h->GetHP() > 0 ){ h->SetHP(0); }	//the engine plays the death on its next tick
	}
	else{
		h->SetHP(in->hp);
		//keep the held weapon's model in step with the host
		int selectweapon;
		weapon *held[TOTAL_HAVEWEAPON];
		for(int i=0; i<TOTAL_HAVEWEAPON; i++){ held[i] = NULL; }
		h->GetWeapon(&selectweapon, held, NULL, NULL);
		int current = NETHUMAN_NOWEAPON;
		if( held[selectweapon] != NULL ){ held[selectweapon]->GetParamData(&current, NULL, NULL); }
		if( (in->weapon != current)&&(in->weapon != NETHUMAN_NOWEAPON) ){ CheatNewWeapon(in->id, in->weapon); }
	}
	h->SetInvincibleFlag( (in->flags & NETHUMAN_INVINCIBLE) != 0 );
	Human_NetMove[in->id] = alive ? in->move : 0;
	return created;
}

//! @brief [XOPS-ONLINE] Puppet worlds: keep replicated humans walking between snapshots
void ObjectManager::ApplyNetPuppetMoves()
{
	for(int i=0; i<MAX_HUMAN; i++){
		int m = Human_NetMove[i];
		if( (m == 0)||(HumanIndex[i].GetEnableFlag() == false) ){ continue; }
		if( m & MOVEFLAG_FORWARD ){ HumanIndex[i].SetMoveForward(); }
		if( m & MOVEFLAG_BACK ){ HumanIndex[i].SetMoveBack(); }
		if( m & MOVEFLAG_LEFT ){ HumanIndex[i].SetMoveLeft(); }
		if( m & MOVEFLAG_RIGHT ){ HumanIndex[i].SetMoveRight(); }
		if( m & MOVEFLAG_WALK ){ HumanIndex[i].SetMoveWalk(); }
	}
}

int ObjectManager::CheckGameOverorComplete()
{
	//メモ：
	//
	//　本来は、ゲームオーバー判定を先に行い、次にゲームクリアー判定を実装した方が効率的です。
	//　しかし、本家XOPSはゲームクリアーの判定を優先するため、先にゲームクリアーの判定を行っています。
	//　生きている敵の数を（総HPとして）数え、敵の数（総HP）が 0 ならば、ゲームクリアーと判定します。

	//ゲームクリアー判定
	int MyTeamid, teamid;
	int TotalEnemyHP = 0;
	HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &MyTeamid);	//プレイヤーのチーム番号を取得
	for(int i=0; i<MAX_HUMAN; i++){
		//初期化されていなければ処理しない
		if( HumanIndex[i].GetEnableFlag() == false ){ continue; }

		//調べる対象のチーム番号を取得
		HumanIndex[i].GetParamData(NULL, NULL, NULL, &teamid);

		//異なるチーム番号（＝敵）ならば
		if( teamid != MyTeamid ){
			//生きていれば、敵のHPとして加算
			//if( HumanIndex[i].GetDeadFlag() == false ){
			if( HumanIndex[i].GetHP() > 0 ){
				TotalEnemyHP += HumanIndex[i].GetHP();
			}
		}
	}
	//[OpenXOPS-web] staged (not yet deployed) waves keep the mission running
	if( (TotalEnemyHP == 0)&&(GetStagedCount() == 0) ){	//全敵のHPが 0 ならば
		return 1;
	}


	//ゲームオーバー判定
	if( HumanIndex[Player_HumanID].GetEnableFlag() == true ){	//操作対象が有効ならば　（注：裏技による変更対策）
		if( HumanIndex[Player_HumanID].GetDeadFlag() == true ){		//プレイヤーが死亡していれば
			return 2;
		}
	}

	return 0;
}

//! @brief オブジェクトの情報を取得
//! @param camera_x カメラのX座標
//! @param camera_y カメラのY座標
//! @param camera_z カメラのZ座標
//! @param camera_rx カメラの横軸角度
//! @param camera_ry カメラの縦軸角度
//! @param color 色を取得するポインタ
//! @param infostr 文字を取得するポインタ
//! @return 表示情報あり：true　表示情報なし：false
bool ObjectManager::GetObjectInfoTag(float camera_x, float camera_y, float camera_z, float camera_rx, float camera_ry, int *color, char *infostr)
{
	if( color == NULL ){ return false; }
	if( infostr == NULL ){ return false; }

	float dist = 50.0f;
	float px, py, pz;
	float rx, ry;
	float r;
	int Player_teamID;

	//文字を初期化
	infostr[0] = '\0';

	//プレイヤーのチーム番号を取得
	HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &Player_teamID);

	//人
	for(int i=0; i<MAX_HUMAN; i++){
		//プレイヤー自身なら処理しない
		if( i == Player_HumanID ){ continue; }

		if( HumanIndex[i].GetEnableFlag() == true ){
			HumanIndex[i].GetPosData(&px, &py, &pz, NULL);

			//視点を基準に対象までの角度を算出
			if( CheckTargetAngle(camera_x, camera_y, camera_z, camera_rx, 0.0f, px, py, pz, dist, &rx, NULL, &r) == true ){
				int team;

				//角度上、視界に入っていれば
				if( fabsf(rx) < DegreeToRadian(10) ){
					HumanIndex[i].GetParamData(NULL, NULL, NULL, &team);
					if( team == Player_teamID ){
						*color = d3dg->GetColorCode(0.0f,0.0f,1.0f,1.0f);
						sprintf(infostr, "Human[%d]  HP %d : Friend", i, HumanIndex[i].GetHP());
					}
					else{
						*color = d3dg->GetColorCode(1.0f,0.0f,0.0f,1.0f);
						sprintf(infostr, "Human[%d]  HP %d : Enemy", i, HumanIndex[i].GetHP());
					}
					dist = sqrtf(r);
				}
			}
		}
	}

	//武器
	for(int i=0; i<MAX_WEAPON; i++){
		if( (WeaponIndex[i].GetEnableFlag() == true)&&(WeaponIndex[i].GetUsingFlag() == false) ){
			int lnbs, nbs;

			WeaponIndex[i].GetPosData(&px, &py, &pz, NULL);
			WeaponIndex[i].GetParamData(NULL, &lnbs, &nbs);

			//視点を基準に対象までの角度を算出
			if( CheckTargetAngle(camera_x, camera_y, camera_z, camera_rx, camera_ry, px, py, pz, dist, &rx, &ry, &r) == true ){
				//角度上、視界に入っていれば
				if( (fabsf(rx) < DegreeToRadian(10))&&(fabsf(ry) < DegreeToRadian(10)) ){
					*color = d3dg->GetColorCode(0.0f,1.0f,0.0f,1.0f);
					sprintf(infostr, "Weapon[%d]  %d:%d", i, lnbs, (nbs - lnbs));
					dist = sqrtf(r);
				}
			}
		}
	}

	//小物
	for(int i=0; i<MAX_SMALLOBJECT; i++){
		if( SmallObjectIndex[i].GetEnableFlag() == true ){
			SmallObjectIndex[i].GetPosData(&px, &py, &pz, NULL);
			
			//視点を基準に対象までの角度を算出
			if( CheckTargetAngle(camera_x, camera_y, camera_z, camera_rx, camera_ry, px, py, pz, dist, &rx, &ry, &r) == true ){
				//角度上、視界に入っていれば
				if( (fabsf(rx) < DegreeToRadian(10))&&(fabsf(ry) < DegreeToRadian(10)) ){
					*color = d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f);
					sprintf(infostr, "SmallObject[%d]  HP %d", i, SmallObjectIndex[i].GetHP());
					dist = sqrtf(r);
				}
			}
		}
	}

	//文字が設定されているかどうかで、有効な処理がされたか判定。
	if( infostr[0] == NULL ){
		return false;
	}
	return true;
}

//! @brief オブジェクトの主計算処理
//! @param cmdF5id 上昇機能（F5裏技）させる人データ番号（-1で機能無効）
//! @param demomode デモモード
//! @param screen 画面を暗くする
//! @return 常に 0
//! @attention 一般的に cmdF5id は、F5裏技使用中はプレイヤー番号（GetPlayerID()関数で取得）、未使用時は -1 を指定します。
//! @attention demomode は主にメニュー画面で使用します。有効にすると、銃弾・手榴弾を処理しません。
int ObjectManager::Process(int cmdF5id, bool demomode, bool screen)
{
	//このフレームの戦歴を初期化
	for(int i=0; i<MAX_HUMAN; i++){
		Human_ontarget[i] = 0.0f;
		Human_kill[i] = 0;
		Human_headshot[i] = 0;
	}

	//人オブジェクトの処理
	for(int i=0; i<MAX_HUMAN; i++){
		bool cmdF5, player;
		int rtn;
		int block_id = -1;
		int block_face = -1;
		int block_textureid = -1;
		int MoveMode;
		int JumpLandingMode;

		//プレイヤーかどうか判定
		if( Player_HumanID == i ){
			player = true;
		}
		else{
			player = false;
		}

		if( i == cmdF5id ){
			cmdF5 = true;
		}
		else{
			cmdF5 = false;
		}

		rtn = HumanIndex[i].ProcessObject(CollD, BlockData, AddCollisionFlag, player, cmdF5);
		if( rtn == 2 ){
			//死亡時のエフェクト
			DeadEffect(&(HumanIndex[i]));
		}
		if( rtn == 4 ){
			int teamid, player_teamid;
			HumanIndex[i].GetParamData(NULL, NULL, NULL, &teamid);
			HumanIndex[Player_HumanID].GetParamData(NULL, NULL, NULL, &player_teamid);

			//ログ関係の処理
			ObjectLog->DiedHuman(-1, i, -1, teamid, player_teamid);
		}

		HumanIndex[i].SetDarkModelFlag(screen);

		//足元にあるブロックのテクスチャー番号を取得する
		HumanIndex[i].GetUnderBlock(&block_id, &block_face);
		if( block_id != -1 ){
			blockdata bdata;
			BlockData->Getdata(&bdata, block_id);
			block_textureid = bdata.material[block_face].textureID;
		}
		else{
			block_textureid = -1;
		}

		MoveMode = HumanIndex[i].GetMovemode(false);
		JumpLandingMode = HumanIndex[i].GetJumpLanding(false);

		//足音
		if( (1 <= MoveMode)&&(MoveMode <= 4) ){
			//足音追加
			//　※足元にあるブロックのテクスチャー番号を、足音の種類番号とする。
			float posx, posy, posz;
			int teamid;
			int cnt;
			HumanIndex[i].GetPosData(&posx, &posy, &posz, NULL);
			HumanIndex[i].GetParamData(NULL, NULL, NULL, &teamid);
			cnt = HumanIndex[i].GetMoveMotionCount();
			GameSound->SetFootsteps(posx, posy, posz, block_textureid, MoveMode - 1, teamid, cnt);
		}

		//ジャンプする音
		if( JumpLandingMode == 1 ){
			//足音追加
			float posx, posy, posz;
			int teamid;
			HumanIndex[i].GetPosData(&posx, &posy, &posz, NULL);
			HumanIndex[i].GetParamData(NULL, NULL, NULL, &teamid);
			GameSound->SetFootsteps(posx, posy, posz, block_textureid, 4, teamid, 0);
		}

		//着地する音
		if( JumpLandingMode == 2 ){
			//足音追加
			float posx, posy, posz;
			int teamid;
			HumanIndex[i].GetPosData(&posx, &posy, &posz, NULL);
			HumanIndex[i].GetParamData(NULL, NULL, NULL, &teamid);
			GameSound->SetFootsteps(posx, posy, posz, block_textureid, 5, teamid, 0);
		}
	}

	//武器オブジェクトの処理
	for(int i=0; i<MAX_WEAPON; i++){
		WeaponIndex[i].ProcessObject(CollD);
		WeaponIndex[i].SetDarkModelFlag(screen);
	}

	//小物オブジェクトの処理
	for(int i=0; i<MAX_SMALLOBJECT; i++){
		SmallObjectIndex[i].ProcessObject();
		SmallObjectIndex[i].SetDarkModelFlag(screen);
	}

	if( demomode == false ){
		//弾オブジェクトの処理
		for(int i=0; i<MAX_BULLET; i++){
			float bx, by, bz, brx, bry, speed;
			int teamid;
			float mx, my, mz;

			CollideBullet(&BulletIndex[i]);		//当たり判定を実行
			BulletIndex[i].ProcessObject();		//主計算
			BulletIndex[i].SetDarkModelFlag(screen);

			if( BulletIndex[i].GetEnableFlag() == true ){
				//弾の座標と角度を取得
				BulletIndex[i].GetParamData(NULL, NULL, &speed, &teamid, NULL, NULL);
				BulletIndex[i].GetPosData(&bx, &by, &bz, &brx, &bry);
				mx = cosf(brx)*cosf(bry)*speed;
				my = sinf(bry)*speed;
				mz = sinf(brx)*cosf(bry)*speed;
				GameSound->PassingBullet(bx, by, bz, mx, my, mz, teamid);
			}
		}
	}
	else{
		//全ての弾オブジェクトを消す
		for(int i=0; i<MAX_BULLET; i++){
			if( BulletIndex[i].GetEnableFlag() == true ){
				BulletIndex[i].SetEnableFlag(false);
			}
		}
	}

	//武器を発砲した際のエフェクトを出す
	for(int i=0; i<MAX_HUMAN; i++){
		if( Human_ShotFlag[i] == true ){
			ShotWeaponEffect(i);
			Human_ShotFlag[i] = false;
		}

		if( Human_YakkyouCnt[i] == 0 ){
			ShotWeaponYakkyou(i);
		}
		if( Human_YakkyouCnt[i] > -1 ){ Human_YakkyouCnt[i] -= 1; }
	}

	//エフェクトオブジェクトの処理
	for(int i=0; i<MAX_EFFECT; i++){
		int id, face;
		float pos_x, pos_y, pos_z;
		if( CollideBlood(&(EffectIndex[i]), &id, &face, &pos_x, &pos_y, &pos_z) == true ){
			AddMapEffect(id, face, pos_x, pos_y, pos_z, 10.0f, DegreeToRadian(10)*GetRand(18), Resource->GetEffectBloodTexture(),  0, 0, 1.0f, -0.002f, (int)GAMEFPS * 20);
			EffectIndex[i].SetEnableFlag(false);
		}
		else{
			EffectIndex[i].ProcessObject();
			EffectIndex[i].SetDarkModelFlag(screen);
		}
	}

	if( demomode == false ){
		//手榴弾の処理
		for(int i=0; i<MAX_GRENADE; i++){
			float speed = GrenadeIndex[i].GetSpeed();

			//主計算
			int rcr = GrenadeIndex[i].ProcessObject(CollD);
			GrenadeIndex[i].SetDarkModelFlag(screen);

			//バウンド・跳ね返ったならば
			if( rcr == 1 ){
				if( speed > 3.4f ){
					//座標を取得し、効果音再生
					float x, y, z;
					int teamid;
					GrenadeIndex[i].GetPosData(&x, &y, &z, NULL, NULL);
					GrenadeIndex[i].GetParamData(NULL, &teamid, NULL, NULL);
					GameSound->GrenadeBound(x, y, z, teamid);
				}
			}

			//爆発したなら
			if( rcr == 2 ){
				GrenadeExplosion(&(GrenadeIndex[i]));
			}
		}
	}
	else{
		//全ての手榴弾を消す
		for(int i=0; i<MAX_GRENADE; i++){
			if( GrenadeIndex[i].GetEnableFlag() == true ){
				GrenadeIndex[i].SetEnableFlag(false);
			}
		}
	}


	//武器を拾う処理
	for(int i=0; i<MAX_HUMAN; i++){
		if( HumanIndex[i].GetEnableFlag() == true ){
			//武器を拾うだけで毎フレーム総当たりで処理する意味はないので、各武器2フレームに1回処理にケチってます　（＾＾；
			//フレーム数が偶数目の時は、データ番号が偶数の武器。奇数の時は、奇数の武器を処理。
			for(int j=(framecnt%2); j<MAX_WEAPON; j+=2){
				PickupWeapon(&HumanIndex[i], &WeaponIndex[j]);
			}
		}
	}

	for(int i=0; i<MAX_HUMAN; i++){
		if( HumanIndex[i].GetEnableFlag() == true ){
			for(int j=i+1; j<MAX_HUMAN; j++){
				if( HumanIndex[j].GetEnableFlag() == true ){

					//人同士の当たり判定（押し合い）
					CollideHuman(&HumanIndex[i], &HumanIndex[j]);

				}
			}
		}
	}

	//ログ関係の処理
	ObjectLog->Process();

	framecnt += 1;

	return 0;
}

//! @brief 現フレームの 命中率・倒した敵の数・ヘットショット数 の取得
//! @param id 取得する人のデータ番号
//! @param ontarget 命中数を受け取るポインタ
//! @param kill 倒した敵の数を受け取るポインタ
//! @param headshot 敵の頭部に命中した数を受け取るポインタ
//! @return 成功：true　失敗：false
bool ObjectManager::GetHumanShotInfo(int id, float *ontarget, int *kill, int *headshot)
{
	if( (id < 0)||(MAX_HUMAN <= id) ){ return false; }
	if( (ontarget == NULL)||(kill == NULL)||(headshot == NULL) ){ return false; }

	*ontarget = Human_ontarget[id];
	*kill = Human_kill[id];
	*headshot = Human_headshot[id];
	return true;
}

//! @brief エフェクトをソートする
//! @param camera_x カメラのX座標
//! @param camera_y カメラのY座標
//! @param camera_z カメラのZ座標
//! @param data 結果を受け取る effectdata構造体　（要素数：MAX_EFFECT）
//! @return 有効なエフェクトの数
int ObjectManager::SortEffect(float camera_x, float camera_y, float camera_z, effectdata data[])
{
	int cnt = 0;

	//カメラからの距離を求めつつ、総数を数える。
	for(int i=0; i<MAX_EFFECT; i++){
		if( EffectIndex[i].GetEnableFlag() == true ){
			float ex, ey, ez;
			float x, y, z;

			//座標を取得し、距離を計算、データに登録。
			EffectIndex[i].GetPosData(&ex, &ey, &ez, NULL);
			x = ex - camera_x;
			y = ey - camera_y;
			z = ez - camera_z;
			data[cnt].id = i;
			data[cnt].dist = x*x + y*y + z*z;

			//カウントを加算
			cnt += 1;
		}
	}

	//単純挿入ソート
	effectdata temp;
	for(int i=1; i<cnt; i++) {
		int j;
		temp = data[i];
		for(j=i; j>0 && data[j-1].dist<temp.dist; j--){
			data[j] = data[j-1];
		}
		data[j] = temp;
	}

	return cnt;
}

//! @brief オブジェクトの描画処理
//! @param camera_x カメラのX座標
//! @param camera_y カメラのY座標
//! @param camera_z カメラのZ座標
//! @param camera_rx カメラの横軸角度
//! @param camera_ry カメラの縦軸角度
//! @param HidePlayer プレイヤーの描画設定　（描画する：0　描画しない：1　腕と武器のみ描画する：2）
//! @param NoModel モデル描画無効化
void ObjectManager::Render(float camera_x, float camera_y, float camera_z, float camera_rx, float camera_ry, int HidePlayer, bool NoModel)
{
	//ワールド座標を原点へ
	d3dg->ResetWorldTransform();

	//人描画
	for(int i=0; i<MAX_HUMAN; i++){
		bool DrawArm, player;

		//人のテクスチャリフレッシュフラグが有効なら
		if( Human_FrameTextureRefresh[i] == true ){
			if( i > 0 ){
				//前の人が持っている武器を取得
				int WeaponType = HumanIndex[i-1].GetMainWeaponTypeNO();

				if( WeaponType != ID_WEAPON_NONE ){
					int model, texture;

					//武器を持っていれば、その武器のテクスチャを適用
					Resource->GetWeaponModelTexture(WeaponType, &model, &texture);
					HumanIndex[i].SetTexture(texture);
				}
				else{
					//手ぶらなら、人のテクスチャをそのまま反映
					HumanIndex[i].SetTexture( HumanIndex[i-1].GetTexture() );
				}
			}
		}

		//腕の描画
		if( HidePlayer == 0 ){
			DrawArm = false;
		}
		else if( Player_HumanID != i ){	// HidePlayer != 0
			DrawArm = false;
		}
		else if( HidePlayer == 2 ){		// Player_HumanID == i
			DrawArm = true;
		}
		else{							// Player_HumanID == i && HidePlayer == 1
			continue;	//プレイヤー自身を描画しないならスキップ
		}

		//プレイヤーかどうか判定
		if( Player_HumanID == i ){
			player = true;
		}
		else{
			player = false;
		}

		HumanIndex[i].Render(d3dg, Resource, DrawArm, player, NoModel);

		/*
		//当たり判定の簡易表示
		float x, y, z;
		HumanIndex[i].GetPosData(&x, &y, &z, NULL);
		if( HumanIndex[i].GetHP() > 0 ){
			d3dg->SetWorldTransform(x, y, z, 0.0f, 0.0f, 1.0f);
			d3dg->Renderline(HUMAN_BULLETCOLLISION_HEAD_R, HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H + HUMAN_BULLETCOLLISION_HEAD_H, 0.0f, HUMAN_BULLETCOLLISION_HEAD_R * -1, HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H + HUMAN_BULLETCOLLISION_HEAD_H, 0.0f);
			d3dg->Renderline(0.0f, HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H + HUMAN_BULLETCOLLISION_HEAD_H, HUMAN_BULLETCOLLISION_HEAD_R, 0.0f, HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H + HUMAN_BULLETCOLLISION_HEAD_H, HUMAN_BULLETCOLLISION_HEAD_R * -1);
			d3dg->Renderline(HUMAN_BULLETCOLLISION_UP_R, HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H, 0.0f, HUMAN_BULLETCOLLISION_UP_R * -1, HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H, 0.0f);
			d3dg->Renderline(0.0f, HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H, HUMAN_BULLETCOLLISION_UP_R, 0.0f, HUMAN_BULLETCOLLISION_LEG_H + HUMAN_BULLETCOLLISION_UP_H, HUMAN_BULLETCOLLISION_UP_R * -1);
			d3dg->Renderline(HUMAN_BULLETCOLLISION_LEG_R, HUMAN_BULLETCOLLISION_LEG_H, 0.0f, HUMAN_BULLETCOLLISION_LEG_R * -1, HUMAN_BULLETCOLLISION_LEG_H, 0.0f);
			d3dg->Renderline(0.0f, HUMAN_BULLETCOLLISION_LEG_H, HUMAN_BULLETCOLLISION_LEG_R, 0.0f, HUMAN_BULLETCOLLISION_LEG_H, HUMAN_BULLETCOLLISION_LEG_R * -1);
		}
		*/
	}

	//武器描画
	for(int i=0; i<MAX_WEAPON; i++){
		WeaponIndex[i].Render(d3dg, NoModel);
	}

	//小物描画
	for(int i=0; i<MAX_SMALLOBJECT; i++){
		SmallObjectIndex[i].Render(d3dg, NoModel);
	}

	//弾描画
	for(int i=0; i<MAX_BULLET; i++){
		//[OpenXOPS-web] The player's own laser flies down the line of sight and is
		//seen end-on as a blob over the crosshair; skip it in first person.
		if( (HidePlayer != 0)&&(IsBulletHiddenInFirstPerson(i) == true) ){ continue; }
		BulletIndex[i].Render(d3dg, NoModel);
	}

	//手榴弾描画
	for(int i=0; i<MAX_GRENADE; i++){
		GrenadeIndex[i].Render(d3dg, NoModel);
	}

	//エフェクト描画
	d3dg->StartEffectRender();
	/*
	for(int i=0; i<MAX_EFFECT; i++){
		EffectIndex[i].Render(d3dg, camera_rx, camera_ry, NoModel);
	}
	*/
	effectdata data[MAX_EFFECT];
	int cnt = SortEffect(camera_x, camera_y, camera_z, data);
	for(int i=0; i<cnt; i++) {
		EffectIndex[ data[i].id ].Render(d3dg, camera_rx, camera_ry, NoModel);
	}
	d3dg->EndEffectRender();
}

//! @brief オブジェクトログの描画処理
//! @param x 左上の x座標
//! @param y 左上の y座標
void ObjectManager::RenderLog(int x, int y)
{
	ObjectLog->Render(x, y);
}

//! @brief データの解放
void ObjectManager::Cleanup()
{
	//ポイントデータ解放
	CleanupPointDataToObject();
}

//! @brief コンストラクタ
BulletObjectHumanIndex::BulletObjectHumanIndex()
{
	HumanIndex = new bool[MAX_HUMAN];

	Init();
}

//! @brief ディストラクタ
BulletObjectHumanIndex::~BulletObjectHumanIndex()
{
	if( HumanIndex != NULL ){ delete [] HumanIndex; }
}

//! @brief 初期化
void BulletObjectHumanIndex::Init()
{
	for(int i=0; i<MAX_HUMAN; i++){
		HumanIndex[i] = false;
	}
}

//! @brief フラグを取得
//! @param id 人のデータ番号
//! @return フラグ値
bool BulletObjectHumanIndex::GetIndexFlag(int id)
{
	if( (id < 0)||(MAX_HUMAN <= id) ){ return false; }
	return HumanIndex[id];
}

//! @brief フラグを有効化
//! @param id 人のデータ番号
void BulletObjectHumanIndex::SetIndexFlag(int id)
{
	if( (id < 0)||(MAX_HUMAN <= id) ){ return; }
	HumanIndex[id] = true;
}

//! @brief コンストラクタ
ObjectManagerLog::ObjectManagerLog()
{
	d3dg = NULL;
	for(int i=0; i<MAX_OBJECTMANAGER_LOGLEN; i++){
		TextStr[i] = new char [MAX_OBJECTMANAGER_LOGLINES];
	}
	ClearLog();
}

//! @brief ディストラクタ
ObjectManagerLog::~ObjectManagerLog()
{
	for(int i=0; i<MAX_OBJECTMANAGER_LOGLEN; i++){
		delete [] TextStr[i];
	}
}

//! @brief 参照するクラスを設定
//! @param in_d3dg 描画処理クラス
//! @attention この関数で設定を行わないと、クラス自体が正しく機能しません。
void ObjectManagerLog::SetClass(D3DGraphics *in_d3dg)
{
	d3dg = in_d3dg;
}

//! @brief オブジェクトログを初期化
void ObjectManagerLog::ClearLog()
{
	for(int i=0; i<MAX_OBJECTMANAGER_LOGLEN; i++){
		TextCnt[i] = -1;
		TextStr[i][0] = '\0';
		TextColor[i] = 0;
	}
}

//! @brief 情報を追記
//! @param str 文字列　（改行コード：<b>不可</b>）
void ObjectManagerLog::InfoLog(const char *str)
{
	if( str == NULL ){ return; }
	AddTextLog(MAX_OBJECTMANAGER_LOGCNT, str, d3dg->GetColorCode(0.0f,1.0f,0.0f,1.0f));
}

//! @brief 人を追加した
//! @param humanID 人のID
//! @param TeamID チーム番号
//! @param PlayerTeamID プレイヤーのチーム番号
void ObjectManagerLog::AddHuman(int humanID, int TeamID, int PlayerTeamID)
{
	char str[64];
	int str_color;

	//文字の色を決定
	if( PlayerTeamID == TeamID ){ str_color = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f); }
	else{ str_color = d3dg->GetColorCode(1.0f,0.5f,0.0f,1.0f); }

	sprintf(str, "Added human[%d]", humanID);

	AddTextLog(MAX_OBJECTMANAGER_LOGCNT, str, str_color);
}

//! @brief 人が死亡した
//! @param ShothumanID  攻撃をした人のID
//! @param HitHumanID 攻撃を受けた人のID
//! @param ShothumanTeamID 攻撃をした人のチーム番号
//! @param HitHumanTeamID 攻撃を受けた人のチーム番号
//! @param PlayerTeamID プレイヤーのチーム番号
void ObjectManagerLog::DiedHuman(int ShothumanID, int HitHumanID, int ShothumanTeamID, int HitHumanTeamID, int PlayerTeamID)
{
	char str[64];
	int str_color;

	//文字の色を決定
	if( PlayerTeamID == HitHumanTeamID ){ str_color = d3dg->GetColorCode(1.0f,0.5f,0.0f,1.0f); }
	else{ str_color = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f); }

	if( (ShothumanID == -1)||(ShothumanID == HitHumanID) ){
		sprintf(str, "human[%d] died", HitHumanID);
	}
	else{
		sprintf(str, "human[%d] killed human[%d]", ShothumanID, HitHumanID);

		//TK判定
		if( ShothumanTeamID == HitHumanTeamID ){
			strcat(str, " (TK)");
		}
	}

	AddTextLog(MAX_OBJECTMANAGER_LOGCNT, str, str_color);
}

//! @brief 人が蘇生した
//! @param humanID 人のID
//! @param TeamID チーム番号
//! @param PlayerTeamID プレイヤーのチーム番号
void ObjectManagerLog::ReviveHuman(int humanID, int TeamID, int PlayerTeamID)
{
	char str[64];
	int str_color;

	//文字の色を決定
	if( PlayerTeamID == TeamID ){ str_color = d3dg->GetColorCode(0.0f,1.0f,1.0f,1.0f); }
	else{ str_color = d3dg->GetColorCode(1.0f,0.5f,0.0f,1.0f); }

	sprintf(str, "Revived human[%d]", humanID);

	AddTextLog(MAX_OBJECTMANAGER_LOGCNT, str, str_color);
}

//! @brief 小物を追加した
//! @param objID 小物のID
void ObjectManagerLog::AddSmallObject(int objID)
{
	char str[64];
	sprintf(str, "Added SmallObject[%d]", objID);
	AddTextLog(MAX_OBJECTMANAGER_LOGCNT, str, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
}

//! @brief 小物を破壊した
//! @param objID 小物のID
void ObjectManagerLog::BreakSmallObject(int objID)
{
	char str[64];
	sprintf(str, "Breaked SmallObject[%d]", objID);
	AddTextLog(MAX_OBJECTMANAGER_LOGCNT, str, d3dg->GetColorCode(1.0f,1.0f,0.0f,1.0f));
}

//! @brief オブジェクトログ追加
//! @param cnt 描画フレーム数
//! @param str 文字列　（改行コード：<b>不可</b>）
//! @param color 色
//! @return 1行上書き：true　追加のみ：false
bool ObjectManagerLog::AddTextLog(int cnt, const char *str, int color)
{
	if( str == NULL ){ return false; }

	//空いている行があるなら、その行に書いて終了
	for(int i=0; i<MAX_OBJECTMANAGER_LOGLEN; i++){
		if( TextCnt[i] == -1 ){
			TextCnt[i] = cnt;
			strcpy(TextStr[i], str);
			TextColor[i] = color;
			return false;
		}
	}

	//空いている行がなければ、1行づつ詰める
	for(int i=0; i<MAX_OBJECTMANAGER_LOGLEN-1; i++){
		TextCnt[i] = TextCnt[i+1];
		strcpy(TextStr[i], TextStr[i+1]);
		TextColor[i] = TextColor[i+1];
	}

	//最後の行に書く
	TextCnt[MAX_OBJECTMANAGER_LOGLEN-1] = cnt;
	strcpy(TextStr[MAX_OBJECTMANAGER_LOGLEN-1], str);
	TextColor[MAX_OBJECTMANAGER_LOGLEN-1] = color;
	return true;
}

//! @brief オブジェクトログの主計算処理
void ObjectManagerLog::Process()
{
	//各行ごとにカウント処理
	for(int i=0; i<MAX_OBJECTMANAGER_LOGLEN; i++){
		if( TextCnt[i] == 0 ){
			TextCnt[i] = -1;
			TextStr[i][0] = '\0';
			TextColor[i] = 0;
		}
		else if( TextCnt[i] > 0 ){
			TextCnt[i] -= 1;
		}
	}

	//ゼロカウントの行を詰める
	for(int i=0; i<MAX_OBJECTMANAGER_LOGLEN-1; i++){
		if( TextCnt[i] == 0 ){
			for(int j=i; j<MAX_OBJECTMANAGER_LOGLEN-1; j++){
				TextCnt[j] = TextCnt[j+1];
				strcpy(TextStr[j], TextStr[j+1]);
				TextColor[j] = TextColor[j+1];
			}

			//最後の行は空行に
			TextCnt[MAX_OBJECTMANAGER_LOGLEN-1] = -1;
			TextStr[MAX_OBJECTMANAGER_LOGLEN-1][0] = '\0';
			TextColor[MAX_OBJECTMANAGER_LOGLEN-1] = 0;
		}
	}
}

//! @brief オブジェクトログの描画処理
//! @param x 左上の x座標
//! @param y 左上の y座標
//! @attention 上から逆順で描画されます。
void ObjectManagerLog::Render(int x, int y)
{
#ifdef ENABLE_DEBUGCONSOLE
	if( d3dg == NULL ){ return; }

	for(int i=MAX_OBJECTMANAGER_LOGLEN-1; i>=0; i--){
		if( TextCnt[i] >= 0 ){
			d3dg->Draw2DTextureDebugFontText(x+2+1, y+2+1, TextStr[i], d3dg->GetColorCode(0.1f,0.1f,0.1f,1.0f));
			d3dg->Draw2DTextureDebugFontText(x+2, y+2, TextStr[i], TextColor[i]);
		}
		y += 20;
	}
#endif
}