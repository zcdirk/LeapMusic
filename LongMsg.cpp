#include "StdAfx.h"
#include "LongMsg.h"
#include "MIDIOutDevice.h"

using midi::CLongMsg;

// ȱʡ���캯��
CLongMsg::CLongMsg() :
m_Msg(0),
m_Length(0)
{}


// ���캯��
CLongMsg::CLongMsg(const char *Msg, DWORD Length) :
m_Msg(0),
m_Length(0)
{
    SetMsg(Msg, Length);
}

CLongMsg::CLongMsg(const CLongMsg &Msg)
{
    m_Msg = 0;
    m_Length = 0;

    *this = Msg;
}


// ��������
CLongMsg::~CLongMsg()
{
    // ���������ͷ���Դ
    if(m_Msg != 0)
    {
        delete [] m_Msg;
    }
}


// ��ֵ
CLongMsg &CLongMsg::operator = (const CLongMsg &Msg)
{
    // �������ֵ
    if(this != &Msg)
    {
        SetMsg(Msg.m_Msg, Msg.m_Length);
    }

    return *this;
}


// ��MIDI��Ϣ
void CLongMsg::SendMsg(midi::CMIDIOutDevice &OutDevice)
{
    OutDevice.SendMsg(m_Msg, m_Length);
}


// ������Ϣ
void CLongMsg::SetMsg(const char *Msg, DWORD Length)
{
    // ������Ϣ�Դ��������ͷ�
    if(m_Msg != 0)
    {
        delete [] m_Msg;
    }

    // ������Ϣ����ռ䲢��ʼ��
    m_Msg = new char[Length];
    m_Length = Length;

    for(DWORD i = 0; i < m_Length; i++)
    {
        m_Msg[i] = Msg[i];
    }
}


//���麯��
char &CLongMsg::operator [] (int i)
{
    // �߽���
    if(m_Length == 0 || i < 0 || i >= m_Length)
    {
        throw CLongMsgIndexOutOfBounds();
    }

    return m_Msg[i];
}