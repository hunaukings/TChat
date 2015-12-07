#include "CameraDS.h"

extern "C"
{
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
//#include "libavdevice/avdevice.h"
};

class CameraYUV:public CCameraDS
{
public:
	CameraYUV()
		:pFrame(NULL),pFrameYUV(NULL),img_convert_ctx(NULL),tmp(NULL),opened(0)
	{

	}
	virtual ~CameraYUV()
	{
		if(pFrameYUV)
			av_frame_free(&pFrameYUV);
		if(pFrame)
			av_frame_free(&pFrame);
		if(tmp)
			free(tmp);
	}
	virtual int OpenCamera(int *w,int *h)
	{
		if(!CCameraDS::OpenCamera(1,w,h,true))
			return -1;
		width = *w;
		height = *h;
		pFrame=av_frame_alloc();
		pFrameYUV=av_frame_alloc();
		uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, width, height));
		tmp = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_BGR24, width, height));
		avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, width, height);
		img_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_BGR24, width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
		if(!img_convert_ctx)
			return -1;
		opened = 1;
		return 0;
	}
	virtual AVFrame *GetFrame()
	{
		Img *img = NULL;
		uint8_t *dst = NULL;
		uint8_t *src = NULL;
		if(!opened)
			return NULL;
		pFrame->format = AV_PIX_FMT_RGB24;
	
		img = QueryFrame();
		if(!img)
			return NULL;
		int linesize = width*3;
		pFrame->data[0] = img->data;
		pFrame->linesize[0] = width*3;
		dst = tmp;
		src = img->data + linesize*(height-1);
		for(int i = 0;i<height;i++){
			memcpy(dst,src,linesize);
			dst+=linesize;
			src-=linesize;
		}
		pFrame->data[0] = tmp;
		pFrame->linesize[0] = linesize;
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, height, pFrameYUV->data, pFrameYUV->linesize);	
		return pFrameYUV;
	}
	virtual void Close(){
		CloseCamera();
		if(img_convert_ctx)
			sws_freeContext(img_convert_ctx);
	}
protected:
private:
	struct SwsContext *img_convert_ctx;
	AVFrame *pFrame,*pFrameYUV;
	int width,height;
	uint8_t *tmp;
	int  opened;
};