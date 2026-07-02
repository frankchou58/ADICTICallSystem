#pragma once
#include <wrl.h>
#include <WebView2.h>


// CWebConsoleDlg 對話方塊
//
// 內嵌 WebView2 顯示 ADICTICallCenter.Web 網頁版控制台，跟原本 4 個分頁並列，
// 純粹多一個入口，不取代任何既有功能（Web 版沒有即時推播，只有快照）。
// 網址預設 http://localhost:8842，可用登錄檔 SystemSetting\WebConsoleURL 覆寫，
// 跟 ApiEmployeeNo/ApiPassword 同一套機制（沒有 GUI 可以編輯）。

class CWebConsoleDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CWebConsoleDlg)

public:
	CWebConsoleDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~CWebConsoleDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CWebConsoleDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();

private:
	Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_WebViewController;
	Microsoft::WRL::ComPtr<ICoreWebView2> m_WebView;
	// WebView2 初始化是非同步的，成功接管畫面前先顯示狀態/錯誤文字，
	// 避免像一開始那樣「失敗了但畫面整個空白、看不出發生什麼事」。
	CStatic m_StatusLabel;

	CString GetWebConsoleURL();
	void InitWebView2();
	void ShowStatus(LPCTSTR pText);
};
