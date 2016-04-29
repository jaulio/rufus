
// EndlessUsbToolDlg.h : header file
//

#pragma once

#include "localization.h"

// CEndlessUsbToolDlg dialog
class CEndlessUsbToolDlg : public CDHtmlDialog
{
	// Construction
public:
	CEndlessUsbToolDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ENDLESSUSBTOOL_DIALOG, IDH = IDR_HTML_ENDLESSUSBTOOL_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// For dragging the window
	HRESULT OnHtmlMouseDown(IHTMLElement* pElement);
	//// Disable text selection
	//HRESULT OnHtmlSelectStart(IHTMLElement* pElement);

	// First Page Handlers
	HRESULT OnTryEndlessSelected(IHTMLElement* pElement);
	HRESULT OnInstallEndlessSelected(IHTMLElement* pElement);
	HRESULT OnLinkClicked(IHTMLElement* pElement);
	HRESULT OnLanguageChanged(IHTMLElement* pElement);

	// Select File Page Handlers
	HRESULT OnSelectFilePreviousClicked(IHTMLElement* pElement);
	HRESULT OnSelectFileNextClicked(IHTMLElement* pElement);
	HRESULT OnSelectFileButtonClicked(IHTMLElement* pElement);

	// Select USB Page handlers
	HRESULT OnSelectUSBPreviousClicked(IHTMLElement* pElement);
	HRESULT OnSelectUSBNextClicked(IHTMLElement* pElement);

	// Thank You page handlers
	HRESULT OnCloseAppClicked(IHTMLElement* pElement);

	// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()

	STDMETHODIMP ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
	STDMETHODIMP TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID);

	// Browse navigation handling methods
	void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);

private:
	bool m_fullInstall;
	loc_cmd* m_selectedLocale;
	char m_localizationFile[MAX_PATH] = "";

	CComPtr<IHTMLDocument3> m_spHtmlDoc3;

	void LoadLocalizationData();
	void AddLanguagesToUI();
	void ApplyRufusLocalization();

	void ChangePage(PCTSTR oldPage, PCTSTR newPage);
};
