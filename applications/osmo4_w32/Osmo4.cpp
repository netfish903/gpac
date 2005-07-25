// GPAC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Osmo4.h"

#include "MainFrm.h"
#include "OpenUrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// WinGPAC

BEGIN_MESSAGE_MAP(WinGPAC, CWinApp)
	//{{AFX_MSG_MAP(WinGPAC)
	ON_COMMAND(ID_FILEOPEN, OnOpenFile)
	ON_COMMAND(ID_FILE_STEP, OnFileStep)
	ON_COMMAND(ID_OPEN_URL, OnOpenUrl)
	ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
	ON_COMMAND(ID_FILE_PLAY, OnFilePlay)
	ON_UPDATE_COMMAND_UI(ID_FILE_PLAY, OnUpdateFilePlay)
	ON_UPDATE_COMMAND_UI(ID_FILE_STEP, OnUpdateFileStep)
	ON_COMMAND(ID_FILE_STOP, OnFileStop)
	ON_UPDATE_COMMAND_UI(ID_FILE_STOP, OnUpdateFileStop)
	ON_COMMAND(ID_SWITCH_RENDER, OnSwitchRender)
	ON_COMMAND(ID_RELOAD_TERMINAL, OnReloadTerminal)
	ON_UPDATE_COMMAND_UI(ID_FILE_RELOAD, OnUpdateFileStop)
	ON_COMMAND(ID_H_ABOUT, OnAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// WinGPAC construction

WinGPAC::WinGPAC()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only WinGPAC object

WinGPAC theApp;



class UserPassDialog : public CDialog
{
// Construction
public:
	UserPassDialog(CWnd* pParent = NULL);   // standard constructor

	Bool GetPassword(const char *site_url, char *user, char *password);

// Dialog Data
	//{{AFX_DATA(UserPassDialog)
	enum { IDD = IDD_PASSWD };
	CStatic	m_SiteURL;
	CEdit m_User;
	CEdit m_Pass;
	//}}AFX_DATA

	void SetSelection(u32 sel);
	char cur_ext[200], cur_mime[200];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(UserPassDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	const char *m_site_url;
	char *m_user, *m_password;

	// Generated message map functions
	//{{AFX_MSG(UserPassDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	//}}AFX_MSG
};

UserPassDialog::UserPassDialog(CWnd* pParent /*=NULL*/)
	: CDialog(UserPassDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptStream)
	//}}AFX_DATA_INIT
}

void UserPassDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(UserPassDialog)
	DDX_Control(pDX, IDC_TXT_SITE, m_SiteURL);
	DDX_Control(pDX, IDC_EDIT_USER, m_User);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_Pass);
	//}}AFX_DATA_MAP
}

BOOL UserPassDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_SiteURL.SetWindowText(m_site_url);
	m_User.SetWindowText(m_user);
	m_Pass.SetWindowText("");
	return TRUE;
}

void UserPassDialog::OnClose()
{
	m_User.GetWindowText(m_user, 50);
	m_Pass.GetWindowText(m_password, 50);
}

Bool UserPassDialog::GetPassword(const char *site_url, char *user, char *password)
{
	m_site_url = site_url;
	m_user = user;
	if (DoModal() != IDOK) return 0;
	return 1;
}

Bool is_supported_file(GF_Config *cfg, const char *fileName, Bool disable_no_ext)
{
	char szExt[20], *ext, mimes[1000];
	u32 keyCount, i;

	ext = strrchr(fileName, '/');
	if (ext) {
		ext = strchr(fileName, '.');
		/*fixme - a proper browser should check mime & co here*/
		if (!ext) return strstr(fileName, "http://") ? 0 : 1;
	}
	
	ext = strrchr(fileName, '.');
	/*this may be anything so let's try*/
	if (!ext) {
		return !disable_no_ext;
	}

	strcpy(szExt, ext+1);
	ext =strrchr(szExt, '#');
	if (ext) ext[0] = 0;
	strlwr(szExt);

	keyCount = gf_cfg_get_key_count(cfg, "MimeTypes");
	for (i=0; i<keyCount; i++) {
		const char *sKey;
		char *ext;
		sKey = (char *) gf_cfg_get_key_name(cfg, "MimeTypes", i);
		if (!sKey) continue;
		sKey = gf_cfg_get_key(cfg, "MimeTypes", sKey);
		strcpy(mimes, sKey+1);
		ext = strchr(mimes, '"');
		if (!ext) continue;
		ext[0] = 0;
		ext = mimes;
		while (ext) {
			if (!strnicmp(ext, szExt, strlen(szExt))) return 1;
			ext = strchr(ext, ' ');
			if (ext) ext+=1;
		}
	}
	if (!strstr(fileName, "http://")) return 1;
	/*looks like a regular web link_*/
	return 0;
}

static log_msg(char *msg)
{
	::MessageBox(NULL, msg, "GPAC", MB_OK);
}
Bool Osmo4_EventProc(void *priv, GF_Event *evt)
{
	u32 dur;
	WinGPAC *gpac = (WinGPAC *) priv;
	CMainFrame *pFrame = (CMainFrame *) gpac->m_pMainWnd;
	/*shutdown*/
	if (!pFrame) return 0;

	switch (evt->type) {
	case GF_EVT_DURATION:
		dur = (u32) (1000 * evt->duration.duration);
		//if (dur<1100) dur = 0;
		pFrame->m_pPlayList->SetDuration((u32) evt->duration.duration );
		gpac->max_duration = dur;
		gpac->can_seek = evt->duration.can_seek;
		if (!gpac->can_seek) {
			pFrame->m_Sliders.m_PosSlider.EnableWindow(FALSE);
		} else {
			pFrame->m_Sliders.m_PosSlider.EnableWindow(TRUE);
			pFrame->m_Sliders.m_PosSlider.SetRangeMin(0);
			pFrame->m_Sliders.m_PosSlider.SetRangeMax(dur);
		}
		break;

	case GF_EVT_MESSAGE:
		if (!evt->message.service || !strcmp(evt->message.service, (LPCSTR) pFrame->m_pPlayList->GetURL() )) {
			pFrame->console_service = "main service";
		} else {
			pFrame->console_service = evt->message.service;
		}
		if (evt->message.error!=GF_OK) {
			if (evt->message.error<GF_OK || !gpac->m_NoConsole) {
				pFrame->console_err = evt->message.error;
				pFrame->console_message = evt->message.message;
				gpac->m_pMainWnd->PostMessage(WM_CONSOLEMSG, 0, 0);

				/*any error before connection confirm is a service connection error*/
				if (!gpac->m_isopen) pFrame->m_pPlayList->SetDead();
			}
			return 0;
		}
		if (gpac->m_NoConsole) return 0;

		/*process user message*/
		pFrame->console_err = GF_OK;
		pFrame->console_message = evt->message.message;
		gpac->m_pMainWnd->PostMessage(WM_CONSOLEMSG, 0, 0);
		break;

	case GF_EVT_SIZE:
		gpac->orig_width = evt->size.width;
		gpac->orig_height = evt->size.height;
		if (gpac->m_term) {
			pFrame->PostMessage(WM_SETSIZE, evt->size.width, evt->size.height);
			/*not sure what's wrong, not sleeping result to another attempt to lock gpac's renderer which deadlocks!!*/
			gf_sleep(20);
		}
		break;

	case GF_EVT_CONNECT:
		pFrame->BuildStreamList(1);
		if (evt->connect.is_connected) {
			pFrame->BuildChapterList(0);
			gpac->m_isopen = 1;
		} else {
			gpac->max_duration = 0;
			gpac->m_isopen = 0;
			pFrame->BuildChapterList(1);
		}
		pFrame->m_wndToolBar.SetButtonInfo(5, ID_FILE_PLAY, TBBS_BUTTON, gpac->m_isopen ? 4 : 3);
		pFrame->m_Sliders.m_PosSlider.SetPos(0);
		pFrame->SetProgTimer(1);
		pFrame->SetFocus();
		pFrame->SetForegroundWindow();

		break;

	case GF_EVT_QUIT:
		pFrame->PostMessage(WM_CLOSE, 0L, 0L);
		break;
	case GF_EVT_VKEYDOWN:
		if (gpac->can_seek && evt->key.key_states & GF_KM_ALT) {
			s32 res;
			switch (evt->key.vk_code) {
			case GF_VK_LEFT:
				res = gf_term_get_time_in_ms(gpac->m_term) - 5*gpac->max_duration/100;
				if (res<0) res=0;
				gpac->PlayFromTime(res);
				break;
			case GF_VK_RIGHT:
				res = gf_term_get_time_in_ms(gpac->m_term) + 5*gpac->max_duration/100;
				if ((u32) res>=gpac->max_duration) res = 0;
				gpac->PlayFromTime(res);
				break;
			case GF_VK_DOWN:
				res = gf_term_get_time_in_ms(gpac->m_term) - 60000;
				if (res<0) res=0;
				gpac->PlayFromTime(res);
				break;
			case GF_VK_UP:
				res = gf_term_get_time_in_ms(gpac->m_term) + 60000;
				if ((u32) res>=gpac->max_duration) res = 0;
				gpac->PlayFromTime(res);
				break;
			}
		} else if (evt->key.key_states & GF_KM_CTRL) {
			switch (evt->key.vk_code) {
			case GF_VK_LEFT:
				pFrame->m_pPlayList->PlayPrev();
				break;
			case GF_VK_RIGHT:
				pFrame->m_pPlayList->PlayNext();
				break;
			}
		} else {
			switch (evt->key.vk_code) {
			case GF_VK_HOME:
				gf_term_set_option(gpac->m_term, GF_OPT_NAVIGATION_TYPE, 1);
				break;
			case GF_VK_ESCAPE:
				pFrame->PostMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
				break;
			}
		}
		break;
	case GF_EVT_NAVIGATE:
		/*fixme - a proper browser would require checking mime type & co*/
		//if (!is_supported_file(gpac->m_user.config, evt->navigate.to_url, 0)) return 0;
		/*store URL since it may be destroyed, and post message*/
		gpac->m_navigate_url = evt->navigate.to_url;
		pFrame->PostMessage(WM_NAVIGATE, NULL, NULL);
		return 1;
	case GF_EVT_VIEWPOINTS:
		pFrame->BuildViewList();
		return 0;
	case GF_EVT_STREAMLIST:
		pFrame->BuildStreamList(0);
		return 0;
	case GF_EVT_LDOUBLECLICK:
		pFrame->PostMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
		return 0;
	case GF_EVT_AUTHORIZATION:
		UserPassDialog passdlg;
		return passdlg.GetPassword(evt->auth.site_url, evt->auth.user, evt->auth.password);
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// WinGPAC initialization

BOOL WinGPAC::InitInstance()
{
	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	SetRegistryKey(_T("GPAC"));

	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;
	pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);
	m_pMainWnd->DragAcceptFiles();

	strcpy((char *) szAppPath, AfxGetApp()->m_pszHelpFilePath);
	while (szAppPath[strlen((char *) szAppPath)-1] != '\\') szAppPath[strlen((char *) szAppPath)-1] = 0;

	/*setup user*/
	memset(&m_user, 0, sizeof(GF_User));

	Bool first_launch = 0;
	/*init config and modules*/
	m_user.config = gf_cfg_new((const char *) szAppPath, "GPAC.cfg");
	if (!m_user.config) {
		first_launch = 1;
		/*create blank config file in the exe dir*/
		unsigned char config_file[MAX_PATH];
		strcpy((char *) config_file, (const char *) szAppPath);
		strcat((char *) config_file, "GPAC.cfg");
		FILE *ft = fopen((const char *) config_file, "wt");
		fclose(ft);
		m_user.config = gf_cfg_new((const char *) szAppPath, "GPAC.cfg");
		if (!m_user.config) {
			MessageBox(NULL, "GPAC Configuration file not found", "Fatal Error", MB_OK);
			m_pMainWnd->PostMessage(WM_CLOSE);
		}
	}

	const char *str = gf_cfg_get_key(m_user.config, "General", "ModulesDirectory");
	m_user.modules = gf_modules_new((const unsigned char *) str, m_user.config);
	if (!m_user.modules) {
		const char *sOpt;
		/*inital launch*/
		m_user.modules = gf_modules_new((const unsigned char *) szAppPath, m_user.config);
		if (m_user.modules) {
			gf_cfg_set_key(m_user.config, "General", "ModulesDirectory", (const char *) szAppPath);

			sOpt = gf_cfg_get_key(m_user.config, "Rendering", "Raster2D");
			if (!sOpt) gf_cfg_set_key(m_user.config, "Rendering", "Raster2D", "gdip_rend");

			sOpt = gf_cfg_get_key(m_user.config, "General", "CacheDirectory");
			if (!sOpt) {
				unsigned char str_path[MAX_PATH];
				sprintf((char *) str_path, "%scache", szAppPath);
				gf_cfg_set_key(m_user.config, "General", "CacheDirectory", (const char *) str_path);
			}
			/*setup UDP traffic autodetect*/
			gf_cfg_set_key(m_user.config, "Network", "AutoReconfigUDP", "yes");
			gf_cfg_set_key(m_user.config, "Network", "UDPNotAvailable", "no");
			gf_cfg_set_key(m_user.config, "Network", "UDPTimeout", "10000");
			gf_cfg_set_key(m_user.config, "Network", "BufferLength", "3000");

			/*first launch, register all files ext*/
			u32 i;
			for (i=0; i<gf_modules_get_count(m_user.modules); i++) {
				GF_InputService *ifce = (GF_InputService *) gf_modules_load_interface(m_user.modules, i, GF_NET_CLIENT_INTERFACE);
				if (!ifce) continue;
				if (ifce) {
					ifce->CanHandleURL(ifce, "test.test");
					gf_modules_close_interface((GF_BaseInterface *)ifce);
				}
			}
		}

		/*check audio config on windows, force config*/
		sOpt = gf_cfg_get_key(m_user.config, "Audio", "ForceConfig");
		if (!sOpt) {
			gf_cfg_set_key(m_user.config, "Audio", "ForceConfig", "yes");
			gf_cfg_set_key(m_user.config, "Audio", "NumBuffers", "8");
			gf_cfg_set_key(m_user.config, "Audio", "BuffersPerSecond", "16");
		}
		/*by default use GDIplus, much faster than freetype on font loading*/
		gf_cfg_set_key(m_user.config, "FontEngine", "DriverName", "gdip_rend");

	}	
	if (! gf_modules_get_count(m_user.modules) ) {
		MessageBox(NULL, "No modules available - system cannot work", "Fatal Error", MB_OK);
		m_pMainWnd->PostMessage(WM_CLOSE);
	}

	/*setup font dir*/
	str = gf_cfg_get_key(m_user.config, "FontEngine", "FontDirectory");
	if (!str) {
		char szFtPath[MAX_PATH];
		::GetWindowsDirectory((char*)szFtPath, MAX_PATH);
		if (szFtPath[strlen((char*)szFtPath)-1] != '\\') strcat((char*)szFtPath, "\\");
		strcat((char *)szFtPath, "Fonts");
		gf_cfg_set_key(m_user.config, "FontEngine", "FontDirectory", (const char *) szFtPath);
	}

	/*check video driver, if none or raw_out use dx_hw by default*/
	str = gf_cfg_get_key(m_user.config, "Video", "DriverName");
	if (!str || !stricmp(str, "raw_out")) {
		gf_cfg_set_key(m_user.config, "Video", "DriverName", "dx_hw");
	}

	m_user.opaque = this;
	m_user.os_window_handler = pFrame->m_pWndView->m_hWnd;
	m_user.EventProc = Osmo4_EventProc;

	m_reset = 0;
	m_prev_time = 0;
	orig_width = 320;
	orig_height = 240;

	m_term = gf_term_new(&m_user);
	if (! m_term) {
		MessageBox(NULL, "Cannot load GPAC Terminal", "Fatal Error", MB_OK);
		m_pMainWnd->PostMessage(WM_CLOSE);
		return TRUE;
	}
	SetOptions();
	UpdateRenderSwitch();

	pFrame->SendMessage(WM_SETSIZE, orig_width, orig_height);
	pFrame->m_Address.ReloadURLs();

	pFrame->m_Sliders.SetVolume();

	m_reconnect_time = 0;


	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (! cmdInfo.m_strFileName.IsEmpty()) {
		pFrame->m_pPlayList->QueueURL(cmdInfo.m_strFileName);
		pFrame->m_pPlayList->RefreshList();
		pFrame->m_pPlayList->PlayNext();
	} else {
		char sPL[MAX_PATH];
		strcpy((char *) sPL, szAppPath);
		strcat(sPL, "gpac_pl.m3u");
		pFrame->m_pPlayList->OpenPlayList(sPL);
		const char *sOpt = gf_cfg_get_key(GetApp()->m_user.config, "General", "PLEntry");
		if (sOpt) {
			pFrame->m_pPlayList->m_cur_entry = atoi(sOpt);
			if (pFrame->m_pPlayList->m_cur_entry>=(s32)gf_list_count(pFrame->m_pPlayList->m_entries))
				pFrame->m_pPlayList->m_cur_entry = -1;
		} else {
			pFrame->m_pPlayList->m_cur_entry = -1;
		}
	}
	pFrame->SetFocus();
	pFrame->SetForegroundWindow();
	return TRUE;
}


void WinGPAC::ReloadTerminal()
{
	Bool reconnect = m_isopen;
	CMainFrame *pFrame = (CMainFrame *) m_pMainWnd;
	pFrame->console_err = GF_OK;
	pFrame->console_message = "Reloading GPAC Terminal";
	m_pMainWnd->PostMessage(WM_CONSOLEMSG, 0, 0);

	m_reconnect_time = 0;
	if (can_seek) m_reconnect_time = gf_term_get_time_in_ms(m_term);

	gf_term_del(m_term);
	m_term = gf_term_new(&m_user);
	if (!m_term) {
		MessageBox(NULL, "Fatal Error !!", "Couldn't change renderer", MB_OK);
		m_pMainWnd->PostMessage(WM_DESTROY);
		return;
	}
	pFrame->console_message = "GPAC Terminal reloaded";
	m_pMainWnd->PostMessage(WM_CONSOLEMSG, 0, 0);
	UpdateRenderSwitch();
	if (reconnect) m_pMainWnd->PostMessage(WM_OPENURL);
}


/////////////////////////////////////////////////////////////////////////////
// WinGPAC message handlers





/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnGogpac();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_GOGPAC, OnGogpac)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void WinGPAC::OnAbout() 
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CString str = "GPAC/Osmo4 - version " GPAC_VERSION;
	SetWindowText(str);
	return TRUE;  
}

void CAboutDlg::OnGogpac() 
{
	ShellExecute(NULL, "open", "http://gpac.sourceforge.net", NULL, NULL, SW_SHOWNORMAL);
}

/////////////////////////////////////////////////////////////////////////////
// WinGPAC message handlers


int WinGPAC::ExitInstance() 
{

	gf_term_del(m_term);
	gf_modules_del(m_user.modules);
	gf_cfg_del(m_user.config);
	return CWinApp::ExitInstance();
}

void WinGPAC::SetOptions()
{
	const char *sOpt = gf_cfg_get_key(m_user.config, "General", "Loop");
	m_Loop = (sOpt && !stricmp(sOpt, "yes")) ? 1 : 0;
	sOpt = gf_cfg_get_key(m_user.config, "General", "LookForSubtitles");
	m_LookForSubtitles = (sOpt && !stricmp(sOpt, "yes")) ? 1 : 0;
	sOpt = gf_cfg_get_key(m_user.config, "General", "ConsoleOff");
	m_NoConsole = (sOpt && !stricmp(sOpt, "yes")) ? 1 : 0;
	sOpt = gf_cfg_get_key(m_user.config, "General", "ViewXMT");
	m_ViewXMTA  = (sOpt && !stricmp(sOpt, "yes")) ? 1 : 0;
}


void WinGPAC::OnOpenUrl() 
{
	COpenUrl url;
	if (url.DoModal() != IDOK) return;

	CMainFrame *pFrame = (CMainFrame *) m_pMainWnd;
	pFrame->m_pPlayList->Truncate();
	pFrame->m_pPlayList->QueueURL(url.m_url);
	pFrame->m_pPlayList->RefreshList();
	pFrame->m_pPlayList->PlayNext();
}


CString WinGPAC::GetFileFilter()
{
	u32 keyCount, i;
	CString sFiles;
	CString sExts;
	CString supportedFiles;

	/*force MP4 and 3GP files at beginning to make sure they are selected (Win32 bug with too large filters)*/
	supportedFiles = "All Known Files|*.m3u;*.pls;*.mp4;*.3gp;*.3g2";

	sExts = "";
	sFiles = "";
	keyCount = gf_cfg_get_key_count(m_user.config, "MimeTypes");
	for (i=0; i<keyCount; i++) {
		const char *sMime;
		Bool first;
		char *sKey;
		const char *opt;
		char szKeyList[1000], sDesc[1000];
		sMime = gf_cfg_get_key_name(m_user.config, "MimeTypes", i);
		if (!sMime) continue;
		CString sOpt;
		opt = gf_cfg_get_key(m_user.config, "MimeTypes", sMime);
		/*remove module name*/
		strcpy(szKeyList, opt+1);
		sKey = strrchr(szKeyList, '\"');
		if (!sKey) continue;
		sKey[0] = 0;
		/*get description*/
		sKey = strrchr(szKeyList, '\"');
		if (!sKey) continue;
		strcpy(sDesc, sKey+1);
		sKey[0] = 0;
		sKey = strrchr(szKeyList, '\"');
		if (!sKey) continue;
		sKey[0] = 0;

		/*if same description for # mime types skip (means an old mime syntax)*/
		if (sFiles.Find(sDesc)>=0) continue;
		/*if same extensions for # mime types skip (don't polluate the file list)*/
		if (sExts.Find(szKeyList)>=0) continue;

		sExts += szKeyList;
		sExts += " ";
		sFiles += sDesc;
		sFiles += "|";

		first = 1;

		sOpt = CString(szKeyList);
		while (1) {
			
			int pos = sOpt.Find(' ');
			CString ext = (pos==-1) ? sOpt : sOpt.Left(pos);
			/*WATCHOUT: we do have some "double" ext , eg .wrl.gz - these are NOT supported by windows*/
			if (ext.Find(".")<0) {
				if (!first) {
					sFiles += ";";
				} else {
					first = 0;
				}
				sFiles += "*.";
				sFiles += ext;

				CString sext = ext;
				sext += ";";
				if (supportedFiles.Find(sext)<0) {
					supportedFiles += ";*.";
					supportedFiles += ext;
				}
			}

			if (sOpt==ext) break;
			CString rem;
			rem.Format("%s ", (LPCTSTR) ext);
			sOpt.Replace((LPCTSTR) rem, "");
		}
		sFiles += "|";
	}
	supportedFiles += "|";
	supportedFiles += sFiles;
	supportedFiles += "M3U Playlists|*.m3u|ShoutCast Playlists|*.pls|All Files |*.*|";
	return supportedFiles;
}

void WinGPAC::OnOpenFile() 
{
	CString sFiles = GetFileFilter();
	u32 nb_items;
	
	/*looks like there's a bug here, main filter isn't used correctly while the others are*/
	CFileDialog fd(TRUE,NULL,NULL, OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST , sFiles);
	fd.m_ofn.nMaxFile = 25000;
	fd.m_ofn.lpstrFile = (char *) malloc(sizeof(char) * fd.m_ofn.nMaxFile);
	fd.m_ofn.lpstrFile[0] = 0;

	if (fd.DoModal()!=IDOK) {
		free(fd.m_ofn.lpstrFile);
		return;
	}

	CMainFrame *pFrame = (CMainFrame *) m_pMainWnd;

	nb_items = 0;
	POSITION pos = fd.GetStartPosition();
	while (pos) {
		CString file = fd.GetNextPathName(pos);
		nb_items++;
	}
	/*if several items, act as playlist (replace playlist), otherwise as browser (lost all "next" context)*/
	if (nb_items==1) 
		pFrame->m_pPlayList->Truncate();
	else
		pFrame->m_pPlayList->Clear();

	pos = fd.GetStartPosition();
	while (pos) {
		CString file = fd.GetNextPathName(pos);
		pFrame->m_pPlayList->QueueURL(file);
	}
	free(fd.m_ofn.lpstrFile);
	pFrame->m_pPlayList->RefreshList();
	pFrame->m_pPlayList->PlayNext();
}


void WinGPAC::Pause()
{
	if (!m_isopen) return;
	gf_term_set_option(m_term, GF_OPT_PLAY_STATE, (gf_term_get_option(m_term, GF_OPT_PLAY_STATE)==GF_STATE_PLAYING) ? GF_STATE_PAUSED : GF_STATE_PLAYING);
}

void WinGPAC::OnMainPause() 
{
	Pause();	
}

void WinGPAC::OnFileStep() 
{
	gf_term_set_option(m_term, GF_OPT_PLAY_STATE, GF_STATE_STEP_PAUSE);
	((CMainFrame *) m_pMainWnd)->m_wndToolBar.SetButtonInfo(5, ID_FILE_PLAY, TBBS_BUTTON, 3);
}
void WinGPAC::OnUpdateFileStep(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_isopen && !m_reset);	
}

void WinGPAC::PlayFromTime(u32 time)
{
	gf_term_play_from_time(m_term, time);
	m_reset = 0;
}


void WinGPAC::OnFileReload() 
{
	gf_term_disconnect(m_term);
	m_pMainWnd->PostMessage(WM_OPENURL);
}

void WinGPAC::OnFilePlay() 
{
	if (m_isopen) {
		if (m_reset) {
			m_reset = 0;
			PlayFromTime(0);
			((CMainFrame *)m_pMainWnd)->SetProgTimer(1);
		} else {
			Pause();
		}
		if (gf_term_get_option(m_term, GF_OPT_PLAY_STATE)==GF_STATE_PLAYING) {
			((CMainFrame *) m_pMainWnd)->m_wndToolBar.SetButtonInfo(5, ID_FILE_PLAY, TBBS_BUTTON, 4);
		} else {
			((CMainFrame *) m_pMainWnd)->m_wndToolBar.SetButtonInfo(5, ID_FILE_PLAY, TBBS_BUTTON, 3);
		}
	} else {
		((CMainFrame *) m_pMainWnd)->m_pPlayList->Play();
	}
}

void WinGPAC::OnUpdateFilePlay(CCmdUI* pCmdUI) 
{
	if (m_isopen) {
		pCmdUI->Enable(TRUE);	
		if (pCmdUI->m_nID==ID_FILE_PLAY) {
			if (!m_isopen) {
				pCmdUI->SetText("Play/Pause\tCtrl+P");
			} else if (gf_term_get_option(m_term, GF_OPT_PLAY_STATE)==GF_STATE_PLAYING) {
				pCmdUI->SetText("Pause\tCtrl+P");
			} else {
				pCmdUI->SetText("Resume\tCtrl+P");
			}
		}
	} else {
		pCmdUI->Enable(((CMainFrame *)m_pMainWnd)->m_pPlayList->HasValidEntries() );	
		pCmdUI->SetText("Play\tCtrl+P");
	}
}

void WinGPAC::OnFileStop() 
{
	CMainFrame *pFrame = (CMainFrame *) m_pMainWnd;
	if (m_reset) return;
	if (gf_term_get_option(m_term, GF_OPT_PLAY_STATE)==GF_STATE_PLAYING) Pause();
	m_reset = 1;
	pFrame->m_Sliders.m_PosSlider.SetPos(0);
	pFrame->SetProgTimer(0);
	pFrame->m_wndToolBar.SetButtonInfo(5, ID_FILE_PLAY, TBBS_BUTTON, 3);
}

void WinGPAC::OnUpdateFileStop(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_isopen);	
}

void WinGPAC::OnSwitchRender() 
{
	const char *opt = gf_cfg_get_key(m_user.config, "Rendering", "RendererName");
	if (!stricmp(opt, "GPAC 2D Renderer"))
		gf_cfg_set_key(m_user.config, "Rendering", "RendererName", "GPAC 3D Renderer");
	else
		gf_cfg_set_key(m_user.config, "Rendering", "RendererName", "GPAC 2D Renderer");

	ReloadTerminal();
}

void WinGPAC::UpdateRenderSwitch()
{
	const char *opt = gf_cfg_get_key(m_user.config, "Rendering", "RendererName");
	if (!stricmp(opt, "GPAC 3D Renderer"))
		((CMainFrame *) m_pMainWnd)->m_wndToolBar.SetButtonInfo(12, ID_SWITCH_RENDER, TBBS_BUTTON, 10);
	else
		((CMainFrame *) m_pMainWnd)->m_wndToolBar.SetButtonInfo(12, ID_SWITCH_RENDER, TBBS_BUTTON, 9);
}

void WinGPAC::OnReloadTerminal() 
{
	ReloadTerminal();
}

