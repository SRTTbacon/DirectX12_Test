#include "Model.h"
#include "..\\Core\\FBXLoader\\FBXLoader.h"
#include "..\\Core\\DescriptorHeap\\DescriptorHeap.h"
#include "..\\Core\\Texture2D\\Texture2D.h"

using namespace DirectX;

// �g���q��u�������鏈��
#include <filesystem>
namespace fs = std::filesystem;
static std::wstring ReplaceExtension(const std::wstring& origin, const char* ext)
{
	fs::path p = origin.c_str();
	return p.replace_extension(ext).c_str();
}

void Model::Initialize(const wchar_t* fileName, Camera* camera, bool bCharacter)
{
	m_camera = camera;

	ImportSettings importSetting = ImportSettings
	{
		fileName,
		meshes,
		false,
		true, //�e�N�X�`����UV��V�������]���Ă���ۂ��H�̂œǂݍ��ݎ���UV���W���t�]������
		bCharacter
	};

	FBXLoader loader;
	if (!loader.Load(importSetting))
	{
		printf("FBX�̃��[�h�Ɏ��s\n");
		return;
	}

	//���b�V���̐��������_�o�b�t�@��p�ӂ���
	vertexBuffers.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(Vertex) * meshes[i].Vertices.size();
		auto stride = sizeof(Vertex);
		auto vertices = meshes[i].Vertices.data();
		for (size_t j = 0; j < meshes[i].Vertices.size(); j++)
			vertices[j].Position.x += 0.5f;
		auto pVB = new VertexBuffer(size, stride, vertices);
		if (!pVB->IsValid())
		{
			printf("���_�o�b�t�@�̐����Ɏ��s\n");
			return;
		}

		vertexBuffers.push_back(pVB);
	}

	//���b�V���̐������C���f�b�N�X�o�b�t�@��p�ӂ���
	indexBuffers.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(uint32_t) * meshes[i].Indices.size();
		auto indices = meshes[i].Indices.data();
		auto pIB = new IndexBuffer(size, indices);
		if (!pIB->IsValid())
		{
			printf("�C���f�b�N�X�o�b�t�@�̐����Ɏ��s\n");
			return;
		}

		indexBuffers.push_back(pIB);
	}

	// �}�e���A���̓ǂݍ���
	descriptorHeap = new DescriptorHeap();
	materialHandles.clear();
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto texPath = ReplaceExtension(meshes[i].DiffuseMap, "png"); //
		auto mainTex = Texture2D::Get(texPath);
		auto handle = descriptorHeap->Register(mainTex);
		//printf("Texture = %s\n", FBXLoader::ToUTF8(texPath).c_str());
		materialHandles.push_back(handle);
	}

	rootSignature = new RootSignature();
	if (!rootSignature->IsValid())
	{
		printf("���[�g�V�O�l�`���̐����Ɏ��s\n");
		return;
	}

	pipelineState = new PipelineState();
	pipelineState->SetInputLayout(Vertex::InputLayout);
	pipelineState->SetRootSignature(rootSignature->Get());
	pipelineState->SetVS(L"x64/Debug/SimpleVS.cso");
	pipelineState->SetPS(L"x64/Debug/SimplePS.cso");
	pipelineState->Create();
	if (!pipelineState->IsValid())
	{
		printf("�p�C�v���C���X�e�[�g�̐����Ɏ��s\n");
		return;
	}

	m_pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_rot = XMFLOAT3(-90.0f, 0.0f, 0.0f);
}

void Model::Update()
{
	Transform* ptr1 = m_camera->m_constantBuffer;
	Transform* ptr2 = m_constantBuffer[g_Engine->CurrentBackBufferIndex()]->GetPtr<Transform>();

	XMMATRIX matScale = XMMatrixScaling(10.0f, 10.0f, 10.0f);
	XMMATRIX matPos = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	XMMATRIX matRot = XMMatrixRotationX(m_rot.x);
	matRot *= XMMatrixRotationY(m_rot.y);
	matRot *= XMMatrixRotationZ(m_rot.z);

	ptr2->World = matScale * matRot * matPos;
	ptr2->View = ptr1->View;
	ptr2->Proj = ptr1->Proj;
}

void Model::Draw()
{
	UINT currentBufferIndex = g_Engine->CurrentBackBufferIndex();
	auto commandList = g_Engine->CommandList();
	auto materialHeap = descriptorHeap->GetHeap(); // �f�B�X�N���v�^�q�[�v

	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto vbView = vertexBuffers[i]->View();
		auto ibView = indexBuffers[i]->View();
		commandList->SetGraphicsRootSignature(rootSignature->Get());
		commandList->SetPipelineState(pipelineState->Get());
		commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer[currentBufferIndex]->GetAddress());

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vbView);
		commandList->IASetIndexBuffer(&ibView);

		commandList->SetDescriptorHeaps(1, &materialHeap); // �g�p����f�B�X�N���v�^�q�[�v���Z�b�g
		commandList->SetGraphicsRootDescriptorTable(1, materialHandles[i]->HandleGPU); // ���̃��b�V���ɑΉ�����f�B�X�N���v�^�e�[�u�����Z�b�g

		commandList->DrawIndexedInstanced((UINT)meshes[i].Indices.size(), 1, 0, 0, 0);
	}
}

void Model::SetPosition(DirectX::XMFLOAT3 pos)
{
	m_pos = pos;
}

Model::Model()
{
	for (int i = 0; i < Engine::FRAME_BUFFER_COUNT; i++) {
		m_constantBuffer[i] = new ConstantBuffer(sizeof(Transform));
	}
}
