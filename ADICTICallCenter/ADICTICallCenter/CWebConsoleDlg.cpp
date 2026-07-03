// CWebConsoleDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallCenter.h"
#include "CWebConsoleDlg.h"
#include "afxdialogex.h"

using namespace Microsoft::WRL;


// CWebConsoleDlg 對話方塊

IMPLEMENT_DYNAMIC(CWebConsoleDlg, CDialogEx)

CWebConsoleDlg::CWebConsoleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CWebConsoleDlg, pParent)
{

}

CWebConsoleDlg::~CWebConsoleDlg()
{
}

void CWebConsoleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWebConsoleDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CWebConsoleDlg 訊息處理常式

CString CWebConsoleDlg::GetWebConsoleURL()
{
	// 主機跟著「設定」分頁裡的「資料庫後端網址」走（API 跟 Web 版通常部署在
	// 同一台主機，只是埠號不同），不要另外獨立設定，換主機時才不會漏改。
	// Port 維持獨立設定（Web 版預設站台是 8842，跟 API 的 8841 不同），
	// 沒有 GUI 可以編輯，需要覆寫時用 regedit 在 SystemSetting 底下加
	// WebConsoleURLPort。
	CString Host = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndURL", "localhost");
	CString Port = AfxGetApp()->GetProfileString("SystemSetting", "WebConsoleURLPort", "8842");

	CString Url;
	Url.Format(_T("http://%s:%s"), (LPCTSTR)Host, (LPCTSTR)Port);
	return Url;
}

BOOL CWebConsoleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect Rect;
	GetClientRect(&Rect);
	m_StatusLabel.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, Rect, this);
	ShowStatus(_T("正在載入網頁版控制台..."));

	InitWebView2();

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

void CWebConsoleDlg::ShowStatus(LPCTSTR pText)
{
	m_StatusLabel.SetWindowText(pText);
	m_StatusLabel.ShowWindow(SW_SHOW);
}

void CWebConsoleDlg::InitWebView2()
{
	CString UserDataFolder;
	TCHAR LocalAppData[MAX_PATH] = { 0 };
	if (GetEnvironmentVariable(_T("LOCALAPPDATA"), LocalAppData, MAX_PATH) > 0)
	{
		UserDataFolder.Format(_T("%s\\ADICTICallCenter\\WebView2"), LocalAppData);
	}

	CString Url = GetWebConsoleURL();
	HWND hWnd = m_hWnd;

	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
		nullptr,
		UserDataFolder.IsEmpty() ? nullptr : (LPCWSTR)CStringW(UserDataFolder),
		nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[this, hWnd, Url](HRESULT result, ICoreWebView2Environment* pEnv) -> HRESULT
			{
				if (FAILED(result) || pEnv == nullptr)
				{
					CString Msg;
					Msg.Format(_T("WebView2 環境建立失敗 (HRESULT=0x%08X)，請確認已安裝 WebView2 Runtime。"), result);
					ShowStatus(Msg);
					return S_OK;
				}

				pEnv->CreateCoreWebView2Controller(hWnd,
					Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
						[this, Url](HRESULT result, ICoreWebView2Controller* pController) -> HRESULT
						{
							if (FAILED(result) || pController == nullptr)
							{
								CString Msg;
								Msg.Format(_T("WebView2 控制項建立失敗 (HRESULT=0x%08X)。"), result);
								ShowStatus(Msg);
								return S_OK;
							}

							m_WebViewController = pController;
							m_WebViewController->put_IsVisible(TRUE);
							m_WebViewController->get_CoreWebView2(&m_WebView);

							CRect Rect;
							GetClientRect(&Rect);
							m_WebViewController->put_Bounds(Rect);

							if (m_WebView == nullptr)
							{
								ShowStatus(_T("WebView2 控制項已建立，但取不到 CoreWebView2 物件。"));
								return S_OK;
							}

							CString LoadingMsg;
							LoadingMsg.Format(_T("正在載入 %s ...(區域大小 %dx%d)"), (LPCTSTR)Url, Rect.Width(), Rect.Height());
							ShowStatus(LoadingMsg);

							EventRegistrationToken NavToken;
							m_WebView->add_NavigationCompleted(
								Callback<ICoreWebView2NavigationCompletedEventHandler>(
									[this](ICoreWebView2* pSender, ICoreWebView2NavigationCompletedEventArgs* pArgs) -> HRESULT
									{
										BOOL bSuccess = FALSE;
										pArgs->get_IsSuccess(&bSuccess);
										if (bSuccess)
										{
											m_StatusLabel.ShowWindow(SW_HIDE);
										}
										else
										{
											COREWEBVIEW2_WEB_ERROR_STATUS ErrorStatus = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
											pArgs->get_WebErrorStatus(&ErrorStatus);
											CString Msg;
											Msg.Format(_T("網頁載入失敗 (WebErrorStatus=%d)，請確認 %s 這個網址可以連上。"), (int)ErrorStatus, (LPCTSTR)GetWebConsoleURL());
											ShowStatus(Msg);
										}
										return S_OK;
									}).Get(),
								&NavToken);

							HRESULT NavHr = m_WebView->Navigate((LPCWSTR)CStringW(Url));
							if (FAILED(NavHr))
							{
								CString Msg;
								Msg.Format(_T("呼叫 Navigate 失敗 (HRESULT=0x%08X)。"), NavHr);
								ShowStatus(Msg);
							}

							return S_OK;
						}).Get());

				return S_OK;
			}).Get());

	if (FAILED(hr))
	{
		CString Msg;
		Msg.Format(_T("呼叫 CreateCoreWebView2EnvironmentWithOptions 失敗 (HRESULT=0x%08X)。"), hr);
		ShowStatus(Msg);
	}
}

void CWebConsoleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	CRect Rect(0, 0, cx, cy);
	if (m_StatusLabel.GetSafeHwnd() != nullptr)
	{
		m_StatusLabel.MoveWindow(Rect);
	}
	if (m_WebViewController != nullptr)
	{
		m_WebViewController->put_Bounds(Rect);
	}
}

void CWebConsoleDlg::Reload()
{
	if (m_WebView != nullptr)
	{
		m_WebView->Reload();
	}
}

void CWebConsoleDlg::OnDestroy()
{
	if (m_WebViewController != nullptr)
	{
		m_WebViewController->Close();
		m_WebViewController = nullptr;
		m_WebView = nullptr;
	}

	CDialogEx::OnDestroy();
}
