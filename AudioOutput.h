#pragma once

#include "BipBuffer.h"
#include "AutoLocker.h"
#include "SDL2/include/SDL.h"
#pragma comment(lib,"SDL2/lib/SDL2.lib")

class AudioOutput:public BipBuffer
{
public:
	AudioOutput(void);
	virtual ~AudioOutput(void);
	virtual int Open(uint32_t sample_nb,uint32_t sample_rate,uint32_t channels);
	virtual int InputData(uint8_t* data,int size);
	static void __cdecl audio_callback(void *udata,Uint8 *stream,int len)
	{
		int  n = 0;
		int& size = n;
		uint8_t *data = NULL;
		AudioOutput *audiodata = (AudioOutput*)udata;
		SDL_memset(stream, 0, len); 
		{
			CAutoLocker m_autoLocker(&audiodata->Audio_locker);
			data = audiodata->GetContiguousBlock(size);
			if(!data)
				return;
			if(size<len)
				len = size;
			SDL_MixAudio(stream,data,len,SDL_MIX_MAXVOLUME);
			audiodata->DecommitBlock(len);
		}
	}
private:
	SDL_AudioSpec       wanted_spec;
	CLocker	            Audio_locker;
	int                 DevideOpened;
};

