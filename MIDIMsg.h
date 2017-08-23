#ifndef MIDI_MSG_H
#define MIDI_MSG_H


#include "stdafx.h"    // For DWORD data type

//����
namespace midi
{
	class CMIDIOutDevice;
}


namespace midi
{
	//����MIDI��Ϣ��Ļ���
	class CMIDIMsg
	{
	public:
		virtual ~CMIDIMsg() {}

		// ����MIDI��Ϣ
		virtual void SendMsg(midi::CMIDIOutDevice &OutDevice) = 0;

		// ��ȡMIDI��Ϣ����
		virtual DWORD GetLength() const = 0;

		// ��ȡMIDI��Ϣ
		virtual const char *GetMsg() const = 0;

		// ��ȡ������ʱ���ʶ
		DWORD GetTimeStamp() const { return m_TimeStamp; }
		void SetTimeStamp(DWORD TimeStamp) { m_TimeStamp = TimeStamp; }

	private:
		DWORD m_TimeStamp;
	};
}


#endif