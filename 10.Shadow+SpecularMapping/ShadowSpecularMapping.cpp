//**********************************************************************
//
// ShadowSpecularMapping.cpp
//
//**********************************************************************

#include "ShadowSpecularMapping.h"
#include <stdio.h>

// ��������
#define PI				3.14159265f
#define FOV				(PI/4.0f)
#define ASPECT_RATIO	(WIN_WIDTH/(float)WIN_HEIGHT)
#define NEAR_PLANE		1
#define FAR_PLANE		10000

// ȸ����
float					rotationY = 0.0f;

// D3D ����
LPDIRECT3D9             d3d = NULL;				// D3D
LPDIRECT3DDEVICE9       d3dDevice = NULL;				// D3D ��ġ
			
//��Ʈ
ID3DXFont*              font = NULL;

// ��
LPD3DXMESH				torus = NULL;
LPD3DXMESH				disc = NULL;

// ���̴�
LPD3DXEFFECT			applyShadowShader = NULL;
LPD3DXEFFECT			createShadowShader = NULL;
LPD3DXEFFECT			applyDisc = NULL;

// �ؽ�ó
LPDIRECT3DTEXTURE9		stoneDM = NULL;
LPDIRECT3DTEXTURE9		stoneSM = NULL;

// ���α׷� �̸�
const char*				appName = "�׸���+���� ����";

//ī�޶� ��ġ
D3DXVECTOR4				worldCameraPosition(200.0f, 200.0f,200.0f, 1.0f);

//���� ��ġ
D3DXVECTOR4				worldLightPosition(500.0f, 500.0f, -500.0f, 1.0f);

//���� ����
D3DXVECTOR4				lightColor(0.7f, 0.7f, 1.0f, 1.0f);

//��ü�� ����
D3DXVECTOR4				discColor(0.0f, 1.0f, 1.0f, 1.0f);

//�׸��ڸ� ���� Ÿ��
LPDIRECT3DTEXTURE9		shadowRenderTarget = NULL;
LPDIRECT3DSURFACE9		shadowDepthStencil = NULL; //�׸��� ���� �׸� �� ����� ���� ����

// ������
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	// ������ Ŭ������ ����Ѵ�.
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		appName, NULL };
	RegisterClassEx(&wc);

	// ���α׷� â�� �����Ѵ�.
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	HWND hWnd = CreateWindow(appName, appName,
		style, CW_USEDEFAULT, 0, WIN_WIDTH, WIN_HEIGHT,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	// Client Rect ũ�Ⱑ WIN_WIDTH, WIN_HEIGHT�� ������ ũ�⸦ �����Ѵ�.
	POINT ptDiff;
	RECT rcClient, rcWindow;

	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd, rcWindow.left, rcWindow.top, WIN_WIDTH + ptDiff.x, WIN_HEIGHT + ptDiff.y, TRUE);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	// D3D�� ����� ��� ���� �ʱ�ȭ�Ѵ�.
	if (!InitEverything(hWnd))
		PostQuitMessage(1);

	// �޽��� ����
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else // �޽����� ������ ������ ������Ʈ�ϰ� ����� �׸���
		{
			PlayDemo();
		}
	}

	UnregisterClass(appName, wc.hInstance);
	return 0;
}

// �޽��� ó����
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		ProcessInput(hWnd, wParam);
		break;

	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Ű���� �Է�ó��
void ProcessInput(HWND hWnd, WPARAM keyPress)
{
	switch (keyPress)
	{
	case 'W':
		worldCameraPosition.x -= 10.0f;
		break;
	case 'S':
		worldCameraPosition.x += 10.0f;
		break;
	case 'A':
		worldCameraPosition.z -= 10.0f;
		break;
	case 'D':
		worldCameraPosition.z += 10.0f;
		break;
		// ESC Ű�� ������ ���α׷��� �����Ѵ�.
	case VK_ESCAPE:
		PostMessage(hWnd, WM_DESTROY, 0L, 0L);
		break;
	}
}

//���� ����
void PlayDemo()
{
	Update();
	RenderFrame();
}

// ���ӷ��� ������Ʈ
void Update()
{
}

//������
void RenderFrame()
{
	D3DCOLOR bgColour = 0x000000FF;

	d3dDevice->Clear(0, NULL, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), bgColour, 1.0f, 0);

	d3dDevice->BeginScene();
	{
		RenderScene();				// 3D ��ü���� �׸���.
		RenderInfo();				// ����� ���� ���� ����Ѵ�.
	}
	d3dDevice->EndScene();

	d3dDevice->Present(NULL, NULL, NULL, NULL);
}

// 3D ��ü���� �׸���.
void RenderScene()
{
#pragma region ����-�� ����� �����.
	D3DXMATRIXA16 lightViewMatrix;
	{
		D3DXVECTOR3		eye(worldLightPosition.x, worldLightPosition.y, worldLightPosition.z);
		D3DXVECTOR3		at(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3		up(0.0f, 1.0f, 0.0f);
		D3DXMatrixLookAtLH(&lightViewMatrix, &eye, &at, &up);
	}
#pragma endregion

#pragma region ����-���� ����� �����.
	D3DXMATRIXA16 lightProjectionMatrix;
	{
		D3DXMatrixPerspectiveFovLH(&lightProjectionMatrix, D3DX_PI / 4.0f, 1, 1, 1000);
	}
#pragma endregion

#pragma region ��-���� ����� �����.
	D3DXMATRIXA16 viewProjectionMatrix;
	{
		//�� ����� �����.
		D3DXMATRIXA16 viewMatrix;
		D3DXVECTOR3		eye(worldCameraPosition.x, worldCameraPosition.y, worldCameraPosition.z);
		D3DXVECTOR3		at(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3		up(0.0f, 1.0f, 0.0f);
		D3DXMatrixLookAtLH(&viewMatrix, &eye, &at, &up);
		//���� ����� �����.
		D3DXMATRIXA16 projectionMatrix;
		D3DXMatrixPerspectiveFovLH(&projectionMatrix, FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE);
		//��-������� = �� ��� X ���� ���
		D3DXMatrixMultiply(&viewProjectionMatrix, &viewMatrix, &projectionMatrix);
	}
#pragma endregion

#pragma region torus�� ���� ����� �����.
	D3DXMATRIXA16 torusWorldMatrix;
	{
		rotationY += 0.4f*PI / 180.0f;
		if (rotationY > 2 * PI)
		{
			rotationY -= 2 * PI;
		}
		D3DXMatrixRotationY(&torusWorldMatrix, rotationY);
	}
#pragma endregion

#pragma region disc�� ���� ����� �����.
	D3DXMATRIXA16 discWorldMatrix;
	{
		D3DXMATRIXA16 scaleMatrix;
		D3DXMatrixScaling(&scaleMatrix, 2.5f, 2.5f, 2.5f);

		D3DXMATRIXA16 translationMatrix;
		D3DXMatrixTranslation(&translationMatrix, 0.0f, -40.0f, 0.0f);

		D3DXMatrixMultiply(&discWorldMatrix, &scaleMatrix, &translationMatrix);
	}
#pragma endregion

	//����ۿ� ���̹���
	LPDIRECT3DSURFACE9 backBuffer = NULL;
	LPDIRECT3DSURFACE9 depthStencilBuffer = NULL;

	d3dDevice->GetRenderTarget(0, &backBuffer);
	d3dDevice->GetDepthStencilSurface(&depthStencilBuffer);

#pragma region �׸��� �����
	LPDIRECT3DSURFACE9 shadowSurface = NULL;
	if (SUCCEEDED(shadowRenderTarget->GetSurfaceLevel(0, &shadowSurface)))
	{
		d3dDevice->SetRenderTarget(0, shadowSurface);
		shadowSurface->Release();
		shadowSurface = NULL;
	}
	d3dDevice->SetDepthStencilSurface(shadowDepthStencil);

	//���� �����ӿ� �׷ȴ� �׸��� ������ �����.
	d3dDevice->Clear(0, NULL, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), 0xFFFFFFFF, 1.0f, 0);

	//createShadowShader ���� �������� ����
	createShadowShader->SetMatrix("worldMatrix", &torusWorldMatrix);
	createShadowShader->SetMatrix("lightViewMatrix", &lightViewMatrix);
	createShadowShader->SetMatrix("lightProjectionMatrix", &lightProjectionMatrix);

	//createShadowShader ����
	{
		UINT numPasses = 0;
		createShadowShader->Begin(&numPasses, NULL);
		for (UINT i = 0; i < numPasses; i++)
		{
			createShadowShader->BeginPass(i);
			torus->DrawSubset(0);
			createShadowShader->EndPass();
		}
		createShadowShader->End();
	}
#pragma endregion
	
	//�ϵ���� ��/���� ���۸� ���
	d3dDevice->SetRenderTarget(0, backBuffer);
	d3dDevice->SetDepthStencilSurface(depthStencilBuffer);

	backBuffer->Release();
	backBuffer = NULL;
	depthStencilBuffer->Release();
	depthStencilBuffer = NULL;

#pragma region �׸��� ������
	//applyShadowShader ���� �������� ����
	applyShadowShader->SetVector("lightColor", &lightColor);
	applyShadowShader->SetVector("worldLightPosition", &worldLightPosition);
	applyShadowShader->SetVector("worldCameraPosition", &worldCameraPosition);
	applyShadowShader->SetMatrix("worldMatrix", &torusWorldMatrix);

	applyShadowShader->SetMatrix("lightViewMatrix", &lightViewMatrix);
	applyShadowShader->SetMatrix("lightProjectionMatrix", &lightProjectionMatrix);
	applyShadowShader->SetMatrix("viewProjectionMatrix", &viewProjectionMatrix);
	
	applyShadowShader->SetTexture("diffuseMap_Tex", stoneDM);
	applyShadowShader->SetTexture("specularMap_Tex", stoneSM);
	applyShadowShader->SetTexture("ShadowMap_Tex", shadowRenderTarget);
	//applyShadowShader ����
	{
		UINT numPasses = 0;
		applyShadowShader->Begin(&numPasses, NULL);
		for (UINT i = 0; i < numPasses; i++)
		{
			applyShadowShader->BeginPass(i);
			torus->DrawSubset(0);
			applyShadowShader->EndPass();
		}
		applyShadowShader->End();
	}
#pragma endregion

#pragma region ��ũ �׸���
	//applyDisc ���� �������� ����
	applyDisc->SetVector("worldLightPosition", &worldLightPosition);
	applyDisc->SetMatrix("worldMatrix", &discWorldMatrix);
	applyDisc->SetMatrix("lightViewMatrix", &lightViewMatrix);
	applyDisc->SetMatrix("lightProjectionMatrix", &lightProjectionMatrix);
	applyDisc->SetMatrix("viewProjectionMatrix", &viewProjectionMatrix);
	applyDisc->SetTexture("ShadowMap_Tex", shadowRenderTarget);
	applyDisc->SetVector("objectColor", &discColor);

	//applyDisc ����
	{
		UINT numPasses = 0;
		applyDisc->Begin(&numPasses, NULL);
		for (UINT i = 0; i < numPasses; i++)
		{
			applyDisc->BeginPass(i);
			disc->DrawSubset(0);
			applyDisc->EndPass();
		}
		applyDisc->End();
	}
#pragma endregion
}

// ����� ���� ���� ���.
void RenderInfo()
{
	// �ؽ�Ʈ ����
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);

	// �ؽ�Ʈ�� ����� ��ġ
	RECT rct;
	rct.left = 5;
	rct.right = WIN_WIDTH;
	rct.top = 5;
	rct.bottom = WIN_HEIGHT / 3;

	//����� �ؽ�Ʈ
	char string[100];
	sprintf(string, "ESC: ��������\n\n");

	// Ű �Է� ������ ���
	font->DrawText(NULL, string, -1, &rct, 0, fontColor);
}

//�ʱ�ȭ �ڵ�
bool InitEverything(HWND hWnd)
{
	// D3D�� �ʱ�ȭ
	if (!InitD3D(hWnd))
	{
		return false;
	}

	//�׸��ڸ��� ũ�� ����
	const int shadowMapSize = 2048;

	//���� Ÿ�� ����
	if (FAILED(d3dDevice->CreateTexture(shadowMapSize, shadowMapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &shadowRenderTarget, NULL)))
	{
		return false;
	}

	//�׸��ڸʰ� ������ ũ���� ���� ���۸� �����.
	if (FAILED(d3dDevice->CreateDepthStencilSurface(shadowMapSize, shadowMapSize, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, true, &shadowDepthStencil, NULL)))
	{
		return false;
	}

	// ��, ���̴�, �ؽ�ó���� �ε�
	if (!LoadAssets())
	{
		return false;
	}

	// ��Ʈ�� �ε�
	if (FAILED(D3DXCreateFont(d3dDevice, 20, 10, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE),
		"Arial", &font)))
	{
		return false;
	}

	return true;
}

// D3D ��ü �� ��ġ �ʱ�ȭ
bool InitD3D(HWND hWnd)
{
	// D3D ��ü
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d)
	{
		return false;
	}

	// D3D��ġ�� �����ϴµ� �ʿ��� ����ü�� ä���ִ´�.
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferWidth = WIN_WIDTH;
	d3dpp.BackBufferHeight = WIN_HEIGHT;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality = 0;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.Windowed = TRUE;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
	d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	d3dpp.FullScreen_RefreshRateInHz = 0;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	// D3D��ġ�� �����Ѵ�.
	if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,D3DCREATE_HARDWARE_VERTEXPROCESSING,&d3dpp, &d3dDevice)))
	{
		return false;
	}

	return true;
}

bool LoadAssets()
{
	// �ؽ�ó �ε�
	stoneDM = LoadTexture("Fieldstone_DM.tga");
	if (!stoneDM)
	{
		return false;
	}

	stoneSM = LoadTexture("Fieldstone_SM.tga");
	if (!stoneSM)
	{
		return false;
	}
	// ���̴� �ε�
	applyShadowShader = LoadShader("ApplyShadow.fx");
	if (!applyShadowShader)
	{
		return false;
	}
	createShadowShader = LoadShader("CreateShadow.fx");
	if (!applyShadowShader)
	{
		return false;
	}
	applyDisc = LoadShader("ApplyDisc.fx");
	if (!applyDisc)
	{
		return false;
	}
	// �� �ε�
	torus = LoadModel("Torus.x");
	if (!torus)
	{
		return false;
	}
	disc = LoadModel("Disc.x");
	if (!disc)
	{
		return false;
	}

	return true;
}

// ���̴� �ε�
LPD3DXEFFECT LoadShader(const char * filename)
{
	LPD3DXEFFECT ret = NULL;

	LPD3DXBUFFER pError = NULL;
	DWORD dwShaderFlags = 0;

#if _DEBUG
	dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

	D3DXCreateEffectFromFile(d3dDevice, filename,
		NULL, NULL, dwShaderFlags, NULL, &ret, &pError);

	// ���̴� �ε��� ������ ��� outputâ�� ���̴�
	// ������ ������ ����Ѵ�.
	if (!ret && pError)
	{
		int size = pError->GetBufferSize();
		void *ack = pError->GetBufferPointer();

		if (ack)
		{
			char* str = new char[size];
			sprintf(str, (const char*)ack, size);
			OutputDebugString(str);
			delete[] str;
		}
	}

	return ret;
}

// �� �ε�
LPD3DXMESH LoadModel(const char * filename)
{
	LPD3DXMESH ret = NULL;
	if (FAILED(D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM, d3dDevice, NULL, NULL, NULL, NULL, &ret)))
	{
		OutputDebugString("�� �ε� ����: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	};

	return ret;
}

// �ؽ�ó �ε�
LPDIRECT3DTEXTURE9 LoadTexture(const char * filename)
{
	LPDIRECT3DTEXTURE9 ret = NULL;
	if (FAILED(D3DXCreateTextureFromFile(d3dDevice, filename, &ret)))
	{
		OutputDebugString("�ؽ�ó �ε� ����: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	}

	return ret;
}

//�޸� ���� �ڵ�
void Cleanup()
{
	// ��Ʈ�� release �Ѵ�.
	if (font)
	{
		font->Release();
		font = NULL;
	}

	// ���� release �Ѵ�.
	if (torus)
	{
		torus->Release();
		torus = NULL;
	}
	if (disc)
	{
		disc->Release();
		disc = NULL;
	}

	// ���̴��� release �Ѵ�.
	if (applyShadowShader)
	{
		applyShadowShader->Release();
		applyShadowShader = NULL;
	}
	if (createShadowShader)
	{
		createShadowShader->Release();
		createShadowShader = NULL;
	}
	if (applyDisc)
	{
		applyDisc->Release();
		applyDisc = NULL;
	}

	// �ؽ�ó�� release �Ѵ�.
	if (shadowRenderTarget)
	{
		shadowRenderTarget->Release();
		shadowRenderTarget = NULL;
	}
	if (shadowDepthStencil)
	{
		shadowDepthStencil->Release();
		shadowDepthStencil = NULL;
	}
	if (stoneDM)
	{
		stoneDM->Release();
		stoneDM = NULL;
	}
	if (stoneSM)
	{
		stoneSM->Release();
		stoneSM = NULL;
	}

	// D3D�� release �Ѵ�.
	if (d3dDevice)
	{
		d3dDevice->Release();
		d3dDevice = NULL;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = NULL;
	}
}

