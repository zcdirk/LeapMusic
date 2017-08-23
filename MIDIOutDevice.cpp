#include<stdafx.h>
#include "MIDIOutDevice.h"
#include "midi.h"


using midi::CMIDIOutDevice;
using midi::CMIDIOutException;

//���캯��
CMIDIOutDevice::CMIDIOutHeader::CMIDIOutHeader(HMIDIOUT DevHandle,
                                               LPSTR Msg, 
                                               DWORD MsgLength) :
m_DevHandle(DevHandle)
{
    // ��ʼ��ͷָ��
    m_MIDIHdr.lpData         = Msg;
    m_MIDIHdr.dwBufferLength = MsgLength;
    m_MIDIHdr.dwFlags        = 0;

    // ׼��ͷָ��
    MMRESULT Result = ::midiOutPrepareHeader(DevHandle, &m_MIDIHdr,
                                             sizeof m_MIDIHdr);

    // ������������ִ���׳�
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIOutException(Result);
    }
}


// ��������
CMIDIOutDevice::CMIDIOutHeader::~CMIDIOutHeader()
{
    ::midiOutUnprepareHeader(m_DevHandle, &m_MIDIHdr, 
                             sizeof m_MIDIHdr);
}


// ���ͳ���Ϣ
void CMIDIOutDevice::CMIDIOutHeader::SendMsg()
{
    MMRESULT Result = ::midiOutLongMsg(m_DevHandle, &m_MIDIHdr,
                                       sizeof m_MIDIHdr);

    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIOutException(Result);
    }
}

// ���캯��
CMIDIOutDevice::CHeaderQueue::CHeaderQueue()
{
    ::InitializeCriticalSection(&m_CriticalSection);
}


// ��������
CMIDIOutDevice::CHeaderQueue::~CHeaderQueue()
{
    RemoveAll();

    ::DeleteCriticalSection(&m_CriticalSection);
}


// Ϊ�������ͷָ��
void CMIDIOutDevice::CHeaderQueue::AddHeader(
                               CMIDIOutDevice::CMIDIOutHeader *Header)
{
    ::EnterCriticalSection(&m_CriticalSection);

    m_HdrQueue.push(Header);

    ::LeaveCriticalSection(&m_CriticalSection);
}


// �Ӷ������Ƴ�ͷָ��
void CMIDIOutDevice::CHeaderQueue::RemoveHeader()
{
    ::EnterCriticalSection(&m_CriticalSection);

    if(!m_HdrQueue.empty())
    {
        delete m_HdrQueue.front();
        m_HdrQueue.pop();
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}


//���ͷָ�����
void CMIDIOutDevice::CHeaderQueue::RemoveAll()
{
    ::EnterCriticalSection(&m_CriticalSection);

    while(!m_HdrQueue.empty())
    {
        delete m_HdrQueue.front();
        m_HdrQueue.pop();
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}


// �ж϶����Ƿ�Ϊ��
bool CMIDIOutDevice::CHeaderQueue::IsEmpty()
{
    bool Result;

    ::EnterCriticalSection(&m_CriticalSection);

    Result = m_HdrQueue.empty();

    ::LeaveCriticalSection(&m_CriticalSection);

    return Result;
}


// ���캯������MIDI����豸���ڹر�״̬��
CMIDIOutDevice::CMIDIOutDevice() :
m_State(CLOSED)
{
    // ���޷������¼���ִ���׳��쳣
    if(!CreateEvent())
    {
        throw CMIDIOutEventFailure();
    }
}


// ���캯������MIDI����豸���ڴ�״̬��
CMIDIOutDevice::CMIDIOutDevice(UINT DeviceId) :
m_State(CLOSED)
{
    // ���豸
    Open(DeviceId);

    // ���޷������¼���ִ���׳��쳣
    if(!CreateEvent())
    {
        Close();
        throw CMIDIOutEventFailure();
    }
}


// ��������
CMIDIOutDevice::~CMIDIOutDevice()
{
    // �ر��豸
    Close();

    // �ر��¼�����Ϣ���
    ::CloseHandle(m_Event);
}


// ��MIDI����豸
void CMIDIOutDevice::Open(UINT DeviceId)
{
    // ȷ����ǰ�豸�ѱ��ر�
    Close();

    // ��MIDI����豸
    MMRESULT Result = ::midiOutOpen(&m_DevHandle, DeviceId, 
                                 reinterpret_cast<DWORD>(MidiOutProc),
                                 reinterpret_cast<DWORD>(this),
                                 CALLBACK_FUNCTION);

    // �����Դ��豸��m_State��ΪOPENED״̬
    if(Result == MMSYSERR_NOERROR)
    {
        m_State = OPENED;
        m_WorkerThread = AfxBeginThread(
            reinterpret_cast<AFX_THREADPROC>(HeaderProc), this);
    }
    // ����ʧ����ִ���׳��쳣
    else
    {
        ::SetEvent(m_Event);
        ::WaitForSingleObject(m_WorkerThread->m_hThread, INFINITE);

        throw CMIDIOutException(Result);
    }
}


// �ر�MIDI����豸
void CMIDIOutDevice::Close()
{
    // ���豸���ڼ�¼���ڹر�ǰֹͣ��¼
    if(m_State == OPENED)
    {
        // �ı�״̬
        m_State = CLOSED;

        // ֪ͨ�������̲��ȴ����̽���
        ::SetEvent(m_Event);
        ::WaitForSingleObject(m_WorkerThread->m_hThread, INFINITE);

        // ���ͷ�ļ�����
        m_HdrQueue.RemoveAll();

        // �ر�MIDI����豸
        ::midiOutClose(m_DevHandle);
    }
}


// ���Ͷ���Ϣ
void CMIDIOutDevice::SendMsg(DWORD Msg)
{
    if(m_State == OPENED)
    {
        MMRESULT Result = ::midiOutShortMsg(m_DevHandle, Msg);

        if(Result != MMSYSERR_NOERROR)
        {
            throw CMIDIOutException(Result);
        }
    }
}


// ���ͳ���Ϣ
void CMIDIOutDevice::SendMsg(LPSTR Msg, DWORD MsgLength)
{
    if(m_State == OPENED)
    {  
        CMIDIOutHeader *Header;

        try
        {
            // �����µ�ͷָ���Դ���ϵͳ�ض���Ϣ
            Header = new CMIDIOutHeader(m_DevHandle, Msg, MsgLength);
        }
        // ���ڴ����ʧ����ִ���׳��쳣
        catch(const std::bad_alloc &)
        {
            throw CMIDIOutMemFailure();
        }
        // ����������ͷָ����ִ���׳��쳣
        catch(const CMIDIOutException &)
        {
            throw;
        }

        try
        {
            // ����ϵͳ�ض���Ϣ
            Header->SendMsg();

            // Ϊ�������ͷָ��
            m_HdrQueue.AddHeader(Header);
        }
        // ����ϵͳ�����ض���Ϣʧ�����ͷ�ָ�벢ִ���׳�
        catch(const CMIDIOutException &)
        {
            delete Header;
            throw;
        }
    }
}


// �ж�MIDI����豸�Ƿ��
bool CMIDIOutDevice::IsOpen() const
{
    return (m_State == OPENED);
}


// ��ȡ�豸ID
UINT CMIDIOutDevice::GetDevID() const
{
    UINT DeviceID;
    MMRESULT Result = ::midiOutGetID(m_DevHandle, &DeviceID);

    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIOutException(Result);
    }

    return DeviceID;
}


// ��ȡMIDI����豸������
void CMIDIOutDevice::GetDevCaps(UINT DeviceId, MIDIOUTCAPS &Caps)
{
    MMRESULT Result = ::midiOutGetDevCaps(DeviceId, &Caps, 
                                          sizeof Caps);

    // ���޷������豸������ִ���׳��쳣
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIOutException(Result);
    }
}


// �����¼���֪ͨ�߳�
bool CMIDIOutDevice::CreateEvent()
{
    bool Result = true;

    m_Event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    // ���޷�����ʱ���򷵻�false
    if(m_Event == NULL)
    {
        Result = false;
    }

    return Result;
}

//������MIDI����¼�����ô˺���
void CALLBACK CMIDIOutDevice::MidiOutProc(HMIDIOUT MidiOut, UINT Msg,
                                          DWORD_PTR Instance, DWORD_PTR Param1,
                                          DWORD_PTR Param2)
{
    CMIDIOutDevice *Device;
    
    Device = reinterpret_cast<CMIDIOutDevice *>(Instance);

    if(Msg == MOM_DONE)
    {
        ::SetEvent(Device->m_Event);
    }
}


// ���ڴ���ͷָ����̺߳���
DWORD CMIDIOutDevice::HeaderProc(LPVOID Parameter)
{
    CMIDIOutDevice *Device; 
    
    Device = reinterpret_cast<CMIDIOutDevice *>(Parameter);

    // ��MIDI����豸����ѭ������
    while(Device->m_State == OPENED)
    {
        ::WaitForSingleObject(Device->m_Event, INFINITE);

        // ȷ���豸��
        if(Device->m_State == OPENED)
        {
            // �Ƴ������ָ��
            Device->m_HdrQueue.RemoveHeader();
        }
    }

    return 0;
}



