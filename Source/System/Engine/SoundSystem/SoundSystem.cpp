#include "SoundSystem.h"

#undef PlaySound

#define GetBassError BASS_ErrorGetCode() != BASS_OK

//コンストラクタ
//引数 : HWND ウィンドウハンドル
SoundSystem::SoundSystem(HWND pHandle)
{
	//引数1 : 再生デバイス (-1が現在選択されているデバイスで、インデックスを変更すると、例えばモニターのスピーカーなどから再生される。基本-1)
	//引数2 : サンプリングレート (44.1kHz、48kHz、96kHz、192kHzなどが主流。数値が大きくなるほどサウンド次第では高音質になるが、負荷が大きくなる)
	//引数3 : フラグ (ステレオ再生であれば基本0指定。3D表現を行うときや、あえてモノラルで再生するのであれば "BASS_DEVICE_*" で指定)
	//引数4 : よくわからんからnullptr
	BASS_Init(-1, 96000, 0, pHandle, nullptr);
}

SoundSystem::~SoundSystem()
{
	//すべてのロードされているサウンドを解放
	for (UINT i = 0; i < m_soundHandles.size(); i++) {
		if (m_soundHandles[i] && m_soundHandles[i]->streamHandle > 0) {
			m_soundHandles[i]->Release();

			delete m_soundHandles[i];
		}
	}
}

//ファイルからサウンドのハンドルを作成
//戻り値 : サウンド情報が入った構造体
SoundHandle* SoundSystem::LoadSound(std::string filePath, bool bPlay)
{
	UINT soundHandle = 0;

	BASS_SetConfig(BASS_CONFIG_BUFFER, 100);

	//.flac形式は少々特殊なため関数が分かれている
	if (GetFileExtension(filePath) == ".flac") {
		soundHandle = BASS_FLAC_StreamCreateFile(false, filePath.c_str(), 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE);
	}
	else {
		soundHandle = BASS_StreamCreateFile(false, filePath.c_str(), 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE);
	}

	if (GetBassError) {
		printf("サウンドの読み込み時にエラーが発生しました。エラーコード:%d\n", BASS_ErrorGetCode());
		return nullptr;
	}

	//ピッチやエフェクトが適応できる形に変更
	int fxHandle = BASS_FX_TempoCreate(soundHandle, BASS_FX_FREESOURCE);

	if (GetBassError) {
		printf("サウンドの読み込み時にエラーが発生しました。エラーコード:%d\n", BASS_ErrorGetCode());
		BASS_StreamFree(soundHandle);
		return nullptr;
	}

	//サウンドの状態を保持する構造体
	float freq;
	BASS_ChannelGetAttribute(fxHandle, BASS_ATTRIB_TEMPO_FREQ, &freq);
	SoundHandle* pHandle = new SoundHandle(fxHandle, freq);

	// 再生終了時のコールバックを設定
	HSYNC syncHandle = BASS_ChannelSetSync(fxHandle, BASS_SYNC_END, 0, &SoundSystem::PlaybackEndCallback, pHandle);
	if (!syncHandle) {
		return nullptr;
	}

	//ロード後すぐに再生
	if (bPlay) {
		pHandle->PlaySound(true);
	}

	m_soundHandles.push_back(pHandle);
	return pHandle;
}

void SoundSystem::Update()
{
	for (UINT i = 0; i < m_soundHandles.size(); i++) {
		SoundHandle* pSound = m_soundHandles[i];
		if (pSound->streamHandle > 0) {
			//LoadSoundでBASS_CONFIG_BUFFERを100に設定する関係で、PCによっては音がブツブツするのを防ぐ
			BASS_ChannelUpdate(pSound->streamHandle, 300);
		}
		else {
			//サウンドがReleaseされていたらメモリから削除
			delete pSound;
			std::vector<SoundHandle*>::iterator it = m_soundHandles.begin();
			it += i;
			m_soundHandles.erase(it);
		}
	}
}

void SoundSystem::PlaybackEndCallback(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	SoundHandle* soundHandle = static_cast<SoundHandle*>(user);
	//そのサウンドがループ再生を許可していれば、再度再生
	if (soundHandle && soundHandle->bLooping) {
		soundHandle->PlaySound();
	}
}

SoundHandle::SoundHandle(const UINT handle, const float freq)
	: streamHandle(handle)		//ハンドル
	, maxSoundTime(BASS_ChannelBytes2Seconds(handle, BASS_ChannelGetLength(handle, BASS_POS_BYTE)))	//サウンドの長さ
	, defaultFrequency(freq)	//初期の周波数
	, speed(1.0f)				//再生速度
	, volume(1.0f)				//音量
	, bPlaying(false)
	, bLooping(false)
{
}

SoundHandle::~SoundHandle()
{
	Release();
}

//サウンドを再生
//引数 : 最初から再生するか (falseの場合、PauseSound()を呼んだ時間から再開)
void SoundHandle::PlaySound(bool bRestart)
{
	if (streamHandle == 0)
		return;

	bPlaying = true;
	UpdateProperty();

	BASS_ChannelPlay(streamHandle, bRestart);

	if (GetBassError) {
		printf("サウンドの再生時にエラーが発生しました。エラーコード:%d\n", BASS_ErrorGetCode());
		bPlaying = false;
		return;
	}

}

//サウンドの一時停止
void SoundHandle::PauseSound()
{
	if (streamHandle == 0)
		return;

	BASS_ChannelPause(streamHandle);

	bPlaying = false;
}

//プロパティ(速度や音量など)を更新
void SoundHandle::UpdateProperty() const
{
	//ハンドルがなかったり、再生が停止している場合は処理を終える (再生開始時に呼ばれるため)
	if (streamHandle == 0 || !bPlaying)
		return;

	//再生速度と音量を変更
	BASS_ChannelSetAttribute(streamHandle, BASS_ATTRIB_TEMPO_FREQ, defaultFrequency * speed);
	BASS_ChannelSetAttribute(streamHandle, BASS_ATTRIB_VOL, volume);
}

//再生位置を変更
//引数 : float 現在の再生位置からの相対位置(秒)
void SoundHandle::ChangePosition(double relativeTime) const
{
	if (streamHandle == 0)
		return;

	//現在の再生時間を取得
	QWORD position = BASS_ChannelGetPosition(streamHandle, BASS_POS_BYTE);
	double nowPosition = BASS_ChannelBytes2Seconds(streamHandle, position);

	//相対時間を追加
	nowPosition += relativeTime;

	//再生位置を変更
	SetPosition(nowPosition);
}

//再生位置を変更
//引数 : double 移動後の位置 (秒)
void SoundHandle::SetPosition(double toTime) const
{
	if (streamHandle == 0)
		return;

	//範囲制限
	if (toTime < 0.0)
		toTime = 0.0f;
	else if (toTime > maxSoundTime)
		toTime = maxSoundTime;

	//再生位置をtoTimeに変更 (秒での指定はできないため、秒からBASS_POS_BYTEへ変換)
	BASS_ChannelSetPosition(streamHandle, BASS_ChannelSeconds2Bytes(streamHandle, toTime), BASS_POS_BYTE);
}

//サウンドの解放
void SoundHandle::Release() const
{
	BASS_CHANNELINFO info;

	//既にサウンドが解放していれば処理を終える
	if (streamHandle == 0 || BASS_ChannelGetInfo(streamHandle, &info))
		return;

	BASS_ChannelStop(streamHandle);
	BASS_StreamFree(streamHandle);
}
