#ifndef _DXRender_
#define _DXRender_

float failcount = 0;
float domeDis[2] = { 0, 0};
const int desiredFPS = 1000 / 30;
bool canFPS = true;

D3DXVECTOR2 orbitAngleAccel = D3DXVECTOR2(0,0);
D3DXVECTOR2 orbitAngleFixed = D3DXVECTOR2(0, 0);

D3DXVECTOR2 orbitViewAccel = D3DXVECTOR2(0, 0);
D3DXVECTOR2 orbitViewFixed  = D3DXVECTOR2(0, 0);

#include "Constantes.h"
#include "GameObjects.h"

class DXRender
{
private:
	//Global Declarations//
	IDXGISwapChain* SwapChain;
	ID3D11Device* d3d11Device;
	ID3D11DeviceContext* d3d11DevCon;
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;
	ID3D11Texture2D* BackBuffer;
	ID3D11Texture2D* PostProc;

	ID3D11ShaderResourceView* refRes;//Textura de postproceso

	IDirectInputDevice8* DIKeyboard;
	IDirectInputDevice8* DIMouse;
	IDirectInputDevice8* DIJoystick;
	DIMOUSESTATE mouseLastState;
	LPDIRECTINPUT8 DirectInput;

	D3DXMATRIX View;
	D3DXMATRIX Projection;

	MPrimitives *skydomo[2];
	MPrimitives *spaceDome;

	MPrimitives *rogue;
	MPrimitives *citizen;
	MPrimitives *pride;
	MPrimitives *cube;

	//Camara *camara;

	float rotPerFrame;
public:
	DXRender()
	{
		rotPerFrame = 0.0f;
	}

	bool InitDevice(HWND hwnd, int Width, int Height)
	{
		//Describe our Buffer
		DXGI_MODE_DESC bufferDesc;

		ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

		bufferDesc.Width = Width;
		bufferDesc.Height = Height;
		bufferDesc.RefreshRate.Numerator = 1000;
		bufferDesc.RefreshRate.Denominator = 60;
		bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST;
		bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		//Describe our SwapChain
		DXGI_SWAP_CHAIN_DESC swapChainDesc;

		ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

		swapChainDesc.BufferDesc = bufferDesc;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = D3D11_BIND_SHADER_RESOURCE;

		//Create our SwapChain
		D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
			D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon);

		//Create our BackBuffer
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

		//Create our Render Target
		d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView);
		BackBuffer->Release();

		//Crea la descripcion del raster, el cual determina como se dibujaran los poligonos
		D3D11_RASTERIZER_DESC rasterDesc;
		ID3D11RasterizerState* rasterState;
		rasterDesc.AntialiasedLineEnable = false;
		rasterDesc.CullMode = D3D11_CULL_NONE;//se puede manipular para transparencias, cambiar a D3D11_CULL_NONE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.DepthClipEnable = false;
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.FrontCounterClockwise = false;
		rasterDesc.MultisampleEnable = false;
		rasterDesc.ScissorEnable = false;
		rasterDesc.SlopeScaledDepthBias = 0;

		d3d11Device->CreateRasterizerState(&rasterDesc, &rasterState);
		d3d11DevCon->RSSetState(rasterState);

		//Describe our Depth/Stencil Buffer
		D3D11_TEXTURE2D_DESC depthStencilDesc;
		ID3D11Texture2D* depthStencilBuffer;

		depthStencilDesc.Width     = Width;
		depthStencilDesc.Height    = Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count   = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0; 
		depthStencilDesc.MiscFlags      = 0;

		//Create the Depth/Stencil View
		d3d11Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
		d3d11Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);


		D3D11_BLEND_DESC blendDesc;
		ID3D11BlendState* blendState;
		ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		d3d11Device->CreateBlendState(&blendDesc, &blendState);

		d3d11DevCon->OMSetBlendState(blendState, 0, 0xffffffff);
		d3d11DevCon->OMSetRenderTargets( 1, &renderTargetView, depthStencilView );

		skydomo[0] = new MPrimitives(d3d11Device, d3d11DevCon, "Models/SkyBox.txt", L"Texturas/World2_Sky - copia.png", PS_SHADELESS);
		skydomo[1] = new MPrimitives(d3d11Device, d3d11DevCon, "Models/SkyBox.txt", L"Texturas/World2_Sky - copia.png", PS_SHADELESS);
		spaceDome = new MPrimitives(d3d11Device, d3d11DevCon, "Models/SkyBox.txt", L"Texturas/skyspace.png", PS_SHADELESS);

		D3DXMATRIX skyRotMat, terrMat;
		D3DXMatrixTranslation(&skyRotMat, 512, -2, 512);
		D3DXMatrixScaling(&terrMat, 1000, 1000, 1000);
		skydomo[0]->World = terrMat*skyRotMat;

		D3DXMatrixTranslation(&skyRotMat, 512, 5000, 512);
		D3DXMatrixScaling(&terrMat, 1000, 1000, 1000);
		skydomo[1]->World = terrMat*skyRotMat;

		terreno[0] = new MPrimitives(d3d11Device, d3d11DevCon, 50.0f, L"Texturas/arena.jpg", "Texturas/Heightmap.bmp", PS_NORMALMAP);
		terreno[0]->setNormalMap(L"Texturas/heightmapnormal.png");
		terreno[1] = new MPrimitives(d3d11Device, d3d11DevCon, 50.0f, L"Texturas/Pgrass.jpg", "Texturas/Heightmap2.bmp", PS_NORMALMAP);
		terreno[1]->setNormalMap(L"Texturas/Aguas.bmp");

		rogue = new MPrimitives(d3d11Device, d3d11DevCon,"Models/RogueSentinel.txt", L"Texturas/RogueSentinel.png", PS_STATICLIGHT);
		rogue->setNormalMap(L"Texturas/RogueSentinelLight.png");
		
		citizen = new MPrimitives(d3d11Device, d3d11DevCon, "Models/CitizenSquad.txt", L"Texturas/CitizenSquad.png", PS_DIFFUSE);

		laser = new MPrimitives(d3d11Device, d3d11DevCon, "Models/Laser.txt", L"Texturas/Laser.png", PS_SHADELESS);

		pride = new MPrimitives(d3d11Device, d3d11DevCon, "Models/WSPride.txt", L"Texturas/WsPride.png", PS_STATICLIGHT);
		pride->setNormalMap(L"Texturas/WsPrideLight.png");

		cube = new MPrimitives(d3d11Device, d3d11DevCon, 3, NULL, PS_SHADELESS);

		HealthCap	= new MPrimitives(d3d11Device, d3d11DevCon, 16, 16, 2, L"Texturas/HealthCap.png", PS_SHADELESS);
		Oxigen		= new MPrimitives(d3d11Device, d3d11DevCon, .01, L"Texturas/Oxy.png", PS_SHADELESS);
		Life		= new MPrimitives(d3d11Device, d3d11DevCon, .01, L"Texturas/Life.png", PS_SHADELESS);
		AttackCap   = new MPrimitives(d3d11Device, d3d11DevCon, 16, 16, 2, L"Texturas/AttackCap.png", PS_SHADELESS);
		Fail		= new MPrimitives(d3d11Device, d3d11DevCon, .2, L"Texturas/Fail.png", PS_SHADELESS);

		Objects = new GameObject(pride, new oWsPride(), { 0,20,-10 }, NULL);

		for (int i = 0; i<20; i++)
			Objects->Add(rogue, new oRogueSentinel(), D3DXVECTOR3( (rand()%512-128)*2, (rand()%256)*2, (rand()%512-128)*2));

		Behaviour *nobj = NULL;
		for (int i = 0; i<20; i++){
			nobj = new oRogueSentinel();
			Objects->Add(citizen, nobj, D3DXVECTOR3( (rand()%512-128)*2, 5000-(rand()%256)*2, (rand()%512-128)*2));

		}

		return true;
	}

	bool InitDirectInput(HINSTANCE hInstance, HWND hwnd)
	{
		DirectInput8Create(hInstance,
			DIRECTINPUT_VERSION,
			IID_IDirectInput8,
			(void**)&DirectInput,
			NULL); 

		DirectInput->CreateDevice(GUID_SysKeyboard,
			&DIKeyboard,
			NULL);

		DirectInput->CreateDevice(GUID_SysMouse,
			&DIMouse,
			NULL);

		DirectInput->CreateDevice(GUID_Joystick,
			&DIJoystick,
			NULL);


		DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
		DIKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

		DIMouse->SetDataFormat(&c_dfDIMouse);
		DIMouse->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);

		return true;
	}

	void DetectInput()
	{

		DIKeyboard->Acquire();
		DIMouse->Acquire();

		DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);

		DIKeyboard->GetDeviceState(sizeof(keyboardState),(LPVOID)&keyboardState);

		ZeroMemory(&state, sizeof(XINPUT_STATE));
		if (XInputGetState(0, &state) == ERROR_SUCCESS)//for (i) para muchos
		{
			float deadzoneX = 0.20f;
			float deadzoneY = 0.10f;

			//LerfStick
			float normLX = fmaxf(-1, (float)state.Gamepad.sThumbLX / 32767);
			float normLY = fmaxf(-1, (float)state.Gamepad.sThumbLY / 32767);
			//RightStick
			float normRX = fmaxf(-1, (float)state.Gamepad.sThumbRX / 32767);
			float normRY = fmaxf(-1, (float)state.Gamepad.sThumbRY / 32767);

			Stick[0].x = (abs(normLX) < deadzoneX ? 0 : normLX);
			Stick[0].y = (abs(normLY) < deadzoneY ? 0 : normLY);

			Stick[1].x = (abs(normRX) < deadzoneX ? 0 : normRX);
			Stick[1].y = (abs(normRY) < deadzoneY ? 0 : normRY);

			Trigger[0] = (float)state.Gamepad.bLeftTrigger  / 255;
			Trigger[1] = (float)state.Gamepad.bRightTrigger / 255;
		}



		float speed = 0.01f;
		//float moveLeftRight   = (keyboardState[DIK_D] - keyboardState[DIK_A]) *speed;
		//float moveBackForward = (keyboardState[DIK_W] - keyboardState[DIK_S]) *speed;

		if (keyboardState[DIK_1] || state.Gamepad.wButtons & XINPUT_GAMEPAD_START) {
			if (canFPS)
				TPS = !TPS;
			canFPS = false;
		}
		else canFPS = true;

		float smoothness = 0.001f;
		if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) || mouseCurrState.rgbButtons[MB_MIDDLE]) {
			orbitAngleAccel.x -= (mouseCurrState.lX + Stick[1].x * 30)*4.5 * smoothness * (1-2*TPS);
			orbitAngleAccel.y += (Stick[1].y * 25 - mouseCurrState.lY)*5.0 * smoothness * (1-2*TPS);
		}
		else {
			camAccel.x += (mouseCurrState.lX + Stick[1].x*40) * smoothness * .75;
			camAccel.y += (mouseCurrState.lY - Stick[1].y*35) * smoothness;
		}


		camara->OrbitAngle.x += orbitAngleAccel.x;
		camara->OrbitAngle.y += orbitAngleAccel.y;
		orbitAngleAccel *= .9-(TPS*.2);

		camara->OrbitView.y -= (1 - 2 * !TPS)*.25;

		//orbitViewFixed += orbitViewAccel;

		if (camara->OrbitView.y > -12)
			camara->OrbitView.y = -12;//  +orbitViewFixed.x;
		else
			if (camara->OrbitView.y < -20.5)
				camara->OrbitView.y = -20.5;// +orbitViewFixed.y;

	

		camara->distance -= (1 - 2 * !TPS)*.5;
		if (camara->distance > 10)
			camara->distance = 10;
		else
		if (camara->distance < 0)
			camara->distance = 0;

		camara->UpdateCameraTPS(0, 0, camAccel.x, camAccel.y);
		camAccel *= .5;
	}

	void ReleaseObjects()
	{
		//Release the COM Objects we created
		SwapChain->Release();
		d3d11Device->Release();
		d3d11DevCon->Release();
		renderTargetView->Release();
		depthStencilView->Release();
	}

	bool InitScene(int Width, int Height)
	{
		//Create the Viewport
		D3D11_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = Width;
		viewport.Height = Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		//Set the Viewport
		d3d11DevCon->RSSetViewports(1, &viewport);

		//Create the camera
		camara = new Camara(D3DXVECTOR3(0.0f, 4.0f, -20.0f), D3DXVECTOR3(1.0f, 0.0f, 1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f), Width, Height);
		camara->OrbitAngle.y = 15;


		return true;
	}

	void UpdateScene()
	{
		domeDis[0] = distance3D({ 0,camara->tpsPosition.y,0 }, { 0, 0, 0 });
		skydomo[0]->light.ambient[3] = min(.2 + ((.0008 + (domeDis[0] > 1000)*.005)*abs(1000 - domeDis[0])), 2);
		

		domeDis[1] = distance3D({ 0,camara->tpsPosition.y,0 }, { 0, 5000, 0 });
		skydomo[1]->light.ambient[3] = min(.2 + ((.0008 + (domeDis[1] > 1000)*.005)*abs(1000 - domeDis[1])), 2);

		if ((domeDis[0] > 1000) && (domeDis[1] > 1000)) 
			_oxygen -= .1;
		else
			_oxygen += .05;
		if (_oxygen > 100)
			_oxygen = 100;

		D3DXMATRIX skyRotMat, terrMat;

		D3DXMatrixTranslation(&skyRotMat, camara->tpsPosition.x, camara->tpsPosition.y, camara->tpsPosition.z);
		D3DXMatrixScaling(&terrMat, 5000, 5000, 5000);
		spaceDome->World = terrMat *skyRotMat;

		D3DXMatrixScaling(&terreno[0]->World, 2, 4, 2);
		D3DXMatrixTranslation(&terrMat, 0, -2, 0);
		terreno[0]->UpdateScene(terreno[0]->World*terrMat);

		D3DXMatrixScaling(&terreno[1]->World, 2, -4, 2);
		D3DXMatrixTranslation(&terrMat, 0, 5000, 0);
		terreno[1]->UpdateScene(terreno[1]->World*terrMat);

		D3DXMatrixTranslation(&Life->World  , 3, 0, 0);
		D3DXMatrixIdentity(&cube->World);
	}

	void DrawScene()
	{

		//Clear our backbuffer to the updated color
		D3DXCOLOR bgColor(0.0f, 0.0f, 0.0f, 1.0f);


		//A pantalla
		d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);
		//Refresh the Depth/Stencil view
		d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


		if (!((skydomo[0]->light.ambient[3] > .9) && domeDis[0] > 1000))
			terreno[0]->DrawScene(camara);

		if (!((skydomo[1]->light.ambient[3] > .9) && domeDis[1] > 1000))
			terreno[1]->DrawScene(camara);

		spaceDome->DrawScene(camara);
		skydomo[domeDis[0] < domeDis[1]]->DrawScene(camara);
		skydomo[domeDis[0] > domeDis[1]]->DrawScene(camara);
		LoopObjects();


		camara->setFov(10);
		D3DXMATRIX tmp, tmp1, tmp2, tmp3;
		D3DXVECTOR3 fwd = getFowardSpd({ camara->Yaw, camara->Pitch, camara->Roll }, { -0.14f,-0.12f,1 });
		D3DXMatrixTranslation(&tmp, camara->tpsPosition.x + fwd.x, camara->tpsPosition.y + fwd.y, camara->tpsPosition.z + fwd.z);
		D3DXMatrixRotationYawPitchRoll(&tmp1, camara->Yaw, camara->Pitch, camara->Roll);
		D3DXMatrixScaling(&tmp2, 1, .25, 0);


		d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		D3DXVECTOR3 point = getFowardSpd({ camara->Yaw, camara->Pitch, camara->Roll }, { 0, 0.0025f ,0 });

		for (int i = 0; i < _oxygen / 2; i++) {
			D3DXMatrixTranslation(&tmp3, camara->tpsPosition.x + fwd.x + point.x*i, camara->tpsPosition.y + fwd.y + point.y*i, camara->tpsPosition.z + fwd.z + point.z*i);
			Oxigen->World = tmp2*tmp1*tmp3;
			Oxigen->DrawScene(camara);
		}
		fwd = getFowardSpd({ camara->Yaw, camara->Pitch, camara->Roll }, { -0.15f,-0.12f,1 });
		for (int i = 0; i < _health / 2; i++) {
			D3DXMatrixTranslation(&tmp3, camara->tpsPosition.x + fwd.x + point.x*i, camara->tpsPosition.y + fwd.y + point.y*i, camara->tpsPosition.z + fwd.z + point.z*i);
			Life->World = tmp2*tmp1*tmp3;
			Life->DrawScene(camara);
		}

		if (_health <= 0) 
		{
			fwd = getFowardSpd({ camara->Yaw, camara->Pitch, camara->Roll }, { 0.0f,-0.05f,1.0f });
			D3DXMatrixTranslation(&tmp3, camara->tpsPosition.x + fwd.x, camara->tpsPosition.y + fwd.y, camara->tpsPosition.z + fwd.z);
			Fail->World = tmp1*tmp3;
			Fail->DrawScene(camara);
			Fail->light.ambient[3] -= .04;
			if (Fail->light.ambient[3] <= 0)
				Fail->light.ambient[3] = 2;
			cube->light.ambient[3] += .005;
			cube->DrawScene(camara);
		}
		else
			cube->light.ambient[3] = -1;

		camara->setFov(_FOV);
		//Oxigen->DrawScene(camara);
		//Life->DrawScene(camara);

		SwapChain->Present(0, 0);
	}
};

#endif