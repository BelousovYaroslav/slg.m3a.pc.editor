// t7_slg_pc_editor.h : main header file for the T7_SLG_PC_EDITOR application
//

#if !defined(AFX_T7_SLG_PC_EDITOR_H__5E3E71EB_A0C4_45F4_9ADC_F8EA94004BDA__INCLUDED_)
#define AFX_T7_SLG_PC_EDITOR_H__5E3E71EB_A0C4_45F4_9ADC_F8EA94004BDA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "Logger.h"
#include "Settings.h"

/////////////////////////////////////////////////////////////////////////////
// CT7_slg_pc_editorApp:
// See t7_slg_pc_editor.cpp for the implementation of this class
//

class CT7_slg_pc_editorApp : public CWinApp
{
public:
	CT7_slg_pc_editorApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CT7_slg_pc_editorApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CT7_slg_pc_editorApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	int  GetComPort( void) { return m_nComPort;}
	void SetComPort( int newVal) { m_nComPort = newVal;}
	int  GetComBaudrate( void) { return m_nComBaudrate;}
	void SetComBaudrate( int newVal) { m_nComBaudrate = newVal;}
  void SetLastIncomingData( void);
  COleDateTime GetLastIncomingData( void) { return m_dtmLastIncomingDataTime;}
  CSettings *GetSettings( void) { return &m_pSettings;}
  CLogger * GetLogger() { return &m_pLogger;}
private:
	int m_nComPort;
	int m_nComBaudrate;
  COleDateTime m_dtmLastIncomingDataTime;
  CLogger m_pLogger;

public:
  char m_bLockBit;
	int m_nEmergencyCode;
  double m_dblScaleCoeff;

  int m_nMajorVersion, m_nMiddleVersion, m_nMinorVersion; //верси€


  double m_dblAmpl;     //заданна€ амплитуда колебаний
  int m_nHangerTact;    //код такта подставки
  double m_dblMcoeff;   //коэффициент ошумлени€
  int m_nStartMode;     //начальна€ мода
  double m_dblDecCoeff; //коэффициент вычета

  double m_dblAmplAnglmin;    //контрольный сигнал раскачки с ƒ”—а
  double m_dblControlI1min;   //контрольный ток поджига I1
  double m_dblControlI2min;   //контрольный ток поджига I2
  int m_nHVApplyMax;          //количество импульсов поджига в пачке
  int m_nHVApplyDuration;     //длительность импульсов поджига (мсек)
  int m_nHVApplyPacks;        //количество пачек импульсов поджига
  

  int m_nSignCoeff;           //знаковый коэффициент
  short m_shDeviceSerialNumber;   //серийный номер прибора
  int m_nDateY;                   //дата.год
  int m_nDateM;                   //дата.мес€ц
  int m_nDateD;                   //дата.день

  CString m_strOrganization;      //организаци€
  

  CSettings m_pSettings;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_T7_SLG_PC_EDITOR_H__5E3E71EB_A0C4_45F4_9ADC_F8EA94004BDA__INCLUDED_)
