#pragma once
#include "Dx.h"

#define CREATE_GRAPHICS_PSO DxGraphicsPipelineGenerator()

struct DxRootSignature
{
    com<ID3D12RootSignature> rootSignature;
    uint32* descriptorTableSizes;
    uint32 numDescriptorTables;
    uint32 tableRootParameterMask;      //32 bit. bit X is 1 if and only if pParameters[X].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE
    uint32 totalNumParameters;
};



DxRootSignature CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc);

Dx12CommandSignature CreateCommandSignature(DxRootSignature dxRootSignature,
                                            D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc);
Dx12CommandSignature CreateCommandSignature(DxRootSignature rootSignature,
                                            D3D12_INDIRECT_ARGUMENT_DESC* indirectArgumentDescs, uint32 numArgumentDescs,
                                            uint32 commandStructureSize);

struct DxGraphicsPipelineGenerator
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;

    DxGraphicsPipelineGenerator()
    {
        desc = {};
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.RasterizerState.FrontCounterClockwise = true; // keep same with unreal default opinion : CM_CW
        desc.SampleDesc = {1, 0};
        desc.SampleMask = 0xFFFFFFFF;
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    }

    operator const D3D12_GRAPHICS_PIPELINE_STATE_DESC&()
    {
        return desc;
    }

    DxGraphicsPipelineGenerator& RootSignature(const DxRootSignature& rootSignature)
    {
        desc.pRootSignature = rootSignature.rootSignature.Get();
        return *this;
    }
    DxGraphicsPipelineGenerator& Vs(Dx12Blob shader)
    {
        desc.VS = CD3DX12_SHADER_BYTECODE(shader.Get());
        return *this;
    }
    DxGraphicsPipelineGenerator& Ps(Dx12Blob shader)
    {
        desc.PS = CD3DX12_SHADER_BYTECODE(shader.Get());
        return *this;
    }
    DxGraphicsPipelineGenerator& Gs(Dx12Blob shader)
    {
        desc.GS = CD3DX12_SHADER_BYTECODE(shader.Get());
        return *this;
    }
    DxGraphicsPipelineGenerator& Ds(Dx12Blob shader)
    {
        desc.DS = CD3DX12_SHADER_BYTECODE(shader.Get());
        return *this;
    }
    DxGraphicsPipelineGenerator& Hs(Dx12Blob shader)
    {
        desc.HS = CD3DX12_SHADER_BYTECODE(shader.Get());
        return *this;
    }

    template<uint32 numElements>
    DxGraphicsPipelineGenerator& InputLayout(D3D12_INPUT_ELEMENT_DESC (&elementDescs)[numElements])
    {
        desc.InputLayout = {elementDescs, numElements};
        return *this;
    }
    
    DxGraphicsPipelineGenerator& RenderTargets(const DXGI_FORMAT* rtFormats, uint32 numRenderTargets,
                                               DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN)
    {
        memcpy(desc.RTVFormats, rtFormats, numRenderTargets * sizeof(numRenderTargets));
        desc.NumRenderTargets = numRenderTargets;
        desc.DSVFormat = dsvFormat;
        return *this;
    }

    DxGraphicsPipelineGenerator& CullFrontFaces()
    {
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
        return *this;
    }

    DxGraphicsPipelineGenerator& CullingOff()
    {
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        return *this;
    }

    DxGraphicsPipelineGenerator& RasterizeClockwise(bool isClockwise)
    {
        desc.RasterizerState.FrontCounterClockwise = !isClockwise;
        return *this;
    }

    DxGraphicsPipelineGenerator& DepthSettings(bool depthEnable = true, bool depthWrite = true,
                                               D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_LESS)
    {
        desc.DepthStencilState.DepthEnable = depthEnable;
        desc.DepthStencilState.DepthWriteMask = depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        desc.DepthStencilState.DepthFunc = comparisonFunc;
        return *this;
    }
};

class DxPipelineStreamBase
{
    virtual void SetVertexShader(Dx12Blob blob)
    {
    }
    virtual void SetPixelShader(Dx12Blob blob)
    {
    }
    virtual void SetDomainShader(Dx12Blob blob)
    {
    }
    virtual void SetHullShader(Dx12Blob blob)
    {
    }
    virtual void SetGeometryShader(Dx12Blob blob)
    {
    }
    virtual void SetMeshShader(Dx12Blob blob)
    {
    }
    virtual void SetAmplificationShader(Dx12Blob blob)
    {
    } //dispatch threadgroups of Mesh Shaders.
    virtual void SetRootSignature(DxRootSignature rootSignature) = 0;
};

struct RootDescriptorTable : CD3DX12_ROOT_PARAMETER
{
    RootDescriptorTable(uint32 numDescriptorRanges, const D3D12_DESCRIPTOR_RANGE* descriptorRanges, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
    {
        InitAsDescriptorTable(numDescriptorRanges, descriptorRanges, visibility);
    }
};

template<typename T>
struct RootConstants : CD3DX12_ROOT_PARAMETER
{
    RootConstants(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
    {
        InitAsConstants(sizeof(T)/4, shaderRegister, space, visibility);
    }
};

struct RootCbv : CD3DX12_ROOT_PARAMETER
{
    RootCbv(uint32 shaderRegister,
        uint32 space = 0,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
    {
        InitAsConstantBufferView(shaderRegister, space, visibility);
    }
};

struct DxPipeline
{
    Dx12PipelineState* pipelineStateObject;
    DxRootSignature* rootSignature;
};
union GraphicsPipelineFiles
{
    struct
    {
        const char* vs;
        const char* ps;
        const char* ds;
        const char* hs;
        const char* gs;
        const char* ms;
        const char* as;
    };
    const char* shaders[7] = {};
};
enum RsFile
{
    RsInVS,
    RSInPS,
    RsInDS,
    RsInHS,
    RSInGS,
    RsInMeshShader,
    RsInAmplificationShaderShader,
    RsFileTypeCount
};

DxPipeline CreateReloadablePipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files,
                                    DxRootSignature rootSignature);
DxPipeline CreateReloadablePipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files,
                                    RsFile rootSignature = RSInPS);
DxPipeline CreateReloadablePipeline(const char* csFile);