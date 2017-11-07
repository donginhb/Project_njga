//#include "stdafx.h"
#include "Packet.h"


#define MAX_PATH 260

int SECellByName(DOMElement* pElement, void* pRef)
{
    string tmp1 = pElement->get_tag();
    string tmp2 = ((REF_INPUT*)pRef)->sTag;

    if (pElement->get_tag() == ((REF_INPUT*)pRef)->sTag)
    {
        ((REF_INPUT*)pRef)->sFoundElement = pElement;
        return 1;
    }

    return 0;
}

int SECellByAttr(DOMElement* pElement, void* pRef)
{
    if (pElement->get_tag() == ((REF_INPUT*)pRef)->sTag)
    {
        string m_tmp_att;
        pElement->GetAttribute(((REF_INPUT*)pRef)->sAttr.c_str(), m_tmp_att);

        if (m_tmp_att == ((REF_INPUT*)pRef)->sValue.c_str())
        {
            ((REF_INPUT*)pRef)->sFoundElement = pElement;
            return 1;
        }
    }

    return 0;
}

/**********************************
   class CPacket Implement
***********************************/
CPacket::CPacket()
{
    m_CurrentElement = NULL;
    m_FoundElement = NULL;
    m_bInitFlag = false;
}

CPacket::~CPacket()
{

}


DOMElement* CPacket::SearchElement(char* szNodeName)
{
    string::size_type begin = 0;

    DOMElement* pCurrentElement = NULL;

    //判断是否绝对路径
    if (szNodeName[0] == '/' || szNodeName[0] == '\\')
    {
        pCurrentElement = m_DomTree.getDocumentElement();
        begin = 1;
    }
    else
    {
        if (m_CurrentElement != NULL)
        {
            pCurrentElement = m_CurrentElement;
        }
        else
        {
            pCurrentElement = m_DomTree.getDocumentElement();
            m_CurrentElement = pCurrentElement;
        }
    }

    if (pCurrentElement == NULL)
    {
        return NULL;
    }

    ////////////////////////
    // 当前tag名
    m_FoundElement = NULL;

    vector<string> m_PathElement;
    vector<string>::iterator it;

    if (begin == 1)
    {
        split_bychar(m_PathElement, szNodeName + 1, '/');
    }
    else
    {
        split_bychar(m_PathElement, szNodeName, '/');
    }

    it = m_PathElement.begin();
    REF_INPUT in_sRef;

    for (; it != m_PathElement.end(); it++)
    {
        in_sRef.sFoundElement = NULL;

        m_FoundElement = NULL;
        in_sRef.sTag = (*it);
        LPENUM_CALLBACK_FUNC pFun = &(SECellByName);
        m_DomTree.EnumerateElements(pFun, pCurrentElement, (void*)&in_sRef);
        m_FoundElement = (DOMElement*)in_sRef.sFoundElement;

        if (!m_FoundElement)
        {
            return NULL;
        }

        pCurrentElement = m_FoundElement;
    }

    return m_FoundElement;
}

DOMElement* CPacket::SearchElement(char* szNodeName, char* szProper, char* szId)
{
    string::size_type begin = 0;

    DOMElement* pCurrentElement = NULL;

    //判断是否绝对路径
    if (szNodeName[0] == '/' || szNodeName[0] == '\\')
    {
        pCurrentElement = m_DomTree.getDocumentElement();
        begin = 1;
    }
    else
    {
        if (m_CurrentElement != NULL)
        {
            pCurrentElement = m_CurrentElement;
        }
        else
        {
            pCurrentElement = m_DomTree.getDocumentElement();
            m_CurrentElement = pCurrentElement;
        }
    }

    if (pCurrentElement == NULL)
    {
        //return false;
        return NULL;
    }

    ////////////////////////
    // 当前tag名
    m_FoundElement = NULL;

    vector<string> m_PathElement;
    vector<string>::iterator it;

    if (begin == 1)
    {
        split_bychar(m_PathElement, szNodeName + 1, '/');
    }
    else
    {
        split_bychar(m_PathElement, szNodeName, '/');
    }

    m_CrrentAttr = szProper;
    m_CrrentValue = szId;

    it = m_PathElement.begin();

    for (; it != m_PathElement.end(); it++)
    {
        //m_CurrentTag = (*it);
        m_FoundElement = NULL;

        REF_INPUT in_sRef;
        in_sRef.sTag = (*it);

        if (it == m_PathElement.end() - 1)
        {
            m_DomTree.EnumerateElements(SECellByAttr, pCurrentElement, (void*)&in_sRef);
        }
        else
        {
            m_DomTree.EnumerateElements(SECellByName, pCurrentElement, (void*)&in_sRef);
        }

        m_FoundElement = (DOMElement*)in_sRef.sFoundElement;
        pCurrentElement = m_FoundElement;
    }

    return m_FoundElement;
}

DOMElement* CPacket::CreateElement(char szNodeName[])
{
    string::size_type begin = 0;

    if (strcmp(szNodeName, ".\\") == 0 || strcmp(szNodeName, "./") == 0)
    {
        return m_CurrentElement;
    }

    DOMElement* pCurrentElement = NULL;
    DOMElement* pParentElement = NULL;

    //判断是否绝对路径
    if (szNodeName[0] == '/' || szNodeName[0] == '\\')
    {
        pCurrentElement = m_DomTree.getDocumentElement();
        begin = 1;
    }
    else
    {
        if (m_CurrentElement != NULL)
        {
            pCurrentElement = m_CurrentElement;
        }
        else
        {
            pCurrentElement = m_DomTree.getDocumentElement();
            m_CurrentElement = pCurrentElement;
        }
    }

    pParentElement = pCurrentElement;

    string pTmpnode = szNodeName;

    string::size_type m_PosBgn = pTmpnode.find_last_of("/");

    if (m_PosBgn == string::npos)
    {
        pCurrentElement = pCurrentElement->AddChild(szNodeName, NULL);
    }
    else if (begin == 1 && m_PosBgn == 0)
    {
        pCurrentElement = pCurrentElement->AddChild(szNodeName + 1, NULL);
    }
    else
    {

        m_PosBgn = 0;
        string::size_type m_PosCur = m_PosBgn;

        m_PosBgn = pTmpnode.find("/", m_PosCur + 1);

        while (m_PosBgn != string::npos)
        {
            pCurrentElement = SearchElement((char*)pTmpnode.substr(0, m_PosBgn).c_str());

            if (!pCurrentElement)
            {
                pCurrentElement = pParentElement->AddChild(pTmpnode.substr(m_PosCur, m_PosBgn - m_PosCur).c_str(), NULL);
            }

            m_PosCur = m_PosBgn + 1;
            m_PosBgn = pTmpnode.find("/", m_PosCur);
            pParentElement = pCurrentElement;
        }

        m_PosBgn = pTmpnode.size();
        pCurrentElement = pParentElement->AddChild(pTmpnode.substr(m_PosCur, m_PosBgn - m_PosCur).c_str(), NULL);

    }

    //m_CurrentElement = pCurrentElement;
    return pCurrentElement;
}

bool CPacket::SetCurrentElement(char szNodeName[])
{
    DOMElement* pCurrElem = SearchElement(szNodeName);

    if (pCurrElem == NULL)
    {
        return false;
    }

    m_CurrentElement = pCurrElem;
    return true;
}

bool CPacket::GetElementValue(char* szNodeName, char* szOutBuf)
{
    DOMElement* elem = SearchElement(szNodeName);

    if (elem == NULL)
    {
        return false;
    }

    //XMLString::transcode(elem->getTextContent(), szOutBuf, TRANSCODESIZE);
    if (NULL != elem->getTextContent())
    {
        strcpy(szOutBuf, elem->getTextContent());
    }

    return true;
}

bool CPacket::GetElementValue(char* szNodeName, char* szOutBuf, char* szProper, char* szId)
{
    DOMElement* elem = SearchElement(szNodeName, szProper, szId);

    if (elem == NULL)
    {
        return false;
    }

    //XMLString::transcode(elem->getTextContent(), szOutBuf, TRANSCODESIZE);
    if (NULL != elem->getTextContent())
    {
        strcpy(szOutBuf, elem->getTextContent());
    }

    return true;
}

bool CPacket::GetElementAttr(DOMElement* pelem, char* szAttrName, string& szAttrValue)
{
    if (pelem == NULL)
    {
        return false;
    }

    pelem->GetAttribute(szAttrName, szAttrValue);

    return true;
}

bool CPacket::GetElementAttr(char* szNodeName, char* szAttrName, string& szAttrValue)
{
    DOMElement* elem = SearchElement(szNodeName);

    if (elem == NULL)
    {
        return false;
    }

    elem->GetAttribute(szAttrName, szAttrValue);

    return true;
}

bool CPacket::SetElementAttr(char* szNodeName, char* szAttrName, char* szAttrValue)
{
    DOMElement* elem = SearchElement(szNodeName);

    if (elem == NULL)
    {
        return false;
    }

    elem->SetAttribute(szAttrName, szAttrValue);
    return true;
}

bool CPacket::SetElementAttr(DOMElement* pelem, char* szAttrName, char* szAttrValue)
{
    if (pelem == NULL)
    {
        return false;
    }

    pelem->SetAttribute(szAttrName, szAttrValue);
    return true;
}


bool CPacket::SetElementValue(char* szNodeName, char* szinBuf)
{
    DOMElement* elem = SearchElement(szNodeName);

    if (elem == NULL)
    {
        //m_LastError = AD_ERROR_ERROR;
        return false;
    }

    elem->setTextContent(szinBuf);
    return true;
}

bool CPacket::SetElementValue(DOMElement* pelem, char* szinBuf)
{
    if (pelem == NULL)
    {
        return false;
    }

    pelem->setTextContent(szinBuf);
    return true;
}

bool CPacket::Initialize()
{
    try
    {
        DOMElement* pCurrentElement = m_DomTree.getDocumentElement();

        if (!pCurrentElement)
        {
            return false;
        }


        string rc = pCurrentElement->get_tag();

        if (rc.size() == 0)
        {
            pCurrentElement->set_tag("Xml");
        }

        m_CurrentElement = pCurrentElement;

        m_bInitFlag = true;
    }
    catch (...)
    {

        return false;
    }

    return true;
}

void CPacket::ResetCurrentNode()
{
    m_CurrentElement = m_DomTree.getDocumentElement();
}

int CPacket::split_bychar(vector<string>& vec, const string& str, const char seperate)
{
    string::size_type pos1, pos2;
    string word;
    pos1 = pos2 = 0;
    vec.clear();

    while ((pos2 = str.find_first_of(seperate, pos1)) != string::npos)
    {
        word = str.substr(pos1, pos2 - pos1);
        pos1 = pos2 + 1;

        if (!word.empty())
        {
            vec.push_back(word);
        }
    }

    word = str.substr(pos1);

    if (!word.empty())
    {
        vec.push_back(word);
    }

    return 0;
}


int CPacket::BuiltTree(const char* pBuf, int nBufSize)
{
    int rc = m_DomTree.LoadXML(pBuf, nBufSize);
    m_CurrentElement = m_DomTree.getDocumentElement();
    return rc;
}

string CPacket::GetXml(DOMElement* pElement, string strEncode)
{
    //int iRet = 0;

    m_DomTree.BuildXML(pElement, strEncode);

    //iRet = m_DomTree.ConvertXMLToUTF8();

    //if (0 == iRet)
    //{
    //return m_DomTree.get_utf_8_xml();
    //}
    //else
    //{
    return m_DomTree.get_xml();
    //}
}

int CPacket::SetRootTag(string pTag)
{
    DOMElement* m_Element = m_DomTree.getDocumentElement();

    if (m_Element)
    {
        m_Element->set_tag(pTag.c_str());
        return 1;
    }
    else
    {
        return 0;
    }
}

int CPacket::BuiltTree(MLPCSTR pXmlPath)
{
    int rc = m_DomTree.LoadXML(pXmlPath);
    m_CurrentElement = m_DomTree.getDocumentElement();
    return rc;
}

DOMElement* CPacket::SearchNextElement(bool pMoveNext)
{
    DOMElement* pCurrentElement = m_CurrentElement->GetNextElement();

    if (pMoveNext)
    {
        if (pCurrentElement)
        {
            m_CurrentElement = pCurrentElement;
        }
    }

    return pCurrentElement;
}

DOMElement* CPacket::SearchNextElement(char* szNodeName, char* szProper, char* szId)
{
    DOMElement* pCurrentElement = m_CurrentElement;

    if (!pCurrentElement)
    {
        return NULL;
    }

    string m_NodeName;
    string m_Proper;
    string m_Id;

    m_NodeName = szNodeName;

    if (szProper)
    {
        m_Proper = szProper;
    }

    if (szId)
    {
        m_Id = szId;
    }

    while ((pCurrentElement = pCurrentElement->GetNextElement()) != NULL)
    {
        string tmp = pCurrentElement->getTextContent();

        if (pCurrentElement->get_tag() == m_NodeName)
        {
            if (szProper)
            {
                string AtrrValue;
                pCurrentElement->GetAttribute(m_Proper.c_str(), AtrrValue);

                if (m_Id != AtrrValue)
                {
                    continue;
                }
            }

            break;
        }
    }

    return pCurrentElement;
}
