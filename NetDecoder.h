#include "Caster.h"
#include "Decoder.h"

class NetDecoder:public Caster ,public Decoder{
public:
	NetDecoder(){}
	virtual ~NetDecoder(){}
	virtual void FrameArrived(unsigned char* data,int size,int type){
		static AVFrame *pic = NULL;
		if(!pic)
			pic = av_frame_alloc();

		Decode(data,size,type,pic);
	}
};