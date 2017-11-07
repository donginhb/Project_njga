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
	//����һ��XML���ĵ�����
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
	//��ø�Ԫ�ء�
	RootElement = myDocument->RootElement();
	if (!RootElement)
	{// ��������
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
		mgwlog("MsgOp::GetMsgType()�����쳣\n");
		return "";
	}
	return "";
}

string MsgOp::GetMsgText(string strNodeName)
{
	RootElement = myDocument->RootElement();
	if (!RootElement)
	{// ��������
		return "";
	}

	try
	{
		if(RootElement)
		{
			currNode = RootElement->FirstChildElement(); //���ĵ�һ���ӽڵ� 
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
		mgwlog("MsgOp::GetMsgText�����쳣\n");
		return "";
	}
	return "";
}
