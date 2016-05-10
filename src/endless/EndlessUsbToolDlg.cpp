
// EndlessUsbToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EndlessUsbTool.h"
#include "EndlessUsbToolDlg.h"
#include "afxdialogex.h"

#include <windowsx.h>
#include <dbt.h>

#include <atlpath.h>

// Rufus include files
extern "C" {
#include "rufus.h"
#include "missing.h"
#include "msapi_utf8.h"
#include "drive.h"
#include "bled/bled.h"

// RADU: try to remove the need for all of these
OPENED_LIBRARIES_VARS;
HWND hMainDialog = NULL, hLog = NULL;
BOOL right_to_left_mode = FALSE;
GetTickCount64_t pfGetTickCount64 = NULL;
RUFUS_UPDATE update = { { 0,0,0 },{ 0,0 }, NULL, NULL };
char *ini_file = NULL;
BOOL usb_debug = FALSE;
WORD selected_langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

extern const loc_control_id control_id[];
extern loc_dlg_list loc_dlg[];
extern char** msg_table;

// Trying to reuse as much as rufus as possible
HWND hDeviceList = NULL, hBootType = NULL, hPartitionScheme = NULL, hFileSystem = NULL, hClusterSize = NULL, hLabel = NULL;
HWND hNBPasses = NULL, hDiskID = NULL, hInfo = NULL;
HINSTANCE hMainInstance = NULL;

BOOL detect_fakes = FALSE;

char app_dir[MAX_PATH], system_dir[MAX_PATH], sysnative_dir[MAX_PATH];
char* image_path = NULL;
// Number of steps for each FS for FCC_STRUCTURE_PROGRESS
const int nb_steps[FS_MAX] = { 5, 5, 12, 1, 10 };
BOOL format_op_in_progress = FALSE;
BOOL allow_dual_uefi_bios, togo_mode, force_large_fat32, enable_ntfs_compression = FALSE, lock_drive = TRUE;
BOOL zero_drive = FALSE, preserve_timestamps, list_non_usb_removable_drives = FALSE;
BOOL use_own_c32[NB_OLD_C32] = { FALSE, FALSE };
uint16_t rufus_version[3], embedded_sl_version[2];
char embedded_sl_version_str[2][12] = { "?.??", "?.??" };
char embedded_sl_version_ext[2][32];
uint32_t dur_mins, dur_secs;
StrArray DriveID, DriveLabel;
BOOL enable_HDDs = FALSE, use_fake_units, enable_vmdk;
int dialog_showing = 0;
};

#include "localization.h"
// End Rufus include files

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define IFFALSE_GOTOERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); goto error; }
#define IFFALSE_RETURN(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); return; }
#define IFFALSE_RETURN_VALUE(__CONDITION__, __ERRROR_MSG__, __RET__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); return __RET__; }
#define IFFALSE_BREAK(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); break; }

// HTML element ids and classes
#define CLASS_PAGE_HEADER_TITLE         "PageHeaderTitle"
//First page elements
#define ELEMENT_LEFT_PANEL              "LeftPanel"
#define ELEMENT_RIGHT_PANEL             "RightPanel"
#define ELEMENT_COMPARE_OPTIONS         "CompareOptionsLink"
#define ELEMENT_LANGUAGE_SELECT         "LanguageSelect"
//Select File page elements
#define ELEMENT_SELFILE_PREV_BUTTON     "SelectFilePreviousButton"
#define ELEMENT_SELFILE_NEXT_BUTTON     "SelectFileNextButton"
#define ELEMENT_SELFILE_BUTTON          "SelectFileButton"
#define ELEMENT_FILES_SELECT            "LocalImagesSelect"
//Select USB page elements
#define ELEMENT_SELUSB_PREV_BUTTON      "SelectUSBPreviousButton"
#define ELEMENT_SELUSB_NEXT_BUTTON      "SelectUSBNextButton"
#define ELEMENT_SELUSB_USB_DRIVES       "USBDiskSelect"
//Installing page elements
#define ELEMENT_SECUREBOOT_HOWTO        "SecureBootHowTo"
#define ELEMENT_INSTALL_STATUS          "CreatingInstallerText"
//Thank You page elements
#define ELEMENT_CLOSE_BUTTON            "CloseAppButton"
#define ELEMENT_SECUREBOOT_HOWTO2       "SecureBootHowToReminder"


enum custom_message {
    WM_FILES_CHANGED = UM_NO_UPDATE + 1,
    WM_FINISHED_IMG_SCANNING,
    WM_UPDATE_PROGRESS
};

#define TID_UPDATE_FILES                TID_REFRESH_TIMER + 1

// utility method for quick char* UTF8 conversion to BSTR
CComBSTR UTF8ToBSTR(const char *txt) {
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, txt, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[wchars_num];
	MultiByteToWideChar(CP_UTF8, 0, txt, -1, wstr, wchars_num);
	CComBSTR return_value(wstr);
	delete[] wstr;

	return return_value;
}

extern "C" void UpdateProgress(int op, float percent)
{
    static int oldPercent = 0;
    static int oldOp = -1;
    bool change = false;

    if (op != oldOp) {
        oldOp = op;
        oldPercent = (int)floor(percent);
        change = true;
    } else if(oldPercent != (int)floor(percent)) {
        oldPercent = (int)floor(percent);
        change = true;
    }

    if(change) PostMessage(hMainDialog, WM_UPDATE_PROGRESS, (WPARAM)op, (LPARAM)oldPercent);
}

// CEndlessUsbToolDlg dialog

BEGIN_DHTML_EVENT_MAP(CEndlessUsbToolDlg)
	// For dragging the window
	DHTML_EVENT_CLASS(DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN, _T(CLASS_PAGE_HEADER_TITLE), OnHtmlMouseDown)

	// First Page Handlers		
	DHTML_EVENT_ELEMENT(DISPID_HTMLELEMENTEVENTS_ONCLICK, _T(ELEMENT_LEFT_PANEL), OnTryEndlessSelected)
	DHTML_EVENT_ELEMENT(DISPID_HTMLELEMENTEVENTS_ONCLICK, _T(ELEMENT_RIGHT_PANEL), OnInstallEndlessSelected)
	DHTML_EVENT_ONCHANGE(_T(ELEMENT_LANGUAGE_SELECT), OnLanguageChanged)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_COMPARE_OPTIONS), OnLinkClicked)

	// Select File Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELFILE_PREV_BUTTON), OnSelectFilePreviousClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELFILE_NEXT_BUTTON), OnSelectFileNextClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELFILE_BUTTON), OnSelectFileButtonClicked)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_FILES_SELECT), OnSelectedImageFileChanged)

	// Select USB Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELUSB_PREV_BUTTON), OnSelectUSBPreviousClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELUSB_NEXT_BUTTON), OnSelectUSBNextClicked)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_SELUSB_USB_DRIVES), OnSelectedUSBDiskChanged)

	// Installing Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SECUREBOOT_HOWTO), OnLinkClicked)

	// Thank You Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_CLOSE_BUTTON), OnCloseAppClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SECUREBOOT_HOWTO2), OnLinkClicked)

END_DHTML_EVENT_MAP()

BEGIN_DISPATCH_MAP(CEndlessUsbToolDlg, CDHtmlDialog)
    DISP_FUNCTION(CEndlessUsbToolDlg, "Debug", JavascriptDebug, VT_EMPTY, VTS_BSTR)
END_DISPATCH_MAP()


CEndlessUsbToolDlg::CEndlessUsbToolDlg(CWnd* pParent /*=NULL*/)
    : CDHtmlDialog(IDD_ENDLESSUSBTOOL_DIALOG, IDR_HTML_ENDLESSUSBTOOL_DIALOG, pParent),
    m_selectedLocale(NULL),
    m_fullInstall(false),
    m_localizationFile(""),
    m_shellNotificationsRegister(0),
    m_lastDevicesRefresh(0),
    m_spHtmlDoc3(NULL),
    m_lgpSet(FALSE),
    m_lgpExistingKey(FALSE),
    m_FilesChangedHandle(INVALID_HANDLE_VALUE),
    m_fileScanThread(INVALID_HANDLE_VALUE),
    m_formatThread(INVALID_HANDLE_VALUE),
    m_spStatusElem(NULL),
    m_spWindowElem(NULL),
    m_dispexWindow(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_closingApplicationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

void CEndlessUsbToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CEndlessUsbToolDlg, CDHtmlDialog)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// Browse navigation handling methods
void CEndlessUsbToolDlg::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
	CDHtmlDialog::OnDocumentComplete(pDisp, szUrl);

	uprintf("OnDocumentComplete '%ls'", szUrl);

	if (this->m_spHtmlDoc == NULL) {
		uprintf("CEndlessUsbToolDlg::OnDocumentComplete m_spHtmlDoc==NULL");
		return;
	}

	if (m_spHtmlDoc3 == NULL) {
		HRESULT hr = m_spHtmlDoc->QueryInterface(IID_IHTMLDocument3, (void**)&m_spHtmlDoc3);
		IFFALSE_GOTOERROR(SUCCEEDED(hr) && m_spHtmlDoc3 != NULL, "Error when querying IID_IHTMLDocument3 interface.");
	}

	AddLanguagesToUI();
	ApplyRufusLocalization(); //apply_localization(IDD_ENDLESSUSBTOOL_DIALOG, GetSafeHwnd());

	return;
error:
	uprintf("OnDocumentComplete Exit with error");
}

void CEndlessUsbToolDlg::InitRufus()
{
    // RADU: try to remove the need for this
    hMainDialog = m_hWnd;
    hDeviceList = m_hWnd;
    hBootType = m_hWnd;
    hPartitionScheme = m_hWnd;
    hFileSystem = m_hWnd;
    hClusterSize = m_hWnd;
    hLabel = m_hWnd;
    hNBPasses = m_hWnd;
    hDiskID = m_hWnd;
    hInfo = m_hWnd;

    hMainInstance = AfxGetResourceHandle();

    // Retrieve the current application directory as well as the system & sysnative dirs
    if (GetCurrentDirectoryU(sizeof(app_dir), app_dir) == 0) {
        uprintf("Could not get current directory: %s", WindowsErrorString());
        app_dir[0] = 0;
    }
    if (GetSystemDirectoryU(system_dir, sizeof(system_dir)) == 0) {
        uprintf("Could not get system directory: %s", WindowsErrorString());
        safe_strcpy(system_dir, sizeof(system_dir), "C:\\Windows\\System32");
    }
    // Construct Sysnative ourselves as there is no GetSysnativeDirectory() call
    // By default (64bit app running on 64 bit OS or 32 bit app running on 32 bit OS)
    // Sysnative and System32 are the same
    safe_strcpy(sysnative_dir, sizeof(sysnative_dir), system_dir);
    // But if the app is 32 bit and the OS is 64 bit, Sysnative must differ from System32
#if (!defined(_WIN64) && !defined(BUILD64))
    if (is_x64()) {
        if (GetSystemWindowsDirectoryU(sysnative_dir, sizeof(sysnative_dir)) == 0) {
            uprintf("Could not get Windows directory: %s", WindowsErrorString());
            safe_strcpy(sysnative_dir, sizeof(sysnative_dir), "C:\\Windows");
        }
        safe_strcat(sysnative_dir, sizeof(sysnative_dir), "\\Sysnative");
    }
#endif

    // Initialize COM for folder selection
    IGNORE_RETVAL(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));

    // Some dialogs have Rich Edit controls and won't display without this
    if (GetLibraryHandle("Riched20") == NULL) {
        uprintf("Could not load RichEdit library - some dialogs may not display: %s\n", WindowsErrorString());
    }

    // Set the Windows version
    GetWindowsVersion();

    // We use local group policies rather than direct registry manipulation
    // 0x9e disables removable and fixed drive notifications
    m_lgpSet = SetLGP(FALSE, &m_lgpExistingKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", "NoDriveTypeAutorun", 0x9e);

    if (nWindowsVersion > WINDOWS_XP) {
        // Re-enable AutoMount if needed
        if (!GetAutoMount(&m_automount)) {
            uprintf("Could not get AutoMount status");
            m_automount = TRUE;	// So that we don't try to change its status on exit
        } else if (!m_automount) {
            uprintf("AutoMount was detected as disabled - temporary re-enabling it");
            if (!SetAutoMount(TRUE)) {
                uprintf("Failed to enable AutoMount");
            }
        }
    }
    srand((unsigned int)_GetTickCount64());
}

// The scanning process can be blocking for message processing => use a thread
DWORD WINAPI CEndlessUsbToolDlg::RufusISOScanThread(LPVOID param)
{

    if (image_path == NULL)
        goto out;
    PrintInfoDebug(0, MSG_202);

    img_report.is_iso = (BOOLEAN)ExtractISO(image_path, "", TRUE);
    img_report.is_bootable_img = (BOOLEAN)IsBootableImage(image_path);
    if (!img_report.is_iso && !img_report.is_bootable_img) {
        // Failed to scan image
        PrintInfoDebug(0, MSG_203);
        safe_free(image_path);
        PrintStatus(0, MSG_086);
        goto out;
    }

    if (img_report.is_bootable_img) {
        uprintf("  Image is a %sbootable %s image",
            (img_report.compression_type != BLED_COMPRESSION_NONE) ? "compressed " : "", img_report.is_vhd ? "VHD" : "disk");
    }

out:
    ::SendMessage(hMainDialog, WM_FINISHED_IMG_SCANNING, 0, 0);

    PrintInfo(0, MSG_210);
    ExitThread(0);
}


// CEndlessUsbToolDlg message handlers

BOOL CEndlessUsbToolDlg::OnInitDialog()
{
	CDHtmlDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//refuse dragging file into this dialog
	m_pBrowserApp->put_RegisterAsDropTarget(VARIANT_FALSE);
	m_pBrowserApp->put_RegisterAsBrowser(FALSE);
    SetHostFlags(DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_NO3DBORDER);
    // Expose logging method to Javascript
    EnableAutomation();
    LPDISPATCH pDisp = GetIDispatch(FALSE);
    SetExternalDispatch(pDisp);
    //Disable JavaScript errors
    m_pBrowserApp->put_Silent(VARIANT_TRUE);

	// Make round corners
	//// Remove caption and border
	SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE)
		& (~(WS_CAPTION | WS_BORDER)));

	//  Get the rectangle
	CRect rect;
	GetWindowRect(&rect);
	int w = rect.Width();
	int h = rect.Height();

	int radius = 19;
	CRgn rgn;
	rgn.CreateRoundRectRgn(0, 0, w, h, radius, radius);
	
	//  Set the window region
	SetWindowRgn(static_cast<HRGN>(rgn.GetSafeHandle()), TRUE);

	// Init localization before doing anything else
	LoadLocalizationData();

	InitRufus();

    // For Rufus
    CheckDlgButton(IDC_BOOT, BST_CHECKED);
    CheckDlgButton(IDC_QUICK_FORMAT, BST_CHECKED);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CEndlessUsbToolDlg::Uninit()
{
    // wait for File scanning thread
    SetEvent(m_closingApplicationEvent);
    if (m_fileScanThread != INVALID_HANDLE_VALUE) {
        uprintf("Waiting for scan files thread.");
        WaitForSingleObject(m_fileScanThread, INFINITE);
        m_fileScanThread = INVALID_HANDLE_VALUE;
    }
    CloseHandle(m_closingApplicationEvent);

    // unregister from notifications and delete drive list related memory
    LeavingDevicesPage();

    StrArrayDestroy(&DriveID);
    StrArrayDestroy(&DriveLabel);

    // revert settings we changed
    if (m_lgpSet) {
        SetLGP(TRUE, &m_lgpExistingKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", "NoDriveTypeAutorun", 0);
    }
    if ((nWindowsVersion > WINDOWS_XP) && (!m_automount) && (!SetAutoMount(FALSE))) {
        uprintf("Failed to restore AutoMount to disabled");
    }

    // delete image file entries related memory
    CString currentPath;
    pFileImageEntry_t currentEntry = NULL;
    for (POSITION position = m_imageFiles.GetStartPosition(); position != NULL; ) {
        m_imageFiles.GetNextAssoc(position, currentPath, currentEntry);
        delete currentEntry;
    }
    m_imageFiles.RemoveAll();

    // uninit localization memory
    exit_localization();

    safe_free(image_path);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEndlessUsbToolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDHtmlDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEndlessUsbToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CEndlessUsbToolDlg::PreTranslateMessage(MSG* pMsg)
{
    // disable any key
    if (pMsg->message == WM_KEYDOWN) {
        return TRUE;
    }

    return CDHtmlDialog::PreTranslateMessage(pMsg);
}

void CEndlessUsbToolDlg::JavascriptDebug(LPCTSTR debugMsg)
{
    CComBSTR msg = debugMsg;
    uprintf("Javascript - [%ls]", msg);
}

/*
* Device Refresh Timer
*/
void CALLBACK CEndlessUsbToolDlg::RefreshTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    switch (idEvent) {
    case TID_REFRESH_TIMER:
        // DO NOT USE WM_DEVICECHANGE - IT MAY BE FILTERED OUT BY WINDOWS!
        ::PostMessage(hWnd, UM_MEDIA_CHANGE, 0, 0);
        break;
    case TID_UPDATE_FILES:
        ::PostMessage(hWnd, WM_FILES_CHANGED, 0, 0);
        break;
    default:
        uprintf("Timer not handled [%d]", idEvent);
        break;
    }
}

LRESULT CEndlessUsbToolDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message >= CB_GETEDITSEL && message < CB_MSGMAX) {
        CComPtr<IHTMLElement> pElement;
        CComPtr<IHTMLSelectElement> selectElement;
        CComPtr<IHTMLOptionElement> optionElement;
        HRESULT hr;

        hr = m_spHtmlDoc3->getElementById(CComBSTR(ELEMENT_SELUSB_USB_DRIVES), &pElement);
        IFFALSE_GOTOERROR(SUCCEEDED(hr) && pElement != NULL, "Error when querying for languages HTML element.");

        hr = pElement.QueryInterface<IHTMLSelectElement>(&selectElement);
        IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for IHTMLSelectElement interface");

        switch (message) {
        case CB_RESETCONTENT:
        {
            hr = selectElement->put_length(0);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error clearing elements from USB drive list.");
            break;
        }

        case CB_ADDSTRING:
        {
            CComBSTR text = (wchar_t*)lParam;
            long index = 0;
            hr = AddEntryToSelect(selectElement, text, text, &index);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error adding item in USB drive list.");
            return index;
        }

        case CB_SETCURSEL:
        case CB_SETITEMDATA:
        case CB_GETITEMDATA:
        {
            CComPtr<IDispatch> pDispatch;
            CComBSTR valueBSTR;
            long index = (long)wParam;
            long value = (long)lParam;

            hr = selectElement->item(CComVariant(index), CComVariant(0), &pDispatch);
            IFFALSE_GOTOERROR(SUCCEEDED(hr) && pDispatch != NULL, "Error when querying for element at requested index.");

            hr = pDispatch.QueryInterface<IHTMLOptionElement>(&optionElement);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error when querying for IHTMLOptionElement interface");

            if (message == CB_SETITEMDATA) {
                hr = ::VarBstrFromI4(value, LOCALE_SYSTEM_DEFAULT, 0, &valueBSTR);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error converting int value to BSTR.");

                hr = optionElement->put_value(valueBSTR);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error setting value for requested element.");
            }
            else if (message == CB_GETITEMDATA) {
                hr = optionElement->get_value(&valueBSTR);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error getting value for requested element.");

                hr = ::VarI4FromStr(valueBSTR, LOCALE_SYSTEM_DEFAULT, 0, &value);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error converting BSTR to int value.");
                return value;
            }
            else if (message == CB_SETCURSEL) {
                hr = optionElement->put_selected(VARIANT_TRUE);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error setting selected for requested element.");
            }

            break;
        }

        case CB_GETCURSEL:
        {
            long index = -1;
            hr = selectElement->get_selectedIndex(&index);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for selected index.");

            return index;
        }

        case CB_GETCOUNT:
        {
            long count = 0;
            hr = selectElement->get_length(&count);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for list count.");

            return count;
        }

        // Ignore
        case CB_SETDROPPEDWIDTH:
            break;

        default:
            luprintf("Untreated CB message %d(0x%X)", message, message);
            break;
        }

        return 0;

    error:
        uprintf("CEndlessUsbToolDlg::WindowProc called with %d[0x%X] (%d, %d); FAILURE %x", message, message, lParam, wParam, hr);
        return -1;
    } else {
        switch (message) {
        case WM_SETTEXT:
        {
            if (m_spStatusElem == NULL) {
                HRESULT hr = m_spHtmlDoc3->getElementById(_T(ELEMENT_INSTALL_STATUS), &m_spStatusElem);
                IFFALSE_BREAK(SUCCEEDED(hr) && m_spStatusElem != NULL, "Error querying for status element.");
            }
            CComBSTR bstrStatus = (wchar_t*)lParam;
            m_spStatusElem->put_innerText(bstrStatus);
            break;
        }
        case WM_CLIENTSHUTDOWN:
        case WM_QUERYENDSESSION:
        case WM_ENDSESSION:
            // TODO: Do we want to use ShutdownBlockReasonCreate() in Vista and later to stop
            // forced shutdown? See https://msdn.microsoft.com/en-us/library/ms700677.aspx
            if (m_formatThread != INVALID_HANDLE_VALUE) {
                // WM_QUERYENDSESSION uses this value to prevent shutdown
                return (INT_PTR)TRUE;
            }
            PostMessage(WM_COMMAND, (WPARAM)IDCANCEL, (LPARAM)0);
            break;

        case UM_MEDIA_CHANGE:
            wParam = DBT_CUSTOMEVENT;
            // Fall through
        case WM_DEVICECHANGE:
            // The Windows hotplug subsystem sucks. Among other things, if you insert a GPT partitioned
            // USB drive with zero partitions, the only device messages you will get are a stream of
            // DBT_DEVNODES_CHANGED and that's it. But those messages are also issued when you get a
            // DBT_DEVICEARRIVAL and DBT_DEVICEREMOVECOMPLETE, and there's a whole slew of them so we
            // can't really issue a refresh for each one we receive
            // What we do then is arm a timer on DBT_DEVNODES_CHANGED, if it's been more than 1 second
            // since last refresh/arm timer, and have that timer send DBT_CUSTOMEVENT when it expires.
            // DO *NOT* USE WM_DEVICECHANGE AS THE MESSAGE FROM THE TIMER PROC, as it may be filtered!
            // For instance filtering will occur when (un)plugging in a FreeBSD UFD on Windows 8.
            // Instead, use a custom user message, such as UM_MEDIA_CHANGE, to set DBT_CUSTOMEVENT.
            if (m_formatThread == INVALID_HANDLE_VALUE) {
                switch (wParam) {
                case DBT_DEVICEARRIVAL:
                case DBT_DEVICEREMOVECOMPLETE:
                case DBT_CUSTOMEVENT:	// Sent by our timer refresh function or for card reader media change
                    m_lastDevicesRefresh = _GetTickCount64();
                    KillTimer(TID_REFRESH_TIMER);
                    GetUSBDevices((DWORD)ComboBox_GetItemData(hDeviceList, ComboBox_GetCurSel(hDeviceList)));
                    OnSelectedUSBDiskChanged(NULL);
                    return (INT_PTR)TRUE;
                case DBT_DEVNODES_CHANGED:
                    // If it's been more than a second since last device refresh, arm a refresh timer
                    if (_GetTickCount64() > m_lastDevicesRefresh + 1000) {
                        m_lastDevicesRefresh = _GetTickCount64();
                        SetTimer(TID_REFRESH_TIMER, 1000, RefreshTimer);
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        case UM_PROGRESS_INIT:
        {
            uprintf("Started to scan the provided image.");
            break;
        }
        case WM_FINISHED_IMG_SCANNING:
        {
            uprintf("Image scanning done.");
            if (!img_report.is_bootable_img ||
                (img_report.compression_type != BLED_COMPRESSION_GZIP && img_report.compression_type != BLED_COMPRESSION_XZ)) {
                uprintf("FAILURE: selected image is not bootable and compresion is different frome what is expected: xz/gz");
            } else {
                uprintf("Bootable image selected with correct format.");

                // Start formatting
                FormatStatus = 0;

                int nDeviceIndex = ComboBox_GetCurSel(hDeviceList);
                if (nDeviceIndex != CB_ERR) {
                    DWORD DeviceNum = (DWORD)ComboBox_GetItemData(hDeviceList, nDeviceIndex);
                    m_formatThread = CreateThread(NULL, 0, FormatThread, (LPVOID)(uintptr_t)DeviceNum, 0, NULL);
                    if (m_formatThread == NULL) {
                        uprintf("Unable to start formatting thread");
                        FormatStatus = ERROR_SEVERITY_ERROR | FAC(FACILITY_STORAGE) | APPERR(ERROR_CANT_START_THREAD);
                        PostMessage(UM_FORMAT_COMPLETED, (WPARAM)FALSE, 0);
                    }
                }
            }

            break;
        }

        case WM_UPDATE_PROGRESS:
        {
            HRESULT hr;
            int op = (int)wParam;
            int percent = (int)lParam;
            if (m_spWindowElem == NULL) {
                hr = m_spHtmlDoc->get_parentWindow(&m_spWindowElem);
                IFFALSE_BREAK(SUCCEEDED(hr) && m_spWindowElem != NULL, "Error querying for parent window.");
            }
            if (m_dispWindow == NULL) {
                hr = m_spWindowElem->QueryInterface(&m_dispWindow);
                IFFALSE_BREAK(SUCCEEDED(hr) && m_dispWindow != NULL, "Error querying for CComDispatchDriver.");
            }
            if (m_dispexWindow == NULL) {
                hr = m_spWindowElem->QueryInterface(&m_dispexWindow);
                IFFALSE_BREAK(SUCCEEDED(hr) && m_dispexWindow != NULL, "Error querying for IDispatchEx.");
            }

            DISPID dispidSetProgress = -1;
            hr = m_dispexWindow->GetDispID(_T("setProgress"), fdexNameCaseSensitive, &dispidSetProgress);
            IFFALSE_BREAK(SUCCEEDED(hr), "Error getting setProgress dispid");
            hr = m_dispWindow.Invoke1(dispidSetProgress, &CComVariant(percent));
            IFFALSE_BREAK(SUCCEEDED(hr), "Error when calling set progress.");
            break;
        }
        case UM_PROGRESS_EXIT:
        case UM_NO_UPDATE:
            luprintf("Untreated Rufus UM message %d(0x%X)", message, message);
            break;
        case UM_FORMAT_COMPLETED:
        {
            if (!IS_ERROR(FormatStatus)) {
                PrintInfo(0, MSG_210);
                m_formatThread = INVALID_HANDLE_VALUE;
                ChangePage(_T("InstallingPage"), _T("ThankYouPage"));
            }
            else if (SCODE_CODE(FormatStatus) == ERROR_CANCELLED) {
                PrintInfo(0, MSG_211);
                Uninit();
                AfxPostQuitMessage(0);
                //Notification(MSG_INFO, NULL, lmprintf(MSG_211), lmprintf(MSG_041));
            }
            else {
                PrintInfo(0, MSG_212);
                //Notification(MSG_ERROR, NULL, lmprintf(MSG_042), lmprintf(MSG_043, StrError(FormatStatus, FALSE)));
            }
            FormatStatus = 0;
            break;
        }
        case WM_FILES_CHANGED:
        {
            UpdateFileEntries();
            break;
        }
        default:
            //luprintf("Untreated message %d(0x%X)", message, message);
            break;
        }
    }

    return CDHtmlDialog::WindowProc(message, wParam, lParam);
}

// Disable context menu
STDMETHODIMP CEndlessUsbToolDlg::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
	return S_OK;
}

// prevent refresh
STDMETHODIMP CEndlessUsbToolDlg::TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID)
{
	if (lpMsg && lpMsg->message == WM_KEYDOWN &&
		(lpMsg->wParam == VK_F5 ||
			lpMsg->wParam == VK_CONTROL))
	{
		return S_OK;
	}
	return CDHtmlDialog::TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID);
}

// Drag window
HRESULT CEndlessUsbToolDlg::OnHtmlMouseDown(IHTMLElement* pElement)
{
	POINT pt;
	GetCursorPos(&pt);
	SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));

	return S_OK;
}

// Private methods
void CEndlessUsbToolDlg::LoadLocalizationData()
{
	const char* rufus_loc = "rufus.loc";
	int lcid = GetUserDefaultUILanguage(); //0x0418;
	BYTE *loc_data;
	DWORD loc_size, size;
	char tmp_path[MAX_PATH] = "", loc_file[MAX_PATH] = "";
	HANDLE hFile = NULL;

	init_localization();

	loc_data = (BYTE*)GetResource(hMainInstance, MAKEINTRESOURCEA(IDR_LC_ENDLESS_LOC), _RT_RCDATA, "embedded.loc", &loc_size, FALSE);
	if ((GetTempPathU(sizeof(tmp_path), tmp_path) == 0)
		|| (GetTempFileNameU(tmp_path, APPLICATION_NAME, 0, m_localizationFile) == 0)
		|| (m_localizationFile[0] == 0)) {
		// Last ditch effort to get a loc file - just extract it to the current directory
		safe_strcpy(m_localizationFile, sizeof(m_localizationFile), rufus_loc);
	}

	hFile = CreateFileU(m_localizationFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if ((hFile == INVALID_HANDLE_VALUE) || (!WriteFileWithRetry(hFile, loc_data, loc_size, &size, WRITE_RETRIES))) {
		uprintf("localization: unable to extract '%s': %s", m_localizationFile, WindowsErrorString());
		safe_closehandle(hFile);
		goto error;
	}
	uprintf("localization: extracted data to '%s'", m_localizationFile);
	safe_closehandle(hFile);

	if ((!get_supported_locales(m_localizationFile))
		|| ((m_selectedLocale = get_locale_from_lcid(lcid, TRUE)) == NULL)) {
		uprintf("FATAL: Could not access locale!");
		goto error;
	}
	selected_langid = get_language_id(m_selectedLocale);

	if (get_loc_data_file(m_localizationFile, m_selectedLocale))
		uprintf("Save locale to settings?");

	return;

error:
	uprintf("Exiting with error");
	MessageBoxU(NULL, "The locale data is missing or invalid. This application will now exit.", "Fatal error", MB_ICONSTOP | MB_SYSTEMMODAL);

}

void CEndlessUsbToolDlg::ApplyRufusLocalization()
{
	loc_cmd* lcmd = NULL;
	int dlg_id = IDD_ENDLESSUSBTOOL_DIALOG;
	HWND hCtrl = NULL;
	HWND hDlg = GetSafeHwnd();

	if (this->m_spHtmlDoc == NULL) {
		uprintf("CEndlessUsbToolDlg::ApplyRufusLocalization m_spHtmlDoc==NULL");
		return;
	}

	HRESULT hr;
	CComPtr<IHTMLElement> pElement = NULL;

	// go through the ids and update the text
	list_for_each_entry(lcmd, &loc_dlg[dlg_id - IDD_DIALOG].list, loc_cmd, list) {
		if (lcmd->command <= LC_TEXT) {
			if (lcmd->ctrl_id == dlg_id) {
				luprint("Updating dialog title not implemented");
			} else {
				pElement = NULL;
				hr = m_spHtmlDoc3->getElementById(CComBSTR(lcmd->txt[0]), &pElement);
				if (FAILED(hr) || pElement == NULL) {
					luprintf("control '%s' is not part of dialog '%s'\n", lcmd->txt[0], control_id[dlg_id - IDD_DIALOG].name);
				}
			}
		}

		switch (lcmd->command) {
		case LC_TEXT:
			if (pElement != NULL) {
				if ((lcmd->txt[1] != NULL) && (lcmd->txt[1][0] != 0)) {
					hr = pElement->put_innerText(UTF8ToBSTR(lcmd->txt[1]));
					if (FAILED(hr)) {
						luprintf("error when updating control '%s', hr='%x'\n", lcmd->txt[0], hr);
					}
				}
			}
			break;
		case LC_MOVE:
			luprint("LC_MOVE not implemented");
			break;
		case LC_SIZE:
			luprint("LC_SIZE not implemented");
			break;
		}
	}

	return;
}

void CEndlessUsbToolDlg::AddLanguagesToUI()
{
	loc_cmd* lcmd = NULL;
	CComPtr<IHTMLElement> pElement;
	CComPtr<IHTMLSelectElement> selectElement;
	CComPtr<IHTMLOptionElement> optionElement;
	HRESULT hr;

	hr = m_spHtmlDoc3->getElementById(CComBSTR(ELEMENT_LANGUAGE_SELECT), &pElement);
	IFFALSE_RETURN(SUCCEEDED(hr) && pElement != NULL, "Error when querying for languages HTML element.");

	hr = pElement.QueryInterface<IHTMLSelectElement>(&selectElement);
	IFFALSE_RETURN(SUCCEEDED(hr), "Error querying for IHTMLSelectElement interface");

	hr = selectElement->put_length(0);

	int index = 0;
	// Add languages to dropdown and apply localization
	list_for_each_entry(lcmd, &locale_list, loc_cmd, list) {
		luprintf("Language available : %s", lcmd->txt[1]);

		hr = AddEntryToSelect(selectElement, UTF8ToBSTR(lcmd->txt[0]), UTF8ToBSTR(lcmd->txt[1]), NULL, m_selectedLocale == lcmd ? TRUE : FALSE);
		IFFALSE_RETURN(SUCCEEDED(hr), "Error adding the new option element to the select element");
	}
}


void CEndlessUsbToolDlg::ChangePage(PCTSTR oldPage, PCTSTR newPage)
{
	CComPtr<IHTMLElement> pOldPage = NULL, pNewPage = NULL;
	HRESULT hr;

	hr = m_spHtmlDoc3->getElementById(CComBSTR(oldPage), &pOldPage);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pOldPage != NULL, "Error querying for visible page.");

	hr = m_spHtmlDoc3->getElementById(CComBSTR(newPage), &pNewPage);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pOldPage != NULL, "Error querying for new page.");

	hr = pOldPage->put_className(CComBSTR("WizardPage hidden"));
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pOldPage != NULL, "Error when updating the classname for the visible page.");
	pNewPage->put_className(CComBSTR("WizardPage"));
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pOldPage != NULL, "Error when updating the classname for the new page.");

	return;

error:
	// RADU: LOG the HR value
	return;
}

HRESULT CEndlessUsbToolDlg::AddEntryToSelect(PCTSTR selectId, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected)
{
    CComPtr<IHTMLElement> pElement;
    CComPtr<IHTMLSelectElement> selectElement;
    HRESULT hr;

    hr = m_spHtmlDoc3->getElementById(CComBSTR(selectId), &pElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr) && pElement != NULL, "Error when querying for select element.");

    hr = pElement.QueryInterface<IHTMLSelectElement>(&selectElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for IHTMLSelectElement interface");

    return AddEntryToSelect(selectElement, value, text, outIndex, selected);

error:
    luprintf("AddEntryToSelect error on select [%s] and entry (%s, %s)", selectId, text, value);
    return hr;
}

HRESULT CEndlessUsbToolDlg::AddEntryToSelect(CComPtr<IHTMLSelectElement> &selectElem, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected)
{
    CComPtr<IHTMLElement> pElement;
    CComPtr<IHTMLOptionElement> optionElement;
    HRESULT hr;

    hr = m_spHtmlDoc->createElement(CComBSTR("option"), &pElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error when creating the option element");

    hr = pElement.QueryInterface<IHTMLOptionElement>(&optionElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error when querying for IHTMLOptionElement interface");

    optionElement->put_selected(selected);
    optionElement->put_value(value);
    optionElement->put_text(text);

    long length;
    hr = selectElem->get_length(&length);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying the select element length.");
    if (outIndex != NULL) {
        *outIndex = length;
    }

    hr = selectElem->add(pElement, CComVariant(length));
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error adding the new option element to the select element");

    return S_OK;
error:
    luprintf("AddEntryToSelect error on adding entry (%ls, %ls)", text, value);
    return hr;
}

//HRESULT CEndlessUsbToolDlg::OnHtmlSelectStart(IHTMLElement* pElement)
//{
//	POINT pt;
//	GetCursorPos(&pt);
//	::SendMessage(m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
//
//	//do not pass the event to the IE server/JavaScript
//	return S_FALSE;
//}


// First Page Handlers
HRESULT CEndlessUsbToolDlg::OnTryEndlessSelected(IHTMLElement* pElement)
{
	m_fullInstall = false;
    UpdateFileEntries();
	ChangePage(_T("FirstPage"), _T("SelectFilePage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnInstallEndlessSelected(IHTMLElement* pElement)
{
	m_fullInstall = true;
    UpdateFileEntries();
	ChangePage(_T("FirstPage"), _T("SelectFilePage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnLinkClicked(IHTMLElement* pElement)
{
	AfxMessageBox(_T("Not implemented yet"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnLanguageChanged(IHTMLElement* pElement)
{
	CComPtr<IHTMLSelectElement> selectElement;
	CComBSTR selectedValue;

	HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement, (void**)&selectElement);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && selectElement != NULL, "Error querying for IHTMLSelectElement interface");

	hr = selectElement->get_value(&selectedValue);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && selectElement != NULL, "Error getting selected language value");

	char* p = _com_util::ConvertBSTRToString(selectedValue);
	m_selectedLocale = get_locale_from_name(p, TRUE);
	delete[] p;
	selected_langid = get_language_id(m_selectedLocale);

	reinit_localization();
	msg_table = NULL;

	if (get_loc_data_file(m_localizationFile, m_selectedLocale))
		uprintf("Save locale to settings?");

	ApplyRufusLocalization();

	return S_OK;

error:
	//RADU: what to do on error?
	return S_OK;
}

void CEndlessUsbToolDlg::UpdateFileEntries()
{
    CComPtr<IHTMLElement> pElement;
    CComPtr<IHTMLSelectElement> selectElement;
    HRESULT hr;
    WIN32_FIND_DATA findFileData;
    HANDLE findFilesHandle = FindFirstFile(_T("*.*"), &findFileData);
    DWORD error = GetLastError();
    POSITION position;
    pFileImageEntry_t currentEntry = NULL;
    CString currentPath;
    BOOL fileAccessException = false;

    // start the change notifications thread if not already started
    if (m_fileScanThread == INVALID_HANDLE_VALUE) {
        m_fileScanThread = CreateThread(NULL, 0, CEndlessUsbToolDlg::FileScanThread, (LPVOID)m_closingApplicationEvent, 0, NULL);
    }

    // get needed HTML elements
    hr = m_spHtmlDoc3->getElementById(CComBSTR(ELEMENT_FILES_SELECT), &pElement);
    IFFALSE_RETURN(SUCCEEDED(hr) && pElement != NULL, "Error when querying for languages HTML element.");

    hr = pElement.QueryInterface<IHTMLSelectElement>(&selectElement);
    IFFALSE_RETURN(SUCCEEDED(hr), "Error querying for IHTMLSelectElement interface");

    if (m_imageFiles.IsEmpty()) {
        hr = selectElement->put_length(0);
        IFFALSE_RETURN(SUCCEEDED(hr), "Error removing all elements from HTML element");
    }

    // mark all files as not present
    for (position = m_imageFiles.GetStartPosition(); position != NULL; ) {
        m_imageFiles.GetNextAssoc(position, currentPath, currentEntry);
        currentEntry->stillPresent = FALSE;
    }

    if (findFilesHandle == INVALID_HANDLE_VALUE) {
        uprintf("UpdateFileEntries: No files found in current directory [%s]", app_dir);
        goto checkEntries;
    }

    do {
        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
        CString currentFile = findFileData.cFileName;
        CString extension = currentFile.Right(currentFile.GetLength() - currentFile.ReverseFind('.'));
        if (extension != L".gz" && extension != L".xz" && extension != L".img") continue;

        if (!PathFileExists(findFileData.cFileName)) continue;

        try {
            CFile file(findFileData.cFileName, CFile::modeRead);
            CString displayText = file.GetFileName() + " - ";
            displayText += SizeToHumanReadable(file.GetLength(), FALSE, use_fake_units);

            pFileImageEntry_t currentEntry = NULL;
            if (!m_imageFiles.Lookup(file.GetFilePath(), currentEntry)) {
                currentEntry = new FileImageEntry_t;
                currentEntry->autoAdded = TRUE;
                currentEntry->filePath = file.GetFilePath();
                AddEntryToSelect(selectElement, CComBSTR(currentEntry->filePath), CComBSTR(displayText), &currentEntry->htmlIndex, 0);
                IFFALSE_RETURN(SUCCEEDED(hr), "Error adding item in image file list.");

                m_imageFiles.SetAt(currentEntry->filePath, currentEntry);
                m_imageIndexToPath.AddTail(currentEntry->filePath);
            }
            currentEntry->stillPresent = TRUE;

            // RADU: do we need to care about the size?

        } catch (CFileException *ex) {
            uprintf("CFileException on file [%ls] with cause [%d] and OS error [%d]", findFileData.cFileName, ex->m_cause, ex->m_lOsError);
            ex->Delete();
            fileAccessException = TRUE;
        }
    } while (FindNextFile(findFilesHandle, &findFileData) != 0);

checkEntries:
    int htmlIndexChange = 0;
    for (position = m_imageIndexToPath.GetHeadPosition(); position != NULL; ) {
        POSITION currentPosition = position;
        currentPath = m_imageIndexToPath.GetNext(position);
        if (!m_imageFiles.Lookup(currentPath, currentEntry)) {
            uprintf("Error: path [%s] not found anymore, removing.");
            m_imageIndexToPath.RemoveAt(currentPosition);
            continue;
        }

        if (!currentEntry->autoAdded) {
            findFilesHandle = FindFirstFile(currentEntry->filePath, &findFileData);
            if (findFilesHandle != INVALID_HANDLE_VALUE) {
                currentEntry->stillPresent = TRUE;
            }
        }

        if (htmlIndexChange != 0) {
            uprintf("Changing HTML index with %d for %ls", htmlIndexChange, currentEntry->filePath);
            currentEntry->htmlIndex -= htmlIndexChange;
        }

        if (!currentEntry->stillPresent) {
            uprintf("Removing %ls", currentPath);
            selectElement->remove(currentEntry->htmlIndex);
            delete currentEntry;
            m_imageFiles.RemoveKey(currentPath);
            htmlIndexChange++;
            m_imageIndexToPath.RemoveAt(currentPosition);
        }
    }

    FindClose(findFilesHandle);

    // if there was an exception when accessing one of the files do a retry in 1 second
    if (fileAccessException) {
        SetTimer(TID_UPDATE_FILES, 1000, RefreshTimer);
    } else {
        KillTimer(TID_UPDATE_FILES);
    }

    // set selected image for Rufus code
    CComBSTR selectedValue;
    hr = selectElement->get_value(&selectedValue);
    IFFALSE_RETURN(SUCCEEDED(hr) && selectElement != NULL, "Error getting selected file value");

    safe_free(image_path);
    if (selectedValue != NULL) {
        image_path = _com_util::ConvertBSTRToString(selectedValue);
    }
}

DWORD WINAPI CEndlessUsbToolDlg::FileScanThread(void* param)
{
    DWORD error = 0;
    HANDLE handlesToWaitFor[2];
    DWORD changeNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME;
    //changeNotifyFilter |= FILE_NOTIFY_CHANGE_SIZE;

    handlesToWaitFor[0] = (HANDLE)param;
    handlesToWaitFor[1] = FindFirstChangeNotificationA(app_dir, FALSE, changeNotifyFilter);
    if (handlesToWaitFor[1] == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        uprintf("Error on FindFirstChangeNotificationA error=[%d]", error);
        return error;
    }

    while (TRUE)
    {
        DWORD dwWaitStatus = WaitForMultipleObjects(2, handlesToWaitFor, FALSE, INFINITE);
        switch (dwWaitStatus)
        {
        case WAIT_OBJECT_0:
            uprintf("FileScanThread: exit requested.");
            FindCloseChangeNotification(handlesToWaitFor[1]);
            return 0;
        case WAIT_OBJECT_0 + 1:
            ::PostMessage(hMainDialog, WM_FILES_CHANGED, 0, 0);
            if (FindNextChangeNotification(handlesToWaitFor[1]) == FALSE) {
                error = GetLastError();
                uprintf("FileScanThread ERROR: FindNextChangeNotification function failed with error [%d].", GetLastError());
                return error;
            }
            break;
        default:
            uprintf("FileScanThread: unhandled wait status [%d]", dwWaitStatus);
            return GetLastError();
        }
    }

    return 0;
}

// Select File Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectFilePreviousClicked(IHTMLElement* pElement)
{
	ChangePage(_T("SelectFilePage"), _T("FirstPage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectFileNextClicked(IHTMLElement* pElement)
{
    LPITEMIDLIST pidlDesktop = NULL;
    SHChangeNotifyEntry NotifyEntry;

    // RADU: move this to another thread
    GetUSBDevices(0);
    OnSelectedUSBDiskChanged(NULL);

    // Register MEDIA_INSERTED/MEDIA_REMOVED notifications for card readers
    if (SUCCEEDED(SHGetSpecialFolderLocation(0, CSIDL_DESKTOP, &pidlDesktop))) {
        NotifyEntry.pidl = pidlDesktop;
        NotifyEntry.fRecursive = TRUE;
        // NB: The following only works if the media is already formatted.
        // If you insert a blank card, notifications will not be sent... :(
        m_shellNotificationsRegister = SHChangeNotifyRegister(m_hWnd, 0x0001 | 0x0002 | 0x8000,
            SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED, UM_MEDIA_CHANGE, 1, &NotifyEntry);
    }

	ChangePage(_T("SelectFilePage"), _T("SelectUSBPage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectFileButtonClicked(IHTMLElement* pElement)
{
    AfxMessageBox(_T("Not implemented yet."));
    //EXT_DECL(img_ext, NULL, __VA_GROUP__("*.img;*.gz;*.xz"), __VA_GROUP__(lmprintf(MSG_095)));

    //char *image_path = FileDialog(FALSE, NULL, &img_ext, 0);
    //CString selectedFilePath(image_path);

    //CFile file(selectedFilePath, CFile::modeRead);

    //CString displayText = file.GetFileName() + " - ";
    //displayText += SizeToHumanReadable(file.GetLength(), FALSE, use_fake_units);
    //AddEntryToSelect(_T(ELEMENT_FILES_SELECT), CComBSTR(selectedFilePath), CComBSTR(displayText), NULL, 1);

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectedImageFileChanged(IHTMLElement* pElement)
{
    CComPtr<IHTMLSelectElement> selectElement;
    CComBSTR selectedValue;

    HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement, (void**)&selectElement);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && selectElement != NULL, "Error querying for IHTMLSelectElement interface", S_OK);

    hr = selectElement->get_value(&selectedValue);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && selectElement != NULL, "Error getting selected file value", S_OK);

    safe_free(image_path);
    image_path = _com_util::ConvertBSTRToString(selectedValue);
    uprintf("OnSelectedImageFileChanged to [%s]", image_path);

    return S_OK;
}

// Select USB Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectUSBPreviousClicked(IHTMLElement* pElement)
{
	LeavingDevicesPage();
	ChangePage(_T("SelectUSBPage"), _T("SelectFilePage"));

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectUSBNextClicked(IHTMLElement* pElement)
{
	LeavingDevicesPage();
	ChangePage(_T("SelectUSBPage"), _T("InstallingPage"));

    FormatStatus = 0;
    if (CreateThread(NULL, 0, CEndlessUsbToolDlg::RufusISOScanThread, NULL, 0, NULL) == NULL) {
        uprintf("Unable to start ISO scanning thread");
        FormatStatus = ERROR_SEVERITY_ERROR | FAC(FACILITY_STORAGE) | APPERR(ERROR_CANT_START_THREAD);
    }

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectedUSBDiskChanged(IHTMLElement* pElement)
{
    //int i;
    char fs_type[32];
    int deviceIndex = ComboBox_GetCurSel(hDeviceList);

    IFFALSE_GOTOERROR(deviceIndex >= 0, "Selected drive index is invalid.");

    memset(&SelectedDrive, 0, sizeof(SelectedDrive));
    SelectedDrive.DeviceNumber = (DWORD)ComboBox_GetItemData(hDeviceList, deviceIndex);
    GetDrivePartitionData(SelectedDrive.DeviceNumber, fs_type, sizeof(fs_type), FALSE);

    return S_OK;

error:
    return S_OK;
}


void CEndlessUsbToolDlg::LeavingDevicesPage()
{
    if (m_shellNotificationsRegister != 0) {
        SHChangeNotifyDeregister(m_shellNotificationsRegister);
        m_shellNotificationsRegister = 0;
    }
}

// Thank You Page Handlers
HRESULT CEndlessUsbToolDlg::OnCloseAppClicked(IHTMLElement* pElement)
{
	Uninit();
	AfxPostQuitMessage(0);

	return S_OK;
}

void CEndlessUsbToolDlg::OnClose()
{
    bool format_in_progress = m_formatThread != INVALID_HANDLE_VALUE;

    if (format_in_progress) {
        int result = MessageBoxExU(hMainDialog, lmprintf(MSG_105), lmprintf(MSG_049), MB_YESNO | MB_ICONWARNING | MB_IS_RTL, selected_langid);
        if (result == IDYES) {
            FormatStatus = ERROR_SEVERITY_ERROR | FAC(FACILITY_STORAGE) | ERROR_CANCELLED;
            PrintInfo(0, MSG_201);
            uprintf("Cancelling");
        }
        return;
    }
    Uninit();

	CDHtmlDialog::OnClose();
}
