#include <Windows.h>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <al.h>
#include <alc.h>
#pragma comment(lib, "OpenAL32.lib")
using namespace std;

struct SoundResource {
	unsigned char* data = NULL;
	char type[4][4];
	DWORD size, chunkSize;
	short formatType, channels;
	DWORD avgBytesPerSec;
	short bytesPerSample, bitsPerSample;
	DWORD dataSize;
	ALenum format = 0;
	ALuint sampleRate; //Also called 'frequency'

	ALenum getAudioFormat() {
		if (bitsPerSample == 8) {
			if (channels == 1)
				return AL_FORMAT_MONO8;
			else if (channels == 2)
				return AL_FORMAT_STEREO8;
		}
		else if (bitsPerSample == 16)
		{
			if (channels == 1)
				return AL_FORMAT_MONO16;
			else if (channels == 2)
				return AL_FORMAT_STEREO16;
		}
		return 0;
	}

	SoundResource(char* file) {
		FILE *fp = NULL;
		fp = fopen(file, "rb");

		fread(type[0], sizeof(char), 4, fp);
		if (strncmp(type[0], "RIFF", 4) != 0)
			return;

		fread(&size, sizeof(DWORD), 1, fp);
		fread(type[1], sizeof(char), 4, fp);
		if (strncmp(type[1], "WAVE", 4) != 0)
			return;

		fread(type[2], sizeof(char), 4, fp);
		if (strncmp(type[2], "fmt ", 4) != 0)
			return;

		fread(&chunkSize, sizeof(DWORD), 1, fp);
		fread(&formatType, sizeof(short), 1, fp);
		fread(&channels, sizeof(short), 1, fp);
		fread(&sampleRate, sizeof(DWORD), 1, fp);
		fread(&avgBytesPerSec, sizeof(DWORD), 1, fp);
		fread(&bytesPerSample, sizeof(short), 1, fp);
		fread(&bitsPerSample, sizeof(short), 1, fp);
		fread(type[3], sizeof(char), 4, fp);

		while (strncmp(type[3], "data", 4) != 0) {
			type[3][0] = type[3][1];
			type[3][1] = type[3][2];
			type[3][2] = type[3][3];
			fread(&type[3][3], sizeof(char), 1, fp);
		}

		fread(&dataSize, sizeof(DWORD), 1, fp);

		data = new unsigned char[dataSize];
		fread(data, sizeof(BYTE), dataSize, fp);

		format = getAudioFormat();
	}
	~SoundResource() {
		if (data != NULL)
			delete[] data;
	}
};

struct ALDevice {
private:
	ALuint channelNumber;
	ALuint index = 0;
	ALuint *source;
	ALuint *buffer;

	void setPlay(SoundResource *sound, ALfloat Pitch, ALfloat Gain, ALenum loop) {
		alSourceStop(source[index]);
		alSourceUnqueueBuffers(source[index], 1, &buffer[index]);
		alBufferData(buffer[index], sound->format, sound->data, sound->dataSize, sound->sampleRate);
		alSourcei(source[index], AL_BUFFER, buffer[index]);
		alSourcei(source[index], AL_LOOPING, loop);
		alSourcef	(source[index], AL_PITCH, Pitch);
		alSourcef	(source[index], AL_GAIN, Gain);
		alSourcePlay(source[index]);
	}
public:
	bool Good = false;
	ALCdevice *soundDev;
	ALCcontext * soundCon;

	ALDevice(ALuint channels) {
		soundDev = alcOpenDevice(NULL);
		if (!soundDev)
			return;

		soundCon = alcCreateContext(soundDev, NULL);
		alcMakeContextCurrent(soundCon);
		if (!soundCon)
			return;

		channelNumber = channels;
		buffer = new ALuint[channels + 1];
		source = new ALuint[channels + 1];
		alGenBuffers(channels + 1, buffer);
		alGenSources(channels + 1, source);
		Good = true;
		alListeneri(AL_CHANNELS, 2);
	}
	int playBackMusic(SoundResource *sound, ALfloat Pitch, ALfloat Gain) {
		if (!Good) return -1;
		auto tmpindex = index;

		index = channelNumber;
		setPlay(sound, Pitch, Gain, AL_TRUE);
		index = tmpindex;

		return channelNumber;
	}
	int playSound(SoundResource *sound, ALfloat Pitch, ALfloat Gain) {
		if (!Good) return -1;
		if (index >= channelNumber - 1)//-1 de el backmusic
			index = 0;
		setPlay(sound, Pitch, Gain, AL_FALSE);
		return index++;
	}
};