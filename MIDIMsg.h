#ifndef MIDI_MSG_H
#define MIDI_MSG_H


#include "stdafx.h"    // For DWORD data type

//声明
namespace midi
{
	class CMIDIOutDevice;
}


namespace midi
{
	//所有MIDI消息类的基类
	class CMIDIMsg
	{
	public:
		virtual ~CMIDIMsg() {}

		// 发送MIDI消息
		virtual void SendMsg(midi::CMIDIOutDevice &OutDevice) = 0;

		// 获取MIDI消息长度
		virtual DWORD GetLength() const = 0;

		// 获取MIDI消息
		virtual const char *GetMsg() const = 0;

		// 获取或设置时间标识
		DWORD GetTimeStamp() const { return m_TimeStamp; }
		void SetTimeStamp(DWORD TimeStamp) { m_TimeStamp = TimeStamp; }

	private:
		DWORD m_TimeStamp;
	};
}


#endif