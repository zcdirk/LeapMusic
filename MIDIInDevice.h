#ifndef MIDI_IN_DEVICE_H
#define MIDI_IN_DEVICE_H

#pragma warning(disable:4786) 

#include "stdafx.h"
#include <mmsystem.h>
#include <exception> 
#include <queue>


namespace midi
{
	//抽象类（不准许实例化）接收从CMIDIInDevice对象发出的消息
    class CMIDIReceiver
    {
    public:
        virtual ~CMIDIReceiver() {}

        // 接收短消息
        virtual void ReceiveMsg(DWORD Msg, DWORD TimeStamp) = 0;

        // 接收长消息
        virtual void ReceiveMsg(LPSTR Msg, DWORD BytesRecorded,
                                DWORD TimeStamp) = 0;

        // 若收到无效短消息
        virtual void OnError(DWORD Msg, DWORD TimeStamp) = 0;

        // 若收到无效长消息
        virtual void OnError(LPSTR Msg, DWORD BytesRecorded,
                             DWORD TimeStamp) = 0;
    };


    // 压缩midiInGetErrorText消息
    class CMIDIInException : public std::exception
    {
    public:
        CMIDIInException(MMRESULT ErrCode) throw()
        { ::midiInGetErrorText(ErrCode, m_ErrMsg, sizeof m_ErrMsg); }

        const char *what() const throw() { return m_ErrMsg; }

    private:
        char m_ErrMsg[128];
    };


    // 若CMIDIInDevice对象的内存分配不成功则抛出 
    class CMIDIInMemFailure : public std::bad_alloc
    {
    public:
        const char *what() const throw()
        { return "CMIDIInDevice对象的内存分配失败 "
                 "失败。"; }
    };


    // 若CMIDIInDevice不能产生消息事件则抛出
    class CMIDIInEventFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "不能产生消息事件 "
                 "CMIDIInDevice对象"; }
    };


    // 当CMIDIInDevice无法产生工作线程则抛出
    class CMIDIInThreadFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "无法为CMIDIInDevice产生工作线程 "
                 "对象"; }
    };

	//此类代表MIDI输入设备
    class CMIDIInDevice
    {
    public:
        // 构造函数（若MIDI输入设备处于关闭状态）
        CMIDIInDevice();

        // 构造函数（若MIDI输入设备处于关闭状态）同时设置接收器
        CMIDIInDevice(CMIDIReceiver &Receiver);

        // 构造函数（若MIDI输入设备处于打开状态）
        CMIDIInDevice(UINT DeviceId, CMIDIReceiver &Receiver);

        // 析构函数
        ~CMIDIInDevice();

        // 打开MIDI输入设备
        void Open(UINT DeviceId);

        // 关闭MIDI输入设备
        void Close();

        //增加一缓冲区以存放特定消息
        void AddSysExBuffer(LPSTR Buffer, DWORD BufferLength);

        // 开始记录进程
        void StartRecording();

        // 停止记录进程
        void StopRecording();

        // 注册MIDI接收器并返回前一接收器
        CMIDIReceiver *SetReceiver(CMIDIReceiver &Receiver);

        // 若设备打开则返回true
        bool IsOpen() const;

        // 若设备正在记录则返回true
        bool IsRecording() const;

        // 获取设备ID
        UINT GetDevID() const;

        // 获取系统的MIDI输入设数目
        static UINT GetNumDevs() { return midiInGetNumDevs(); }

        // 获取MIDI输入设备的容量
        static void GetDevCaps(UINT DeviceId, MIDIINCAPS &Caps);

    private:
        // 被禁止的赋值和复制
        CMIDIInDevice(const CMIDIInDevice &);
        CMIDIInDevice &operator = (const CMIDIInDevice &);

        // 创建事件以通知线程
        bool CreateEvent();

        // 若发生MIDI输入事件则调用此函数
        static void CALLBACK MidiInProc(HMIDIIN MidiIn, UINT Msg,
                                        DWORD_PTR Instance, DWORD_PTR Param1, 
                                        DWORD_PTR Param2);

        // 用于处理头指针的线程函数
        static DWORD WINAPI HeaderProc(LPVOID Parameter);

    private:
        // 为MIDI输入压缩MIDIHDR结构
        class CMIDIInHeader
        {
        public:
            CMIDIInHeader(HMIDIIN DevHandle, LPSTR Buffer, 
                          DWORD BufferLength);
            ~CMIDIInHeader();

            // 为接收系统的特定消息添加缓区
            void AddSysExBuffer();

        private:
            HMIDIIN m_DevHandle;
            MIDIHDR m_MIDIHdr;
        };


        // 用于存储CMIDIInHeader对象的线程安全队列
        class CHeaderQueue
        {
        public:
            CHeaderQueue();
            ~CHeaderQueue();

            void AddHeader(CMIDIInHeader *Header);
            void RemoveHeader();
            void RemoveAll();
            bool IsEmpty();

        private:
            std::queue<CMIDIInHeader *> m_HdrQueue;
            CRITICAL_SECTION m_CriticalSection;
        };

    private:
        HMIDIIN        m_DevHandle;
        HANDLE         m_Event;
		CWinThread    *m_Thread;
        CMIDIReceiver *m_Receiver;
        CHeaderQueue   m_HdrQueue;
        enum State { CLOSED, OPENED, RECORDING } m_State;
    };
}


#endif
