#include "Object3d.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

/// <summary>
/// �ÓI�����o�ϐ��̎���
/// </summary>
const float Object3d::radius = 5.0f;				// ��ʂ̔��a
const float Object3d::prizmHeight = 8.0f;			// ���̍���
ID3D12Device* Object3d::device = nullptr;
UINT Object3d::descriptorHandleIncrementSize = 0;
ID3D12GraphicsCommandList* Object3d::cmdList = nullptr;
ComPtr<ID3D12RootSignature> Object3d::rootsignature;
ComPtr<ID3D12PipelineState> Object3d::pipelinestate;
ComPtr<ID3D12DescriptorHeap> Object3d::descHeap;
ComPtr<ID3D12Resource> Object3d::vertBuff;
//ComPtr<ID3D12Resource> Object3d::indexBuff;
ComPtr<ID3D12Resource> Object3d::texbuff;
CD3DX12_CPU_DESCRIPTOR_HANDLE Object3d::cpuDescHandleSRV;
CD3DX12_GPU_DESCRIPTOR_HANDLE Object3d::gpuDescHandleSRV;
XMMATRIX Object3d::matView{};
XMMATRIX Object3d::matProjection{};
//XMFLOAT3 Object3d::eye = { 0, 0, -50.0f };
XMFLOAT3 Object3d::eye = { 0, 0, -5.0f };
XMFLOAT3 Object3d::target = { 0, 0, 0 };
XMFLOAT3 Object3d::up = { 0, 1, 0 };
D3D12_VERTEX_BUFFER_VIEW Object3d::vbView{};
//D3D12_INDEX_BUFFER_VIEW Object3d::ibView{};
//Object3d::VertexPosNormalUv Object3d::vertices[vertexCount];
Object3d::VertexPos Object3d::vertices[vertexCount];
//unsigned short Object3d::indices[indexCount];

XMMATRIX Object3d::matBillboard = XMMatrixIdentity();
XMMATRIX Object3d::matBillboardY = XMMatrixIdentity();

void Object3d::StaticInitialize(ID3D12Device* device, int window_width, int window_height)
{
    // nullptr�`�F�b�N
    assert(device);

    Object3d::device = device;

    // �f�X�N���v�^�q�[�v�̏�����
    InitializeDescriptorHeap();

    // �J����������
    InitializeCamera(window_width, window_height);

    // �p�C�v���C��������
    InitializeGraphicsPipeline();

    // �e�N�X�`���ǂݍ���
    LoadTexture();

    // ���f������
    CreateModel();

}

void Object3d::PreDraw(ID3D12GraphicsCommandList* cmdList)
{
    // PreDraw��PostDraw���y�A�ŌĂ΂�Ă��Ȃ���΃G���[
    assert(Object3d::cmdList == nullptr);

    // �R�}���h���X�g���Z�b�g
    Object3d::cmdList = cmdList;

    // �p�C�v���C���X�e�[�g�̐ݒ�
    cmdList->SetPipelineState(pipelinestate.Get());
    // ���[�g�V�O�l�`���̐ݒ�
    cmdList->SetGraphicsRootSignature(rootsignature.Get());
    // �v���~�e�B�u�`���ݒ�
    //cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void Object3d::PostDraw()
{
    // �R�}���h���X�g������
    Object3d::cmdList = nullptr;
}

Object3d* Object3d::Create()
{
    // 3D�I�u�W�F�N�g�̃C���X�^���X�𐶐�
    Object3d* object3d = new Object3d();
    if (object3d == nullptr) {
        return nullptr;
    }

    // ������
    if (!object3d->Initialize()) {
        delete object3d;
        assert(0);
        return nullptr;
    }

    return object3d;
}

void Object3d::SetEye(XMFLOAT3 eye)
{
    Object3d::eye = eye;

    UpdateViewMatrix();
}

void Object3d::SetTarget(XMFLOAT3 target)
{
    Object3d::target = target;

    UpdateViewMatrix();
}

void Object3d::CameraMoveVector(XMFLOAT3 move)
{
    XMFLOAT3 eye_moved = GetEye();
    XMFLOAT3 target_moved = GetTarget();

    eye_moved.x += move.x;
    eye_moved.y += move.y;
    eye_moved.z += move.z;

    target_moved.x += move.x;
    target_moved.y += move.y;
    target_moved.z += move.z;

    SetEye(eye_moved);
    SetTarget(target_moved);
}

void Object3d::CameraMoveEyeVector(XMFLOAT3 move)
{
    XMFLOAT3 eye_moved = GetEye();

    eye_moved.x += move.x;
    eye_moved.y += move.y;
    eye_moved.z += move.z;

    SetEye(eye_moved);
}

void Object3d::InitializeDescriptorHeap()
{
    HRESULT result = S_FALSE;

    // �f�X�N���v�^�q�[�v�𐶐�	
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
    descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_���猩����悤��
    descHeapDesc.NumDescriptors = 1; // �V�F�[�_�[���\�[�X�r���[1��
    result = device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeap));//����
    if (FAILED(result)) {
        assert(0);
    }

    // �f�X�N���v�^�T�C�Y���擾
    descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}

void Object3d::InitializeCamera(int window_width, int window_height)
{
    // �r���[�s��̐���
    UpdateViewMatrix();

    // ���s���e�ɂ��ˉe�s��̐���
    //constMap->mat = XMMatrixOrthographicOffCenterLH(
    //	0, window_width,
    //	window_height, 0,
    //	0, 1);
    // �������e�ɂ��ˉe�s��̐���
    matProjection = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(60.0f),
        (float)window_width / window_height,
        0.1f, 1000.0f
    );
}

void Object3d::InitializeGraphicsPipeline()
{
    HRESULT result = S_FALSE;
    ComPtr<ID3DBlob> vsBlob; // ���_�V�F�[�_�I�u�W�F�N�g
    ComPtr<ID3DBlob> gsBlob; // �W�I���g���V�F�[�_�I�u�W�F�N�g
    ComPtr<ID3DBlob> psBlob;	// �s�N�Z���V�F�[�_�I�u�W�F�N�g
    ComPtr<ID3DBlob> errorBlob; // �G���[�I�u�W�F�N�g

    // ���_�V�F�[�_�̓ǂݍ��݂ƃR���p�C��
    result = D3DCompileFromFile(
        L"Resources/Shaders/BasicVertexShader.hlsl",	// �V�F�[�_�t�@�C����
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
        "main", "vs_5_0",	// �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
        0,
        &vsBlob, &errorBlob);
    if (FAILED(result)) {
        // errorBlob����G���[���e��string�^�ɃR�s�[
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // �G���[���e���o�̓E�B���h�E�ɕ\��
        OutputDebugStringA(errstr.c_str());
        exit(1);
    }

    // �W�I���g���V�F�[�_�̓ǂݍ��݂ƃR���p�C��
    result = D3DCompileFromFile(
        L"Resources/Shaders/BasicGeometryShader.hlsl",	// �V�F�[�_�t�@�C����
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
        "main", "gs_5_0",	// �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
        0,
        &gsBlob, &errorBlob);
    if (FAILED(result)) {
        // errorBlob����G���[���e��string�^�ɃR�s�[
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // �G���[���e���o�̓E�B���h�E�ɕ\��
        OutputDebugStringA(errstr.c_str());
        exit(1);
    }

    // �s�N�Z���V�F�[�_�̓ǂݍ��݂ƃR���p�C��
    result = D3DCompileFromFile(
        L"Resources/Shaders/BasicPixelShader.hlsl",	// �V�F�[�_�t�@�C����
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
        "main", "ps_5_0",	// �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
        0,
        &psBlob, &errorBlob);
    if (FAILED(result)) {
        // errorBlob����G���[���e��string�^�ɃR�s�[
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // �G���[���e���o�̓E�B���h�E�ɕ\��
        OutputDebugStringA(errstr.c_str());
        exit(1);
    }

    // ���_���C�A�E�g
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { // xy���W(1�s�ŏ������ق������₷��)
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        //{ // �@���x�N�g��(1�s�ŏ������ق������₷��)
        //	"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
        //	D3D12_APPEND_ALIGNED_ELEMENT,
        //	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        //},
        //{ // uv���W(1�s�ŏ������ق������₷��)
        //	"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
        //	D3D12_APPEND_ALIGNED_ELEMENT,
        //	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        //},
    };

    // �O���t�B�b�N�X�p�C�v���C���̗����ݒ�
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};
    gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    gpipeline.GS = CD3DX12_SHADER_BYTECODE(gsBlob.Get());
    gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

    // �T���v���}�X�N
    gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // �W���ݒ�
    // ���X�^���C�U�X�e�[�g
    gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    //gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    //gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    // �f�v�X�X�e���V���X�e�[�g
    gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // �����_�[�^�[�Q�b�g�̃u�����h�ݒ�
    D3D12_RENDER_TARGET_BLEND_DESC blenddesc{};
    blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;	// RBGA�S�Ẵ`�����l����`��
    blenddesc.BlendEnable = true;
    blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
    blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

    blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;

    // �u�����h�X�e�[�g�̐ݒ�
    gpipeline.BlendState.RenderTarget[0] = blenddesc;
    gpipeline.BlendState.AlphaToCoverageEnable = false;	//����ON

    // �[�x�o�b�t�@�̃t�H�[�}�b�g
    gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    // ���_���C�A�E�g�̐ݒ�
    gpipeline.InputLayout.pInputElementDescs = inputLayout;
    gpipeline.InputLayout.NumElements = _countof(inputLayout);

    // �}�`�̌`��ݒ�i�O�p�`�j
    //gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

    gpipeline.NumRenderTargets = 1;	// �`��Ώۂ�1��
    gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0�`255�w���RGBA
    gpipeline.SampleDesc.Count = 1; // 1�s�N�Z���ɂ�1��T���v�����O

    // �f�X�N���v�^�����W
    CD3DX12_DESCRIPTOR_RANGE descRangeSRV;
    descRangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 ���W�X�^

    // ���[�g�p�����[�^
    CD3DX12_ROOT_PARAMETER rootparams[2];
    rootparams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootparams[1].InitAsDescriptorTable(1, &descRangeSRV, D3D12_SHADER_VISIBILITY_ALL);

    // �X�^�e�B�b�N�T���v���[
    CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

    // ���[�g�V�O�l�`���̐ݒ�
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSigBlob;
    // �o�[�W������������̃V���A���C�Y
    result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
    // ���[�g�V�O�l�`���̐���
    result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
    assert(SUCCEEDED(result));

    gpipeline.pRootSignature = rootsignature.Get();

    // �O���t�B�b�N�X�p�C�v���C���̐���
    result = device->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelinestate));
    assert(SUCCEEDED(result));

}

void Object3d::LoadTexture()
{
    HRESULT result = S_FALSE;

    TexMetadata metadata{};
    ScratchImage scratchImg{};

    // WIC�e�N�X�`���̃��[�h
    result = LoadFromWICFile(L"Resources/keroBozu.png", WIC_FLAGS_NONE, &metadata, scratchImg);
    assert(SUCCEEDED(result));

    ScratchImage mipChain{};
    // �~�b�v�}�b�v����
    result = GenerateMipMaps(
        scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(),
        TEX_FILTER_DEFAULT, 0, mipChain);
    if (SUCCEEDED(result)) {
        scratchImg = std::move(mipChain);
        metadata = scratchImg.GetMetadata();
    }

    // �ǂݍ��񂾃f�B�t���[�Y�e�N�X�`����SRGB�Ƃ��Ĉ���
    metadata.format = MakeSRGB(metadata.format);

    // ���\�[�X�ݒ�
    CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        metadata.format, metadata.width, (UINT)metadata.height, (UINT16)metadata.arraySize,
        (UINT16)metadata.mipLevels);

    // �q�[�v�v���p�e�B
    CD3DX12_HEAP_PROPERTIES heapProps =
        CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

    // �e�N�X�`���p�o�b�t�@�̐���
    result = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texresDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, // �e�N�X�`���p�w��
        nullptr, IID_PPV_ARGS(&texbuff));
    assert(SUCCEEDED(result));

    // �e�N�X�`���o�b�t�@�Ƀf�[�^�]��
    for (size_t i = 0; i < metadata.mipLevels; i++) {
        const Image* img = scratchImg.GetImage(i, 0, 0); // ���f�[�^���o
        result = texbuff->WriteToSubresource(
            (UINT)i,
            nullptr,              // �S�̈�փR�s�[
            img->pixels,          // ���f�[�^�A�h���X
            (UINT)img->rowPitch,  // 1���C���T�C�Y
            (UINT)img->slicePitch // 1���T�C�Y
        );
        assert(SUCCEEDED(result));
    }

    // �V�F�[�_���\�[�X�r���[�쐬
    cpuDescHandleSRV = CD3DX12_CPU_DESCRIPTOR_HANDLE(descHeap->GetCPUDescriptorHandleForHeapStart(), 0, descriptorHandleIncrementSize);
    gpuDescHandleSRV = CD3DX12_GPU_DESCRIPTOR_HANDLE(descHeap->GetGPUDescriptorHandleForHeapStart(), 0, descriptorHandleIncrementSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // �ݒ�\����
    D3D12_RESOURCE_DESC resDesc = texbuff->GetDesc();

    srvDesc.Format = resDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(texbuff.Get(), //�r���[�Ɗ֘A�t����o�b�t�@
        &srvDesc, //�e�N�X�`���ݒ���
        cpuDescHandleSRV
    );

}

void Object3d::CreateModel()
{
    HRESULT result = S_FALSE;

    //�l�p�`�̒��_�f�[�^
    //VertexPosNormalUv verticesSquare[] = {
    //	{{-5.0f,-5.0f,0.0f},{0,0,1},{0,1}},//����
    //	{{-5.0f,+5.0f,0.0f},{0,0,1},{0,0}},//����
    //	{{+5.0f,-5.0f,0.0f},{0,0,1},{1,1}},//�E��
    //	{{+5.0f,+5.0f,0.0f},{0,0,1},{1,0}},//�E��
    //};
    //VertexPosNormalUv verticesPoint[] = {
    //	{{0.0f,0.0f,0.0f},{0,0,1},{0,1}}
    //};
    VertexPos verticesPoint[] = {
        {{0.0f,0.0f,0.0f}}
    };


    //�����o�ϐ��ɃR�s�[
    //std::copy(std::begin(verticesSquare), std::end(verticesSquare), vertices);
    std::copy(std::begin(verticesPoint), std::end(verticesPoint), vertices);

    //�l�p�`�̃C���f�b�N�X�f�[�^
    unsigned short indicesSquare[] = {
        0,1,2,//�O�p�`1
        2,1,3,//�O�p�`2
    };

    //�����o�ϐ��ɃR�s�[
    //std::copy(std::begin(indicesSquare), std::end(indicesSquare), indices);

    UINT sizeVB = static_cast<UINT>(sizeof(vertices));

    // �q�[�v�v���p�e�B
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    // ���\�[�X�ݒ�
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeVB);

    // ���_�o�b�t�@����
    result = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&vertBuff));
    assert(SUCCEEDED(result));

    // ���_�o�b�t�@�ւ̃f�[�^�]��
    //VertexPosNormalUv* vertMap = nullptr;
    VertexPos* vertMap = nullptr;
    result = vertBuff->Map(0, nullptr, (void**)&vertMap);
    if (SUCCEEDED(result)) {
        memcpy(vertMap, vertices, sizeof(vertices));
        vertBuff->Unmap(0, nullptr);
    }

    // ���_�o�b�t�@�r���[�̍쐬
    vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
    vbView.SizeInBytes = sizeof(vertices);
    vbView.StrideInBytes = sizeof(vertices[0]);

    //UINT sizeIB = static_cast<UINT>(sizeof(indices));
    // ���\�[�X�ݒ�
    //resourceDesc.Width = sizeIB;

    // �C���f�b�N�X�o�b�t�@����
    //result = device->CreateCommittedResource(
    //	&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
    //	IID_PPV_ARGS(&indexBuff));

    // �C���f�b�N�X�o�b�t�@�ւ̃f�[�^�]��
    unsigned short* indexMap = nullptr;
    //result = indexBuff->Map(0, nullptr, (void**)&indexMap);
    if (SUCCEEDED(result)) {

        // �S�C���f�b�N�X�ɑ΂���
        //for (int i = 0; i < _countof(indices); i++)
        //{
        //	indexMap[i] = indices[i];	// �C���f�b�N�X���R�s�[
        //}

        //indexBuff->Unmap(0, nullptr);
    }

    // �C���f�b�N�X�o�b�t�@�r���[�̍쐬
    //ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
    //ibView.Format = DXGI_FORMAT_R16_UINT;
    //ibView.SizeInBytes = sizeof(indices);
}

void Object3d::UpdateViewMatrix()
{
    // �r���[�s��̍X�V
    XMVECTOR eyePosition = XMLoadFloat3(&eye);			//���_���W
    XMVECTOR targetPosition = XMLoadFloat3(&target);	//�����_���W
    XMVECTOR upVector = XMLoadFloat3(&up);				//���̏����

    XMVECTOR cameraAxisZ = XMVectorSubtract(targetPosition, eyePosition);	//�����Z

    assert(!XMVector3Equal(cameraAxisZ, XMVectorZero()));	//0����Ȃ���
    assert(!XMVector3IsInfinite(cameraAxisZ));				//��������Ȃ���
    assert(!XMVector3Equal(upVector, XMVectorZero()));		//0����Ȃ���
    assert(!XMVector3IsInfinite(upVector));					//��������Ȃ���

    cameraAxisZ = XMVector3Normalize(cameraAxisZ);	//���K��

    XMVECTOR cameraAxisX;
    cameraAxisX = XMVector3Cross(upVector, cameraAxisZ);	//�O�ώg����X�����߂�

    cameraAxisX = XMVector3Normalize(cameraAxisX);	//���K��

    XMVECTOR cameraAxisY;
    cameraAxisY = XMVector3Cross(cameraAxisZ, cameraAxisX);	//�O�ώg����Y�����߂�

    XMMATRIX matCameraRot;	//�J������]�s��
    //�J�������W�n�����[���h���W�n�̕ϊ��s��
    matCameraRot.r[0] = cameraAxisX;
    matCameraRot.r[1] = cameraAxisY;
    matCameraRot.r[2] = cameraAxisZ;
    matCameraRot.r[3] = XMVectorSet(0, 0, 0, 1);

    //�t�s�񋁂߂�
    matView = XMMatrixTranspose(matCameraRot);

    //���_���W��-1�����������W
    XMVECTOR reverseEyePosition = XMVectorNegate(eyePosition);	//���]
    //�J�����̈ʒu���烏�[���h���_�ւ̃x�N�g��
    XMVECTOR tX = XMVector3Dot(cameraAxisX, reverseEyePosition);	//X����
    XMVECTOR tY = XMVector3Dot(cameraAxisY, reverseEyePosition);	//Y����
    XMVECTOR tZ = XMVector3Dot(cameraAxisZ, reverseEyePosition);	//Z����
    //1�̃x�N�g���ɂ܂Ƃ߂�
    XMVECTOR translation = XMVectorSet(tX.m128_f32[0], tY.m128_f32[1], tZ.m128_f32[2], 1.0f);

    //�r���[�s��ɕ��s�ړ�������ݒ�
    matView.r[3] = translation;

#pragma region �S�����r���{�[�h�s��̌v�Z
    //�r���{�[�h�s��
    matBillboard.r[0] = cameraAxisX;
    matBillboard.r[1] = cameraAxisY;
    matBillboard.r[2] = cameraAxisZ;
    matBillboard.r[3] = XMVectorSet(0, 0, 0, 1);
#pragma endregion
#pragma region �S�����r���{�[�h�s��̌v�Z
    //�J����X���AY���AZ��
    XMVECTOR ybillCameraAxisX, ybillCameraAxisY, ybillCameraAxisZ;

    //X���͋���
    ybillCameraAxisX = cameraAxisX;
    //Y���̓��[���h���W�n��Y��
    ybillCameraAxisY = XMVector3Normalize(upVector);
    //Z����X����Y���̊O�ςŋ��܂�
    ybillCameraAxisZ = XMVector3Cross(ybillCameraAxisX, ybillCameraAxisY);

    //Y�����r���{�[�h�s��
    matBillboardY.r[0] = ybillCameraAxisX;
    matBillboardY.r[1] = ybillCameraAxisY;
    matBillboardY.r[2] = ybillCameraAxisZ;
    matBillboardY.r[3] = XMVectorSet(0, 0, 0, 1);
#pragma endregion
}

bool Object3d::Initialize()
{
    // nullptr�`�F�b�N
    assert(device);

    // �q�[�v�v���p�e�B
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    // ���\�[�X�ݒ�
    CD3DX12_RESOURCE_DESC resourceDesc =
        CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff);

    HRESULT result;

    // �萔�o�b�t�@�̐���
    result = device->CreateCommittedResource(
        &heapProps, // �A�b�v���[�h�\
        D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&constBuff));
    assert(SUCCEEDED(result));

    return true;
}

void Object3d::Update()
{
    HRESULT result;
    XMMATRIX matScale, matRot, matTrans;

    // �X�P�[���A��]�A���s�ړ��s��̌v�Z
    matScale = XMMatrixScaling(scale.x, scale.y, scale.z);
    matRot = XMMatrixIdentity();
    matRot *= XMMatrixRotationZ(XMConvertToRadians(rotation.z));	//�r���{�[�h�̎��̓��[�J����]��Z���݂̂Ɍ���
    matTrans = XMMatrixTranslation(position.x, position.y, position.z);

    // ���[���h�s��̍���
    matWorld = XMMatrixIdentity(); // �ό`�����Z�b�g

    matWorld *= matScale;	//���[���h�s��ɃX�P�[�����O�𔽉f
    matWorld *= matRot;		//���[���h�s��ɉ�]�𔽉f

    if (isYBillboard)
    {
        //matWorld *= matBillboardY;	//Y���r���{�[�h�s���������
    }
    else
    {
        //matWorld *= matBillboard;	//�r���{�[�h�s���������
    }

    matWorld *= matTrans;	//���[���h�s��ɕ��s�ړ��𔽉f

    // �e�I�u�W�F�N�g�������
    if (parent != nullptr) {
        // �e�I�u�W�F�N�g�̃��[���h�s����|����
        matWorld *= parent->matWorld;
    }

    // �萔�o�b�t�@�փf�[�^�]��
    ConstBufferData* constMap = nullptr;
    result = constBuff->Map(0, nullptr, (void**)&constMap);
    //constMap->color = color;
    //constMap->mat = matWorld * matView * matProjection;	// �s��̍���
    constMap->mat = matView * matProjection;
    constBuff->Unmap(0, nullptr);
}

void Object3d::Draw()
{
    // nullptr�`�F�b�N
    assert(device);
    assert(Object3d::cmdList);

    // ���_�o�b�t�@�̐ݒ�
    cmdList->IASetVertexBuffers(0, 1, &vbView);
    // �C���f�b�N�X�o�b�t�@�̐ݒ�
    //cmdList->IASetIndexBuffer(&ibView);

    // �f�X�N���v�^�q�[�v�̔z��
    ID3D12DescriptorHeap* ppHeaps[] = { descHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // �萔�o�b�t�@�r���[���Z�b�g
    cmdList->SetGraphicsRootConstantBufferView(0, constBuff->GetGPUVirtualAddress());
    // �V�F�[�_���\�[�X�r���[���Z�b�g
    cmdList->SetGraphicsRootDescriptorTable(1, gpuDescHandleSRV);
    // �`��R�}���h
    //cmdList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);
    //cmdList->DrawIndexedInstanced(3, 1, 0, 0, 0);
    cmdList->DrawInstanced(_countof(vertices), 1, 0, 0);
}