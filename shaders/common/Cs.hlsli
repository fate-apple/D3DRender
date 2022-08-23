#ifndef CS_H
#define CS_H

struct CsInput
{
    uint3 groupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 groupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 dispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  groupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
    // SV_DispatchThreadID = groupThreadID * (threadInGroupX, threadInGroupY, threadInGroupZ) + SV_GroupID
    // SV_GroupIndex = groupThreadID.Z * threadInGroupX * threadInGroupY + groupThreadID.Y * threadInGroupX + groupThreadID.X
};

#endif