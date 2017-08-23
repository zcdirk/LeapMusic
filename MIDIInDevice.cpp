#include<stdafx.h>
#include "MIDIInDevice.h"
#include "midi.h"

using namespace midi;

// ���캯��
CMIDIInDevice::CMIDIInHeader::CMIDIInHeader(HMIDIIN DevHandle,
                                            LPSTR Buffer, 
                                            DWORD BufferLength) :
m_DevHandle(DevHandle)
{
    // ��ʼ��ͷָ��
    m_MIDIHdr.lpData         = Buffer;
    m_MIDIHdr.dwBufferLength = BufferLength;
    m_MIDIHdr.dwFlags        = 0;

    // ׼��ͷָ��
    MMRESULT Result = ::midiInPrepareHeader(DevHandle, &m_MIDIHdr,
                                            sizeof m_MIDIHdr);

    // ������������ִ���׳�
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIInException(Result);
    }
}


// ��������
CMIDIInDevice::CMIDIInHeader::~CMIDIInHeader()
{
    ::midiInUnprepareHeader(m_DevHandle, &m_MIDIHdr, 
                            sizeof m_MIDIHdr);
}


// Ϊ�������ϵͳ�ض�������
void CMIDIInDevice::CMIDIInHeader::AddSysExBuffer()
{
    MMRESULT Result = ::midiInAddBuffer(m_DevHandle, &m_MIDIHdr,
                                        sizeof m_MIDIHdr);

    // �������������׳�����
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIInException(Result);
    }
}

// ���캯��
CMIDIInDevice::CHeaderQueue::CHeaderQueue()
{
    ::InitializeCriticalSection(&m_CriticalSection);
}


// ��������
CMIDIInDevice::CHeaderQueue::~CHeaderQueue()
{
    RemoveAll();

    ::DeleteCriticalSection(&m_CriticalSection);
}


// Ϊ�������ͷָ��
void CMIDIInDevice::CHeaderQueue::AddHeader(
                                 CMIDIInDevice::CMIDIInHeader *Header)
{
    ::EnterCriticalSection(&m_CriticalSection);

    m_HdrQueue.push(Header);

    ::LeaveCriticalSection(&m_CriticalSection);
}


// �Ӷ������Ƴ�ͷָ��
void CMIDIInDevice::CHeaderQueue::RemoveHeader()
{
    ::EnterCriticalSection(&m_CriticalSection);

    if(!m_HdrQueue.empty())
    {
        delete m_HdrQueue.front();
        m_HdrQueue.pop();
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}


// ���ͷָ�����
void CMIDIInDevice::CHeaderQueue::RemoveAll()
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
bool CMIDIInDevice::CHeaderQueue::IsEmpty()
{
    bool Result;

    ::EnterCriticalSection(&m_CriticalSection);

    Result = m_HdrQueue.empty();

    ::LeaveCriticalSection(&m_CriticalSection);

    return Result;
}

// ���캯������MIDI�����豸���ڹر�״̬��
CMIDIInDevice::CMIDIInDevice() :
m_Receiver(0),
m_State(CLOSED)
{
    // ���޷������¼���ִ���׳��쳣
    if(!CreateEvent())
    {
        throw CMIDIInEventFailure();
    }
}


//���캯������MIDI�����豸���ڹر�״̬��ͬʱ���ý�����
CMIDIInDevice::CMIDIInDevice(CMIDIReceiver &Receiver) :
m_Receiver(&Receiver),
m_State(CLOSED)
{
    // ���޷������¼���ִ���׳��쳣
    if(!CreateEvent())
    {
        throw CMIDIInEventFailure();
    }
}


// ���캯������MIDI�����豸���ڴ�״̬��
CMIDIInDevice::CMIDIInDevice(UINT DeviceId, CMIDIReceiver &Receiver) :
m_Receiver(&Receiver),
m_State(CLOSED)
{
    // ���豸
    Open(DeviceId);

    // ���޷������¼���ִ���׳��쳣
    if(!CreateEvent())
    {
        Close();
        throw CMIDIInEventFailure();
    }
}


// ��������
CMIDIInDevice::~CMIDIInDevice()
{
    // �ر��豸
    Close();

    // �ر��¼�����Ϣ���
    ::CloseHandle(m_Event);
}


// ��MIDI�����豸
void CMIDIInDevice::Open(UINT DeviceId)
{
    // ȷ����ǰ�豸�ѱ��ر�
    Close();

    // ��MIDI�����豸
    MMRESULT Result = ::midiInOpen(&m_DevHandle, DeviceId, 
                                  reinterpret_cast<DWORD>(MidiInProc),
                                  reinterpret_cast<DWORD>(this),
                                  CALLBACK_FUNCTION);

    // �����Դ��豸��m_State��ΪOPENED״̬
    if(Result == MMSYSERR_NOERROR)
    {
        m_State = OPENED;
    }
    // ����ʧ����ִ���׳��쳣
    else
    {
        throw CMIDIInException(Result);
    }
}


// �ر�MIDI�����豸
void CMIDIInDevice::Close()
{
    // ���豸���ڼ�¼���ڹر�ǰֹͣ��¼
    if(m_State == RECORDING)
    {
        StopRecording();
    }

    // ���豸�Ѵ�
    if(m_State == OPENED)
    {
        // �ر��豸
        MMRESULT Result = ::midiInClose(m_DevHandle);

        // ������������ִ���׳��쳣
        if(Result != MMSYSERR_NOERROR)
        {
            throw CMIDIInException(Result);
        }

        //�ı�״̬
        m_State = CLOSED;
    }
}


//����һ�������Դ���ض���Ϣ
void CMIDIInDevice::AddSysExBuffer(LPSTR Buffer, DWORD BufferLength)
{
    CMIDIInHeader *Header;

    try
    {
        // �����µ�ͷָ��
        Header = new CMIDIInHeader(m_DevHandle, Buffer, BufferLength);
    }
    // ���ڴ����ʧ����ִ���׳��쳣
    catch(const std::bad_alloc &)
    {
        throw CMIDIInMemFailure();
    }
    // ����������ͷָ����ִ���׳��쳣
    {
        throw;
    }

    try
    {
        // Ϊ�������ͷָ��
        Header->AddSysExBuffer();
        m_HdrQueue.AddHeader(Header);
    }
    // ���޷�Ϊ���д�����������ɾ��ͷָ�벢ִ���׳��쳣
    catch(const CMIDIInDevice &)
    {
        delete Header;
        throw;
    }
}


// ��ʼ��¼����
void CMIDIInDevice::StartRecording()
{
    // ����MIDI�豸���򿪺�Ž��м�¼
    if(m_State == OPENED)
    { 
        // �ı�״̬
        m_State = RECORDING;

        m_Thread = ::AfxBeginThread((AFX_THREADPROC)HeaderProc, this);

        // ��ʼ��¼
        MMRESULT Result = ::midiInStart(m_DevHandle);

        // �����Լ�¼ʧ��
        if(Result != MMSYSERR_NOERROR)
        {
            // ���½�״̬תΪ��״̬
            m_State = OPENED;

            // ֪ͨ�����߳̽���
            ::SetEvent(m_Event);

			::WaitForSingleObject(m_Thread->m_hThread, INFINITE);

            // ִ���׳��쳣
            throw CMIDIInException(Result);
        }
    }
}


// ֹͣ��¼����
void CMIDIInDevice::StopRecording()
{
    // ���豸���ڼ�¼
    if(m_State == RECORDING)
    {
        // �ı��豸״̬
        m_State = OPENED;

        // ֪ͨ�����߳̽���
        ::SetEvent(m_Event);

		::WaitForSingleObject(m_Thread->m_hThread, INFINITE);

        //����MIDI�����豸
        ::midiInReset(m_DevHandle);

        // ���ͷָ�����
        m_HdrQueue.RemoveAll();
    }
}


// ע��MIDI������������ǰһ������
CMIDIReceiver *CMIDIInDevice::SetReceiver(CMIDIReceiver &Receiver)
{
    CMIDIReceiver *PrevReceiver = m_Receiver;

    m_Receiver = &Receiver;

    return PrevReceiver;
}


// �ж�MIDI�����豸�Ƿ��
bool CMIDIInDevice::IsOpen() const
{
    return ((m_State == OPENED) || (m_State == RECORDING));
}


// �ж�MIDI�����豸�Ƿ��ڼ�¼
bool CMIDIInDevice::IsRecording() const
{
    return (m_State == RECORDING);
}


// ��ȡ�豸ID
UINT CMIDIInDevice::GetDevID() const
{
    UINT DeviceID;
    MMRESULT Result = ::midiInGetID(m_DevHandle, &DeviceID);

    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIInException(Result);
    }

    return DeviceID;
}


// ��ȡMIDI�����豸������
void CMIDIInDevice::GetDevCaps(UINT DeviceId, MIDIINCAPS &Caps)
{
    MMRESULT Result = ::midiInGetDevCaps(DeviceId, &Caps, 
                                         sizeof Caps);

    //���޷������豸������ִ���׳��쳣
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIInException(Result);
    }
}


// �����¼���֪ͨ�߳�
bool CMIDIInDevice::CreateEvent()
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


// ������MIDI�����¼�����ô˺���
void CALLBACK CMIDIInDevice::MidiInProc(HMIDIIN MidiIn, UINT Msg,
                                        DWORD_PTR Instance, DWORD_PTR Param1,
                                        DWORD_PTR Param2)
{
    CMIDIInDevice *Device;
    
    Device = reinterpret_cast<CMIDIInDevice *>(Instance);

    if(Device->m_Receiver != 0)
    {
        switch(Msg)
        {
        case MIM_DATA:      // ���ն���Ϣ
            Device->m_Receiver->ReceiveMsg(Param1, Param2);
            break;

        case MIM_ERROR:     // �����յ���Ч�Ķ���Ϣ
            Device->m_Receiver->OnError(Param1, Param2);
            break;

        case MIM_LONGDATA:  // ����ϵͳ�ض���Ϣ������Ϣ��
            if(Device->m_State == RECORDING)
            {
                MIDIHDR *MidiHdr = reinterpret_cast<MIDIHDR *>(Param1);
                Device->m_Receiver->ReceiveMsg(MidiHdr->lpData, 
                                               MidiHdr->dwBytesRecorded, 
                                               Param2);
                ::SetEvent(Device->m_Event);
            }
            break;

        case MIM_LONGERROR: // ���յ���Ч�ĳ���Ϣ
            if(Device->m_State == RECORDING)
            {
                MIDIHDR *MidiHdr = reinterpret_cast<MIDIHDR *>(Param1);
                Device->m_Receiver->OnError(MidiHdr->lpData,
                                            MidiHdr->dwBytesRecorded,
                                            Param2);
                ::SetEvent(Device->m_Event);
            }
            break;
        }
    }
}


// ���ڴ���ͷָ����̺߳���
DWORD CMIDIInDevice::HeaderProc(LPVOID Parameter)
{
    CMIDIInDevice *Device; 
    
    Device = reinterpret_cast<CMIDIInDevice *>(Parameter);

    // ��MIDI�����豸���ڼ�¼״̬��ѭ������
    while(Device->m_State == RECORDING)
    {
        ::WaitForSingleObject(Device->m_Event, INFINITE);

        // ȷ���豸���ڼ�¼
        if(Device->m_State == RECORDING)
        {
            // �Ƴ������ָ��
            Device->m_HdrQueue.RemoveHeader();
        }
    }

    return 0;
}
