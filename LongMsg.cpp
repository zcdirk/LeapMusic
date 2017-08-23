#include "StdAfx.h"
#include "LongMsg.h"
#include "MIDIOutDevice.h"

using midi::CLongMsg;

// 缺省构造函数
CLongMsg::CLongMsg() :
m_Msg(0),
m_Length(0)
{}


// 构造函数
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


// 析构函数
CLongMsg::~CLongMsg()
{
    // 若存在则释放资源
    if(m_Msg != 0)
    {
        delete [] m_Msg;
    }
}


// 赋值
CLongMsg &CLongMsg::operator = (const CLongMsg &Msg)
{
    // 检测自身值
    if(this != &Msg)
    {
        SetMsg(Msg.m_Msg, Msg.m_Length);
    }

    return *this;
}


// 发MIDI消息
void CLongMsg::SendMsg(midi::CMIDIOutDevice &OutDevice)
{
    OutDevice.SendMsg(m_Msg, m_Length);
}


// 设置消息
void CLongMsg::SetMsg(const char *Msg, DWORD Length)
{
    // 若旧消息仍存在则将其释放
    if(m_Msg != 0)
    {
        delete [] m_Msg;
    }

    // 给新消息分配空间并初始化
    m_Msg = new char[Length];
    m_Length = Length;

    for(DWORD i = 0; i < m_Length; i++)
    {
        m_Msg[i] = Msg[i];
    }
}


//数组函数
char &CLongMsg::operator [] (int i)
{
    // 边界检测
    if(m_Length == 0 || i < 0 || i >= m_Length)
    {
        throw CLongMsgIndexOutOfBounds();
    }

    return m_Msg[i];
}