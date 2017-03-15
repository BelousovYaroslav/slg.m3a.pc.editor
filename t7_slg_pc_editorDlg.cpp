// t7_slg_pc_editorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "t7_slg_pc_editor.h"
#include "t7_slg_pc_editorDlg.h"
#include "McCommands.h"
#include "AnalogueParamsConstList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TIMER_FLUSH_COM 10000
#define TIMER_COM_STATE 10001
#define TIMER_REFRESH_DATA 10002

#define TIMER_SEND_CMDS_TO_MC 10004
#define TIMER_POLLER 10005

extern HANDLE gl_hThread;
extern DWORD WINAPI BigThread(LPVOID lparam);

extern CT7_slg_pc_editorApp theApp;
extern int gl_nCircleBufferGet;
extern int gl_nCircleBufferPut;
extern int gl_GetCircleBufferDistance( void);
extern bool PutByteInCircleBuffer(BYTE bt);
extern BOOL m_bStopBigThreadFlag;

extern int gl_nMarkerFails;
extern int gl_nCheckSummFails;
extern int gl_nReceivedPacks;

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
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CT7_slg_pc_editorDlg dialog

CT7_slg_pc_editorDlg::~CT7_slg_pc_editorDlg() {
  int nSize = m_queCommandQueue.size();
  for( int i = 0; i < nSize; i++) {
    CMcCommandItem *item = ( CMcCommandItem *) m_queCommandQueue.front();
    m_queCommandQueue.pop();
    delete item;
  }
}

CT7_slg_pc_editorDlg::CT7_slg_pc_editorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CT7_slg_pc_editorDlg::IDD, pParent)
{
  theApp.GetLogger()->LogTrace( "CT7_slg_pc_editorDlg::CT7_slg_pc_editorDlg: in");
	//{{AFX_DATA_INIT(CT7_slg_pc_editorDlg)
	m_nComPort = 0;
	m_nPortSpeed = 0;
	m_strConnectionStatus = _T("Разъединено");
	m_strLblEmergency = _T("");
	m_strMcVersion = _T("-");
	m_nCalibCurrentTemperature = 0;
	m_strMarkerFails = _T("");
	m_strCheckSummFails = _T("");
	m_dblScaleCoeff = 0.0;
	m_strRecievedPacks = _T("");
	m_nSignCoeffIn = 0;
	m_nSignCoeffOut = 1;
	m_dblAmplCodeOut = 0.0;
	m_dblAmplCodeIn = 0.0;
	m_nTactCodeIn = 0;
	m_nTactCodeOut = 0;
	m_dblMinI1In = 0.0;
	m_dblMinI1Out = 0.0;
	m_dblMinI2In = 0.0;
	m_dblMinI2Out = 0.0;
	m_dblMinAmplAngIn = 0.0;
	m_dblMinAmplAngOut = 0.0;
	m_dblMCoeffIn = 0.0;
	m_dblMCoeffOut = 0.0;
	m_dblDecCoeffIn = 0.0;
	m_dblDecCoeffOut = 0.0;
	m_nDeviceSerialNumIn = 0;
	m_nDeviceSerialNumOut = 0;
	m_nHvApplyIn = 0;
	m_nHvApplyOut = 0;
	m_nHvApplyDurIn = 0;
	m_nHvApplyDurOut = 0;
	m_strOrganizationOut = _T("");
	m_strOrganizationIn = _T("");
	m_dblStartModeOut = 0.0;
	m_dblStartModeIn = 0.0;
	m_nHvApplyPacksIn = 0;
	m_nHvApplyPacksOut = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  theApp.GetLogger()->LogTrace( "CT7_slg_pc_editorDlg::CT7_slg_pc_editorDlg: out");

  m_nPollParams[ 0] = AMPLITUDE;
  m_nPollParams[ 1] = TACT_CODE;
  m_nPollParams[ 2] = M_COEFF;
  m_nPollParams[ 3] = STARTMODE;
  m_nPollParams[ 4] = DECCOEFF;
  m_nPollParams[ 5] = CONTROL_I1;
  m_nPollParams[ 6] = CONTROL_I2;
  m_nPollParams[ 7] = CONTROL_AA;
  m_nPollParams[ 8] = HV_APPLY_COUNT_SET;
  m_nPollParams[ 9] = HV_APPLY_DURAT_SET;
  m_nPollParams[10] = HV_APPLY_PACKS;

  m_nPollParams[11] = SIGNCOEFF;
  m_nPollParams[12] = DEVNUM;
  m_nPollParams[13] = STARTMODE;
  m_nPollParams[14] = DATE_Y;
  m_nPollParams[15] = DATE_M;
  m_nPollParams[16] = DATE_D;
  m_nPollParams[17] = ORG;
  m_nPollParams[18] = VERSION;
  m_nPollParams[19] = CALIB_T1;  
  
  m_nPollParams[20] = CALIB_T1;
  m_nPollParams[21] = T1_TD1;
  m_nPollParams[22] = T1_TD2;
  m_nPollParams[23] = T1_TD3;
  m_nPollParams[24] = CALIB_T2;
  m_nPollParams[25] = T2_TD1;
  m_nPollParams[26] = T2_TD2;
  m_nPollParams[27] = T2_TD3;

}

void CT7_slg_pc_editorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CT7_slg_pc_editorDlg)
	DDX_CBIndex(pDX, IDC_CMB_COM, m_nComPort);
	DDX_CBIndex(pDX, IDC_CMB_PORT_SPEED, m_nPortSpeed);
	DDX_Text(pDX, IDC_STR_CONN_STATE, m_strConnectionStatus);
	DDX_Control(pDX, IDC_MSCOMM, m_ctlCOM);
	DDX_Text(pDX, IDC_LBL_EMERGENCY, m_strLblEmergency);
	DDX_Text(pDX, IDC_STR_MC_VER, m_strMcVersion);
	DDX_Text(pDX, IDC_EDT_TCALIB_T_CURRENT, m_nCalibCurrentTemperature);
	DDX_Text(pDX, IDC_LBL_MFAILS, m_strMarkerFails);
	DDX_Text(pDX, IDC_LBL_CSFAILS, m_strCheckSummFails);
	DDX_Control(pDX, IDC_DTP_DATE_IN, m_ctlDtpDevDateIn);
	DDX_Control(pDX, IDC_DTP_DATE_OUT, m_ctlDtpDevDateOut);
	DDX_Text(pDX, IDC_EDT_SCALE_COEFF, m_dblScaleCoeff);
	DDX_Text(pDX, IDC_LBL_PACKS, m_strRecievedPacks);
	DDX_Text(pDX, IDC_EDT_SIGN_COEFF_IN, m_nSignCoeffIn);
	DDX_Text(pDX, IDC_EDT_SIGN_COEFF_OUT, m_nSignCoeffOut);
	DDX_Text(pDX, IDC_EDT_AMPL_CODE_OUT, m_dblAmplCodeOut);
	DDX_Text(pDX, IDC_EDT_AMPL_CODE_IN, m_dblAmplCodeIn);
	DDX_Text(pDX, IDC_EDT_TACT_CODE_IN, m_nTactCodeIn);
	DDX_Text(pDX, IDC_EDT_TACT_CODE_OUT, m_nTactCodeOut);
	DDX_Text(pDX, IDC_EDT_MIN_CUR1_IN, m_dblMinI1In);
	DDX_Text(pDX, IDC_EDT_MIN_CUR1_OUT, m_dblMinI1Out);
	DDX_Text(pDX, IDC_EDT_MIN_CUR2_IN, m_dblMinI2In);
	DDX_Text(pDX, IDC_EDT_MIN_CUR2_OUT, m_dblMinI2Out);
	DDX_Text(pDX, IDC_EDT_MIN_AMPL_ANG_IN, m_dblMinAmplAngIn);
	DDX_Text(pDX, IDC_EDT_MIN_AMPL_ANG_OUT, m_dblMinAmplAngOut);
	DDX_Text(pDX, IDC_EDT_M_COEFF_IN, m_dblMCoeffIn);
	DDX_Text(pDX, IDC_EDT_M_COEFF_OUT, m_dblMCoeffOut);
	DDX_Text(pDX, IDC_EDT_DEC_COEFF_IN, m_dblDecCoeffIn);
	DDX_Text(pDX, IDC_EDT_DEC_COEFF_OUT, m_dblDecCoeffOut);
	DDX_Text(pDX, IDC_EDT_SER_NUM_IN, m_nDeviceSerialNumIn);
	DDX_Text(pDX, IDC_EDT_SER_NUM_OUT, m_nDeviceSerialNumOut);
	DDX_Text(pDX, IDC_EDT_HV_APPLY_SET_IN, m_nHvApplyIn);
	DDX_Text(pDX, IDC_EDT_HV_APPLY_SET_OUT, m_nHvApplyOut);
	DDX_Text(pDX, IDC_EDT_HV_APPLY_DUR_SET_IN, m_nHvApplyDurIn);
	DDX_Text(pDX, IDC_EDT_HV_APPLY_DUR_SET_OUT, m_nHvApplyDurOut);
	DDX_Text(pDX, IDC_EDT_ORGANIZATION_OUT, m_strOrganizationOut);
	DDX_Text(pDX, IDC_EDT_ORGANIZATION_IN, m_strOrganizationIn);
	DDX_Text(pDX, IDC_EDT_START_MODE_OUT, m_dblStartModeOut);
	DDX_Text(pDX, IDC_EDT_START_MODE_IN, m_dblStartModeIn);
	DDX_Text(pDX, IDC_EDT_HV_APPLY_PACKS_IN, m_nHvApplyPacksIn);
	DDX_Text(pDX, IDC_EDT_HV_APPLY_PACKS_OUT, m_nHvApplyPacksOut);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CT7_slg_pc_editorDlg, CDialog)
	//{{AFX_MSG_MAP(CT7_slg_pc_editorDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_CONNECT, OnBtnConnect)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, OnBtnDisconnect)
	ON_EN_CHANGE(IDC_EDT_ORGANISATION_OUT, OnChangeEdtOrganisationOut)
	ON_BN_CLICKED(IDC_BTN_APPLY_SCALE_COEFF, OnBtnApplyScaleCoeff)
	ON_EN_KILLFOCUS(IDC_EDT_SIGN_COEFF_OUT, OnKillfocusEdtSignCoeffOut)
	ON_BN_CLICKED(IDC_BTN_REQUEST_SERNUM, OnBtnRequestSernum)
	ON_BN_CLICKED(IDC_BTN_REQUEST_DATE, OnBtnRequestDate)
	ON_BN_CLICKED(IDC_BTN_REQUEST_ORG, OnBtnRequestOrg)
	ON_CBN_SELCHANGE(IDC_CMB_COM, OnSelchangeCmbCom)
	ON_CBN_SELCHANGE(IDC_CMB_PORT_SPEED, OnSelchangeCmbPortSpeed)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_DEVSERNUM, OnBtnSetreadtosentDevsernum)
	ON_BN_CLICKED(IDC_BTN_SEND_SERIAL, OnBtnSendSerial)
	ON_BN_CLICKED(IDC_BTN_SEND_CALIB, OnBtnSendCalib)
	ON_BN_CLICKED(IDC_BTN_REQUEST_AMPL, OnBtnRequestAmpl)
	ON_BN_CLICKED(IDC_BTN_REQUEST_TACT_CODE, OnBtnRequestTactCode)
	ON_BN_CLICKED(IDC_BTN_REQUEST_M_COEFF, OnBtnRequestMCoeff)
	ON_BN_CLICKED(IDC_BTN_REQUEST_START_MODE, OnBtnRequestStartMode)
	ON_BN_CLICKED(IDC_BTN_REQUEST_CONTROL_I1, OnBtnRequestControlI1)
	ON_BN_CLICKED(IDC_BTN_REQUEST_CONTROL_I2, OnBtnRequestControlI2)
	ON_BN_CLICKED(IDC_BTN_REQUEST_CONTROL_AA, OnBtnRequestControlAa)
	ON_BN_CLICKED(IDC_BTN_REQUEST_DEC_COEFF, OnBtnRequestDecCoeff)
	ON_BN_CLICKED(IDC_BTN_REQUEST_SIGN_COEFF, OnBtnRequestSignCoeff)
	ON_BN_CLICKED(IDC_BTN_REQUEST_HV_APPLY_SET, OnBtnRequestHvApplySet)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_AMPL, OnBtnCpReadToSentAmpl)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_TACT, OnBtnSetreadtosentTact)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_MCOEFF, OnBtnSetreadtosentMcoeff)
	ON_BN_CLICKED(IDC_BTN_REQUEST_CALIB, OnBtnRequestCalib)
	ON_BN_CLICKED(IDC_BTN_SEND_AMPL, OnBtnSendAmpl)
	ON_BN_CLICKED(IDC_BTN_SEND_TACTCODE, OnBtnSendTactcode)
	ON_BN_CLICKED(IDC_BTN_SEND_M_COEFF, OnBtnSendMCoeff)
	ON_BN_CLICKED(IDC_BTN_SEND_START_MODE, OnBtnSendStartMode)
	ON_BN_CLICKED(IDC_BTN_SEND_DEC_COEFF, OnBtnSendDecCoeff)
	ON_BN_CLICKED(IDC_BTN_SEND_I1_MIN, OnBtnSendI1Min)
	ON_BN_CLICKED(IDC_BTN_SEND_I2_MIN, OnBtnSendI2Min)
	ON_BN_CLICKED(IDC_BTN_SEND_AMPLANG_MIN, OnBtnSendAmplangMin)
	ON_BN_CLICKED(IDC_BTN_SEND_HV_APPLY_SET, OnBtnSendHvApplySet)
	ON_BN_CLICKED(IDC_BTN_SEND_SIGNCOEFF, OnBtnSendSigncoeff)
	ON_BN_CLICKED(IDC_BTN_SEND_DATE, OnBtnSendDate)
	ON_BN_CLICKED(IDC_BTN_SEND_ORGANIZATION, OnBtnSendOrganization)
	ON_BN_CLICKED(IDC_BTN_SAVE_TO_FLASH_P1, OnBtnSaveToFlashP1)
	ON_BN_CLICKED(IDC_BTN_SAVE_TO_FLASH_P2, OnBtnSaveToFlashP2)
	ON_BN_CLICKED(IDC_BTN_SAVE_TO_FLASH_P3, OnBtnSaveToFlashP3)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_STARTMODE, OnBtnSetreadtosentStartmode)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_DECCOEFF, OnBtnSetreadtosentDeccoeff)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_I1MIN, OnBtnSetreadtosentI1min)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_I2MIN, OnBtnSetreadtosentI2min)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_AMPLANGMIN, OnBtnSetreadtosentAmplangmin)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_DATE, OnBtnSetreadtosentDate)
	ON_BN_CLICKED(IDC_BTN_REQUEST_HV_APPLY_DUR_SET, OnBtnRequestHvApplyDurSet)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_HV_APPLY_SET, OnBtnSetreadtosentHvApplySet)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_HV_APPLY_DUR_SET, OnBtnSetreadtosentHvApplyDurSet)
	ON_BN_CLICKED(IDC_BTN_SEND_HV_APPLY_DUR_SET, OnBtnSendHvApplyDurSet)
	ON_BN_CLICKED(IDC_BTN_LOCK_DEVICE, OnBtnLockDevice)
	ON_BN_CLICKED(IDC_BTN_VERSION, OnBtnVersion)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_SIGNCOEFF, OnDeltaposSpinSigncoeff)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_SIGN, OnBtnSetreadtosentSign)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_ORG, OnBtnSetreadtosentOrg)
	ON_BN_CLICKED(IDC_BTN_REQUEST_HV_APPLY_PACKS, OnBtnRequestHvApplyPack)
	ON_BN_CLICKED(IDC_BTN_SETREADTOSENT_HV_APPLY_PACKS, OnBtnSetreadtosentHvApplyPacks)
	ON_BN_CLICKED(IDC_BTN_SEND_HV_APPLY_PACKS, OnBtnSendHvApplyPacks)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CT7_slg_pc_editorDlg message handlers

BOOL CT7_slg_pc_editorDlg::OnInitDialog()
{
  theApp.GetLogger()->LogInfo( "CT7_slg_pc_editorDlg::OnInitDialog(): in");

	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	SetControlsState( FALSE, TRUE);
	m_nComPort = (( CT7_slg_pc_editorApp *) AfxGetApp())->GetComPort();
	m_nPortSpeed = (( CT7_slg_pc_editorApp *) AfxGetApp())->GetComBaudrate();
	UpdateData( FALSE);


  m_ctlCOM.SetCommPort( 1);
	m_ctlCOM.SetSettings( _T("115200,N,8,1"));

  m_ctlCOM.SetInputMode( 1);

	m_ctlCOM.SetInBufferCount( 0);
  m_ctlCOM.SetInBufferSize( 2048);
  m_ctlCOM.SetInputLen( 280);

	m_ctlCOM.SetOutBufferCount( 0);
	m_ctlCOM.SetOutBufferSize( 3);

  SetTimer( TIMER_COM_STATE, 1000, NULL);
  SetTimer( TIMER_SEND_CMDS_TO_MC, 100, NULL);
  CString str;
  str.Format( _T("%.3f"), theApp.m_dblScaleCoeff);
  GetDlgItem( IDC_EDT_SCALE_COEFF)->SetWindowText( str);

  theApp.GetLogger()->LogInfo( "CT7_slg_pc_editorDlg::OnInitDialog(): out");

  /*VARIANT var;
  var.vt = VT_I4;
  var.lVal = 1;     m_ctlDtpDevDateOut.SetDay( var);
  var.lVal = 1;     m_ctlDtpDevDateOut.SetMonth( var);
  var.lVal = 2015;  m_ctlDtpDevDateOut.SetYear( var);
  */
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CT7_slg_pc_editorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CT7_slg_pc_editorDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CT7_slg_pc_editorDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CT7_slg_pc_editorDlg::SetControlsState( BOOL bDeviceConnectionPresent, BOOL bCanConnect)
{
  GetDlgItem( IDC_CMB_COM)->EnableWindow( !bDeviceConnectionPresent);
	GetDlgItem( IDC_CMB_PORT_SPEED)->EnableWindow( !bDeviceConnectionPresent);
	GetDlgItem( IDC_BTN_CONNECT)->EnableWindow( bCanConnect);
	GetDlgItem( IDC_BTN_DISCONNECT)->EnableWindow( bDeviceConnectionPresent);


  //амплитуда
  GetDlgItem( IDC_BTN_REQUEST_AMPL)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_AMPL_CODE_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_AMPL)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //код такта подставки
  GetDlgItem( IDC_BTN_REQUEST_TACT_CODE)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_TACT_CODE_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_TACTCODE)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //коэффициент ошумления
  GetDlgItem( IDC_BTN_REQUEST_M_COEFF)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
	GetDlgItem( IDC_EDT_M_COEFF_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_M_COEFF)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //начальная мода
  GetDlgItem( IDC_BTN_REQUEST_START_MODE)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
	GetDlgItem( IDC_EDT_START_MODE_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);  
  GetDlgItem( IDC_BTN_SEND_START_MODE)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //коэффициент вычета
  GetDlgItem( IDC_BTN_REQUEST_DEC_COEFF)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_DEC_COEFF_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_DEC_COEFF)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  // ***** ***** ***** ***** ***** ***** ***** ***** *****
  //Минимальный AmplAng
  GetDlgItem( IDC_BTN_REQUEST_CONTROL_AA)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_MIN_AMPL_ANG_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_AMPLANG_MIN)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //минимальный I1
  GetDlgItem( IDC_BTN_REQUEST_CONTROL_I1)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_MIN_CUR1_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_I1_MIN)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //минимальный I2
  GetDlgItem( IDC_BTN_REQUEST_CONTROL_I2)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_MIN_CUR2_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_I2_MIN)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //Кол-во импульсов поджига
  GetDlgItem( IDC_BTN_REQUEST_HV_APPLY_SET)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_HV_APPLY_SET_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_HV_APPLY_SET)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //Длительность импульсов поджига
  GetDlgItem( IDC_BTN_REQUEST_HV_APPLY_DUR_SET)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_HV_APPLY_DUR_SET_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_HV_APPLY_DUR_SET)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //Количество пачек импульсов поджига
  GetDlgItem( IDC_BTN_REQUEST_HV_APPLY_PACKS)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_HV_APPLY_PACKS_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_HV_APPLY_PACKS)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  // ***** ***** ***** ***** ***** ***** ***** ***** *****
  //Знаковый коэффициент
  GetDlgItem( IDC_BTN_REQUEST_SIGN_COEFF)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_SIGN_COEFF_OUT)->EnableWindow( FALSE);//bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_SPIN_SIGNCOEFF)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_SIGNCOEFF)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //серийный номер
  GetDlgItem( IDC_BTN_REQUEST_SERNUM)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_SER_NUM_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_SERIAL)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //Дата прибора
  GetDlgItem( IDC_BTN_REQUEST_DATE)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_DTP_DATE_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_DATE)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);

  //организация
  GetDlgItem( IDC_BTN_REQUEST_ORG)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_EDT_ORGANIZATION_OUT)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
  GetDlgItem( IDC_BTN_SEND_ORGANIZATION)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);


  /*
	GetDlgItem( IDC_EDT_TCALIB_T1_TD1)->EnableWindow( bDeviceConnectionPresent);
	GetDlgItem( IDC_EDT_TCALIB_T1_TD2)->EnableWindow( bDeviceConnectionPresent);
	GetDlgItem( IDC_EDT_TCALIB_T2_TD1)->EnableWindow( bDeviceConnectionPresent);
	GetDlgItem( IDC_EDT_TCALIB_T2_TD2)->EnableWindow( bDeviceConnectionPresent);
  */
	GetDlgItem( IDC_BTN_RESET_CALIB)->EnableWindow( bDeviceConnectionPresent && !theApp.m_bLockBit);
		
  GetDlgItem( IDC_BTN_LOCK_DEVICE)->EnableWindow( bDeviceConnectionPresent);
  GetDlgItem( IDC_BTN_LOCK_DEVICE)->SetWindowText( theApp.m_bLockBit ? "Разблокировать устройство" : "Заблокировать устройство");
}

void CT7_slg_pc_editorDlg::OnDestroy() 
{
	UpdateData( TRUE);
	CDialog::OnDestroy();
	
	(( CT7_slg_pc_editorApp *) AfxGetApp())->SetComPort( m_nComPort);
	(( CT7_slg_pc_editorApp *) AfxGetApp())->SetComBaudrate( m_nPortSpeed);
}

void CT7_slg_pc_editorDlg::OnBtnConnect() 
{
  UpdateData( TRUE);
  m_ctlCOM.SetCommPort( 1 + m_nComPort);
  switch( m_nPortSpeed) {
    case 0: m_ctlCOM.SetSettings( _T("57600,N,8,1")); break;
    case 1: m_ctlCOM.SetSettings( _T("115200,N,8,1")); break;
    case 2: m_ctlCOM.SetSettings( _T("128000,N,8,1")); break;
    case 3: m_ctlCOM.SetSettings( _T("256000,N,8,1")); break;
    case 4: m_ctlCOM.SetSettings( _T("460800,N,8,1")); break;
    case 5: m_ctlCOM.SetSettings( _T("512000,N,8,1")); break;
    case 6: m_ctlCOM.SetSettings( _T("921600,N,8,1")); break;
  }

	gl_nReceivedPacks = 0;
  m_ctlCOM.SetPortOpen( true);

  //SetTimer( TIMER_FLUSH_COM, 100, NULL);
  
  
  m_bStopBigThreadFlag = FALSE;
  
  DWORD id2;
  gl_hThread = ::CreateThread(0, 0, &BigThread, 0, 0, &id2);

  GetDlgItem( IDC_BTN_CONNECT)->EnableWindow( FALSE);
  SetTimer( TIMER_REFRESH_DATA, 500, NULL);
  SetTimer( TIMER_POLLER, 100, NULL);

  m_nPollCounter = 0;
  
  gl_nMarkerFails = 0;
  gl_nCheckSummFails = 0;
}

void CT7_slg_pc_editorDlg::OnBtnDisconnect() 
{
	m_ctlCOM.SetPortOpen( false);

	KillTimer( TIMER_FLUSH_COM);
  KillTimer( TIMER_REFRESH_DATA);
  KillTimer( TIMER_POLLER);

	m_bStopBigThreadFlag = true;
		
  SetControlsState( FALSE, TRUE);

	gl_nCircleBufferGet = 0;
  gl_nCircleBufferPut = 0;	
}

void CT7_slg_pc_editorDlg::OnTimer(UINT nIDEvent) 
{
  switch( nIDEvent) {
    case TIMER_POLLER:
      if( m_ctlCOM.GetPortOpen()) {
        if( m_queCommandQueue.size() < 5) {          
          CMcCommandItem *item = new CMcCommandItem(
              MC_COMMAND_REQ,
              m_nPollParams[ m_nPollCounter],
              0, 0);
          m_queCommandQueue.push( item);
          m_nPollCounter = ( ++m_nPollCounter) % POLL_PARAMS_LEN;
        }
      }
      else {
        KillTimer( TIMER_POLLER);
      }
    break;

    case TIMER_FLUSH_COM:
      //theApp.GetLogger()->LogTrace( "CT7_slg_pc_editorDlg::OnTimer(TIMER_FLUSH_COM)");
      if( m_ctlCOM.GetPortOpen()) {
				
				short nBytes = m_ctlCOM.GetInBufferCount();
				if( nBytes > 10) {
					VARIANT var = m_ctlCOM.GetInput();
					//m_ctlCOM.SetInBufferCount(0);
					
					char *buff;//[2048];
					int count = var.parray->rgsabound[0].cElements;

          theApp.GetLogger()->LogInfo( "nB: %d   Cn: %d", nBytes, count);

					buff = new char[count];
					/*
					if( count<0 || count > 2048) {
						CString strError;
						strError.Format( _T("COUNT: %d"), count);
						AfxMessageBox( strError);
						Beep( 1000, 1000);
					}
					*/
					memcpy( buff, var.parray->pvData, count);

					for( int qq=0; qq<count; qq++) {
						//fprintf( ((CSlg2App*) AfxGetApp())->fhb, "%x", buff[qq]);
						if( !PutByteInCircleBuffer( buff[qq])) {
							m_ctlCOM.SetPortOpen( false);
							for( int err=0; err < ERROR_2; err++) {
								Beep( 100, 100);
								Sleep( 200);
							}
							theApp.m_nEmergencyCode = 1002;
							m_strLblEmergency.Format( _T("1002"));
							GetDlgItem( IDC_LBL_EMERGENCY)->SetWindowText( m_strLblEmergency);

							m_ctlCOM.SetPortOpen( false);

							KillTimer( TIMER_FLUSH_COM);
              KillTimer( TIMER_REFRESH_DATA);
              
              
							m_bStopBigThreadFlag = true;
		

							gl_nCircleBufferGet = 0;
							gl_nCircleBufferPut = 0;

              SetControlsState( FALSE, FALSE);
						}
					}
          delete buff;
				}
			}
			else {
				for( int err=0; err < ERROR_5; err++) {
					Beep( 100, 100);
					Sleep( 200);
				}

        KillTimer( TIMER_FLUSH_COM);
        KillTimer( TIMER_REFRESH_DATA);
        
        
        m_bStopBigThreadFlag = true;
		

				gl_nCircleBufferGet = 0;
				gl_nCircleBufferPut = 0;

				theApp.m_nEmergencyCode = 1005;
				m_strLblEmergency.Format( _T("1005"));
				GetDlgItem( IDC_LBL_EMERGENCY)->SetWindowText( m_strLblEmergency);

        SetControlsState( FALSE, FALSE);
			}
    break;

    case TIMER_COM_STATE:
      UpdateData( TRUE);
      theApp.GetLogger()->LogTrace( "CT7_slg_pc_editorDlg::OnTimer(TIMER_COM_STATE)");
      if( m_ctlCOM.GetPortOpen()) {
        
        COleDateTimeSpan span = COleDateTime::GetCurrentTime() - theApp.GetLastIncomingData();
        if( span.GetTotalSeconds() > 1) {
          m_strConnectionStatus = "COM-порт открыт. Потока данных нет.";
        }
        else {
          m_strConnectionStatus = "Соединено. Поток данных есть.";
        }
      }
      else
        m_strConnectionStatus = "Разъединено";
      
      GetDlgItem( IDC_STR_CONN_STATE)->SetWindowText( m_strConnectionStatus);
    break;

    case TIMER_REFRESH_DATA: {
      theApp.GetLogger()->LogTrace( "CT7_slg_pc_editorDlg::OnTimer:TIMER_REFRESH_DATA: theApp.m_dblAngl=%f", theApp.m_dblAmpl);
      
      CString strTmp;

      //Код амплитуды
      strTmp.Format( "%.2f", theApp.m_dblAmpl);
      GetDlgItem( IDC_EDT_AMPL_CODE_IN)->SetWindowText( strTmp);
      
      //Код такта подставки
      strTmp.Format( "%d", theApp.m_nHangerTact);
      GetDlgItem( IDC_EDT_TACT_CODE_IN)->SetWindowText( strTmp);

      //Коэффициент М
      strTmp.Format( "%.2f", theApp.m_dblMcoeff);
      GetDlgItem( IDC_EDT_M_COEFF_IN)->SetWindowText( strTmp);

      //Начальная мода
      strTmp.Format( "%.2f", ( double) theApp.m_nStartMode / 100.);
      GetDlgItem( IDC_EDT_START_MODE_IN)->SetWindowText( strTmp);
      
      //Коэффициент вычета
      strTmp.Format( "%.5f", theApp.m_dblDecCoeff);
      GetDlgItem( IDC_EDT_DEC_COEFF_IN)->SetWindowText( strTmp);

      // ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
      //Минимальный ток I1
      strTmp.Format( "%.4f", theApp.m_dblControlI1min);
      GetDlgItem( IDC_EDT_MIN_CUR1_IN)->SetWindowText( strTmp);
      
      //Минимальный ток I2
      strTmp.Format( "%.4f", theApp.m_dblControlI2min);
      GetDlgItem( IDC_EDT_MIN_CUR2_IN)->SetWindowText( strTmp);

      //Минимальный AmplAng
      strTmp.Format( "%.4f", theApp.m_dblAmplAnglmin);
      GetDlgItem( IDC_EDT_MIN_AMPL_ANG_IN)->SetWindowText( strTmp);

      //Количество импульсов в пачке применения 3kV (поджига)
      strTmp.Format( "%d", theApp.m_nHVApplyMax);
      GetDlgItem( IDC_EDT_HV_APPLY_SET_IN)->SetWindowText( strTmp);

      //Длительность применения 3kV
      strTmp.Format( "%d", theApp.m_nHVApplyDuration);
      GetDlgItem( IDC_EDT_HV_APPLY_DUR_SET_IN)->SetWindowText( strTmp);

      //Количество пачек применения 3kV (поджига)
      strTmp.Format( "%d", theApp.m_nHVApplyPacks);
      GetDlgItem( IDC_EDT_HV_APPLY_PACKS_IN)->SetWindowText( strTmp);

      // ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
      //Знаковый коэффициент
      strTmp.Format( "%d", theApp.m_nSignCoeff);
      GetDlgItem( IDC_EDT_SIGN_COEFF_IN)->SetWindowText( strTmp);

      //серийный номер
      strTmp.Format( "%d", theApp.m_shDeviceSerialNumber);
      GetDlgItem( IDC_EDT_SER_NUM_IN)->SetWindowText( strTmp);

      //дата прибора
      VARIANT varIndex;
      varIndex.vt = VT_I4;
      
      varIndex.lVal = theApp.m_nDateY;      
      m_ctlDtpDevDateIn.SetYear( varIndex);

      varIndex.lVal = theApp.m_nDateD;
      m_ctlDtpDevDateIn.SetDay( varIndex);

      varIndex.lVal = theApp.m_nDateM;
      m_ctlDtpDevDateIn.SetMonth( varIndex);

      


      //организация
      //m_strOrganization = theApp.m_strOrganization;

      // ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
      //версия ПО
      strTmp.Format( _T("%d.%d.%d"), theApp.m_nMajorVersion, theApp.m_nMiddleVersion, theApp.m_nMinorVersion);
      GetDlgItem( IDC_STR_MC_VER)->SetWindowText( strTmp);

      //ошибки начала протокола
      strTmp.Format( "%d", gl_nMarkerFails);
      GetDlgItem( IDC_LBL_MFAILS)->SetWindowText( strTmp);
      
      //ошибки конца протокола
      strTmp.Format( "%d", gl_nCheckSummFails);
      GetDlgItem( IDC_LBL_CSFAILS)->SetWindowText( strTmp);
      
      //принятых посылок
      strTmp.Format( "%d", gl_nReceivedPacks);
      GetDlgItem( IDC_LBL_PACKS)->SetWindowText( strTmp);
      
      //код ошибки
      strTmp.Format( "%d", theApp.m_nEmergencyCode);
      GetDlgItem( IDC_LBL_EMERGENCY)->SetWindowText( strTmp);      
    }
    break;

    case TIMER_SEND_CMDS_TO_MC:
      if( m_queCommandQueue.size() > 0) {
        //SetControlsState( FALSE, FALSE);
        CMcCommandItem *item = ( CMcCommandItem *) m_queCommandQueue.front();
        m_queCommandQueue.pop();
        SendCommandToMc( item->m_nCommand, item->m_nParam1, item->m_nParam2, item->m_nParam3);
        theApp.GetLogger()->LogDebug( "CT7_slg_pc_editorDlg::OnTimer( TIMER_SEND_CMDS_TO_MC): 0x%02x 0x%02x 0x%02x 0x%02x",
              item->m_nCommand, item->m_nParam1, item->m_nParam2, item->m_nParam3);
        
        if( item->m_nCommand == MC_COMMAND_SET)
            theApp.GetLogger()->LogDebug( "CT7_slg_pc_editorDlg::OnTimer( TIMER_SEND_CMDS_TO_MC): SET 0x%02x 0x%02x 0x%02x",
              item->m_nParam1, item->m_nParam2, item->m_nParam3);

        delete item;
      }
      else {
        //SetControlsState( m_ctlCOM.GetPortOpen(), !m_ctlCOM.GetPortOpen());
      }
      SetControlsState( m_ctlCOM.GetPortOpen(), !m_ctlCOM.GetPortOpen());
    break;
  }
	CDialog::OnTimer(nIDEvent);
}

void CT7_slg_pc_editorDlg::OnChangeEdtOrganisationOut() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData( TRUE);
  if( m_strOrganizationOut.GetLength() >=16) {
    m_strOrganizationOut = m_strOrganizationOut.Left( 16);
    //GetDlgItem( IDC_EDT_ORGANIZATION_OUT)->SetWindowText( m_strOrganizationOut);
  }	
}

void CT7_slg_pc_editorDlg::OnBtnApplyScaleCoeff() 
{
  UpdateData( TRUE);
  
  CString str;
  GetDlgItem( IDC_EDT_SCALE_COEFF)->GetWindowText( str);
  double dblScaleCoeff = atof( str);
  if( dblScaleCoeff < 2.5 || dblScaleCoeff > 3.3) {
    dblScaleCoeff = 2.9;    
  }
  theApp.m_dblScaleCoeff = dblScaleCoeff;

  str.Format( _T("%.3f"), theApp.m_dblScaleCoeff);
  GetDlgItem( IDC_EDT_SCALE_COEFF)->SetWindowText( str);  
}

void CT7_slg_pc_editorDlg::OnKillfocusEdtSignCoeffOut() 
{
  int nPrevVal = theApp.m_nSignCoeff;
  UpdateData( TRUE);
  if( m_nSignCoeffIn != 1 || m_nSignCoeffIn != -1) {
    m_nSignCoeffIn = nPrevVal;
    CString strTmp;
    strTmp.Format( "%d", m_nSignCoeffIn);
    GetDlgItem( IDC_EDT_SIGN_COEFF_OUT)->SetWindowText( strTmp);
  }
}

void CT7_slg_pc_editorDlg::SendCommandToMc(BYTE b1, BYTE b2, BYTE b3, BYTE b4)
{
  char str[4];
	str[0] = b1;
	str[1] = b2;
	str[2] = b3;
  str[3] = b4;

	SAFEARRAY *psa;
  SAFEARRAYBOUND rgsabound[1];
  rgsabound[0].lLbound = 0;
  rgsabound[0].cElements = 4;
  psa = SafeArrayCreate(VT_UI1, 1, rgsabound);
	memcpy( psa->pvData, str, 4);
	
	VARIANT var;
	var.vt = VT_ARRAY | VT_UI1;
	var.parray = psa;

	if( m_ctlCOM.GetPortOpen()) {
		m_ctlCOM.SetOutput( var);
		int s = m_ctlCOM.GetOutBufferCount();
	}
	else {
		Beep( 5000, 100);
	}

	SafeArrayDestroy( psa);
}







//DEL void CT7_slg_pc_editorDlg::SetInteractionState(BOOL bEnable)
//DEL {
//DEL   GetDlgItem( IDC_BTN_REQUEST_PARAMS)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_AMPL_CODE_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_AMPL)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_TACT_CODE_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_TACTCODE)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_M_COEFF_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_M_COEFF)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_START_MODE_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_START_MODE)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_MIN_CUR1_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_I1_MIN)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_MIN_CUR2_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_I2_MIN)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_MIN_AMPL_ANG_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_AMPLANG_MIN)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_DEC_COEFF_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_DEC_COEFF)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_SIGN_COEFF_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_SIGNCOEFF)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_EDT_PHASE_SHIFT_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_PHASE_SHIFT)->EnableWindow( bEnable);
//DEL   
//DEL   GetDlgItem( IDC_BTN_REQUEST_SERNUM)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_EDT_SER_NUM_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_SERIAL)->EnableWindow( bEnable);
//DEL   
//DEL   GetDlgItem( IDC_BTN_REQUEST_DATE)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_DTP_DATE_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_DATE)->EnableWindow( bEnable);
//DEL 
//DEL   GetDlgItem( IDC_BTN_REQUEST_ORG)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_EDT_ORGANISATION_OUT)->EnableWindow( bEnable);
//DEL   GetDlgItem( IDC_BTN_SEND_ORGANIZATION)->EnableWindow( bEnable);
//DEL }

void CT7_slg_pc_editorDlg::OnSelchangeCmbCom() 
{
  UpdateData( TRUE);
}

void CT7_slg_pc_editorDlg::OnSelchangeCmbPortSpeed() 
{
  UpdateData( TRUE);
}



void CT7_slg_pc_editorDlg::OnBtnSendCalib() 
{
  /*
  UpdateData( TRUE);
	short val = ( short) m_nCalibCurrentTemperature;
	val += THERMO_CALIB_PARAMS_BASE;
  CMcCommandItem *item = new CMcCommandItem( 11 ??????? , 0, 0);
  m_queCommandQueue.push( item);
	//SendCommandToMc( 11, (char) ( val & 0xFF), (char) ( ( val & 0xFF00) >> 8));
  */
}

void CT7_slg_pc_editorDlg::OnBtnRequestAmpl() 
{
  //Запрос параметра "амплитуда колебаний"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, AMPLITUDE, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_AMPLITUDE, 0, 0);  
}

void CT7_slg_pc_editorDlg::OnBtnRequestTactCode() 
{
  //Запрос параметра "Код такта подставки"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, TACT_CODE, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_CODETACT, 0, 0);  
}

void CT7_slg_pc_editorDlg::OnBtnRequestMCoeff() 
{
	//Запрос параметра "Коэффициент М"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, M_COEFF, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_M_COEFF, 0, 0);  
}

void CT7_slg_pc_editorDlg::OnBtnRequestStartMode() 
{
  //Запрос параметра "Начальная мода"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, STARTMODE, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_START_MODE, 0, 0);
}

void CT7_slg_pc_editorDlg::OnBtnRequestControlI1() 
{
  //Запрос параметра "Контрольный ток I1"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, CONTROL_I1, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_CONTROL_I1, 0, 0);
}

void CT7_slg_pc_editorDlg::OnBtnRequestControlI2() 
{
	//Запрос параметра "Контрольный ток I2"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, CONTROL_I2, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_CONTROL_I2, 0, 0);
}

void CT7_slg_pc_editorDlg::OnBtnRequestControlAa() 
{
  //Запрос параметра "Контрольное значение AmplAng"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, CONTROL_AA, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_CONTROL_AA, 0, 0);	
}

void CT7_slg_pc_editorDlg::OnBtnRequestDecCoeff() 
{
	//Запрос параметра "Коэффициент вычета"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, DECCOEFF, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_DEC_COEFF, 0, 0);	
}

void CT7_slg_pc_editorDlg::OnBtnRequestSignCoeff() 
{
  //Запрос параметра "Знаковый коэффициент dU"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, SIGNCOEFF, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_SIGN_COEFF, 0, 0);	
}

void CT7_slg_pc_editorDlg::OnBtnRequestHvApplySet() 
{
  //Запрос параметра "Количество импульсов 3kV"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, HV_APPLY_COUNT_SET, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_HV_APPLY_DURATION, 0, 0);	
}

void CT7_slg_pc_editorDlg::OnBtnRequestHvApplyDurSet() 
{
	//Запрос параметра "Длительность импульсов 3kV"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, HV_APPLY_DURAT_SET, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_HV_APPLY_DURATION, 0, 0);	
}

void CT7_slg_pc_editorDlg::OnBtnRequestSernum() 
{
  //Запрос параметра "Серийный номер"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, DEVNUM, 0, 0);
  m_queCommandQueue.push( item);
}

void CT7_slg_pc_editorDlg::OnBtnRequestDate() 
{
  //Запрос параметра "Дата прибора"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, DATE_Y, 0, 0);
  m_queCommandQueue.push( item);
  item = new CMcCommandItem( MC_COMMAND_REQ, DATE_M, 0, 0);
  m_queCommandQueue.push( item);
  item = new CMcCommandItem( MC_COMMAND_REQ, DATE_D, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_DEV_DATE, 0, 0);
}

void CT7_slg_pc_editorDlg::OnBtnRequestOrg() 
{
  //Запрос параметра "Организация"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, ORG, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_ORGANIZATION, 0, 0);
}

void CT7_slg_pc_editorDlg::OnBtnRequestCalib() 
{
	//Запрос данных температурной калибровки
  CMcCommandItem *item;
  item = new CMcCommandItem( MC_COMMAND_REQ, CALIB_T1, 0, 0);
  m_queCommandQueue.push( item);
  item = new CMcCommandItem( MC_COMMAND_REQ, T1_TD1, 0, 0);
  m_queCommandQueue.push( item);
  item = new CMcCommandItem( MC_COMMAND_REQ, T1_TD2, 0, 0);
  m_queCommandQueue.push( item);
  item = new CMcCommandItem( MC_COMMAND_REQ, T1_TD3, 0, 0);
  m_queCommandQueue.push( item);

  item = new CMcCommandItem( MC_COMMAND_REQ, CALIB_T2, 0, 0);
  m_queCommandQueue.push( item);
  item = new CMcCommandItem( MC_COMMAND_REQ, T2_TD1, 0, 0);
  m_queCommandQueue.push( item);
  item = new CMcCommandItem( MC_COMMAND_REQ, T2_TD2, 0, 0);
  m_queCommandQueue.push( item);
  item = new CMcCommandItem( MC_COMMAND_REQ, T2_TD3, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_GET_T_CALIB, 0, 0);
}


/***********************************
* команды от кнопок "скопировать"
***********************************/

void CT7_slg_pc_editorDlg::OnBtnCpReadToSentAmpl() 
{
  //скопировать значение параметра "амплитуда" полученное в готовое к высылке
  UpdateData( TRUE);
  m_dblAmplCodeOut = theApp.m_dblAmpl;
  CString strTmp;
  strTmp.Format( "%.2f", m_dblAmplCodeIn);
  GetDlgItem( IDC_EDT_AMPL_CODE_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentTact() 
{
	//скопировать значение параметра "Код такта подставки" полученное в готовое к высылке
  UpdateData( TRUE);
  m_nTactCodeOut = m_nTactCodeIn;
  CString strTmp;
  strTmp.Format( "%d", m_nTactCodeIn);
  GetDlgItem( IDC_EDT_TACT_CODE_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentMcoeff() 
{
  //скопировать значение параметра "Коэффициент ошумления" полученное в готовое к высылке
  UpdateData( TRUE);
  m_dblMCoeffOut = theApp.m_dblMcoeff;
  CString strTmp;
  strTmp.Format( "%.2f", m_dblMCoeffIn);
  GetDlgItem( IDC_EDT_M_COEFF_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentStartmode() 
{
	//скопировать значение параметра "Начальная мода" полученное в готовое к высылке
  UpdateData( TRUE);
  m_dblStartModeOut = ( double) theApp.m_nStartMode / 100.;
  CString strTmp;
  strTmp.Format( "%.2f", m_dblStartModeIn);
  GetDlgItem( IDC_EDT_START_MODE_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentDeccoeff() 
{
  //скопировать значение параметра "Коэффициент вычета" полученное в готовое к высылке
	UpdateData( TRUE);
  m_dblDecCoeffOut = theApp.m_dblDecCoeff;
  CString strTmp;
  strTmp.Format( "%.5f", m_dblDecCoeffIn);
  GetDlgItem( IDC_EDT_DEC_COEFF_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentAmplangmin() 
{
	//скопировать значение параметра "Минимальный AmplAng" полученное в готовое к высылке
	UpdateData( TRUE);
  m_dblMinAmplAngOut = m_dblMinAmplAngIn;
  CString strTmp;
  strTmp.Format( "%.2f", m_dblMinAmplAngIn);
  GetDlgItem( IDC_EDT_MIN_AMPL_ANG_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentI1min() 
{
	//скопировать значение параметра "Минимальный ток I1" полученное в готовое к высылке
	UpdateData( TRUE);
  m_dblMinI1Out = m_dblMinI1In;
  CString strTmp;
  strTmp.Format( "%.2f", m_dblMinI1In);
  GetDlgItem( IDC_EDT_MIN_CUR1_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentI2min() 
{
	//скопировать значение параметра "Минимальный ток I2" полученное в готовое к высылке
	UpdateData( TRUE);
  m_dblMinI2Out = m_dblMinI2In;
  CString strTmp;
  strTmp.Format( "%.2f", m_dblMinI2In);
  GetDlgItem( IDC_EDT_MIN_CUR2_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentHvApplySet() 
{
	//скопировать значение параметра "Количество HV импульсов" полученное в готовое к высылке
  UpdateData( TRUE);
  m_nHvApplyOut = m_nHvApplyIn;
  CString strTmp;
  strTmp.Format( "%d", m_nHvApplyIn);
  GetDlgItem( IDC_EDT_HV_APPLY_SET_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentHvApplyDurSet() 
{
	//скопировать значение параметра "Длительность HV импульсов" полученное в готовое к высылке
	UpdateData( TRUE);
  m_nHvApplyDurOut = m_nHvApplyDurIn;
  CString strTmp;
  strTmp.Format( "%d", m_nHvApplyDurIn);
  GetDlgItem( IDC_EDT_HV_APPLY_DUR_SET_OUT)->SetWindowText( strTmp);
}


void CT7_slg_pc_editorDlg::OnBtnSetreadtosentSign() 
{
	//скопировать значение параметра "Знаковый коэффициент" полученное в готовое к высылке
  UpdateData( TRUE);
  m_nSignCoeffOut = m_nSignCoeffIn;
  CString strTmp;
  strTmp.Format( "%d", m_nSignCoeffIn);
  GetDlgItem( IDC_EDT_SIGN_COEFF_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentDevsernum() 
{
  //скопировать значение параметра "Серийный номер" полученное в готовое к высылке
  UpdateData( TRUE);
  m_nDeviceSerialNumOut = m_nDeviceSerialNumIn;
  CString strTmp;
  strTmp.Format( "%d", m_nDeviceSerialNumIn);
  GetDlgItem( IDC_EDT_SER_NUM_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentDate() 
{
  //скопировать значение параметра "Дата ??? прибора" полученное в готовое к высылке
	VARIANT var = m_ctlDtpDevDateIn.GetYear();
  m_ctlDtpDevDateOut.SetYear( var);
  var = m_ctlDtpDevDateIn.GetMonth();
  m_ctlDtpDevDateOut.SetMonth( var);
  var = m_ctlDtpDevDateIn.GetDay();
  m_ctlDtpDevDateOut.SetDay( var);
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentOrg() 
{
	//скопировать значение параметра "Организация" полученное в готовое к высылке
  UpdateData( TRUE);
  m_strOrganizationOut = m_strOrganizationIn;
  GetDlgItem( IDC_EDT_ORGANIZATION_OUT)->SetWindowText( m_strOrganizationIn);
}


/***********************************
* команды от кнопок "послать"
***********************************/
void CT7_slg_pc_editorDlg::OnBtnSendAmpl() 
{
	//команда "послать значение амплитуды"
  UpdateData( TRUE);
  if( m_dblAmplCodeOut < 10 || m_dblAmplCodeOut > 250) {
    m_dblAmplCodeOut = m_dblAmplCodeIn;
    
    CString strTmp;
    strTmp.Format( "%.2f", m_dblAmplCodeIn);
    GetDlgItem( IDC_EDT_AMPL_CODE_OUT)->SetWindowText( strTmp);
  }
  
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_SET,
                            AMPLITUDE,
                            ( char) ( m_dblAmplCodeOut / m_dblScaleCoeff),
                            0);
  m_queCommandQueue.push( item);

  //SendCommandToMc( MC_COMMAND_SET_AMPLITUDE, ( char) ( m_dblAmplCodeOut / m_dblScaleCoeff), 0);
}

void CT7_slg_pc_editorDlg::OnBtnSendTactcode() 
{
	//команда "послать код такта подставки"
	UpdateData( TRUE);
  if( m_nTactCodeOut < 0 || m_nTactCodeOut > 2) {
    m_nTactCodeOut = m_nTactCodeIn;
    
    CString strTmp;
    strTmp.Format( "%d", m_nTactCodeIn);
    GetDlgItem( IDC_EDT_TACT_CODE_OUT)->SetWindowText( strTmp);
  }
  
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_SET,
                            TACT_CODE,
                            ( char) m_nTactCodeOut,
                            0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_SET_CODETACT, ( char) m_nTactCodeOut, 0);
}

void CT7_slg_pc_editorDlg::OnBtnSendMCoeff() 
{
	//команда "послать коэффициент ошумления"

}

void CT7_slg_pc_editorDlg::OnBtnSendStartMode() 
{
	//команда "послать начальную моду"
  UpdateData( TRUE);
  if( m_dblStartModeOut > 2.5) m_dblStartModeOut = 2.5;
  if( m_dblStartModeOut < 0.)  m_dblStartModeOut = 0.;
  short code = ( short) ( m_dblStartModeOut * 100.);
  unsigned char b1 = code & 0xFF;
  unsigned char b2 = (code & 0xFF00)  >> 8;

  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_SET,
                            STARTMODE,
                            b1, b2);
  m_queCommandQueue.push( item);
	//SendCommandToMc( MC_COMMAND_SET_CONTROL_I1, b1, b2);

  CString strTmp;
  strTmp.Format( "%.2f", m_dblStartModeOut);
  GetDlgItem( IDC_EDT_START_MODE_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSendDecCoeff() 
{
	//команда "послать коэффициент вычета"
}

void CT7_slg_pc_editorDlg::OnBtnSendI1Min() 
{
	//команда "послать контрольный ток поджига I1"
	UpdateData( TRUE);
  if( m_dblMinI1Out > 0.75) m_dblMinI1Out = 0.75;
  if( m_dblMinI1Out < 0.)   m_dblMinI1Out = 0.;
  short code = ( short) ( m_dblMinI1Out / 0.75 * 65535.);
  unsigned char b1 = code & 0xFF;
  unsigned char b2 = (code & 0xFF00)  >> 8;

  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_SET,
                            CONTROL_I1,
                            b1, b2);
  m_queCommandQueue.push( item);
	//SendCommandToMc( MC_COMMAND_SET_CONTROL_I1, b1, b2);

  CString strTmp;
  strTmp.Format( "%.2f", m_dblMinI1Out);
  GetDlgItem( IDC_EDT_MIN_CUR1_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSendI2Min() 
{
	//команда "послать контрольный ток поджига I2"
	UpdateData( TRUE);
  if( m_dblMinI2Out > 0.75) m_dblMinI2Out = 0.75;
  if( m_dblMinI2Out < 0.)   m_dblMinI2Out = 0.;
  short code = ( short) ( m_dblMinI2Out / 0.75 * 65535.);
  unsigned char b1 = code & 0xFF;
  unsigned char b2 = (code & 0xFF00)  >> 8;

  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_SET,
                            CONTROL_I2,
                            b1, b2);
  m_queCommandQueue.push( item);
	//SendCommandToMc( MC_COMMAND_SET_CONTROL_I2, b1, b2);
  
  CString strTmp;
  strTmp.Format( "%.2f", m_dblMinI2Out);
  GetDlgItem( IDC_EDT_MIN_CUR2_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSendAmplangMin() 
{
	//команда "послать контрольное напряжение AA"
  UpdateData( TRUE);
  if( m_dblMinAmplAngOut > 3.) m_dblMinAmplAngOut = 3.;
  if( m_dblMinAmplAngOut < 0.) m_dblMinAmplAngOut = 0.;
  short code = ( short) ( m_dblMinAmplAngOut / 3.0 * 65534.);
  unsigned char b1 = code & 0xFF;
  unsigned char b2 = (code & 0xFF00)  >> 8;

  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_SET,
                            CONTROL_AA,
                            b1, b2);
  m_queCommandQueue.push( item);
	//SendCommandToMc( MC_COMMAND_SET_CONTROL_AA, b1, b2);

  CString strTmp;
  strTmp.Format( "%.2f", m_dblMinAmplAngOut);
  GetDlgItem( IDC_EDT_MIN_AMPL_ANG_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSendHvApplySet() 
{
	//команда "послать количество HV импульсов в пачке"
	UpdateData( TRUE);
  CMcCommandItem *item;
  item = new CMcCommandItem( MC_COMMAND_SET,
                                  HV_APPLY_COUNT_SET,
                                  ( m_nHvApplyOut & 0xFF),
                                  0);
  m_queCommandQueue.push( item);

  item = new CMcCommandItem( MC_COMMAND_REQ,
                                  HV_APPLY_COUNT_SET,
                                  0,
                                  0);
  m_queCommandQueue.push( item);
}

void CT7_slg_pc_editorDlg::OnBtnSendHvApplyDurSet() 
{
	//команда "послать длительность HV импульсов"
	UpdateData( TRUE);
  CMcCommandItem *item;
  item = new CMcCommandItem( MC_COMMAND_SET,
                                  HV_APPLY_DURAT_SET,
                                  ( m_nHvApplyDurOut & 0xFF),
                                  ( m_nHvApplyDurOut & 0xFF00) >> 8);
  m_queCommandQueue.push( item);

  item = new CMcCommandItem( MC_COMMAND_REQ,
                                  HV_APPLY_DURAT_SET,
                                  0,
                                  0);
  m_queCommandQueue.push( item);
}

void CT7_slg_pc_editorDlg::OnBtnSendSigncoeff() 
{
	//команда "послать знаковый коэффициент dU"
  UpdateData( TRUE);
  if( m_nSignCoeffOut < 0) {
    m_nSignCoeffOut = -1;
    GetDlgItem( IDC_EDT_SIGN_COEFF_OUT)->SetWindowText( "-1");
  }
  else {
    m_nSignCoeffOut = 1;
    GetDlgItem( IDC_EDT_SIGN_COEFF_OUT)->SetWindowText( "1");
  }
	
  CMcCommandItem *item;
  item = new CMcCommandItem( MC_COMMAND_SET,
                                  SIGNCOEFF,
                                  ( ( m_nSignCoeffOut + 1) & 0xFF),
                                  0);
  m_queCommandQueue.push( item);
}

void CT7_slg_pc_editorDlg::OnBtnSendSerial() 
{
  //команда "послать серийный номер"
  UpdateData( TRUE);
  CMcCommandItem *item;
  item = new CMcCommandItem( MC_COMMAND_SET,
                                  DEVNUM,
                                  ( m_nDeviceSerialNumOut & 0xFF),
                                  ( m_nDeviceSerialNumOut & 0xFF00) >> 8);
  m_queCommandQueue.push( item);

  item = new CMcCommandItem( MC_COMMAND_REQ,
                                  DEVNUM,
                                  0,
                                  0);
  m_queCommandQueue.push( item);
  
  /*
  CString strTmp;
  strTmp.Format( "%d", m_nDeviceSerialNumOut);
  GetDlgItem( IDC_EDT_SER_NUM_OUT)->SetWindowText( strTmp);
  */
}

void CT7_slg_pc_editorDlg::OnBtnSendDate() 
{
  //команда "послать дату прибора"
  UpdateData( TRUE);
  VARIANT var = m_ctlDtpDevDateOut.GetYear();
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_SET,
                            DATE_Y,
                            ( var.lVal & 0xFF),
                            ( var.lVal & 0xFF00) >> 8);
  m_queCommandQueue.push( item);

  var = m_ctlDtpDevDateOut.GetMonth();
  item = new CMcCommandItem( MC_COMMAND_SET,
                            DATE_M,
                            var.lVal & 0xFFFF,
                            0);
  m_queCommandQueue.push( item);

  var = m_ctlDtpDevDateOut.GetDay();
  item = new CMcCommandItem( MC_COMMAND_SET,
                            DATE_D,
                            var.lVal & 0xFFFF,
                            0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_SET_DEVICE_ID, ( m_nDeviceSerialNumOut & 0xFF00) >> 8, ( m_nDeviceSerialNumOut & 0xFF));
  
  //UpdateData( FALSE);
}

void CT7_slg_pc_editorDlg::OnBtnSendOrganization() 
{
	UpdateData( TRUE);
  CMcCommandItem *item;
  for( int i=0; i<16; i++) {

    char symbol;
    if( i < m_strOrganizationOut.GetLength()) {
      symbol =  m_strOrganizationOut.GetAt( i);
    }
    else {
      symbol = ' ';
    }
    item = new CMcCommandItem( MC_COMMAND_SET, ORG_B1 + i, symbol, 0);
    m_queCommandQueue.push( item);
  }

  //GetDlgItem( IDC_EDT_ORGANIZATION_OUT)->SetWindowText( m_strOrganization);
}

void CT7_slg_pc_editorDlg::QueueCommandToMc(char btCmd, char btParam1, char btParam2, char btParam3)
{
  m_queCommandQueue.push( new CMcCommandItem( btCmd, btParam1, btParam2, btParam3));
}

BEGIN_EVENTSINK_MAP(CT7_slg_pc_editorDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CT7_slg_pc_editorDlg)
	ON_EVENT(CT7_slg_pc_editorDlg, IDC_MSCOMM, 1 /* OnComm */, OnOnCommMscomm, VTS_NONE)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CT7_slg_pc_editorDlg::OnOnCommMscomm() 
{
  short event = m_ctlCOM.GetCommEvent();
	theApp.GetLogger()->LogTrace( "CT7_slg_pc_editorDlg::OnOnCommMscomm(): in. event: %d. GetPortOpen(): %d", event, m_ctlCOM.GetPortOpen());	
  switch( event) {
    case 2:
      //theApp.GetLogger()->LogTrace( "CT7_slg_pc_editorDlg::OnTimer(TIMER_FLUSH_COM)");
      if( m_ctlCOM.GetPortOpen()) {
				
				short nBytes = m_ctlCOM.GetInBufferCount();
				if( nBytes > 10) {
					VARIANT var = m_ctlCOM.GetInput();
					m_ctlCOM.SetInBufferCount(0);
					
					char *buff;//[2048];
					int count = var.parray->rgsabound[0].cElements;

          theApp.GetLogger()->LogInfo( "nB: %d   Cn: %d", nBytes, count);

					buff = new char[count];
					/*
					if( count<0 || count > 2048) {
						CString strError;
						strError.Format( _T("COUNT: %d"), count);
						AfxMessageBox( strError);
						Beep( 1000, 1000);
					}
					*/
					memcpy( buff, var.parray->pvData, count);

					for( int qq=0; qq<count; qq++) {
						//fprintf( ((CSlg2App*) AfxGetApp())->fhb, "%x", buff[qq]);
						if( !PutByteInCircleBuffer( buff[qq])) {
							m_ctlCOM.SetPortOpen( false);
							for( int err=0; err < ERROR_2; err++) {
								Beep( 100, 100);
								Sleep( 200);
							}
							theApp.m_nEmergencyCode = 1002;
							m_strLblEmergency.Format( _T("1002"));
							GetDlgItem( IDC_LBL_EMERGENCY)->SetWindowText( m_strLblEmergency);

							m_ctlCOM.SetPortOpen( false);

							KillTimer( TIMER_FLUSH_COM);
              KillTimer( TIMER_REFRESH_DATA);
              
              
							m_bStopBigThreadFlag = true;
		

							gl_nCircleBufferGet = 0;
							gl_nCircleBufferPut = 0;

              SetControlsState( FALSE, FALSE);
						}
					}
          delete buff;
				}
			}
			else {
				for( int err=0; err < ERROR_5; err++) {
					Beep( 100, 100);
					Sleep( 200);
				}

        KillTimer( TIMER_FLUSH_COM);
        KillTimer( TIMER_REFRESH_DATA);
        
        
        m_bStopBigThreadFlag = true;
		

				gl_nCircleBufferGet = 0;
				gl_nCircleBufferPut = 0;

				theApp.m_nEmergencyCode = 1005;
				m_strLblEmergency.Format( _T("1005"));
				GetDlgItem( IDC_LBL_EMERGENCY)->SetWindowText( m_strLblEmergency);

        SetControlsState( FALSE, FALSE);
			}
    break;
  }
}


void CT7_slg_pc_editorDlg::OnBtnSaveToFlashP1() 
{
	//команда "сохранить параметры во флэш память p1"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_ACT_SAVE_FLASH_PARAMS, 0, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_ACT_SAVE_FLASH_PARAMS, 0, 0);
}

void CT7_slg_pc_editorDlg::OnBtnSaveToFlashP2() 
{
	//команда "сохранить параметры во флэш память p2"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_ACT_SAVE_FLASH_PARAMS, 1, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_ACT_SAVE_FLASH_PARAMS, 1, 0);
}

void CT7_slg_pc_editorDlg::OnBtnSaveToFlashP3() 
{
	//команда "сохранить параметры во флэш память p3"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_ACT_SAVE_FLASH_PARAMS, 2, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_ACT_SAVE_FLASH_PARAMS, 2, 0);
}

void CT7_slg_pc_editorDlg::OnBtnLockDevice() 
{
  //"Заблокировать устройство" или "Разблокировать устройство"
  if( theApp.m_bLockBit == 1) {
    //устройство заблокировано
    CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_ACT_UNLOCK_DEVICE, 0x5A, 0x55, 0x5A);
    m_queCommandQueue.push( item);

    MessageBox( _T("Необходимо сохранить страницу 1, и перезапустить прибор."));
  }
  else {
    //устройство открыто
    CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_ACT_LOCK_DEVICE, 0x55, 0x5A, 0x55);
    m_queCommandQueue.push( item);

    MessageBox( _T("Необходимо сохранить страницу 1, и перезапустить прибор."));
  }
  
}

void CT7_slg_pc_editorDlg::OnBtnVersion() 
{
  //команда "запросить версию"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, VERSION, 0, 0);
  m_queCommandQueue.push( item);
}

void CT7_slg_pc_editorDlg::OnDeltaposSpinSigncoeff( NMHDR *pNMHDR, LRESULT *pResult) 
{
	NM_UPDOWN *pNMUpDown = ( NM_UPDOWN *) pNMHDR;
  GetDlgItem( IDC_EDT_SIGN_COEFF_OUT)->SetWindowText( pNMUpDown->iDelta > 0 ? "-1" : "1");
	*pResult = 0;
}

void CT7_slg_pc_editorDlg::OnBtnRequestHvApplyPack() 
{
	//Запрос параметра "Количество пачек импульсов поджига"
  CMcCommandItem *item = new CMcCommandItem( MC_COMMAND_REQ, HV_APPLY_PACKS, 0, 0);
  m_queCommandQueue.push( item);
  //SendCommandToMc( MC_COMMAND_REQ_HV_APPLY_DURATION, 0, 0);	
}

void CT7_slg_pc_editorDlg::OnBtnSetreadtosentHvApplyPacks() 
{
	//скопировать значение параметра "Количество пачек импульсов поджига" полученное в готовое к высылке
	UpdateData( TRUE);
  m_nHvApplyPacksOut = m_nHvApplyPacksIn;
  CString strTmp;
  strTmp.Format( "%d", m_nHvApplyPacksIn);
  GetDlgItem( IDC_EDT_HV_APPLY_PACKS_OUT)->SetWindowText( strTmp);
}

void CT7_slg_pc_editorDlg::OnBtnSendHvApplyPacks() 
{
	//команда "послать количество пачек"
  UpdateData( TRUE);
  CMcCommandItem *item;
  item = new CMcCommandItem( MC_COMMAND_SET,
                                  HV_APPLY_PACKS,
                                  ( m_nHvApplyPacksOut & 0xFF),
                                  0);
  m_queCommandQueue.push( item);

  item = new CMcCommandItem( MC_COMMAND_REQ,
                                  HV_APPLY_PACKS,
                                  0,
                                  0);
  m_queCommandQueue.push( item);
}
