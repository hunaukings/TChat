
// TDlg.cpp : 实现文件
//


#include "stdafx.h"
#include "T.h"
#include "TDlg.h"
#include "afxdialogex.h"
#include <vfw.h>
#include <Windows.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	//ON_MESSAGE_VOID(WM_CLOSE,myClose)
END_MESSAGE_MAP()


// CTDlg 对话框




CTDlg::CTDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listbox);
	DDX_Control(pDX, IDC_PIC_VIEW, pic1);
}

BEGIN_MESSAGE_MAP(CTDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CTDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CTDlg::OnBnClickedCancel)
	ON_LBN_DBLCLK(IDC_LIST1, &CTDlg::OnLbnDblclkList1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK1, &CTDlg::OnBnClickedCheck1)
//	ON_WM_CLOSE()
END_MESSAGE_MAP()

#define TIMERID 8
// CTDlg 消息处理程序

BOOL CTDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	AllocConsole();
	c = new MyCaster(GetSafeHwnd(),GetDlgItem(IDC_PIC_VIEW)->GetSafeHwnd());
	camera = new CameraYUV();
	micphone = new MicphoneCap();
	if(micphone->OpenMic()<0){
		//::MessageBoxA(GetSafeHwnd(),"不能打开麦克风设备","ERR",0);
//		_cprintf("不能打开麦克风\n");
	}
	c->init(3779);
	c->Start();
	c->open();
	c->onChatRequest("192.168.191.3");
	if(camera->OpenCamera(&w,&h)<0){
		//::MessageBoxA(GetSafeHwnd(),"不能打开摄像头设备","ERR",0);
//		_cprintf("不能打开摄像头\n");
	}else{
		if(e.Init(w,h)<0)
			return 0;
	}
	InitSdl(w,h);
	SetTimer(TIMERID,40,0);
	frame = av_frame_alloc();
	return TRUE;							// 除非将焦点设置到控件，否则返回 TRUE
}

void CTDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTDlg::OnPaint()
{
	
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}

	m_listbox.ResetContent();
	c->GetClientList(&clist);
	vector<ClientPoint>::iterator iter;
	for(iter = clist.begin(); iter != clist.end() ; iter++){
		int a,b,c,d;
		a = iter->ip & 0xFF ;
		b = (iter->ip & 0xFF00 ) >> 8;
		c = (iter->ip & 0xFF0000 ) >> 16;
		d = (iter->ip & 0xFF000000 ) >> 24;
		CString  str;
		str.Format(_T("%d.%d.%d.%d"),a,b,c,d);
		m_listbox.AddString(str);
	}
//	Refresh(NULL);
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CTDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


void CTDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码

	CDialogEx::OnCancel();
}


void CTDlg::OnLbnDblclkList1()
{
	char multibuf[256] = {0};
	WCHAR buf[256] = {0};
	int x = m_listbox.GetCaretIndex();
	m_listbox.GetText(x,buf);
	::WideCharToMultiByte( CP_ACP,0,buf,-1,multibuf,256,NULL,NULL);
	c->onChatRequest(multibuf);
	int y = 0;
	// TODO: 在此添加控件通知处理程序代码
}


void CTDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	static uint8_t *data = NULL;
	static int size = 0;
	AVPicture *pic;// = d.GetPicture();
	pic = (AVPicture*)camera->GetFrame();
	static int firstpresendtime = GetTickCount();
	int current = GetTickCount();

	if(!data)
		data = (uint8_t*)malloc(10000);

	EncodecOutData *out;
	if(pic){
		Refresh(pic);
		if(c->HasChatRequest()){
			if(e.Encode(pic,&out,current-firstpresendtime)>0){
				uint32_t n = 0;
				for(uint32_t i = 0;i<out->number;i++)
					n+=out->size[i];
				c->SendNALU(out->data[0],n,0,out->timestamp);
			}
			size = micphone->GetAudioFrame(data,&size);
			if(size>0)
				c->SendNALU(data,size,1,GetTickCount());
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

void CTDlg::InitSdl( int w,int h )
{
	SDL_Init(SDL_INIT_VIDEO);
	pWindow = SDL_CreateWindowFrom((void*)GetDlgItem(IDC_PIC_VIEW2)->GetSafeHwnd());
	pRender = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
	pTexture = SDL_CreateTexture(pRender,SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,w,h);
}

void CTDlg::Refresh( AVPicture *p )
{
	uint8_t d[6] = {0,0,0,0,128,128};
	//黑色背景色
	SDL_Rect sdlRT,dstRT;
	SDL_GetWindowSize(pWindow,&dstRT.w,&dstRT.h);

	if(BST_CHECKED != IsDlgButtonChecked( IDC_CHECK1) || !p){
		sdlRT.w = 2;
		sdlRT.h = 2;
		dstRT.x = sdlRT.x = 0;
		dstRT.y = sdlRT.y = 0;
		SDL_UpdateYUVTexture( pTexture, &sdlRT,d,2,d+4,1,d+5,1);
	}
	else{
		sdlRT.w = w;
		sdlRT.h = h;
		dstRT.x = sdlRT.x = 0;
		dstRT.y = sdlRT.y = 0;
		SDL_UpdateYUVTexture( pTexture, &sdlRT,p->data[0],p->linesize[0],p->data[1],p->linesize[1],p->data[2],p->linesize[2]);
	}
	SDL_RenderClear(pRender);
	SDL_RenderCopy(pRender, pTexture, &sdlRT, &dstRT);
	SDL_RenderPresent(pRender);
}


void CTDlg::OnBnClickedCheck1()
{
	//::SendMessageA(this->GetSafeHwnd(),WM_PAINT,0,0);
	// TODO: 在此添加控件通知处理程序代码
}
void CTDlg::myClose()
{

}

