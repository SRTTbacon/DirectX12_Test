#pragma once

#include <Windows.h>
#include <DirectXMath.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include "AK/AkDefaultIOHookDeferred.h"
#include <random>
#include <fstream>
#include <unordered_map>

//3D音響を扱えるサウンドエンジン"Wwise"を使用するためのライブラリ
//原神とかDetroitとかOverwatch2とかで使用されているため、信頼性◎
//本来ならサウンドエンジンごとプロジェクトに入れるのが一般的だけど、今回はdllを使用

struct Emitter
{
	const AkGameObjectID objectID;

	DirectX::XMFLOAT3 position;	//再生位置
	DirectX::XMFLOAT3 front;		//前方向ベクトル
	DirectX::XMFLOAT3 top;		//上方向ベクトル

	Emitter(AkGameObjectID id);
};

using EmitterRef = std::shared_ptr<Emitter>;

class WwiseSoundHandle
{
public:
	WwiseSoundHandle(const EmitterRef pEmitter, AkPlayingID playingID);

public:
	inline EmitterRef GetEmitter() const { return m_pEmitter; }
	inline AkPlayingID GetPlayingID() const { return m_playingID; }

private:
	const EmitterRef m_pEmitter;
	const AkPlayingID m_playingID;
};

class WwiseSoundSystem
{
public:
	//Wwiseの初期化
	//引数 : char* init.bnkのファイルパス
	UINT Initialize(const char* initBNKStr);

	//更新 (何か再生中は毎フレームに1度実行させる)
	void Update();

	//エミッターを追加
	//位置が変化するオブジェクトの数だけ追加する
	EmitterRef Add_Emitter();

	//エミッターを削除
	//引数 : Emitter Add_Emitter()で取得
	void Delete_Emitter(const EmitterRef emitter);

	//.bnkファイルをロード
	//引数 : char* .bnkファイルのパス
	//戻り値 : エラーコード
	AKRESULT Load_Bank(const char* bnkFileStr);

	//.bnkをアンロード
	//引数 : char* .bnkファイルのパス
	//戻り値 : エラーコード
	AKRESULT UnLoad_Bank(const char* bnkFileStr);

	//イベント名を再生
	//引数 : char* イベント名
	//戻り値 : サウンドハンドル
	WwiseSoundHandle Play(const char* eventNameStr);

	//3D上の位置が変化するイベントを再生
	//引数 : char* イベント名, Emitter エミッター
	//戻り値 : サウンドハンドル
	WwiseSoundHandle Play(const char* eventNameStr, const EmitterRef emitter);

	//イベントIDを再生
	//引数 : UINT イベントID
	//戻り値 : サウンドハンドル
	WwiseSoundHandle Play(UINT eventID);

	//3D上の位置が変化するイベントを再生
	//引数 : UINT イベントID, Emitter エミッター
	//戻り値 : サウンドハンドル
	WwiseSoundHandle Play(UINT eventID, const EmitterRef emitter);

	//イベントを停止
	//引数 : UINT Play()で取得したID
	void Stop(WwiseSoundHandle soundHandle);

	//すべてのイベントを停止
	void Stop_All();

	//すべてのイベントを一時停止
	//戻り値 : 成功したらtrue
	bool Pause_All();

	//一時停止中のすべてのイベントを再生(途中から)
	//戻り値 : 成功したらtrue
	bool Play_All();

	//Stateを設定
	//引数 : char* 親ステートの名前, char* 子ステートの名前
	//戻り値 : 成功したらtrue
	bool Set_State(const char* parentNameStr, const char* childNameStr);

	//RTPCを設定
	//引数 : char* RTPCの名前, float 数値(Wwise側で設定した範囲)
	//戻り値 : 成功したらtrue
	bool Set_RTPC(const char* rtpcNameStr, float value);

	//リスナーの位置、角度を設定
	//引数(省略) : 3D上のxyzの位置, 前方向(正規化), 上方向(正規化)
	void Set_Listener_Position(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, const DirectX::XMFLOAT3& top);

	//エミッターの位置、角度を更新
	//引数 : Emitter エミッター
	void Set_Emitter(const EmitterRef emitter);

	//Wwiseを解放
	//この関数を実行後は再度Init()を実行するまで使用できません。
	void Dispose();

	//初期化されているか
	bool IsInited();

	//Wwiseが最後に出力したエラーを取得
	UINT Get_Result_Index() const;

private:
	CAkDefaultIOHookDeferred m_lowLevelIO;

	AKRESULT m_lastResult;
	UINT m_initBankID;

	AkGameObjectID m_listenerID = 0;

	AkGameObjectID GetRandomGameObjectID();

	std::unordered_map<std::string, char*> m_pBankData;

	static void AkCallbackFunction(AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo);
};
