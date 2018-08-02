//**********************************************************************
//
// ShaderFramework.cpp
// 
//
//**********************************************************************

#include "ShaderFramework.h"
#include <stdio.h>

#define PI           3.14159265f
#define FOV          (PI/4.0f)							// 시야각
#define ASPECT_RATIO (WIN_WIDTH/(float)WIN_HEIGHT)		// 화면의 종횡비
#define NEAR_PLANE   1									// 근접 평면
#define FAR_PLANE    10000								// 원거리 평면


//----------------------------------------------------------------------
// 전역변수
//----------------------------------------------------------------------

// D3D 관련
LPDIRECT3D9             gpD3D = NULL;				// D3D
LPDIRECT3DDEVICE9       gpD3DDevice = NULL;			// D3D 장치												
ID3DXFont*              gpFont = NULL;				//폰트		

// 모델
//LPD3DXMESH				gpSphere = NULL;
LPD3DXMESH				gpTeapot = NULL;
//LPD3DXMESH					gpTorus = NULL;
//LPD3DXMESH					gpDisc = NULL;

// 쉐이더
//LPD3DXEFFECT			gpTextureMappingShader = NULL;
//LPD3DXEFFECT			gpLightingShader = NULL;
//LPD3DXEFFECT			gpSpecularMappingShader = NULL;
//LPD3DXEFFECT			gpToonShader = NULL;
//LPD3DXEFFECT			gpNormalMappingShader = NULL;
LPD3DXEFFECT			gpEnvironmentMappingShader = NULL;
//LPD3DXEFFECT			gpUVAnimationShader = NULL;
//LPD3DXEFFECT			gpApplyShadowShader = NULL;
//LPD3DXEFFECT			gpCreateShadowShader = NULL;
LPD3DXEFFECT			gpNoEffect = NULL;
LPD3DXEFFECT			gpGrayScale = NULL;
LPD3DXEFFECT			gpSepia = NULL;

// 텍스처
//LPDIRECT3DTEXTURE9		gpEarthDM = NULL;
LPDIRECT3DTEXTURE9		gpStoneDM = NULL;
LPDIRECT3DTEXTURE9		gpStoneSM = NULL;
LPDIRECT3DTEXTURE9		gpStoneNM = NULL;
LPDIRECT3DCUBETEXTURE9	gpSnowENV = NULL;

// 프로그램 이름
const char*				gAppName = "데모 프레임워크";

// 회전값
float					gRotationY = 0.0f;

//빛의 위치
D3DXVECTOR4				gWorldLightPosition(500.0f, 500.0f, -500.0f, 1.0f);

//카메라 위치
D3DXVECTOR4				gWorldCameraPosition(0.0f, 0.0f, -200.0f, 1.0f);

//빛의 색상
D3DXVECTOR4				gLightColor(0.7f, 0.7f, 1.0f, 1.0f);

//표면의 색상
//D3DXVECTOR4				gSurfaceColor = D3DXVECTOR4(1.0f, 1.0f, 0.0f, 1.0f); //노란색 

//물체의 색상
D3DXVECTOR4				gTorusColor(1, 1, 0, 1);
D3DXVECTOR4				gDiscColor(0, 1, 1, 1);

//그림자맵 렌더 타깃
LPDIRECT3DTEXTURE9		gpShadowRenderTarget = NULL;
LPDIRECT3DSURFACE9		gpShadowDepthStencil = NULL;

//화면을 가득 채우는 사각형
LPDIRECT3DVERTEXDECLARATION9	gpFullscreenQuadDecl = NULL;
LPDIRECT3DVERTEXBUFFER9			gpFullscreenQuadVB = NULL;
LPDIRECT3DINDEXBUFFER9			gpFullscreenQuadIB = NULL;

// 장면 렌더타깃
LPDIRECT3DTEXTURE9		gpSceneRenderTarget = NULL;

// 사용할 포스트프로세스 쉐이더의 색인
int gPostProcessIndex = 0;

//-----------------------------------------------------------------------
// 프로그램 진입점/메시지 루프
//-----------------------------------------------------------------------

// 진입점
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	// 윈도우 클래스를 등록한다.
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		gAppName, NULL };
	RegisterClassEx(&wc);

	// 프로그램 창을 생성한다.
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	HWND hWnd = CreateWindow(gAppName, gAppName,
		style, CW_USEDEFAULT, 0, WIN_WIDTH, WIN_HEIGHT,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	// Client Rect 크기가 WIN_WIDTH, WIN_HEIGHT와 같도록 크기를 조정한다.
	POINT ptDiff;
	RECT rcClient, rcWindow;

	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd, rcWindow.left, rcWindow.top, WIN_WIDTH + ptDiff.x, WIN_HEIGHT + ptDiff.y, TRUE);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	// D3D를 비롯한 모든 것을 초기화한다.
	if (!InitEverything(hWnd))
		PostQuitMessage(1);

	// 메시지 루프
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else // 메시지가 없으면 게임을 업데이트하고 장면을 그린다
		{
			PlayDemo();
		}
	}

	UnregisterClass(gAppName, wc.hInstance);
	return 0;
}

// 메시지 처리기
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

// 키보드 입력처리
void ProcessInput(HWND hWnd, WPARAM keyPress)
{
	switch (keyPress)
	{
		// ESC 키가 눌리면 프로그램을 종료한다.
	case VK_ESCAPE:
		PostMessage(hWnd, WM_DESTROY, 0L, 0L);
		break;
	}
}

//게임 루프
void PlayDemo()
{
	Update();
	RenderFrame();
}

// 게임로직 업데이트
void Update()
{
}

//렌더링
void RenderFrame()
{
	D3DCOLOR bgColour = 0xFF0000FF;	// 배경색상 - 파랑

	gpD3DDevice->Clear(0, NULL, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), bgColour, 1.0f, 0);

	gpD3DDevice->BeginScene();
	{
		RenderScene();				// 3D 물체등을 그린다.
		RenderInfo();				// 디버그 정보 등을 출력한다.
	}
	gpD3DDevice->EndScene();

	gpD3DDevice->Present(NULL, NULL, NULL, NULL);
}

// 3D 물체등을 그린다.
void RenderScene()
{
#pragma region 각종 변환 행렬
	// 광원-뷰 행렬을 만든다.
	D3DXMATRIXA16 matLightView;
	{
		D3DXVECTOR3 vEyePt(gWorldLightPosition.x, gWorldLightPosition.y, gWorldLightPosition.z);
		D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
		D3DXMatrixLookAtLH(&matLightView, &vEyePt, &vLookatPt, &vUpVec);
	}

	// 광원-투영 행렬을 만든다.
	D3DXMATRIXA16 matLightProjection;
	{
		D3DXMatrixPerspectiveFovLH(&matLightProjection, D3DX_PI / 4.0f, 1, 1, 3000);
	}

	// 뷰/투영행렬을 만든다.
	D3DXMATRIXA16 matViewProjection;
	{
		// 뷰 행렬을 만든다.
		D3DXMATRIXA16 matView;
		D3DXVECTOR3 vEyePt(gWorldCameraPosition.x, gWorldCameraPosition.y, gWorldCameraPosition.z);
		D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
		D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);

		// 투영행렬을 만든다.
		D3DXMATRIXA16			matProjection;
		D3DXMatrixPerspectiveFovLH(&matProjection, FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE);

		D3DXMatrixMultiply(&matViewProjection, &matView, &matProjection);
	}

	// torus의 월드행렬을 만든다.
	D3DXMATRIXA16			matTorusWorld;
	{
		// 프레임마다 0.4도씩 회전을 시킨다.
		gRotationY += 0.4f * PI / 180.0f;
		if (gRotationY > 2 * PI)
		{
			gRotationY -= 2 * PI;
		}

		D3DXMatrixRotationY(&matTorusWorld, gRotationY);
	}

	// 디스크의 월드행렬을 만든다.
	D3DXMATRIXA16			matDiscWorld;
	{
		D3DXMATRIXA16 matScale;
		D3DXMatrixScaling(&matScale, 2, 2, 2);

		D3DXMATRIXA16 matTrans;
		D3DXMatrixTranslation(&matTrans, 0, -40, 0);

		D3DXMatrixMultiply(&matDiscWorld, &matScale, &matTrans);
	}

	// 현재 하드웨어 벡버퍼와 깊이버퍼
	//LPDIRECT3DSURFACE9 pHWBackBuffer = NULL;//벡버퍼
	//LPDIRECT3DSURFACE9 pHWDepthStencilBuffer = NULL;//깊이버퍼
	//gpD3DDevice->GetRenderTarget(0, &pHWBackBuffer);
	//gpD3DDevice->GetDepthStencilSurface(&pHWDepthStencilBuffer);
#pragma endregion
#pragma region 그림자 텍스처
	//// 1. 그림자 만들기
	//// 그림자 맵의 렌더타깃과 깊이버퍼를 사용한다.
	//LPDIRECT3DSURFACE9 pShadowSurface = NULL;
	//if (SUCCEEDED(gpShadowRenderTarget->GetSurfaceLevel(0, &pShadowSurface)))
	//{
	//	gpD3DDevice->SetRenderTarget(0, pShadowSurface);
	//	pShadowSurface->Release();
	//	pShadowSurface = NULL;
	//}
	//gpD3DDevice->SetDepthStencilSurface(gpShadowDepthStencil);

	//// 저번 프레임에 그렸던 그림자 정보를 흰색으로 지움
	//gpD3DDevice->Clear(0, NULL, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), 0xFFFFFFFF, 1.0f, 0);

	//// 그림자 만들기 쉐이더 전역변수들을 설정
	//gpEnvironmentMappingShader->SetMatrix("gWorldMatrix", &matTorusWorld);
	//gpEnvironmentMappingShader->SetMatrix("gLightViewMatrix", &matLightView);
	//gpEnvironmentMappingShader->SetMatrix("gLightProjectionMatrix", &matLightProjection);

	//// 그림자 만들기 쉐이더를 시작
	//{
	//	UINT numPasses = 0;
	//	gpEnvironmentMappingShader->Begin(&numPasses, NULL);
	//	{
	//		for (UINT i = 0; i < numPasses; ++i)
	//		{
	//			gpEnvironmentMappingShader->BeginPass(i);
	//			{
	//				// 원환체를 그린다.
	//				/*gpTorus->DrawSubset(0);*/

	//				gpTeapot->DrawSubset(0);
	//			}
	//			gpEnvironmentMappingShader->EndPass();
	//		}
	//	}
	//	gpEnvironmentMappingShader->End();
	//}

	//// 2. 그림자 입히기
	//// 하드웨어 백버퍼/깊이버퍼를 사용한다.
	//gpD3DDevice->SetRenderTarget(0, pHWBackBuffer);
	//gpD3DDevice->SetDepthStencilSurface(pHWDepthStencilBuffer);

	////참조 카운터가 증가하여 GPU메모리 누수가 나는걸 방지하기 위한 코드
	//pHWBackBuffer->Release();
	//pHWBackBuffer = NULL;
	//pHWDepthStencilBuffer->Release();
	//pHWDepthStencilBuffer = NULL;

	//// 그림자 입히기 쉐이더 전역변수들을 설정
	//gpEnvironmentMappingShader->SetMatrix("gWorldMatrix", &matTorusWorld);	//원환체
	//gpEnvironmentMappingShader->SetMatrix("gViewProjectionMatrix", &matViewProjection);//월드/뷰/투영 행렬
	//gpEnvironmentMappingShader->SetMatrix("gLightViewMatrix", &matLightView);//광원-뷰 행렬
	//gpEnvironmentMappingShader->SetMatrix("gLightProjectionMatrix", &matLightProjection);//광원-투영 행렬

	////난반사광
	//gpApplyShadowShader->SetVector("gWorldLightPosition", &gWorldLightPosition);

	//gpApplyShadowShader->SetVector("gObjectColor", &gTorusColor);//물체의 색

	//gpApplyShadowShader->SetTexture("ShadowMap_Tex", gpShadowRenderTarget);//그림자 맵

	// 쉐이더를 시작한다.
	//UINT numPasses = 0;
	//gpApplyShadowShader->Begin(&numPasses, NULL);
	//{
	//	for (UINT i = 0; i < numPasses; ++i)
	//	{
	//		gpApplyShadowShader->BeginPass(i);
	//		{
	//			// 원환체를 그린다.
	//			gpTorus->DrawSubset(0);

	//			// 디스크를 그린다.
	//			gpApplyShadowShader->SetMatrix("gWorldMatrix", &matDiscWorld);
	//			gpApplyShadowShader->SetVector("gObjectColor", &gDiscColor);
	//			gpApplyShadowShader->CommitChanges(); //블록사이 변수값을 바꿔줄때 호출하는 함수
	//			gpDisc->DrawSubset(0);
	//		}
	//		gpApplyShadowShader->EndPass();
	//	}
	//}
	//gpApplyShadowShader->End();
#pragma endregion
	/////////////////////////
	// 1. 장면을 렌더타깃 안에 그린다
	/////////////////////////
	// 현재 하드웨어 벡버퍼
	LPDIRECT3DSURFACE9 pHWBackBuffer = NULL;
	gpD3DDevice->GetRenderTarget(0, &pHWBackBuffer);

	// 렌더타깃 위에 그린다.
	LPDIRECT3DSURFACE9 pSceneSurface = NULL;
	if (SUCCEEDED(gpSceneRenderTarget->GetSurfaceLevel(0, &pSceneSurface)))
	{
		gpD3DDevice->SetRenderTarget(0, pSceneSurface);
		pSceneSurface->Release();
		pSceneSurface = NULL;
	}

	// 저번 프레임에 그렸던 장면을 지운다
	gpD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);

	// 뷰 행렬을 만든다.
	D3DXMATRIXA16 matView;
	D3DXVECTOR3 vEyePt(gWorldCameraPosition.x, gWorldCameraPosition.y, gWorldCameraPosition.z);
	D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);

	// 투영행렬을 만든다.
	D3DXMATRIXA16			matProjection;
	D3DXMatrixPerspectiveFovLH(&matProjection, FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE);

	// 프레임마다 0.4도씩 회전을 시킨다.
	gRotationY += 0.4f * PI / 180.0f;
	if (gRotationY > 2 * PI)
	{
		gRotationY -= 2 * PI;
	}

	// 월드행렬을 만든다.
	D3DXMATRIXA16			matWorld;
	D3DXMatrixRotationY(&matWorld, gRotationY);

	// 월드/뷰/투영행렬을 미리 곱한다.
	D3DXMATRIXA16 matWorldView;
	D3DXMATRIXA16 matWorldViewProjection;
	D3DXMatrixMultiply(&matWorldView, &matWorld, &matView);
	D3DXMatrixMultiply(&matWorldViewProjection, &matWorldView, &matProjection);

	// 쉐이더 전역변수들을 설정
	gpEnvironmentMappingShader->SetMatrix("gWorldMatrix", &matWorld);
	gpEnvironmentMappingShader->SetMatrix("gWorldViewProjectionMatrix", &matWorldViewProjection);

	gpEnvironmentMappingShader->SetVector("gWorldLightPosition", &gWorldLightPosition);
	gpEnvironmentMappingShader->SetVector("gWorldCameraPosition", &gWorldCameraPosition);

	gpEnvironmentMappingShader->SetVector("gLightColor", &gLightColor);
	gpEnvironmentMappingShader->SetTexture("DiffuseMap_Tex", gpStoneDM);
	gpEnvironmentMappingShader->SetTexture("SpecularMap_Tex", gpStoneSM);
	gpEnvironmentMappingShader->SetTexture("NormalMap_Tex", gpStoneNM);
	gpEnvironmentMappingShader->SetTexture("EnvironmentMap_Tex", gpSnowENV);

	// 쉐이더를 시작한다.
	UINT numPasses = 0;
	gpEnvironmentMappingShader->Begin(&numPasses, NULL);
	{
		for (UINT i = 0; i < numPasses; ++i)
		{
			gpEnvironmentMappingShader->BeginPass(i);
			{
				// 구체를 그린다.
				gpTeapot->DrawSubset(0);
			}
			gpEnvironmentMappingShader->EndPass();
		}
	}
	gpEnvironmentMappingShader->End();


	/////////////////////////
	// 2. 포스트프로세싱을 적용한다.
	/////////////////////////
	// 하드웨어 백버퍼를 사용한다.
	gpD3DDevice->SetRenderTarget(0, pHWBackBuffer);
	pHWBackBuffer->Release();
	pHWBackBuffer = NULL;

	// 사용할 포스트프로세스 효과
	LPD3DXEFFECT effectToUse = gpNoEffect;
	if (gPostProcessIndex == 1)
	{
		effectToUse = gpGrayScale;
	}
	else if (gPostProcessIndex == 2)
	{
		effectToUse = gpSepia;
	}

	effectToUse->SetTexture("SceneTexture_Tex", gpSceneRenderTarget);
	effectToUse->Begin(&numPasses, NULL);
	{
		for (UINT i = 0; i < numPasses; ++i)
		{
			effectToUse->BeginPass(i);
			{
				// 화면가득 사각형을 그린다.
				gpD3DDevice->SetStreamSource(0, gpFullscreenQuadVB, 0, sizeof(float) * 5);
				gpD3DDevice->SetIndices(gpFullscreenQuadIB);
				gpD3DDevice->SetVertexDeclaration(gpFullscreenQuadDecl);
				gpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 6, 0, 2);
			}
			effectToUse->EndPass();
		}
	}
	effectToUse->End();





}

// 디버그 정보 등을 출력.
void RenderInfo()
{
	// 텍스트 색상
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);

	// 텍스트를 출력할 위치
	RECT rct;
	rct.left = 5;
	rct.right = WIN_WIDTH / 3;
	rct.top = 5;
	rct.bottom = WIN_HEIGHT / 3;

	// 키 입력 정보를 출력
	gpFont->DrawText(NULL, "데모 프레임워크\n\nESC: 데모종료", -1, &rct, 0, fontColor);
}

//초기화 코드
bool InitEverything(HWND hWnd)
{
	// D3D를 초기화
	if (!InitD3D(hWnd))
	{
		return false;
	}

	//렌더 타깃을 만든다
	//const int shadowMapSize = 2048;
	//if (FAILED(gpD3DDevice->CreateTexture(shadowMapSize, shadowMapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F,
	//	D3DPOOL_DEFAULT, &gpShadowRenderTarget, NULL)))
	//{
	//	return false;
	//}
	if (FAILED(gpD3DDevice->CreateTexture(WIN_WIDTH, WIN_HEIGHT,
		1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8,
		D3DPOOL_DEFAULT, &gpSceneRenderTarget, NULL)))
	{
		return false;
	}

	//그림자 맵과 동일한 크기의 깊이 버퍼를 만든다.
	//if (FAILED(gpD3DDevice->CreateDepthStencilSurface(shadowMapSize, shadowMapSize,
	//	D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, true, &gpShadowDepthStencil, NULL)))
	//{
	//	return false;
	//}

	// 모델, 쉐이더, 텍스처등을 로딩
	if (!LoadAssets())
	{
		return false;
	}

	// 폰트를 로딩
	if (FAILED(D3DXCreateFont(gpD3DDevice, 20, 10, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE),
		"Arial", &gpFont)))
	{
		return false;
	}



	return true;
}

// D3D 객체 및 장치 초기화
bool InitD3D(HWND hWnd)
{
	// D3D 객체
	gpD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!gpD3D)
	{
		return false;
	}

	// D3D장치를 생성하는데 필요한 구조체를 채워넣는다.
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

	// D3D장치를 생성한다.
	if (FAILED(gpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &gpD3DDevice)))
	{
		return false;
	}

	return true;
}

//에셋 로딩
bool LoadAssets()
{
	// 텍스처 로딩
	//gpEarthDM = LoadTexture("Earth.jpg");
	//if (!gpEarthDM)
	//{
	//	return false;
	//}
	gpStoneDM = LoadTexture("Fieldstone_DM.tga");
	if (!gpStoneDM)
	{
		return false;
	}
	gpStoneSM = LoadTexture("Fieldstone_SM.tga");
	if (!gpStoneSM)
	{
		return false;
	}
	gpStoneNM = LoadTexture("Fieldstone_NM.tga");
	if (!gpStoneNM)
	{
		return false;
	}
	D3DXCreateCubeTextureFromFile(gpD3DDevice, "Snow_ENV.dds", &gpSnowENV);
	if (!gpSnowENV)
	{
		return false;
	}

	// 쉐이더 로딩
	gpEnvironmentMappingShader = LoadShader("EnvironmentMapping.fx");
	if (!gpEnvironmentMappingShader)
	{
		return false;
	}

	gpNoEffect = LoadShader("NoEffect.fx");
	if (!gpNoEffect)
	{
		return false;
	}

	gpGrayScale = LoadShader("Grayscale.fx");
	if (!gpGrayScale)
	{
		return false;
	}

	gpSepia = LoadShader("Sepia.fx");
	if (!gpSepia)
	{
		return false;
	}

	//gpApplyShadowShader = LoadShader("ApplyShadow.fx");
	//if (!gpApplyShadowShader)
	//{
	//	return false;
	//}
	//gpCreateShadowShader = LoadShader("CreateShadow.fx");
	//if (!gpCreateShadowShader)
	//{
	//	return false;
	//}

	// 모델 로딩
	//gpTorus = LoadModel("torus.x");
	//if (!gpTorus)
	//{
	//	return false;
	//}
	//gpSphere= LoadModel("TeapotWithTangent.x");
	//if (!gpSphere)
	//{
	//	return false;
	//}
	//gpDisc = LoadModel("disc.x");
	//if (!gpDisc)
	//{
	//	return false;
	//}
	gpTeapot = LoadModel("TeapotWithTangent.x");
	if (!gpTeapot)
	{
		return false;
	}

	return true;
}


// 쉐이더 로딩
LPD3DXEFFECT LoadShader(const char * filename)
{
	LPD3DXEFFECT ret = NULL;

	LPD3DXBUFFER pError = NULL;
	DWORD dwShaderFlags = 0;

#if _DEBUG
	dwShaderFlags |= D3DXSHADER_DEBUG;
#endif
	D3DXCreateEffectFromFile(gpD3DDevice, filename, NULL, NULL, dwShaderFlags, NULL, &ret, &pError);

	// 쉐이더 로딩에 실패한 경우 output창에 쉐이더
	// 컴파일 에러를 출력한다.
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

// 모델 로딩
LPD3DXMESH LoadModel(const char * filename)
{
	LPD3DXMESH ret = NULL;
	if (FAILED(D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM, gpD3DDevice, NULL, NULL, NULL, NULL, &ret)))
	{
		OutputDebugString("모델 로딩 실패: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	};

	return ret;
}

// 텍스처 로딩
LPDIRECT3DTEXTURE9 LoadTexture(const char * filename)
{
	LPDIRECT3DTEXTURE9 ret = NULL;
	if (FAILED(D3DXCreateTextureFromFile(gpD3DDevice, filename, &ret)))
	{
		OutputDebugString("텍스처 로딩 실패: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	}

	return ret;
}

void InitFullScreenQuad()
{
	// 정점 선언을 만든다
	D3DVERTEXELEMENT9 vertex[3];
	int offset = 0;
	int i = 0;

	// 위치
	vertex[i].Stream = 0;
	vertex[i].Offset = offset;
	vertex[i].Type = D3DDECLTYPE_FLOAT3;
	vertex[i].Method = D3DDECLMETHOD_DEFAULT;
	vertex[i].Usage = D3DDECLUSAGE_POSITION;
	vertex[i].UsageIndex = 0;

	offset += sizeof(float) * 3;
	++i;

	// UV좌표 0
	vertex[i].Stream = 0;
	vertex[i].Offset = offset;
	vertex[i].Type = D3DDECLTYPE_FLOAT2;
	vertex[i].Method = D3DDECLMETHOD_DEFAULT;
	vertex[i].Usage = D3DDECLUSAGE_TEXCOORD;
	vertex[i].UsageIndex = 0;

	offset += sizeof(float) * 2;
	++i;

	// 정점포맷의 끝임을 표현 (D3DDECL_END())
	vertex[i].Stream = 0xFF;
	vertex[i].Offset = 0;
	vertex[i].Type = D3DDECLTYPE_UNUSED;
	vertex[i].Method = 0;
	vertex[i].Usage = 0;
	vertex[i].UsageIndex = 0;

	gpD3DDevice->CreateVertexDeclaration(vertex, &gpFullscreenQuadDecl);

	// 정점버퍼를 만든다.
	gpD3DDevice->CreateVertexBuffer(offset * 4, 0, 0, D3DPOOL_MANAGED, &gpFullscreenQuadVB, NULL);
	void * vertexData = NULL;
	gpFullscreenQuadVB->Lock(0, 0, &vertexData, 0);
	{
		float * data = (float*)vertexData;
		*data++ = -1.0f;	*data++ = 1.0f;		*data++ = 0.0f;
		*data++ = 0.0f;		*data++ = 0.0f;

		*data++ = 1.0f;		*data++ = 1.0f;		*data++ = 0.0f;
		*data++ = 1.0f;		*data++ = 0;

		*data++ = 1.0f;		*data++ = -1.0f;	*data++ = 0.0f;
		*data++ = 1.0f;		*data++ = 1.0f;

		*data++ = -1.0f;	*data++ = -1.0f;	*data++ = 0.0f;
		*data++ = 0.0f;		*data++ = 1.0f;
	}
	gpFullscreenQuadVB->Unlock();

	// 인덱스 버퍼를 만든다.
	gpD3DDevice->CreateIndexBuffer(sizeof(short) * 6, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &gpFullscreenQuadIB, NULL);
	void * indexData = NULL;
	gpFullscreenQuadIB->Lock(0, 0, &indexData, 0);
	{
		unsigned short * data = (unsigned short*)indexData;
		*data++ = 0;	*data++ = 1;	*data++ = 3;
		*data++ = 3;	*data++ = 1;	*data++ = 2;
	}
	gpFullscreenQuadIB->Unlock();
}

// 뒷 정리 코드
void Cleanup()
{
	// 폰트를 release 한다.
	if (gpFont)
	{
		gpFont->Release();
		gpFont = NULL;
	}

	// 모델을 release 한다.
	//if (gpSphere)
	//{
	//	gpSphere->Release();
	//	gpSphere = NULL;
	//}
	if (gpTeapot)
	{
		gpTeapot->Release();
		gpTeapot = NULL;
	}
	//if (gpTorus)
	//{
	//	gpTorus->Release();
	//	gpTorus = NULL;
	//}
	//if (gpDisc)
	//{
	//	gpDisc->Release();
	//	gpDisc = NULL;
	//}

	// 쉐이더를 release 한다.
	//if (gpApplyShadowShader)
	//{
	//	gpApplyShadowShader->Release();
	//	gpApplyShadowShader = NULL;
	//}

	//if (gpCreateShadowShader)
	//{
	//	gpCreateShadowShader->Release();
	//	gpCreateShadowShader = NULL;
	//}
	if (gpEnvironmentMappingShader)
	{
		gpEnvironmentMappingShader->Release();
		gpEnvironmentMappingShader = NULL;
	}

	if (gpNoEffect)
	{
		gpNoEffect->Release();
		gpNoEffect = NULL;
	}

	if (gpGrayScale)
	{
		gpGrayScale->Release();
		gpGrayScale = NULL;
	}

	if (gpSepia)
	{
		gpSepia->Release();
		gpSepia = NULL;
	}

	// 텍스처를 release 한다.
	//if (gpEarthDM)
	//{
	//	gpEarthDM->Release();
	//	gpEarthDM = NULL;
	//}
	//if (gpShadowRenderTarget)
	//{
	//	gpShadowRenderTarget->Release();
	//	gpShadowRenderTarget = NULL;
	//}
	//if (gpShadowDepthStencil)
	//{
	//	gpShadowDepthStencil->Release();
	//	gpShadowDepthStencil = NULL;
	//}

	if (gpStoneDM)
	{
		gpStoneDM->Release();
		gpStoneDM = NULL;
	}

	if (gpStoneSM)
	{
		gpStoneSM->Release();
		gpStoneSM = NULL;
	}

	if (gpStoneNM)
	{
		gpStoneNM->Release();
		gpStoneNM = NULL;
	}
	if (gpSnowENV)
	{
		gpSnowENV->Release();
		gpSnowENV = NULL;
	}

	// D3D를 release 한다.
	if (gpD3DDevice)
	{
		gpD3DDevice->Release();
		gpD3DDevice = NULL;
	}

	if (gpD3D)
	{
		gpD3D->Release();
		gpD3D = NULL;
	}



	//렌더타깃을 해제한다
	if (gpSceneRenderTarget)
	{
		gpSceneRenderTarget->Release();
		gpSceneRenderTarget = NULL;
	}

	//화면 크기 사각형을 해제한다
	if (gpFullscreenQuadDecl)
	{
		gpFullscreenQuadDecl->Release();
		gpFullscreenQuadDecl = NULL;
	}
	if (gpFullscreenQuadVB)
	{
		gpFullscreenQuadVB->Release();
		gpFullscreenQuadVB = NULL;
	}
	if (gpFullscreenQuadIB)
	{
		gpFullscreenQuadIB->Release();
		gpFullscreenQuadIB = NULL;
	}
}

