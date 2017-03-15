// t7_slg_pc_editorDlg.h : header file
//
//{{AFX_INCLUDES()
#include "mscomm.h"
#include "dtpicker.h"
//}}AFX_INCLUDES
#include "McCommandItem.h"
#include <queue>

#if !defined(AFX_T7_SLG_PC_EDITORDLG_H__E235708D_58CA_4E0D_94E7_8E15EE394261__INCLUDED_)
#define AFX_T7_SLG_PC_EDITORDLG_H__E235708D_58CA_4E0D_94E7_8E15EE394261__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define POLL_PARAMS_LEN 28
/////////////////////////////////////////////////////////////////////////////
// CT7_slg_pc_editorDlg dialog

class CT7_slg_pc_editorDlg : public CDialog
{
// Construction
public:
	int m_nPollCounter;
	void SendCommandToMc( BYTE b1, BYTE b2, BYTE b3, BYTE b4);
	void SetControlsState( BOOL bDeviceConnectionPresent, BOOL bCanConnect);
	CT7_slg_pc_editorDlg(CWnd* pParent = NULL);	// standard constructor
  ~CT7_slg_pc_editorDlg();	// standard destructor

// Dialog Data
	//{{AFX_DATA(CT7_slg_pc_editorDlg)
	enum { IDD = IDD_T7_SLG_PC_EDITOR_DIALOG };
	int		m_nComPort;
	int		m_nPortSpeed;
	CString	m_strConnectionStatus;
	CMSComm	m_ctlCOM;
	CString	m_strLblEmergency;
	CString	m_strMcVersion;
	int		m_nCalibCurrentTemperature;
	CString	m_strMarkerFails;
	CString	m_strCheckSummFails;
	CDTPicker	m_ctlDtpDevDateIn;
	CDTPicker	m_ctlDtpDevDateOut;
	double	m_dblScaleCoeff;
	CString	m_strRecievedPacks;
	int		m_nSignCoeffIn;
	int		m_nSignCoeffOut;
	double	m_dblAmplCodeOut;
	double	m_dblAmplCodeIn;
	int		m_nTactCodeIn;
	int		m_nTactCodeOut;
	double	m_dblMinI1In;
	double	m_dblMinI1Out;
	double	m_dblMinI2In;
	double	m_dblMinI2Out;
	double	m_dblMinAmplAngIn;
	double	m_dblMinAmplAngOut;
	double	m_dblMCoeffIn;
	double	m_dblMCoeffOut;
	double	m_dblDecCoeffIn;
	double	m_dblDecCoeffOut;
	int		m_nDeviceSerialNumIn;
	int		m_nDeviceSerialNumOut;
	int		m_nHvApplyIn;
	int		m_nHvApplyOut;
	int		m_nHvApplyDurIn;
	int		m_nHvApplyDurOut;
	CString	m_strOrganizationOut;
	CString	m_strOrganizationIn;
	double	m_dblStartModeOut;
	double	m_dblStartModeIn;
	int		m_nHvApplyPacksIn;
	int		m_nHvApplyPacksOut;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CT7_slg_pc_editorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CT7_slg_pc_editorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	afx_msg void OnBtnConnect();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBtnDisconnect();
	afx_msg void OnChangeEdtOrganisationOut();
	afx_msg void OnBtnApplyScaleCoeff();
	afx_msg void OnKillfocusEdtSignCoeffOut();
	afx_msg void OnBtnRequestSernum();
	afx_msg void OnBtnRequestDate();
	afx_msg void OnBtnRequestOrg();
	afx_msg void OnSelchangeCmbCom();
	afx_msg void OnSelchangeCmbPortSpeed();
	afx_msg void OnBtnSetreadtosentDevsernum();
	afx_msg void OnBtnSendSerial();
	afx_msg void OnBtnSendCalib();
	afx_msg void OnBtnRequestAmpl();
	afx_msg void OnBtnRequestTactCode();
	afx_msg void OnBtnRequestMCoeff();
	afx_msg void OnBtnRequestStartMode();
	afx_msg void OnBtnRequestControlI1();
	afx_msg void OnBtnRequestControlI2();
	afx_msg void OnBtnRequestControlAa();
	afx_msg void OnBtnRequestDecCoeff();
	afx_msg void OnBtnRequestSignCoeff();
	afx_msg void OnBtnRequestHvApplySet();
	afx_msg void OnBtnCpReadToSentAmpl();
	afx_msg void OnBtnSetreadtosentTact();
	afx_msg void OnBtnSetreadtosentMcoeff();
	afx_msg void OnBtnRequestCalib();
	afx_msg void OnBtnSendAmpl();
	afx_msg void OnBtnSendTactcode();
	afx_msg void OnBtnSendMCoeff();
	afx_msg void OnBtnSendStartMode();
	afx_msg void OnBtnSendDecCoeff();
	afx_msg void OnBtnSendI1Min();
	afx_msg void OnBtnSendI2Min();
	afx_msg void OnBtnSendAmplangMin();
	afx_msg void OnBtnSendHvApplySet();
	afx_msg void OnBtnSendSigncoeff();
	afx_msg void OnBtnSendDate();
	afx_msg void OnBtnSendOrganization();
	afx_msg void OnOnCommMscomm();
	afx_msg void OnBtnSaveToFlashP1();
	afx_msg void OnBtnSaveToFlashP2();
	afx_msg void OnBtnSaveToFlashP3();
	afx_msg void OnBtnSetreadtosentStartmode();
	afx_msg void OnBtnSetreadtosentDeccoeff();
	afx_msg void OnBtnSetreadtosentI1min();
	afx_msg void OnBtnSetreadtosentI2min();
	afx_msg void OnBtnSetreadtosentAmplangmin();
	afx_msg void OnBtnSetreadtosentDate();
	afx_msg void OnBtnRequestHvApplyDurSet();
	afx_msg void OnBtnSetreadtosentHvApplySet();
	afx_msg void OnBtnSetreadtosentHvApplyDurSet();
	afx_msg void OnBtnSendHvApplyDurSet();
	afx_msg void OnBtnLockDevice();
	afx_msg void OnBtnVersion();
	afx_msg void OnDeltaposSpinSigncoeff(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBtnSetreadtosentSign();
	afx_msg void OnBtnSetreadtosentOrg();
	afx_msg void OnBtnRequestHvApplyPack();
	afx_msg void OnBtnSetreadtosentHvApplyPacks();
	afx_msg void OnBtnSendHvApplyPacks();
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void QueueCommandToMc( char btCmd, char btParam1, char btParam2, char btParam3);
  std::queue <CMcCommandItem *> m_queCommandQueue;
  int m_nPollParams[ POLL_PARAMS_LEN];
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_T7_SLG_PC_EDITORDLG_H__E235708D_58CA_4E0D_94E7_8E15EE394261__INCLUDED_)
