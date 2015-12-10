#ifndef _DECODE_H_
#define _DECODE_H_




#include <stdio.h>
//#include <conio.h>
extern "C"{
#include "libavcodec/avcodec.h"
};
#pragma comment(lib,"lib/avcodec.lib")
#pragma comment(lib,"lib/avutil.lib")


class Decoder{
public:
	Decoder()
		:videocodecctx(NULL),audiocodecctx(NULL),opened(0),swr_ctx(NULL),dst_data(0),dst_size(0)
	{
		f = NULL;
	}
	virtual ~Decoder(){}
	virtual int open()
	{
		int ret = 0;
		AVCodec *videocodec;

		AVCodec *audiocodec;

		avcodec_register_all();

		videocodec = avcodec_find_decoder(AV_CODEC_ID_H264);
		audiocodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
		if(!videocodec || !audiocodec) {
		//	_cprintf("can not find decoder (%s)\n");
			return -1;
		}
		videocodecctx = avcodec_alloc_context3(videocodec);
		audiocodecctx = avcodec_alloc_context3(audiocodec);

		if(!videocodecctx || !audiocodec)
			return -1;
		ret = avcodec_open2(videocodecctx,videocodec,NULL);
		if(ret<0){
		//	_cprintf("can not open videocodec\n");
			return -1;
		}
		ret = avcodec_open2(audiocodecctx,audiocodec,NULL);
		if(ret<0){
		//	_cprintf("can not open audiocodec\n");
			return -1;
		}
		opened = 1;
		return 0;
	}
	virtual int Decode(unsigned char* data,int size,int type,AVFrame *p)
	{
		if(!opened)
			return -1;
		AVPacket pkt;
		memset(&pkt,0x0,sizeof(pkt));
		pkt.data = data;
		pkt.size = size;
		int get_frame = 0;
		int get_audioframe = 0;
		int l = 0;
		int ret = 0;
#if 1
		
		if(!f){
			f = fopen("123.h264","ab+");
			
			if(!f){
				int x = GetLastError();
				perror("open");
				return 0;
			}
		}
		
		fwrite(data,size,1,f);
		fflush(f);
#endif 



		if(type ==1){
			AVFrame f= {0};
			f.data[0] = data;
			f.linesize[0] = size;
			RefreshAudio(&f);
		}else if(type == 0){
			l = avcodec_decode_video2(videocodecctx, p, &get_frame, &pkt);
		}
		if(l<0)
			;
		//	_cprintf("decode err\n");
		if(get_frame){
			RefreshVideo(p);
			return 1;
		}		
		return 0;
	}
	virtual void RefreshVideo(AVFrame *p){
		return;
	}
	virtual void RefreshAudio(AVFrame *p){
		return ;
	}
	virtual int close(){return 0;}
private:
	int                 opened;
	AVCodecContext		*videocodecctx;
	AVCodecContext      *audiocodecctx;
	AVFrame				*video_frame;
	AVFrame             *audio_frame;
	AVPicture           *YUVFrame;
	struct SwrContext   *swr_ctx;
	AVPicture           *SRCPicture;
	uint8_t             *dst_data;
	int					dst_size;
    FILE               *f ;
};

#endif /*_DECODE_H_*/