#pragma once
#include "DxTexture.h"

struct DxRenderTarget
{
    DxRenderTarget(uint32 width, uint32 height)
    {
        viewport = {0, 0, (float)width, static_cast<float>(height), 0, 1};
    }

    uint32 numAttachments = 0;
    D3D12_VIEWPORT viewport{};
    DxRtvDescriptorHandle rtv[8]{};
    DxDsvDescriptorHandle dsv = DxDsvDescriptorHandle(CD3DX12_DEFAULT());

    DxRenderTarget& ColorAttachMent(const ref<DxTexture>& attachment, DxRtvDescriptorHandle backup = CD3DX12_DEFAULT())
    {
        assert(_pushIndex < arraySize(rtv));
        rtv[_pushIndex++] = attachment ? attachment->defaultRTV : backup;
        if(attachment) {
            //invalid attachments at the end are ignored
            numAttachments = _pushIndex;
        }
        return *this;
    }

    DxRenderTarget& depthAttachment(const ref<DxTexture>& attachment, DxDsvDescriptorHandle backup = CD3DX12_DEFAULT())
    {
        assert(!dsv);
        dsv = attachment ? attachment -> defaultDSV : backup;
        return *this;
    }
    

private:
    uint32 _pushIndex = 0;
};