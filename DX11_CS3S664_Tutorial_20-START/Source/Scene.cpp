
//
// Scene.cpp
//

#include <stdafx.h>
#include <string.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <System.h>
#include <DirectXTK\DDSTextureLoader.h>
#include <DirectXTK\WICTextureLoader.h>
#include <CGDClock.h>
#include <Scene.h>

#include <Effect.h>
#include <VertexStructures.h>
#include <Texture.h>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

//
// Methods to handle initialisation, update and rendering of the scene
HRESULT Scene::rebuildViewport(){
	// Binds the render target view and depth/stencil view to the pipeline.
	// Sets up viewport for the main window (wndHandle) 
	// Called at initialisation or in response to window resize
	ID3D11DeviceContext *context = system->getDeviceContext();
	if (!context)
		return E_FAIL;
	// Bind the render target view and depth/stencil view to the pipeline.
	ID3D11RenderTargetView* renderTargetView = system->getBackBufferRTV();
	context->OMSetRenderTargets(1, &renderTargetView, system->getDepthStencil());
	// Setup viewport for the main window (wndHandle)
	RECT clientRect;
	GetClientRect(wndHandle, &clientRect);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
	viewport.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	//Set Viewport
	context->RSSetViewports(1, &viewport);
	return S_OK;
}

// Main resource setup for the application.  These are setup around a given Direct3D device.
HRESULT Scene::initialiseSceneResources() {
	ID3D11DeviceContext* context = system->getDeviceContext();
	ID3D11Device* device = system->getDevice();
	if (!device)
		return E_FAIL;
	// Set up viewport for the main window (wndHandle) 
	rebuildViewport();

	blurUtility = new BlurUtility(device, context);

	// Setup main effects (pipeline shaders, states etc)

	// The Effect class is a helper class similar to the depricated DX9 Effect. It stores pipeline shaders, pipeline states  etc and binds them to setup the pipeline to render with a particular Effect. The constructor requires that at least shaders are provided along a description of the vertex structure.
	Effect* basicColourEffect = new Effect(device, "Shaders\\cso\\basic_colour_vs.cso", "Shaders\\cso\\basic_colour_ps.cso", basicVertexDesc, ARRAYSIZE(basicVertexDesc));
	Effect* basicLightingEffect = new Effect(device, "Shaders\\cso\\basic_lighting_vs.cso", "Shaders\\cso\\basic_colour_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	Effect* perPixelLightingEffect = new Effect(device, "Shaders\\cso\\per_pixel_lighting_vs.cso", "Shaders\\cso\\per_pixel_lighting_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	Effect* skyBoxEffect = new Effect(device, "Shaders\\cso\\sky_box_vs.cso", "Shaders\\cso\\sky_box_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));

	Effect* flareEffect = new Effect(device, "Shaders\\cso\\flare_vs.cso", "Shaders\\cso\\flare_ps.cso", flareVertexDesc, ARRAYSIZE(flareVertexDesc));
	Effect* grassEffect = new Effect(device, "Shaders\\cso\\grass_vs.cso", "Shaders\\cso\\grass_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	Effect* treeEffect = new Effect(device, "Shaders\\cso\\tree_vs.cso", "Shaders\\cso\\tree_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	Effect* oceanEffect = new Effect(device, "Shaders\\cso\\ocean_vs.cso", "Shaders\\cso\\ocean_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));

	ID3D11DepthStencilState* skyBoxDSState = skyBoxEffect->getDepthStencilState();
	D3D11_DEPTH_STENCIL_DESC skyBoxDSDesc;
	skyBoxDSState->GetDesc(&skyBoxDSDesc);
	skyBoxDSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	skyBoxDSState->Release(); device->CreateDepthStencilState(&skyBoxDSDesc, &skyBoxDSState);
	skyBoxEffect->setDepthStencilState(skyBoxDSState);

	D3D11_BLEND_DESC flareBlendDesc;
	ID3D11BlendState* flareBlendState = flareEffect->getBlendState();
	flareBlendState->GetDesc(&flareBlendDesc);
	flareBlendDesc.AlphaToCoverageEnable = FALSE;
	flareBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	flareBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	flareBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	flareBlendState->Release(); device->CreateBlendState(&flareBlendDesc, &flareBlendState);
	flareEffect->setBlendState(flareBlendState);

	D3D11_BLEND_DESC grassBlendDesc;
	ID3D11BlendState* grassBlendState = grassEffect->getBlendState();
	grassBlendState->GetDesc(&grassBlendDesc);
	grassBlendDesc.AlphaToCoverageEnable = TRUE;
	grassBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	grassBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	grassBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	grassBlendState->Release(); device->CreateBlendState(&grassBlendDesc, &grassBlendState);
	grassEffect->setBlendState(grassBlendState);
	treeEffect->setBlendState(grassBlendState);

	ID3D11DepthStencilState* grassDSstate = grassEffect->getDepthStencilState();
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	grassDSstate->GetDesc(&dsDesc);
	grassDSstate->Release(); device->CreateDepthStencilState(&dsDesc, &grassDSstate);
	grassEffect->setDepthStencilState(grassDSstate);

	Texture* cubeDayTexture = new Texture(device, L"Resources\\Textures\\grassenvmap1024.dds");
	ID3D11ShaderResourceView* skyBoxTextureArray[] = { cubeDayTexture->getShaderResourceView() };
	box = new Box(device, skyBoxEffect, NULL, 0, skyBoxTextureArray, 1);
	box->setWorldMatrix(box->getWorldMatrix() * XMMatrixScaling(2000, 2000, 2000));
	box->update(context);

	Texture* flare1Texture = new Texture(device, L"Resources\\Textures\\flares\\divine.png");
	Texture* flare2Texture = new Texture(device, L"Resources\\Textures\\flares\\extendring.png");
	ID3D11ShaderResourceView* flare1TextureArray[] = { flare1Texture->getShaderResourceView() };
	ID3D11ShaderResourceView* flare2TextureArray[] = { flare2Texture->getShaderResourceView() };

	for (int i = 0; i < numFlares; i++)
		if (randM1P1() > 0)
			flares[i] = new Flare(XMFLOAT3(-125.0, 60.0, 70.0), XMCOLOR(randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, (float)i / numFlares), device, flareEffect, NULL, 0, flare1TextureArray, 1);
		else
			flares[i] = new Flare(XMFLOAT3(-125.0, 60.0, 70.0), XMCOLOR(randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, (float)i / numFlares), device, flareEffect, NULL, 0, flare2TextureArray, 1);

	Texture* grassAlpha = new Texture(device, L"Resources\\Textures\\grassAlpha.tif");
	Texture* grassTexture = new Texture(device, L"Resources\\Textures\\grass.png");
	ID3D11ShaderResourceView * grassTextureArray[] = { grassTexture->getShaderResourceView(), grassAlpha->getShaderResourceView() };
	Texture* heightMap = new Texture(device, L"Resources\\Textures\\heightmap2.bmp");
	Texture* normalMap = new Texture(device, L"Resources\\Textures\\normalmap.bmp");
	ground = new Terrain(device, context, 32, 32, heightMap->getTexture(), normalMap->getTexture(), grassEffect, NULL, 0, grassTextureArray, 2);
	ground->setWorldMatrix(ground->getWorldMatrix() * XMMatrixTranslation(-16, -0.1, -16) * XMMatrixScaling(1, 3, 1) * XMMatrixRotationY(XMConvertToRadians(0)));
	ground->update(context);

	Texture* treeTexture = new Texture(device, L"Resources\\Textures\\tree.tif");
	ID3D11ShaderResourceView* treeTextureArray[] = { treeTexture->getShaderResourceView() };
	tree = new Model(device, wstring(L"Resources\\Models\\tree.3ds"), treeEffect, NULL, 0, treeTextureArray, 1);
	tree->setWorldMatrix(tree->getWorldMatrix() * XMMatrixTranslation(0, 0, 0) * XMMatrixScaling(10, 10, 10));
	tree->update(context);

	Texture* waterNormalMap = new Texture(device, L"Resources\\Textures\\waves.dds");
	ID3D11ShaderResourceView* waterTextureArray[] = { waterNormalMap->getShaderResourceView(), cubeDayTexture->getShaderResourceView() };
	water = new Grid(16, 16, device, oceanEffect, NULL, 0, waterTextureArray, 2);
	water->setWorldMatrix(water->getWorldMatrix() * XMMatrixTranslation(-8, 0, -8));
	water->update(context);

	// Setup a camera
	// The LookAtCamera is derived from the base Camera class. The constructor for the Camera class requires a valid pointer to the main DirectX device
	// and and 3 vectors to define the initial position, up vector and target for the camera.
	// The camera class  manages a Cbuffer containing view/projection matrix properties. It has methods to update the cbuffers if the camera moves changes  
	// The camera constructor and update methods also attaches the camera CBuffer to the pipeline at slot b1 for vertex and pixel shaders
	mainCamera = new LookAtCamera(device, XMVectorSet(0.0, 0.0, -10.0, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMVectorZero());

	// Add a CBuffer to store light properties - you might consider creating a Light Class to manage this CBuffer
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferLightCPU = (CBufferLight*)_aligned_malloc(sizeof(CBufferLight), 16);

	// Fill out cBufferLightCPU
	cBufferLightCPU->lightVec = XMFLOAT4(-5.0, 2.0, 5.0, 1.0);
	cBufferLightCPU->lightAmbient = XMFLOAT4(0.2, 0.2, 0.2, 1.0);
	cBufferLightCPU->lightDiffuse = XMFLOAT4(0.7, 0.7, 0.7, 1.0);
	cBufferLightCPU->lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);

	// Create GPU resource memory copy of cBufferLight
	// fill out description (Note if we want to update the CBuffer we need  D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferLight);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferLightCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferLightGPU);

	// We dont strictly need to call map here as the GPU CBuffer was initialised from the CPU CBuffer at creation.
	// However if changes are made to the CPU CBuffer during update the we need to copy the data to the GPU CBuffer 
	// using the mapCbuffer helper function provided the in Util.h   

	mapCbuffer(context, cBufferLightCPU, cBufferLightGPU, sizeof(CBufferLight));
	context->VSSetConstantBuffers(2, 1, &cBufferLightGPU);// Attach CBufferLightGPU to register b2 for the vertex shader. Not strictly needed as our vertex shader doesnt require access to this CBuffer
	context->PSSetConstantBuffers(2, 1, &cBufferLightGPU);// Attach CBufferLightGPU to register b2 for the Pixel shader.

	// Add a Cbuffer to stor world/scene properties
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferSceneCPU = (CBufferScene*)_aligned_malloc(sizeof(CBufferScene), 16);

	// Fill out cBufferSceneCPU
	cBufferSceneCPU->windDir = XMFLOAT4(1, 0, 0, 1);
	cBufferSceneCPU->Time = 0.0;
	cBufferSceneCPU->grassHeight = 10.0;

	cbufferInitData.pSysMem = cBufferSceneCPU;// Initialise GPU CBuffer with data from CPU CBuffer
	cbufferDesc.ByteWidth = sizeof(CBufferScene);

	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferSceneGPU);

	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	context->VSSetConstantBuffers(3, 1, &cBufferSceneGPU);// Attach CBufferSceneGPU to register b3 for the vertex shader. Not strictly needed as our vertex shader doesnt require access to this CBuffer
	context->PSSetConstantBuffers(3, 1, &cBufferSceneGPU);// Attach CBufferSceneGPU to register b3 for the Pixel shader

	return S_OK;
}

// Update scene state (perform animations etc)
HRESULT Scene::updateScene(ID3D11DeviceContext *context,Camera *camera) {

	// mainClock is a helper class to manage game time data
	mainClock->tick();
	double dT = mainClock->gameTimeDelta();
	double gT = mainClock->gameTimeElapsed();
	cout << "Game time Elapsed= " << gT << " seconds" << endl;

	// If the CPU CBuffer contents are changed then the changes need to be copied to GPU CBuffer with the mapCbuffer helper function
	mainCamera->update(context);

	// Update the scene time as it is needed to animate the water
	cBufferSceneCPU->Time = gT;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	
	return S_OK;
}

void Scene::DrawGround(ID3D11DeviceContext* context)
{
	if (ground)
	{
		for (int i = 0; i < numGrassPasses; i++)
		{
			cBufferSceneCPU->grassHeight = (grassLength / numGrassPasses) * i;
			mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
			ground->render(context);
		}
	}
}

void Scene::DrawFlare(ID3D11DeviceContext *context)
{
// Draw the Fire (Draw all transparent objects last)
if (flares) {

	ID3D11RenderTargetView * tempRT[1] = { 0 };
	ID3D11DepthStencilView *tempDS = nullptr;

		// Set NULL depth buffer so we can also use the Depth Buffer as a shader resource
		// This is OK as we dont need depth testing for the flares

		ID3D11DepthStencilView * nullDSV[1]; nullDSV[0] = NULL;
		context->OMGetRenderTargets(1, tempRT, &tempDS);
		context->OMSetRenderTargets(1, tempRT, NULL);
		ID3D11ShaderResourceView *depthSRV = system->getDepthStencilSRV();
		context->VSSetShaderResources(1, 1, &depthSRV);

		for (int i = 0; i < numFlares; i++)
			flares[i]->render(context);

		ID3D11ShaderResourceView * nullSRV[1]; nullSRV[0] = NULL; // Used to release depth shader resource so it is available for writing
		context->VSSetShaderResources(1, 1, nullSRV);
		// Return default (read and write) depth buffer view.
		context->OMSetRenderTargets(1, tempRT, tempDS);

	}
}

// Render scene
HRESULT Scene::renderScene() {

	ID3D11DeviceContext *context = system->getDeviceContext();
	
	// Validate window and D3D context
	if (isMinimised() || !context)
		return E_FAIL;
	
	// Clear the screen
	static const FLOAT clearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	context->ClearRenderTargetView(system->getBackBufferRTV(), clearColor);
	context->ClearDepthStencilView(system->getDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render Scene objects

	if (box)
		box->render(context);

	DrawFlare(context);

	DrawGround(context);
	
	tree->render(context);

	water->render(context);

	// Present current frame to the screen
	HRESULT hr = system->presentBackBuffer();

	return S_OK;
}

//
// Event handling methods
//
// Process mouse move with the left button held down
void Scene::handleMouseLDrag(const POINT &disp) {
	//LookAtCamera

	mainCamera->rotateElevation((float)-disp.y * 0.01f);
	mainCamera->rotateOnYAxis((float)-disp.x * 0.01f);

	//FirstPersonCamera
	//	mainCamera->elevate((float)-disp.y * 0.01f);
	//	mainCamera->turn((float)-disp.x * 0.01f);
}

// Process mouse wheel movement
void Scene::handleMouseWheel(const short zDelta) {
	//LookAtCamera

	if (zDelta<0)
		mainCamera->zoomCamera(1.2f);
	else if (zDelta>0)
		mainCamera->zoomCamera(0.9f);
	cout << "zoom" << endl;
	//FirstPersonCamera
	//mainCamera->move(zDelta*0.01);
}

// Process key down event.  keyCode indicates the key pressed while extKeyFlags indicates the extended key status at the time of the key down event (see http://msdn.microsoft.com/en-gb/library/windows/desktop/ms646280%28v=vs.85%29.aspx).
void Scene::handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags) {
	// Add key down handler here...
}

// Process key up event.  keyCode indicates the key released while extKeyFlags indicates the extended key status at the time of the key up event (see http://msdn.microsoft.com/en-us/library/windows/desktop/ms646281%28v=vs.85%29.aspx).
void Scene::handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags) {
	// Add key up handler here...
}






// Clock handling methods
void Scene::startClock() {
	mainClock->start();
}

void Scene::stopClock() {
	mainClock->stop();
}

void Scene::reportTimingData() {

	cout << "Actual time elapsed = " << mainClock->actualTimeElapsed() << endl;
	cout << "Game time elapsed = " << mainClock->gameTimeElapsed() << endl << endl;
	mainClock->reportTimingData();
}

// Private constructor
Scene::Scene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {
	try
	{
		// 1. Register window class for main DirectX window
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
		wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = wndClassName;
		wcex.hIconSm = NULL;
		if (!RegisterClassEx(&wcex))
			throw exception("Cannot register window class for Scene HWND");
		// 2. Store instance handle in our global variable
		hInst = hInstance;
		// 3. Setup window rect and resize according to set styles
		RECT		windowRect;
		windowRect.left = 0;
		windowRect.right = _width;
		windowRect.top = 0;
		windowRect.bottom = _height;
		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;
		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);
		// 4. Create and validate the main window handle
		wndHandle = CreateWindowEx(dwExStyle, wndClassName, wndTitle, dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 500, 500, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL, NULL, hInst, this);
		if (!wndHandle)
			throw exception("Cannot create main window handle");
		ShowWindow(wndHandle, nCmdShow);
		UpdateWindow(wndHandle);
		SetFocus(wndHandle);
		// 5. Create DirectX host environment (associated with main application wnd)
		system = System::CreateDirectXSystem(wndHandle);
		if (!system)
			throw exception("Cannot create Direct3D device and context model");
		// 6. Setup application-specific objects
		HRESULT hr = initialiseSceneResources();
		if (!SUCCEEDED(hr))
			throw exception("Cannot initalise scene resources");
		// 7. Create main clock / FPS timer (do this last with deferred start of 3 seconds so min FPS / SPF are not skewed by start-up events firing and taking CPU cycles).
		mainClock = CGDClock::CreateClock(string("mainClock"), 3.0f);
		if (!mainClock)
			throw exception("Cannot create main clock / timer");
	}
	catch (exception &e)
	{
		cout << e.what() << endl;
		// Re-throw exception
		throw;
	}
}

// Helper function to call updateScene followed by renderScene
HRESULT Scene::updateAndRenderScene() {
	ID3D11DeviceContext *context = system->getDeviceContext();
	HRESULT hr = updateScene(context, (Camera*)mainCamera);
	if (SUCCEEDED(hr))
		hr = renderScene();
	return hr;
}

// Return TRUE if the window is in a minimised state, FALSE otherwise
BOOL Scene::isMinimised() {

	WINDOWPLACEMENT				wp;

	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);

	return (GetWindowPlacement(wndHandle, &wp) != 0 && wp.showCmd == SW_SHOWMINIMIZED);
}

//
// Public interface implementation
//
// Method to create the main Scene instance
Scene* Scene::CreateScene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {
	static bool _scene_created = false;
	Scene *system = nullptr;
	if (!_scene_created) {
		system = new Scene(_width, _height, wndClassName, wndTitle, nCmdShow, hInstance, WndProc);
		if (system)
			_scene_created = true;
	}
	return system;
}

// Destructor
Scene::~Scene() {
	//Clean Up
	if (wndHandle)
		DestroyWindow(wndHandle);
}

// Call DestoryWindow on the HWND
void Scene::destoryWindow() {
	if (wndHandle != NULL) {
		HWND hWnd = wndHandle;
		wndHandle = NULL;
		DestroyWindow(hWnd);
	}
}

//
// Private interface implementation
//
// Resize swap chain buffers and update pipeline viewport configurations in response to a window resize event
HRESULT Scene::resizeResources() {
	if (system) {
		// Only process resize if the System *system exists (on initial resize window creation this will not be the case so this branch is ignored)
		HRESULT hr = system->resizeSwapChainBuffers(wndHandle);
		rebuildViewport();
		RECT clientRect;
		GetClientRect(wndHandle, &clientRect);
		if (!isMinimised())
			renderScene();
	}
	return S_OK;
}

