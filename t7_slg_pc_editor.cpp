// t7_slg_pc_editor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "t7_slg_pc_editor.h"
#include "t7_slg_pc_editorDlg.h"
#include "DlgSettings.h"
#include "AnalogueParamsConstList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// The one and only CT7_slg_pc_editorApp object

CT7_slg_pc_editorApp theApp;




//КОЛЬЦЕВОЙ БУФЕР БАЙТ
#define CYCLE_BUFFER_LEN 32768

char gl_bCircleBuffer[ CYCLE_BUFFER_LEN];
int gl_nCircleBufferGet;
int gl_nCircleBufferPut;
bool gl_PutByteInCircleBuffer( BYTE bt);

int gl_nMarkerFails;
int gl_nCheckSummFails;
int gl_nReceivedPacks;

int gl_GetCircleBufferDistance( void)
{
	return( ( CYCLE_BUFFER_LEN + gl_nCircleBufferPut - gl_nCircleBufferGet) % CYCLE_BUFFER_LEN);
}

bool PutByteInCircleBuffer(BYTE bt)
{
	if( gl_nCircleBufferPut == gl_nCircleBufferGet - 1) return false;
	gl_bCircleBuffer[ gl_nCircleBufferPut] = bt;
	gl_nCircleBufferPut = ( ++gl_nCircleBufferPut) % CYCLE_BUFFER_LEN;
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// ПОТОК РАСПРЕДЕЛЕНИЯ ДАННЫХ

BOOL m_bStopBigThreadFlag;
HANDLE gl_hThread;
DWORD WINAPI BigThread(LPVOID lparam)
{
	bool bFirstPoint = true;
	double Kcoeff = 1.0;
	CString strError;
	
	BOOL bFirst100msecPointSkipped = false;

	m_bStopBigThreadFlag = false;


	while( !m_bStopBigThreadFlag) {
		int distance = ( CYCLE_BUFFER_LEN + gl_nCircleBufferPut - gl_nCircleBufferGet) % CYCLE_BUFFER_LEN;

    if( distance > 50) {
      theApp.GetLogger()->LogTrace( "BigThread. Positions: P: %d   G: %d   D: %d  MF: %d", gl_nCircleBufferPut, gl_nCircleBufferGet, distance, gl_nMarkerFails);

			BYTE byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11, byte12;
      BYTE btCheckSumm = 0;
			short param, nSaTime;
      unsigned short nCur1;
			double dAngle_inc, dCur1, dSaTime;
			
			int nMarkerCounter = 0;
      BOOL bMarkerFailOnce = true;
			do {

				//прокручиваем буфер выискивая последовательные 2 байта маркера (0x55, 0xAA)
				if( ( ( CYCLE_BUFFER_LEN + gl_nCircleBufferPut - gl_nCircleBufferGet) % CYCLE_BUFFER_LEN) > 20) {
					byte1 = gl_bCircleBuffer[ gl_nCircleBufferGet];
					gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;

          theApp.GetLogger()->LogTrace( "BigThread. Marker search. nMarkerCounter = %d. BYTE=0x%02x", nMarkerCounter, byte1);

          switch( nMarkerCounter) {
            case 0:
              if( byte1 == 0x11)
                nMarkerCounter=1;
              else if( byte1 == 0x55)
                nMarkerCounter=2;
              else {
                if( bMarkerFailOnce) {
                  gl_nMarkerFails++;
                  theApp.GetLogger()->LogTrace( "MF=%d", gl_nMarkerFails);
                  bMarkerFailOnce = false;
                }
              }
            break;

            case 1:
              //маркер передачи dn,du
              if( byte1 == 0x88)
                nMarkerCounter=3;
              else {
                nMarkerCounter = 0;
                if( bMarkerFailOnce) {
                  gl_nMarkerFails++;
                  theApp.GetLogger()->LogTrace( "MF=%d", gl_nMarkerFails);
                  bMarkerFailOnce = false;
                }
              }
            break;

            case 2:
              //маркер передачи dw
              if( byte1 == 0xAA)
                nMarkerCounter=3;
              else {
                nMarkerCounter = 0;
                if( bMarkerFailOnce) {
                  gl_nMarkerFails++;
                  theApp.GetLogger()->LogTrace( "MF=%d", gl_nMarkerFails);
                  bMarkerFailOnce = false;
                }
              }
            break;
          }
				} 
			} while( nMarkerCounter != 3);

      gl_nReceivedPacks = (++gl_nReceivedPacks);// % 4096;
      theApp.GetLogger()->LogTrace( "BigThread. Marker found. Package %03d", gl_nReceivedPacks);

      //ПРИРАЩЕНИЕ УГЛА: 4 байта
			byte1 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			byte2 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			byte3 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			byte4 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			

			//НОМЕР ЧЕРЕДУЮЩЕГОСЯ (ТЕХНОЛОГИЧЕСКОГО, АНАЛОГОВОГО) ПАРАМЕТРА. 1 байт
			byte5 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			param = byte5;
      //theApp.GetLogger()->LogDebug( "BigThread. Analogue param descriptor: 0x%02x", byte5);
	
			//ЗНАЧЕНИЕ ТЕХНОЛОГИЧЕСКОГО (АНАЛОГОВОГО) ПАРАМЕТРА. 2 Байта
			byte6 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			byte7 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			nCur1 = ( byte7 << 8) + byte6;
			dCur1 = ( double) (unsigned short) nCur1;
      //theApp.GetLogger()->LogDebug( "BigThread. Analogue param value: 0x%02x 0x%02x", byte6, byte7);
			
			//SA TIMING.
      //ИНТЕРВАЛ ВРЕМЕНИ МЕЖДУ ТЕКУЩИМ И ПРЕДЫДУЩИМ МОМЕНТАМИ ФИКСАЦИИ ПАРАМЕТРОВ. 2 БАЙТА
			byte8 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			byte9 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
			nSaTime = ( byte9 << 8) + byte8;
			dSaTime = nSaTime;
      //theApp.GetLogger()->LogDebug( "BigThread. Tact time: 0x%02x 0x%02x", byte8, byte9);

      //ПОРЯДКОВЫЙ НОМЕР СООБЩЕНИЯ. 1 БАЙТ
      byte10 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
      //theApp.GetLogger()->LogDebug( "BigThread. Package number: 0x%02x", byte10);

			//EMERGENCY CODE
      //КОД ОШИБКИ. 1 БАЙТ
			byte11 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;

      
			
      //CHEKSUMM
      //КОНТРОЛЬНАЯ СУММА, CS. 1 байт
			byte12 = gl_bCircleBuffer[ gl_nCircleBufferGet];
			gl_nCircleBufferGet = ( gl_nCircleBufferGet + 1) % CYCLE_BUFFER_LEN;
      //theApp.GetLogger()->LogDebug( "BigThread. Checksumm: 0x%02x", byte12);
			
      btCheckSumm = byte1;
      btCheckSumm += byte2;
      btCheckSumm += byte3;
      btCheckSumm += byte4;
      btCheckSumm += byte5;
      btCheckSumm += byte6;
      btCheckSumm += byte7;
      btCheckSumm += byte8;
      btCheckSumm += byte9;
      btCheckSumm += byte10;
      btCheckSumm += byte11;
      
      theApp.GetLogger()->LogTrace( "BigThread. Package bytes: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
        byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11, byte12);

      if( btCheckSumm != byte12) {
        //TODO несовпадение контрольной суммы
        CString strMsg;
        strMsg.Format( _T("\tBytes: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\tCS(mc): %02x\tCS(pc): %02x"),
                byte1, byte2, byte3, byte4, byte5, byte6,
                byte7, byte8, byte9, byte10, byte11, byte12, btCheckSumm);
        theApp.GetLogger()->LogDebug( _T("Несовпадение контрольной суммы! FAIL!") + strMsg);
        gl_nCheckSummFails++;
        continue;
      }


      //Временная отметка последней пришедей пачки
      theApp.SetLastIncomingData();

      //ОБРАБОТКА БАЙТА ОШИБКИ
      char bVeracity  = ( byte11 & 0x80) >> 7;
      char bLockBit   = ( byte11 & 0x40) >> 6;
      char bSyncAsync = ( byte11 & 0x20) >> 5;
      char bdWdNdU    = ( byte11 & 0x10) >> 4;

      /*
      char bVeracity  = ( byte11 & 0x80) >> 3;
      char bLockBit   = ( byte11 & 0x40) >> 2;
      char bSyncAsync = ( byte11 & 0x20) >> 1;
      char bdWdNdU    = ( byte11 & 0x10) >> 0;
      */

      char bErrorCode = byte11 & 0x0F;

      //локбит
      theApp.m_bLockBit = bLockBit;

      //байт кода ошибки
			if( theApp.m_nEmergencyCode == 0)
				theApp.m_nEmergencyCode = bErrorCode;
      //theApp.GetLogger()->LogDebug( "BigThread. Emergency code: 0x%02x", byte11);


      //ПРИРАЩЕНИЕ УГЛА
      if( bdWdNdU == 0) {
        //к нам пришло приращение угла

        //theApp.GetLogger()->LogDebug( "BigThread. Angle increment: 0x%02x 0x%02x 0x%02x 0x%02x", byte1, byte2, byte3, byte4);
			  //float f_dN;
        int n_dN;
			  char *ptr;
        //ptr = ( char *) &f_dN;
        ptr = ( char *) &n_dN;
			  ptr[0] = byte1;
			  ptr[1] = byte2;
			  ptr[2] = byte3;
			  ptr[3] = byte4;
        dAngle_inc = ( ( double) n_dN / 2147483647. * 99310.);
      }
      else {
        //к нам пришли dNdU

      }
		

      //анализ аналогового параметра
      switch( byte5) {
        // ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
        //page 1
        //код амплитуды
        case AMPLITUDE:
          theApp.m_dblAmpl = ( double) nCur1 * theApp.m_dblScaleCoeff;
          theApp.GetLogger()->LogTrace( "BigThread::Got AMPLITUDE: %d", nCur1);
        break;

        //такт подставки
				case TACT_CODE: theApp.m_nHangerTact = nCur1; theApp.GetLogger()->LogTrace( "BigThread::Got TACT_CODE: %d", nCur1); break;

        //коэффициент M
				case M_COEFF: theApp.m_dblMcoeff = ( double) nCur1 / 250.; theApp.GetLogger()->LogTrace( "BigThread::Got M_COEFF: %d", nCur1); break;

        //начальная мода
				case STARTMODE: theApp.m_nStartMode = nCur1; theApp.GetLogger()->LogTrace( "BigThread::Got STARTMODE: %d", nCur1);  break;

        //коэффициент вычета
        case DECCOEFF:
          theApp.m_dblDecCoeff = ( double) nCur1 / 65535.; /*Beep( 100, 100);*/ theApp.GetLogger()->LogTrace( "BigThread::Got DECCOEFF: %d", nCur1);
        break;

        
        // ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
        //page 2
        //контрольный AmplAng
				case CONTROL_AA: theApp.m_dblAmplAnglmin = ( double) nCur1 / 65535. * 3.; theApp.GetLogger()->LogTrace( "BigThread::Got CONTROL_AA: %d", nCur1);  break;

        //контрольный ток I1
        case CONTROL_I1: theApp.m_dblControlI1min = ( double) nCur1 / 65535. * 0.75; theApp.GetLogger()->LogTrace( "BigThread::Got CONTROL_I1: %d", nCur1);  break;

        //контрольный ток I2
			  case CONTROL_I2: theApp.m_dblControlI2min = ( double) nCur1 / 65535. * 0.75; theApp.GetLogger()->LogTrace( "BigThread::Got CONTROL_I2: %d", nCur1);  break;

        //Количество импульсов поджига
        case HV_APPLY_COUNT_SET: theApp.m_nHVApplyMax = nCur1; theApp.GetLogger()->LogTrace( "BigThread::Got HV_APPLY_COUNT_SET: %d", nCur1);  break;

        //Длительность импульсов поджига
			  case HV_APPLY_DURAT_SET:
          theApp.m_nHVApplyDuration = nCur1; theApp.GetLogger()->LogTrace( "BigThread::Got HV_APPLY_DURAT_SET: %d", nCur1);  break;

        //Количество пачек импульсов поджига
			  case HV_APPLY_PACKS:
          theApp.m_nHVApplyPacks = nCur1; theApp.GetLogger()->LogTrace( "BigThread::Got HV_APPLY_PACKS: %d", nCur1);  break;

        // ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
        //page 3
        //Знаковый коэффициент
        case SIGNCOEFF: theApp.m_nSignCoeff = nCur1 - 1; theApp.GetLogger()->LogTrace( "BigThread::Got SIGNCOEFF: %d", nCur1); break;

        case DEVNUM: //Device_Num
          theApp.GetLogger()->LogTrace( "BigThread::Got DEVNUM: %d", nCur1); 
          theApp.m_shDeviceSerialNumber = nCur1;
        break;

        case DATE_Y: //Date.Year
          theApp.GetLogger()->LogTrace( "BigThread::Got DATE_Y: %d", nCur1); 
          theApp.m_nDateY = nCur1;
          //nD = theApp.m_dtmDate.GetDay(), nM = theApp.m_dtmDate.GetMonth(), nY = theApp.m_dtmDate.GetYear();
          //theApp.m_dtmDate.SetDate( nY, nM, nCur1);
        break;    

        case DATE_M: //Date.Month
          theApp.GetLogger()->LogTrace( "BigThread::Got DATE_M: %d", nCur1); 
          theApp.m_nDateM = nCur1;
          //nD = theApp.m_dtmDate.GetDay(), nM = theApp.m_dtmDate.GetMonth(), nY = theApp.m_dtmDate.GetYear();
          //theApp.m_dtmDate.SetDate( nY, nCur1, nD);
        break;    

        case DATE_D: //Date.Day
          theApp.GetLogger()->LogTrace( "BigThread::Got DATE_D: %d", nCur1); 
          theApp.m_nDateD = nCur1;
          //nD = theApp.m_dtmDate.GetDay(), nM = theApp.m_dtmDate.GetMonth(), nY = theApp.m_dtmDate.GetYear();
          //theApp.m_dtmDate.SetDate( nCur1, nM, nD);
        break;

        case ORG_B1: //Organization.Byte1
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B1: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 0, ( BYTE) nCur1);
        break;

        case ORG_B2: //Organization.Byte2
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B2: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 1, ( BYTE) nCur1);
        break;

        case ORG_B3: //Organization.Byte3
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B3: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 2, ( BYTE) nCur1);
        break;

        case ORG_B4: //Organization.Byte4
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B4: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 3, ( BYTE) nCur1);
        break;    
        
        case ORG_B5: //Organization.Byte5
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B5: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 4, ( BYTE) nCur1);
        break;    
        
        case ORG_B6: //Organization.Byte6
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B6: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 5, ( BYTE) nCur1);
        break;    
        
        case ORG_B7: //Organization.Byte7
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B7: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 6, ( BYTE) nCur1);
        break;    
        
        case ORG_B8: //Organization.Byte8
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B8: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 7, ( BYTE) nCur1);
        break;    
        
        case ORG_B9: //Organization.Byte9
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B9: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 8, ( BYTE) nCur1);
        break;    
        
        case ORG_B10: //Organization.Byte10
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B10: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 9, ( BYTE) nCur1);
        break;    
        
        case ORG_B11: //Organization.Byte11
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B11: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 10, ( BYTE) nCur1);
        break;    
        
        case ORG_B12: //Organization.Byte12
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B12: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 11, ( BYTE) nCur1);
        break;    
        
        case ORG_B13: //Organization.Byte13
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B13: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 12, ( BYTE) nCur1);
        break;    
        
        case ORG_B14: //Organization.Byte14
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B14: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 13, ( BYTE) nCur1);
        break;    
        
        case ORG_B15: //Organization.Byte15
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B15: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 14, ( BYTE) nCur1);
        break;    

        case ORG_B16: //Organization.Byte16    БЕЗ завершающего 0 на конце!!!!!
          theApp.GetLogger()->LogDebug( "BigThread::Got ORG_B16: %d", nCur1); 
          theApp.m_strOrganization.SetAt( 15, ( BYTE) nCur1);
          ( ( CT7_slg_pc_editorDlg *) theApp.m_pMainWnd)->m_strOrganizationIn = theApp.m_strOrganization;
          ( ( CT7_slg_pc_editorDlg *) theApp.m_pMainWnd)->GetDlgItem( IDC_EDT_ORGANIZATION_IN)->SetWindowText( theApp.m_strOrganization);
        break;    

        // ***** ***** ***** ***** ***** ***** ***** ***** ***** *****
        //Rest
        //версия ПО
				case VERSION:
          theApp.GetLogger()->LogDebug( "BigThread::Got VERSION"); 
          theApp.m_nMajorVersion   = ( byte6 / 16);//major
					theApp.m_nMiddleVersion  = byte6 % 16;		//middle
					theApp.m_nMinorVersion   = byte7 / 16;		//minor					
				break; 
			}

				

		} //если растояние кольцевого буфера > 50		
		else
			Sleep( 1);
	} //while жизни потока

	return 1;
}




/////////////////////////////////////////////////////////////////////////////
// CT7_slg_pc_editorApp

BEGIN_MESSAGE_MAP(CT7_slg_pc_editorApp, CWinApp)
	//{{AFX_MSG_MAP(CT7_slg_pc_editorApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CT7_slg_pc_editorApp construction

CT7_slg_pc_editorApp::CT7_slg_pc_editorApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// CT7_slg_pc_editorApp initialization

BOOL CT7_slg_pc_editorApp::InitInstance()
{

  char bt=0;
  char gl_b_PerimeterReset = 0;
  char gl_chLockBit = 1;
  char gl_b_SyncMode = 0;
  char gl_chAngleOutput = 0;
  char gl_c_EmergencyCode = 0;

  bt = gl_b_PerimeterReset ? 0x80 : 0x00;

  //7 bit - 0x40 - lock bit
  bt += gl_chLockBit ? 0x40 : 0x00;

  //6 bit - 0x20 - Sync/Async regime
  bt += gl_b_SyncMode ? 0x20 : 0x00;

  //5 bit - 0x10 - W / dNdU regime
  bt += gl_chAngleOutput ? 0x10 : 0x00;

  //Error code (lower 4 byte)
  bt += gl_c_EmergencyCode;


	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	//ЧТЕНИЕ НАСТРОЕК
  theApp.SetRegistryKey(_T("Alcor Laboratories"));
	theApp.LoadStdProfileSettings();
  m_pSettings.LoadSettings();
	/*m_nComPort = GetProfileInt( _T("SETTINGS"), _T("COM_PORT"), 0);
	m_nComBaudrate = GetProfileInt( _T("SETTINGS"), _T("COM_BAUDRATE"), 0);
  int nScaleCoeff = GetProfileInt( _T("SETTINGS"), _T("SCALECOEFF"), 2900);
  m_dblScaleCoeff = nScaleCoeff / 1000.;*/
  
  m_nComPort = m_pSettings.GetComPort();
  m_nComBaudrate = m_pSettings.GetComBaudrate();
  m_dblScaleCoeff = m_pSettings.GetScaleCoeff();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

  m_dtmLastIncomingDataTime = COleDateTime::GetCurrentTime();

  m_pLogger.Init();
  GetLogger()->LogInfo( "CT7_slg_pc_editorApp::InitInstance(): in");

  m_dblAmpl = 0.0;
  m_nHangerTact = 0;
  m_dblMcoeff = 0.;
  m_nStartMode = 0;
  m_dblControlI1min = 0.;
  m_dblControlI2min = 0.;
  m_nSignCoeff = 0;
  m_dblAmplAnglmin = 0;
  m_nMajorVersion = 0; m_nMiddleVersion = 0; m_nMinorVersion = 0;
  m_dblDecCoeff = 0.;
  m_shDeviceSerialNumber = 1;
  m_nDateY = 2015;
  m_nDateM = 9;
  m_nDateD = 30;

  m_strOrganization = "                 ";
  int nResponse;
  try {
    GetLogger()->LogInfo( "CT7_slg_pc_editorApp::InitInstance(): before dlg construction");
	  CT7_slg_pc_editorDlg dlg;
    //CDlgSettings dlg;

	  m_pMainWnd = &dlg;

    GetLogger()->LogInfo( "CT7_slg_pc_editorApp::InitInstance(): before dlg doModal");
	  nResponse = dlg.DoModal();
  }
  catch( CException *e) {
    e->Delete();
    DWORD k = GetLastError();
    CString strMsg, strMsg2;
    e->GetErrorMessage( strMsg2.GetBuffer( 255), 255);
    strMsg2.ReleaseBuffer();
    strMsg.Format( "GetLastError(): %ld\nMessage:%s", k, strMsg2);
    AfxMessageBox( strMsg);
  }
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if( nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.

  GetLogger()->LogInfo( "CT7_slg_pc_editorApp::InitInstance(): out");
	return FALSE;
}

int CT7_slg_pc_editorApp::ExitInstance() 
{
  m_pSettings.SetComPort( m_nComPort);
  m_pSettings.SetComBaudrate( m_nComBaudrate);
  m_pSettings.SetScaleCoeff( m_dblScaleCoeff);

  m_pSettings.SaveSettings();
	return CWinApp::ExitInstance();
}

void CT7_slg_pc_editorApp::SetLastIncomingData()
{
  m_dtmLastIncomingDataTime = COleDateTime::GetCurrentTime();
}
