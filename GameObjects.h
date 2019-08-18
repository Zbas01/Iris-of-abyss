#define _USE_MATH_DEFINES
#include <math.h>
#include "Camara.h"
#pragma region FUNCIONES
	float angleDifference (float x, float y)
	{
		return atan2(sin(x - y), cos(x - y));
	}
	D3DXVECTOR3 getFoward(D3DXVECTOR3 angle, D3DXVECTOR3 foward) {
		D3DXMATRIX mat;
		D3DXMatrixRotationYawPitchRoll(&mat, angle.x, angle.y, angle.z);
		D3DXVec3TransformCoord(&foward, &foward, &mat);
		D3DXVec3Normalize(&foward, &foward);//define direccion no velocidad 2 = 1, 5 = 1, 1.1 = 1, ...
		return foward;
	}
	D3DXVECTOR3 getFowardSpd(D3DXVECTOR3 angle, D3DXVECTOR3 foward) {
		D3DXMATRIX mat;
		D3DXMatrixRotationYawPitchRoll(&mat, angle.x, angle.y, angle.z);
		D3DXVec3TransformCoord(&foward, &foward, &mat);
		return foward;
	}
	double distance3D(double x1, double x2, double y1, double y2, double z1, double z2) {
		return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
	}
	double distance3D(D3DXVECTOR3 Pos1, D3DXVECTOR3 Pos2){
		return sqrt(pow(Pos1.x - Pos2.x, 2) + pow(Pos1.y - Pos2.y, 2) + pow(Pos1.z - Pos2.z, 2));
	}
	double angle2D(D3DXVECTOR2 p1, D3DXVECTOR2 p2)
	{
		p1.x = p2.x - p1.x;
		p1.y = p2.y - p1.y;
		p1.x = -atan2(p1.y, p1.x);
		return p1.x + D3DXToRadian(360)*(p1.x < 0);
	}
	D3DXVECTOR3 angle3D(D3DXVECTOR3 p1, D3DXVECTOR3 p2)
	{
		p1 = -(p2 - p1);
		p2.x =  atan2(p1.x, p1.z);
		p2.y = atan2(-p1.y, sqrt(p1.x*p1.x + p1.z*p1.z));
		p2.z = 0;
		return p2;
	}
#pragma endregion
#include "MeshPrimitives.h"
#include "Sound2.h"
#include <XInput.h>
#pragma comment(lib, "Xinput.lib")

#define GObj GameObject*Object=(GameObject*)Data;

#define MB_LEFT   0
#define MB_RIGHT  1
#define MB_MIDDLE 2

bool TPS = false;
MPrimitives *Oxigen, *Life, *Break, *Fail, *AttackCap, *HealthCap;
MPrimitives *laser;
MPrimitives *terreno[2];
D3DXVECTOR2 camAccel = { 0, 0 };
FMOD_VECTOR buff[3];


float _oxygen = 100;
float _health = 100;
float _FOV = 0;

SoundDevice sndDev = SoundDevice(64);//Crea el contexto de OpenAL con 32 canales
SoundResource *sndSrc[10];//Crea los recursos de audio


int objectNumber = 0;





struct GameDescription{
	HWND hWnd = NULL;
	ID3D11Device *d3d11Device;
	ID3D11DeviceContext *d3d11DevCon;
	
	
}GameDesc;

DIMOUSESTATE mouseCurrState;
XINPUT_STATE state;
D3DXVECTOR2 Stick[2];
float		Trigger[2];

BYTE keyboardState[256];
Camara *camara;

class Behaviour;
class GameObject;

class Behaviour {
public:
	float pirsuing = 0;
	float health=100;
	int Data;
	int id = -1;	//identifica el tipo de objeto (rogue, pride, laser)
	GameObject* Container = NULL;
	D3DXVECTOR3 Pos = { 0,0,0 };
	D3DXVECTOR3 Rot = { 0,0,0 };
	D3DXVECTOR3 Scale = { .5,.5,.5 };

	MPrimitives *Model;
	D3DXMATRIX World;
	bool visible = true;
	double distance = 0;

	Behaviour() {}
	Behaviour(float X, float Y, float Z) {
		Pos.x = X;
		Pos.y = Y;
		Pos.z = Z;
	}
	Behaviour(D3DXVECTOR3 Pos) {
		this->Pos = Pos;
	}
	~Behaviour() {}

	virtual void step() {}
	virtual void begstep() {}
	virtual void endstep() {}

	virtual void draw() {
		if (Model == NULL) return;
		D3DXMATRIX Tr;
		D3DXMatrixRotationYawPitchRoll(&Tr, Rot.x, Rot.y, Rot.z);
		D3DXMatrixScaling(&Model->World, Scale.x, Scale.y, Scale.z);
		Model->World *= Tr;
		D3DXMatrixTranslation(&Tr, Pos.x, Pos.y, Pos.z);
		Model->World *= Tr;
		Model->DrawScene(camara);
	}
};

struct GameObject {
	bool killMe = false;
	Behaviour *Logic;
	GameObject *next = NULL, *prev;

	GameObject(MPrimitives *Model, Behaviour *Logic, D3DXVECTOR3 Pos, GameObject * prev) {
		if (prev == NULL)
			this->prev = this;
		else
			this->prev = prev;
		this->Logic = Logic;
		if (Logic != NULL) {
			this->Logic->Container = this;
			this->Logic->Data = (int)this;
			this->Logic->Model = Model;
			this->Logic->Pos = Pos;
		}
	}
	void Add(MPrimitives *_Model, Behaviour *Logic, D3DXVECTOR3 _Pos) {
		GameObject *nobj = new GameObject(_Model, Logic, _Pos, this);
		if (next != NULL)
			next->prev = nobj;
		nobj->next = next;
		next = nobj;
	}
	void Destroy() {
		if (Logic != NULL) delete Logic;
			Logic = NULL;
			killMe = true;
	};
}*Objects = NULL, *GUI = NULL;

#pragma region OBJETOS
class oLaser : public Behaviour{
public:
	float speed = 4.0;
	bool init = false;
	float vanish = 10;
	float mScale = 10;
	oLaser(){
		Scale *= 2;
	}
	void step() {
		auto AUX = Objects;
		while (AUX != NULL)
		{
			if (AUX->Logic != NULL){
				if (id == 'G') { //GOOD
					if (AUX->Logic->id == 'R')	//Rogue
						if (distance3D(Pos, AUX->Logic->Pos) <= 3) {
							sndDev.playSound(sndSrc[1], &AUX->Logic->Pos);
							AUX->Logic->health -= rand() % 15 + 10; //de 10 a 20 puntos de vida
							AUX->Logic->pirsuing += 5;	//lo hace enojar
							return Container->Destroy();
						}
				}
				else
					if (AUX->Logic->id == 'P')	//Pride
						if (distance3D(Pos, AUX->Logic->Pos) <= 3) {
							sndDev.playSound(sndSrc[1], &AUX->Logic->Pos);

							AUX->Logic->health -= rand() % 4 + 2; //de 10 a 20 puntos de vida
							return Container->Destroy();
						}

			}
			AUX = AUX->next;
		}
		Scale.z += Scale.z*(Scale.z < 20) * .15;

		Pos += getFoward(Rot, { 0,0,1 })* speed;
		Rot.z += .1;

		vanish -= .1;
		if (vanish < 0)
		{
			Scale.x *= .9;
			Scale.y *= .9;
			if (Scale.x < 0.001)
				return Container->Destroy();

		}
	}
};

class oWsPride: public Behaviour{
public:
	bool done = false;
	D3DXVECTOR2 Accel = {0,2};
	float canShoot = 0;
	bool ShootSide = false;
	float Delay = .5;
	float rechargeSpeed = .7;
	float rotAcc = 0;
	float breakAngle = 0;

	oWsPride() {
		id = 'P';
		Scale *= .75;
		sndDev.createSound(sndSrc[0], "./Audio/ambispace.ogg", true, false);
		sndDev.createSound(sndSrc[1], "./Audio/Magic_Missile.ogg" , false, true);
		sndDev.createSound(sndSrc[2], "./Audio/Flash.wav", false, false);
		sndDev.createSound(sndSrc[3], "./Audio/Egloves.ogg", false, false);
		sndDev.createSound(sndSrc[4], "./Audio/WEEOO1.ogg", false, true);
		sndDev.createSound(sndSrc[5], "./Audio/disable_tech.ogg", false, true);
		sndDev.createSound(sndSrc[6], "./Audio/CastSummon.ogg", false, true);
		sndDev.createSound(sndSrc[7], "./Audio/StaffChaos.ogg", false, true);
		sndDev.createSound(sndSrc[8], "./Audio/Alarm.ogg", false, false);
		sndDev.playBackMusic(sndSrc[0], .5);
		//sndSrc[0]->setMusicChannelVolume(32,50);
	}
	void step() {
		if (_oxygen <= 0) {
			_oxygen = 0;
			health-=.33;
		}
		_health = health;


		if (health <= 0) {
			sndDev.playSound(sndSrc[8], NULL);

			return;
		}

		if (!done) {
			camara->Yaw = 45;
			camara->Pitch = 25;
			done = true;
		}

		if ((keyboardState[DIK_SPACE] || (state.Gamepad.wButtons & XINPUT_GAMEPAD_X)) && breakAngle < -30) {
 			breakAngle = 5;
			sndDev.playSound(sndSrc[1], NULL);
		}

		camara->Pitch -= ((float)(breakAngle > 0) / 5) * D3DXToRadian(90);


		float sprint = 1+(keyboardState[DIK_LSHIFT] || (Trigger[0] > .2)) * 1;

		Accel.x += max(min((float)(keyboardState[DIK_D] - keyboardState[DIK_A]) / 128 + Stick[0].x, 1.0), -1.0)*.08;
		Accel.y += max(min((float)(keyboardState[DIK_W] - keyboardState[DIK_S]) / 128 + Stick[0].y + (sprint > 1), 1.0), -1.0)*.1 *sprint + (breakAngle == 0) * 5;

		if (keyboardState[DIK_RETURN]) {
			char buff[20];
			itoa(objectNumber,buff,10);
			MessageBoxA(0, buff, "Total Objects", 0);
		}


		rotAcc -= camAccel.x*.3;
		camara->Roll = rotAcc * 3;

		camara->setFov(70 + max(min(Accel.y*5, 40),0));
		_FOV = 70 + max(min(Accel.y * 5, 40), 0);
		camara->UpdateCameraTPS(Accel.y + .1,Accel.x, 0, 0);
		camara->Position.y = max(camara->Position.y, terreno[0]->getTerrainCollision(Pos, { 2, 4,2 }));
		camara->Position.y = min(camara->Position.y, 5000+terreno[1]->getTerrainCollision(Pos, { 2, -4,2 }));
		camara->UpdateCameraTPS(0, 0, 0, 0);
		Accel *= .95;

		Delay += (-.05+(TPS||sprint)*.1);
		if (Delay > 1)
			Delay = 1;
		else
			if (Delay < .5)
				Delay = .5;

		Pos += (camara->Position-Pos)*min(Delay,1);
		Rot += (D3DXVECTOR3(camara->Yaw, camara->Pitch, camara->Roll + rotAcc)-Rot)*.8;
		rotAcc *= .95;
		breakAngle--;

		canShoot += rechargeSpeed;

		if ((mouseCurrState.rgbButtons[MB_LEFT] || (Trigger[1]>.2)) && (canShoot >= 1)) {
			canShoot = 0;
			Behaviour *nobj = new oLaser();
			Objects->Add(laser, nobj, Pos);
			nobj->Rot = Rot;
			nobj->id = 'G';
			sndDev.playSound(sndSrc[2], &camara->Position);
			//sndDev.playSound(sndSrc[3], &camara->Position);

			nobj->Pos.x += sin(Rot.x + D3DXToRadian(-90 + 180 * ShootSide))*1.5;
			nobj->Pos.z += cos(Rot.x + D3DXToRadian(-90 + 180 * ShootSide))*1.5;

			nobj->Pos.x += cos(Rot.y + D3DXToRadian(20)) * sin(Rot.x);
			nobj->Pos.y -= sin(Rot.y + D3DXToRadian(20));
			nobj->Pos.z += cos(Rot.y + D3DXToRadian(20)) * cos(Rot.x);
			ShootSide = !ShootSide;
		}
		GameObject *AUX = Objects;
		while (AUX != NULL) {
			if (AUX->Logic != NULL)
				if (AUX->Logic->id == 'H') { //Health capsule
					if ((distance3D(Pos, AUX->Logic->Pos) < 10) && (health < 100))
					{
						health += 25;
						if (health > 100)
							health = 100;
						AUX->Destroy();
					}
				}
			AUX = AUX->next;
		}

	}
};
class oCapsule : public Behaviour {
	void step(){
		if (id != 'H' && id != 'A')
			id = 'H';

		if (id == 'H')
			Model = HealthCap;
		else
			Model = AttackCap;
		Scale.z = 1.5;
		Rot.x += .1;
		Rot.y = M_PI*.75;
	}
};

class oRogueSentinel: public Behaviour{
public:
	float pRotz = 0;
	float rot = 0;
	float changeTime = 0;
	float hoverEv = 0, hoverSnake = 0;
	float acc = 0, sideAcc = 0;
	float canShoot = 0;

	D3DXVECTOR3 randMove;
	oRogueSentinel(){
		id = 'R';
	}
	void step() {
		pRotz = Rot.z;
		if (health <= 0) {
			sndDev.playSound(sndSrc[7], &Pos);
			Behaviour *nobj = new oCapsule();
			Objects->Add(HealthCap, nobj, Pos);
			if (rand()%5)
				nobj->id = 'H';
			else
				nobj->id = 'A';
			Container->Destroy();
			return;
		}

		bool isPirsuit = (pirsuing > 0);
		pirsuing = max((distance < 150)*5, pirsuing);

		if (isPirsuit){
			D3DXVECTOR3 playerDir = angle3D(Pos, camara->Position);
			Rot.x -= angleDifference(Rot.x, playerDir.x)*.3;
			Rot.y -= angleDifference(Rot.y, playerDir.y)*.3;

			if (distance < 25.0f) {	//Esta muy cercas
				canShoot += .025;
				sideAcc *= .8;
				acc = min(acc + .1, 1);
			}
			else
				if (distance > 60.0f) { //Esta muy lejos
					canShoot += .075;
					sideAcc *= .8;
					acc = max(acc - .1, -1.5);
					if (!isPirsuit)
						sndDev.playSound(sndSrc[4], &Pos);
					hoverSnake = fmodf(hoverSnake + .16, 360);
				}
				else { //Esta muy comodo
					canShoot += .125;


					acc *= .9;
					sideAcc = min(sideAcc +.05, 1);
					if (changeTime <= .1) {
						changeTime = max(rand() % 10,.5);
						randMove = D3DXVECTOR3(1 - rand() % 3, (1 - rand() % 3)*.1, 0);
						if (randMove.x == 0)
							changeTime = .5;
						sideAcc = 0;
					}
					changeTime*=.94;

					Pos += getFowardSpd(Rot, randMove)*sideAcc*max(changeTime,1)*.7;
				}
				isPirsuit = true;
			//Dispara
			if (canShoot >= 1) {
				canShoot = 0;
				Behaviour *nobj = new oLaser();
				Objects->Add(laser, nobj, Pos);
				if (rand()%3 == 0) //33% probable de disparar bien
					nobj->Rot = playerDir + D3DXVECTOR3(0, M_PI, 0);
				else
					nobj->Rot = Rot + D3DXVECTOR3(0, M_PI, 0);

				nobj->id = 'B';
				sndDev.playSound(sndSrc[6], &Pos);
			}
			Pos.y = max(camara->Position.y, terreno[0]->getTerrainCollision(Pos, { 2, 4,2 }));
			Pos.y = min(camara->Position.y, 5000 + terreno[1]->getTerrainCollision(Pos, { 2, -4,2 }));
		}
		else {
			isPirsuit = false;
			acc *= .8;
		}
		hoverEv = fmodf(hoverEv + .16, 360);
		Pos.y += sin(hoverEv)*.1;
		Pos += getFowardSpd(Rot, { sin(hoverSnake)*abs(acc),0, acc });
		pirsuing -= .016;
		Rot.z = pRotz;

	}
};

class oCitizenSquad: public Behaviour{
	oCitizenSquad(){}
	~oCitizenSquad(){}
	void step(){
	}
	void begstep(){

	}
	void endstep(){

	}
	void draw(){

	}
};

#pragma endregion


void LoopObjects(){
	GameObject *AUX = Objects;
	D3DXVECTOR3 S, Pos;
	D3DXQUATERNION R;
	objectNumber = 0;

	//Distancias
	AUX = Objects;
	while (AUX != NULL)
	{
		if(AUX->Logic != NULL)
			AUX->Logic->distance = distance3D(AUX->Logic->Pos, camara->Position);
		AUX = AUX->next;
	}

	AUX = Objects;
	//Background Z-Order for blending
	while (AUX != NULL)
	{
		GameObject *nAUX = AUX->next;
		if (AUX->Logic == NULL) {
			AUX = nAUX;
			continue;
		}

		if (nAUX != NULL && nAUX->Logic!= NULL)
			if (AUX->Logic->distance < nAUX->Logic->distance)
			{
				GameObject* nNext = nAUX->next, *sPrev = AUX->prev;

				nAUX->prev = sPrev;
				nAUX->next = AUX;

				sPrev->next = nAUX;
				if(nNext != NULL)
					nNext->prev = AUX;

				AUX->next = nNext;
				AUX->prev = nAUX;

				if (AUX == Objects) {
					Objects		= nAUX;
					nAUX->prev  = nAUX;
				}
			}
		objectNumber++;
		AUX = nAUX;
	}

	//BegStep
	AUX = Objects;
	while (AUX != NULL)
	{
		if (AUX->Logic != NULL)
			AUX->Logic->begstep();
		AUX = AUX->next;
	}

	//Step
	AUX = Objects;
	while (AUX != NULL)
	{
		if (AUX->Logic != NULL && !AUX->killMe)
			AUX->Logic->step();
		AUX = AUX->next;
	}

	//EndStep
	AUX = Objects;
	while (AUX != NULL)
	{
		if (AUX->Logic != NULL && !AUX->killMe)
			AUX->Logic->endstep();
		AUX = AUX->next;
	}

	//Draw
	AUX = Objects;
	while (AUX != NULL)
	{
		if (AUX->Logic != NULL && !AUX->killMe)
			AUX->Logic->draw();
		AUX = AUX->next;
	}
	//Kill
	AUX = Objects;
	while (AUX != NULL)
	{
		auto nAUX = AUX->next;
		if (AUX->killMe)
			if (AUX->prev != AUX) {
				AUX->prev->next = AUX->next;
				if (AUX->next!=NULL)
					AUX->next->prev = AUX->prev;
			}
			else
			if (AUX->next != NULL) {
				AUX->next->prev = AUX->next;
				delete Objects;
				Objects = nAUX;
			}
		AUX = nAUX;
	}
	sndDev.updateListener(camara->tpsPosition, camara->updateFoward(), 60);
}
