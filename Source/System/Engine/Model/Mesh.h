#pragma once

#include <d3d12.h>

#include "..\\..\\ComPtr.h"

struct MeshData
{
    ComPtr<ID3D12Resource> vertexBuffer;            //頂点バッファ(GPUメモリに頂点情報を保存)
    ComPtr<ID3D12Resource> indexBuffer;             //インデックスバッファ(GPUメモリに入っている頂点バッファの各頂点にインデックスを付けて保存)
    ComPtr<ID3D12Resource> contentsBuffer;          //頂点数やシェイプキーの数など初期化後一度だけ送信する用 (ヒューマノイドモデルのみ設定)
    ComPtr<ID3D12Resource> shapeDeltasBuffer;       //各頂点に対するシェイプキーの位置情報                   (ヒューマノイドモデルのみ設定)
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;      //頂点バッファのデータ内容とサイズを保持
    D3D12_INDEX_BUFFER_VIEW indexBufferView;        //インデックスバッファのデータ内容とサイズを保持

    UINT vertexCount;                               //頂点数
    UINT indexCount;                                //インデックス数
    UINT shapeDataIndex;                            //ディスクリプタヒープ上のシェイプキーのデータが存在する場所

    MeshData()
        : indexBufferView(D3D12_INDEX_BUFFER_VIEW())
        , vertexBufferView(D3D12_VERTEX_BUFFER_VIEW())
        , vertexCount(0)
        , indexCount(0)
        , shapeDataIndex(0)
    {
    }
};
