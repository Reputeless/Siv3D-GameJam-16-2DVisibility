
struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

cbuffer psConstants1 : register( b1 )
{
	float2 g_lightPos;
	float g_lightDistance;
};

float4 PS(VS_OUTPUT input) : SV_Target
{
	const float d = saturate(distance(input.position.xy, g_lightPos) / g_lightDistance);

	input.color.rgb *= (1.0 - d);

	return input.color;
}
