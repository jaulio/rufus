
// EndlessUsbToolDlg.h : header file
//

#pragma once

#include "localization.h"
#include "DownloadManager.h"

typedef struct FileImageEntry {
    CString filePath;
    ULONGLONG size;
    BOOL autoAdded;
    LONG htmlIndex;
    BOOL stillPresent;
} FileImageEntry_t, *pFileImageEntry_t;

typedef struct RemoteImageEntry {
    ULONGLONG compressedSize;
    ULONGLONG extractedSize;
    CString personality;
    CString urlFile;
    CString urlSignature;
    CString displayName;
    CString downloadJobName;
    CString version;
} RemoteImageEntry_t, *pRemoteImageEntry_t;

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
    HRESULT OnSelectedRemoteFileChanged(IHTMLElement* pElement);
    HRESULT OnSelectedImageTypeChanged(IHTMLElement* pElement);
    HRESULT OnDownloadLightButtonClicked(IHTMLElement* pElement);
    HRESULT OnDownloadFullButtonClicked(IHTMLElement* pElement);

	// Select USB Page handlers
	HRESULT OnSelectUSBPreviousClicked(IHTMLElement* pElement);
	HRESULT OnSelectUSBNextClicked(IHTMLElement* pElement);
    HRESULT OnSelectedUSBDiskChanged(IHTMLElement* pElement);
    HRESULT OnAgreementCheckboxChanged(IHTMLElement* pElement);

    // Install Page handlers
    HRESULT OnInstallCancelClicked(IHTMLElement* pElement);

	// Error/Thank You page handlers
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
	bool m_liveInstall;
	loc_cmd* m_selectedLocale;
	char m_localizationFile[MAX_PATH];
	ULONG m_shellNotificationsRegister;
	uint64_t m_lastDevicesRefresh;
    BOOL m_lgpSet;
    BOOL m_lgpExistingKey;	// For LGP set/restore
    BOOL m_automount;
    HANDLE m_FilesChangedHandle;
    HANDLE m_cancelOperationEvent;
    HANDLE m_closeAppEvent;
    HANDLE m_fileScanThread;
    HANDLE m_operationThread;
    bool m_useLocalFile;
    long m_selectedRemoteIndex;
    long m_baseImageRemoteIndex;
    bool m_usbDeleteAgreement;
    bool m_closeRequested;
    int m_currentStep;
    CMap<CString, LPCTSTR, pFileImageEntry_t, pFileImageEntry_t> m_imageFiles;
    CList<CString> m_imageIndexToPath;
    CList<RemoteImageEntry_t> m_remoteImages;
    RemoteImageEntry_t m_installerImage;
    FileImageEntry_t m_localInstallerImage;
    static CMap<CString, LPCTSTR, uint32_t, uint32_t> m_personalityToLocaleMsg;

    CString m_localFile;
    CString m_localFileSig;
    ULONGLONG m_selectedFileSize;

    CString m_LiveFile;
    CString m_LiveFileSig;
    CString m_appDir;

	CComPtr<IHTMLDocument3> m_spHtmlDoc3;
    CComPtr<IHTMLElement> m_spStatusElem;
    CComPtr<IHTMLWindow2> m_spWindowElem;
    CComDispatchDriver m_dispWindow;
    CComPtr<IDispatchEx> m_dispexWindow;

    DownloadManager m_downloadManager;
    DWORD m_ieVersion;

    void StartOperationThread(int operation, LPTHREAD_START_ROUTINE threadRoutine, LPVOID param = NULL);

	void InitRufus();
    static DWORD WINAPI RufusISOScanThread(LPVOID param);

	void LoadLocalizationData();
	void AddLanguagesToUI();
	void ApplyRufusLocalization();

	void ChangePage(PCTSTR oldPage, PCTSTR newPage);

    HRESULT GetSelectElement(PCTSTR selectId, CComPtr<IHTMLSelectElement> &selectElem);
    HRESULT ClearSelectElement(PCTSTR selectId);
	HRESULT AddEntryToSelect(PCTSTR selectId, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected = FALSE);
	HRESULT AddEntryToSelect(CComPtr<IHTMLSelectElement> &selectElem, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected = FALSE);
    static bool IsButtonDisabled(IHTMLElement *pElement);

	void LeavingDevicesPage();

    void UpdateFileEntries(bool shouldInit = false);
    static DWORD WINAPI FileScanThread(void* param);

    void StartJSONDownload();
    void UpdateDownloadOptions();
    bool UnpackFile(LPCSTR archive, LPCSTR destination);
    bool ParseJsonFile(LPCTSTR filename, bool isInstallerJson, bool parseMockJson = false);

	void Uninit();

    void ErrorOccured(const CString errorMessage);

    HRESULT CallJavascript(LPCTSTR method, CComVariant parameter1, CComVariant parameter2 = NULL);
    void UpdateCurrentStep(int currentStep);
    bool CancelInstall();
    DownloadType_t GetSelectedDownloadType();

    static DWORD WINAPI FileVerificationThread(void* param);
    static bool FileHashingCallback(__int64 currentSize, __int64 totalSize, LPVOID context);
    
    static DWORD WINAPI FileCopyThread(void* param);
    static DWORD CALLBACK CEndlessUsbToolDlg::CopyProgressRoutine(
        LARGE_INTEGER TotalFileSize,
        LARGE_INTEGER TotalBytesTransferred,
        LARGE_INTEGER StreamSize,
        LARGE_INTEGER StreamBytesTransferred,
        DWORD         dwStreamNumber,
        DWORD         dwCallbackReason,
        HANDLE        hSourceFile,
        HANDLE        hDestinationFile,
        LPVOID        lpData
    );

    static bool ParseImgFileName(const CString& filename, CString &personality, CString &version, bool &installerImage);
    void GetImgDisplayName(CString &displayName, const CString &version, const CString &personality, ULONGLONG size = 0);

    static ULONGLONG GetExtractedSize(const CString& filename);

    void GetIEVersion();
};
