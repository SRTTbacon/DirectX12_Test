#pragma once
#include "bass.h"
#include "bassflac.h"
#include "bass_fx.h"
#include "unordered_map"

#include "..\\..\\Main\\Utility.h"

#undef PlaySound	//WinAPIのPlaySoundと被るため

//サウンド再生システム Bass Audio Libraryを使用。 営利目的の場合は使用不可
//詳しくは "https://www.un4seen.com/" を参照
//対応フォーマット : Bass Audio Libraryが対応しているもの
//	.wav .mp3 .ogg .flac .aac .wma などなど プラグインを導入すればプラスで対応可
//商用利用する場合はXAudio2かWwiseを推奨

//サウンドの状態を保持する構造体
class SoundHandle
{
public:
	const UINT m_streamHandle;		//サウンドハンドル
	const double m_maxSoundTime;	//サウンドの長さ (秒単位)
	const float m_defaultFrequency;	//初期の周波数

	float m_speed;			//速度 (0.0f～)
	float m_volume;			//音量 (0.0f～1.0f)

	bool m_bLooping;		//ループさせるか

	SoundHandle(const UINT handle, const float freq);
	~SoundHandle();

	//サウンドの再生
	//引数 : bool 最初から再生するか (falseの場合、PauseSound()を呼んだ時間から再開)
	void PlaySound(bool bRestart = true);

	//サウンドの一時停止
	void PauseSound();

	//プロパティ(速度や音量など) を更新
	void UpdateProperty();

	//再生位置を変更 (相対的に)
	//引数 : double 現在の再生位置からの相対位置(秒)
	void ChangePosition(double relativeTime) const;

	//再生位置を変更
	//引数 : double 移動後の位置 (秒)
	void SetPosition(double toTime) const;

	//サウンドを解放
	//この関数実行後は使用不可になる
	void Release() const;

	inline bool IsPlaying() const { return m_bPlaying; }

private:
	bool m_bPlaying;			//再生中かどうか

	bool IsVaild() const;
};

class SoundSystem
{
public:
	//コンストラクタ
	//引数 : HWND ウィンドウハンドル
	SoundSystem(HWND pHandle);

	~SoundSystem();

	//ファイルからサウンドのハンドルを作成
	//引数 : std::string サウンドのファイルパス, bool すぐに再生するか
	//戻り値 : サウンド情報が入っているハンドル
	SoundHandle* LoadSound(std::string filePath, bool bPlay = false);

	//再生中のサウンドをすべて更新
	void Update();

private:
	//サウンド情報一覧
	std::vector<SoundHandle*> m_soundHandles;

	//再生終了時のコールバック関数
	static void CALLBACK PlaybackEndCallback(HSYNC handle, DWORD channel, DWORD data, void* user);
};
