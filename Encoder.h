#ifndef _ENCODER_
#define _ENCODER_

#include <windows.h>
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;

#include <stdio.h>
extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};

extern "C"{
#include "x264/x264.h"
}
#pragma comment(lib,"x264/libx264.lib")



#define PIXFORMAT_BGR 1
#define PIXFORMAT_I420 2


typedef struct EncoderContext{
	int w,h;
	x264_param_t *param;
	x264_nal_t   *nal;
	x264_picture_t* in_pic;
	x264_t* m_h;
	uint8_t *data[100];
	int      size[100];
	uint32_t     nal_number;
	
}EncoderContext;

typedef struct EncodecOutData{
	uint8_t *data[10];
	uint32_t size[10];
	uint32_t number;
	uint32_t timestamp;
}EncodecOutData;

#define  MAX_BUF_LENGTH 5000000

class Encoder{
public:
	Encoder()
		:ctx(NULL),pktbuf(NULL),outdata(NULL)
	{

	}
	virtual ~Encoder(){
		if(ctx)
			free(ctx);
		if(pktbuf)
			free(pktbuf);
		if(outdata)
			free(outdata);
	}
	virtual int Init(int w,int h)
	{
		EncoderContext *s = (EncoderContext*)malloc(sizeof(EncoderContext));
		memset(s,0x00,sizeof(EncoderContext));
		s->param = (x264_param_t*)malloc(sizeof(x264_param_t));
		s->w = w;
		s->h = h;
		x264_param_default_preset(s->param, "superfast", "zerolatency");

		x264_param_apply_profile(s->param,"high10");
		s->param->i_width = w;
		s->param->i_height = h;		

		s->param->rc.i_bitrate = 2000;
		s->param->rc.i_rc_method = X264_RC_ABR;
		s->param->b_annexb = true;
		s->param->i_bframe = 0;
		s->param->i_keyint_max = 10;
		s->param->i_csp = X264_CSP_I420;
		s->in_pic = (x264_picture_t*)malloc(sizeof(x264_picture_t));
		x264_picture_alloc( s->in_pic, X264_CSP_I420, w, h );
		s->in_pic->i_type = X264_TYPE_AUTO;
		s->nal =(x264_nal_t *)malloc(sizeof(x264_nal_t ));

		memset(s->nal,0x0,sizeof(x264_nal_t)) ;

		if((s->m_h = x264_encoder_open(s->param)) == NULL )
		{
			return -1;
		}
		ctx = s;
		pktbuf = (uint8_t*)malloc(MAX_BUF_LENGTH);
		outdata = (EncodecOutData*)malloc(sizeof(EncodecOutData));
		return 0;
	}
	virtual int Encode(AVPicture* pic,EncodecOutData **out,int timestamp)
	{
		int ret = 0;
		int n_nal; 
		outdata->number = 0;
		x264_picture_t pic_out; 
		ctx->in_pic->i_type = X264_TYPE_AUTO;
		ctx->in_pic->i_qpplus1 = 0;
		ctx->in_pic->img.i_csp = X264_CSP_I420;
		ctx->in_pic->img.plane[0] = pic->data[0];
		ctx->in_pic->img.plane[1] = pic->data[1];
		ctx->in_pic->img.plane[2] = pic->data[2];
		ctx->in_pic->img.i_stride[0] = pic->linesize[0];
		ctx->in_pic->img.i_stride[1] = pic->linesize[1];
		ctx->in_pic->img.i_stride[2] = pic->linesize[2];
		ctx->in_pic->i_pts+=1; 
		int x = GetTickCount();
		if((ret=x264_encoder_encode(ctx->m_h,&ctx->nal,&n_nal,ctx->in_pic,&pic_out))<0){  
			return -1;
		}  
		x264_nal_t *my_nal; 
		unsigned char* b = pktbuf;
		int len = 0;
		int i = 0;
		if(n_nal>0){
			for(my_nal=ctx->nal;my_nal<ctx->nal+n_nal;++my_nal){ 
				outdata->data[i] = b;
				outdata->size[i] = my_nal->i_payload;
				len += my_nal->i_payload;
				if(len > MAX_BUF_LENGTH || i>=100){
//					_cprintf("Encode out data larger than MAX_BUF\n");
					return -1;
				}
				memcpy(b,my_nal->p_payload,my_nal->i_payload);
				b+=my_nal->i_payload;
				i++;
				outdata->number = i;
				outdata->timestamp = (uint32_t)pic_out.i_pts;
				
			}
			x = GetTickCount();
		}
		*out = outdata;
		return i;
	}
	virtual int close(){
		if(ctx){
			if(ctx->m_h)
				x264_encoder_close(ctx->m_h);
            if(ctx->in_pic)
				x264_picture_clean(ctx->in_pic);
		}
		return 0;
	}
private:
	EncoderContext *ctx;
	uint8_t *pktbuf;
	EncodecOutData *outdata;
};

#endif /*_ENCODER_*/