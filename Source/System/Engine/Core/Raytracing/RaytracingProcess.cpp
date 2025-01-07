#include "RaytracingProcess.h"

RaytracingProcess::RaytracingProcess(ID3D12Device6* pDevice, ID3D12GraphicsCommandList4* pCommandList)
    : m_pDevice(pDevice)
    , m_pCommandList(pCommandList)
{
}

HRESULT RaytracingProcess::CreateRaytracingPipeline()
{
    //�V�F�[�_�[���R���p�C��
    ComPtr<ID3DBlob> rayGenShader, missShader, hitShader;
    HRESULT hr = D3DReadFileToBlob(KeyString::SHADER_RAYTRACING_RAYGEN, &rayGenShader);
    if (FAILED(hr)) return hr;

    hr = D3DReadFileToBlob(KeyString::SHADER_RAYTRACING_MISS, &missShader);
    if (FAILED(hr)) return hr;

    hr = D3DReadFileToBlob(KeyString::SHADER_RAYTRACING_HIT, &hitShader);
    if (FAILED(hr)) return hr;

    //�p�C�v���C���L�q�q�̏���
    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    //�T�u�I�u�W�F�N�g��ǉ� (���C�����A�~�X�A�q�b�g�O���[�v�Ȃ�)
    std::vector<D3D12_STATE_SUBOBJECT> subobjects;

    //���C�����V�F�[�_�[
    D3D12_EXPORT_DESC rayGenExport = { L"RayGen", nullptr, D3D12_EXPORT_FLAG_NONE };
    D3D12_DXIL_LIBRARY_DESC rayGenLibrary = {};
    rayGenLibrary.DXILLibrary.pShaderBytecode = rayGenShader->GetBufferPointer();
    rayGenLibrary.DXILLibrary.BytecodeLength = rayGenShader->GetBufferSize();
    rayGenLibrary.NumExports = 1;
    rayGenLibrary.pExports = &rayGenExport;
    D3D12_STATE_SUBOBJECT rayGenSubobject = {};
    rayGenSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    rayGenSubobject.pDesc = &rayGenLibrary;
    subobjects.push_back(rayGenSubobject);

    //�~�X�V�F�[�_�[
    D3D12_EXPORT_DESC missExport = { L"Miss", nullptr, D3D12_EXPORT_FLAG_NONE };
    D3D12_DXIL_LIBRARY_DESC missLibrary = {};
    missLibrary.DXILLibrary.pShaderBytecode = missShader->GetBufferPointer();
    missLibrary.DXILLibrary.BytecodeLength = missShader->GetBufferSize();
    missLibrary.NumExports = 1;
    missLibrary.pExports = &missExport;
    D3D12_STATE_SUBOBJECT missSubobject = {};
    missSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    missSubobject.pDesc = &missLibrary;
    subobjects.push_back(missSubobject);

    //�q�b�g�V�F�[�_�[
    D3D12_EXPORT_DESC hitExport = { L"ClosestHit", nullptr, D3D12_EXPORT_FLAG_NONE };
    D3D12_DXIL_LIBRARY_DESC hitLibrary = {};
    hitLibrary.DXILLibrary.pShaderBytecode = hitShader->GetBufferPointer();
    hitLibrary.DXILLibrary.BytecodeLength = hitShader->GetBufferSize();
    hitLibrary.NumExports = 1;
    hitLibrary.pExports = &hitExport;
    D3D12_STATE_SUBOBJECT hitSubobject = {};
    hitSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    hitSubobject.pDesc = &hitLibrary;
    subobjects.push_back(hitSubobject);

    //�T�u�I�u�W�F�N�g�� stateObjectDesc �ɒǉ�
    stateObjectDesc.pSubobjects = subobjects.data();
    stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());

    //�p�C�v���C����ԃI�u�W�F�N�g�̍쐬
    hr = m_pDevice->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_pRaytracingStateObject));
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

HRESULT RaytracingProcess::CreateBLAS(Model* pModel)
{
    //�W�I���g���L�q�q���쐬
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
    for (const Mesh* pMesh : pModel->m_meshes) {
        D3D12_RAYTRACING_GEOMETRY_DESC geometry = {};
        geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        geometry.Triangles.VertexBuffer.StartAddress = pMesh->vertexBuffer->GetGPUVirtualAddress();
        geometry.Triangles.VertexBuffer.StrideInBytes = pMesh->vertexBufferView.StrideInBytes;
        geometry.Triangles.VertexCount = pMesh->vertexCount;
        geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT; //���_�t�H�[�}�b�g (float3)
        geometry.Triangles.IndexBuffer = pMesh->indexBuffer->GetGPUVirtualAddress();
        geometry.Triangles.IndexCount = pMesh->indexCount;
        geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT; //�C���f�b�N�X�t�H�[�}�b�g (32bit)

        geometryDescs.push_back(geometry);
    }

    //BLAS�쐬�̍\�z����ݒ�
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = static_cast<UINT>(geometryDescs.size());
    inputs.pGeometryDescs = geometryDescs.data();
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    //�K�v�ȃT�C�Y���擾
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    //Scratch�o�b�t�@�̍쐬
    CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes);

    CD3DX12_HEAP_PROPERTIES heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    BLAS blas;

    HRESULT hr = m_pDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&blas.scratchBuffer));
    if (FAILED(hr)) {
        return hr;
    }

    //BLAS�o�b�t�@�̍쐬
    CD3DX12_RESOURCE_DESC blasDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes);
    hr = m_pDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &blasDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&blas.blasBuffer));
    if (FAILED(hr)) {
        return hr;
    }

    //BLAS�\�z�R�}���h���L�^
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.ScratchAccelerationStructureData = blas.scratchBuffer->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = blas.blasBuffer->GetGPUVirtualAddress();

    m_pCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    //�o���A��ǉ�
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(blas.blasBuffer.Get());
    m_pCommandList->ResourceBarrier(1, &barrier);

    m_blasList.push_back(blas);

    return S_OK;
}
