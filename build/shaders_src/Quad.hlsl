
struct VS_INPUT_QUAD {
    uint   VertexID     :   SV_VertexID;
};

struct VS_OUTPUT_QUAD {
    float4 Pos          :   SV_POSITION;
    float2 TexCoord     :   TEXCOORD0;
};


VS_OUTPUT_QUAD VS(VS_INPUT_QUAD input) {
	VS_OUTPUT_QUAD output = (VS_OUTPUT_QUAD)0;

	uint id = input.VertexID;

	float x = -1, y = -1;
	x = (id == 2) ? 3.0 : -1.0;
	y = (id == 1) ? 3.0 : -1.0;

	output.Pos			= float4(x, y, 1.0f, 1.0f);

	output.TexCoord		= output.Pos.xy * 0.5f + 0.5f;
	output.TexCoord.y	= 1.0 - output.TexCoord.y;

	return output;
}

float4 PS(VS_OUTPUT_QUAD input) : SV_Target{
	return float4(input.TexCoord.x, input.TexCoord.y, 0.0f, 1.0f);
}
