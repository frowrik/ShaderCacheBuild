
struct VS_INPUT_IMGUI {
	float2 pos          :   POSITION;
	float4 col          :   COLOR0;
	float2 uv           :   TEXCOORD0;
};

struct VS_OUTPUT_IMGUI {
    float4 pos          :   SV_POSITION;
    float4 col          :   COLOR0;
    float2 uv           :   TEXCOORD0;
};

cbuffer ImGui_Buffer : register(b0) {
    float4x4        ProjectionMatrix;
};

SamplerState 	sampler0:        register(s0);
Texture2D 		texture0:        register(t0);

VS_OUTPUT_IMGUI VS(VS_INPUT_IMGUI input) {
	VS_OUTPUT_IMGUI output;
	output.pos	= mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));
	output.col	= input.col;
	output.uv	= input.uv;
	return output;
};

float4 PS(VS_OUTPUT_IMGUI input) : SV_Target{
	float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
	return out_col;
};
