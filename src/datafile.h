//! @file datafile.h
//! @brief データ管理クラスの宣言

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

#ifndef DATAFILE_H
#define DATAFILE_H

#define MAX_BLOCKS 400			//!< 最大ブロック数 [XOPS-ONLINE] 160 -> 400 for bigger arenas
#define TOTAL_BLOCKTEXTURE 10	//!< 合計テクスチャ数

//#define MAX_POINTS 200
#define MAX_POINTS 256			//!< 最大ポイント数

//! @brief 追加小物の数
//! @attention 設定値を増やす場合、MAX_MODEL・MAX_TEXTURE・MAX_LOADSOUNDの値も適切に見直すこと
//! @note 追加小物設定ファイルの下部に、続けて次の追加小物の設定値を記載します。ただし、各追加小物の設定値の間にダミー行（"//"推奨）を挿んでください。
#define MAX_ADDSMALLOBJECT 1

#define MAX_POINTMESSAGES 16			//!< .msgファイルの最大メッセージ数
#define MAX_POINTMESSAGEBYTE (71+2)		//!< .msgファイル 1行あたりの最大バイト数

#define LIGHT_RX DegreeToRadian(190)	//!< ライトの横軸角度（ブロックの陰影計算用）
#define LIGHT_RY DegreeToRadian(120)	//!< ライトの縦軸角度（ブロックの陰影計算用）

#ifndef H_LAYERLEVEL
 #define H_LAYERLEVEL 1		//!< Select include file.
#endif
#include "main.h"

#include <ctype.h>
#include <sys/stat.h>

//! ブロックデータ用構造体　（blockdata構造体で使用）
typedef struct
{
	int textureID;				//!< textureID
	float u[4];					//!< textureUV-U
	float v[4];					//!< textureUV-V
	float center_x;				//!< center
	float center_y;				//!< center
	float center_z;				//!< center
	float vx;					//!< normal vector 
	float vy;					//!< normal vector 
	float vz;					//!< normal vector 
	float shadow;				//!< shadow 
} b_material;
//! ブロックデータ用構造体
typedef struct
{
	int id;						//!< DataID
	float x[8];					//!< Position
	float y[8];					//!< Position
	float z[8];					//!< Position
	b_material material[6];		//!< Material data
} blockdata;

//! ポイントデータ用構造体
typedef struct
{
	int id;						//!< DataID
	float x;					//!< Position
	float y;					//!< Position
	float z;					//!< Position
	float r;					//!< Rotation
	signed short int p1;		//!< Param
	signed short int p2;		//!< Param
	signed short int p3;		//!< Param
	signed short int p4;		//!< Param
} pointdata;

//ポイントデータのP1の種類番号
#define POINT_P1TYPE_HUMAN			1	//!< Param1 type
#define POINT_P1TYPE_WEAPON			2	//!< Param1 type
#define POINT_P1TYPE_AIPATH			3	//!< Param1 type
#define POINT_P1TYPE_HUMANINFO		4	//!< Param1 type
#define POINT_P1TYPE_SMALLOBJ		5	//!< Param1 type
#define POINT_P1TYPE_HUMAN2			6	//!< Param1 type
#define POINT_P1TYPE_RAND_WEAPON	7	//!< Param1 type
#define POINT_P1TYPE_RAND_AIPATH	8	//!< Param1 type
#define POINT_P1TYPE_STAGED_HUMAN	50	//!< [OpenXOPS-web] human held until event 41 deploys group p4 (docs/MISSION-UFO-FIELD.md)

//! 追加小物用の構造体
typedef struct
{
	char modelpath[_MAX_PATH];		//!< モデルデータパス
	char texturepath[_MAX_PATH];	//!< テクスチャパス
	int decide;						//!< 当たり判定の大きさ
	int hp;							//!< 耐久力
	char soundpath[_MAX_PATH];		//!< サウンドデータパス
	int jump;						//!< 飛び具合
} addsmallobject;

//! @brief データを管理するクラス（基底クラス）
//! @details ゲームのデータを、ファイルから読み込み処理するクラス群の基底クラスです。
class DataInterface
{
protected:
	int datas;		//!< データ数

public:
	DataInterface();
	~DataInterface();
	virtual int LoadFiledata(const char *fname);
	virtual int GetTotaldatas();
	virtual int Getdata(void *out_data, int id);
};

//! @brief ブロックデータを管理するクラス
//! @details マップデータとして使う、ブロックデータファイルを管理します。
//! @details ファイルの読み込みのほか、マップへの影（各面の明るさ表現）も計算します。
class BlockDataInterface : public DataInterface
{
	blockdata *data;								//!< ブロックデータを表す構造体
	char texture[TOTAL_BLOCKTEXTURE][_MAX_PATH];	//!< テクスチャ名

public:
	BlockDataInterface();
	~BlockDataInterface();
	int LoadFiledata(const char *fname);
	void CalculationBlockdata(bool darkflag);
	int GetTexture(char *fname, int id);
	int Getdata(blockdata *out_data, int id);
};

bool blockdataface(int faceID, int* vID, int* uvID);

//! @brief ポイントデータを管理するクラス
//! @details ミッションデータとして使う、ポイントデータファイルを管理します。
//! @details ファイルの読み込みのほか、ポイントの検索機能もあります。
class PointDataInterface : public DataInterface
{
	pointdata *data;							//!< ブロックデータを表す構造体
	char *text[MAX_POINTMESSAGES];				//!< イベントメッセージ

	int LoadMSGFiledata(char *fname);

public:
	PointDataInterface();
	~PointDataInterface();
	int LoadFiledata(const char *fname);
	int Getdata(pointdata *out_data, int id);
	int SetParam(int id, signed short int p1, signed short int p2, signed short int p3, signed short int p4);
	int GetMessageText(char *str, int id);
	int SearchPointdata(int* id, unsigned char pmask, signed short int p1, signed short int p2, signed short int p3, signed short int p4, int offset = 0);
	int SearchPointdata(pointdata *out_data, unsigned char pmask, signed short int p1, signed short int p2, signed short int p3, signed short int p4, int offset = 0);
};

//! @brief MIFを管理するクラス
//! @details MIF（MISSION INFORMATION FILE）を管理します。
//! @details 標準ミッションのブリーフィングファイル（.txt）にも対応しています。
class MIFInterface : public DataInterface
{
	bool mif;								//!< ファイル形式が .mif
	char mission_name[24];					//!< ミッション識別名
	char mission_fullname[64];				//!< ミッション正式名称
	char blockfile_path[_MAX_PATH];			//!< ブロックデータファイル
	char pointfile_path[_MAX_PATH];			//!< ポイントデータファイル
	int skynumber;							//!< 背景空のID　（なし：0）
	char picturefileA_path[_MAX_PATH];		//!< ブリーフィング画像A
	char picturefileB_path[_MAX_PATH];		//!< ブリーフィング画像B（追加分）
	char addsmallobject_path[_MAX_PATH];	//!< 追加小物情報ファイルへのパス
	char briefingtext[816];					//!< ブリーフィング文章・本文
	float AddSmallObjectCover[MAX_ADDSMALLOBJECT][4] = {}; // [OpenXOPS-web] optional body hulls
	bool collision;					//!< 当たり判定を多めに行う
	bool screen;					//!< 画面を暗めにする
	addsmallobject *AddSmallObjectData;		//!< 追加小物の情報

	int LoadDefaultTextFiledata(const char *fname);
	int LoadMissionInfoFiledata(const char *fname);
	int LoadAddSmallObjectFiledata(const char *fname);
	bool ChangeExePathToFullPath(char *dir, char *fname);

public:
	MIFInterface();
	~MIFInterface();
	int LoadFiledata(const char *fname, bool addfile);
	bool GetFiletype();
	char* GetMissionName();
	char* GetMissionFullname();
	void GetDatafilePath(char *blockfile, char *pointfile);
	int GetSkynumber();
	void GetPicturefilePath(char *picturefileA, char *picturefileB);
	char* GetBriefingText();
	bool GetCollisionFlag();
	bool GetScreenFlag();
	char* GetAddSmallobjectFile();
	char* GetAddSmallobjectModelPath(int id);
	char* GetAddSmallobjectTexturePath(int id);
	int GetAddSmallobjectDecide(int id);
	int GetAddSmallobjectHP(int id);
	char* GetAddSmallobjectSoundPath(int id);
	int GetAddSmallobjectJump(int id);
	const float* GetAddSmallobjectCover(int id); // [OpenXOPS-web] half-width, min-y, max-y, half-length
};

//! @brief ADDONリストを管理するクラス
//! @details 特定のディレクトリに入った.mifをADDONリストとして管理します。
class AddonList : public DataInterface
{
	char filename[MAX_ADDONLIST][_MAX_PATH];	//!< .mifファイル名
	char mission_name[MAX_ADDONLIST][24];		//!< ミッション識別名

	void GetMIFlist(const char *dir);
	void GetMissionName(const char *dir);
	void Sort();

public:
	AddonList();
	~AddonList();
	int LoadFiledata(const char *dir);
	char *GetMissionName(int id);
	char *GetFileName(int id);
};

//! @brief INIファイルを管理するクラス
//! @details INIファイルを管理します。
class INIFileInterface
{
	FILE *inifp;	//!< ファイルポインタ

public:
	INIFileInterface();
	~INIFileInterface();
	bool LoadINIFile(const char *fname);
	int GetINIFileString(const char *sectionname, const char *keyname, const char *defaultvalue, char *valuestr, int strbuflen);
	int GetINIFileInt(const char *sectionname, const char *keyname, int defaultvalue, int *errorcode);
	float GetINIFileFloat(const char *sectionname, const char *keyname, float defaultvalue, int *errorcode);
	bool GetINIFileBool(const char *sectionname, const char *keyname, bool defaultvalue, int *errorcode);
	void ReleaseINIFile();
};

int DeleteLinefeed(char *str);
bool CheckFullPath(const char *path);
void GetFileDirectory(const char *path, char *dir);
bool CheckFileExtension(const char *filepath, const char *checkstr);
bool CheckDirectory(const char *dir);

#endif