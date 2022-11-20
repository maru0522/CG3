#include "GameScene.h"
#include <cassert>
#include <time.h>

using namespace DirectX;

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
    delete spriteBG;
    delete particleMan;
    delete sprite1;
    delete sprite2;
}

void GameScene::Initialize(DirectXCommon* dxCommon, Input* input)
{
    // nullptrチェック
    assert(dxCommon);
    assert(input);

    this->dxCommon = dxCommon;
    this->input = input;

    // デバッグテキスト用テクスチャ読み込み
    Sprite::LoadTexture(debugTextTexNumber, L"Resources/debugfont.png");
    // デバッグテキスト初期化
    debugText.Initialize(debugTextTexNumber);

    // テクスチャ読み込み
    Sprite::LoadTexture(1, L"Resources/background.png");

    // 背景スプライト生成
    spriteBG = Sprite::Create(1, { 0.0f,0.0f });

    //ランダム数生成用
    srand(time(nullptr));

    // 3Dオブジェクト生成
    particleMan = ParticleManager::Create();

    //出現範囲-20~20のランダムで決める
    particleMan->Update();

    //前景スプライト生成
    //テクスチャ2番に読み込み
    Sprite::LoadTexture(2, L"Resources/texture.png");

    //座標{0,0}に、テクスチャ2番のスプライトを生成
    sprite1 = Sprite::Create(2, { 0,0 });
    sprite2 = Sprite::Create(2, { 500,500 }, { 1,0,0,1 }, { 0, 0 }, false, true);
}

void GameScene::Update()
{

    //SPACEキーでビルボードの種類切り替え
    if (input->TriggerKey(DIK_SPACE))
    {
        if (particleMan->GetBillboard())
        {
            particleMan->SetBillboard(false);
        }
        else
        {
            particleMan->SetBillboard(true);
        }
    }

    // カメラ移動
    if (input->PushKey(DIK_W) || input->PushKey(DIK_S) || input->PushKey(DIK_D) || input->PushKey(DIK_A))
    {
        if (input->PushKey(DIK_W)) { ParticleManager::CameraMoveEyeVector({ 0.0f,+1.0f,0.0f }); }
        else if (input->PushKey(DIK_S)) { ParticleManager::CameraMoveEyeVector({ 0.0f,-1.0f,0.0f }); }
        if (input->PushKey(DIK_D)) { ParticleManager::CameraMoveEyeVector({ +1.0f,0.0f,0.0f }); }
        else if (input->PushKey(DIK_A)) { ParticleManager::CameraMoveEyeVector({ -1.0f,0.0f,0.0f }); }
    }


    particleMan->Update();


    //スプライト移動
    if (input->PushKey(DIK_SPACE))
    {
        //現在の座標取得
        XMFLOAT2 position = sprite1->GetPosition();
        //移動後の座標を計算
        position.x += 1.0f;
        //座標の変更を反映
        sprite1->SetPosition(position);
    }
}

void GameScene::Draw()
{
    // コマンドリストの取得
    ID3D12GraphicsCommandList* cmdList = dxCommon->GetCommandList();

#pragma region 背景スプライト描画
    // 背景スプライト描画前処理
    Sprite::PreDraw(cmdList);
    // 背景スプライト描画
    spriteBG->Draw();

    /// <summary>
    /// ここに背景スプライトの描画処理を追加できる
    /// </summary>

    // スプライト描画後処理
    Sprite::PostDraw();
    // 深度バッファクリア
    dxCommon->ClearDepthBuffer();
#pragma endregion

#pragma region 3Dオブジェクト描画
    // 3Dオブジェクト描画前処理
    ParticleManager::PreDraw(cmdList);

    // 3Dオブクジェクトの描画
    particleMan->Draw();

    /// <summary>
    /// ここに3Dオブジェクトの描画処理を追加できる
    /// </summary>

    // 3Dオブジェクト描画後処理
    ParticleManager::PostDraw();
#pragma endregion

#pragma region 前景スプライト描画
    // 前景スプライト描画前処理
    Sprite::PreDraw(cmdList);

    /// <summary>
    /// ここに前景スプライトの描画処理を追加できる
    /// </summary>
    //sprite1->Draw();
    //sprite2->Draw();
    //debugText.Print("SPACE Billboard Change", 0, 0, 3);

    // デバッグテキストの描画
    debugText.DrawAll(cmdList);

    // スプライト描画後処理
    Sprite::PostDraw();
#pragma endregion
}