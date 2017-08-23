#pragma once
#include "Leap.h"
//#include "MusicPlayer.h"

using namespace std;
using namespace Leap;
//添加主要事件处理类
class CPianoMelody
{
public:
	CPianoMelody();
	bool			UpdateData(const Frame &frame);//更新当前手指
	bool			OccurGesture(const KeyTapGesture &keyTap);
	bool			OccurGesture(SwipeGesture &swip);

private:
	bool			m_bFingersInited;//确认手指已经更新了
	int				m_currentFinger;
	int				m_currentPianoArea;
	bool			m_keyTapOccured;
	bool			m_swipOccured;
	FingerList		m_fullFingerList;
};


CPianoMelody::CPianoMelody()
{
	m_bFingersInited = false;
	m_currentFinger = -1;
	m_keyTapOccured = false;
	m_currentPianoArea = -1;
}
bool	CPianoMelody::UpdateData(const Frame &frame)
{
	FingerList fingerList = frame.fingers();
	return true;
}
//截取发生的屏幕点击事件，进行演奏
bool	CPianoMelody::OccurGesture(const KeyTapGesture &keyTap)
{
	Pointable pt = keyTap.pointable();
	Vector v = pt.tipVelocity();
	Vector pos = pt.tipPosition();
	int xPos = pos.x;
	if (xPos <= -200)
	{
		xPos = -200;
	}
	else if (xPos >= 200)
	{
		xPos = 200;
	}
	int MyFingerDown = (pos.x + 200) / 50;

	return false;
}

CPianoMelody	*g_pianoMelody;


