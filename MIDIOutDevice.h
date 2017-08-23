#ifndef MIDI_OUT_DEVICE_H
#define MIDI_OUT_DEVICE_H

#pragma warning(disable:4786)

#include "stdafx.h"
#include <mmsystem.h>
#include <exception> 
#include <queue>

namespace midi
{
    // ѹ��midiOutGetErrorText��Ϣ
    class CMIDIOutException : public std::exception
    {
    public:
        CMIDIOutException(MMRESULT ErrCode) throw()
        { ::midiOutGetErrorText(ErrCode, m_ErrMsg, sizeof m_ErrMsg); }

        const char *what() const throw() { return m_ErrMsg; }

    private:
        char m_ErrMsg[128];
    };


    // ��ΪCMIDIOutDevice��������ڴ�ʧ����ִ���׳��쳣 
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


    // ���޷����������߳���ִ���׳��쳣
    class CMIDIOutThreadFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "Unable to create worker thread for CMIDIOutDevice "
                 "object."; }
    };

    //������������MIDI����豸
    class CMIDIOutDevice
    {
    public:
        // ���캯�����豸���ڹر�״̬��
        CMIDIOutDevice();

        // ���캯�����豸���ڴ�״̬��
        CMIDIOutDevice(UINT DeviceId);

        // ��������
        ~CMIDIOutDevice();

        // ��MIDI����豸
        void Open(UINT DeviceId);

        // �ر�MIDI����豸
        void Close();

        // ���Ͷ���Ϣ
        void SendMsg(DWORD Msg);

        // ���ͳ���Ϣ
        void SendMsg(LPSTR Msg, DWORD MsgLength);

        // ���豸���򷵻�true
        bool IsOpen() const;

        // ��ȡ�豸ID
        UINT GetDevID() const;

        // ��ȡMIDI�������Ŀ
        static UINT GetNumDevs() { return midiOutGetNumDevs(); }

        // ��ȡMIDI�����豸������
        static void GetDevCaps(UINT DeviceId, MIDIOUTCAPS &Caps);

    private:
        // ����ֹ�ĸ�ֵ�͸���
        CMIDIOutDevice(const CMIDIOutDevice &);
        CMIDIOutDevice &operator = (const CMIDIOutDevice &);

        // �����¼���֪ͨ�߳�
        bool CreateEvent();

        // ������MIDI����¼�����ô˺���
        static void CALLBACK MidiOutProc(HMIDIOUT MidiOut, UINT Msg,
                                         DWORD_PTR Instance, DWORD_PTR Param1, 
                                         DWORD_PTR Param2);

        // ���ڴ���ͷָ����̺߳���
        static DWORD WINAPI HeaderProc(LPVOID Parameter);

    private:
        // ΪMIDI���ѹ��MIDIHDR�ṹ
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


        // ���ڴ洢CMIDIInHeader������̰߳�ȫ����
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
