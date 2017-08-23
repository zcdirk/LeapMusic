#include<stdafx.h>
#include "MIDIOutDevice.h"
#include "midi.h"


using midi::CMIDIOutDevice;
using midi::CMIDIOutException;

//构造函数
CMIDIOutDevice::CMIDIOutHeader::CMIDIOutHeader(HMIDIOUT DevHandle,
                                               LPSTR Msg, 
                                               DWORD MsgLength) :
m_DevHandle(DevHandle)
{
    // 初始化头指针
    m_MIDIHdr.lpData         = Msg;
    m_MIDIHdr.dwBufferLength = MsgLength;
    m_MIDIHdr.dwFlags        = 0;

    // 准备头指针
    MMRESULT Result = ::midiOutPrepareHeader(DevHandle, &m_MIDIHdr,
                                             sizeof m_MIDIHdr);

    // 若发生错误则执行抛出
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIOutException(Result);
    }
}


// 析构函数
CMIDIOutDevice::CMIDIOutHeader::~CMIDIOutHeader()
{
    ::midiOutUnprepareHeader(m_DevHandle, &m_MIDIHdr, 
                             sizeof m_MIDIHdr);
}


// 发送长消息
void CMIDIOutDevice::CMIDIOutHeader::SendMsg()
{
    MMRESULT Result = ::midiOutLongMsg(m_DevHandle, &m_MIDIHdr,
                                       sizeof m_MIDIHdr);

    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIOutException(Result);
    }
}

// 构造函数
CMIDIOutDevice::CHeaderQueue::CHeaderQueue()
{
    ::InitializeCriticalSection(&m_CriticalSection);
}


// 析构函数
CMIDIOutDevice::CHeaderQueue::~CHeaderQueue()
{
    RemoveAll();

    ::DeleteCriticalSection(&m_CriticalSection);
}


// 为队列添加头指针
void CMIDIOutDevice::CHeaderQueue::AddHeader(
                               CMIDIOutDevice::CMIDIOutHeader *Header)
{
    ::EnterCriticalSection(&m_CriticalSection);

    m_HdrQueue.push(Header);

    ::LeaveCriticalSection(&m_CriticalSection);
}


// 从队列中移除头指针
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


//清空头指针队列
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


// 判断队列是否为空
bool CMIDIOutDevice::CHeaderQueue::IsEmpty()
{
    bool Result;

    ::EnterCriticalSection(&m_CriticalSection);

    Result = m_HdrQueue.empty();

    ::LeaveCriticalSection(&m_CriticalSection);

    return Result;
}


// 构造函数（若MIDI输出设备处于关闭状态）
CMIDIOutDevice::CMIDIOutDevice() :
m_State(CLOSED)
{
    // 若无法创建事件则执行抛出异常
    if(!CreateEvent())
    {
        throw CMIDIOutEventFailure();
    }
}


// 构造函数（若MIDI输出设备处于打开状态）
CMIDIOutDevice::CMIDIOutDevice(UINT DeviceId) :
m_State(CLOSED)
{
    // 打开设备
    Open(DeviceId);

    // 若无法创建事件则执行抛出异常
    if(!CreateEvent())
    {
        Close();
        throw CMIDIOutEventFailure();
    }
}


// 析构函数
CMIDIOutDevice::~CMIDIOutDevice()
{
    // 关闭设备
    Close();

    // 关闭事件的消息句柄
    ::CloseHandle(m_Event);
}


// 打开MIDI输出设备
void CMIDIOutDevice::Open(UINT DeviceId)
{
    // 确保先前设备已被关闭
    Close();

    // 打开MIDI输出设备
    MMRESULT Result = ::midiOutOpen(&m_DevHandle, DeviceId, 
                                 reinterpret_cast<DWORD>(MidiOutProc),
                                 reinterpret_cast<DWORD>(this),
                                 CALLBACK_FUNCTION);

    // 若可以打开设备则将m_State变为OPENED状态
    if(Result == MMSYSERR_NOERROR)
    {
        m_State = OPENED;
        m_WorkerThread = AfxBeginThread(
            reinterpret_cast<AFX_THREADPROC>(HeaderProc), this);
    }
    // 若打开失败则执行抛出异常
    else
    {
        ::SetEvent(m_Event);
        ::WaitForSingleObject(m_WorkerThread->m_hThread, INFINITE);

        throw CMIDIOutException(Result);
    }
}


// 关闭MIDI输出设备
void CMIDIOutDevice::Close()
{
    // 若设备正在记录则在关闭前停止记录
    if(m_State == OPENED)
    {
        // 改变状态
        m_State = CLOSED;

        // 通知工作进程并等待进程结束
        ::SetEvent(m_Event);
        ::WaitForSingleObject(m_WorkerThread->m_hThread, INFINITE);

        // 清空头文件队列
        m_HdrQueue.RemoveAll();

        // 关闭MIDI输出设备
        ::midiOutClose(m_DevHandle);
    }
}


// 发送短消息
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


// 发送长消息
void CMIDIOutDevice::SendMsg(LPSTR Msg, DWORD MsgLength)
{
    if(m_State == OPENED)
    {  
        CMIDIOutHeader *Header;

        try
        {
            // 创建新的头指针以传递系统特定消息
            Header = new CMIDIOutHeader(m_DevHandle, Msg, MsgLength);
        }
        // 若内存分配失败则执行抛出异常
        catch(const std::bad_alloc &)
        {
            throw CMIDIOutMemFailure();
        }
        // 若不能生成头指针则执行抛出异常
        catch(const CMIDIOutException &)
        {
            throw;
        }

        try
        {
            // 传递系统特定消息
            Header->SendMsg();

            // 为队列添加头指针
            m_HdrQueue.AddHeader(Header);
        }
        // 若向系统发送特定消息失败则释放指针并执行抛出
        catch(const CMIDIOutException &)
        {
            delete Header;
            throw;
        }
    }
}


// 判断MIDI输出设备是否打开
bool CMIDIOutDevice::IsOpen() const
{
    return (m_State == OPENED);
}


// 获取设备ID
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


// 获取MIDI输出设备的容量
void CMIDIOutDevice::GetDevCaps(UINT DeviceId, MIDIOUTCAPS &Caps)
{
    MMRESULT Result = ::midiOutGetDevCaps(DeviceId, &Caps, 
                                          sizeof Caps);

    // 若无法检索设备容量则执行抛出异常
    if(Result != MMSYSERR_NOERROR)
    {
        throw CMIDIOutException(Result);
    }
}


// 创建事件以通知线程
bool CMIDIOutDevice::CreateEvent()
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

//若发生MIDI输出事件则调用此函数
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


// 用于处理头指针的线程函数
DWORD CMIDIOutDevice::HeaderProc(LPVOID Parameter)
{
    CMIDIOutDevice *Device; 
    
    Device = reinterpret_cast<CMIDIOutDevice *>(Parameter);

    // 若MIDI输出设备打开则循环进行
    while(Device->m_State == OPENED)
    {
        ::WaitForSingleObject(Device->m_Event, INFINITE);

        // 确保设备打开
        if(Device->m_State == OPENED)
        {
            // 移除已完成指针
            Device->m_HdrQueue.RemoveHeader();
        }
    }

    return 0;
}



