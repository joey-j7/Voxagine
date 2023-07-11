#include "Defines.hlsl"

CONSTANT_BUFFER Data : register(b0)
{
    matrix mvp;
    matrix mv;
	
    float4 camPosition;
    float4 camOffset;
	
	float4 viewport;
    float4 lightDirection;
	
    uint4 worldSize;
	
	uint sdfCount;
	uint particleCount;

    uint lightCount;
    float padding;
};

struct SDF
{
	float3 Position;
	float3 Extents;
	uint MapperID;
	float Distance;
};

struct LightPoint
{
    float3 pos;
    float3 col;
    float range;
    float strength;
};

STRUCTURED_BUFFER(SDF) SDFs : register(t0);
STRUCTURED_BUFFER(LightPoint) pointLights : register(t1);

#ifdef __PSSL__
BUFFER(uint) voxelWorldData : register(t2);
#else
BUFFER(float4) voxelWorldData : register(t2);
#endif

Texture2D<float4> particlePass : register(t3);
Texture2D<float> depthPass : register(t4);
Texture2D<float> shadowPass : register(t5);

SamplerState s0 : register(s0);
SamplerState s1 : register(s1);

struct CollisionResult
{
	float Distance;
	float Penetration;
	bool Collided;
};

struct SDFCheck
{
	uint SDFID;
	CollisionResult Collision;
	bool ShouldCheck;
};

#define MAX_SDF_CHECK_COUNT 32

static uint maxVoxelID;

static SDFCheck checkSDFs[MAX_SDF_CHECK_COUNT];
static uint checkSDFCount = 0;

static float3 rayOrigin, rayDirection, rayPosition;

static float nearestDistance = 10000;
static uint nearestSDF = 0;
	
static float3 chunkSize, invChunkSize;

#include "SDFMarcher.hlsl"
#include "AmbientOcclusion.hlsl"

static MarchResult res;

CollisionResult RayAABB(float3 boxMin, float3 boxMax, float3 rayOrigin, float3 invRayDirection)
{
    float3 tp0 = (boxMin - rayOrigin) * invRayDirection;
	float3 tp1 = (boxMax - rayOrigin) * invRayDirection;
	float3 tmin = min(tp0, tp1), tmax = max(tp0, tp1);
	
	float maxMin = max(tmin.x, max(tmin.y, tmin.z));
	float minMax = min(tmax.x, min(tmax.y, tmax.z));
	
	CollisionResult result;
	result.Distance = max(0.0, maxMin);
	result.Penetration = minMax - maxMin;
	result.Collided = minMax > 0.0 && maxMin < minMax; //>
	
	return result;
}

void CheckIfNearest(SDFCheck check, uint id)
{
	if (!check.ShouldCheck)
		return;
		
	if (check.ShouldCheck && check.Collision.Distance < nearestDistance)
	{
		nearestDistance = check.Collision.Distance;
		nearestSDF = id;
	}
}

void GetCollidingAABBs(float3 origin, float3 direction)
{
	nearestDistance = 10000;
	nearestSDF = 0;
	
	checkSDFCount = 0;
	
	float3 invDirection = 1.0 / direction;
	CollisionResult result;
	
	// Gather boxes that will hit the ray
	for (uint i = 0; i < sdfCount; ++i)
	{
		// Clamp to 1.0 so that it doesn't clip with the ground mesh
		result = RayAABB(
			max(float3(-1000, 1.0, -1000), SDFs[i].Position - SDFs[i].Extents),
			max(float3(-1000, 1.0, -1000), SDFs[i].Position + SDFs[i].Extents),
			origin, invDirection
		);
		
		if (result.Collided)
		{
			checkSDFs[checkSDFCount].SDFID = i;
			checkSDFs[checkSDFCount].Collision = result;
			checkSDFs[checkSDFCount].ShouldCheck = true;
		
			CheckIfNearest(checkSDFs[checkSDFCount], checkSDFCount);
			
			checkSDFCount++;
			
			#if OPTIMIZED
			break;
			#endif
		}
	}
}

float GetDirectLighting(float offset) {
    float fDifference = clamp(dot(res.Normal, -lightDirection.xyz), 0.0, 1.0);
	float fShadow = fDifference * DIRECT_VALUE * offset + AMBIENT_VALUE;

	return fShadow;
	
#if DIRECT_LIGHTING
	//return fShadow;
	
	float3 position = (res.SmoothPosition + res.Normal * 0.01);
	
#if DIRECT_LIGHTING == 1
	GetCollidingAABBs(position, -lightDirection.xyz);
#endif

	if (fDifference > 0.1)
	{
#if DIRECT_LIGHTING == 1
		if (checkSDFCount > 0)
		{
			position += -lightDirection.xyz * max(0.01, nearestDistance);
#endif
			
			MarchResult res2 = March(
				position - lightDirection.xyz * (offset - 2.0),
				-lightDirection.xyz,
				float3(
					float(worldSize.x),
					float(worldSize.y),
					float(worldSize.z)
				),
				128,
				false
			);
		
			if (res2.Color.a > 0.0) {
				//fShadow = min(res2.Distance * 0.0125 * fDifference + AMBIENT_VALUE, 1.0);
                fShadow = AMBIENT_VALUE;
        }			
#if DIRECT_LIGHTING == 1
		}
#endif
	}	
#endif

	return fShadow;
}

//Point lighting functions
float GetPointLightingSpecular(float3 camPos, float3 fragPos, float3 norm, LightPoint light){
    //BLINN-PHONG////////////////////////////////////////////////////////////////////////////////
    /*float3 ldir = normalize(light.pos-fragPos);
    
    float3 viewDir = normalize(camPos - fragPos);
    
    float3 halfwayDir = normalize(ldir + viewDir);  
    return pow(max(dot(norm, halfwayDir), 0.0), 8.0) * (light.strength*1.5);*/

    //PHONG//////////////////////////////////////////////////////////////////////////////////////
    float3 viewDir = normalize(camPos - fragPos);
    float3 ldir = normalize(light.pos-fragPos);
    float3 reflectDir = reflect(-ldir, norm);
    float specAngle = max(dot(reflectDir, viewDir), 0.0);
	
    // note that the exponent is different here
    return pow(specAngle, 8.0);
}

float GetPointLighting(float3 fragPos, float3 norm, LightPoint light){
    float dist = max(light.range-distance(fragPos, light.pos),0.0)/light.range;
    float3 lVec = normalize(light.pos-fragPos);
    float dotres = max(dot(norm,lVec),0.0);
    
    return dist*dotres*light.strength;
}

struct FS_in
{
	float2 UVs			: TEXCOORD0;
	float4 Position		: POS_OUT;
	float4 Direction 	: POSITION;
};

float4 main(FS_in IN) : TAR_OUT
{
	float4 particleColor = particlePass.Sample(s1, IN.Position.xy / viewport.xy);
    float dist = depthPass.Sample(s0, IN.Position.xy / viewport.xy);
    float distShad = shadowPass.Sample(s1, IN.Position.xy / viewport.xy);
	
	//return float4(distShad/2000, distShad/2000, distShad/2000, 1.0);
	
    /*if (distShad < 0.f)
    {
        return float4(1.0,0.0,0.0,1.0);

    }
    else
    {
        return float4(0.0, 1.0, 0.0, 1.0);

    }*/
    // return float4(dist / 1000.0, dist / 1000.0, dist / 1000.0, 1.0);
    //return float4(distShad/100.0,distShad/100.0,distShad/100.0,1.0);
	
	if (particleColor.r != 0.0 || particleColor.g != 0.0 || particleColor.b != 0.0)
	{
		particleColor.a = 1.0;

        return particleColor;
	}
    
	uint maxDim = distance(float3(0.0, 0.0, 0.0), worldSize.xyz);
	
	// Construct ray
	rayOrigin = camPosition.xyz - camOffset.xyz;
	rayDirection = normalize(IN.Direction.xyz);
	
	maxVoxelID = worldSize.x * worldSize.y * worldSize.z;
	chunkSize = float3(float(worldSize.x), float(worldSize.y), float(worldSize.z));
	invChunkSize = 1.0 / chunkSize;
	
	res = March(
		rayOrigin + rayDirection * (dist - 0.5),
		rayDirection,
		chunkSize,
		maxDim,
		true
	);
	
	// Test shader performance
	// return float4(numStepsTaken * 0.01, 0.0, 0.0, 1.0);
	
    if (res.Color.a != 0)
    {
        float3 endPos = res.Position;
    
		// Ambient occlusion
        float4 ambient = GetAmbientOcclusion(endPos, res.Mask, res.SRDirection, res.Normal, res.UV) * 2.0 - 1.0;
		
        //if (length(ambient) > 0.0)
        //    res.Color *= ambient;
		
		// Apply lighting
        float fDirectLighting = GetDirectLighting(distShad);

		// Shine line
        if (res.Normal.z < -0.5 && res.UV.y >= 0.9 && !IsVoxel(endPos + float3(0.0, 1.0, 0.0)))
            fDirectLighting *= 1.02 * (1.0 + max(0.0, res.UV.y - 0.9) * 4.0);
        else if (res.Normal.y > 0.5 && res.UV.y <= 0.1 && !IsVoxel(endPos + float3(0.0, 0.0, -1.0)))
            fDirectLighting *= 1.02 * (1.0 + max(0.0, 0.1 - res.UV.y) * 4.0);

        //Light colour data
        float3 lightCol = float3(fDirectLighting, fDirectLighting, fDirectLighting);
        //float3 specCol = float3(0.0,0.0,0.0);

        //res.Color.xyz *= lightCol;
        //return float4(res.Color.xyz,1.0);

        //Get point light data
        for (uint i = 0; i < lightCount; i++)
        {
            float strength = GetPointLighting(res.Position, res.Normal, pointLights[i]);
            lightCol += max(pointLights[i].col * strength, 0.0);
             //float specStrength = GetPointLightingSpecular(camPosition.xyz, res.Position, res.Normal, pointLights[i]);
             //specCol += pointLights[i].col * specStrength;
        }
         
        if (length(ambient) > 0.0)
            lightCol *= ambient;
         
        res.Color.xyz *= lightCol;
        //res.Color.xyz += specCol;
		
		// Shine line
		//if (res.Normal.z < -0.5 && res.UV.y >= 0.9 && !IsVoxel(endPos + float3(0.0, 1.0, 0.0)))
		//	res.Color *= 1.02 * (1.0 + max(0.0, res.UV.y - 0.9) * 4.0);
		//else if (res.Normal.y > 0.5 && res.UV.y <= 0.1 && !IsVoxel(endPos + float3(0.0, 0.0, -1.0)))
		//	res.Color *= 1.02 * (1.0 + max(0.0, 0.1 - res.UV.y) * 4.0);
		
        res.Color.a = 1.0;
		
        //return float4(lightCol.r,lightCol.g,lightCol.b,1.0);
        return res.Color;
    }

	// "Endless" ground
	if (rayOrigin.y >= 0 && rayDirection.y < 0)
	{
		float3 endPos;
		
		float t = (-1.0 * rayOrigin.y) / rayDirection.y;
		float3 realEndPos = float3(rayOrigin.x + rayDirection.x * t, 0.0, rayOrigin.z + rayDirection.z * t);
		
		endPos.x = abs(fmod(realEndPos.x, chunkSize.x));
		endPos.y = 0.0;
		endPos.z = abs(fmod(realEndPos.z, chunkSize.z));
		
		res.Color = GetVoxel(endPos);
		
		//res.Normal = float3(0.0, 1.0, 0.0);
		//res.SmoothPosition = realEndPos;
		//res.Position = floor(realEndPos);
		
		//float fDirectLighting = GetDirectLighting();
		//res.Color.xyz *= float3(fDirectLighting, fDirectLighting, fDirectLighting);

        res.Color *= 0.6;
		res.Color.a = 1.0;
		
		return res.Color;
	}
	
	return float4(150.0 / 255.0, 230.0 / 255.0, 255.0 / 255.0, 1.0);
}