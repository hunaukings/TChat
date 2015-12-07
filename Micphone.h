#pragma once

#include <dsound.h>
#define NUM_REC_NOTIFICATIONS  16
#include "AutoLocker.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif
#ifndef MAX
#define MAX(a,b)        ( (a) > (b) ? (a) : (b) )
#endif


extern "C"{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "SDL2/include/SDL.h"
};
#pragma comment(lib,"lib/avcodec.lib")
#pragma comment(lib,"lib/avutil.lib")
#pragma comment(lib,"lib/swscale.lib")

class CAdoFrameHandler {
public:
	virtual void AdoFrameData(BYTE* pBuffer, long lBufferSize) = 0 ; 
};
class CCaptureAudio
{
public:
	CCaptureAudio(void)
	{
		if(FAILED(CoInitialize(NULL))) /*, COINIT_APARTMENTTHREADED)))*/
		{
			//AfxMessageBox("CCaptureAudio CoInitialize Failed!\r\n"); 
			return;
		}
		m_pCapDev = NULL ;
		m_pCapBuf = NULL ;
		m_pNotify = NULL ;
		// set default wave format PCM
		ZeroMemory( &m_wfxInput, sizeof(m_wfxInput));
		m_wfxInput.wFormatTag = WAVE_FORMAT_PCM;
		m_guidCapDevId = GUID_NULL ; 
		m_bRecording = FALSE ; 
		m_hNotifyEvent = NULL ; 
	}
	virtual ~CCaptureAudio(void)
	{
		CoUninitialize() ; 
	}
	static BOOL CALLBACK enum_dev_proc(LPGUID lpGUID, LPCTSTR lpszDesc, 
		LPCTSTR lpszDrvName, LPVOID lpContext) 
	{
		HWND hList = (HWND)lpContext;
		if(!hList) return FALSE ; 
		LPGUID lpTemp = NULL;
		if (lpGUID != NULL) {
			// NULL only for "Primary Sound Driver".
			if ((lpTemp = (LPGUID)malloc(sizeof(GUID))) == NULL) return(TRUE);
			memcpy(lpTemp, lpGUID, sizeof(GUID));
		}
		::SendMessage(hList, CB_ADDSTRING, 0,(LPARAM)lpszDesc);
		::SendMessage(hList, LB_SETITEMDATA, 0, (LPARAM)lpTemp) ; 
		free(lpTemp);
		return(TRUE);
	}
	static void notify_capture_thd(LPVOID data)
	{
		CCaptureAudio * pado = static_cast<CCaptureAudio*>(data); 
		MSG   msg;
		HRESULT hr ; 
		DWORD dwResult ; 
		while(pado->m_bRecording) {
			dwResult = MsgWaitForMultipleObjects( 1, &(pado->m_hNotifyEvent), FALSE, INFINITE, QS_ALLEVENTS );
			switch( dwResult ) {
			case WAIT_OBJECT_0 + 0:
				// g_hNotificationEvents[0] is signaled
				// This means that DirectSound just finished playing 
				// a piece of the buffer, so we need to fill the circular 
				// buffer with new sound from the wav file
				if( FAILED( hr = pado->RecordCapturedData() ) ) {
					//				AfxMessageBox("Error handling DirectSound notifications.") ; 
					pado->m_bRecording = FALSE ; 
				}
				break;
			case WAIT_OBJECT_0 + 1:
				// Windows messages are available
				while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) { 
					TranslateMessage( &msg ); 
					DispatchMessage( &msg ); 
					if( msg.message == WM_QUIT ) pado->m_bRecording = FALSE ; 
				}
				break;
			}
		}
		//	AfxEndThread(0, TRUE) ; 
		return; 
	}
	BOOL EnumDevices(HWND hList) 
	{
		if (FAILED(DirectSoundCaptureEnumerate (
			(LPDSENUMCALLBACK)(CCaptureAudio::enum_dev_proc),
			(VOID*)&hList)))
		{
			return(FALSE);
		}
		return (TRUE) ; 
	}
	BOOL Open(void)
	{
		HRESULT hr ; 
		if(!m_bRecording) {
			hr = InitDirectSound() ; 
		}
		return (FAILED(hr)) ? FALSE : TRUE ; 
	}
	BOOL Close() 
	{
		HRESULT hr ; 
		hr = FreeDirectSound() ; 
		CloseHandle(m_hNotifyEvent) ; 
		return (FAILED(hr)) ? FALSE : TRUE ; 
	}
	HRESULT InitDirectSound(GUID dev_id = GUID_NULL)
	{
		HRESULT hr ; 
		m_guidCapDevId = dev_id ;
		ZeroMemory( &m_aPosNotify, sizeof(DSBPOSITIONNOTIFY) * (NUM_REC_NOTIFICATIONS + 1) ) ;
		m_dwCapBufSize = 0 ;
		m_dwNotifySize = 0 ;
		// Create IDirectSoundCapture using the preferred capture device
		hr = DirectSoundCaptureCreate(&m_guidCapDevId, &m_pCapDev, NULL ) ;
		// init wave format 
		SetWavFormat(&m_wfxInput) ;
		return hr ; 
	}
	HRESULT FreeDirectSound()
	{
		// Release DirectSound interfaces
		SAFE_RELEASE( m_pNotify ) ;
		SAFE_RELEASE( m_pCapBuf ) ;
		SAFE_RELEASE( m_pCapDev ) ; 
		return S_OK;
	}
	HRESULT CreateCaptureBuffer(WAVEFORMATEX * wfx) 
	{
		HRESULT hr;
		DSCBUFFERDESC dscbd;
		SAFE_RELEASE( m_pNotify );
		SAFE_RELEASE( m_pCapBuf );
		// Set the notification size
		m_dwNotifySize = MAX( 1024, wfx->nAvgBytesPerSec / 8 ) ; 
		m_dwNotifySize -= m_dwNotifySize % wfx->nBlockAlign ;
		// Set the buffer sizes 
		m_dwCapBufSize = m_dwNotifySize * NUM_REC_NOTIFICATIONS;
		SAFE_RELEASE( m_pNotify );
		SAFE_RELEASE( m_pCapBuf );
		// Create the capture buffer
		ZeroMemory( &dscbd, sizeof(dscbd) );
		dscbd.dwSize        = sizeof(dscbd);
		dscbd.dwBufferBytes = m_dwCapBufSize;
		dscbd.lpwfxFormat   = wfx ; // Set the format during creatation
		if( FAILED( hr = m_pCapDev->CreateCaptureBuffer( &dscbd, &m_pCapBuf, NULL ) ) )
			return S_FALSE ;
		m_dwNextCapOffset = 0;
		if( FAILED( hr = InitNotifications() ) )
			return S_FALSE ;
		return S_OK;
	}
	HRESULT InitNotifications() 
	{
		HRESULT hr; 
		int i ; 
		if( NULL == m_pCapBuf )
			return S_FALSE;
		// create auto notify event 
		m_hNotifyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
		// Create a notification event, for when the sound stops playing
		if( FAILED( hr = m_pCapBuf->QueryInterface( IID_IDirectSoundNotify, (VOID**)&m_pNotify ) ) )
			return S_FALSE ;
		// Setup the notification positions
		for( i = 0; i < NUM_REC_NOTIFICATIONS; i++ ) {
			m_aPosNotify[i].dwOffset = (m_dwNotifySize * i) + m_dwNotifySize - 1;
			m_aPosNotify[i].hEventNotify = m_hNotifyEvent;             
		}
		// Tell DirectSound when to notify us. the notification will come in the from 
		// of signaled events that are handled in WinMain()
		if( FAILED( hr = m_pNotify->SetNotificationPositions( NUM_REC_NOTIFICATIONS, m_aPosNotify ) ) )
			return S_FALSE ;
		return S_OK;
	}
	HRESULT StartOrStopRecord(BOOL bStartRec)
	{
		HRESULT hr;
		if( bStartRec ) {
			// Create a capture buffer, and tell the capture 
			// buffer to start recording   
			if( FAILED( hr = CreateCaptureBuffer( &m_wfxInput ) ) )
				return S_FALSE ;
			if( FAILED( hr = m_pCapBuf->Start( DSCBSTART_LOOPING ) ) )
				return S_FALSE ;
			// create notify event recv thread 
			//_beginthread()
			_beginthread(CCaptureAudio::notify_capture_thd, 0,(LPVOID)(this)) ;
		} else { 
			// Stop the capture and read any data that 
			// was not caught by a notification
			if( NULL == m_pCapBuf )
				return S_OK;
			// wait until the notify_event_thd thread exit and release the resources.
			Sleep(500) ;
			// Stop the buffer, and read any data that was not 
			// caught by a notification
			if( FAILED( hr = m_pCapBuf->Stop() ) )
				return S_OK ;
			if( FAILED( hr = RecordCapturedData() ) )
				return S_FALSE ; 
		}
		return S_OK;
	}
	HRESULT RecordCapturedData() 
	{
		HRESULT hr;
		VOID*   pbCaptureData    = NULL;
		DWORD   dwCaptureLength;
		VOID*   pbCaptureData2   = NULL;
		DWORD   dwCaptureLength2;
		DWORD   dwReadPos;
		DWORD   dwCapturePos;
		LONG lLockSize;
		if( NULL == m_pCapBuf )
			return S_FALSE; 

		if( FAILED( hr = m_pCapBuf->GetCurrentPosition( &dwCapturePos, &dwReadPos ) ) )
			return S_FALSE;
		lLockSize = dwReadPos - m_dwNextCapOffset;
		if( lLockSize < 0 )
			lLockSize += m_dwCapBufSize;
		// Block align lock size so that we are always write on a boundary
		lLockSize -= (lLockSize % m_dwNotifySize);
		if( lLockSize == 0 )
			return S_FALSE;
		// Lock the capture buffer down
		if( FAILED( hr = m_pCapBuf->Lock( m_dwNextCapOffset, lLockSize,
			&pbCaptureData, &dwCaptureLength, 
			&pbCaptureData2, &dwCaptureLength2, 0L ) ) )
			return S_FALSE ;
		// call the outer data handler
		if(m_frame_handler) {
			m_frame_handler->AdoFrameData((BYTE*)pbCaptureData, dwCaptureLength) ; 
		}

		// Move the capture offset along
		m_dwNextCapOffset += dwCaptureLength; 
		m_dwNextCapOffset %= m_dwCapBufSize; // Circular buffer
		if( pbCaptureData2 != NULL ) {
			// call the outer data handler 
			if(m_frame_handler) {
				m_frame_handler->AdoFrameData((BYTE*)pbCaptureData, dwCaptureLength) ; 
			}
			// Move the capture offset along
			m_dwNextCapOffset += dwCaptureLength2; 
			m_dwNextCapOffset %= m_dwCapBufSize; // Circular buffer
		}
		// Unlock the capture buffer
		m_pCapBuf->Unlock( pbCaptureData,  dwCaptureLength, pbCaptureData2, dwCaptureLength2 );
		return S_OK;
	}
	void SetWavFormat(WAVEFORMATEX * wfx)
	{
		// get the default capture wave formate 
		ZeroMemory(wfx, sizeof(WAVEFORMATEX)) ; 
		wfx->wFormatTag = WAVE_FORMAT_PCM;
		// 8KHz, 16 bits PCM, Mono
		wfx->nSamplesPerSec = 8000 ; 
		wfx->wBitsPerSample = 16 ; 
		wfx->nChannels  = 1 ;
		wfx->nBlockAlign = wfx->nChannels * ( wfx->wBitsPerSample / 8 ) ; 
		wfx->nAvgBytesPerSec = wfx->nBlockAlign * wfx->nSamplesPerSec;
	}
	void GrabAudioFrames(BOOL bGrabAudioFrames, CAdoFrameHandler* frame_handler) 
	{
		m_frame_handler = frame_handler ; 
		m_bRecording = bGrabAudioFrames ; 
		StartOrStopRecord(m_bRecording) ; 
	}

	BOOL        m_bRecording ;  //recording now ? also used by event recv thread
protected:
	LPDIRECTSOUNDCAPTURE8    m_pCapDev ;   //capture device ptr
	LPDIRECTSOUNDCAPTUREBUFFER m_pCapBuf ;   //capture loop buffer ptr
	LPDIRECTSOUNDNOTIFY8    m_pNotify ;   //capture auto-notify event callback handler ptr
	GUID        m_guidCapDevId ;  //capture device id
	WAVEFORMATEX      m_wfxInput;   //input wave format description struct
	DSBPOSITIONNOTIFY     m_aPosNotify[NUM_REC_NOTIFICATIONS + 1]; //notify flag array 
	HANDLE        m_hNotifyEvent;   //notify event 
	BOOL        m_abInputFmtSupported[20];
	DWORD        m_dwCapBufSize;  //capture loop buffer size 
	DWORD        m_dwNextCapOffset;//offset in loop buffer 
	DWORD        m_dwNotifySize;  //notify pos when loop buffer need to emit the event
	CAdoFrameHandler*     m_frame_handler ; // outer frame data dealer ptr 
public: // callback func to add enum devices string name 
	//static void notify_capture_thd(LPVOID data) ;
protected:
//	HRESULT InitDirectSound(GUID dev_id = GUID_NULL) ; 
//	HRESULT FreeDirectSound() ; 
//	HRESULT InitNotifications() ; 
//	HRESULT CreateCaptureBuffer(WAVEFORMATEX * wfx) ; 
//	HRESULT StartOrStopRecord(BOOL bStartRec) ;
//	HRESULT RecordCapturedData() ; 
//	void    SetWavFormat(WAVEFORMATEX * wfx) ;
public:
//	CCaptureAudio(void);
//	~CCaptureAudio(void);
//	BOOL EnumDevices(HWND hList) ;
//	BOOL Open(void) ; 
//	BOOL Close() ; 
//	void GrabAudioFrames(BOOL bGrabAudioFrames, CAdoFrameHandler* frame_handler) ; 
};
class MicphoneCap:public CAdoFrameHandler,public CCaptureAudio{

public:
	MicphoneCap()
		:buffer(NULL),size(0),capatial(0),get(0),m_frame(NULL)
	{

	}
	virtual ~MicphoneCap()
	{
		if(buffer)
			free(buffer);
	}
    virtual int OpenMic()
	{
//		AVCodec *codec;

		if(!Open())
			return -1;

		GrabAudioFrames(TRUE, this);

	//	avcodec_register_all();

	//	codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	//	if(!codec){
	//		return -1;
	//	}
	//	codecctx = avcodec_alloc_context3(codec);
	//	if(!codecctx)
	//		return -1;  
	////	codecctx->bit_rate = 8000*2;  
	//	codecctx->sample_rate = 8000;  
	//	codecctx->channels = 1;  
	//	codecctx->sample_fmt = AV_SAMPLE_FMT_S16; 
	//	codecctx->channels = 1;
	//	codecctx->channel_layout = AV_CH_LAYOUT_MONO;
	//	//codecctx->sample_fmt = codecctx->codec->sample_fmts[0];  
	//	if(avcodec_open2(codecctx,codec,NULL)<0)
	//		return -1;
	//	m_frame = av_frame_alloc();
		return 0;
	}
	virtual void AdoFrameData(BYTE* pBuffer, long lBufferSize)
	{
		//return ;
		//int got_frame = 0;
		//int ret = 0;
		//AVPacket pkt = {0};
		//av_init_packet(&pkt);
		//
		//m_frame->data[0] = pBuffer;
		//m_frame->linesize[0] = lBufferSize;
 
		//ret = avcodec_encode_audio2(codecctx,&pkt,m_frame,&got_frame);
		//if(ret<0){
		//	av_free_packet(&pkt);
		//	return;
		//}
		//if(got_frame){
		if(capatial<lBufferSize){
			buffer = (unsigned char *)realloc(buffer,lBufferSize);
			capatial = lBufferSize;
		}
		//}
		{
			CAutoLocker m_autoLocker(&m_locker);
			memcpy(buffer,pBuffer,lBufferSize);
			get = TRUE;
			size = lBufferSize;
		}
	//	av_free_packet(&pkt);
	}
	virtual int GetAudioFrame(unsigned char* obuffer,int *osize)
	{
		CAutoLocker m_autoLocker(&m_locker);
		if(!get)
			return 0;
		memcpy(obuffer,buffer,size);
		get = FALSE;
		*osize = size;
		return size;
	}
	virtual void close(){
		CCaptureAudio::Close();
	}
private:
	unsigned char *buffer;
	unsigned int  size;
	unsigned int  capatial;
	BOOL          get;
protected:
	CCaptureAudio   m_cap_ado ; 
	int             number;
	AVFrame         *m_frame;
	AVCodecContext  *codecctx;
	SDL_AudioSpec   wanted_spec;
	CLocker		    m_locker;
};