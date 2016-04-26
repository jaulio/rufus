
// EndlessUsbToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EndlessUsbTool.h"
#include "EndlessUsbToolDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// TODO: log the error
#define IFFALSE_GOTOERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { goto error; }

// CEndlessUsbToolDlg dialog

BEGIN_DHTML_EVENT_MAP(CEndlessUsbToolDlg)
	// For dragging the window
	DHTML_EVENT_CLASS(DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN, _T("PageHeaderTitle"), OnHtmlMouseDown)

	// First Page Handlers		
	DHTML_EVENT_ELEMENT(DISPID_HTMLELEMENTEVENTS_ONCLICK, _T("LeftPanel"), OnTryEndlessSelected)
	DHTML_EVENT_ELEMENT(DISPID_HTMLELEMENTEVENTS_ONCLICK, _T("RightPanel"), OnInstallEndlessSelected)

	DHTML_EVENT_ONCLICK(_T("CompareOptionsLink"), OnCompareOptionsClicked)
	DHTML_EVENT_ONCLICK(_T("LanguageLink"), OnSelectLanguageClicked)

	// Select File Page handlers
	DHTML_EVENT_ONCLICK(_T("SelectFilePreviousButton"), OnSelectFilePreviousClicked)
	DHTML_EVENT_ONCLICK(_T("SelectFileNextButton"), OnSelectFileNextClicked)

	// Select USB Page handlers
	DHTML_EVENT_ONCLICK(_T("SelectUSBPreviousButton"), OnSelectUSBPreviousClicked)
	DHTML_EVENT_ONCLICK(_T("SelectUSBNextButton"), OnSelectUSBNextClicked)	

END_DHTML_EVENT_MAP()


CEndlessUsbToolDlg::CEndlessUsbToolDlg(CWnd* pParent /*=NULL*/)
	: CDHtmlDialog(IDD_ENDLESSUSBTOOL_DIALOG, IDR_HTML_ENDLESSUSBTOOL_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEndlessUsbToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CEndlessUsbToolDlg, CDHtmlDialog)
END_MESSAGE_MAP()


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

	CRgn rgn;

	int radius = 19;
	//  Create the shape we want
	rgn.CreateRoundRectRgn(0, 0, w, h, radius, radius);
	/*rgn.CreateRoundRectRgn(rect.TopLeft().x, rect.TopLeft().y, rect.BottomRight().x,
		rect.BottomRight().y, radius, radius);*/
	
	//  Set the window region
	SetWindowRgn(static_cast<HRGN>(rgn.GetSafeHandle()), TRUE);

	//rgn.Detach();

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
	//return CDHtmlDialog::ShowContextMenu(dwID, ppt, pcmdtReserved, pdispReserved);
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



void CEndlessUsbToolDlg::ChangePage(PCTSTR oldPage, PCTSTR newPage)
{
	CComPtr<IHTMLDocument3> spHtmlDoc3;
	CComPtr<IHTMLElement> pOldPage = NULL, pNewPage = NULL;
	HRESULT hr = m_spHtmlDoc->QueryInterface(IID_IHTMLDocument3, (void**)&spHtmlDoc3);

	IFFALSE_GOTOERROR(SUCCEEDED(hr) && spHtmlDoc3 != NULL, _T("Error when querying IID_IHTMLDocument3 interface."));
	
	hr = spHtmlDoc3->getElementById(CComBSTR(oldPage), &pOldPage);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pOldPage != NULL, "Error querying for visible page.");

	hr = spHtmlDoc3->getElementById(CComBSTR(newPage), &pNewPage);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pOldPage != NULL, "Error querying for new page.");

	hr = pOldPage->put_className(CComBSTR("WizardPage hidden"));
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pOldPage != NULL, "Error when updating the classname for the visible page.");
	pNewPage->put_className(CComBSTR("WizardPage"));
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pOldPage != NULL, "Error when updating the classname for the new page.");

	return;

error:
	// TODO: LOG the HR value
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
	m_fullInstall = FALSE;
	ChangePage(_T("FirstPage"), _T("SelectFilePage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnInstallEndlessSelected(IHTMLElement* pElement)
{
	m_fullInstall = TRUE;
	ChangePage(_T("FirstPage"), _T("SelectFilePage"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnCompareOptionsClicked(IHTMLElement* pElement)
{
	AfxMessageBox(_T("Not implemented yet"));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectLanguageClicked(IHTMLElement* pElement)
{
	AfxMessageBox(_T("Not implemented yet"));

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