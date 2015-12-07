#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#pragma pack(1)


typedef struct TelegramFrame{
	unsigned char* data;
	unsigned int   size;
	unsigned char  arrived;
	unsigned char  used;

}TelegramFrame;

typedef struct FrameHeader{
	unsigned int timestamp;
	unsigned int length;
	unsigned char num;
	unsigned char totalNum;
	unsigned char type;
}FrameHeader;

typedef struct TelegramLink{
	unsigned int timestamp;
	unsigned int totalNum;
	unsigned int arrivedNum;
	TelegramFrame *f;
	unsigned int MaxFrameNumber;
	unsigned char* data;
	unsigned char* w_prt;
	unsigned int capatialty;
	unsigned int size;
	unsigned char type;
}TelegramLink;

#pragma pack()

class Telegram
{
public:
	Telegram(){}
	virtual ~Telegram()
	{
		if(link.f)
			free(link.f);
	}
	virtual int initbuf(int MaxMsgLen,int MaxFrameNumber){
		memset(&link,0x00,sizeof(TelegramLink));
		link.f = (TelegramFrame*)malloc(sizeof(TelegramFrame)*MaxFrameNumber);
		link.MaxFrameNumber = MaxFrameNumber;
		link.capatialty = MaxMsgLen;
		link.data = (unsigned char*)malloc(MaxMsgLen);
		link.w_prt = link.data;
		return 0;
	}
	virtual int onGram(FrameHeader *h,unsigned char* data,int size)
	{
//		_cprintf("Recv udp frame timestamp = %9d,total = %03d,number = %03d,type = %d\n",h->timestamp,h->totalNum,h->num,h->type);
		if(link.totalNum==0){
			link.totalNum = h->totalNum;
			link.timestamp = h->timestamp;
		}else if(h->timestamp!=link.timestamp){
			reset();
			link.totalNum = h->totalNum;
			link.timestamp = h->timestamp;
			memcpy(link.w_prt,data,size);
			link.f[h->num].data = link.w_prt;
			link.f[h->num].arrived = 1;
			link.f[h->num].size = size;
			link.arrivedNum ++;
			link.size += size;
			link.w_prt += size;
			if(link.arrivedNum==link.totalNum){
				//_cprintf("get total time = %d current = %d\n",h->timestamp,GetTickCount());
				link.type = h->type;
				return 1;
			}
			return 0;
		}
		memcpy(link.w_prt,data,size);
		link.f[h->num].data = link.w_prt;
		link.f[h->num].arrived = 1;
		link.f[h->num].size = size;
		link.arrivedNum ++;
		link.size += size;
		link.w_prt += size;
		if(link.arrivedNum==link.totalNum){
			//_cprintf("get total time = %d current = %d\n",h->timestamp,GetTickCount());
			link.type = h->type;
			return 1;
		}
		return 0;
	}
	virtual int GetLastMessage(unsigned char* data,int size,int *type)
	{
		unsigned char* d = data;
		int          get = 0;
		if(link.arrivedNum!=link.totalNum)
			return -1;
		for(unsigned int i = 0;i<link.totalNum;i++){
			memcpy(d,link.f[i].data,link.f[i].size);
			d+=link.f[i].size;
			get += link.f[i].size;
		}
		*type = (int)link.type;
		reset();
		return get;
	}
	virtual void reset()
	{
		link.arrivedNum = 0;
		link.size = 0;
		link.timestamp = 0;
		link.totalNum = 0;
		link.w_prt = link.data;
		memset(link.f,0x0,sizeof(TelegramFrame)*link.MaxFrameNumber);
	}

	virtual int setTimeoutTime(int t){
		return 0;
	}
protected:

private:
	TelegramLink link;
};
