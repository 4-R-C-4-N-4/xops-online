//! @file object.cpp
//! @brief objectクラスの定義

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

#include "object.h"

//! @brief コンストラクタ
object::object(class ParameterInfo *in_Param, float x, float y, float z, float rx, float size, bool flag)
{
	Param = in_Param;
	pos_x = x;
	pos_y = y;
	pos_z = z;
	rotation_x = rx;
	EnableFlag = flag;
	model_size = size;

	id_parameter = 0;
	id_model = -1;
	id_texture = -1;
	DarkModelFlag = false;
}

//! @brief ディストラクタ
object::~object()
{}

//! @brief 設定値を管理するクラスを登録
//! @attention 各関数を使用する前に実行すること。
void object::SetParameterInfoClass(class ParameterInfo *in_Param)
{
	Param = in_Param;
}

//! @brief 座標と角度を設定
//! @param x X座標
//! @param y Y座標
//! @param z Z座標
//! @param rx 横軸回転
void object::SetPosData(float x, float y, float z, float rx)
{
	pos_x = x;
	pos_y = y;
	pos_z = z;
	rotation_x = rx;
}

//! @brief 座標と角度を取得
//! @param x X座標を受け取るポインタ（NULL可）
//! @param y Y座標を受け取るポインタ（NULL可）
//! @param z Z座標を受け取るポインタ（NULL可）
//! @param rx 横軸回転を受け取るポインタ（NULL可）
void object::GetPosData(float *x, float *y, float *z, float *rx)
{
	if( x != NULL ){ *x = pos_x; }
	if( y != NULL ){ *y = pos_y; }
	if( z != NULL ){ *z = pos_z; }
	if( rx != NULL ){ *rx = rotation_x; }
}

//! @brief 有効化フラグを設定
//! @param flag 設定するフラグ
void object::SetEnableFlag(bool flag)
{
	EnableFlag = flag;
}

//! @brief 有効化フラグを取得
//! @return 現在設定されているフラグ
bool object::GetEnableFlag()
{
	return EnableFlag;
}

//! @brief モデルデータを設定
//! @param id モデルのデータ番号
//! @param size 描画倍率
void object::SetModel(int id, float size)
{
	id_model = id;
	model_size = size;
}

//! @brief モデルデータを取得
//! @param id モデルのデータ番号を受け取るポインタ（NULL可）
//! @param size 描画倍率を受け取るポインタ（NULL可）
void object::GetModel(int *id, float *size)
{
	if( id != NULL ){ *id = id_model; }
	if( size != NULL ){ *size = model_size; }
}

//! @brief テクスチャを設定
//! @param id テクスチャのデータ番号
void object::SetTexture(int id)
{
	id_texture = id;
}

//! @brief テクスチャを取得
//! @return テクスチャのデータ番号
int object::GetTexture()
{
	return id_texture;
}

//! @brief モデルを暗くするフラグを設定
//! @param flag フラグ
void object::SetDarkModelFlag(bool flag)
{
	DarkModelFlag = flag;
}

//! @brief 計算を実行（自由落下など）
int object::ProcessObject()
{
	return 0;
}

//! @brief 描画
//! @param d3dg D3DGraphicsのポインタ
//! @param NoModel モデル描画無効化
void object::Render(D3DGraphics *d3dg, bool NoModel)
{
	if( d3dg == NULL ){ return; }
	if( EnableFlag == false ){ return; }

	d3dg->SetWorldTransform(pos_x, pos_y, pos_z, rotation_x, 0.0f, model_size);
	d3dg->RenderModel(id_model, id_texture, DarkModelFlag, false, NoModel);
}

//! @brief コンストラクタ
human::human(class ParameterInfo *in_Param, float x, float y, float z, float rx, int id_param, int dataid, signed short int p4, int team, bool flag)
{
	//HumanParameter data;

	Param = in_Param;
	pos_x = x;
	pos_y = y;
	pos_z = z;
	move_x = 0.0f;
	move_y = 0.0f;
	move_z = 0.0f;
	move_y_flag = false;
	move_y_upper = 0;
	move_y_landing = false;
	LiftSpeed = 0.0f;	//[XOPS-ONLINE]
	underblock_id = -1;
	underblock_face = -1;
	rotation_x = rx;
	id_parameter = id_param;
	upmodel_size = 9.4f;
	armmodel_size = 9.0f;
	legmodel_size = 9.0f;
	EnableFlag = flag;
	rotation_y = 0.0f;
	armrotation_y = 0.0f;
	point_dataid = dataid;
	point_p4 = p4;
	teamid = team;

	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		weapon[i] = NULL;
		noweapon_bullets[i] = 0;
		noweapon_loadbullets[i] = 0;
	}
	selectweapon = 1;
	selectweaponcnt = 0;
	changeweaponidcnt = 0;
	weaponshotcnt = 0;
	weaponreloadcnt = 0;
	weaponshotframe = false;
	weaponburstmodecnt = 0;
	//if( Param->GetHuman(id_param, &data) == 0 ){
	//	hp = data.hp;
	//}
	//else{
		hp = 0;
	//}
	deadstate = 0;
	add_ry = 0.0f;
	id_texture = -1;

	move_rx = 0.0f;
	MoveFlag = 0x00;
	MoveFlag_lt = MoveFlag;
	scopemode = 0;
	HitFlag = false;
	Hit_rx = 0.0f;
	totalmove = 0.0f;
	StateGunsightErrorRange = 0;
	ReactionGunsightErrorRange = 0;
	Invincible = false;

	MotionCtrl = new HumanMotionControl;
}

//! @brief ディストラクタ
human::~human()
{
	delete MotionCtrl;
}

//! @brief 設定値を管理するクラスを登録
//! @attention 各関数を使用する前に実行すること。
void human::SetParameterInfoClass(class ParameterInfo *in_Param)
{
	Param = in_Param;

	MotionCtrl->SetParameterInfoClass(in_Param);
}

//! @brief 設定値を設定
//! @param id_param 人の種類番号
//! @param dataid ポイントのデータ番号
//! @param p4 第4パラメータ
//! @param team チーム番号
//! @param init オブジェクトを初期化
//! @warning init引数をtrueにして初期化する場合、先にSetPosData()関数で角度を設定しておくこと。
void human::SetParamData(int id_param, int dataid, signed short int p4, int team, bool init)
{
	id_parameter = id_param;
	point_dataid = dataid;
	point_p4 = p4;
	teamid = team;

	if( init == true ){
		HumanParameter data;

		move_x = 0.0f;
		move_y = 0.0f;
		move_z = 0.0f;
		move_y_flag = false;
		move_y_upper = 0;
		move_y_landing = false;
		LiftSpeed = 0.0f;	//[XOPS-ONLINE]
		underblock_id = -1;
		underblock_face = -1;
		rotation_y = 0.0f;
		armrotation_y = DegreeToRadian(-30);

		for(int i=0; i<TOTAL_HAVEWEAPON; i++){
			weapon[i] = NULL;
			noweapon_bullets[i] = 0;
			noweapon_loadbullets[i] = 0;
		}
		selectweapon = 1;
		selectweaponcnt = 0;
		changeweaponidcnt = 0;
		weaponshotcnt = 0;
		weaponreloadcnt = 0;
		weaponshotframe = false;
		weaponburstmodecnt = 0;
		if( Param->GetHuman(id_param, &data) == 0 ){
			hp = data.hp;
		}
		else{
			hp = 0;
		}
		deadstate = 0;
		add_ry = 0.0f;
		MoveFlag = 0x00;
		MoveFlag_lt = MoveFlag;
		scopemode = 0;
		HitFlag = false;
		Hit_rx = 0.0f;
		totalmove = 0.0f;
		Invincible = false;

		MotionCtrl->Init(rotation_x);
	}
}

//! @brief 設定値を取得
//! @param id_param 人の種類番号を受け取るポインタ（NULL可）
//! @param dataid ポイントのデータ番号を受け取るポインタ（NULL可）
//! @param p4 第4パラメータを受け取るポインタ（NULL可）
//! @param team チーム番号を受け取るポインタ（NULL可）
void human::GetParamData(int *id_param, int *dataid, signed short int *p4, int *team)
{
	if( id_param != NULL ){ *id_param = id_parameter; }
	if( dataid != NULL ){ *dataid = point_dataid; }
	if( p4 != NULL ){ *p4 = point_p4; }
	if( team != NULL ){ *team = teamid; }
}

//! @brief HPを取得
//! @return HP
int human::GetHP()
{
	return hp;
}

//! @brief HPを設定
//! @param in_hp 新たに設定するHP
//! @return 成功：true　失敗：false
//! @attention HPが1以上の人に対して実行しないと失敗します。
bool human::SetHP(int in_hp)
{
	if( hp > 0 ){
		hp = in_hp;
		return true;
	}
	return false;
}


//! @brief 死体かどうか判定
//! @return 死体：true　死体でない：false
//! @warning 完全に静止した状態を「死体」と判定します。倒れている最中の人は対象に含まれないため、hp <= 0 が全て死体と判定されるとは限りません。
bool human::GetDeadFlag()
{
	if( deadstate == 5 ){ return true; }
	return false;
}

//! @brief 死亡モーション中かどうか判定
//! @return モーション中である：true　モーション中でない：false
//! @attention 体が倒れている最中や、足を滑られている状態を判定します。落下中は判定しません。
bool human::GetDeadMotionFlag()
{
	if( deadstate == 1 ){ return true; }
	if( deadstate == 3 ){ return true; }
	return false;
}

//! @brief チーム番号を設定（上書き）
//! @param id 新しいチーム番号
void human::SetTeamID(int id)
{
	teamid = id;
}

//! @brief 無敵フラグを取得
//! @return 無敵：true　通常：false
//! @attention 無敵状態の場合、銃弾・手榴弾の爆発・落下　によるダメージを一切受けません。
bool human::GetInvincibleFlag()
{
	return Invincible;
}

//! @brief 無敵フラグを設定
//! @param flag 無敵：true　通常：false
//! @attention 無敵状態の場合、銃弾・手榴弾の爆発・落下　によるダメージを一切受けません。
void human::SetInvincibleFlag(bool flag)
{
	Invincible = flag;
}

//! @brief 前処理の移動量を取得
//! @param *x X軸移動量を取得するポインタ（NULL可）
//! @param *y Y軸移動量を取得するポインタ（NULL可）
//! @param *z Z軸移動量を取得するポインタ（NULL可）
void human::GetMovePos(float *x, float *y, float *z)
{
	if( x != NULL ){ *x = move_x; }
	if( y != NULL ){ *y = move_y; }
	if( z != NULL ){ *z = move_z; }
}

//! @brief モデルデータを設定
//! @param upmodel 上半身のモデル
//! @param armmodel[] 腕のモデルの配列（配列数：TOTAL_ARMMODE）
//! @param legmodel 足（静止状態）のモデル
//! @param walkmodel[] 腕のモデルの配列（配列数：TOTAL_WALKMODE）
//! @param runmodel[] 腕のモデルの配列（配列数：TOTAL_RUNMODE）
void human::SetModel(int upmodel, int armmodel[], int legmodel, int walkmodel[], int runmodel[])
{
	MotionCtrl->SetModel(upmodel, armmodel, legmodel, walkmodel, runmodel);
}

//! @brief 武器を設定
//! @param in_weapon[] 設定するweaponクラスのポインタ配列
//! @param nlnbs[] 武器未所持時の装弾数を設定するint配列
//! @param nnbs[] 武器未所持時の合計弾数を設定するint配列
//! @warning 通常は PickupWeapon()関数 を使用すること
void human::SetWeapon(class weapon *in_weapon[], int nlnbs[], int nnbs[])
{
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		if( in_weapon[i] == NULL ){
			weapon[i] = NULL;
		}
		else{
			//武器を正しく拾えれば、所持武器として登録
			if( in_weapon[i]->Pickup() == 0 ){
				weapon[i] = in_weapon[i];
			}
		}
		noweapon_loadbullets[i] = nlnbs[i];
		noweapon_bullets[i] = nnbs[i];
	}

	//[OpenXOPS-web] 選択中の武器スロットが未所持(NULL)なら、所持している
	//最初のスロットに切り替える。AddHumanIndex()はHUMANINFO経由で武器を
	//設定する際、スロット1(武器A)はPOINT_P1TYPE_HUMAN点のみ設定され、
	//HUMAN2点(controller pointで使われることが多い)ではスロット0(武器B)
	//のみが設定される。selectweaponの初期値は1固定(コンストラクタ)のため、
	//スロット0だけ持っている人物はこのままでは「武器なし(NONE)」表示に
	//なってしまう。実際に持っている武器があるなら、それを表示・使用でき
	//るようにする。
	if( weapon[selectweapon] == NULL ){
		for(int i=0; i<TOTAL_HAVEWEAPON; i++){
			if( weapon[i] != NULL ){
				selectweapon = i;
				break;
			}
		}
	}
}

//! @brief 武器を拾う
//! @param in_weapon[] 設定するweaponクラスのポインタ
//! @return 成功：1　失敗：0
int human::PickupWeapon(class weapon *in_weapon)
{
	if( in_weapon == NULL ){ return 0; }

	if( weapon[selectweapon] == NULL ){
		//武器を正しく拾えれば、所持武器として登録
		if( in_weapon->Pickup() == 0 ){
			weapon[selectweapon] = in_weapon;

			//腕の角度（反動）を設定
			class weapon *nowweapon;
			int id_param = 0;
			nowweapon = weapon[selectweapon];
			nowweapon->GetParamData(&id_param, NULL, NULL);
			MotionCtrl->PickupWeapon(id_param);

			//未所持時の弾数情報は初期化
			noweapon_bullets[selectweapon] = 0;
			noweapon_loadbullets[selectweapon] = 0;

			//切り替え完了のカウント
			selectweaponcnt = 10;

			return 1;
		}
	}
	return 0;
}

//! @brief 武器を切り替える（持ち替える）
//! @param id 持ち替える武器　（-1 で次の武器）
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
void human::ChangeHaveWeapon(int id)
{
	//体力がなければ失敗
	if( hp <= 0 ){ return; }

	//リロード中なら失敗
	if( weaponreloadcnt > 0 ){ return; }

	//同じ武器に切り替えようとしているなら、失敗
	if( selectweapon == id ){ return; }

	//武器切り替え中なら失敗
	if( selectweaponcnt > 0 ){ return; }
	if( changeweaponidcnt > 0 ){ return; }

	if( id == -1 ){
		//次の武器番号を選択
		selectweapon += 1;
		if( selectweapon == TOTAL_HAVEWEAPON ){
			selectweapon = 0;
		}
	}
	else{
		//武器番号が範囲内か確認
		if( (id < 0)||(TOTAL_HAVEWEAPON <= id ) ){ return; }
		selectweapon = id;
	}

	//スコープ表示を解除
	SetDisableScope();

	//現在装備する武器のクラスを取得
	class weapon *nowweapon;
	nowweapon = weapon[selectweapon];

	//腕の角度（反動）を設定
	if( nowweapon == NULL ){	//手ぶら
		MotionCtrl->ChangeHaveWeapon(ID_WEAPON_NONE);
	}
	else{
		int id_param = 0;
		nowweapon->GetParamData(&id_param, NULL, NULL);
		MotionCtrl->ChangeHaveWeapon(id_param);
	}

	//切り替え完了のカウント
	selectweaponcnt = 10;

	//バーストモード用カウント
	weaponburstmodecnt = 0;
}

//! @brief 武器の切り替えカウントを取得
//! @return カウント数　（1以上で切り替え中）
int human::GetChangeHaveWeaponCnt()
{
	return selectweaponcnt;
}

//! @brief 武器の種類切り替え
//! @return 成功：0　失敗：1
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
int human::ChangeWeaponID()
{
	//武器切り替え中なら失敗
	//if( selectweaponcnt > 0 ){ return 1; }
	if( changeweaponidcnt > 0 ){ return 1; }

	//武器を装備してなければ失敗
	if( weapon[selectweapon] == NULL ){ return 1; }

	//装備している武器の情報を取得
	int param_id, lnbs, nbs;
	weapon[selectweapon]->GetParamData(&param_id, &lnbs, &nbs);

	//武器の種類切り替え先（新しい武器番号）を調べる
	WeaponParameter ParamData;
	int ChangeWeaponID, ChangeCnt;
	if( Param->GetWeapon(param_id, &ParamData) != 0 ){ return 1; }
	ChangeWeaponID = ParamData.ChangeWeaponID;
	ChangeCnt = ParamData.ChangeWeaponCnt;

	//新しい武器番号が正しいか確認
	if( ChangeWeaponID == param_id ){ return 1; }
	if( (ChangeWeaponID < 0)||(TOTAL_PARAMETERINFO_WEAPON <= ChangeWeaponID) ){ return 1; }

	//カウントが有効にもかかわらずリロード中なら切り替えない
	if( (ChangeCnt > 0)&&(weaponreloadcnt > 0) ){ return 1; }

	//設定を適用
	weapon[selectweapon]->SetParamData(ChangeWeaponID, lnbs, nbs, false);

	//もしスコープ使用中ならば
	if( scopemode != 0 ){
		if( ChangeCnt == 0 ){
			//新しい武器の情報を取得
			if( Param->GetWeapon(ChangeWeaponID, &ParamData) != 0 ){ return 1; }

			if( scopemode == ParamData.scopemode ){
				//同じスコープが使えるならば、何もせずスコープ維持
			}
			else if( ParamData.scopemode == 0 ){
				//スコープが使えないならば、スコープ解除
				SetDisableScope();
			}
			else{
				//異なるスコープが使えるならば、スコープ再設定
				SetEnableScope();
			}
		}
		else{
			//スコープ解除
			SetDisableScope();
		}
	}

	//モーション実行
	MotionCtrl->ChangeWeaponID(ChangeWeaponID);

	//切り替え完了のカウント
	changeweaponidcnt = ChangeCnt;

	//バーストモード用カウント
	weaponburstmodecnt = 0;

	return 0;
}

//! @brief 武器の種類切り替えカウントを取得
//! @return カウント数　（1以上で切り替え中）
int human::GetChangeWeaponIDCnt()
{
	return changeweaponidcnt;
}

//! @brief 武器のリロードカウントを取得
//! @return カウント数　（リロード中：1以上）
int human::GetWeaponReloadCnt()
{
	return weaponreloadcnt;
}

//! @brief 武器を取得
//! @param out_selectweapon 選択されている武器　（0 ～ [TOTAL_HAVEWEAPON]-1）
//! @param out_weapon 受け取るweaponクラスのポインタ配列　（配列数：TOTAL_HAVEWEAPON）
//! @param nlnbs[] 武器未所持時の装弾数を受け取るint配列　（配列数：TOTAL_HAVEWEAPON、NULL可）
//! @param nnbs[] 武器未所持時の合計弾数を受け取るint配列　（配列数：TOTAL_HAVEWEAPON、NULL可）
void human::GetWeapon(int *out_selectweapon, class weapon *out_weapon[], int nlnbs[], int nnbs[])
{
	*out_selectweapon = selectweapon;
	for(int i=0; i<TOTAL_HAVEWEAPON; i++){
		out_weapon[i] = weapon[i];
		if( nlnbs != NULL ){ nlnbs[i] = noweapon_loadbullets[i]; }
		if( nnbs != NULL ){ nnbs[i] = noweapon_bullets[i]; }
	}
}

//! @brief 現在装備している武器の種類番号を取得
//! @return 武器の種類番号（0 ～ TOTAL_PARAMETERINFO_WEAPON -1）
//! @attention 現在手に持っている武器の種類番号です。　GetWeapon()関数 を用いて調べるのと同等です。
int human::GetMainWeaponTypeNO()
{
	if( weapon[selectweapon] == NULL ){
		return ID_WEAPON_NONE;
	}

	int id_param;
	weapon[selectweapon]->GetParamData(&id_param, NULL, NULL);
	return id_param;
}

//! @brief 連射設定を取得
//! @return 連射可能：true　連射不可：false
bool human::GetWeaponBlazingmode()
{
	int param_id;
	WeaponParameter data;

	//武器を装備していなければ、失敗
	if( weapon[selectweapon] == NULL ){ return false; }

	//装備している武器の種類番号を取得
	weapon[selectweapon]->GetParamData(&param_id, NULL, NULL);

	//連射設定を返す
	Param->GetWeapon(param_id, &data);
	if( data.burstcnt == 1 ){ return false; }
	return true;
}

//! @brief 発砲処理
//! @param weapon_paramid 発砲した武器の番号を受け取るポインタ
//! @param GunsightErrorRange 発砲した際の照準誤差を受け取るポインタ
//! @return 成功：true　失敗：false
//! @attention 弾オブジェクトの処理や、発砲音の再生は別途行う必要があります。
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
bool human::ShotWeapon(int *weapon_paramid, int *GunsightErrorRange)
{
	if( weapon_paramid == NULL ){ return false; }
	if( GunsightErrorRange == NULL ){ return false; }

	int param_id;

	//武器切り替え中なら失敗
	if( selectweaponcnt > 0 ){ return false; }
	if( changeweaponidcnt > 0 ){ return false; }

	//武器を装備していなければ、失敗
	if( weapon[selectweapon] == NULL ){ return false; }

	//発砲要求は受け入れ、フラグ有効化
	weaponshotframe = true;

	//発射間隔に満たないか、リロード中ならば失敗
	if( weaponshotcnt > 0 ){ return false; }
	if( weaponreloadcnt > 0 ){ return false; }

	//武器の種類番号を取得
	weapon[selectweapon]->GetParamData(&param_id, NULL, NULL);

	//武器の設定値を取得
	WeaponParameter ParamData;
	ScopeParameter sparam;
	if( Param->GetWeapon(param_id, &ParamData) != 0 ){ return false; }
	Param->GetScopeParam(ParamData.scopemode, &sparam);

	//バーストモードが有効で、連射弾数が超えていれば失敗
	if( (ParamData.burstcnt > 0)&&(weaponburstmodecnt >= ParamData.burstcnt) ){ return false; }

	//弾の発射処理を行う
	if( weapon[selectweapon]->Shot() != 0 ){ return false; }

	//発射弾数としてカウント
	weaponburstmodecnt += 1;


	//連射カウントを設定
	weaponshotcnt = ParamData.blazings;

	//スコープ未使用時の誤差を設定
	if( (scopemode == 0)&&(sparam.WeaponNoScopeReaction > 0) ){
		ReactionGunsightErrorRange = sparam.WeaponNoScopeReaction;
	}


	//武器の種類番号と誤差を返す
	*weapon_paramid = param_id;
	*GunsightErrorRange = StateGunsightErrorRange + ReactionGunsightErrorRange;


	//誤差計算
	if( ( (scopemode == 0)&&(sparam.NoScopeGunsight == true) )||( (scopemode != 0)&&(sparam.ScopeGunsight == 0) ) ){
		//照準が表示されるならば、設定された誤差を加算。
		ReactionGunsightErrorRange += ParamData.reaction;
	}

	//スコープを使用している状態の反動を加算
	if( (sparam.WeaponRecoil_ScopeRx != 0.0f)||(sparam.WeaponRecoil_ScopeRyMin != 0.0f)||(sparam.WeaponRecoil_ScopeRyMax != 0.0f) ){
		if( (scopemode != 0)||(sparam.WeaponRecoil_ScopeOnly == false) ){
			int rx_rnds, ry_rnds;
			float rx_degree, ry_degree;

			//ランダムの数値範囲計算
			rx_rnds = (int)(sparam.WeaponRecoil_ScopeRx*10) * 2;
			ry_rnds = (int)(sparam.WeaponRecoil_ScopeRyMax*10) - (int)(sparam.WeaponRecoil_ScopeRyMin*10);

			//反動（角度）決定
			rx_degree = sparam.WeaponRecoil_ScopeRx*-1 + ((float)GetRand(rx_rnds))/10;
			ry_degree = sparam.WeaponRecoil_ScopeRyMin + ((float)GetRand(ry_rnds))/10;

			//反動加算
			rotation_x += DegreeToRadian(rx_degree);
			armrotation_y += DegreeToRadian(ry_degree);
		}
	}

	//腕に反動を伝える
	MotionCtrl->ShotWeapon(param_id);

	//武器が無くなっていれば、装備から外した扱いに。　（手榴弾用）
	if( weapon[selectweapon]->GetEnableFlag() == false ){
		weapon[selectweapon] = NULL;
	}
	return true;
}

//! @brief リロード
//! @return 成功：true　失敗：false
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
bool human::ReloadWeapon()
{
	//武器切り替え中なら失敗
	if( selectweaponcnt > 0 ){ return false; }
	if( changeweaponidcnt > 0 ){ return false; }

	//何かしらの武器を装備していれば～
	if( weapon[selectweapon] != NULL ){
		//リロード中なら失敗
		if( weaponreloadcnt > 0 ){ return false; }

		//リロード処理を開始
		if( weapon[selectweapon]->StartReload() != 0 ){ return false; }

		//スコープモードを解除
		SetDisableScope();

		//モーション実行
		class weapon *nowweapon;
		int id_param = 0;
		nowweapon = weapon[selectweapon];
		nowweapon->GetParamData(&id_param, NULL, NULL);
		MotionCtrl->ReloadWeapon(id_param);

		//武器の性能値を取得
		int param_id;
		WeaponParameter ParamData;
		weapon[selectweapon]->GetParamData(&param_id, NULL, NULL);
		if( Param->GetWeapon(param_id, &ParamData) != 0 ){ return 1; }

		//リロードカウントを設定
		weaponreloadcnt = ParamData.reloads + 1;

		//バーストモード用カウントを設定
		weaponburstmodecnt = 0;

		return true;
	}
	return false;
}

//! @brief 武器を捨てる
//! @return 成功：true　失敗：false
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
bool human::DumpWeapon()
{
	//武器切り替え中なら失敗
	if( selectweaponcnt > 0 ){ return false; }
	if( changeweaponidcnt > 0 ){ return false; }

	//何かしらの武器を装備していれば～
	if( weapon[selectweapon] != NULL ){
		//リロード中なら失敗
		if( weaponreloadcnt > 0 ){ return false; }

		//武器を捨て、装備を解除
		weapon[selectweapon]->Dropoff(pos_x, pos_y, pos_z, rotation_x, 1.63f);
		weapon[selectweapon] = NULL;

		//未所持時の弾数情報は初期化
		noweapon_bullets[selectweapon] = 0;
		noweapon_loadbullets[selectweapon] = 0;

		//スコープモードを解除
		SetDisableScope();

		//モーション実行
		MotionCtrl->DumpWeapon();

		return true;
	}

	return false;
}

//! @brief 前進（走り）を設定
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
void human::SetMoveForward()
{
	SetFlag(MoveFlag, MOVEFLAG_FORWARD);
}

//! @brief 後退を設定
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
void human::SetMoveBack()
{
	SetFlag(MoveFlag, MOVEFLAG_BACK);
}

//! @brief 左走りを設定
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
void human::SetMoveLeft()
{
	SetFlag(MoveFlag, MOVEFLAG_LEFT);
}

//! @brief 右走りを設定
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
void human::SetMoveRight()
{
	SetFlag(MoveFlag, MOVEFLAG_RIGHT);
}

//! @brief 歩きを設定
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
void human::SetMoveWalk()
{
	SetFlag(MoveFlag, MOVEFLAG_WALK);
}

//! @brief 人の移動モードを取得
//! @param nowdata 現在の値を取得：true　前フレームの値を使用：false
//! @return 歩き：1　前走り：2　後ろ走り：3　左右走り：4　移動してない：0
int human::GetMovemode(bool nowdata)
{
	//使用されていないか、処理されていなければ終了
	if( EnableFlag == false ){ return 0; }
	if( hp <= 0 ){ return 0; }

	if( nowdata == false ){	//前のデータ
		//歩きならば 1
		if( GetFlag(MoveFlag_lt, MOVEFLAG_WALK) ){
			return 1;
		}
		//前走りならば 2
		if( GetFlag(MoveFlag_lt, MOVEFLAG_FORWARD) ){
			return 2;
		}
		//後ろ走りならば 3
		if( GetFlag(MoveFlag_lt, MOVEFLAG_BACK) ){
			return 3;
		}
		//左右走りならば 4
		if( GetFlag(MoveFlag_lt, (MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) ){
			return 4;
		}
	}
	else{					//現在のデータ
		//歩きならば 1
		if( GetFlag(MoveFlag, MOVEFLAG_WALK) ){
			return 1;
		}
		//前走りならば 2
		if( GetFlag(MoveFlag, MOVEFLAG_FORWARD) ){
			return 2;
		}
		//後ろ走りならば 3
		if( GetFlag(MoveFlag, MOVEFLAG_BACK) ){
			return 3;
		}
		//左右走りならば 4
		if( GetFlag(MoveFlag, (MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) ){
			return 4;
		}
	}

	//移動してない
	return 0;
}

//! @brief 移動方向を表すフラグを取得
//! @return 設定値（Human_MoveFlag列挙型）
//! @attention デバック用関数です。通常はGetMovemode()関数を使用してください。
int human::GetMoveFlag(bool nowdata)
{
	if( nowdata == false ){	//前のデータ
		return MoveFlag_lt;
	}
	else{					//現在のデータ
		return MoveFlag;
	}
}

//! @brief スコープを設定
//! @return 成功：true　失敗：false
bool human::SetEnableScope()
{
	int param_id;
	WeaponParameter data;

	//何も武器を装備してなければ失敗
	if( weapon[selectweapon] == NULL ){ return false; }

	//リロード中なら失敗
	if( weaponreloadcnt > 0 ){ return false; }

	//武器の種類番号を取得
	weapon[selectweapon]->GetParamData(&param_id, NULL, NULL);

	//武器の設定値を取得
	Param->GetWeapon(param_id, &data);

	//スコープを設定
	scopemode = data.scopemode;
	return true;
}

//! @brief スコープを解除
void human::SetDisableScope()
{
	scopemode = 0;
}

//! @brief スコープ設定を取得
int human::GetScopeMode()
{
	return scopemode;
}

//! @brief 横軸と縦軸の向きを取得
//! @param rx 横軸を取得するポインタ
//! @param ry 縦軸を取得するポインタ
void human::GetRxRy(float *rx, float *ry)
{
	if( (rx == NULL)||(ry == NULL) ){ return; }

	*rx = rotation_x;
	*ry = armrotation_y;
}

//! @brief 横軸と縦軸の向きを設定
//! @param rx 設定する横軸
//! @param ry 設定する縦軸
void human::SetRxRy(float rx, float ry)
{
	rotation_x = rx;
	armrotation_y = ry;
}

//! @brief 全体の回転角度取得
//! @return 縦軸を取得するポインタ
//! @warning 死亡して倒れる際の角度です。GetRxRy()関数で受け取る値とは異なります。
float human::GetDeadRy()
{
	return rotation_y;
}

//! @brief ジャンプ
//! @attention ゲーム上から直接呼び出すことは避け、ObjectManagerクラスから呼び出してください。
void human::Jump()
{
	SetFlag(MoveFlag, MOVEFLAG_JUMP);
}

//! @brief ジャンプ・着地の取得
//! @param nowdata 現在の値を取得：true　前フレームの値を使用：false
//! @return ジャンプ：1　着地：2　それ以外：0
//! @attention 着地の判定は常に現在の値を取得します。前フレームの状態は取得しません。
//! @note 足音再生判定用
int human::GetJumpLanding(bool nowdata)
{
	//使用されていないか、処理されていなければ終了
	if( EnableFlag == false ){ return 0; }
	if( hp <= 0 ){ return 0; }

	if( nowdata == false ){	//前のデータ
		if( move_y_landing == true ){ return 2; }
		if( GetFlag(MoveFlag_lt, MOVEFLAG_JUMP) ){ return 1; }
	}
	else{
		if( move_y_landing == true ){ return 2; }
		if( GetFlag(MoveFlag, MOVEFLAG_JUMP) ){ return 1; }
	}

	//ジャンプしていない
	return 0;
}

//! @brief 足元のブロックID・面番号を取得
//! @param id 足元のブロックIDを受け取るポインタ（NULL可）
//! @param face 足元のブロックの面番号を受け取るポインタ（NULL可）
//! @attention 空中の場合など足元にブロックがない場合、ブロックIDと面番号は -1 を返します。
void human::GetUnderBlock(int *id, int *face)
{
	if( id != NULL ){ *id = underblock_id; }
	if( face != NULL ){ *face = underblock_face; }
}

//! @brief 押しだす・力を加える
//! @param rx 横軸
//! @param ry 縦軸
//! @param speed 速度
void human::AddPosOrder(float rx, float ry, float speed)
{
	move_x += cosf(rx) * cosf(ry) * speed;
	move_y += sinf(ry) * speed;
	move_z += sinf(rx) * cosf(ry) * speed;
}

//! @brief 弾が 頭 にヒット
//! @param attacks 弾の攻撃力
void human::HitBulletHead(int attacks)
{
	if( Invincible == false ){
		hp -= (int)((float)attacks * HUMAN_DAMAGE_HEAD) + GetRand(16);
	}
	ReactionGunsightErrorRange = 15;
}

//! @brief 弾が 上半身 にヒット
//! @param attacks 弾の攻撃力
void human::HitBulletUp(int attacks)
{
	if( Invincible == false ){
		hp -= (int)((float)attacks * HUMAN_DAMAGE_UP) + GetRand(8);
	}
	ReactionGunsightErrorRange = 12;
}

//! @brief 弾が 下半身 にヒット
//! @param attacks 弾の攻撃力
void human::HitBulletLeg(int attacks)
{
	if( Invincible == false ){
		hp -= (int)((float)attacks * HUMAN_DAMAGE_LEG) + GetRand(6);
	}
	ReactionGunsightErrorRange = 8;
}

//! @brief ゾンビの攻撃がヒット
void human::HitZombieAttack()
{
	if( Invincible == false ){
		hp -= HUMAN_DAMAGE_ZOMBIEU + GetRand(HUMAN_DAMAGE_ZOMBIEA);
	}
	ReactionGunsightErrorRange = 10;
}

//! @brief 手榴弾の爆風がヒット
//! @param attacks 爆風の攻撃力
//! @attention 距離による計算を事前に済ませてください。
void human::HitGrenadeExplosion(int attacks)
{
	if( Invincible == false ){
		hp -= attacks;
	}
	ReactionGunsightErrorRange = 10;
}

//! @brief 被弾フラグをセット
//! @param rx 被弾した方向
void human::SetHitFlag(float rx)
{
	HitFlag = true;
	Hit_rx = rx;
}

//! @brief 被弾したかチェックする
//! @param rx 被弾した方向を受け取るポインタ（NULL可）
//! @return 被弾した：true　被弾してない：false
//! @attention 実行すると、フラグは false に初期化されます。
bool human::CheckHit(float *rx)
{
	bool returnflag = HitFlag;
	if( rx != NULL ){ *rx = Hit_rx; }
	HitFlag = false;
	//Hit_rx = 0.0f;
	return returnflag;
}

//! @brief 合計移動量を取得
//! @return 合計移動量
float human::GetTotalMove()
{
	return totalmove;
}

//! @brief 歩き・走りモーションのカウント値取得
//! @return カウント値
int human::GetMoveMotionCount()
{
	return MotionCtrl->GetMoveMotionCnt();
}

//! @brief 照準の状態誤差の処理
//!	@attention ControlProcess()より前で処理すること
void human::GunsightErrorRange()
{
	//初期化
	StateGunsightErrorRange = 0;

	//各操作による誤差を設定
	if( GetFlag(MoveFlag, MOVEFLAG_WALK) ){
		StateGunsightErrorRange = 4;
	}
	if( GetFlag(MoveFlag, MOVEFLAG_FORWARD) ){
		StateGunsightErrorRange = 8;
	}
	if( GetFlag(MoveFlag, MOVEFLAG_BACK) ){
		StateGunsightErrorRange = 6;
	}
	if( GetFlag(MoveFlag, (MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) ){
		StateGunsightErrorRange = 7;
	}
	if( move_y_flag == true ){
		StateGunsightErrorRange = 22;
	}
	if( hp < 40 ){
		StateGunsightErrorRange += 3;
	}

	//何か武器を装備していれば
	if( weapon[selectweapon] != NULL ){
		//武器の設定データを取得
		int param;
		WeaponParameter data;
		weapon[selectweapon]->GetParamData(&param, NULL, NULL);
		Param->GetWeapon(param, &data);

		//誤差を 1 減らす
		ReactionGunsightErrorRange -= 1;

		//誤差の範囲を補正
		if( ReactionGunsightErrorRange < 0 ){ ReactionGunsightErrorRange = 0; }
		if( ReactionGunsightErrorRange > data.ErrorRangeMAX ){ ReactionGunsightErrorRange = data.ErrorRangeMAX; }
	}
	else{
		ReactionGunsightErrorRange = 0;
	}
}

//! @brief 死亡判定と倒れる処理
//! @return 静止した死体：4　倒れ終わった直後：3　倒れている最中：2　倒れ始める：1　何もしない：0
int human::CheckAndProcessDead(class Collision *CollD)
{
	if( CollD == NULL ){ return 0; }

	if( hp <= 0 ){
		//腕の角度
		if( armrotation_y < 0.0f ){
			armrotation_y -= DegreeToRadian(6);
			if( armrotation_y < DegreeToRadian(-90) ){ armrotation_y = DegreeToRadian(-90); }
		}
		else{
			armrotation_y += DegreeToRadian(6);
			if( armrotation_y > DegreeToRadian(90) ){ armrotation_y = DegreeToRadian(90); }
		}
	}

#ifdef HUMAN_DEADBODY_COLLISION

	//メモ：
	//
	//状態：0
	//［何もしない］
	//　HPが0以下で、頭が付かぬなら　状態：1
	//　HPが0以下で、壁に頭が付くなら　状態：2
	//
	//状態：1
	//［傾き始める］
	//　135度行ったら　状態：2
	//　壁に頭を打ったら　状態：3
	//
	//状態：2
	//［落下］
	//　足が地面に付いたら　状態：4
	//
	//状態：3
	//［足を滑らせる］
	//　　
	//状態：4
	//［固定］

	float check_posx, check_posy, check_posz;

	if( deadstate == 0 ){
		if( hp <= 0 ){		//HPが 0 以下になった（死亡した）瞬間なら、倒し始める
			float tr;

			//最後に攻撃を受けた方向を計算
			tr = Hit_rx - rotation_x;
			tr = AngleNormalization(tr);

			//体の角度
			if( ((float)M_PI/2*-1 < tr)&&(tr < (float)M_PI/2) ){
				add_ry = HUMAN_DEADADDRY;
			}
			else{
				add_ry = HUMAN_DEADADDRY * -1;
			}

			//死体が埋まらぬよう、高さを +1.0 する
			pos_y += 1.0f;

			//所持している武器を全て捨てる
			for(int i=0; i<TOTAL_HAVEWEAPON; i++){
				if( weapon[i] != NULL ){
					weapon[i]->Dropoff(pos_x, pos_y, pos_z, DegreeToRadian(10)*GetRand(36), 1.5f);
					weapon[i] = NULL;

					//未所持時の弾数情報は初期化
					noweapon_bullets[i] = 0;
					noweapon_loadbullets[i] = 0;
				}
			}

			//スコープモードを解除
			SetDisableScope();

			//各種カウント初期化
			selectweaponcnt = 0;
			changeweaponidcnt = 0;
			weaponshotcnt = 0;
			weaponreloadcnt = 0;

			//次のフレームの頭の座標を取得
			check_posx = pos_x + cosf(rotation_x*-1 - (float)M_PI/2) * sinf(add_ry) * HUMAN_HEIGHT;
			check_posy = pos_y + cosf(add_ry) * HUMAN_HEIGHT;
			check_posz = pos_z + sinf(rotation_x*-1 - (float)M_PI/2) * sinf(add_ry) * HUMAN_HEIGHT;

			if( CollD->CheckALLBlockInside(check_posx, check_posy, check_posz) == true ){
				deadstate = 2;
			}
			else{
				rotation_y += add_ry;
				deadstate = 1;
			}
			return 1;
		}
		return 0;
	}

	if( deadstate == 1 ){
		//135度以上倒れていれば
		if( fabsf(rotation_y) >= DegreeToRadian(135) ){
			deadstate = 2;
			move_y = 0.0f;
			return 2;
		}

		if( pos_y <= (HUMAN_DEADLINE + 10.0f) ){
			//90度以上倒れていれば
			if( fabsf(rotation_y) >= DegreeToRadian(90) ){
				deadstate = 4;
				return 2;
			}
		}

		//前後に倒す
		if( rotation_y > 0.0f ){		//rotation_y < DegreeToRadian(135)
			add_ry += HUMAN_DEADADDRY;
		}
		else if( rotation_y < 0.0f ){	//rotation_y > DegreeToRadian(-135)
			add_ry -= HUMAN_DEADADDRY;
		}

		if( pos_y <= HUMAN_DEADLINE ){
			rotation_y += add_ry;
		}
		else{
			//次のフレームの頭の座標を取得
			check_posx = pos_x + cosf(rotation_x*-1 - (float)M_PI/2) * sinf(rotation_y + add_ry) * HUMAN_HEIGHT;
			check_posy = pos_y + cosf(rotation_y + add_ry) * HUMAN_HEIGHT;
			check_posz = pos_z + sinf(rotation_x*-1 - (float)M_PI/2) * sinf(rotation_y + add_ry) * HUMAN_HEIGHT;

			if( CollD->CheckALLBlockInside(check_posx, check_posy, check_posz) == true ){
				deadstate = 3;
			}
			else{
				rotation_y += add_ry;
			}
		}

		return 2;
	}

	if( deadstate == 2 ){
		if( pos_y <= HUMAN_DEADLINE ){
			deadstate = 4;
			return 2;
		}

		if( move_y > -4.0f ){
			move_y -= 0.1f;
		}

		//次のフレームの足の座標
		check_posx = pos_x;
		check_posy = pos_y - move_y - HUMAN_HEIGHT*0.5f;
		check_posz = pos_z;

		if( CollD->CheckALLBlockInside(check_posx, check_posy, check_posz) == true ){
			deadstate = 4;
		}
		else{
			pos_y += move_y;
		}

		return 2;
	}

	if( deadstate == 3 ){
		//deadstate = 4;

		//90度以上倒れていれば
		if( fabsf(rotation_y) >= DegreeToRadian(90) ){
			deadstate = 4;
			return 2;
		}

		//前後に倒す
		if( rotation_y > 0.0f ){		//rotation_y < DegreeToRadian(90)
			add_ry += HUMAN_DEADADDRY;
		}
		else if( rotation_y < 0.0f ){	//rotation_y > DegreeToRadian(-90)
			add_ry -= HUMAN_DEADADDRY;
		}

		//次のフレームの足の座標を取得
		check_posx = pos_x - cosf(rotation_x*-1 - (float)M_PI/2) * sinf(rotation_y + add_ry) * HUMAN_HEIGHT;
		check_posy = pos_y + 0.1f;
		check_posz = pos_z - sinf(rotation_x*-1 - (float)M_PI/2) * sinf(rotation_y + add_ry) * HUMAN_HEIGHT;

		if( CollD->CheckALLBlockInside(check_posx, check_posy, check_posz) == true ){
			deadstate = 4;
			return 2;
		}

		//次のフレームの頭の座標を取得
		check_posx = pos_x - cosf(rotation_x*-1 - (float)M_PI/2) * sinf(rotation_y + add_ry) * HUMAN_HEIGHT;
		check_posy = pos_y + cosf(rotation_y + add_ry) * HUMAN_HEIGHT;
		check_posz = pos_z - sinf(rotation_x*-1 - (float)M_PI/2) * sinf(rotation_y + add_ry) * HUMAN_HEIGHT;

		if( CollD->CheckALLBlockInside(check_posx, check_posy, check_posz) == true ){
			deadstate = 4;
			return 2;
		}

		//足の座標を移動
		pos_x -= cosf(rotation_x*-1 - (float)M_PI/2) * sinf(add_ry) * HUMAN_HEIGHT;
		pos_z -= sinf(rotation_x*-1 - (float)M_PI/2) * sinf(add_ry) * HUMAN_HEIGHT;

		rotation_y += add_ry;
		return 2;
	}

	if( deadstate == 4 ){
		//何もしない（固定）
		deadstate = 5;
		return 3;
	}

	if( deadstate == 5 ){
		//何もしない（固定）
		return 4;
	}

	return 0;
#else
	if( fabsf(rotation_y) >= DegreeToRadian(90) ){
		return 4;
	}
	else if( rotation_y > 0.0f ){		//倒れ始めていれば、そのまま倒れる。
		add_ry += HUMAN_DEADADDRY;
		rotation_y += add_ry;
		if( rotation_y >= DegreeToRadian(90) ){
			rotation_y = DegreeToRadian(90);
			deadstate = 5;
			return 3;
		}
		return 2;
	}
	else if( rotation_y < 0.0f ){	//倒れ始めていれば、そのまま倒れる。
		add_ry -= HUMAN_DEADADDRY;
		rotation_y += add_ry;
		if( rotation_y <= DegreeToRadian(-90) ){
			rotation_y = DegreeToRadian(-90);
			deadstate = 5;
			return 3;
		}
		return 2;
	}
	else if( hp <= 0 ){		//HPが 0 以下になった（死亡した）瞬間なら、倒し始める
		float tr;

		//最後に攻撃を受けた方向を計算
		tr = Hit_rx - rotation_x;
		tr = AngleNormalization(tr);

		//体の角度
		if( ((float)M_PI/2*-1 < tr)&&(tr < (float)M_PI/2) ){
			add_ry = HUMAN_DEADADDRY;
		}
		else{
			add_ry = HUMAN_DEADADDRY * -1;
		}

		rotation_y += add_ry;

		//死体が埋まらぬよう、高さを +1.0 する
		pos_y += 1.0f;

		//所持している武器を全て捨てる
		for(int i=0; i<TOTAL_HAVEWEAPON; i++){
			if( weapon[i] != NULL ){
				weapon[i]->Dropoff(pos_x, pos_y, pos_z, DegreeToRadian(10)*GetRand(36), 1.5f);
				weapon[i] = NULL;

				//未所持時の弾数情報は初期化
				noweapon_bullets[i] = 0;
				noweapon_loadbullets[i] = 0;
			}
		}

		//スコープモードを解除
		SetDisableScope();

		deadstate = 1;
		return 1;
	}

	return 0;
#endif
}

//! @brief 操作による移動計算
//! @attention 実行すると、各キーフラグは false に初期化されます。
void human::ControlProcess()
{
	//進行方向と速度を決定
	if( GetFlag(MoveFlag, MOVEFLAG_WALK) ){
		move_rx = DegreeToRadian(0);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_PROGRESSWALK_ACCELERATION);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == MOVEFLAG_FORWARD ){
		move_rx = DegreeToRadian(0);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_PROGRESSRUN_ACCELERATION);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == MOVEFLAG_BACK ){
		move_rx = DegreeToRadian(180);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_REGRESSRUN_ACCELERATION);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == MOVEFLAG_LEFT ){
		move_rx = DegreeToRadian(90);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_SIDEWAYSRUN_ACCELERATION);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == MOVEFLAG_RIGHT ){
		move_rx = DegreeToRadian(-90);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_SIDEWAYSRUN_ACCELERATION);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == (MOVEFLAG_FORWARD | MOVEFLAG_LEFT) ){
		move_rx = DegreeToRadian(45);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_PROGRESSRUN_SIDEWAYSRUN_ACCELERATION);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == (MOVEFLAG_BACK | MOVEFLAG_LEFT) ){
		move_rx = DegreeToRadian(135);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_REGRESSRUN_SIDEWAYSRUN_ACCELERATION);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == (MOVEFLAG_BACK | MOVEFLAG_RIGHT) ){
		move_rx = DegreeToRadian(-135);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_REGRESSRUN_SIDEWAYSRUN_ACCELERATION);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == (MOVEFLAG_FORWARD | MOVEFLAG_RIGHT) ){
		move_rx = DegreeToRadian(-45);
		AddPosOrder(rotation_x*-1 + move_rx + (float)M_PI/2, 0.0f, HUMAN_PROGRESSRUN_SIDEWAYSRUN_ACCELERATION);
	}
	else{
		move_rx = 0.0f;
	}

	//フラグをバックアップ
	MoveFlag_lt = MoveFlag;

	//キーフラグを元に戻す
	MoveFlag = 0x00;
}

//! @brief マップとの当たり判定
//! @param CollD Collisionクラスのポインタ
//! @param inblockdata BlockDataInterfaceクラスのポインタ
//! @param AddCollisionFlag 追加の当たり判定フラグ
//! @param player 対象の人物がプレイヤーかどうか
void human::CollisionMap(class Collision *CollD, class BlockDataInterface *inblockdata, bool AddCollisionFlag, bool player)
{
	if( CollD == NULL ){ return; }
	if( inblockdata == NULL ){ return; }

	float pos_x2, pos_y2, pos_z2;
	float dist_x, dist_y, dist_z;
	float box_min_x, box_min_y, box_min_z, box_max_x, box_max_y, box_max_z;
	bool CheckBlockID[MAX_BLOCKS];
	float speed;
	float Dist;
	bool FallFlag;

	//ブロックIDと面番号は、ひとまず -1 にする。
	underblock_id = -1;
	underblock_face = -1;

	//現時点の座標をバックアップ
	pos_x2 = pos_x;
	pos_y2 = pos_y;
	pos_z2 = pos_z;

	//移動反映
	pos_x += move_x;
	pos_z += move_z;

	//次フレームの移動量を減衰
	move_x *= HUMAN_ATTENUATION;
	move_z *= HUMAN_ATTENUATION;

	//※本家XOPSではここで人同士の当たり判定を行う。
	//　OpenXOPSにおいては別タイミングで計算するため、以下dist_yは 0.0f 確定であり値としては無意味。

	//移動ベクトル算出
	dist_x = pos_x - pos_x2;
	dist_y = pos_y - pos_y2;
	dist_z = pos_z - pos_z2;
	speed = sqrtf(dist_x*dist_x + dist_z*dist_z);
	VectorNormalization(&dist_x, &dist_y, &dist_z);

	int bs = inblockdata->GetTotaldatas();

	//人のAABBを作成
	box_min_x = pos_x - HUMAN_MAPCOLLISION_CHECK_MAXDIST - COLLISION_ADDSIZE;
	box_min_y = pos_y - HUMAN_MAPCOLLISION_CHECK_MAXDIST - COLLISION_ADDSIZE;
	box_min_z = pos_z - HUMAN_MAPCOLLISION_CHECK_MAXDIST - COLLISION_ADDSIZE;
	box_max_x = pos_x + HUMAN_MAPCOLLISION_CHECK_MAXDIST + COLLISION_ADDSIZE;
	box_max_y = (pos_y + HUMAN_HEIGHT) + HUMAN_MAPCOLLISION_CHECK_MAXDIST + COLLISION_ADDSIZE;
	box_max_z = pos_z + HUMAN_MAPCOLLISION_CHECK_MAXDIST + COLLISION_ADDSIZE;

	//AABB判定で対象ブロックを粗削りする
	for(int i=0; i<bs; i++){
		CheckBlockID[i] = CollD->CheckBlockAABB(i, box_min_x, box_min_y, box_min_z, box_max_x, box_max_y, box_max_z);
	}

	if( (speed != 0.0f)||(move_y != 0.0f) ){

		//頭部で当たり判定
		for(int i=0; i<bs; i++){
			if( CheckBlockID[i] == true ){
				if( CollisionBlockScratch(CollD, inblockdata, i, &pos_x, &pos_y, &pos_z, pos_x2, pos_y2, pos_z2, pos_x, pos_y + HUMAN_HEIGHT-0.22f, pos_z, 0x01) == true ){
					if( move_y > 0.0f ){ move_y = 0.0f; }
				}
			}
		}

		//足元で当たり判定
		for(int i=0; i<bs; i++){
			if( CheckBlockID[i] == true ){
				CollisionBlockScratch(CollD, inblockdata, i, &pos_x, &pos_y, &pos_z, pos_x2, pos_y2, pos_z2, pos_x, pos_y, pos_z, 0x00);
			}
		}

		//腰くらいで当たり判定
		for(int i=0; i<bs; i++){
			if( CheckBlockID[i] == true ){
				CollisionBlockScratch(CollD, inblockdata, i, &pos_x, &pos_y, &pos_z, pos_x2, pos_y2, pos_z2, pos_x + dist_x*HUMAN_MAPCOLLISION_R, pos_y + HUMAN_MAPCOLLISION_HEIGHT, pos_z + dist_z*HUMAN_MAPCOLLISION_R, 0x02);
				CollisionBlockScratch(CollD, inblockdata, i, &pos_x, &pos_y, &pos_z, pos_x2, pos_y2, pos_z2, pos_x + dist_z*HUMAN_MAPCOLLISION_R, pos_y + HUMAN_MAPCOLLISION_HEIGHT, pos_z + dist_x*HUMAN_MAPCOLLISION_R, 0x02);
				CollisionBlockScratch(CollD, inblockdata, i, &pos_x, &pos_y, &pos_z, pos_x2, pos_y2, pos_z2, pos_x - dist_z*HUMAN_MAPCOLLISION_R, pos_y + HUMAN_MAPCOLLISION_HEIGHT, pos_z - dist_x*HUMAN_MAPCOLLISION_R, 0x02);
			}
		}

		if( AddCollisionFlag == true ){
			//腰付近の追加当たり判定
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					CollisionBlockScratch(CollD, inblockdata, i, &pos_x, &pos_y, &pos_z, pos_x2, pos_y2, pos_z2, pos_x, pos_y + HUMAN_MAPCOLLISION_ADD_HEIGHT_A, pos_z, 0x02);
					CollisionBlockScratch(CollD, inblockdata, i, &pos_x, &pos_y, &pos_z, pos_x2, pos_y2, pos_z2, pos_x, pos_y + HUMAN_MAPCOLLISION_ADD_HEIGHT_B, pos_z, 0x02);
				}
			}
		}

		//斜面を登る
		if( (fabsf(move_x) > 0.2f)||(fabsf(move_z) > 0.2f) ){
			if( move_y_upper == 0 ){
				bool flag = false;
				for(int i=0; i<bs; i++){
					if( CheckBlockID[i] == true ){
						//足のつま先のみブロックに埋まっていれば
						if(
							(CollD->CheckBlockInside(i, pos_x + dist_x*2.0f, pos_y, pos_z + dist_z*2.0f, true, NULL) == true)&&
							(CollD->CheckBlockInside(i, pos_x + dist_x*2.0f, pos_y + HUMAN_MAPCOLLISION_CLIMBHEIGHT, pos_z + dist_z*2.0f, true, NULL) == false)
						){
							flag = true;

							int face;
							blockdata bdata;

							//足元の面の角度を取得
							//　※足元にブロックがあることは確定なので、DummyRayは必要ない・・・はず。
							if( CollD->CheckBlockIntersectRay(i, pos_x, pos_y, pos_z, 0.0f, -1.0f, 0.0f, &face, &Dist, 1.2f) == true ){

								inblockdata->Getdata(&bdata, i);

								//斜面に立っているならば、上昇無効
								if( acosf(bdata.material[face].vy) > HUMAN_MAPCOLLISION_SLOPEANGLE ){
									flag = false;
									break;
								}
							}
						}
					}
				}

				//上昇する（登る）
				if( flag == true ){
					pos_y += HUMAN_MAPCOLLISION_CLIMBADDY;
					move_y *= 0.2f;
				}
			}
		}

		//移動先がブロックへめり込んでいるなら移動を無効にする
		for(int i=0; i<bs; i++){
			if( CheckBlockID[i] == true ){
				//　※レイの始点がブロックの内側かは判定しない。
				if( CollD->CheckBlockIntersectRay(i, pos_x, pos_y + HUMAN_HEIGHT-2.0f, pos_z, 0.0f, -1.0f, 0.0f, NULL, &Dist, (HUMAN_HEIGHT-2.0f*2)) == true ){
					pos_x = pos_x2;
					pos_z = pos_z2;
				}

				//移動先がブロックにめり込む＆移動先もめり込む　ならば、移動を無効にする
				if( CollD->CheckBlockInside(i, pos_x, pos_y + HUMAN_HEIGHT-0.6f, pos_z, true, NULL) == true ){
					//メモ：↓この判定怪しい。十分に遠い位置であれば良く、11倍である必要はない？
					//　※AABBの判定範囲越えて遠いので、全部ブロック調べ直す。
					if( CollD->CheckALLBlockInside(pos_x + move_x*11.0f, pos_y + HUMAN_HEIGHT-0.6f, pos_z + move_z*11.0f) == true ){
						pos_x = pos_x2;
						pos_y = pos_y2;
						pos_z = pos_z2;
						if( move_y > 0.0f ){ move_y = 0.0f; }
					}
				}
			}
		}
	}

	if( move_y_upper > 0 ) { move_y_upper -= 1; }

	//落下処理
	move_y_flag = false;
	FallFlag = false;
	for(int ycnt=0; ycnt<3; ycnt++){
		int cnt;
		float ang = atan2f(move_z, move_x);
		float x, y, z;

		//落下
		pos_y += move_y*0.33f;

		if( player == true ){
			y = pos_y + HUMAN_MAPCOLLISION_GROUND_HEIGHT;

			//4方向判定 No.1
			cnt = 0;
			x = pos_x + cosf(ang)*HUMAN_MAPCOLLISION_GROUND_R1;
			z = pos_z + sinf(ang)*HUMAN_MAPCOLLISION_GROUND_R1;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ cnt += 1; break; }
				}
			}
			x = pos_x - cosf(ang)*HUMAN_MAPCOLLISION_GROUND_R1;
			z = pos_z - sinf(ang)*HUMAN_MAPCOLLISION_GROUND_R1;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ cnt += 1; break; }
				}
			}
			x = pos_x + cosf(ang + (float)M_PI/2)*HUMAN_MAPCOLLISION_GROUND_R1;
			z = pos_z + sinf(ang + (float)M_PI/2)*HUMAN_MAPCOLLISION_GROUND_R1;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ cnt += 1; break; }
				}
			}
			x = pos_x + cosf(ang - (float)M_PI/2)*HUMAN_MAPCOLLISION_GROUND_R1;
			z = pos_z + sinf(ang - (float)M_PI/2)*HUMAN_MAPCOLLISION_GROUND_R1;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ cnt += 1; break; }
				}
			}
			if( cnt == 4 ){ FallFlag = true; break; }

			//4方向判定 No.2
			cnt = 0;
			x = pos_x + cosf(ang)*HUMAN_MAPCOLLISION_GROUND_R2;
			z = pos_z + sinf(ang)*HUMAN_MAPCOLLISION_GROUND_R2;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ cnt += 1; break; }
				}
			}
			x = pos_x - cosf(ang)*HUMAN_MAPCOLLISION_GROUND_R2;
			z = pos_z - sinf(ang)*HUMAN_MAPCOLLISION_GROUND_R2;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ cnt += 1; break; }
				}
			}
			x = pos_x + cosf(ang + (float)M_PI/2)*HUMAN_MAPCOLLISION_GROUND_R2;
			z = pos_z + sinf(ang + (float)M_PI/2)*HUMAN_MAPCOLLISION_GROUND_R2;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ cnt += 1; break; }
				}
			}
			x = pos_x + cosf(ang - (float)M_PI/2)*HUMAN_MAPCOLLISION_GROUND_R2;
			z = pos_z + sinf(ang - (float)M_PI/2)*HUMAN_MAPCOLLISION_GROUND_R2;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ cnt += 1; break; }
				}
			}
			if( cnt == 4 ){ FallFlag = true; break; }
		}
		else{
			y = pos_y + HUMAN_MAPCOLLISION_GROUND_HEIGHT;

			//真下判定
			x = pos_x;
			z = pos_z;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ FallFlag = true; break; }
				}
			}
			if( FallFlag == true ){ break; }

			//外側へ少し外して判定
			x = pos_x + cosf(ang)*HUMAN_MAPCOLLISION_GROUND_R3;
			z = pos_z + sinf(ang)*HUMAN_MAPCOLLISION_GROUND_R3;
			for(int i=0; i<bs; i++){
				if( CheckBlockID[i] == true ){
					if( CollD->CheckBlockInside(i, x, y, z, true, NULL) == true ){ FallFlag = true; break; }
				}
			}
			if( FallFlag == true ){ break; }
		}

		//落下速度加算
		move_y -= HUMAN_DAMAGE_SPEED;
		if( move_y < HUMAN_DAMAGE_MAXSPEED ){ move_y = HUMAN_DAMAGE_MAXSPEED; }

		move_y_flag = true;
	}

	move_y_landing = false;

	//地面に接地している場合の処理
	if( FallFlag == true ){
		int id, face;
		blockdata bdata;
		bool flag = true;

		if( move_y < HUMAN_MAPCOLLISION_GROUND_HEIGHT ){ move_y_landing = true; }

		if( Invincible == false ){
			//落下ダメージを与える
			if( move_y < HUMAN_DAMAGE_MINSPEED ){
				hp -= GetRand(6) + (int)((float)HUMAN_DAMAGE_MAXFALL / fabsf(HUMAN_DAMAGE_MAXSPEED - (HUMAN_DAMAGE_MINSPEED)) * fabsf(move_y - (HUMAN_DAMAGE_MINSPEED)));
			}
		}

		move_y = 0.0f;

		if( (GetFlag(MoveFlag_lt, MOVEFLAG_JUMP))&&(move_y_upper == 0) ){
			move_y = HUMAN_JUMP_SPEED;

			//モーション実行
			MotionCtrl->Jump();
		}

		if( GetRand(4) == 0 ){
			if( (GetFlag(MoveFlag_lt, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT | MOVEFLAG_WALK)) == 0x00)&&(move_y_upper == 0) ){ flag = false; }
		}

		//足元のブロックと面番号を取得
		id = -1;
		for(int i=0; i<bs; i++){
			if( CheckBlockID[i] == true ){
				if( CollD->CheckBlockIntersectRay(i, pos_x, pos_y + 2.5f, pos_z, 0.0f, -1.0f, 0.0f, &face, &Dist, (2.5f + 1.0f)) == true ){
					id = i;
					break;
				}
			}
		}

		if( id != -1 ){
			//ブロックIDと面番号を記憶
			underblock_id = id;
			underblock_face = face;

			if( flag == true ){
				inblockdata->Getdata(&bdata, id);

				//斜面に立っているならば、斜面を滑らせる処理
				if( acosf(bdata.material[face].vy) > HUMAN_MAPCOLLISION_SLOPEANGLE ){
					move_x = bdata.material[face].vx * 1.2f;
					move_y = bdata.material[face].vy * -0.5f;
					move_z = bdata.material[face].vz * 1.2f;

					for(int i=0; i<bs; i++){
						if( CheckBlockID[i] == true ){
							if( CollD->CheckBlockInside(i, pos_x + move_x*3.0f, pos_y + move_y*3.0f, pos_z + move_z*3.0f, true, NULL) == true ){
								move_y = 0.0f;
								if( CollD->CheckBlockInside(i, pos_x + move_x*3.0f, pos_y, pos_z + move_z*3.0f, true, NULL) == true ){
									move_x = 0.0f;
									move_z = 0.0f;
									break;
								}
							}
						}
					}

					move_y_upper = 8;
				}
			}
		}
	}

	//[XOPS-ONLINE] gravity lift: hold a steady climb while inside a lift column
	if( LiftSpeed > 0.0f ){
		if( move_y < LiftSpeed ){ move_y = LiftSpeed; }
		move_y_landing = false;
		LiftSpeed = 0.0f;
	}

	//移動ベクトル再算出
	dist_x = pos_x - pos_x2;
	dist_y = pos_y - pos_y2;
	dist_z = pos_z - pos_z2;

	Dist = sqrtf(dist_x*dist_x + dist_z*dist_z);
	speed = sqrtf((move_x*2)*(move_x*2) + (move_z*2)*(move_z*2));

	//明らかに移動量がおかしいならば元に戻す
	if( (sqrtf(dist_x*dist_x + dist_y*dist_y + dist_z*dist_z) > HUMAN_MAPCOLLISION_CHECK_MAXDIST)||(Dist - speed > 1.0f) ){
		pos_x = pos_x2;
		pos_y = pos_y2;
		pos_z = pos_z2;
	}

	if( player == true ){
		speed = VectorNormalization(&dist_x, &dist_y, &dist_z);

		//移動先との間にブロックがあるならば元に戻す
		//　※レイの始点がブロックの内側かは判定しない。
		if( CollD->CheckALLBlockIntersectRay(pos_x2, pos_y2 + HUMAN_MAPCOLLISION_CHECK_HEIGHT, pos_z2, dist_x, dist_y, dist_z, NULL, NULL, &Dist, speed) == true ){
			pos_x = pos_x2;
			pos_y = pos_y2;
			pos_z = pos_z2;
		}
	}
}

//! @brief ブロックと衝突時のベクトル計算
//! @param CollD Collisionクラスのポインタ
//! @param inblockdata BlockDataInterfaceクラスのポインタ
//! @param blockid ブロックID
//! @param px X座標
//! @param py Y座標
//! @param pz Z座標
//! @param px_old 前フレームのX座標
//! @param py_old 前フレームのY座標
//! @param pz_old 前フレームのZ座標
//! @param in_vx 判定するX座標
//! @param in_vy 判定するY座標
//! @param in_vz 判定するZ座標
//! @param mode 座標補正モード（0x00：通常、0x01：Y座標上昇禁止、0x02：Y座標固定）
//! @return 成功：true　失敗：false
bool human::CollisionBlockScratch(class Collision *CollD, class BlockDataInterface *inblockdata, int blockid, float *px, float *py, float *pz, float px_old, float py_old, float pz_old, float in_vx, float in_vy, float in_vz, int mode)
{
	if( CollD == NULL ){ return false; }
	if( inblockdata == NULL ){ return false; }
	if( (blockid < 0)||(inblockdata->GetTotaldatas() <= blockid) ){ return false; }
	//if( (*px == px_old)&&(*py == py_old)&&(*pz == pz_old) ){ return false; }

	float px2, py2, pz2;
	float vx, vy, vz;
	float dist;
	int face;
	float face_vx, face_vy, face_vz, face_angle, face_angle_per;
	float temp;

	//メモ：
	//OpenXOPSの当たり判定は、計算誤差を吸収するため僅かに大きめにブロックを判定している。
	//そのため、足元のブロックのつなぎ目で引っ掛かる場合が生じる。
	//対策として、足元の判定（mode 0x00 通常）の場合は、判定するY座標を補正する。
	if( mode == 0 ){
		in_vy += COLLISION_ADDSIZE;
	}

	px2 = *px;
	py2 = *py;
	pz2 = *pz;

	//現在の移動ベクトルと距離算出
	vx = *px - px_old;
	vy = *py - py_old;
	vz = *pz - pz_old;
	dist = VectorNormalization(&vx, &vy, &vz);

	//AABBを作る
	float rmin_x, rmin_y, rmin_z, rmax_x, rmax_y, rmax_z;
	GetAABBRay(in_vx - vx*dist, in_vy - vy*dist, in_vz - vz*dist, vx, vy, vz, dist, &rmin_x, &rmin_y, &rmin_z, &rmax_x, &rmax_y, &rmax_z);

	//追突したブロック面取得
	face = -1;
	if( CollD->CheckBlockAABB(blockid, rmin_x, rmin_y, rmin_z, rmax_x, rmax_y, rmax_z) == false ){ return false; } 
	if( CollD->CheckBlockIntersectRay(blockid, in_vx - vx*dist, in_vy - vy*dist, in_vz - vz*dist, vx, vy, vz, &face, &temp, dist) == false ){ return false; }

	//ブロックの面に対するベクトルのなす角を求める
	if( CollD->AngleVector2(blockid, face, vx, vy, vz, &face_angle) == false ){ return false; }

	//角度の割合算出
	//if( face_angle != 0.0f ){ face_angle_per = 1.0f - face_angle / ((float)M_PI); }
	if( face_angle != 0.0f ){ face_angle_per = ((float)M_PI) / face_angle - 1.0f; }
	else{ face_angle_per = 0.0f; }

	//ブロック面の法線ベクトル取得
	blockdata bdata;
	inblockdata->Getdata(&bdata, blockid);
	face_vx = bdata.material[face].vx;
	face_vy = bdata.material[face].vy;
	face_vz = bdata.material[face].vz;

	//移動ベクトル＋法線ベクトルを算出
	float vx2, vy2, vz2;
	vx2 = vx + face_vx;
	vy2 = vy + face_vy;
	vz2 = vz + face_vz;
	VectorNormalization(&vx2, &vy2, &vz2);

	//なす角に応じて移動ベクトルを算出
	vx = vx2 * (1.0f-face_angle_per) + vx * face_angle_per;
	vy = vy2 * (1.0f-face_angle_per) + vy * face_angle_per;
	vz = vz2 * (1.0f-face_angle_per) + vz * face_angle_per;
	VectorNormalization(&vx, &vy, &vz);

	//水平方向に移動しないならば、法線ベクトルを用いる
	if( (vx == 0.0f)&&(vz == 0.0f) ){
		vx = bdata.material[face].vx;
		vy = bdata.material[face].vy;
		vz = bdata.material[face].vz;
	}

	//最終的な位置を計算
	temp = face_angle_per * dist;
	*px = vx*temp + px_old;
	*py = vy*temp + py_old;
	*pz = vz*temp + pz_old;

	//最終的な位置がブロックの内側ならば、移動無効
	//if( CollD->CheckALLBlockInside(*px, *py, *pz) == true ){
	if( CollD->CheckBlockInside(blockid, *px, *py, *pz, true, NULL) == true ){
		*px = px_old;
		*py = py_old;
		*pz = pz_old;
	}

	//座標補正
	if( mode == 0x01 ){
		if( *py > py2 ){ *py = py2; }	//今より上には移動させない
	}
	if( mode == 0x02 ){
		*py = py2;						//Y座標は変えない
	}

	return true;
}

//! @brief 計算を実行（当たり判定）
//! @param CollD Collisionのポインタ
//! @param inblockdata BlockDataInterfaceのポインタ
//! @param AddCollisionFlag 追加の当たり判定フラグ
//! @param player 対象の人物がプレイヤーかどうか
//! @param F5mode 上昇機能（F5裏技）のフラグ　（有効：true　無効：false）
//! @return 処理なし：0　通常処理：1　死亡して倒れ終わった直後：2　静止した死体：3　地形により死亡した直後：4
int human::ProcessObject(class Collision *CollD, class BlockDataInterface *inblockdata, bool AddCollisionFlag, bool player, bool F5mode)
{
	if( CollD == NULL ){ return 0; }
	if( inblockdata == NULL ){ return 0; }
	if( EnableFlag == false ){ return 0; }

	//F5を使用していれば、強制的に上昇
	if( F5mode == true ){
		move_y = 0.0f;
		pos_y += 5.0f;
	}

	if( deadstate == 5 ){ return 3; }

	int WeaponReloadMotionCnt;
	int WeaponChangeWeaponIDCnt;
	int CheckDead;

	//武器切り替えカウント
	if( selectweaponcnt > 0 ){
		selectweaponcnt -= 1;
	}
	if( changeweaponidcnt > 0 ){
		changeweaponidcnt -= 1;
	}

	//連射カウントが残っていれば、1 減らす
	if( weaponshotcnt > 0 ){
		weaponshotcnt -= 1;
	}

	//今回のフレームで発砲しなかったならば、バーストモード用カウントを 0 にする。
	if( weaponshotframe == false ){ weaponburstmodecnt = 0; }
	weaponshotframe = false;

	//リロードカウントが残っていれば 1 減らし、リロードカウントが 0 ならばリロード処理を実行
	if( weaponreloadcnt > 0 ){
		weaponreloadcnt -= 1;
		if( weaponreloadcnt == 0 ){
			if( weapon[selectweapon] != NULL ){ weapon[selectweapon]->RunReload(); }
		}
	}

	//リロード・武器ID切替カウント取得（モーション用）
	if( weapon[selectweapon] != NULL ){
		WeaponReloadMotionCnt = weaponreloadcnt;
		WeaponChangeWeaponIDCnt = changeweaponidcnt;
	}
	else{
		WeaponReloadMotionCnt = 0;
		WeaponChangeWeaponIDCnt = 0;
	}

	//照準の状態誤差の処理
	GunsightErrorRange();

	//死亡判定と倒れる処理
	CheckDead = CheckAndProcessDead(CollD);
	if( CheckDead == 3 ){ return 2; }

	//if( CheckDead == 0 ){
	if( (CheckDead == 0)||(deadstate == 1) ){
		int hp_old = hp;

		//進行方向と速度を決定
		ControlProcess();

		//マップとの当たり判定
		CollisionMap(CollD, inblockdata, AddCollisionFlag, player);

		//今回のマップとの当たり判定でHPがゼロになったなら、地形による死亡
		if( (hp_old > 0)&&(hp <= 0) ){
			return 4;
		}

		//今回の当たり判定で-100.0より下に落ちたら、死亡
		if( CheckDead == 0 ){
			if( pos_y < HUMAN_DEADLINE ){
				//pos_y = HUMAN_DEADLINE;
				hp = 0;
				return 4;
			}
		}
	}


	//現在装備する武器のクラスを取得
	class weapon *nowweapon;
	int weapon_paramid;
	nowweapon = weapon[selectweapon];

	if( nowweapon == NULL ){		//手ぶら
		weapon_paramid = ID_WEAPON_NONE;
	}
	else{							//何か武器を持っている
		nowweapon->GetParamData(&weapon_paramid, NULL, NULL);
	}

	//モーション計算
	MotionCtrl->ProcessObject(rotation_x, armrotation_y, weapon_paramid, WeaponReloadMotionCnt, WeaponChangeWeaponIDCnt, MoveFlag_lt, hp, player);

	if( CheckDead != 0 ){ return 3; }
	return 1;
}

//! @brief 標準誤差を取得
int human::GetGunsightErrorRange()
{
	return StateGunsightErrorRange + ReactionGunsightErrorRange;
}

//! @brief 描画
//! @param d3dg D3DGraphicsのポインタ
//! @param Resource ResourceManagerのポインタ
//! @param DrawArmOnly 腕と武器のみ描画する
//! @param player 対象の人物がプレイヤーかどうか
//! @param NoModel モデル描画無効化
//! @todo 腕の位置を行列で求める
void human::Render(class D3DGraphics *d3dg, class ResourceManager *Resource, bool DrawArmOnly, bool player, bool NoModel)
{
	//未使用引数対策
	UNREFERENCED_PARAMETER(player);

	//正しく初期化されていなければ、処理しない
	if( d3dg == NULL ){ return; }
	if( Resource == NULL ){ return; }
	if( EnableFlag == false ){ return; }

	//モーション取得
	int upmodel, armmodel, legmodel;
	float armry, legrx;
	MotionCtrl->GetRenderMotion(&armry, &legrx, &upmodel, &armmodel, &legmodel);

	//現在装備する武器のクラスを取得
	class weapon *nowweapon;
	nowweapon = weapon[selectweapon];

	if( DrawArmOnly == false ){
		float add_x, add_y, add_z;

		//上半身を描画
		if( rotation_y != 0.0f ){		//死亡して倒れている or 倒れ始めた
			add_x = cosf(rotation_x*-1 - (float)M_PI/2)*sinf(rotation_y)*-1.0f;
			add_y = cosf(rotation_y)*-1.0f;
			add_z = sinf(rotation_x*-1 - (float)M_PI/2)*sinf(rotation_y)*-1.0f;
		}
		else{
			add_x = 0.0f;
			add_y = -1.0f;
			add_z = 0.0f;
		}
		d3dg->SetWorldTransform(pos_x + add_x, pos_y + add_y, pos_z + add_z, rotation_x + (float)M_PI, rotation_y, upmodel_size);
		d3dg->RenderModel(upmodel, id_texture, DarkModelFlag, false, NoModel);

		//足を描画
		if( rotation_y != 0.0f ){		//死亡して倒れている or 倒れ始めた
			add_x = cosf(rotation_x*-1 - (float)M_PI/2)*sinf(rotation_y)*-0.2f;
			add_y = cosf(rotation_y)*-0.2f;
			add_z = sinf(rotation_x*-1 - (float)M_PI/2)*sinf(rotation_y)*-0.2f;
		}
		else{
			add_x = 0.0f;
			add_y = -0.2f;
			add_z = 0.0f;
		}
		d3dg->SetWorldTransform(pos_x + add_x, pos_y + add_y, pos_z + add_z, legrx + (float)M_PI, rotation_y, legmodel_size);
		d3dg->RenderModel(legmodel, id_texture, DarkModelFlag, false, NoModel);
	}

	//腕を描画
	if( rotation_y != 0.0f ){		//死亡して倒れている or 倒れ始めた
		float add_x = cosf(rotation_x*-1 - (float)M_PI/2)*sinf(rotation_y)*16.0f;
		float add_y = cosf(rotation_y)*16.0f;
		float add_z = sinf(rotation_x*-1 - (float)M_PI/2)*sinf(rotation_y)*16.0f;
		d3dg->SetWorldTransform(pos_x + add_x, pos_y + add_y, pos_z + add_z, rotation_x + (float)M_PI, armrotation_y + rotation_y, armmodel_size);
		d3dg->RenderModel(armmodel, id_texture, DarkModelFlag, false, NoModel);
	}
	else if( nowweapon == NULL ){	//手ぶら
		d3dg->SetWorldTransform(pos_x, pos_y + 16.0f, pos_z, rotation_x + (float)M_PI, armry, armmodel_size);
		d3dg->RenderModel(armmodel, id_texture, DarkModelFlag, false, NoModel);
	}
	else{							//何か武器を持っている
		//武器のモデルとテクスチャを取得
		int id_param;
		WeaponParameter paramdata;
		int model, texture;
		nowweapon->GetParamData(&id_param, NULL, NULL);
		Param->GetWeapon(id_param, &paramdata);
		Resource->GetWeaponModelTexture(id_param, &model, &texture);

		//腕を描画
		d3dg->SetWorldTransform(pos_x, pos_y + 16.0f, pos_z, rotation_x + (float)M_PI, armry, armmodel_size);
		d3dg->RenderModel(armmodel, id_texture, DarkModelFlag, false, NoModel);

		//武器を描画
		d3dg->SetWorldTransformHumanWeapon(pos_x, pos_y + 16.0f, pos_z, paramdata.mx/10*-1, paramdata.my/10, paramdata.mz/10*-1, rotation_x + (float)M_PI, armry, paramdata.size);
		d3dg->RenderModel(model, texture, DarkModelFlag, false, NoModel);
	}
}

//! @brief コンストラクタ
weapon::weapon(class ParameterInfo *in_Param, float x, float y, float z, float rx, int id_param, int nbs, bool flag)
{
	Param = in_Param;
	pos_x = x;
	pos_y = y;
	pos_z = z;
	move_x = 0.0f;
	move_y = 0.0f;
	move_z = 0.0f;
	rotation_x = rx;
	EnableFlag = flag;

	id_parameter = id_param;
	usingflag = false;
	bullets = nbs;
	Loadbullets = 0;
	motionflag = true;

	if( Param != NULL ){
		WeaponParameter ParamData;
		if( Param->GetWeapon(id_param, &ParamData) == 0 ){
			model_size = ParamData.size;
			//id_model = ParamData.id_model;
			//id_texture = ParamData.id_texture;
		}
	}
	id_model = -1;
	id_texture = -1;
}

//! @brief ディストラクタ
weapon::~weapon()
{}

//! @brief 座標と角度を設定
//! @param x X座標
//! @param y Y座標
//! @param z Z座標
//! @param rx 横軸回転
void weapon::SetPosData(float x, float y, float z, float rx)
{
	pos_x = x;
	pos_y = y;
	pos_z = z;
	move_x = 0.0f;
	move_y = 0.0f;
	move_z = 0.0f;
	rotation_x = rx;
}

//! @brief 設定値を設定
//! @param id_param 武器の種類番号
//! @param lnbs 装弾数
//! @param nbs 合計弾数
//! @param init オブジェクトを初期化
void weapon::SetParamData(int id_param, int lnbs, int nbs, bool init)
{
	id_parameter = id_param;
	bullets = nbs;
	Loadbullets = lnbs;

	if( init == true ){
		usingflag = false;
		motionflag = true;
	}
}

//! @brief 設定値を取得
//! @param id_param 武器の種類番号を受け取るポインタ（NULL可）
//! @param lnbs 装弾数を受け取るポインタ（NULL可）
//! @param nbs 合計弾数を受け取るポインタ（NULL可）
void weapon::GetParamData(int *id_param, int *lnbs, int *nbs)
{
	if( id_param != NULL ){ *id_param = id_parameter; }
	if( lnbs != NULL ){ *lnbs = Loadbullets; }
	if( nbs != NULL ){ *nbs = bullets; }
}

//! @brief 武器の使用状況の取得
//! @return 使用中：true　未使用：false
bool weapon::GetUsingFlag()
{
	return usingflag;
}

//! @brief 武器を拾う
//! @return 成功：0　失敗：1
int weapon::Pickup()
{
	if( usingflag == true ){ return 1; }
	usingflag = true;
	return 0;
}

//! @brief 武器を捨てる
//! @param x X座標
//! @param y Y座標
//! @param z Z座標
//! @param rx 横軸回転
//! @param speed 捨てる初速
void weapon::Dropoff(float x, float y, float z, float rx, float speed)
{
	//描画する座標と角度を設定
	move_x = cosf(rx*-1 + (float)M_PI/2) * speed;
	move_y = 0.0f;
	move_z = sinf(rx*-1 + (float)M_PI/2) * speed;
	pos_x = x + cosf(rx*-1 + (float)M_PI/2) * 5.0f;
	pos_y = y + 16.0f + move_y;
	pos_z = z + sinf(rx*-1 + (float)M_PI/2) * 5.0f;
	rotation_x = rx + (float)M_PI;

	//未使用（未装備）に設定し、座標移動を有効に
	usingflag = false;
	motionflag = true;
}

//! @brief 発砲
//! @return 成功：0　失敗：1
//! @attention 連射間隔も考慮されます。
//! @attention 関数が失敗するのは、いずれかの条件です。　「連射間隔に満たない」「リロード実行中」「弾がない」「発射弾数がない」「無効な武器の種類が設定されている」
int weapon::Shot()
{
	//クラスが設定されていなければ失敗
	if( Param == NULL ){ return 1; }

	//弾が無ければ失敗
	if( Loadbullets == 0 ){ return 1; }

	//設定値を取得
	WeaponParameter ParamData;
	int param_pellet;
	if( Param->GetWeapon(id_parameter, &ParamData) == 1 ){ return 1; }
	param_pellet = ParamData.pellet;

	//発射する弾の数（ペレット数）が0未満なら失敗
	if( param_pellet <= 0 ){ return 1; }

	//武器が手榴弾ならば～
	if( id_parameter == ID_WEAPON_GRENADE ){
		//弾を減らす
		bullets -= 1;
		Loadbullets -= 1;

		if( (bullets - Loadbullets) <= 0 ){		//（リロードしていない）弾が無くなれば、武器ごと消滅させる。
			EnableFlag = false;
			usingflag = false;
		}
		else if( Loadbullets <= 0 ){			//自動リロード
			Loadbullets = 1;
		}
		return 0;
	}

	//弾を減らす
	Loadbullets -= 1;
	bullets -= 1;

	//[OpenXOPS-web] a thrown item leaves the hand once nothing is left (like the grenade)
	if( (ParamData.throwaway == true)&&(bullets <= 0) ){
		EnableFlag = false;
		usingflag = false;
	}
	return 0;
}

//! @brief リロードを開始
//! @return 成功：0　失敗：1
//! @attention リロード時間も考慮されます。
//! @attention 関数が失敗するのは、いずれかの条件です。　「弾がない」「無効な武器の種類が設定されている」
int weapon::StartReload()
{
	//クラスが設定されていなければ失敗
	if( Param == NULL ){ return 1; }

	//弾が無ければ失敗
	if( (bullets - Loadbullets) == 0 ){ return 1; }
	return 0;
}

//! @brief リロードを実行
//! @attention StartReload()関数と異なり、瞬時に弾を補充します。リロード時間は考慮されません。
int weapon::RunReload()
{
	//クラスが設定されていなければ失敗
	if( Param == NULL ){ return 1; }

	//弾が無ければ失敗
	if( (bullets - Loadbullets) == 0 ){ return 1; }

	//武器の性能値から、装填する弾数を取得
	WeaponParameter ParamData;
	int nbsmax = 0;
	if( Param->GetWeapon(id_parameter, &ParamData) == 0 ){
		nbsmax = ParamData.nbsmax;
	}

	if( (bullets - Loadbullets) < nbsmax ){	//残りの弾数より装填する弾数が多ければ
		bullets = (bullets - Loadbullets);
		Loadbullets = bullets;
	}
	else{									//残りの弾数の方が多ければ
		bullets -= Loadbullets;
		Loadbullets = nbsmax;
	}

	return 0;
}

//! @brief 武器の種類・装弾数の変更
//! @param Resource ResourceManagerのポインタ
//! @param id_param 種類番号
//! @param lnbs 装弾数
//! @param nbs 合計弾数
//! @return 成功：true　失敗：false
//! @attention プレイヤーによる裏技（F6・F7）用に用意された関数です。手榴弾が選択された場合、自動的に弾を補充します。
//! @attention 使用されていない武器オブジェクトに対して実行すると、この関数は失敗します。
bool weapon::ResetWeaponParam(class ResourceManager *Resource, int id_param, int lnbs, int nbs)
{
	//初期化されていなければ、失敗
	if( EnableFlag == false ){ return false; }
	if( Resource == NULL ){ return false; }

	//指定された設定値へ上書き
	id_parameter = id_param;
	bullets = nbs;
	Loadbullets = lnbs;

	//もし手榴弾ならば、自動リロード
	if( id_param == ID_WEAPON_GRENADE ){
		if( (bullets > 0)&&(Loadbullets == 0) ){
			Loadbullets = 1;
		}
	}

	//モデルとテクスチャを設定
	Resource->GetWeaponModelTexture(id_param, &id_model, &id_texture);
	WeaponParameter param;
	if( Param->GetWeapon(id_param, &param) == 0 ){
		model_size = param.size;
	}

	return true;
}

//! @brief 計算を実行（自由落下）
//! @param CollD Collisionのポインタ
int weapon::ProcessObject(class Collision *CollD)
{
	//クラスが設定されていなければ失敗
	if( CollD == NULL ){ return 0; }

	//初期化されていなければ、失敗
	if( EnableFlag == false ){ return 0; }

	//誰にも使われておらず、移動フラグが有効ならば～
	if( (usingflag == false)&&(motionflag == true) ){
		float Dist;
		float maxDist;

		//移動速度を更新
		move_x *= 0.96f;
		move_z *= 0.96f;
		if( move_y > -1.8f ){
			move_y -= 0.3f;
		}

		//ブロックに埋まっていれば処理しない
		if( CollD->CheckALLBlockInside(pos_x, pos_y, pos_z) == true ){
			motionflag = false;
			return 0;
		}

		//水平の移動速度を求める
		maxDist = sqrtf(move_x*move_x + move_z*move_z);
		if( maxDist < 0.1f ){
			maxDist = 0.0f;
			move_x = 0.0f;
			move_z = 0.0f;
		}

		//ブロックに接していれば、それ以上は水平移動しない。
		if( maxDist > 0.0f ){
			if( CollD->CheckALLBlockIntersectDummyRay(pos_x, pos_y, pos_z, move_x/maxDist, 0, move_z/maxDist, NULL, NULL, &Dist, maxDist) == true ){
				//ブロックに埋まらないように手前に
				Dist -= 0.1f;

				//接している座標まで移動
				pos_x += move_x/maxDist * Dist;
				pos_z += move_z/maxDist * Dist;

				//移動量を 0 に
				move_x = 0.0f;
				move_z = 0.0f;
			}
		}

		//落下処理
		for(int i=0; i<2; i++){
			//ブロックに埋まっていれば、そこで処理終了
			if( CollD->CheckALLBlockInside(pos_x, pos_y + move_y*0.5f, pos_z) == true ){
				motionflag = false;
				return 0;
			}

			//落下させる
			pos_y += move_y*0.5f;
		}

		//水平方向の座標を反映
		pos_x += move_x;
		pos_z += move_z;
	}

	return 0;
}

//! @brief 描画
//! @param d3dg D3DGraphicsのポインタ
//! @param NoModel モデル描画無効化
void weapon::Render(class D3DGraphics *d3dg, bool NoModel)
{
	//クラスが設定されていなければ失敗
	if( d3dg == NULL ){ return; }

	//初期化されてないか、誰かに使われていれば処理しない
	if( EnableFlag == false ){ return; }
	if( usingflag == true ){ return; }

	//武器を描画
	d3dg->SetWorldTransform(pos_x, pos_y, pos_z, rotation_x, 0.0f, (float)M_PI/2, model_size);
	d3dg->RenderModel(id_model, id_texture, DarkModelFlag, false, NoModel);
}

//! @brief コンストラクタ
smallobject::smallobject(class ParameterInfo *in_Param, class MIFInterface *in_MIFdata, float x, float y, float z, float rx, int id_param, signed short int p4, bool flag)
{
	Param = in_Param;
	MIFdata = in_MIFdata;
	pos_x = x;
	pos_y = y;
	pos_z = z;
	rotation_x = rx;
	rotation_y = 0.0f;
	EnableFlag = flag;
	model_size = 5.0f;

	id_parameter = id_param;
	point_p4 = p4;

	hp = 0;		//暫定
	jump_rx = 0.0f;
	move_rx = 0.0f;
	add_rx = 0.0f;
	add_ry = 0.0f;
	jump_cnt = 0;

	if( Param != NULL ){
		SmallObjectParameter ParamData;
		if( Param->GetSmallObject(id_param, &ParamData) == 0 ){
			hp = ParamData.hp;
		}
	}
	id_model = -1;
	id_texture = -1;
}

//! @brief ディストラクタ
smallobject::~smallobject()
{}

//! @brief MIFデータを管理するクラスを設定
//! @param in_MIFdata MIFInterfaceクラスのポインタ
void smallobject::SetMIFInterfaceClass(class MIFInterface *in_MIFdata)
{
	MIFdata = in_MIFdata;
}

//! @brief 設定値を設定
//! @param id_param 小物の種類番号
//! @param p4 第4パラメータ
//! @param init オブジェクトを初期化
void smallobject::SetParamData(int id_param, signed short int p4, bool init)
{
	rotation_y = 0.0f;
	id_parameter = id_param;
	point_p4 = p4;

	if( init == true ){
		hp = 0;		//暫定
		jump_rx = 0.0f;
		move_rx = 0.0f;
		add_rx = 0.0f;
		add_ry = 0.0f;
		jump_cnt = 0;

		if( (TOTAL_PARAMETERINFO_SMALLOBJECT <= id_param)&&(id_param <= (TOTAL_PARAMETERINFO_SMALLOBJECT + MAX_ADDSMALLOBJECT -1)) ){
			if( MIFdata != NULL ){
				hp = MIFdata->GetAddSmallobjectHP(id_param - TOTAL_PARAMETERINFO_SMALLOBJECT);
			}
		}
		else{
			if( Param != NULL ){
				SmallObjectParameter ParamData;
				if( Param->GetSmallObject(id_param, &ParamData) == 0 ){
					hp = ParamData.hp;
				}
			}
		}
	}
}

//! @brief 設定値を取得
//! @param id_param 小物の種類番号を受け取るポインタ（NULL可）
//! @param p4 第4パラメータを受け取るポインタ（NULL可）
void smallobject::GetParamData(int *id_param, signed short int *p4)
{
	if( id_param != NULL ){ *id_param = id_parameter; }
	if( p4 != NULL ){ *p4 = point_p4; }
}

//! @brief 体力を取得
//! @return 体力値
int smallobject::GetHP()
{
	return hp;
}

//! @brief ブロックの上に移動
//! @param CollD Collisionのポインタ
//! @return 元の座標からの移動量（0で移動なし）
//! @attention ブロックに埋まっている場合でも、下のブロックまで すり抜けます。
float smallobject::CollisionMap(class Collision *CollD)
{
	//クラスが設定されていなければ失敗
	if( CollD == NULL ){ return 0.0f; }

	float Dist;
	SmallObjectParameter ParamData;

	//下方向に当たり判定
	if( CollD->CheckALLBlockIntersectRay(pos_x, pos_y + COLLISION_ADDSIZE, pos_z, 0, -1, 0, NULL, NULL, &Dist, 1000.0f) == true ){
		//当たり判定の大きさを取得
		if( (TOTAL_PARAMETERINFO_SMALLOBJECT <= id_parameter)&&(id_parameter <= (TOTAL_PARAMETERINFO_SMALLOBJECT + MAX_ADDSMALLOBJECT -1)) ){
			Dist -= (float)MIFdata->GetAddSmallobjectDecide(id_parameter - TOTAL_PARAMETERINFO_SMALLOBJECT)/10.0f;
		}
		else{
			if( Param->GetSmallObject(id_parameter, &ParamData) == 0 ){
				Dist -= (float)ParamData.decide/10.0f;
			}
		}

		//座標を移動する
		pos_y -= Dist;
		return Dist;
	}

	return 0.0f;
}

//! @brief 弾がヒットした
//! @param attacks 弾の攻撃力
void smallobject::HitBullet(int attacks)
{
	hp -= attacks;
	if( hp <= 0 ){
		Destruction();
	}
}

//! @brief 手榴弾の爆風がヒットした
//! @param attacks 爆風の攻撃力
//! @attention 距離による計算を事前に済ませてください。
void smallobject::HitGrenadeExplosion(int attacks)
{
	hp -= attacks;
	if( hp <= 0 ){
		Destruction();
	}
}

//! @brief 小物を破壊する
//! @attention 通常は HitBullet()関数 および GrenadeExplosion()関数 から自動的に実行されるため、直接呼び出す必要はありません。
void smallobject::Destruction()
{
	//EnableFlag = false;
	//return;

	int jump;

	//飛び具合を取得
	if( (TOTAL_PARAMETERINFO_SMALLOBJECT <= id_parameter)&&(id_parameter <= (TOTAL_PARAMETERINFO_SMALLOBJECT + MAX_ADDSMALLOBJECT -1)) ){
		jump = MIFdata->GetAddSmallobjectJump(id_parameter - TOTAL_PARAMETERINFO_SMALLOBJECT);
	}
	else{
		SmallObjectParameter paramdata;
		Param->GetSmallObject(id_parameter, &paramdata);
		jump = paramdata.jump;
	}

	//飛ばす
	hp = 0;
	jump_cnt = jump;

	//姿勢設定
	jump_rx = DegreeToRadian(10) * GetRand(36);
	move_rx = (float)jump * 0.04243f;
	add_rx = DegreeToRadian(1) * GetRand(20);
	add_ry = DegreeToRadian(1) * GetRand(20);
}

//! @brief 計算を実行（破壊時の移動など）
int smallobject::ProcessObject()
{
	//描画されていないか、体力が残っていなければ処理しない。
	if( EnableFlag == false ){ return 0; }
	if( hp > 0 ){ return 0; }

	int cnt;
	int jump;

	//飛び具合を取得
	if( (TOTAL_PARAMETERINFO_SMALLOBJECT <= id_parameter)&&(id_parameter <= (TOTAL_PARAMETERINFO_SMALLOBJECT + MAX_ADDSMALLOBJECT -1)) ){
		jump = MIFdata->GetAddSmallobjectJump(id_parameter - TOTAL_PARAMETERINFO_SMALLOBJECT);
	}
	else{
		SmallObjectParameter paramdata;
		Param->GetSmallObject(id_parameter, &paramdata);
		jump = paramdata.jump;
	}
	
	//吹き飛んでいるカウント数を計算
	cnt = jump - jump_cnt;

	//姿勢から座標・角度を計算
	pos_x += cosf(jump_rx) * move_rx;
	pos_y += jump_cnt * 0.1f;
	pos_z += sinf(jump_rx) * move_rx;
	rotation_x += add_rx;
	rotation_y += add_ry;

	jump_cnt -= 1;

	//1秒飛んでいたら描画終了
	if( cnt > (int)GAMEFPS ){
		EnableFlag = false;
		return 2;
	}

	return 1;
}

//! @brief 描画
//! @param d3dg D3DGraphicsのポインタ
//! @param NoModel モデル描画無効化
void smallobject::Render(D3DGraphics *d3dg, bool NoModel)
{
	//クラスが設定されていなければ失敗
	if( d3dg == NULL ){ return; }

	//初期化されていなければ処理しない。
	if( EnableFlag == false ){ return; }

	//描画
	d3dg->SetWorldTransform(pos_x, pos_y, pos_z, rotation_x, rotation_y, model_size);
	d3dg->RenderModel(id_model, id_texture, DarkModelFlag, false, NoModel);
}

//! @brief コンストラクタ
bullet::bullet(int modelid, int textureid)
{
	model_size = 1.0f;
	id_model = modelid;
	id_texture = textureid;
	EnableFlag = false;

	rotation_y = 0.0f;
	attacks = 0;
	penetration = 0;
	speed = 0.0f;
	teamid = 0;
	humanid = 0;
	ontargetcnt = 0.0f;
	cnt = 0;
}

//! @brief ディストラクタ
bullet::~bullet()
{}

//! @brief 座標と角度を設定
//! @param x X座標
//! @param y Y座標
//! @param z Z座標
//! @param rx 横軸回転
//! @param ry 縦軸回転
void bullet::SetPosData(float x, float y, float z, float rx, float ry)
{
	pos_x = x;
	pos_y = y;
	pos_z = z;
	rotation_x = rx;
	rotation_y = ry;
}

//! @brief 設定値を設定
//! @param _attacks 攻撃力
//! @param _penetration 貫通力
//! @param _speed 弾速
//! @param _teamid チーム番号
//! @param _humanid 人のデータ番号
//! @param _ontargetcnt 命中時のカウント数
//! @param init オブジェクトを初期化
void bullet::SetParamData(int _attacks, int _penetration, float _speed, int _teamid, int _humanid, float _ontargetcnt, bool init)
{
	attacks = _attacks;
	penetration = _penetration;
	speed = _speed;
	teamid = _teamid;
	humanid = _humanid;
	ontargetcnt = _ontargetcnt;

	if( init == true ){
		cnt = 0;
	}
}

//! @brief 座標と角度を取得
//! @param x X座標を受け取るポインタ（NULL可）
//! @param y Y座標を受け取るポインタ（NULL可）
//! @param z Z座標を受け取るポインタ（NULL可）
//! @param rx 横軸回転を受け取るポインタ（NULL可）
//! @param ry 縦軸回転を受け取るポインタ（NULL可）
void bullet::GetPosData(float *x, float *y, float *z, float *rx, float *ry)
{
	if( x != NULL ){ *x = pos_x; }
	if( y != NULL ){ *y = pos_y; }
	if( z != NULL ){ *z = pos_z; }
	if( rx != NULL ){ *rx = rotation_x; }
	if( ry != NULL ){ *ry = rotation_y; }
}

//! @brief 設定値を取得
//! @param _attacks 攻撃力を受け取るポインタ（NULL可）
//! @param _penetration 貫通力を受け取るポインタ（NULL可）
//! @param _speed 弾速を受け取るポインタ（NULL可）
//! @param _teamid チーム番号を受け取るポインタ（NULL可）
//! @param _humanid 人のデータ番号を受け取るポインタ（NULL可）
//! @param _ontargetcnt 命中時のカウント数を受け取るポインタ（NULL可）
void bullet::GetParamData(int *_attacks, int *_penetration, float *_speed, int *_teamid, int *_humanid, float *_ontargetcnt)
{
	if( _attacks != NULL ){ *_attacks = attacks; }
	if( _penetration != NULL ){ *_penetration = penetration; }
	if( _speed != NULL ){ *_speed = speed; }
	if( _teamid != NULL ){ *_teamid = teamid; }
	if( _humanid != NULL ){ *_humanid = humanid; }
	if( _ontargetcnt != NULL ){ *_ontargetcnt = ontargetcnt; }
}

//! @brief 計算を実行（弾の進行・時間消滅）
int bullet::ProcessObject()
{
	//初期化されていなければ処理しない
	if( EnableFlag == false ){ return 0; }

	//消滅時間を過ぎていれば、オブジェクトを無効化
	if( cnt > BULLET_DESTROYFRAME ){
		EnableFlag = false;
		return 0;
	}

	//移動処理
	pos_x += cosf(rotation_x)*cosf(rotation_y)*speed;
	pos_y += sinf(rotation_y)*speed;
	pos_z += sinf(rotation_x)*cosf(rotation_y)*speed;
	cnt += 1;

	return 0;
}

//! @brief 描画
//! @param d3dg D3DGraphicsのポインタ
//! @param NoModel モデル描画無効化
void bullet::Render(class D3DGraphics *d3dg, bool NoModel)
{
	//クラスが設定されていなければ失敗
	if( d3dg == NULL ){ return; }

	//初期化されていなければ処理しない。
	if( EnableFlag == false ){ return; }

	//弾を移動前だったら描画しない
	//　弾が頭から突き抜けて見える対策
	if( cnt == 0 ){ return; }

	//描画
	d3dg->SetWorldTransform(pos_x, pos_y, pos_z, (rotation_x * -1 - (float)M_PI/2), rotation_y, model_size);
	d3dg->RenderModel(id_model, id_texture, false, false, NoModel);
}

//! @brief コンストラクタ
grenade::grenade(int modelid, int textureid) : bullet(modelid, textureid)
{
	if( Param != NULL ){
		WeaponParameter ParamData;
		if( Param->GetWeapon(ID_WEAPON_GRENADE, &ParamData) == 0 ){
			model_size = ParamData.size;
		}
	}

	move_x = 0.0f;
	move_y = 0.0f;
	move_z = 0.0f;
}

//! @brief ディストラクタ
grenade::~grenade()
{}

//! @brief 座標と情報を設定
//! @param speed 初速
//! @param _teamid チーム番号
//! @param _humanid 人のデータ番号
//! @param _ontargetcnt 命中時のカウント数
//! @param init オブジェクトを初期化
//! @attention 先に SetPosData() を実行してください。
void grenade::SetParamData(float speed, int _teamid, int _humanid, float _ontargetcnt, bool init)
{
	move_x = cosf(rotation_x) * cosf(rotation_y) * speed;
	move_y = sinf(rotation_y) * speed;
	move_z = sinf(rotation_x) * cosf(rotation_y) * speed;
	teamid = _teamid;
	humanid = _humanid;
	ontargetcnt = _ontargetcnt;

	if( init == true ){
		cnt = 0;
	}
}

//! @brief 設定値を取得
//! @param _speed 速度を受け取るポインタ（NULL可）
//! @param _teamid チーム番号を受け取るポインタ（NULL可）
//! @param _humanid 人のデータ番号を受け取るポインタ（NULL可）
//! @param _ontargetcnt 命中時のカウント数を受け取るポインタ（NULL可）
void grenade::GetParamData(float *_speed, int *_teamid, int *_humanid, float *_ontargetcnt)
{
	if( _speed != NULL ){ *_speed = GetSpeed(); }
	if( _teamid != NULL ){ *_teamid = teamid; }
	if( _humanid != NULL ){ *_humanid = humanid; }
	if( _ontargetcnt != NULL ){ *_ontargetcnt = ontargetcnt; }
}

//! @brief 速度を取得
//! @return 速度
float grenade::GetSpeed()
{
	return sqrtf(move_x*move_x + move_y*move_y + move_z*move_z);
}

//! @brief 計算を実行（手榴弾の進行・爆発）
//! @return 爆発：2　バウンド・跳ね返り：1　それ以外：0
int grenade::ProcessObject(class Collision *CollD)
{
	if( CollD == NULL ){ return 0; }

	//初期化されていなければ処理しない
	if( EnableFlag == false ){ return 0; }

	//時間を過ぎていれば、オブジェクトを無効化し、「爆発」として返す。
	if( cnt > GRENADE_DESTROYFRAME ){
		EnableFlag = false;
		return 2;
	}

	//静止していれば処理しない
	if( (move_x == 0.0f)&&(move_y == 0.0f)&&(move_z == 0.0f) ){
		cnt += 1;
		return 0;
	}

	int id, face;
	float Dist;
	float maxDist = sqrtf(move_x*move_x + move_y*move_y + move_z*move_z);

	//マップに対して当たり判定を実行
	if( CollD->CheckALLBlockIntersectRay(pos_x, pos_y, pos_z, move_x/maxDist, move_y/maxDist, move_z/maxDist, &id, &face, &Dist, maxDist) == true ){
		//衝突している場合～

		float Angle, Acceleration;
		float vx, vy, vz;

		//角度を求める
		CollD->AngleVector(id, face, move_x, move_y, move_z, &Angle);

		//反射するベクトルを求める
		CollD->ReflectVector(id, face, move_x, move_y, move_z, &vx, &vy, &vz);

		//減速率を求める
		Acceleration = Angle*-1 * 0.2546f + 0.7f;

		//反射 and 減速
		move_x = vx * Acceleration;
		move_y = vy * Acceleration;
		move_z = vz * Acceleration;

		//　※衝突した瞬間（フレーム）は、移動しない。

		//移動速度を計算
		move_x *= 0.98f;
		move_y = (move_y - 0.17f) * 0.98f;
		move_z *= 0.98f;

		cnt += 1;
		return 1;
	}

	//座標を移動
	pos_x += move_x;
	pos_y += move_y;
	pos_z += move_z;

	//移動速度を計算
	move_x *= 0.98f;
	move_y = (move_y - 0.17f) * 0.98f;
	move_z *= 0.98f;

	cnt += 1;
	return 0;
}

//! @brief 描画
//! @param d3dg D3DGraphicsのポインタ
//! @param NoModel モデル描画無効化
void grenade::Render(class D3DGraphics *d3dg, bool NoModel)
{
	//クラスが設定されていなければ失敗
	if( d3dg == NULL ){ return; }

	//初期化されていなければ処理しない。
	if( EnableFlag == false ){ return; }

	//描画
	d3dg->SetWorldTransform(pos_x, pos_y, pos_z, (rotation_x * -1 - (float)M_PI/2), 0.0f, (float)M_PI/2, model_size);
	d3dg->RenderModel(id_model, id_texture, DarkModelFlag, false, NoModel);
}

//! @brief コンストラクタ
effect::effect(float x, float y, float z, float size, float rotation, int texture, int count)
{
	pos_x = x;
	pos_y = y;
	pos_z = z;
	move_x = 0.0f;
	move_y = 0.0f;
	move_z = 0.0f;
	addmove_y = 0.0f;
	rotation_x = 0.0f;
	rotation_y = 0.0f;
	rotation_texture = rotation;
	addrotation_texture = 0.0f;
	model_size = size;
	addmodel_size = 0.0f;
	id_texture = texture;
	brightness = 0;
	addbrightness = 0;
	alpha = 1.0f;
	add_alpha = 0.0f;
	cnt = count;
	type = EFFECT_NORMAL;
	if( cnt > 0 ){
		EnableFlag = true;
	}
	else{
		EnableFlag = false;
	}
}

//! @brief ディストラクタ
effect::~effect()
{}

//! @brief 設定値を設定
//! @param in_move_x X軸移動量
//! @param in_move_y Y軸移動量
//! @param in_move_z Z軸移動量
//! @param in_addmove_y Y軸移動量の加算量
//! @param rotation 回転角度
//! @param addrotation 回転角度の加算量
//! @param size 描画倍率
//! @param addsize 描画倍率
//! @param count カウント
//! @param settype エフェクトの種類　（Effect_Type を組み合せる）
void effect::SetParamData(float in_move_x, float in_move_y, float in_move_z, float in_addmove_y, float rotation, float addrotation, float size, float addsize, int count, int settype)
{
	move_x = in_move_x;
	move_y = in_move_y;
	move_z = in_move_z;
	addmove_y = in_addmove_y;
	rotation_x = addrotation;
	rotation_y = 0.0f;
	rotation_texture = rotation;
	addrotation_texture = addrotation;
	model_size = size;
	addmodel_size = addsize;
	cnt = count;
	type = settype;
}

//! @brief テクスチャを設定
//! @param id テクスチャのデータ番号
//! @param in_brightness 明るさ
//! @param in_addbrightness 明るさの加算量
//! @param in_alpha 透明度
//! @param in_addalpha 透明度の加算量
void effect::SetTexture(int id, int in_brightness, int in_addbrightness, float in_alpha, float in_addalpha)
{
	id_texture = id;
	brightness = in_brightness;
	addbrightness = in_addbrightness;
	alpha = in_alpha;
	add_alpha = in_addalpha;
}

//! @brief 横軸と縦軸の向きを設定
//! @param rx 設定する横軸
//! @param ry 設定する縦軸
void effect::SetRxRy(float rx, float ry)
{
	rotation_x = rx;
	rotation_y = ry;
}

//! テクスチャのデータ番号を取得
//! @return テクスチャのデータ番号
int effect::GetTextureID()
{
	return id_texture;
}

//! エフェクトの移動量を取得
//! @param mx X軸移動量を受け取るポインタ
//! @param my Y軸移動量を受け取るポインタ
//! @param mz Z軸移動量を受け取るポインタ
void effect::GetMove(float *mx, float *my, float *mz)
{
	if( (mx == NULL)||(my == NULL)||(mz == NULL) ){ return; }

	*mx = move_x;
	*my = move_y;
	*mz = move_z;
}

//! エフェクトのマップ当たり判定フラグ取得
//! @return 有効：true　無効：false
bool effect::GetCollideMapFlag()
{
	if( type&EFFECT_COLLIDEMAP ){
		return true;
	}
	//else{
		return false;
	//}
}

//! @brief 計算を実行（エフェクト移動・時間消滅）
//! @return 処理実行：1　描画最終フレーム：2　処理なし：0
int effect::ProcessObject()
{
	//初期化されていなければ処理しない
	if( EnableFlag == false ){ return 0; }

	//カウントが終了していれば、処理しないように設定
	if( cnt <= 0 ){
		EnableFlag = false;
		return 2;
	}

	//座標移動
	pos_x += move_x;
	pos_y += move_y;
	pos_z += move_z;
	move_y += addmove_y;

	//回転
	rotation_texture += addrotation_texture;

	//拡大
	model_size += addmodel_size;

	//明るさと透明度
	brightness += addbrightness;
	alpha += add_alpha;

	//透明度がマイナスになったら、処理しないように設定
	if( alpha <= 0.0f ){
		EnableFlag = false;
		cnt = 0;
		return 2;
	}

	//カウントが 255 未満ならば、カウントを 1 引く
	if( cnt < 255 ){
		cnt -= 1;
	}
	return 1;
}

//! @brief 描画
//! @param d3dg D3DGraphicsのポインタ
//! @param camera_rx カメラの横軸角度
//! @param camera_ry カメラの縦軸角度
//! @param NoModel モデル描画無効化
void effect::Render(class D3DGraphics *d3dg, float camera_rx, float camera_ry, bool NoModel)
{
	//クラスが設定されていなければ失敗
	if( d3dg == NULL ){ return; }

	//初期化されていなければ処理しない。
	if( EnableFlag == false ){ return; }

	if( (type & EFFECT_NOBILLBOARD) == 0 ){
		//カメラ座標を適用し、ビルボード化する
		rotation_x = camera_rx*-1;
		rotation_y = camera_ry;
	}

	//描画
	d3dg->SetWorldTransformEffect(pos_x, pos_y, pos_z, rotation_x, rotation_y, rotation_texture, model_size);
	d3dg->RenderBoard(id_texture, alpha, NoModel);
}

//! @brief コンストラクタ
HumanMotionControl::HumanMotionControl(class ParameterInfo *in_Param)
{
	Param = in_Param;

	id_upmodel = -1;
	for(int i=0; i<TOTAL_ARMMODE; i++){
		id_armmodel[i] = -1;
	}
	id_legmodel = -1;
	for(int i=0; i<TOTAL_WALKMODE; i++){
		id_walkmodel[i] = -1;
	}
	for(int i=0; i<TOTAL_RUNMODE; i++){
		id_runmodel[i] = -1;
	}

	reaction_y = 0.0f;
	slowarm = false;
	armmodel_rotation_y = 0.0f;
	legrotation_x = 0.0f;
	walkcnt = 0;
	runcnt = 0;
	armmodelid = -1;
	legmodelid = -1;
}

//! @brief ディストラクタ
HumanMotionControl::~HumanMotionControl()
{}

//! @brief 設定値を管理するクラスを登録
//! @attention 各関数を使用する前に実行すること。
void HumanMotionControl::SetParameterInfoClass(class ParameterInfo *in_Param)
{
	Param = in_Param;
}

//! @brief モデルデータを設定
//! @param upmodel 上半身のモデル
//! @param armmodel[] 腕のモデルの配列（配列数：TOTAL_ARMMODE）
//! @param legmodel 足（静止状態）のモデル
//! @param walkmodel[] 腕のモデルの配列（配列数：TOTAL_WALKMODE）
//! @param runmodel[] 腕のモデルの配列（配列数：TOTAL_RUNMODE）
void HumanMotionControl::SetModel(int upmodel, int armmodel[], int legmodel, int walkmodel[], int runmodel[])
{
	id_upmodel = upmodel;
	for(int i=0; i<TOTAL_ARMMODE; i++){
		id_armmodel[i] = armmodel[i];
	}
	id_legmodel = legmodel;
	for(int i=0; i<TOTAL_WALKMODE; i++){
		id_walkmodel[i] = walkmodel[i];
	}
	for(int i=0; i<TOTAL_RUNMODE; i++){
		id_runmodel[i] = runmodel[i];
	}
}

//! @brief 初期化系関数
//! @param rx 回転角度
void HumanMotionControl::Init(float rx)
{
	reaction_y = 0.0f;
	slowarm = false;
	armmodel_rotation_y = 0.0f;
	legrotation_x = rx;
	walkcnt = 0;
	runcnt = 0;
	armmodelid = -1;
	legmodelid = -1;
}

//! @brief 武器を拾う
//! @param weapon_paramid 武器の種類番号
void HumanMotionControl::PickupWeapon(int weapon_paramid)
{
	//未使用引数対策
	UNREFERENCED_PARAMETER(weapon_paramid);

	//腕の角度（反動）を設定
	reaction_y = DegreeToRadian(-20);
	slowarm = true;
}

//! @brief 武器を切り替える（持ち替える）
//! @param weapon_paramid 武器の種類番号
void HumanMotionControl::ChangeHaveWeapon(int weapon_paramid)
{
	//未使用引数対策
	UNREFERENCED_PARAMETER(weapon_paramid);

	//腕の角度（反動）を設定
	reaction_y = DegreeToRadian(-20);
	slowarm = true;
}

//! @brief 武器の種類切り替え
//! @param weapon_paramid 武器の種類番号
//! @attention この関数は拡張用のダミー関数です。
void HumanMotionControl::ChangeWeaponID(int weapon_paramid)
{
	//未使用引数対策
	UNREFERENCED_PARAMETER(weapon_paramid);
}

//! @brief 発砲処理
//! @param weapon_paramid 武器の種類番号
void HumanMotionControl::ShotWeapon(int weapon_paramid)
{
	//腕に反動を伝える
	if( weapon_paramid == ID_WEAPON_GRENADE ){
		reaction_y = DegreeToRadian(20);
	}
	else{
		//武器の設定値を取得
		WeaponParameter ParamData;
		ScopeParameter sparam;
		if( Param->GetWeapon(weapon_paramid, &ParamData) != 0 ){ return; }
		Param->GetScopeParam(ParamData.scopemode, &sparam);

		if( sparam.WeaponMotionReactionY >= 0.0f ){
			reaction_y = sparam.WeaponMotionReactionY;
		}
		else{
			reaction_y = DegreeToRadian(0.5f) * ParamData.reaction;
		}
	}
	slowarm = false;
}

//! @brief リロード
//! @param weapon_paramid 武器の種類番号
//! @attention この関数は拡張用のダミー関数です。
void HumanMotionControl::ReloadWeapon(int weapon_paramid)
{
	//未使用引数対策
	UNREFERENCED_PARAMETER(weapon_paramid);
}

//! @brief 武器を捨てる
//! @attention この関数は拡張用のダミー関数です。
void HumanMotionControl::DumpWeapon()
{
	//
}

//! @brief ジャンプ
//! @attention この関数は拡張用のダミー関数です。
void HumanMotionControl::Jump()
{
	//
}

//! @brief モーション計算を実行
//! @attention rotation_x 体全体の回転角度
//! @attention armrotation_y 腕の回転角度
//! @attention weapon_paramid 武器の種類番号
//! @attention ReloadCnt 武器のリロードカウント
//! @attention ChangeWeaponIDCnt 武器IDの切替カウント
//! @attention MoveFlag 移動方向を表すフラグ
//! @attention hp 体力
//! @attention PlayerFlag プレイヤーかどうか
void HumanMotionControl::ProcessObject(float rotation_x, float armrotation_y, int weapon_paramid, int ReloadCnt, int ChangeWeaponIDCnt, int MoveFlag, int hp, bool PlayerFlag)
{
	int ArmModelID;
	float move_rx;

	if( slowarm == false ){
		//発砲などによる反動
		if( reaction_y > 0.0f ){
			if( reaction_y > DegreeToRadian(0.01f) ){ reaction_y *= 0.5f; }
			else{ reaction_y = 0.0f; }
		}
		if( reaction_y < 0.0f ){
			if( reaction_y < DegreeToRadian(-0.01f) ){ reaction_y *= 0.5f; }
			else{ reaction_y = 0.0f; }
		}
	}
	else{
		if( (DegreeToRadian(-2.0f) < reaction_y)&&(reaction_y < DegreeToRadian(2.0f)) ){
			reaction_y = 0.0f;
			slowarm = false;
		}
		if( reaction_y > 0.0f ){ reaction_y += DegreeToRadian(-2.0f); }
		if( reaction_y < 0.0f ){ reaction_y += DegreeToRadian(2.0f); }
	}

	//リロード中なら腕の角度を再設定
	if( ReloadCnt > 0 ){
		reaction_y = ARMRAD_RELOADWEAPON;
	}

	//武器ID切替中なら腕の角度を再設定
	if( ChangeWeaponIDCnt > 0 ){
		reaction_y = ARMRAD_RELOADWEAPON;
	}

	if( weapon_paramid == ID_WEAPON_NONE ){	//手ぶら
		if( PlayerFlag == true ){
			armmodel_rotation_y = ARMRAD_NOWEAPON;
		}
		else{
			armmodel_rotation_y = armrotation_y;
		}
		ArmModelID = 0;
	}
	else{							//何か武器を持っている
		//武器の情報を取得
		WeaponParameter paramdata;
		Param->GetWeapon(weapon_paramid, &paramdata);

		armmodel_rotation_y = 0.0f;
		ArmModelID = 0;

		//腕の形と角度を決定
		if( paramdata.WeaponP == 0 ){
			armmodel_rotation_y = armrotation_y + reaction_y;
			ArmModelID = 1;
		}
		if( paramdata.WeaponP == 1 ){
			armmodel_rotation_y = armrotation_y + reaction_y;
			ArmModelID = 2;
		}
		if( paramdata.WeaponP == 2 ){
			armmodel_rotation_y = ARMRAD_NOWEAPON;
			ArmModelID = 0;
		}
	}

	//進行方向を決定
	if( GetFlag(MoveFlag, MOVEFLAG_WALK) ){
		move_rx = DegreeToRadian(0);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == MOVEFLAG_FORWARD ){
		move_rx = DegreeToRadian(0);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == MOVEFLAG_BACK ){
		move_rx = DegreeToRadian(180);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == MOVEFLAG_LEFT ){
		move_rx = DegreeToRadian(90);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == MOVEFLAG_RIGHT ){
		move_rx = DegreeToRadian(-90);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == (MOVEFLAG_FORWARD | MOVEFLAG_LEFT) ){
		move_rx = DegreeToRadian(45);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == (MOVEFLAG_BACK | MOVEFLAG_LEFT) ){
		move_rx = DegreeToRadian(135);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == (MOVEFLAG_BACK | MOVEFLAG_RIGHT) ){
		move_rx = DegreeToRadian(-135);
	}
	else if( GetFlag(MoveFlag, (MOVEFLAG_FORWARD | MOVEFLAG_BACK | MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) == (MOVEFLAG_FORWARD | MOVEFLAG_RIGHT) ){
		move_rx = DegreeToRadian(-45);
	}
	else{
		move_rx = 0.0f;
	}

	//足の角度を算出
	if( hp <= 0 ){
		legrotation_x = rotation_x;
	}
	else{
		float move_rx2;

		//足の向きを求める
		if( fabsf(move_rx) > DegreeToRadian(90)){
			move_rx2 = move_rx + (float)M_PI;
		}
		else{
			move_rx2 = move_rx;
		}
		move_rx2 = AngleNormalization(move_rx2);

		//徐々にその向きに
		legrotation_x = legrotation_x*0.85f + (rotation_x + move_rx2*-1)*0.15f;		// 3/4 + 1/4
	}

	//腕のモデル設定
	armmodelid = id_armmodel[ArmModelID];

	//足のモデル設定
	legmodelid = id_legmodel;	//立ち止まり
	if( hp > 0 ){
		if( GetFlag(MoveFlag, MOVEFLAG_WALK) ){
			legmodelid = id_walkmodel[ (walkcnt/3 % TOTAL_WALKMODE) ];	//歩き
			walkcnt += 1;
			runcnt = 0;
		}
		else{
			if( GetFlag(MoveFlag, (MOVEFLAG_LEFT | MOVEFLAG_RIGHT)) ){
				legmodelid = id_runmodel[ (runcnt/3 % TOTAL_RUNMODE) ];		//左右走り
			}
			if( GetFlag(MoveFlag, MOVEFLAG_FORWARD) ){
				legmodelid = id_runmodel[ (runcnt/2 % TOTAL_RUNMODE) ];		//前走り
			}
			if( GetFlag(MoveFlag, MOVEFLAG_BACK) ){
				legmodelid = id_runmodel[ (runcnt/4 % TOTAL_RUNMODE) ];		//後ろ走り
			}
			walkcnt = 0;
			runcnt += 1;
		}
	}
}

//! @brief モーション取得
//! @param arm_rotation_y 腕の角度を取得するポインタ
//! @param leg_rotation_x 足の角度を取得するポインタ
//! @param upmodel 上半身のモデルのデータ番号を取得するポインタ
//! @param armmodel 腕のモデルのデータ番号を取得するポインタ
//! @param legmodel 足のモデルのデータ番号を取得するポインタ
void HumanMotionControl::GetRenderMotion(float *arm_rotation_y, float *leg_rotation_x, int *upmodel, int *armmodel, int *legmodel)
{
	if( arm_rotation_y != NULL ){ *arm_rotation_y = armmodel_rotation_y; }
	if( leg_rotation_x != NULL ){ *leg_rotation_x = legrotation_x; }

	if( upmodel != NULL ){ *upmodel = id_upmodel; }
	if( armmodel != NULL ){ *armmodel = armmodelid; }
	if( legmodel != NULL ){ *legmodel = legmodelid; }
}

//! @brief 歩き・走りモーションのカウント値取得
//! @return カウント値
int HumanMotionControl::GetMoveMotionCnt()
{
	if( walkcnt > 0 ){ return walkcnt; }
	return runcnt;
}