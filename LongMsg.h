#ifndef LONG_MSG_H
#define LONG_MSG_H

#include "MIDIMsg.h"    // CMIDIMsg����
#include <exception> 


namespace midi
{
    class CLongMsgIndexOutOfBounds : public std::exception
    {
    public:
        const char *what() const
        { return "����Ĳ����Գ�����Χ��"; }
    };

    class CLongMsg : public CMIDIMsg
    {
    public:
        //��������������
        CLongMsg();
        CLongMsg(const char *Msg, DWORD Length);
        CLongMsg(const CLongMsg &Msg);
        virtual ~CLongMsg();

        // ��ֵ
        CLongMsg &operator = (const CLongMsg &Msg);
        
        // ������Ϣ
        void SendMsg(midi::CMIDIOutDevice &OutDevice);

        // ����
        DWORD GetLength() const { return m_Length; }
        const char *GetMsg() const { return m_Msg;}
        void SetMsg(const char *Msg, DWORD Length);

    protected:
        // ���麯������������CLongMsg�����еĶ����ֽ�
        char &operator [] (int i);

    private:
        char *m_Msg;
        DWORD m_Length;        
    };
}


#endif