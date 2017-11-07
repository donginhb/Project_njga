//#include <stdafx.h>
#include "DOMDocument.h"
#include <assert.h>
#include <iconv.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define TEMP_BUF_LEN 1024

DOMDocument::DOMDocument() :
    m_pos(0),
    m_root(NULL)
{
    m_version = "1.0";
    m_encode = "UTF-8";
}

DOMDocument::~DOMDocument()
{

}

int DOMDocument::LoadXML(const char* buffer, unsigned long bufferSize)
{
    assert(buffer);

    if (!buffer)
    {
        return -1;
    }

    m_pos = 0;

    m_xml = buffer;

    (void)ParseXmlInfo();

    return ParseRoot();
}

int DOMDocument::LoadXML(MLPCSTR pathXML)
{
    int ret = 0;
    char* xmlBuffer = NULL;
    FILE* pf = fopen(pathXML, "rb");

    if (!pf)
    {
        //printf("ERROR: Can't open %s\n", pathXML);
        return -1;
    }

    unsigned long pos;

    if (fseek(pf, 0, SEEK_END))
    {
        fclose(pf);
        return -1;
    }

    if (fgetpos(pf, (fpos_t*)&pos))
    {
        fclose(pf);
        return -1;
    }

    if (pos <= 0)
    {
        fclose(pf);
        return -1;
    }

    xmlBuffer = new char[pos];
    assert(xmlBuffer);

    if (!xmlBuffer)
    {
        fclose(pf);
        delete[] xmlBuffer;
        return -1;
    }

    rewind(pf);
    fread(xmlBuffer, pos, 1, pf);
    fclose(pf);

    ret = LoadXML(xmlBuffer, pos);
    delete[] xmlBuffer;
    return ret;
}

int DOMDocument::ParseXmlInfo()
{
    //Try to find the start of the version tag.
    string m_ShortStr;

    //获取版本
    string::size_type version_start = 0;
    string::size_type version_finish = 0;

    m_pos = m_xml.find("version");

    if (m_pos == string::npos)
    {
        return -1;
    }

    version_start = m_xml.find("\"", m_pos) + 1;
    version_finish = m_xml.find("\"", version_start + 1);

    if (version_start != string::npos && version_finish != string::npos)
    {
        m_version = m_xml.substr(version_start, version_finish - version_start);
    }
    else
    {
        version_start = m_xml.find("\'", m_pos) + 1;
        version_finish = m_xml.find("\'", version_start + 1);

        if (version_start != string::npos && version_finish != string::npos)
        {
            m_version = m_xml.substr(version_start, version_finish - version_start);
        }
        else
        {
            m_version = "1.0";
        }
    }


    //获取编码
    version_start = 0;
    version_start = m_xml.find("encoding");

    if (version_start == string::npos)
    {
        return -1;
    }

    version_start = m_xml.find("\"", version_start) + 1;
    version_finish = m_xml.find("\"", version_start + 1);

    if (version_start != string::npos && version_finish != string::npos)
    {
        m_encode = m_xml.substr(version_start, version_finish - version_start);
    }
    else
    {
        version_start = m_xml.find("\'", m_pos) + 1;
        version_finish = m_xml.find("\'", version_start + 1);

        if (version_start != string::npos && version_finish != string::npos)
        {
            m_encode = m_xml.substr(version_start, version_finish - version_start);
        }
        else
        {
            m_encode = "UTF-8";
        }
    }

    return 0;
}

int DOMDocument::BuildXmlInfo(string& strEncode)
{
    string version_str = "<?xml ";
    version_str += "version=\"";
    version_str += m_version;

    version_str += "\" encoding=\"";

    if (!strEncode.empty())
    {
        version_str += strEncode;
    }
    else
    {
        version_str += m_encode;
    }

    version_str += "\" ?>\r\n";
    m_xml = version_str;

    return 0;
}

int DOMDocument::BuildXML(DOMElement* pElement, string& strEncode)
{
    //清空内容
    m_xml = "";
    m_pos = 0;

    int ret = 0;

    if ((ret = BuildXmlInfo(strEncode)) != 0)
    {
        return ret;
    }

    if (!pElement)
    {
        return m_root.BuildElement(m_xml);
    }
    else
    {
        return pElement->BuildElement(m_xml);
    }
}

int DOMDocument::ConvertXMLToUTF8()
{
    int iRet = 0;

    //清空内容
    m_utf8_xml = "";
    size_t inlen = 0;
    size_t outlen = 0;

    inlen = m_xml.length();
    outlen = (inlen / 2) * 3;

    //printf("\r\n ConvertXMLToUTF8:m_xml=%s \r\n", m_xml.c_str());
    //printf("\r\n ConvertXMLToUTF8:inlen=%d,outlen=%d \r\n", inlen, outlen);

    iRet = g2u(m_xml, inlen, m_utf8_xml, outlen);

    //printf("\r\n ConvertXMLToUTF8 g2u:iRet=%d \r\n", iRet);

    return iRet;
}

//代码转换:从一种编码转为另一种编码
int DOMDocument::CodeConvert(char* from_charset, char* to_charset, string inbuf, size_t inlen, string& outbuf, size_t outlen)
{
    int iRet = 0;
    iconv_t cd;
    char* pin1 = (char*)inbuf.c_str();
    char* pout1 = (char*)outbuf.c_str();

    char** pin = &pin1;
    char** pout = &pout1;

    //printf("\r\n CodeConvert pin1=%s \r\n", pin1);
    //printf("\r\n CodeConvert pout1=%s \r\n", pout1);
    //printf("\r\n CodeConvert from_charset=%s \r\n", from_charset);
    //printf("\r\n CodeConvert to_charset=%s \r\n", to_charset);

    cd = iconv_open(to_charset, from_charset);
    //printf("\r\n CodeConvert cd=%d \r\n", cd);

    if (cd == 0)
    {
        return -1;
    }

    //memset(outbuf, 0, outlen);

    iRet = iconv(cd, pin, &inlen, pout, &outlen);

    iconv_close(cd);

    return iRet;
}

//UNICODE码转为GB2312码
int DOMDocument::u2g(string inbuf, size_t inlen, string& outbuf, size_t outlen)
{
    return CodeConvert((char*)"UTF-8", (char*)"GB2312", inbuf, inlen, outbuf, outlen);
}

//GB2312码转为UNICODE码
int DOMDocument::g2u(string inbuf, size_t inlen, string& outbuf, size_t outlen)
{
    return CodeConvert((char*)"GB2312", (char*)"UTF-8", inbuf, inlen, outbuf, outlen);
}

int DOMDocument::findNextTag(string& tagName, string& beforeTag)
{
    tagName = "";
    beforeTag = "";

    string::size_type beforeTagStart = m_xml.find_first_not_of("\t\r\n ", m_pos);

    if (beforeTagStart == string::npos)
    {
        return -1;
    }

    string::size_type beforeTagFinish = m_xml.find("<", beforeTagStart);

    if (beforeTagFinish == string::npos)
    {
        return -1;
    }

    if (beforeTagFinish - beforeTagStart > 0)
    {
        beforeTag = m_xml.substr(beforeTagStart, beforeTagFinish - beforeTagStart);

        // trim left tempString
        TRIM_LEFT(beforeTag);
    }

    // Find the tag finish position.
    string::size_type tag_start = beforeTagFinish;
    string::size_type tag_finish = m_xml.find(">", tag_start);

    if (tag_finish == string::npos)
    {
        return -1;
    }

    tagName = m_xml.substr(tag_start + 1, tag_finish - tag_start - 1);
    m_pos = tag_finish + 1;

    return 0;
}

int DOMDocument::ParseElement(DOMElement* element)
{
    assert(element != NULL);

    if (element == NULL)
    {
        return -1;
    }

    while (m_pos < m_xml.size())
    {
        string tagName;
        string beforeTag;

        if (findNextTag(tagName, beforeTag) == -1)
        {
            //printf("Error: Without a finish tag for <%s>.\n", element->get_tag().c_str());
            return -1;
        }


        /*
        //Modiby litz,2006-08-23
        //为什么还要判断它是否为空?
        if (element->getTextContent().empty() && !beforeTag.empty())
            element->setTextContent(beforeTag.c_str());
            */
        if (!beforeTag.empty())
        {
            element->setTextContent(beforeTag.c_str());
        }


        // Is a end-tag?
        if (tagName[0] == '/')
        {
            // Remove the '/' character.
            string end_tagName;

            end_tagName = tagName;
            end_tagName.erase(0, 1);

            if (element->get_tag() != end_tagName)
            {
                //printf("ERROR: Can't find a end tag for <%s>.\n", element->get_tag().c_str());
                return -1;
            }

            return 0;
        }

        DOMElement* sub_element = new DOMElement(element);

        if (!sub_element)
        {
            return -1;
        }

        // Is it a empty tag?
        if (tagName[tagName.size() - 1] == '/')
        {
            tagName.erase(tagName.size() - 1, 1);

            string actualTagName;

            int m_AttrNum = ParseAttribute(tagName.c_str(), actualTagName, sub_element->_attribute);

            if (m_AttrNum == -1)
            {
                return -1;
            }

            sub_element->set_tag(actualTagName.c_str());
            continue ;
        }

        string actualTagName;
        int m_AttrNum = ParseAttribute(tagName.c_str(), actualTagName, sub_element->_attribute);

        if (m_AttrNum == -1)
        {
            return -1;
        }

        sub_element->set_tag(actualTagName.c_str());

        if (ParseElement(sub_element) != 0)
        {
            return -1;
        }
    }

    //printf("Error: without a finish tag for <%s>.\n", element->get_tag().c_str());
    return -1;
}

int DOMDocument::EnumerateElements(LPENUM_CALLBACK_FUNC pFunc, DOMElement* pElectment, void* pRef)
{
    assert(pFunc != NULL);

    if (!pFunc)
    {
        return -1;
    }

    return pElectment->Enumerate(pFunc, pRef);
}

int DOMDocument::ParseRoot()
{
    string::size_type tag_start = 0;

    string::size_type nPos = 0;

    while ((nPos = m_xml.find("?>", nPos)) != string::npos)
    {
        nPos += 2;
        tag_start = nPos;
    }

    // 查找根结点的前标签
    tag_start = m_xml.find("<", tag_start);

    // Find the next tag start position (it is the content part finish also).
    if (tag_start == string::npos)
    {
        //printf("ERROR: It is not XML Document.\n");
        return -1;
    }

    string::size_type tag_finish = m_xml.find('>', tag_start);

    if (tag_finish == string::npos || tag_finish - tag_start <= 1)
    {
        //printf("ERROR: There is no tag name int the XML root.\n");
        return -1;
    }

    string tagName = m_xml.substr(tag_start + 1, tag_finish - tag_start - 1);
    //如果根结点带属性，去掉
    tag_start = tagName.find(" ", 0);

    if (tag_start != string::npos)
    {
        tagName = tagName.substr(0, tag_start);
    }

    m_root.set_tag(tagName.c_str());

    m_pos = tag_finish + 1;

    // 检验是否有结束节点
    tag_start = m_xml.rfind("/" + tagName);

    if (tag_start == string::npos)
    {
        //printf("ERROR: Root tag nomatch.\n");
        return -1;
    }

    return ParseElement(&m_root);
}

int DOMDocument::ParseAttribute(MLPCSTR tagString, string& tagName, AttributeMap& attribute)
{
    assert(tagString);

    if (!tagString)
    {
        return -1;
    }

    attribute.clear();

    int m_AttrNum = 0;

    string tempString = tagString;

    TRIM_LEFT(tempString);
    TRIM_RIGHT(tempString);

    string::size_type firstSpacePos = tempString.find_first_of(" \t\r\n");

    if (firstSpacePos == string::npos)
    {
        tagName = tempString;
        return 0;
    }

    tagName = tempString.substr(0, firstSpacePos);

    string::size_type startPos = firstSpacePos + 1;
    string::size_type equalPos = tempString.find('=', startPos);

    while (equalPos != string::npos)
    {
        m_AttrNum ++;

        string attributeValue = "";
        string attributeName = tempString.substr(startPos, equalPos - startPos);
        TRIM_LEFT(attributeName);
        TRIM_RIGHT(attributeName);

        string::size_type leftInvertedCommasPos = tempString.find('\"', equalPos + 1);

        if (leftInvertedCommasPos == string::npos)
        {
            leftInvertedCommasPos = tempString.find('\'', equalPos + 1);

            if (leftInvertedCommasPos == string::npos)
            {
                return -1;
            }
        }

        string::size_type rightInvertedCommasPos = tempString.find('\"', leftInvertedCommasPos + 1);

        if (rightInvertedCommasPos == string::npos)
        {
            rightInvertedCommasPos = tempString.find('\'', leftInvertedCommasPos + 1);

            if (rightInvertedCommasPos == string::npos)
            {
                return -1;
            }
        }

        attributeValue = tempString.substr(leftInvertedCommasPos + 1, rightInvertedCommasPos - leftInvertedCommasPos - 1);

        TRIM_LEFT(attributeValue);
        TRIM_RIGHT(attributeValue);

        attribute.insert(StringPair(attributeName, attributeValue));

        startPos = rightInvertedCommasPos + 1;
        equalPos = tempString.find('=', startPos);
    }

    if (startPos <= tempString.size() - 1)
    {
        return -1;
    }

    return m_AttrNum;
}

DOMElement* DOMDocument::getDocumentElement()
{
    return &m_root;
}
