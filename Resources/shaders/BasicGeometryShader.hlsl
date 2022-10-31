#include "BasicShaderHeader.hlsli"

// 三角形の入力から、点を3つ出力するサンプル
[maxvertexcount(3)]
void main(
	point VSOutput input[1] : SV_POSITION,
	// 点ストリーム
	inout TriangleStream<GSOutput> output
)
{
	GSOutput element; // 出力用頂点データ
	// 共通
	element.normal = input[0].normal; // 法線をコピー
	element.uv = input[0].uv; //UVをコピー
	
	// 1点目
	element.svpos = input[0].svpos; // 頂点座標をコピー
	// 頂点を1つ出力（出力リストに追加)
	output.Append(element);
	
	// 2点目
	element.svpos = input[0].svpos + float4(10.0f,10.0f,0,0); // 頂点座標をコピー
	// 頂点を1つ出力（出力リストに追加)
	output.Append(element);
	
	// 3点目
	element.svpos = input[0].svpos + float4(10.0f, 0, 0, 0); // 頂点座標をコピー
	// 頂点を1つ出力（出力リストに追加)
	output.Append(element);
}