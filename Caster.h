
#pragma once

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN

#endif 

#include "event.h"
#include <vector>

//#include "AutoLocker.h"
#include "Telegram.h"
#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

//#include "SDL_thread.h"
//#include "SDL_mutex.h"
#include "OSConfig.h"
using std::vector;


#pragma pack(1)

typedef struct ClientPoint{
	unsigned int ip;
	unsigned int lastttime;
	unsigned int lasttimeofchattick;
	unsigned int chat;
}ClientPoint;

#pragma pack()


#define HEARTBEATTAG  0
#define CONNETREQUEST 1
#define DATAPACKET    2



#define CastPort 3779
#define DataPort 3780

static struct event_base *base = NULL;

#define MAX_MTU 500

class Caster:public Telegram
{
public:
	Caster()
	{

	}
	virtual ~Caster()
	{
		
	}
	virtual void StartUp()
	{
		#ifdef WIN32
				WSADATA wsaData;
				WSAStartup(MAKEWORD(2, 2), &wsaData);
		#endif  /*  WIN32  */
				static int event_inited = 0;
				if(!event_inited){
					event_init();
					base = event_base_new();
					event_inited = 1;
				}
	}
	virtual int init(unsigned short port)
	{
		char 				tmp = 1;
		StartUp();

		if(InitCastSocket(3779)<0)
			return -1;
		if(InitDataSocket(3780)<0)
			return -1;

		initbuf(5000000,1000);
		memset(&CurrentRequestClient,0x0,sizeof(ClientPoint));
		memset(&CurrentRequestServer,0x0,sizeof(ClientPoint));
		return 0;		
	}

#ifdef _WIN32
	static void  Process(void *arg)
#else
	static void  *Process(void *arg)
#endif
	{
		Caster *c = (Caster*)arg;
		
		event_base_dispatch(base);
	}
	virtual void Start()
	{
		running = 1;
		StartThread(Process,NULL,&thread);
	}
	virtual void Stop()
	{
		running = 0;
		void* stat;
		//SDL_WaitThread(thread,&stat);
		WaitThread(thread,&stat);
		int x = 0;
	}
	virtual int Error()
	{
		#ifdef WIN32
		int x =  WSAGetLastError();
				return x;
		#else 
				perror("SOCKET");
				return errno;
		#endif
	}

	virtual int Close()
	{
		running = 0;
		timeval t = {1,0};
		void *stat;
		event_base_loopexit(base,&t);
		WaitThread(thread,&stat);
		int err = Error(); 
		event_del(&revent);
		event_del(&clockevent);
		#ifdef WIN32
				closesocket(sendcastfd);
				closesocket(recvcastfd);
				//WSACleanup();
		#else
				::close(sendcastfd);
				::close(recvcastfd);

		#endif
				return errno;
	}
	int InitCastSocket(short port)
	{
		memset(&cast_addr_remote,0x0,sizeof(cast_addr_remote));
		cast_addr_remote.sin_family = AF_INET;
		cast_addr_remote.sin_port = htons(CastPort);
		cast_addr_remote.sin_addr.s_addr = INADDR_BROADCAST;
	
		memset(&cast_addr_local,0x0,sizeof(cast_addr_local));
	    cast_addr_local.sin_family = AF_INET;
		cast_addr_local.sin_port = htons(CastPort);
		cast_addr_local.sin_addr.s_addr = 0;
		sendcastfd = socket(PF_INET, SOCK_DGRAM,IPPROTO_UDP);
		recvcastfd = socket(PF_INET, SOCK_DGRAM,IPPROTO_UDP);
		if(sendcastfd<0||recvcastfd<0)
			return Error();
#ifndef _WIN32
		int flags = fcntl(sendcastfd, F_GETFL, 0);
		fcntl(sendcastfd, F_SETFL, flags | O_NONBLOCK);
		int bOpt = 1;
		if(setsockopt(sendcastfd, SOL_SOCKET, SO_BROADCAST, &bOpt, sizeof(bOpt))){
			Close();
			return Error();
		}
#else
		u_long  ul = 1;
		char bOpt = 1;
		if(ioctlsocket(sendcastfd, FIONBIO, &ul) < 0)
			return -1;
		if(setsockopt(sendcastfd, SOL_SOCKET, SO_BROADCAST, &bOpt, sizeof(bOpt))){
			Close();
			return Error();
		}
#endif


		if (setsockopt(recvcastfd, SOL_SOCKET, SO_REUSEADDR, &bOpt, sizeof(bOpt))){
			Close();
			return Error();
		}

		if(bind(recvcastfd, (sockaddr*)&cast_addr_local, sizeof(sockaddr))<0)
			return Error();

		event_set(&revent,recvcastfd, EV_READ|EV_PERSIST, CastInput, this); 
		event_base_set(base, &revent); 
		event_add(&revent, NULL); 

		struct timeval t = {0, 500000 };
		evtimer_set(&clockevent, TimeTick, this);
		event_base_set(base, &clockevent); 

		event_add(&clockevent, &t); 
		return 0;
	}
	int InitDataSocket(short port)
	{
		memset(&data_addr_local,0x0,sizeof(data_addr_local));
		data_addr_local.sin_family = AF_INET;
		data_addr_local.sin_port = htons(DataPort);
		data_addr_local.sin_addr.s_addr = 0;

		recvdatafd = socket(PF_INET, SOCK_DGRAM,IPPROTO_UDP);
		senddatafd = socket(PF_INET, SOCK_DGRAM,IPPROTO_UDP);
		if(recvdatafd<0 || senddatafd < 0)
			return Error();

#ifndef _WIN32
		int bOpt = 1;
		int flags = fcntl(sendcastfd, F_GETFL, 0);
		fcntl(sendcastfd, F_SETFL, flags | O_NONBLOCK);

		int snd_size = 50*1024;    
		int optlen = sizeof(snd_size); 
		if( setsockopt(sendcastfd, SOL_SOCKET, SO_SNDBUF, &snd_size, optlen)!=0)
			return -1;
#else
		char bOpt = 1;
		u_long  ul = 1;
		if(ioctlsocket(senddatafd, FIONBIO, &ul) < 0)
			return -1;
		int snd_size = 50*1024;    
		int optlen = sizeof(snd_size); 
		if(setsockopt(sendcastfd, SOL_SOCKET, SO_SNDBUF, (const char*)&snd_size, optlen)<0)
			return -1;
#endif

		if(bind(recvdatafd, (sockaddr*)&data_addr_local, sizeof(sockaddr))<0){
			Close();
			return Error();
		}
		
		if(setsockopt(senddatafd, SOL_SOCKET, SO_BROADCAST, &bOpt, sizeof(bOpt))){
			Close();
			return Error();
		}

		event_set(&recvdataevent,recvdatafd, EV_READ|EV_PERSIST, DataInput, this); 
		event_base_set(base, &recvdataevent); 
		event_add(&recvdataevent, NULL); 

	}
	static void TimeTick(const int fd, const short which, void *arg)
	{
		Caster *c = (Caster*)arg;
		char buff[MAX_MTU] = {0};
		int	changed = 0;
		int nSendSize = sendto(c->sendcastfd, buff, MAX_MTU, 0, (sockaddr*)&(c->cast_addr_remote), sizeof(sockaddr));

		struct timeval t = {0, 500000 };
		evtimer_set(&(c->clockevent), TimeTick, arg);
		event_base_set(base, &(c->clockevent));
		evtimer_add(&(c->clockevent), &t);
		vector<ClientPoint>::iterator iter;
		unsigned long  current = GetTickCount();
		{
			//		CAutoLocker m_autoLocker(&c->m_locker);
			for(unsigned int i = 0; i < c->clientlist.size() ;i++){

				if(current - c->clientlist.at(i).lastttime > 3000){
					c->clientlist.erase(c->clientlist.begin()+i);
					//超时3s没有收到心跳，则说明客户端断开了
					changed = 1;
				}

			}
			if(current - c->CurrentRequestClient.lasttimeofchattick > 3000){
				memset(&c->CurrentRequestClient,0x0,sizeof(ClientPoint));
			}
		}
		if(changed)
			c->onClientChange();
		c->SendChatTick();
	}
	static void  CastInput(int serfd, short iEvent, void *arg)
	{
		unsigned char	buf[MAX_MTU] = {0};
		Caster			*c = (Caster*)arg;
		int				nAddrLen;
		int				GotItSelf = 0;
		int				Changed = 0;
		struct			sockaddr_in sin_from; 
		ClientPoint		p = {0,0,0,0};
		int				current = GetTickCount();
	    nAddrLen      =	sizeof(struct sockaddr_in);
		recvfrom(c->recvcastfd, (char*)buf,MAX_MTU, 0, (sockaddr*)&sin_from,&nAddrLen);

		vector<ClientPoint>::iterator iter;
		{
		//	CAutoLocker m_autoLocker(&c->m_locker);
			for(iter = c->clientlist.begin();iter!=c->clientlist.end();iter++){
				if(iter->ip == sin_from.sin_addr.s_addr){
					GotItSelf = 1;
					//在列表中找到客户端,重置最后一次收到心跳包的时间
					iter->lastttime = current;
					break;
				}
			}
			if(!GotItSelf){
				p.ip = sin_from.sin_addr.s_addr;
				p.lastttime = GetTickCount();
				c->AddClientPoint(p);
				Changed = 1;
			    //没有找到客户端，说明是新客户端发送的心跳
			}	
		}
		c->Error();
		if(Changed)
			c->onClientChange();
		
	}
	static void  DataInput(int serfd, short iEvent, void *arg){
		unsigned char	buf[MAX_MTU] = {0};
		Caster			*c = (Caster*)arg;
		int				nAddrLen;
		int				GotItSelf = 0;
		int				Changed = 0;
		struct			sockaddr_in sin_from; 
		ClientPoint		p = {0,0,0,0};
		int				current = GetTickCount();
		nAddrLen      =	sizeof(struct sockaddr_in);
		int recvlen = recvfrom(c->recvdatafd, (char*)buf,MAX_MTU, 0, (sockaddr*)&sin_from,&nAddrLen);
		if(recvlen!=500){
			;
			return ;
		}
		if(buf[0]==CONNETREQUEST){
			if(c->CurrentRequestClient.ip==0){
				c->CurrentRequestClient.ip = sin_from.sin_addr.s_addr;
				c->CurrentRequestClient.lasttimeofchattick = current;
			}
			else{
				if(c->CurrentRequestClient.ip == sin_from.sin_addr.s_addr){
					c->CurrentRequestClient.lasttimeofchattick = current;
				}
				else{
					;//ignore;
				}
			}
		}else if(buf[0]==DATAPACKET && sin_from.sin_addr.s_addr == c->CurrentRequestServer.ip){
			FrameHeader h;
			memcpy(&h,buf+1,sizeof(FrameHeader));
			c->onNALUarrived(&h,buf+1+sizeof(FrameHeader));
		}
	}
	virtual void onChatRequestByClient(char* client){

	}
	virtual void FrameArrived(unsigned char* data,int size,int type){

	}

	virtual void onNALUarrived(FrameHeader* h,unsigned char* data){

		static unsigned char* buf = NULL;
		int type;
		if(!buf){
			buf = (unsigned char*)malloc(100000);
		}
		if(onGram(h,data,h->length)==1){
			int n = GetLastMessage(buf,100000,&type);
			if(n>0)
				FrameArrived(buf,n,type);
		}
	}

	virtual void SendChatTick()
	{
		char buff[MAX_MTU] = {0};
		SOCKADDR_IN  addr;
		buff[0] = CONNETREQUEST;
		if(CurrentRequestServer.ip!=0){
			addr.sin_family = AF_INET;
			addr.sin_port = htons(DataPort);
			addr.sin_addr.s_addr = CurrentRequestServer.ip; 
			int nSendSize = sendto(sendcastfd, buff, MAX_MTU, 0, (sockaddr*)&addr, sizeof(sockaddr));
		}	
	}

	virtual void onClientChange()
	{
		return ;
	}
	virtual int onClient(void *data,int len){
		return 0;
	}
	virtual void AddClientPoint(ClientPoint p)
	{
	//	CAutoLocker m_autoLocker(&m_locker);
		clientlist.push_back(p);
	}
	virtual void DelClientPoint(vector<ClientPoint>::iterator iter)
	{
	//	CAutoLocker m_autoLocker(&m_locker);
		clientlist.erase(iter);
	}
	virtual void GetClientList(vector<ClientPoint> *l)
	{
//		CAutoLocker m_autoLocker(&m_locker);
		l->clear();
		vector<ClientPoint>::iterator iter;
		for(iter=clientlist.begin();iter!=clientlist.end();iter++)
			l->push_back(*iter);
	}
	virtual void onChatRequest(char* str)
	{
#ifdef WIN32
		CurrentRequestServer.ip = inet_addr(str);
#else
		struct in_addr addrptr;
		inet_aton(str,&addrptr);
		CurrentRequestServer.ip = addrptr.s_addr;
#endif
	}
	virtual void SendNALU(unsigned char* data,int l,unsigned char type,unsigned long timestamp)
	{
		unsigned char buf[MAX_MTU] = {0};
		int HeadLen = sizeof(FrameHeader);
		int DataLen = MAX_MTU - HeadLen - 1;
		unsigned long current = GetTickCount();
		current = timestamp;
	//	_cprintf("Send time %d\n",current);
		SOCKADDR_IN   addr;

		addr.sin_family = AF_INET;
		addr.sin_port = htons(DataPort);
		addr.sin_addr.s_addr = CurrentRequestClient.ip;
		int i = 0;
		int sendlength = 0;
		if(l > DataLen){
			int totalNum = l/DataLen + 1;
			for(;i<totalNum - 1;i++){
				FrameHeader h = {current,DataLen,i,totalNum,type};
				buf[0] = DATAPACKET;
				memcpy(buf+1,(unsigned char*)&h,HeadLen);	
				memcpy(buf+HeadLen+1,data + i*DataLen,DataLen);
				int nSendSize = sendto(senddatafd, (const char*)buf, MAX_MTU, 0, (sockaddr*)&addr, sizeof(sockaddr));
				if(nSendSize!=500){
					int x= 0;
				}
				Sleep(2);
				sendlength += DataLen;
			}
			FrameHeader h = {current,l-sendlength,i,totalNum,type};
			buf[0] = DATAPACKET;
			memcpy(buf+1,(unsigned char*)&h,HeadLen);	
			memcpy(buf+HeadLen+1,data + i*DataLen,l-sendlength);
			sendto(senddatafd, (const char*)buf, MAX_MTU, 0, (sockaddr*)&addr, sizeof(sockaddr));
		}
		else
		{
			FrameHeader h = {current,l,0,1,type};
			buf[0] = DATAPACKET;
			memcpy(buf+1,(unsigned char*)&h,sizeof(FrameHeader));	
			memcpy(buf+HeadLen+1,data,l);
			int nSendSize = sendto(senddatafd, (const char*)buf, MAX_MTU, 0, (sockaddr*)&addr, sizeof(sockaddr));
		}
	}
	virtual int HasChatRequest(){
		if(CurrentRequestClient.ip!=0)
			return 1;
		return 0;
	}
	vector<ClientPoint>	    clientlist;
private:
	int						sendcastfd;
	int						recvcastfd;
	int						recvdatafd;
	int						senddatafd;
	int                     sendchattickfd;
	int                     senddata;
	SOCKADDR_IN				cast_addr;
	SOCKADDR_IN		        cast_addr_remote;
	SOCKADDR_IN		        cast_addr_local;
	SOCKADDR_IN		        data_addr_local;
	SOCKADDR_IN				data_addr_remote;
	SOCKADDR_IN             out_chattick;
	SOCKADDR_IN             in_chattick;
	ClientPoint             CurrentRequestClient;
	ClientPoint             CurrentRequestServer;
	struct	event			revent;
	struct	event			recvdataevent;
	struct  event			clockevent;
	struct  event           chatTickEvent;
	int						running;
	ThreadPid				thread;
//	CLocker					m_locker;
//	SDL_mutex               *lock;
//	EncoderContext          *ctx;
};

