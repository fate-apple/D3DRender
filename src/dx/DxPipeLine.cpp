#include "pch.h"
#include <deque>
#include <set>
#include <unordered_map>

#include "DxContext.h"
#include "DxPipeline.h"
#include "core/string.h"

enum DescType
{
    DescTypeStruct,
    DescTypeStream
};
enum PipelineType
{
    PipelineTypeGraphics,
    PipelineTypeCompute
};
struct ReloadablePipelineState // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    ReloadablePipelineState()
    {
    } //not default.
    PipelineType pipelineType;
    DescType descType;
    Dx12PipelineState pipelineState;
    DxRootSignature* rootSignature;
    D3D12_INPUT_ELEMENT_DESC inputLayout[16];
    union // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        //TODO: sjw.  Anonymous types declared in an anonymous union are an extension. gcc: member with constructor not allowed in anonymous aggregate
        struct
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc;
            D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
            DxPipelineStreamBase* stream;
            GraphicsPipelineFiles graphicsPipelineFiles;
        };
        struct
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc;
            const char* computePipelineFile;
        };
    };

    void Initialize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files,
                    DxRootSignature* rootSignature)
    {
        this->pipelineType = PipelineTypeGraphics;
        this->descType = DescTypeStruct;
        this->graphicsPipelineStateDesc = desc;
        this->graphicsPipelineFiles = files;
        this->rootSignature = rootSignature;

        assert(desc.InputLayout.NumElements <= arraySize(inputLayout));
        memcpy(inputLayout, desc.InputLayout.pInputElementDescs,
               sizeof(D3D12_INPUT_ELEMENT_DESC) * desc.InputLayout.NumElements);
        this->graphicsPipelineStateDesc.InputLayout.pInputElementDescs = (desc.InputLayout.NumElements != 0)
                                                                             ? inputLayout
                                                                             : nullptr;
    }

    void Initialize(const char* file, DxRootSignature* inRootSignature)
    {
        this->pipelineType = PipelineTypeCompute;
        this->computePipelineStateDesc = {};
        this->computePipelineFile = file;
        this->rootSignature = inRootSignature;
    }
};

struct ReloadableRootSignature
{
    const char* file;
    DxRootSignature rootSignature;
};

struct ShaderFile
{
    Dx12Blob blob;
    std::set<ReloadablePipelineState*> usedByPipelines;
    ReloadableRootSignature* rootSignature;
};

static std::unordered_map<std::string, ShaderFile> shaderBlobs; //cache for compiled shader file
static std::deque<ReloadablePipelineState> pipelines;
static std::deque<ReloadableRootSignature> rootSignaturesFromFiles;
static std::deque<DxRootSignature> userRootSignatures;
static std::mutex mutex;
static std::vector<ReloadablePipelineState*> dirtyPipelines;
static std::vector<ReloadableRootSignature*> dirtyRootSignatures;


static void CopyRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC* desc, DxRootSignature& result)
{
    result.totalNumParameters = desc->NumParameters;

    uint32 numDescriptorTables = 0;
    for (uint32 i = 0; i < desc->NumParameters; ++i)
    {
        if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            ++numDescriptorTables;
            setBit(result.tableRootParameterMask, i);
        }
    }

    result.descriptorTableSizes = new uint32[numDescriptorTables];
    result.numDescriptorTables = numDescriptorTables;

    uint32 index = 0;
    for (uint32 i = 0; i < desc->NumParameters; ++i)
    {
        if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            uint32 numRanges = desc->pParameters[i].DescriptorTable.NumDescriptorRanges;
            result.descriptorTableSizes[index] = 0;
            for (uint32 r = 0; r < numRanges; ++r)
            {
                result.descriptorTableSizes[index] += desc->pParameters[i].DescriptorTable.pDescriptorRanges[r].NumDescriptors;
            }
            ++index;
        }
    }
}

//API: CreateRootSignature. init DxRootSignature DescriptorTables
DxRootSignature CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    Dx12Blob rootSignatureBlob;
    Dx12Blob errorBlob;
    CheckResult(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));
    DxRootSignature rootSignature{};
    CheckResult(dxContext.device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
                                                      rootSignatureBlob->GetBufferSize(),
                                                      IID_PPV_ARGS(&rootSignature.rootSignature)));
    CopyRootSignatureDesc(&desc, rootSignature);
    return rootSignature;
}


Dx12CommandSignature CreateCommandSignature(DxRootSignature dxRootSignature,
                                            D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc)
{
    Dx12CommandSignature commandSignature;
    //The root signature is required if any of the commands in the signature will update bindings on the pipeline.
    //If the only command present is a draw or dispatch, the root signature parameter can be set to NULL.
    CheckResult(dxContext.device->CreateCommandSignature(&commandSignatureDesc,
                                                         commandSignatureDesc.NumArgumentDescs == 1
                                                             ? nullptr
                                                             : dxRootSignature.rootSignature.Get(),
                                                         IID_PPV_ARGS(&commandSignature)));
    return commandSignature;
}

Dx12CommandSignature CreateCommandSignature(DxRootSignature rootSignature,
                                            D3D12_INDIRECT_ARGUMENT_DESC* indirectArgumentDescs, uint32 numArgumentDescs,
                                            uint32 commandStructureSize)
{
    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc;
    //An array of D3D12_INDIRECT_ARGUMENT_DESC structures, containing details of the arguments,
    //including whether the argument is a vertex buffer, constant, constant buffer view, shader resource view, or unordered access view.
    commandSignatureDesc.pArgumentDescs = indirectArgumentDescs;
    commandSignatureDesc.NumArgumentDescs = numArgumentDescs;
    //Specifies the size of each command in the drawing buffer, in bytes.
    commandSignatureDesc.ByteStride = commandStructureSize;
    commandSignatureDesc.NodeMask = 0;
    return CreateCommandSignature(rootSignature, commandSignatureDesc);
}

//D3DReadFileToBlob if no cache. update shaderBlob.usedByPipelines
static ReloadableRootSignature* PushBlob(const char* filename, ReloadablePipelineState* pipelineStatePtr,
                                         bool isRootSignature = false)
{
    if(!filename) return nullptr;
    ReloadableRootSignature* result = nullptr;
    auto it = shaderBlobs.find(filename);
    if(it == shaderBlobs.end()) {
        std::wstring filePath = SHADER_BIN_DIR + stringToWstring(filename) + L".cso";
        Dx12Blob blob;
        CheckResult(D3DReadFileToBlob(filePath.c_str(), &blob));
        if(isRootSignature) {
            rootSignaturesFromFiles.push_back({filename, DxRootSignature{nullptr}});
            result = &rootSignaturesFromFiles.back();
        }
        mutex.lock();
        shaderBlobs[filename] = ShaderFile{blob, {pipelineStatePtr}, result};
        mutex.unlock();
    } else {
        mutex.lock();
        it->second.usedByPipelines.insert(pipelineStatePtr);
        mutex.unlock();
        if(isRootSignature) {
            if(!it->second.rootSignature) {
                //when?
                rootSignaturesFromFiles.push_back({filename, DxRootSignature{nullptr}});
                it->second.rootSignature = &rootSignaturesFromFiles.back();
            }
            result = it->second.rootSignature;
        }
    }

    return result;
}

//
DxPipeline CreateReloadablePipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files,
                                    RsFile rsFile)
{
    pipelines.emplace_back();
    auto& reloadablePso = pipelines.back();
    //insert shader with rs in set. use D3DReadFileToBlob to read cso file first time
    //TODO: sjw. use bit in uint8 instead enum to control whether Use Cache
    ReloadableRootSignature* reloadableRootSignature = PushBlob(files.shaders[rsFile], &reloadablePso, true);
    PushBlob(files.vs, &reloadablePso);
    PushBlob(files.ps, &reloadablePso);
    PushBlob(files.gs, &reloadablePso);
    PushBlob(files.ds, &reloadablePso);
    PushBlob(files.hs, &reloadablePso);
    //TODO: sjw. Entire another pipeline?
    assert(!files.ms);
    assert(!files.as);

    DxRootSignature* rootSignature = &reloadableRootSignature->rootSignature;
    reloadablePso.Initialize(desc, files, rootSignature);
    DxPipeline result = {&reloadablePso.pipelineState, rootSignature};
    return result;
}

DxPipeline CreateReloadablePipeline(const char* csFile)
{
    pipelines.emplace_back();
    auto& state = pipelines.back();

    ReloadableRootSignature* reloadableRS = PushBlob(csFile, &state, true);

    DxRootSignature* rootSignature = &reloadableRS->rootSignature;

    state.Initialize(csFile, rootSignature);

    DxPipeline result = { &state.pipelineState, rootSignature };
    return result;
}