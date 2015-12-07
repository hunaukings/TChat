#include "AudioOutput.h"


AudioOutput::AudioOutput(void)
	:DevideOpened(0)
{
}


AudioOutput::~AudioOutput(void)
{
}

int AudioOutput::Open( uint32_t sample_nb,uint32_t sample_rate,uint32_t channels)
{
	wanted_spec.freq = sample_rate;   
	wanted_spec.format = AUDIO_S16SYS;   
	wanted_spec.channels = channels;   
	wanted_spec.silence = 0;   
	wanted_spec.samples = 100;   
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = this;
	if(!DevideOpened){
		wanted_spec.freq = 8000;   
		wanted_spec.format = AUDIO_S16SYS;   
		wanted_spec.channels = 1;   
		wanted_spec.silence = 0;   
		wanted_spec.samples = 100;   
		wanted_spec.callback = audio_callback;
		wanted_spec.userdata = this;

		if (SDL_OpenAudio(&wanted_spec, NULL)<0){   
			printf("can't open audio.\n");     
		}else{
			DevideOpened = 1;
		}  
	}
}
int AudioOutput::InputData( uint8_t* data,int size )
{
	int capacity = 0;
	int& rca = capacity;
	{
		CAutoLocker m_autoLocker(&Audio_locker);
		uint8_t * wp = Reserve(size,rca);
		if(capacity!=size)
			return;
		memcpy(wp,data,size);
		Commit(size);
	}
}
