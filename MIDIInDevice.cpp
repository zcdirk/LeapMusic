#include<stdafx.h>
#include "MIDIInDevice.h"
#include "midi.h"

using namespace midi;

// 构造函数
CMIDIInDevice::CMIDIInHeader::CMIDIInHeader(HMIDIIN DevHandle,
                                            LPSTR Buffer, 
                                            DWORD BufferLength) :
m_DevHandle(DevHandle)
{
    // 初始化头指针
    m_MIDIHdr.lpData         = Buffer;
    m_MIDIHdr.dwBufferLength = BufferLength;
    m_MIDIHdr.dwFlags        = 0;

    // 准备头指针
    MMRESULT Result = ::midiInPrepareHeader(DevHandle, &m_MIDIHdr,
                                            sizeof m_MIDIHdr);

    // 若发生错误则执行抛出
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIInException(Result);
    }
}


// 析构函数
CMIDIInDevice::CMIDIInHeader::~CMIDIInHeader()
{
    ::midiInUnprepareHeader(m_DevHandle, &m_MIDIHdr, 
                            sizeof m_MIDIHdr);
}


// 为队列添加系统特定缓冲区
void CMIDIInDevice::CMIDIInHeader::AddSysExBuffer()
{
    MMRESULT Result = ::midiInAddBuffer(m_DevHandle, &m_MIDIHdr,
                                        sizeof m_MIDIHdr);

    // 若产生错误则抛出异外
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIInException(Result);
    }
}

// 构造函数
CMIDIInDevice::CHeaderQueue::CHeaderQueue()
{
    ::InitializeCriticalSection(&m_CriticalSection);
}


// 析构函数
CMIDIInDevice::CHeaderQueue::~CHeaderQueue()
{
    RemoveAll();

    ::DeleteCriticalSection(&m_CriticalSection);
}


// 为队列添加头指针
void CMIDIInDevice::CHeaderQueue::AddHeader(
                                 CMIDIInDevice::CMIDIInHeader *Header)
{
    ::EnterCriticalSection(&m_CriticalSection);

    m_HdrQueue.push(Header);

    ::LeaveCriticalSection(&m_CriticalSection);
}


// 从队列中移除头指针
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


// 清空头指针队列
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


// 判断队列是否为空
bool CMIDIInDevice::CHeaderQueue::IsEmpty()
{
    bool Result;

    ::EnterCriticalSection(&m_CriticalSection);

    Result = m_HdrQueue.empty();

    ::LeaveCriticalSection(&m_CriticalSection);

    return Result;
}

// 构造函数（若MIDI输入设备处于关闭状态）
CMIDIInDevice::CMIDIInDevice() :
m_Receiver(0),
m_State(CLOSED)
{
    // 若无法创建事件则执行抛出异常
    if(!CreateEvent())
    {
        throw CMIDIInEventFailure();
    }
}


//构造函数（若MIDI输入设备处于关闭状态）同时设置接收器
CMIDIInDevice::CMIDIInDevice(CMIDIReceiver &Receiver) :
m_Receiver(&Receiver),
m_State(CLOSED)
{
    // 若无法创建事件则执行抛出异常
    if(!CreateEvent())
    {
        throw CMIDIInEventFailure();
    }
}


// 构造函数（若MIDI输入设备处于打开状态）
CMIDIInDevice::CMIDIInDevice(UINT DeviceId, CMIDIReceiver &Receiver) :
m_Receiver(&Receiver),
m_State(CLOSED)
{
    // 打开设备
    Open(DeviceId);

    // 若无法创建事件则执行抛出异常
    if(!CreateEvent())
    {
        Close();
        throw CMIDIInEventFailure();
    }
}


// 析构函数
CMIDIInDevice::~CMIDIInDevice()
{
    // 关闭设备
    Close();

    // 关闭事件的消息句柄
    ::CloseHandle(m_Event);
}


// 打开MIDI输入设备
void CMIDIInDevice::Open(UINT DeviceId)
{
    // 确保先前设备已被关闭
    Close();

    // 打开MIDI输入设备
    MMRESULT Result = ::midiInOpen(&m_DevHandle, DeviceId, 
                                  reinterpret_cast<DWORD>(MidiInProc),
                                  reinterpret_cast<DWORD>(this),
                                  CALLBACK_FUNCTION);

    // 若可以打开设备则将m_State变为OPENED状态
    if(Result == MMSYSERR_NOERROR)
    {
        m_State = OPENED;
    }
    // 若打开失败则执行抛出异常
    else
    {
        throw CMIDIInException(Result);
    }
}


// 关闭MIDI输入设备
void CMIDIInDevice::Close()
{
    // 若设备正在记录则在关闭前停止记录
    if(m_State == RECORDING)
    {
        StopRecording();
    }

    // 若设备已打开
    if(m_State == OPENED)
    {
        // 关闭设备
        MMRESULT Result = ::midiInClose(m_DevHandle);

        // 若发生错误则执行抛出异常
        if(Result != MMSYSERR_NOERROR)
        {
            throw CMIDIInException(Result);
        }

        //改变状态
        m_State = CLOSED;
    }
}


//增加一缓冲区以存放特定消息
void CMIDIInDevice::AddSysExBuffer(LPSTR Buffer, DWORD BufferLength)
{
    CMIDIInHeader *Header;

    try
    {
        // 创建新的头指针
        Header = new CMIDIInHeader(m_DevHandle, Buffer, BufferLength);
    }
    // 若内存分配失败则执行抛出异常
    catch(const std::bad_alloc &)
    {
        throw CMIDIInMemFailure();
    }
    // 若不能生成头指针则执行抛出异常
    {
        throw;
    }

    try
    {
        // 为队列添加头指针
        Header->AddSysExBuffer();
        m_HdrQueue.AddHeader(Header);
    }
    // 若无法为队列创建缓冲区则删除头指针并执行抛出异常
    catch(const CMIDIInDevice &)
    {
        delete Header;
        throw;
    }
}


// 开始记录进程
void CMIDIInDevice::StartRecording()
{
    // 仅当MIDI设备被打开后才进行记录
    if(m_State == OPENED)
    { 
        // 改变状态
        m_State = RECORDING;

        m_Thread = ::AfxBeginThread((AFX_THREADPROC)HeaderProc, this);

        // 开始记录
        MMRESULT Result = ::midiInStart(m_DevHandle);

        // 若尝试记录失败
        if(Result != MMSYSERR_NOERROR)
        {
            // 重新将状态转为打开状态
            m_State = OPENED;

            // 通知工作线程结束
            ::SetEvent(m_Event);

			::WaitForSingleObject(m_Thread->m_hThread, INFINITE);

            // 执行抛出异常
            throw CMIDIInException(Result);
        }
    }
}


// 停止记录进程
void CMIDIInDevice::StopRecording()
{
    // 若设备已在记录
    if(m_State == RECORDING)
    {
        // 改变设备状态
        m_State = OPENED;

        // 通知工作线程结束
        ::SetEvent(m_Event);

		::WaitForSingleObject(m_Thread->m_hThread, INFINITE);

        //重置MIDI输入设备
        ::midiInReset(m_DevHandle);

        // 清空头指针队列
        m_HdrQueue.RemoveAll();
    }
}


// 注册MIDI接收器并返回前一接收器
CMIDIReceiver *CMIDIInDevice::SetReceiver(CMIDIReceiver &Receiver)
{
    CMIDIReceiver *PrevReceiver = m_Receiver;

    m_Receiver = &Receiver;

    return PrevReceiver;
}


// 判断MIDI输入设备是否打开
bool CMIDIInDevice::IsOpen() const
{
    return ((m_State == OPENED) || (m_State == RECORDING));
}


// 判断MIDI输入设备是否在记录
bool CMIDIInDevice::IsRecording() const
{
    return (m_State == RECORDING);
}


// 获取设备ID
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


// 获取MIDI输入设备的容量
void CMIDIInDevice::GetDevCaps(UINT DeviceId, MIDIINCAPS &Caps)
{
    MMRESULT Result = ::midiInGetDevCaps(DeviceId, &Caps, 
                                         sizeof Caps);

    //若无法检索设备容量则执行抛出异常
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIInException(Result);
    }
}


// 创建事件以通知线程
bool CMIDIInDevice::CreateEvent()
{
    bool Result = true;

    m_Event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

    // 若无法创建时间则返回false
    if(m_Event == NULL)
    {
        Result = false;
    }

    return Result;
}


// 若发生MIDI输入事件则调用此函数
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
        case MIM_DATA:      // 接收短消息
            Device->m_Receiver->ReceiveMsg(Param1, Param2);
            break;

        case MIM_ERROR:     // 若接收到无效的短消息
            Device->m_Receiver->OnError(Param1, Param2);
            break;

        case MIM_LONGDATA:  // 接收系统特定信息（长消息）
            if(Device->m_State == RECORDING)
            {
                MIDIHDR *MidiHdr = reinterpret_cast<MIDIHDR *>(Param1);
                Device->m_Receiver->ReceiveMsg(MidiHdr->lpData, 
                                               MidiHdr->dwBytesRecorded, 
                                               Param2);
                ::SetEvent(Device->m_Event);
            }
            break;

        case MIM_LONGERROR: // 若收到无效的长消息
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


// 用于处理头指针的线程函数
DWORD CMIDIInDevice::HeaderProc(LPVOID Parameter)
{
    CMIDIInDevice *Device; 
    
    Device = reinterpret_cast<CMIDIInDevice *>(Parameter);

    // 若MIDI输入设备处于记录状态则循环进行
    while(Device->m_State == RECORDING)
    {
        ::WaitForSingleObject(Device->m_Event, INFINITE);

        // 确保设备仍在记录
        if(Device->m_State == RECORDING)
        {
            // 移除已完成指针
            Device->m_HdrQueue.RemoveHeader();
        }
    }

    return 0;
}
