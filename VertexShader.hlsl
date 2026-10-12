// an ultra simple hlsl vertex shader
// TODO: Part 1c
// TODO: Part 2b
	// TODO: Part 3b
	// TODO: Part 3f
// TODO: Part 3g
#pragma pack_matrix(row_major)

cbuffer ShaderVars : register(b0)
{
    float4x4 world[6];
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
}

float4 main(float4 inputPos : POSITION, uint instanceID : SV_InstanceID) : SV_POSITION
{
    // TODO: Part 2i
	// TODO: Part 3b
	// TODO: Part 3g
    // Transform vertex using the world matrix corresponding to the instance ID
    float4 worldPosition = mul(inputPos, world[instanceID]);
    
    // Transform from world space to view space
    float4 viewPosition = mul(worldPosition, viewMatrix);
    
    // Transform from view space to clip space
    float4 clipPosition = mul(viewPosition, projectionMatrix);
    
    return clipPosition;
}