#include "Texture2D.h"
#include "..\\..\\Engine.h"

using namespace DirectX;

std::vector<std::shared_ptr<TextureManage>> Texture2D::s_textures;

Texture2D::Texture2D(std::string path, bool bDDSFile)
{
	Load(path, bDDSFile);
}

Texture2D::Texture2D(std::wstring path, bool bDDSFile)
{
	Load(path, bDDSFile);
}

Texture2D::Texture2D(const char* data, size_t size, bool bDDSFile)
{
	Load(data, size, bDDSFile);
}

Texture2D::Texture2D(std::shared_ptr<TextureManage> pTextureManage)
{
	m_manage = pTextureManage;
}

Texture2D::~Texture2D()
{
	if (!m_manage) {
		return;
	}

	UINT uniqueID = m_manage->uniqueID;

	m_manage = nullptr;

	//TextureManage���Q�Ƃ��Ă��鐔��1(std::vector�̂�)�ɂȂ�������
	for (UINT i = 0; i < static_cast<UINT>(s_textures.size()); i++) {
		if (s_textures[i]->uniqueID == uniqueID && m_manage.use_count() <= 1) {
			std::vector<std::shared_ptr<TextureManage>>::iterator it = s_textures.begin();
			it += i;
			s_textures.erase(it);
		}
	}
}

bool Texture2D::Load(std::string& path, bool bDDSFile)
{
	auto wpath = GetWideString(path);
	return Load(wpath, bDDSFile);
}

bool Texture2D::Load(std::wstring& path, bool bDDSFile)
{
	UINT uniqueID = GenerateIDFromFile(path);

	//���ɓǂݍ��܂�Ă���e�N�X�`�����ǂ����𒲂ׂ�
	for (std::shared_ptr<TextureManage>& tex : s_textures)
	{
		if (tex->uniqueID == uniqueID)
		{
			//���ɓǂݍ��܂�Ă���e�N�X�`�����Q��
			m_manage = tex;
			return true;
		}
	}

	//WIC�e�N�X�`���̃��[�h
	TexMetadata meta = {};
	ScratchImage scratch = {};

	HRESULT hr = 0;
	if (bDDSFile) {
		hr = LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, &meta, scratch);
	}
	else {
		hr = LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &meta, scratch);
	}

	if (FAILED(hr))
	{
		printf("�e�N�X�`���̓ǂݍ��݂Ɏ��s\n");
		return false;
	}

	return Load(uniqueID, meta, scratch);
}

bool Texture2D::Load(const char* data, size_t size, bool bDDSFile)
{
	UINT uniqueID = GenerateIDFromFile(data, size);

	//���ɓǂݍ��܂�Ă���e�N�X�`�����ǂ����𒲂ׂ�
	for (std::shared_ptr<TextureManage>& tex : s_textures)
	{
		if (tex->uniqueID == uniqueID)
		{
			//���ɓǂݍ��܂�Ă���e�N�X�`�����Q��
			m_manage = tex;
			return true;
		}
	}

	//WIC�e�N�X�`���̃��[�h
	TexMetadata meta = {};
	ScratchImage scratch = {};

	HRESULT hr = 0;
	if (bDDSFile) {
		hr = LoadFromDDSMemory(data, size, DDS_FLAGS_NONE, &meta, scratch);
	}
	else {
		hr = LoadFromWICMemory(data, size, WIC_FLAGS_NONE, &meta, scratch);
	}

	if (FAILED(hr))
	{
		printf("�e�N�X�`���̓ǂݍ��݂Ɏ��s\n");
		return false;
	}

	return Load(uniqueID, meta, scratch);
}

bool Texture2D::Load(UINT uniqueID, TexMetadata& meta, ScratchImage& scratch)
{
	//�e�N�X�`���Ǘ��z��ɓo�^
	std::shared_ptr<TextureManage> manage = std::make_shared<TextureManage>();
	manage->uniqueID = uniqueID;
	manage->bSimpleTex = false;

	CD3DX12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(meta.format, static_cast<UINT>(meta.width), static_cast<UINT>(meta.height),
		static_cast<UINT16>(meta.arraySize), static_cast<UINT16>(meta.mipLevels));

	//���\�[�X�𐶐�
	HRESULT hr = g_Engine->GetDevice()->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&manage->pResource)
	);

	if (FAILED(hr))
	{
		printf("�e�N�X�`���̓ǂݍ��݂Ɏ��s �G���[�R�[�h = %1x\n", hr);
		return false;
	}

	//�A�b�v���[�h�o�b�t�@�̃T�C�Y���v�Z
	UINT64 totalBytes = 0;
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(meta.arraySize);
	std::vector<UINT> numRows(meta.arraySize);
	std::vector<UINT64> rowSizeInBytes(meta.arraySize);

	g_Engine->GetDevice()->GetCopyableFootprints(
		&desc, 0, static_cast<UINT>(meta.arraySize), 0,
		layouts.data(), numRows.data(), rowSizeInBytes.data(), &totalBytes
	);

	//�A�b�v���[�h�o�b�t�@���쐬
	ID3D12Resource* pUploadBuffer = g_resourceCopy->CreateUploadBuffer(totalBytes);

	//scratch�̃R�s�[ (���C���X���b�h�����s���Ȃ�)
	void* mappedData = nullptr;
	pUploadBuffer->Map(0, nullptr, &mappedData);

	BYTE* pDestBase = reinterpret_cast<BYTE*>(mappedData);

	for (UINT i = 0; i < meta.arraySize; ++i) {
		const Image* img = scratch.GetImage(0, i, 0);
		BYTE* pDestSlice = pDestBase + layouts[i].Offset;
		BYTE* pSrcSlice = img->pixels;

		for (UINT y = 0; y < numRows[i]; ++y) {
			memcpy(pDestSlice + layouts[i].Footprint.RowPitch * y, pSrcSlice + img->rowPitch * y, rowSizeInBytes[i]);
		}
	}

	pUploadBuffer->Unmap(0, nullptr);
	DWORD endTime = timeGetTime();

	std::thread([=] {
		g_resourceCopy->BeginCopyResource();

		//�e�ʂ��R�s�[
		for (UINT i = 0; i < meta.arraySize; ++i) {
			D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
			dstLocation.pResource = manage->pResource;
			dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLocation.SubresourceIndex = i;

			D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
			srcLocation.pResource = pUploadBuffer;
			srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLocation.PlacedFootprint = layouts[i];

			//�R�s�[
			g_resourceCopy->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
		}

		//�o���A��ݒ�
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(manage->pResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);

		g_resourceCopy->EndCopyResource();
		pUploadBuffer->Release();
		}).detach();

	s_textures.push_back(manage);

	m_manage = manage;

	return true;
}

Texture2D* Texture2D::Get(std::string path)
{
	auto wpath = GetWideString(path);
	return Get(wpath);
}

Texture2D* Texture2D::Get(std::wstring path)
{
	std::wstring ext = GetFileExtension(path);
	bool bDDSMode = _wcsicmp(ext.c_str(), L".dds") == 0;
	Texture2D* tex = new Texture2D(path, bDDSMode);
	if (!tex->IsValid())
	{
		delete tex;
		return GetColor(0.0f, 0.0f, 0.0f); //�ǂݍ��݂Ɏ��s�������͍��P�F�e�N�X�`����Ԃ�
	}
	return tex;
}

Texture2D* Texture2D::GetDDS(std::string path)
{
	auto wpath = GetWideString(path);
	return GetDDS(wpath);
}

Texture2D* Texture2D::GetDDS(std::wstring path)
{
	Texture2D* tex = new Texture2D(path, true);
	if (!tex->IsValid())
	{
		delete tex;
		return GetColor(0.0f, 0.0f, 0.0f); //�ǂݍ��݂Ɏ��s�������͍��P�F�e�N�X�`����Ԃ�
	}
	return tex;
}

Texture2D* Texture2D::Get(const char* data, size_t size)
{
	Texture2D* tex = new Texture2D(data, size, false);
	if (!tex->IsValid())
	{
		delete tex;
		return GetColor(0.0f, 0.0f, 0.0f); //�ǂݍ��݂Ɏ��s�������͍��P�F�e�N�X�`����Ԃ�
	}
	return tex;
}

Texture2D* Texture2D::GetDDS(const char* data, size_t size)
{
	Texture2D* tex = new Texture2D(data, size, true);
	if (!tex->IsValid())
	{
		delete tex;
		return GetColor(0.0f, 0.0f, 0.0f); //�ǂݍ��݂Ɏ��s�������͍��P�F�e�N�X�`����Ԃ�
	}
	return tex;
}

Texture2D* Texture2D::GetColor(float r, float g, float b, float a)
{
	UINT width = 16;
	UINT height = 16;

	size_t rowPitch = (width * sizeof(UINT) + 255) & ~255; //256�̔{���ɂȂ�悤��
	size_t tempWidth = rowPitch / sizeof(UINT);

	std::vector<UINT> data(tempWidth * height);
	std::fill(data.begin(), data.end(), ColorToUINT(r, g, b, a));

	//UINT�f�[�^���o�C�g�P�ʂɕϊ����邽�߂�std::vector<char>
	std::vector<char> charVector(data.size() * sizeof(UINT));
	//�f�[�^���R�s�[
	std::memcpy(charVector.data(), data.data(), data.size() * sizeof(UINT));

	//���ɓǂݍ��܂�Ă���e�N�X�`�����ǂ����𒲂ׂ�
	UINT uniqueID = GenerateIDFromFile(charVector);

	for (std::shared_ptr<TextureManage>& tex : s_textures) {
		if (tex->uniqueID == uniqueID) {
			return new Texture2D(tex);
		}
	}

	ID3D12Resource* pBuffer = GetDefaultResource(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	ID3D12Resource* pUploadBuffer = g_resourceCopy->CreateUploadBuffer(rowPitch * height * sizeof(UINT));

	g_resourceCopy->BeginCopyResource();

	//�A�b�v���[�h�o�b�t�@�ւ̃f�[�^�R�s�[
	void* mappedData = nullptr;
	HRESULT hr = pUploadBuffer->Map(0, nullptr, &mappedData);
	if (FAILED(hr)) {
		printf("�A�b�v���[�h�o�b�t�@�̃}�b�v�Ɏ��s���܂����B\n");
		return nullptr;
	}

	memcpy(mappedData, data.data(), rowPitch * height);
	pUploadBuffer->Unmap(0, nullptr);

	//�R�s�[���ݒ�
	D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
	dstLocation.pResource = pBuffer;
	dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLocation.SubresourceIndex = 0;

	//�R�s�[����ݒ�
	D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
	srcLocation.pResource = pUploadBuffer;
	srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLocation.PlacedFootprint.Footprint.Width = width;
	srcLocation.PlacedFootprint.Footprint.Height = height;
	srcLocation.PlacedFootprint.Footprint.Depth = 1;
	srcLocation.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(rowPitch);
	srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// �R�}���h���X�g�ŃR�s�[����
	g_resourceCopy->GetCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

	//���\�[�X�o���A��STATE��ύX
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	g_resourceCopy->GetCommandList()->ResourceBarrier(1, &barrier);

	g_resourceCopy->EndCopyResource();

	pUploadBuffer->Release();

	std::shared_ptr<TextureManage> manage = std::make_shared<TextureManage>();
	manage->uniqueID = uniqueID;
	manage->pResource = pBuffer;
	manage->bSimpleTex = true;

	s_textures.push_back(manage);

	return new Texture2D(manage);
}

ID3D12Resource* Texture2D::GetDefaultResource(DXGI_FORMAT format, size_t width, size_t height)
{
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, static_cast<UINT>(width), static_cast<UINT>(height), 1, 1);
	CD3DX12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ID3D12Resource* buff = nullptr;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
	HRESULT result = g_Engine->GetDevice()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		state,
		nullptr,
		IID_PPV_ARGS(&buff)
	);
	if (FAILED(result))
	{
		printf("GetDefaultResource : �G���[�R�[�h = %1x\n", result);
		return nullptr;
	}
	return buff;
}

bool Texture2D::IsValid() const
{
	return m_manage != nullptr;
}

bool Texture2D::IsSimpleTex() const
{
	return m_manage->bSimpleTex;
}

ID3D12Resource* Texture2D::Resource()
{
	if (!m_manage) {
		return nullptr;
	}

	return m_manage->pResource;
}

D3D12_SHADER_RESOURCE_VIEW_DESC Texture2D::ViewDesc()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	if (m_manage && m_manage->pResource) {
		desc.Format = m_manage->pResource->GetDesc().Format;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; //2D�e�N�X�`��
		desc.Texture2D.MipLevels = 1; //�~�b�v�}�b�v�͎g�p���Ȃ��̂�1
	}
	return desc;
}

TextureManage::TextureManage()
	: uniqueID(0)
	, bSimpleTex(true)
	, pResource(nullptr)
{
}

TextureManage::~TextureManage()
{
	if (pResource) {
		pResource->Release();
		pResource = nullptr;
	}
}
