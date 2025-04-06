#pragma once

#include "..\\..\\Core\\RootSignature\\RootSignature.h"
#include "..\\..\\Core\\PipelineState\\PipelineState.h"
#include "..\\..\\Core\\DescriptorHeap\\DescriptorHeap2.h"
#include "..\\..\\Lights\\DirectionalLight.h"
#include "..\\..\\Core\\Texture2D\\Texture2D.h"

class Material
{
    friend class ModelManager;

public:
    Material(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, DescriptorHeap* pDescriptorHeap, UINT materialID);
    ~Material();

    //�f�t�H���g�̃e�N�X�`�����Z�b�g
    void Initialize(ID3D12Resource* pDefaultMainTex, ID3D12Resource* pDefaultNormalTex);

    //���[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g���w��
    void SetPipeline(RootSignature* pRootSignature, PipelineState* pPipelineState, ShaderKinds shaderKind);
    //�e�N�X�`������ւ�
    void SetMainTexture(std::string texPath);
    void SetMainTexture(ID3D12Resource* pResource);
    //�m�[�}���}�b�v����ւ�
    void SetNormalMap(std::string texPath);
    //�V�F�C�v�L�[�̃��\�[�X��ݒ�
    //���� : Mesh�N���X����shapeDataIndex, ID3D12Resource* �V�F�C�v���̃��\�[�X
    void SetShapeData(UINT index, ID3D12Resource* pShapeTexture);

    void SetIsTransparent(bool bTransparent);
    void SetOpacity(float value);

public: //�Q�b�^�[
    //�V�F�C�v��񂪓����Ă���n���h�����擾
    //���� : Mesh�N���X����shapeDataIndex
    D3D12_GPU_DESCRIPTOR_HANDLE GetShapeData(UINT offset);

    //�q�̃}�e���A�������������ǂ������擾
    bool GetIsTransparent() const;

    float GetOpacity() const { return m_transparentValue; }

public:
    DirectX::XMFLOAT4 m_ambientColor;
    DirectX::XMFLOAT4 m_diffuseColor;

private:
    ID3D12Device* m_pDevice;                    //�G���W���̃f�o�C�X
    ID3D12GraphicsCommandList* m_pCommandList;  //�G���W���̃R�}���h���X�g

    RootSignature* m_pRootSignature;            //���[�g�V�O�l�`��
    PipelineState* m_pPipelineState;            //�p�C�v���C���X�e�[�g
    DescriptorHeap* m_pDescriptorHeap;          //�}�e���A��

    DirectionalLight* m_pDirectionalLight;      //����
    ComPtr<ID3D12Resource> m_psBufferResource;

    const UINT m_materialID;                    //�}�e���A��ID (�f�B�X�N���v�^�q�[�v��Index�BMaterialManager�ŊǗ�)

    Texture2D* m_pMainTexture;      //�e�N�X�`��
    Texture2D* m_pNormalTexture;    //�m�[�}���}�b�v

    ShaderKinds m_shaderKind;       //�V�F�[�_�[�̎��

    float m_transparentValue;
    bool m_bTransparent;    //�������̃I�u�W�F�N�g���ǂ���

    void* m_pPSBufferMap;

    //���[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g��GPU�ɑ��M
    void ExecutePipeline();
    //�e�N�X�`������GPU�ɑ��M
    void ExecuteShapeData(UINT index);

    //�R�s�[�K�[�h
    void operator =(const Material& src){};
    Material(const Material& src);
};