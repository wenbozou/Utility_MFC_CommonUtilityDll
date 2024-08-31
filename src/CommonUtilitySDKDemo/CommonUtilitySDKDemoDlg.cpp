
// CommonUtilitySDKDemoDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "CommonUtilitySDKDemo.h"
#include "CommonUtilitySDKDemoDlg.h"
#include "afxdialogex.h"


#include "InitLogerSDK.h"
#include "StringConversion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 线程执行的函数
unsigned int WINAPI  ThreadFun(LPVOID p) {

	//DownloadStart
	CCommonUtilitySDKDemoDlg* pDlg = (CCommonUtilitySDKDemoDlg*)p;

	pDlg->DownloadStart();

	_endthreadex(0);
	return 0;
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCommonUtilitySDKDemoDlg 对话框



CCommonUtilitySDKDemoDlg::CCommonUtilitySDKDemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_COMMONUTILITYSDKDEMO_DIALOG, pParent)
	, m_pDownloader(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CCommonUtilitySDKDemoDlg::~CCommonUtilitySDKDemoDlg()
{
	m_pDownloader->Stop();
	Sleep(500);
	if (NULL != m_pDownloader)
	{
		delete m_pDownloader;
		m_pDownloader = NULL;
	}
}

void CCommonUtilitySDKDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCommonUtilitySDKDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CCommonUtilitySDKDemoDlg 消息处理程序

BOOL CCommonUtilitySDKDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	WriteLogA("Demo", "Hello World!");
	ReleaseLoger();

	//CommonUtilitySDK::CStringConversion* pStrConver = new CommonUtilitySDK::CStringConversion();


	// 创建线程
	//m_hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadFun, (void *)this, 0, &m_dwThreadID);

	CString folderPath = L"D:\\test";

	WIN32_FILE_ATTRIBUTE_DATA  dataBuf = { 0 };
	GetFileAttributesEx(folderPath.GetBuffer(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &dataBuf);

	wstring _7zPath = L"C:\\Program Files\\7-Zip\\7z.exe";
	wstring isoPath = L"D:\\test\\cn_Win10_1909_Chinese(Simplified)_x64.iso";
	wstring params = L"";
	CString strParam;
	strParam.Format(L"echo \"%s\" x \"%s\" -o\"%s\" > 2.txt", _7zPath.c_str(), isoPath.c_str(), L"D:\\test");
	//string tempCmd = CommonUtilitySDK::CStringConversion::UnicodeToAnsi(strParam.GetBuffer());
	//strParam.ReleaseBuffer();
	string tempParam = "echo \"C:\\Program Files\\7 - Zip\\7z.exe\" x \"D:\\test\\cn_Win10_1909_Chinese(Simplified)_x64.iso\" -o\"D:\\test\" > 2.txt";
	system(tempParam.c_str());
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCommonUtilitySDKDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CCommonUtilitySDKDemoDlg::OnPaint()
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
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCommonUtilitySDKDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CCommonUtilitySDKDemoDlg::DownloadStart()
{
	if (m_pDownloader == NULL)
	{
		m_pDownloader = new CCurlDownloader();
	}

	
	std::string url = "https://dl.cutome.net//win10/cn_Win10_1909_Chinese(Simplified)_x64.iso";//"https://dl.cutome.net//win7/fr_windows_7_ultimate_x86_dvd_x15-65927.iso";///"http://example.com/file.txt";  // 下载文件的URL
	std::string filename = "D:\\test\\cn_Win10_1909_Chinese(Simplified)_x64.iso";
	m_pDownloader->DownloadStart(url.c_str(), filename.c_str());
}

