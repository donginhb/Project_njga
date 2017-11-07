#if !defined(_DOMDOCUMENT_H_)
#define _DOMDOCUMENT_H_

#include <string.h>
#include <stdio.h>
#include "DOMElement.h"

#define TRIM_LEFT(str)\
    {\
        while (0 < str.size() &&\
                (str[0] == '\r' ||\
                 str[0] == '\n' ||\
                 str[0] == '\t' ||\
                 str[0] == ' '))\
        {\
            str.erase(0, 1);\
        }\
    }


#define TRIM_RIGHT(str)\
    {\
        while (0 < str.size() &&\
                (str[str.size() - 1] == '\r' ||\
                 str[str.size() - 1] == '\n' ||\
                 str[str.size() - 1] == '\t' ||\
                 str[str.size() - 1] == ' '))\
        {\
            str.erase(str.size() - 1, 1);\
        }\
    }


class DOMDocument
{
public:
    DOMElement* getDocumentElement();
    DOMDocument();
    virtual ~DOMDocument();

    // 从一块连续的内存中装入XML，并解析(Parse)成XML Tree
    int LoadXML(const char* buffer, unsigned long bufferSize);

    // 从指定的文件中装入XML，并解析(Parse)成XML Tree
    int LoadXML(MLPCSTR pathXML);

    // 将当前的XML Tree转换成XML文档
    int BuildXML(DOMElement* pElement, string& strEncode);

    //将当前的XML 转换为UTF-8编码
    int ConvertXMLToUTF8();

    int CodeConvert(char* from_charset, char* to_charset, string inbuf, size_t inlen, string& outbuf, size_t outlen);

    int u2g(string inbuf, size_t inlen, string& outbuf, size_t outlen);

    int g2u(string inbuf, size_t inlen, string& outbuf, size_t outlen);

    // 获得XML的version信息
    string get_version() const
    {
        return m_version;
    }

    // 获得XML文档 (std::string类型)
    string get_xml() const
    {
        return m_xml;
    }

    // 获得UTF-8 XML文档 (std::string类型)
    string get_utf_8_xml() const
    {
        return m_utf8_xml;
    }

    // 获得XML　Tree的根节点(root node)
    DOMElement* get_root()
    {
        return &m_root;
    }

    // 指定一个节点为根节点，必须要先释放之前的XML Tree，否则会造成内存泄露
    //void set_root(DOMElement* root){ m_root = *root;}

    // 遍历所有节点，并对每一个节点调用pFunc（其实可以加入一个参数，让遍历行为可以从任意一个节点开始）
    int EnumerateElements(LPENUM_CALLBACK_FUNC pFunc, DOMElement* pElectment, void* pRef);

    // 释放XML Tree

    // 清空保存的XML文档内容

protected:
    string::size_type m_pos;

    string m_xml;
    string m_utf8_xml;

    string m_version;
    string m_encode;

    DOMElement m_root;

    int ParseXmlInfo();

    int ParseRoot();
    int ParseElement(DOMElement* element);
    int ParseAttribute(MLPCSTR tagString, string& tagName, AttributeMap& attribute);

    int findNextTag(string& tagName, string& beforeTag);

    int BuildXmlInfo(string& strEncode);
};

#endif // !defined(_DOMDOCUMENT_H_)
