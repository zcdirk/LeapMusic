#ifndef LONG_MSG_H
#define LONG_MSG_H

#include "MIDIMsg.h"    // CMIDIMsg基类
#include <exception> 


namespace midi
{
    class CLongMsgIndexOutOfBounds : public std::exception
    {
    public:
        const char *what() const
        { return "对象的参数以超出范围。"; }
    };

    class CLongMsg : public CMIDIMsg
    {
    public:
        //构造与析构函数
        CLongMsg();
        CLongMsg(const char *Msg, DWORD Length);
        CLongMsg(const CLongMsg &Msg);
        virtual ~CLongMsg();

        // 赋值
        CLongMsg &operator = (const CLongMsg &Msg);
        
        // 发送信息
        void SendMsg(midi::CMIDIOutDevice &OutDevice);

        // 函数
        DWORD GetLength() const { return m_Length; }
        const char *GetMsg() const { return m_Msg;}
        void SetMsg(const char *Msg, DWORD Length);

    protected:
        // 数组函数，用来访问CLongMsg对象中的独立字节
        char &operator [] (int i);

    private:
        char *m_Msg;
        DWORD m_Length;        
    };
}


#endif