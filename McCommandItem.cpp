// McCommandItem.cpp: implementation of the CMcCommandItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "t7_slg_pc_editor.h"
#include "McCommandItem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMcCommandItem::CMcCommandItem( short cmd, short p1, short p2, short p3)
{
  m_nCommand = cmd;
  m_nParam1 = p1;
  m_nParam2 = p2;
  m_nParam3 = p3;
}

CMcCommandItem::~CMcCommandItem()
{

}
