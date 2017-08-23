#ifndef SHORT_MSG_H
#define SHORT_MSG_H


#include "MIDIMsg.h"   
#include "StdAfx.h"


namespace midi
{
	const DWORD SHORT_MSG_LENGTH = 3;

	class CShortMsg : public CMIDIMsg
	{
	public:
		// 构造函数
		explicit CShortMsg(DWORD TimeStamp = 0);   
		CShortMsg(DWORD Msg, DWORD TimeStamp = 0);
		CShortMsg(unsigned char Status, unsigned char Data1,
			unsigned char Data2, DWORD TimeStamp);
		CShortMsg(unsigned char Command, unsigned char Channel,
			unsigned char Data1, unsigned char Data2,
			DWORD TimeStamp);

		// 发送信息
		void SendMsg(midi::CMIDIOutDevice &OutDevice);

		// 发送无状态字节的信息
		void SendMsgNoStatus(midi::CMIDIOutDevice &OutDevice);


		//函数
		DWORD GetLength() const
		{
			return midi::SHORT_MSG_LENGTH;
		}
		const char *GetMsg() const;
		unsigned char GetStatus() const;
		unsigned char GetChannel() const;
		unsigned char GetCommand() const;
		unsigned char GetData1() const;
		unsigned char GetData2() const;

		void SetMsg(unsigned char Status, unsigned char Data1,
			unsigned char Data2);
		void SetMsg(unsigned char Command, unsigned char Channel,
			unsigned char Data1, unsigned char Data2);

		//将消息打包or解压
		static DWORD PackShortMsg(unsigned char DataByte1,
			unsigned char DataByte2);

		static DWORD PackShortMsg(unsigned char Status,
			unsigned char DataByte1,
			unsigned char DataByte2);

		static DWORD PackShortMsg(unsigned char Command,
			unsigned char Channel,
			unsigned char DataByte1,
			unsigned char DataByte2);

		static void UnpackShortMsg(DWORD Msg, unsigned char &Status,
			unsigned char &DataByte1,
			unsigned char &DataByte2);

		static void UnpackShortMsg(DWORD Msg, unsigned char &Command,
			unsigned char &Channel,
			unsigned char &DataByte1,
			unsigned char &DataByte2);

	private:
		DWORD m_Msg;
		DWORD m_MsgNoStatus;
	};
}


#endif