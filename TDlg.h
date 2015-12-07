
// TDlg.h : 头文件
//

#pragma once

#include "afxwin.h"
#include "Caster.h"
#include "Encoder.h"
#include "stdafx.h"
#include "BipBuffer.h"
#include "CameraYUV.h"
#include "Micphone.h"
#include "NetDecoder.h"
#include "SDL2/include/SDL.h"
#pragma comment(lib,"SDL2/lib/SDL2.lib")



class MyCaster :public NetDecoder,public BipBuffer{
public:
	MyCaster(HWND hwd,HWND view)
		:pWindow(NULL),pTexture(NULL),pRender(NULL),audio_opened(0)
	{
		m_hwd = hwd;
		m_view = view;
		AllocateBuffer(1024*500);
	}
	virtual ~MyCaster(){

	}
	void onClientChange(){
		::SendMessageA(m_hwd,WM_PAINT,0,0);
	//	::PostMessageA(m_hwd,WM_PAINT,0,0);
	}
	virtual int init_sdl(int w,int h){
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
		pWindow = SDL_CreateWindowFrom((void*)m_view);
		pRender = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
		pTexture = SDL_CreateTexture(pRender,SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,w,h);
		width = w;
		height = h;
		return 0;
	}
	virtual void RefreshAudio(AVFrame *p)
	{
		if(!audio_opened){
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
				audio_opened = 1;
			}  
		}
		if(audio_opened){
			SDL_PauseAudio(0); 
			InputData(p->data[0],p->linesize[0]);
		}
	}
	static void __cdecl audio_callback(void *udata,Uint8 *stream,int len)
	{
		int  n = 0;
		int& size = n;
		uint8_t *data = NULL;
		MyCaster *audiodata = (MyCaster*)udata;
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

	virtual void InputData(uint8_t *data,int size){
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


	virtual void RefreshVideo(AVFrame *p){

		if(p->width >0 && p->height>0){
			int w = p->width;
			int h = p->height;
			if(!pWindow){
				SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
				pWindow = SDL_CreateWindowFrom((void*)m_view);
				pRender = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
				pTexture = SDL_CreateTexture(pRender,SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,w,h);
				width = w;
				height = h;
			}
			SDL_Rect sdlRT,dstRT;
			SDL_GetWindowSize(pWindow,&dstRT.w,&dstRT.h);
			sdlRT.w = width;
			sdlRT.h = height;
			dstRT.x = sdlRT.x = 0;
			dstRT.y = sdlRT.y = 0;
			SDL_UpdateYUVTexture( pTexture, &sdlRT,p->data[0],p->linesize[0],p->data[1],p->linesize[1],p->data[2],p->linesize[2]);
			SDL_RenderClear(pRender);
			SDL_RenderCopy(pRender, pTexture, &sdlRT, &dstRT);
			SDL_RenderPresent(pRender);
		}
	}
private:
	HWND m_hwd;
	HWND m_view;
	SDL_Window		    *pWindow ;
	SDL_Renderer	    *pRender;
	SDL_Texture		    *pTexture;
	int width,height;
	int audio_opened;
	SDL_AudioSpec wanted_spec;
	CLocker	            Audio_locker;
};

// CTDlg 对话框
class CTDlg : public CDialogEx//,public MicphoneCap
{
// 构造
public:
	CTDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_T_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void myClose();
	DECLARE_MESSAGE_MAP()
public:
	CListBox m_listbox;
	MyCaster *c;
	vector<ClientPoint>  clist;
	HWND m_hCapWnd;

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CStatic pic1;
	afx_msg void OnLbnDblclkList1();
	virtual void InitSdl(int w,int h);
	virtual void Refresh(AVPicture *p);
	int w,h;
	Encoder e;
//	Decoder de;
	CameraYUV *camera;
	MicphoneCap *micphone;
	AVFrame *frame;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	SDL_Window		    *pWindow ;
	SDL_Renderer	    *pRender;
	SDL_Texture		    *pTexture;
	CCheckListBox       chechbox1;
	CCheckListBox       chechbox2;
	afx_msg void OnBnClickedCheck1();
	
	//afx_msg void OnClose();
};
