// McCommandItem.h: interface for the CMcCommandItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MCCOMMANDITEM_H__4D95438F_ED04_4134_AF0B_8DDCDFE07391__INCLUDED_)
#define AFX_MCCOMMANDITEM_H__4D95438F_ED04_4134_AF0B_8DDCDFE07391__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMcCommandItem  
{
public:
	CMcCommandItem( short cmd, short p1, short p2, short p3);
	virtual ~CMcCommandItem();
  short m_nCommand;
  short m_nParam1;
  short m_nParam2;
  short m_nParam3;
};

#endif // !defined(AFX_MCCOMMANDITEM_H__4D95438F_ED04_4134_AF0B_8DDCDFE07391__INCLUDED_)
