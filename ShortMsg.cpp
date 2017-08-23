#include<stdafx.h>
#include "midi.h"
#include "ShortMsg.h"
#include "MIDIOutDevice.h"
#include "MIDIInDevice.h"

using midi::CShortMsg;

// ���캯��

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


// ������Ϣ
void CShortMsg::SendMsg(midi::CMIDIOutDevice &OutDevice)
{
    OutDevice.SendMsg(m_Msg);
}


// ������״̬�ֽڵ���Ϣ
void CShortMsg::SendMsgNoStatus(midi::CMIDIOutDevice &OutDevice)
{
    OutDevice.SendMsg(m_MsgNoStatus);
}


// ������Ϣ
const char *CShortMsg::GetMsg() const
{
    return reinterpret_cast<const char *>(&m_Msg);
}


// ��ȡ״̬�ֽ�
unsigned char CShortMsg::GetStatus() const
{
    unsigned char Status;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Status, Dummy, Dummy);

    return Status;
}


// ��ȡMIDIƵ��
unsigned char CShortMsg::GetChannel() const
{
    unsigned char Channel;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Dummy, Channel, Dummy, Dummy);

    return Channel;
}


// ��ȡָ����
unsigned char CShortMsg::GetCommand() const
{
    unsigned char Command;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Command, Dummy, Dummy, Dummy);

    return Command;
}


// ��ȡ�����ֽ�1
unsigned char CShortMsg::GetData1() const
{
    unsigned char Data1;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Dummy, Dummy, Data1, Dummy);

    return Data1;
}


// ��ȡ�����ֽ�2
unsigned char CShortMsg::GetData2() const
{
    unsigned char Data2;
    unsigned char Dummy;

    UnpackShortMsg(m_Msg, Dummy, Dummy, Dummy, Data2);

    return Data2;
}


// ������Ϣ
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


// �����ݴ������״̬�ֽڵĶ���Ϣ
DWORD CShortMsg::PackShortMsg(unsigned char DataByte1,
                              unsigned char DataByte2)
{
    DWORD Msg = DataByte1;
    Msg |= DataByte2 << midi::SHORT_MSG_SHIFT;

    return Msg;
}


// �����ݴ������״̬�ֽڵĶ���Ϣ
DWORD CShortMsg::PackShortMsg(unsigned char Status,
                              unsigned char DataByte1,
                              unsigned char DataByte2)
{
    DWORD Msg = Status;
    Msg |= DataByte1 << midi::SHORT_MSG_SHIFT;
    Msg |= DataByte2 << midi::SHORT_MSG_SHIFT * 2;

    return Msg;
}


// �����ݴ���ɶ�Ƶ��ѶϢ
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


// ��ѹ����Ϣ
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


// ��ѹ��Ƶ��ѶϢ
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