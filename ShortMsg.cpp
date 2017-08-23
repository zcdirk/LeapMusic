#include<stdafx.h>
#include "midi.h"
#include "ShortMsg.h"
#include "MIDIOutDevice.h"
#include "MIDIInDevice.h"

using midi::CShortMsg;

// 构造函数

CShortMsg::CShortMsg(DWORD TimeStamp) :
m_Msg(0),
m_MsgNoStatus(0)
{
    SetTimeStamp(TimeStamp);
}

CShortMsg::CShortMsg(DWORD Msg, DWORD TimeStamp) : 
m_Msg(Msg)
{
    unsigned char DataByte1 = GetData1();
    unsigned char DataByte2 = GetData2();

    m_MsgNoStatus = PackShortMsg(DataByte1, DataByte2);

    SetTimeStamp(TimeStamp);
}

CShortMsg::CShortMsg(unsigned char Status, unsigned char Data1, 
                     unsigned char Data2, DWORD TimeStamp)
{
    SetMsg(Status, Data1, Data2);
    m_MsgNoStatus = PackShortMsg(Data1, Data2);

    SetTimeStamp(TimeStamp);
}

CShortMsg::CShortMsg(unsigned char Command, unsigned char Channel, 
                     unsigned char Data1, unsigned char Data2,
                     DWORD TimeStamp)
{
    SetMsg(Command, Channel, Data1, Data2);
    m_MsgNoStatus = PackShortMsg(Data1, Data2);

    SetTimeStamp(TimeStamp);
}


// 发送信息
void CShortMsg::SendMsg(midi::CMIDIOutDevice &OutDevice)
{
    OutDevice.SendMsg(m_Msg);
}


// 发送无状态字节的信息
void CShortMsg::SendMsgNoStatus(midi::CMIDIOutDevice &OutDevice)
{
    OutDevice.SendMsg(m_MsgNoStatus);
}


// 接收消息
const char *CShortMsg::GetMsg() const
{
    return reinterpret_cast<const char *>(&m_Msg);
}


// 获取状态字节
unsigned char CShortMsg::GetStatus() const
{
    unsigned char Status;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Status, Dummy, Dummy);

    return Status;
}


// 获取MIDI频道
unsigned char CShortMsg::GetChannel() const
{
    unsigned char Channel;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Dummy, Channel, Dummy, Dummy);

    return Channel;
}


// 获取指令码
unsigned char CShortMsg::GetCommand() const
{
    unsigned char Command;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Command, Dummy, Dummy, Dummy);

    return Command;
}


// 获取数据字节1
unsigned char CShortMsg::GetData1() const
{
    unsigned char Data1;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Dummy, Dummy, Data1, Dummy);

    return Data1;
}


// 获取数据字节2
unsigned char CShortMsg::GetData2() const
{
    unsigned char Data2;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Dummy, Dummy, Dummy, Data2);

    return Data2;
}


// 设置消息
void CShortMsg::SetMsg(unsigned char Status, unsigned char Data1,
                       unsigned char Data2)
{
    m_Msg = PackShortMsg(Status, Data1, Data2);
    m_MsgNoStatus = PackShortMsg(Data1, Data2);
}

void CShortMsg::SetMsg(unsigned char Command, unsigned char Channel,
                       unsigned char Data1, unsigned char Data2)
{
    m_Msg = PackShortMsg(Command, Channel, Data1, Data2);
    m_MsgNoStatus = PackShortMsg(Data1, Data2);
}


// 将数据打包成无状态字节的短消息
DWORD CShortMsg::PackShortMsg(unsigned char DataByte1,
                              unsigned char DataByte2)
{
    DWORD Msg = DataByte1;
    Msg |= DataByte2 << midi::SHORT_MSG_SHIFT;

    return Msg;
}


// 将数据打包成有状态字节的短消息
DWORD CShortMsg::PackShortMsg(unsigned char Status,
                              unsigned char DataByte1,
                              unsigned char DataByte2)
{
    DWORD Msg = Status;
    Msg |= DataByte1 << midi::SHORT_MSG_SHIFT;
    Msg |= DataByte2 << midi::SHORT_MSG_SHIFT * 2;

    return Msg;
}


// 将数据打包成短频道讯息
DWORD CShortMsg::PackShortMsg(unsigned char Command,
                              unsigned char Channel,
                              unsigned char DataByte1,
                              unsigned char DataByte2)
{
    DWORD Msg = Command | Channel;
    Msg |= DataByte1 << midi::SHORT_MSG_SHIFT;
    Msg |= DataByte2 << midi::SHORT_MSG_SHIFT * 2;

    return Msg;
}


// 解压短消息
void CShortMsg::UnpackShortMsg(DWORD Msg, unsigned char &Status,
                               unsigned char &DataByte1,
                               unsigned char &DataByte2)
{
    Status = static_cast<unsigned char>(Msg);
    DataByte1 = static_cast<unsigned char>
                                   (Msg >> midi::SHORT_MSG_SHIFT);
    DataByte2 = static_cast<unsigned char>
                                   (Msg >> midi::SHORT_MSG_SHIFT * 2);
}


// 解压短频道讯息
void CShortMsg::UnpackShortMsg(DWORD Msg, unsigned char &Command,
                               unsigned char &Channel,
                               unsigned char &DataByte1,
                               unsigned char &DataByte2)
{
    Command = static_cast<unsigned char>(Msg & ~midi::SHORT_MSG_MASK);
    Channel = static_cast<unsigned char>(Msg & midi::SHORT_MSG_MASK);
    DataByte1 = static_cast<unsigned char>
                                   (Msg >> midi::SHORT_MSG_SHIFT);
    DataByte2 = static_cast<unsigned char>
                                   (Msg >> midi::SHORT_MSG_SHIFT * 2);
}