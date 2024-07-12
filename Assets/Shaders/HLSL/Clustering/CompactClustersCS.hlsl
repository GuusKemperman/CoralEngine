// Buffer that holds a flag for each cluster that contain samples
StructuredBuffer<bool> activeClusters : register(t0);
// For each active cluster, append its index
RWStructuredBuffer<uint> RWCompactClusters : register(u0);

[numthreads(1, 1, 1)]
void main(const uint3 DTid : SV_DispatchThreadID)
{
    uint clusterID = DTid.x;
    
    if (activeClusters[clusterID])
    {
        uint index = RWCompactClusters.IncrementCounter();
        RWCompactClusters[index] = clusterID;
    }
}