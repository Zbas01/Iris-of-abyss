#include <Windows.h>
#include <fmod.hpp>

#pragma comment(lib, "fmod_vc.lib" )

using namespace FMOD;

FMOD_VECTOR *ToFMOD(FMOD_VECTOR *Dest, D3DXVECTOR3 Src){
	Dest->x = Src.x;
	Dest->y = Src.y;
	Dest->z = Src.z;
	return Dest;
}
FMOD_VECTOR *ToFMOD(FMOD_VECTOR *Dest, D3DXVECTOR3 *Src){
	if (Src == NULL)
	{
		ZeroMemory(Dest, sizeof(FMOD_VECTOR));
		return Dest;
	}
	Dest->x = Src->x;
	Dest->y = Src->y;
	Dest->z = Src->z;
	return Dest;
}
FMOD_VECTOR *ToFMOD(FMOD_VECTOR *Dest, D3DXVECTOR2 Src){
	Dest->x = Src.x;
	Dest->y = Src.y;
	return Dest;
}
FMOD_VECTOR *ToFMOD(FMOD_VECTOR *Dest, float* Src){
	if (Src == NULL)
	{
		ZeroMemory(Dest, sizeof(FMOD_VECTOR));
		return Dest;
	}
	Dest->x = Src[0];
	Dest->y = Src[1];
	Dest->z = Src[2];
	return Dest;
}
FMOD_VECTOR *ToFMOD(FMOD_VECTOR *Dest, float X, float Y, float Z){
	Dest->x = X;
	Dest->y = Y;
	Dest->z = Z;
	return Dest;
}
FMOD_VECTOR *getVelocity(FMOD_VECTOR *Dest, D3DXVECTOR3 vPrev, D3DXVECTOR3 vNow, float fps){
	return ToFMOD(Dest,(vNow - vPrev) / (1/fps));
}

typedef FMOD::Sound SoundResource;

class SoundDevice{
	Channel		**channelList;
	D3DXVECTOR3  **Position; //Ahora
	D3DXVECTOR3   *PositionB;//Antes
	FMOD_VECTOR   *Velocity;
	int channels = 0;
	int index = 0;
public:
	System      *system;
	SoundDevice(int channels){
		System_Create(&system);
		system->init(channels, FMOD_INIT_NORMAL, 0);
		channelList = new Channel*	  [channels + 1]; // canales + back
		Velocity	= new FMOD_VECTOR [channels + 1]; // canales + listener
		Position    = new D3DXVECTOR3*[channels];
		PositionB   = new D3DXVECTOR3 [channels + 1]; // canales + listener

		ZeroMemory(Velocity , sizeof(FMOD_VECTOR )*channels + 1);
		ZeroMemory(Position , sizeof(D3DXVECTOR3*)*channels);
		ZeroMemory(PositionB, sizeof(D3DXVECTOR3 )*channels + 1);

		FMOD_VECTOR Up;
		system->set3DSettings(0, .25, .20);
		system->set3DNumListeners(1);
		system->set3DListenerAttributes(0, NULL, NULL, NULL, ToFMOD(&Up, 0, 1, 0));
		this->channels = channels;
	}

	void playBackMusic(Sound *sound, float volume){
		channelList[channels]->setPosition(0, FMOD_TIMEUNIT_PCM);
		channelList[channels]->setPaused(false);
		system->playSound(sound, NULL, false, &channelList[channels]);
		channelList[channels]->setVolume(volume);
	}
	void playSound(Sound *sound, D3DXVECTOR3 *Pos){
		FMOD_VECTOR _Pos, _Vel;
		Position [index] =  Pos;
		if (Pos != NULL)
			PositionB[index] = *Pos;
		ZeroMemory(&Velocity[index], sizeof(FMOD_VECTOR));
		
		channelList[index]->setPaused(false);
		system->playSound(sound, NULL, false, &channelList[index++]);
		if (index >= channels)
			index = channels;
	}

	SoundResource* createSound(SoundResource *&Dest, char* file, bool loop, bool _3D){
		if (_3D)
			system->createSound(file, FMOD_3D, NULL, &Dest);
		else
			system->createSound(file, FMOD_DEFAULT, NULL, &Dest);
		if (loop){
			Dest->setMode(FMOD_LOOP_NORMAL);
			Dest->setLoopCount(-1);
		}
		else
			Dest->setMode(FMOD_LOOP_OFF);
		return Dest;
	}

	void updateListener(D3DXVECTOR3 Pos, D3DXVECTOR3 Fwd, int fps){
		bool playing = false;
		FMOD_VECTOR _Pos, _Fwd, _Vel;
		//Listener
		getVelocity(&Velocity[channels], PositionB[channels], Pos, fps);
		PositionB[channels] = Pos;
		system->set3DListenerAttributes(0, ToFMOD(&_Pos, Pos), ToFMOD(&_Vel,{0,0,0}), ToFMOD(&_Fwd, Fwd), NULL);

		for (int i = 0; i < channels; i++){
			if (*Position[i] != NULL){
				//Source
				
				channelList[index]->set3DSpread(180);
				channelList[index]->set3DDopplerLevel(0);
				//getVelocity(&Velocity[i], PositionB[i], *Position[i], fps);
				PositionB[i] = *Position[i];
				channelList[i]->set3DAttributes(ToFMOD(&_Pos, *Position[i]), ToFMOD(&_Vel, { 0, 0, 0 }), NULL);
			}
		}
		system->update();
	}
};