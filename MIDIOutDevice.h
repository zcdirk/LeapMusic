#ifndef MIDI_OUT_DEVICE_H
#define MIDI_OUT_DEVICE_H

#pragma warning(disable:4786)

#include "stdafx.h"
#include <mmsystem.h>
#include <exception> 
#include <queue>

namespace midi
{
    // 压缩midiOutGetErrorText消息
    class CMIDIOutException : public std::exception
    {
    public:
        CMIDIOutException(MMRESULT ErrCode) throw()
        { ::midiOutGetErrorText(ErrCode, m_ErrMsg, sizeof m_ErrMsg); }

        const char *what() const throw() { return m_ErrMsg; }

    private:
        char m_ErrMsg[128];
    };


    // 若为CMIDIOutDevice对象分配内存失败则执行抛出异常 
    class CMIDIOutMemFailure : public std::bad_alloc
    {
    public:
        const char *what() const throw()
        { return "Memory allocation within a CMIDIOutDevice object "
                 "failed."; }
    };


    // Thrown when a CMIDIOutDevice is unable to create a signalling 
    // event
    class CMIDIOutEventFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "Unable to create a signalling event for "
                 "CMIDIOutDevice object."; }
    };


    // 若无法创建工作线程则执行抛出异常
    class CMIDIOutThreadFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "Unable to create worker thread for CMIDIOutDevice "
                 "object."; }
    };

    //此类用来代表MIDI输出设备
    class CMIDIOutDevice
    {
    public:
        // 构造函数（设备处于关闭状态）
        CMIDIOutDevice();

        // 构造函数（设备处于打开状态）
        CMIDIOutDevice(UINT DeviceId);

        // 析构函数
        ~CMIDIOutDevice();

        // 打开MIDI输出设备
        void Open(UINT DeviceId);

        // 关闭MIDI输出设备
        void Close();

        // 发送短消息
        void SendMsg(DWORD Msg);

        // 发送长消息
        void SendMsg(LPSTR Msg, DWORD MsgLength);

        // 若设备打开则返回true
        bool IsOpen() const;

        // 获取设备ID
        UINT GetDevID() const;

        // 获取MIDI输出设数目
        static UINT GetNumDevs() { return midiOutGetNumDevs(); }

        // 获取MIDI输入设备的容量
        static void GetDevCaps(UINT DeviceId, MIDIOUTCAPS &Caps);

    private:
        // 被禁止的赋值和复制
        CMIDIOutDevice(const CMIDIOutDevice &);
        CMIDIOutDevice &operator = (const CMIDIOutDevice &);

        // 创建事件以通知线程
        bool CreateEvent();

        // 若发生MIDI输出事件则调用此函数
        static void CALLBACK MidiOutProc(HMIDIOUT MidiOut, UINT Msg,
                                         DWORD_PTR Instance, DWORD_PTR Param1, 
                                         DWORD_PTR Param2);

        // 用于处理头指针的线程函数
        static DWORD WINAPI HeaderProc(LPVOID Parameter);

    private:
        // 为MIDI输出压缩MIDIHDR结构
        class CMIDIOutHeader
        {
        public:
            CMIDIOutHeader(HMIDIOUT DevHandle, LPSTR Msg, 
                           DWORD MsgLength);
            ~CMIDIOutHeader();

            void SendMsg();

        private:
            HMIDIOUT m_DevHandle;
            MIDIHDR  m_MIDIHdr;
        };


        // 用于存储CMIDIInHeader对象的线程安全队列
        class CHeaderQueue
        {
        public:
            CHeaderQueue();
            ~CHeaderQueue();

            void AddHeader(CMIDIOutHeader *Header);
            void RemoveHeader();
            void RemoveAll();
            bool IsEmpty();

        private:
            std::queue<CMIDIOutHeader *> m_HdrQueue;
            CRITICAL_SECTION m_CriticalSection;
        };

    private:
        HMIDIOUT       m_DevHandle;
        HANDLE         m_Event;
        CWinThread    *m_WorkerThread;
        CHeaderQueue   m_HdrQueue;
        enum State { CLOSED, OPENED } m_State;
    };
}


#endif
