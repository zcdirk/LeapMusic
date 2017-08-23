#pragma once
#include "Leap.h"
//#include "MusicPlayer.h"

using namespace std;
using namespace Leap;
//�����Ҫ�¼�������
class CPianoMelody
{
public:
	CPianoMelody();
	bool			UpdateData(const Frame &frame);//���µ�ǰ��ָ
	bool			OccurGesture(const KeyTapGesture &keyTap);
	bool			OccurGesture(SwipeGesture &swip);

private:
	bool			m_bFingersInited;//ȷ����ָ�Ѿ�������
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
//��ȡ��������Ļ����¼�����������
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


