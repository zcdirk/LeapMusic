#ifndef MIDI_IN_DEVICE_H
#define MIDI_IN_DEVICE_H

#pragma warning(disable:4786) 

#include "stdafx.h"
#include <mmsystem.h>
#include <exception> 
#include <queue>


namespace midi
{
	//�����ࣨ��׼��ʵ���������մ�CMIDIInDevice���󷢳�����Ϣ
    class CMIDIReceiver
    {
    public:
        virtual ~CMIDIReceiver() {}

        // ���ն���Ϣ
        virtual void ReceiveMsg(DWORD Msg, DWORD TimeStamp) = 0;

        // ���ճ���Ϣ
        virtual void ReceiveMsg(LPSTR Msg, DWORD BytesRecorded,
                                DWORD TimeStamp) = 0;

        // ���յ���Ч����Ϣ
        virtual void OnError(DWORD Msg, DWORD TimeStamp) = 0;

        // ���յ���Ч����Ϣ
        virtual void OnError(LPSTR Msg, DWORD BytesRecorded,
                             DWORD TimeStamp) = 0;
    };


    // ѹ��midiInGetErrorText��Ϣ
    class CMIDIInException : public std::exception
    {
    public:
        CMIDIInException(MMRESULT ErrCode) throw()
        { ::midiInGetErrorText(ErrCode, m_ErrMsg, sizeof m_ErrMsg); }

        const char *what() const throw() { return m_ErrMsg; }

    private:
        char m_ErrMsg[128];
    };


    // ��CMIDIInDevice������ڴ���䲻�ɹ����׳� 
    class CMIDIInMemFailure : public std::bad_alloc
    {
    public:
        const char *what() const throw()
        { return "CMIDIInDevice������ڴ����ʧ�� "
                 "ʧ�ܡ�"; }
    };


    // ��CMIDIInDevice���ܲ�����Ϣ�¼����׳�
    class CMIDIInEventFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "���ܲ�����Ϣ�¼� "
                 "CMIDIInDevice����"; }
    };


    // ��CMIDIInDevice�޷����������߳����׳�
    class CMIDIInThreadFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "�޷�ΪCMIDIInDevice���������߳� "
                 "����"; }
    };

	//�������MIDI�����豸
    class CMIDIInDevice
    {
    public:
        // ���캯������MIDI�����豸���ڹر�״̬��
        CMIDIInDevice();

        // ���캯������MIDI�����豸���ڹر�״̬��ͬʱ���ý�����
        CMIDIInDevice(CMIDIReceiver &Receiver);

        // ���캯������MIDI�����豸���ڴ�״̬��
        CMIDIInDevice(UINT DeviceId, CMIDIReceiver &Receiver);

        // ��������
        ~CMIDIInDevice();

        // ��MIDI�����豸
        void Open(UINT DeviceId);

        // �ر�MIDI�����豸
        void Close();

        //����һ�������Դ���ض���Ϣ
        void AddSysExBuffer(LPSTR Buffer, DWORD BufferLength);

        // ��ʼ��¼����
        void StartRecording();

        // ֹͣ��¼����
        void StopRecording();

        // ע��MIDI������������ǰһ������
        CMIDIReceiver *SetReceiver(CMIDIReceiver &Receiver);

        // ���豸���򷵻�true
        bool IsOpen() const;

        // ���豸���ڼ�¼�򷵻�true
        bool IsRecording() const;

        // ��ȡ�豸ID
        UINT GetDevID() const;

        // ��ȡϵͳ��MIDI��������Ŀ
        static UINT GetNumDevs() { return midiInGetNumDevs(); }

        // ��ȡMIDI�����豸������
        static void GetDevCaps(UINT DeviceId, MIDIINCAPS &Caps);

    private:
        // ����ֹ�ĸ�ֵ�͸���
        CMIDIInDevice(const CMIDIInDevice &);
        CMIDIInDevice &operator = (const CMIDIInDevice &);

        // �����¼���֪ͨ�߳�
        bool CreateEvent();

        // ������MIDI�����¼�����ô˺���
        static void CALLBACK MidiInProc(HMIDIIN MidiIn, UINT Msg,
                                        DWORD_PTR Instance, DWORD_PTR Param1, 
                                        DWORD_PTR Param2);

        // ���ڴ���ͷָ����̺߳���
        static DWORD WINAPI HeaderProc(LPVOID Parameter);

    private:
        // ΪMIDI����ѹ��MIDIHDR�ṹ
        class CMIDIInHeader
        {
        public:
            CMIDIInHeader(HMIDIIN DevHandle, LPSTR Buffer, 
                          DWORD BufferLength);
            ~CMIDIInHeader();

            // Ϊ����ϵͳ���ض���Ϣ��ӻ���
            void AddSysExBuffer();

        private:
            HMIDIIN m_DevHandle;
            MIDIHDR m_MIDIHdr;
        };


        // ���ڴ洢CMIDIInHeader������̰߳�ȫ����
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
