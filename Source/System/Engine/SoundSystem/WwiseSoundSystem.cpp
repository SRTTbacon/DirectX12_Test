#include "WwiseSoundSystem.h"
#include <AK/Plugin/AkVorbisDecoderFactory.h>

#include <filesystem>

//----------Emitter----------

Emitter::Emitter(AkGameObjectID id)
	: objectID(id)
	, position(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f))
	, front(DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f))
	, top(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f))
{
}

//----------WwiseSoundHandle----------

WwiseSoundHandle::WwiseSoundHandle(const EmitterRef pEmitter, AkPlayingID playingID)
	: m_pEmitter(pEmitter)
	, m_playingID(playingID)
{
}

//----------WwiseSoundSystem----------

UINT WwiseSoundSystem::Initialize(const char* initBNKStr)
{
	//�������}�l�[�W���[�̏�����
	AkMemSettings memSettings;
	AK::MemoryMgr::GetDefaultSettings(memSettings);
	if (!AK::MemoryMgr::Init(&memSettings))
	{
		return 1000; //�������}�l�[�W���[�̏������Ɏ��s
	}

	//�X�g���[���}�l�[�W���[�̏�����
	AkStreamMgrSettings stmSettings;
	AK::StreamMgr::GetDefaultSettings(stmSettings);
	if (!AK::StreamMgr::Create(stmSettings))
	{
		return 1001; // �X�g���[���}�l�[�W���[�̏������Ɏ��s
	}

	//�f�o�C�X�̏�����
	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

	if (m_lowLevelIO.Init(deviceSettings) != AK_Success) {
		return 1002;
	}

	//�T�E���h�G���W���̏�����
	AkInitSettings initSettings{};
	AkPlatformInitSettings platformInitSettings{};

	AK::SoundEngine::GetDefaultInitSettings(initSettings);
	AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);

	if (AK::SoundEngine::Init(&initSettings, &platformInitSettings) != AK_Success)
	{
		return 1006; //�T�E���h�G���W���̏������Ɏ��s
	}

	//Init.bnk ��ǂݍ���
	std::ifstream ifs(initBNKStr, std::ios_base::in | std::ios_base::binary);
	if (ifs.fail()) {
		return 1003; //Init.bnk �̃I�[�v���Ɏ��s
	}
	std::istreambuf_iterator<char> it_ifs_begin(ifs);
	std::istreambuf_iterator<char> it_ifs_end{};
	std::vector<char> input_data(it_ifs_begin, it_ifs_end);
	if (ifs.fail()) {
		ifs.close();
		return 1004; //Init.bnk �̓ǂݎ��Ɏ��s
	}

	//�ǂݎ�����f�[�^�����[�h
	m_lastResult = AK::SoundEngine::LoadBankMemoryCopy(input_data.data(), (unsigned long)input_data.size(), m_initBankID);

	//���
	ifs.close();
	input_data.clear();

	//���X�i�[��ݒ�
	if (AK::SoundEngine::RegisterGameObj(m_listenerID, "CameraListener") != AK_Success) {
		return 1005;
	}
	AK::SoundEngine::SetDefaultListeners(&m_listenerID, 1);

	return m_lastResult;
}

void WwiseSoundSystem::Update()
{
	AK::SoundEngine::RenderAudio();
}

EmitterRef WwiseSoundSystem::Add_Emitter()
{
	AkGameObjectID gameObjectID = GetRandomGameObjectID();

	m_lastResult = AK::SoundEngine::RegisterGameObj(gameObjectID);
	AkSoundPosition position{};
	position.Set(0.0, 0.0, 0.0, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	m_lastResult = AK::SoundEngine::SetPosition(gameObjectID, position);

	EmitterRef pEmitter = std::make_shared<Emitter>(gameObjectID);

	return pEmitter;
}

void WwiseSoundSystem::Delete_Emitter(const EmitterRef emitter)
{
	if (!emitter) {
		return;
	}

	AK::SoundEngine::UnregisterGameObj(emitter->objectID);
}
AKRESULT WwiseSoundSystem::Load_Bank(const char* bnkFileStr)
{
	if (m_pBankData.find(bnkFileStr) != m_pBankData.end()) {
		return AKRESULT::AK_BankAlreadyLoaded;
	}


	std::ifstream file(bnkFileStr, std::ios::binary | std::ios::ate);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	char* pData = new char[size]; // ���I�m��

	m_pBankData.emplace(bnkFileStr, pData);

	file.read(pData, size);
	file.close(); // `ifstream` �͕��邪�A�������͉�����Ȃ�

	AkBankID bankID;
	AKRESULT m_lastResult = AK::SoundEngine::LoadBankMemoryView(pData, static_cast<AkUInt32>(size), bankID);

	return m_lastResult;
}

AKRESULT WwiseSoundSystem::UnLoad_Bank(const char* bnkFileStr)
{
	if (m_pBankData.find(bnkFileStr) == m_pBankData.end()) {
		return AKRESULT::AK_FileNotFound;
	}

	m_lastResult = AK::SoundEngine::UnloadBank(bnkFileStr, nullptr);

	delete m_pBankData[bnkFileStr];

	m_pBankData.erase(bnkFileStr);

	return m_lastResult;
}

WwiseSoundHandle WwiseSoundSystem::Play(const char* eventNameStr)
{
	EmitterRef emitter = Add_Emitter();

	AkPlayingID id = AK::SoundEngine::PostEvent(eventNameStr, emitter->objectID, AK_EndOfEvent, &AkCallbackFunction);
	WwiseSoundHandle soundHandle(emitter, id);

	return soundHandle;
}

WwiseSoundHandle WwiseSoundSystem::Play(const char* eventNameStr, const EmitterRef emitter)
{
	if (!emitter) {
		return WwiseSoundHandle(nullptr, 0);
	}

	AkPlayingID id = AK::SoundEngine::PostEvent(eventNameStr, emitter->objectID, AK_EndOfEvent, &AkCallbackFunction);
	WwiseSoundHandle soundHandle(emitter, id);

	return soundHandle;
}

WwiseSoundHandle WwiseSoundSystem::Play(UINT eventID)
{
	EmitterRef emitter = Add_Emitter();

	AkPlayingID id = AK::SoundEngine::PostEvent(eventID, emitter->objectID, AK_EndOfEvent, &AkCallbackFunction);

	WwiseSoundHandle soundHandle(emitter, id);

	return soundHandle;
}

WwiseSoundHandle WwiseSoundSystem::Play(UINT eventID, const EmitterRef emitter)
{
	if (!emitter) {
		return WwiseSoundHandle(nullptr, 0);
	}

	AkPlayingID id = AK::SoundEngine::PostEvent(eventID, emitter->objectID, AK_EndOfEvent, &AkCallbackFunction);

	WwiseSoundHandle soundHandle(emitter, id);

	return soundHandle;
}

void WwiseSoundSystem::Stop(WwiseSoundHandle soundHandle)
{
	AK::SoundEngine::StopPlayingID(soundHandle.GetPlayingID());
}

void WwiseSoundSystem::Stop_All()
{
	AK::SoundEngine::StopAll();
}

bool WwiseSoundSystem::Pause_All()
{
	m_lastResult = AK::SoundEngine::Suspend();

	return m_lastResult == AK_Success;
}

bool WwiseSoundSystem::Play_All()
{
	m_lastResult = AK::SoundEngine::WakeupFromSuspend();

	return m_lastResult == AK_Success;
}

bool WwiseSoundSystem::Set_State(const char* parentNameStr, const char* childNameStr)
{
	m_lastResult = AK::SoundEngine::SetState(parentNameStr, childNameStr);

	return m_lastResult == AK_Success;
}

bool WwiseSoundSystem::Set_RTPC(const char* rtpcNameStr, float value)
{
	m_lastResult = AK::SoundEngine::SetRTPCValue(rtpcNameStr, value);

	return m_lastResult == AK_Success;
}

void WwiseSoundSystem::Set_Listener_Position(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, const DirectX::XMFLOAT3& top)
{
	AkSoundPosition soundPos{};
	soundPos.Set(position.x, position.y, position.z, front.x, front.y, front.z, top.x, top.y, top.z);
	m_lastResult = AK::SoundEngine::SetPosition(m_listenerID, soundPos);
}

void WwiseSoundSystem::Set_Emitter(const EmitterRef emitter)
{
	AkSoundPosition soundPos{};
	soundPos.Set(emitter->position.x, emitter->position.y, emitter->position.z, emitter->front.x, emitter->front.y, emitter->front.z, emitter->top.x, emitter->top.y, emitter->top.z);
	m_lastResult = AK::SoundEngine::SetPosition(emitter->objectID, soundPos);
}

void WwiseSoundSystem::Dispose()
{
	if (!IsInited()) {
		return;
	}

	AK::SoundEngine::StopAll();
	AK::SoundEngine::UnregisterAllGameObj();
	AK::SoundEngine::ClearBanks();
	m_lowLevelIO.Term();
	AK::SoundEngine::Term();
	if (AK::IAkStreamMgr::Get())
		AK::IAkStreamMgr::Get()->Destroy();
	AK::MemoryMgr::Term();
	m_lastResult = AKRESULT::AK_Success;
	m_initBankID = 0;

	for (std::pair<std::string, char*> pData : m_pBankData) {
		delete pData.second;
	}

	m_pBankData.clear();
}

bool WwiseSoundSystem::IsInited()
{
	return AK::SoundEngine::IsInitialized();
}

UINT WwiseSoundSystem::Get_Result_Index() const
{
	return m_lastResult;
}

AkGameObjectID WwiseSoundSystem::GetRandomGameObjectID()
{
	//�����_����0x0000000000000000�`0xFFFFFFFFFFFFFFE0�܂ł̃����_���Ȑ��l���擾

	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<ULONGLONG> rand(10, MAXULONGLONG - 33);

	return rand(gen);
}

void WwiseSoundSystem::AkCallbackFunction(AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo)
{
	if (in_eType == AK_EndOfEvent)
	{
		AkEventCallbackInfo* pEventInfo = (AkEventCallbackInfo*)in_pCallbackInfo;
		AkGameObjectID gameObjectID = pEventInfo->gameObjID;

		//�Q�[���I�u�W�F�N�g�̓o�^����
		AK::SoundEngine::UnregisterGameObj(gameObjectID);
	}
}
