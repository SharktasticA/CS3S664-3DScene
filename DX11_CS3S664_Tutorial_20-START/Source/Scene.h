
//
// Scene.h
//

// DirectX interfaces and scene variables (model)
#pragma once
#include <Windows.h>
#include <CGDClock.h>
#include <Camera.h>
#include <LookAtCamera.h>
#include <Triangle.h>
#include <Model.h>
#include <Box.h>
#include <Grid.h>
#include <Flare.h>
#include <Terrain.h>
#include <BlurUtility.h>
#include <CBufferStructures.h>


class Scene{

	HINSTANCE								hInst = NULL;
	HWND									wndHandle = NULL;

	// Strong reference to associated Direct3D device and rendering context.
	System									*system = nullptr;

	D3D11_VIEWPORT							viewport;
	CGDClock								*mainClock;
	LookAtCamera							*mainCamera;

	CBufferScene* cBufferSceneCPU = nullptr;
	ID3D11Buffer *cBufferSceneGPU = nullptr;
	CBufferLight* cBufferLightCPU = nullptr;
	ID3D11Buffer *cBufferLightGPU = nullptr;


	// Add objects to the scene
	Triangle								*triangle = nullptr; //pointer to a Triangle the actual triangle is created in initialiseSceneResources
	Box										*box = nullptr; //pointer to a Triangle the actual triangle is created in initialiseSceneResources
	Model									*orb = nullptr;
	Grid									*water = nullptr;
	BlurUtility								*blurUtility = nullptr;
	static const int						numFlares = 6;
	Flare* flares[numFlares];
	Model* tree = nullptr;
	Grid* grass = nullptr;
	const float	grassLength = 0.05f;
	int numGrassPasses = 80;
	Terrain* ground = nullptr;

	// Private constructor
	Scene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc);
	// Return TRUE if the window is in a minimised state, FALSE otherwise
	BOOL isMinimised();

public:
	// Public methods
	// Method to create the main Scene
	static Scene* CreateScene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc);
	
	// Methods to handle initialisation, update and rendering of the scene
	HRESULT rebuildViewport();
	HRESULT initialiseSceneResources();
	HRESULT updateScene(ID3D11DeviceContext *context, Camera *camera);
	HRESULT renderScene();
	void DrawGrass(ID3D11DeviceContext* context);
	void DrawGround(ID3D11DeviceContext* context);
	void DrawFlare(ID3D11DeviceContext *context);
	// Clock handling methods
	void startClock();
	void stopClock();
	void reportTimingData();

	// Event handling methods
	// Process mouse move with the left button held down
	void handleMouseLDrag(const POINT &disp);
	// Process mouse wheel movement
	void handleMouseWheel(const short zDelta);
	// Process key down event.  keyCode indicates the key pressed while extKeyFlags indicates the extended key status at the time of the key down event (see http://msdn.microsoft.com/en-gb/library/windows/desktop/ms646280%28v=vs.85%29.aspx).
	void handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags);
	// Process key up event.  keyCode indicates the key released while extKeyFlags indicates the extended key status at the time of the key up event (see http://msdn.microsoft.com/en-us/library/windows/desktop/ms646281%28v=vs.85%29.aspx).
	void handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags);
	
	// Helper function to call updateScene followed by renderScene
	HRESULT updateAndRenderScene();

	// Destructor
	~Scene();
	// Decouple the encapsulated HWND and call DestoryWindow on the HWND
	void destoryWindow();
	// Resize swap chain buffers and update pipeline viewport configurations in response to a window resize event
	HRESULT resizeResources();
};
