// MsgOp.cpp: implementation of the MsgOp class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#include "EV9000Mgw.h"
#endif
#include "MsgOp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
extern CEV9000MgwApp theApp;
#endif
MsgOp::MsgOp()
{
	myDocument=NULL;
	RootElement =NULL;
	currNode =NULL;
}

MsgOp::~MsgOp()
{
    MEMORY_DELETE(myDocument);
}

int  MsgOp::InputMsg(char * pMsg)
{
	//创建一个XML的文档对象。
	myDocument = new TiXmlDocument();
	if (!myDocument)
	{	
		return -3;
	}
	myDocument->Parse(pMsg);
	return 0;
}
string MsgOp::GetMsgType()
{
	//获得根元素。
	RootElement = myDocument->RootElement();
	if (!RootElement)
	{// 解析错误
		return "";
	}

	try
	{
		currNode = RootElement;
		if(RootElement)
		{
			return RootElement->Value();
		}else{
			return "";
		}
	}
	catch (...)
	{
		mgwlog("MsgOp::GetMsgType()发生异常\n");
		return "";
	}
	return "";
}

string MsgOp::GetMsgText(string strNodeName)
{
	RootElement = myDocument->RootElement();
	if (!RootElement)
	{// 解析错误
		return "";
	}

	try
	{
		if(RootElement)
		{
			currNode = RootElement->FirstChildElement(); //根的第一个子节点 
			while(currNode)
			{
				if(0==strncmp(currNode->Value(),strNodeName.c_str(),strlen(strNodeName.c_str())))
				{
					return currNode->GetText();
				}
				currNode = currNode->NextSiblingElement();
			}
			return "";
		}else{
			return "";
		}
	}
	catch (...)
	{
		mgwlog("MsgOp::GetMsgText发生异常\n");
		return "";
	}
	return "";
}
