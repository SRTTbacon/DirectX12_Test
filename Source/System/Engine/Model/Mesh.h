#pragma once

#include <d3d12.h>

#include "..\\..\\ComPtr.h"

struct MeshData
{
    ComPtr<ID3D12Resource> vertexBuffer;            //���_�o�b�t�@(GPU�������ɒ��_����ۑ�)
    ComPtr<ID3D12Resource> indexBuffer;             //�C���f�b�N�X�o�b�t�@(GPU�������ɓ����Ă��钸�_�o�b�t�@�̊e���_�ɃC���f�b�N�X��t���ĕۑ�)
    ComPtr<ID3D12Resource> contentsBuffer;          //���_����V�F�C�v�L�[�̐��ȂǏ��������x�������M����p (�q���[�}�m�C�h���f���̂ݐݒ�)
    ComPtr<ID3D12Resource> shapeDeltasBuffer;       //�e���_�ɑ΂���V�F�C�v�L�[�̈ʒu���                   (�q���[�}�m�C�h���f���̂ݐݒ�)
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;      //���_�o�b�t�@�̃f�[�^���e�ƃT�C�Y��ێ�
    D3D12_INDEX_BUFFER_VIEW indexBufferView;        //�C���f�b�N�X�o�b�t�@�̃f�[�^���e�ƃT�C�Y��ێ�

    UINT vertexCount;                               //���_��
    UINT indexCount;                                //�C���f�b�N�X��
    UINT shapeDataIndex;                            //�f�B�X�N���v�^�q�[�v��̃V�F�C�v�L�[�̃f�[�^�����݂���ꏊ

    MeshData()
        : indexBufferView(D3D12_INDEX_BUFFER_VIEW())
        , vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
        , vertexCount(0)
        , indexCount(0)
        , shapeDataIndex(0)
    {
    }
};
