#pragma once

#include "..\\..\\Core\\RootSignature\\RootSignature.h"
#include "..\\..\\Core\\PipelineState\\PipelineState.h"
#include "..\\..\\Core\\DescriptorHeap\\DescriptorHeap2.h"
#include "..\\..\\Lights\\DirectionalLight.h"
#include "..\\..\\Core\\Texture2D\\Texture2D.h"

class Material
{
public:
    Material(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, DirectionalLight* pDirectionalLight, DescriptorHeap* pDescriptorHeap, UINT materialID);
    ~Material();

    //�f�t�H���g�̃e�N�X�`�����Z�b�g
    void Initialize(ID3D12Resource* pDefaultMainTex, ID3D12Resource* pDefaultNormalTex);

    //���[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g���w��
    void SetPipeline(RootSignature* pRootSignature, PipelineState* pPipelineState, ShaderKinds shaderKind);
    //�e�N�X�`������ւ�
    void SetMainTexture(std::string texPath);
    //�m�[�}���}�b�v����ւ�
    void SetNormalMap(std::string texPath);
    //�V�F�C�v�L�[�̃��\�[�X��ݒ�
    void SetShapeData(UINT index, ID3D12Resource* pShapeTexture);

    void SetIsTransparent(bool bTransparent);

    //���[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g��GPU�ɑ��M
    void ExecutePipeline();
    //�e�N�X�`������GPU�ɑ��M
    void ExecuteShapeData(UINT index);

public: //�Q�b�^�[
    D3D12_GPU_DESCRIPTOR_HANDLE GetShapeData(UINT offset);
    bool GetIsTransparent() const;

private:
    ID3D12Device* m_pDevice;                    //�G���W���̃f�o�C�X
    ID3D12GraphicsCommandList* m_pCommandList;  //�G���W���̃R�}���h���X�g

    RootSignature* m_pRootSignature;            //���[�g�V�O�l�`��
    PipelineState* m_pPipelineState;            //�p�C�v���C���X�e�[�g
    DescriptorHeap* m_pDescriptorHeap;          //�}�e���A��

    DirectionalLight* m_pDirectionalLight;      //����

    const UINT m_materialID;                 //�}�e���A��ID (�f�B�X�N���v�^�q�[�v��Index�BMaterialManager�ŊǗ�)

    Texture2D* m_pMainTexture;      //�e�N�X�`��
    Texture2D* m_pNormalTexture;    //�m�[�}���}�b�v

    ShaderKinds m_shaderKind;       //�V�F�[�_�[�̎��

    bool m_bTransparent;    //�������̃I�u�W�F�N�g���ǂ���

    //�R�s�[�K�[�h
    void operator =(const Material& src){};
    Material(const Material& src);
};