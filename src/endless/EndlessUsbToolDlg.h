
// EndlessUsbToolDlg.h : header file
//

#pragma once

#include "localization.h"

typedef struct FileImageEntry {
    CString filePath;
    ULONGLONG size;
    BOOL autoAdded;
    LONG htmlIndex;
    BOOL stillPresent;
} FileImageEntry_t, *pFileImageEntry_t;

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
    HRESULT OnSelectedImageFileChanged(IHTMLElement* pElement);

	// Select USB Page handlers
	HRESULT OnSelectUSBPreviousClicked(IHTMLElement* pElement);
	HRESULT OnSelectUSBNextClicked(IHTMLElement* pElement);
    HRESULT OnSelectedUSBDiskChanged(IHTMLElement* pElement);

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

	BOOL PreTranslateMessage(MSG* pMsg);
	static void CALLBACK RefreshTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    BOOL CanAccessExternal() { return TRUE; } // disable a ActiveX warning
    void JavascriptDebug(LPCTSTR debugMsg);

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
    DECLARE_DISPATCH_MAP()

	STDMETHODIMP ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
	STDMETHODIMP TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID);

	// Browse navigation handling methods
	void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);

private:
	bool m_fullInstall;
	loc_cmd* m_selectedLocale;
	char m_localizationFile[MAX_PATH] = "";
	ULONG m_shellNotificationsRegister;
	uint64_t m_lastDevicesRefresh;
    BOOL m_lgpSet;
    BOOL m_lgpExistingKey;	// For LGP set/restore
    BOOL m_automount;
    HANDLE m_FilesChangedHandle;
    HANDLE m_closingApplicationEvent;
    HANDLE m_fileScanThread;
    HANDLE m_formatThread;
    CMap<CString, LPCTSTR, pFileImageEntry_t, pFileImageEntry_t> m_imageFiles;
    CList<CString> m_imageIndexToPath;

	CComPtr<IHTMLDocument3> m_spHtmlDoc3;
    CComPtr<IHTMLElement> m_spStatusElem;
    CComPtr<IHTMLWindow2> m_spWindowElem;
    CComDispatchDriver m_dispWindow;
    CComPtr<IDispatchEx> m_dispexWindow;

	void InitRufus();
    static DWORD WINAPI RufusISOScanThread(LPVOID param);

	void LoadLocalizationData();
	void AddLanguagesToUI();
	void ApplyRufusLocalization();

	void ChangePage(PCTSTR oldPage, PCTSTR newPage);

	HRESULT AddEntryToSelect(PCTSTR selectId, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected = FALSE);
	HRESULT AddEntryToSelect(CComPtr<IHTMLSelectElement> &selectElem, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected = FALSE);

	void LeavingDevicesPage();

    void UpdateFileEntries();
    static DWORD WINAPI FileScanThread(void* param);

	void Uninit();
};
