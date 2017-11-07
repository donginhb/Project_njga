#ifndef __PACKET__H__
#define __PACKET__H__


#include "DOMElement.h"
#include "DOMDocument.h"

#define ERRORMSGSIZE    512

typedef struct _REF_INPUT
{
    string sTag;
    string sAttr;
    string sValue;
    DOMElement* sFoundElement;
} REF_INPUT;


/*************************************
    函数名：SearchElementCell
    描述: 设置元素属性
    输入：
        pElement
        szinBuf
    输出:
           TRUE:  成功
           FALSE: 失败
*************************************/
int SECellByName(DOMElement* pElement, void* pRef);
int SECellByAttr(DOMElement* pElement, void* pRef);

/***********************************
  类名：CPacket
  描述：用于通讯包描述,其核心为一Dom树
************************************/

class CPacket
{
public:
    CPacket();
    ~CPacket();

    /*************************************
     函数名：BuiltTree
     描述: 根据XMl字串/文件生成DOM树
     输入：文件路径/字串,长度
     输出:
            0:  成功
            -1: 失败
     *************************************/
    int BuiltTree(MLPCSTR pXmlPath);
    int BuiltTree(const char* pBuf, int nBufSize);

    /*************************************
     函数名：SearchElement
     描述: 查找节点/根据节点名,属性查找节点
     输入：节点的名字,属性
     输出:
            节点:  成功
            NULL: 失败
     *************************************/
    DOMElement* SearchElement(char* szNodeName);
    DOMElement* SearchElement(char* szNodeName, char* szProper, char* szId);

    /*************************************
     函数名：SearchNextElement
     描述: 查找当前节点的下一个节点
     注意: 无参数的需要与SetCurrentElement函数配合使用
     输入：节点的名字,路径,属性
            pMoveNext: 是否将当前节点设置成下一个节点
     输出:
            节点: 成功
            NULL: 失败
     *************************************/
    //查找当前节点的下一个节点
    DOMElement* SearchNextElement(bool pMoveNext = 1);
    DOMElement* SearchNextElement(char* szNodeName, char* szProper = NULL, char* szId = NULL);

    /*************************************
     函数名：SetCurrentNode
     描述: 设置当前节点
     输入：节点的全路径
     输出:
            TRUE:  成功
            FALSE: 失败
     *************************************/
    bool SetCurrentElement(char szNodeName[]);
    void SetCurrentElement(DOMElement* elem)
    {
        m_CurrentElement = elem;
    };

    /*************************************
     函数名：GetNodeValue
     描述: 获得节点
     输入：
        szNodeName：可以是相对当前节点路径或全路径
                ".\"表示当前节点
        szOutBuf  : 输出字符串值
     输出:
            TRUE:  成功
            FALSE: 失败
     *************************************/
    bool GetElementValue(char* szNodeName, char* szOutBuf);
    bool GetElementValue(char* szNodeName, char* szOutBuf, char* szProper, char* szId);


    /*************************************
     函数名：SetNodeValue
     描述: 获得节点
     输入：
        szNodeName ：可以是相对当前节点路径或全路径,
        szinBuf    : 输入的字符串值
     输出:
            TRUE:  成功
            FALSE: 失败
     *************************************/
    bool SetElementValue(char* szNodeName, char* szinBuf);
    bool SetElementValue(DOMElement* pelem, char* szinBuf);


    /*************************************
     函数名：GetElementAttr
     描述: 读取元素属性
     输入：
        szNodeName ：可以是相对当前节点路径或全路径,
        szinBuf    : 输入的字符串值
     输出:
            TRUE:       成功
            FALSE:      失败
            szAttrValue:属性的值
     *************************************/
    bool GetElementAttr(char* szNodeName, char* szAttrName, string& szAttrValue);
    bool GetElementAttr(DOMElement* pelem, char* szAttrName, string& szAttrValue);


    /*************************************
     函数名：SetElementAttr
     描述: 设置元素属性
     输入：
        szNodeName ：可以是相对当前节点路径或全路径,
        szAttrName : 属性名称
        szAttrValue: 属性值
     输出:
            TRUE:  成功
            FALSE: 失败
     *************************************/
    bool SetElementAttr(char* szNodeName, char* szAttrName, char* szAttrValue);
    bool SetElementAttr(DOMElement* pelem, char* szAttrName, char* szAttrValue);


public:

    //返回文档的对象
    DOMDocument* GetDOMDocument()
    {
        return &m_DomTree;
    };

    //返回XML的文本串
    string GetXml(DOMElement* pElement, string strEncode = "");

    //设置根节点的标签
    int SetRootTag(string pTag);

    //创建节点,可以用相对路径和绝对路径
    DOMElement* CreateElement(char szNodeName[]);

    //重置当前节点到根节点
    void ResetCurrentNode();


private:

    //切分字符串，放在一个容器中(内部函数)
    int split_bychar(vector<string>& vec, const string& str, const char seperate = '/');

    /*************************************
     函数名：Initialize
     描述: 用于生成包的初始化
     输入：n/N
     输出:
            TRUE:  成功
            FALSE: 失败
    *************************************/
    bool Initialize();


private:
    /*dom树存储了包结构*/
    DOMDocument m_DomTree;
    DOMElement*   m_CurrentElement;


    int m_LastError;
    bool m_bInitFlag;

    // 当前tag名
    string m_CurrentTag;
    string m_CrrentAttr;
    string m_CrrentValue;
    DOMElement* m_FoundElement;
};

#endif
