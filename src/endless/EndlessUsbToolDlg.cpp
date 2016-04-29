
// EndlessUsbToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EndlessUsbTool.h"
#include "EndlessUsbToolDlg.h"
#include "afxdialogex.h"

// Rufus include files
extern "C" {
#include "rufus.h"
#include "missing.h"
#include "msapi_utf8.h"

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
};

#include "localization.h"
// End Rufus include files

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define IFFALSE_GOTOERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); goto error; }
#define IFFALSE_RETURN(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); return; }
#define IFFALSE_RETURN_VALUE(__CONDITION__, __ERRROR_MSG__, __RET__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); return __RET__; }

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
//Select USB page elements
#define ELEMENT_SELUSB_PREV_BUTTON      "SelectUSBPreviousButton"
#define ELEMENT_SELUSB_NEXT_BUTTON      "SelectUSBNextButton"
//Installing page elements
#define ELEMENT_SECUREBOOT_HOWTO        "SecureBootHowTo"
//Thank You page elements
#define ELEMENT_CLOSE_BUTTON            "CloseAppButton"
#define ELEMENT_SECUREBOOT_HOWTO2       "SecureBootHowToReminder"


// utility method for quick char* UTF8 conversion to BSTR
CComBSTR UTF8ToBSTR(const char *txt) {
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, txt, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[wchars_num];
	MultiByteToWideChar(CP_UTF8, 0, txt, -1, wstr, wchars_num);
	CComBSTR return_value(wstr);
	delete[] wstr;

	return return_value;
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

	// Select USB Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELUSB_PREV_BUTTON), OnSelectUSBPreviousClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELUSB_NEXT_BUTTON), OnSelectUSBNextClicked)

	// Installing Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SECUREBOOT_HOWTO), OnLinkClicked)

	// Thank You Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_CLOSE_BUTTON), OnCloseAppClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SECUREBOOT_HOWTO2), OnLinkClicked)

END_DHTML_EVENT_MAP()


CEndlessUsbToolDlg::CEndlessUsbToolDlg(CWnd* pParent /*=NULL*/)
	: CDHtmlDialog(IDD_ENDLESSUSBTOOL_DIALOG, IDR_HTML_ENDLESSUSBTOOL_DIALOG, pParent),
	m_selectedLocale(NULL),
	m_fullInstall(false),
	m_localizationFile(""),
	m_spHtmlDoc3(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
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

	// RADU: try to remove the need for this
	hMainDialog = m_hWnd;

	LoadLocalizationData();

	return TRUE;  // return TRUE  unless you set the focus to a control
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

	loc_data = (BYTE*)GetResource(AfxGetResourceHandle(), MAKEINTRESOURCEA(IDR_LC_ENDLESS_LOC), _RT_RCDATA, "embedded.loc", &loc_size, FALSE);
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

		pElement = NULL;
		hr = m_spHtmlDoc->createElement(CComBSTR("option"), &pElement);
		IFFALSE_RETURN(SUCCEEDED(hr), "Error when creating the option element");

		optionElement = NULL;
		hr = pElement.QueryInterface<IHTMLOptionElement>(&optionElement);
		IFFALSE_RETURN(SUCCEEDED(hr), "Error when querying for IHTMLSelectElement interface");

		optionElement->put_selected(m_selectedLocale == lcmd ? TRUE : FALSE);
		optionElement->put_value(UTF8ToBSTR(lcmd->txt[0]));
		optionElement->put_text(UTF8ToBSTR(lcmd->txt[1]));

		hr = selectElement->add(pElement, CComVariant(index++));
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
	ChangePage(_T("FirstPage"), _T("SelectFilePage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnInstallEndlessSelected(IHTMLElement* pElement)
{
	m_fullInstall = true;
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

// Select File Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectFilePreviousClicked(IHTMLElement* pElement)
{
	ChangePage(_T("SelectFilePage"), _T("FirstPage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectFileNextClicked(IHTMLElement* pElement)
{
	ChangePage(_T("SelectFilePage"), _T("SelectUSBPage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectFileButtonClicked(IHTMLElement* pElement)
{
	AfxMessageBox(_T("Not implemented yet"));

	return S_OK;
}

// Select USB Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectUSBPreviousClicked(IHTMLElement* pElement)
{
	ChangePage(_T("SelectUSBPage"), _T("SelectFilePage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectUSBNextClicked(IHTMLElement* pElement)
{
	ChangePage(_T("SelectUSBPage"), _T("InstallingPage"));

	return S_OK;
}

// Thank You Page Handlers
HRESULT CEndlessUsbToolDlg::OnCloseAppClicked(IHTMLElement* pElement)
{
	AfxPostQuitMessage(0);

	return S_OK;
}

void CEndlessUsbToolDlg::OnClose()
{
	exit_localization();

	CDHtmlDialog::OnClose();
}
