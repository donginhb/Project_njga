// MsgOp.h: interface for the MsgOp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MSGOP_H__7DD7CFFA_7B24_49E4_9B90_344DB459F30F__INCLUDED_)
#define AFX_MSGOP_H__7DD7CFFA_7B24_49E4_9B90_344DB459F30F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef WIN32
#include "MediaDataOp.h"
#else
#include "MgwPublicData.h"
#endif
#include <iostream>
#include <list>
using namespace std;
#include "xml/tinyxml/tinyxml.h"

typedef struct _sipmsgnode{
	string strNodeName;    
	string strNodeText;
}SIPMSGNODE;

//消息解析器
class MsgOp  
{
public:
	MsgOp();
	virtual ~MsgOp();
	int    InputMsg(char * pMsg);
    string GetMsgType();
	string GetMsgText(string strNodeName); //获取指定节点text
	
private:
    
    TiXmlDocument *myDocument;
	TiXmlElement  *RootElement;
	TiXmlElement  *currNode;
};

#endif // !defined(AFX_MSGOP_H__7DD7CFFA_7B24_49E4_9B90_344DB459F30F__INCLUDED_)
