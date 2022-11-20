#include "Particle.hlsli"

// 四角形の頂点数
static const uint vnum = 4;

// 左上が0,0 右下が1,1
static const float2 uv_array[vnum] =
{
	float2(0, 1),
	float2(0, 0),
	float2(1, 1),
	float2(1, 0)
};

// センターからのオフセット
static const float4 offset_array[vnum] =
{
	float4(-0.5f, -0.5f, 0, 0), //左下
	float4(-0.5f, +0.5f, 0, 0), //左上
	float4(+0.5f, -0.5f, 0, 0), //右下
	float4(+0.5f, +0.5f, 0, 0) //右下
};

[maxvertexcount(vnum)]
void main(
	point VSOutput input[1] : SV_POSITION,
	// 点ストリーム
	inout TriangleStream<GSOutput> output
)
{
	GSOutput element; // 出力用頂点データ
	// 4点分まわす
	for (uint i = 0; i < vnum; i++)
	{
        float4 offset = offset_array[i] * input[0].scale;
        offset = mul(matBillboard, offset);
        element.svpos = input[0].pos + offset;
		// ワールド座標ベースで、ずらす
		element.svpos = input[0].pos + offset_array[i]; // 頂点座標をコピー
		// ビュー、射影変換行列
		element.svpos = mul(mat, element.svpos);
		element.uv = uv_array[i];
		output.Append(element);
	}
}