#include<stdafx.h>
#include<Leap.h>
#include<LeapMusicV2.h>
#include<windows.h>

using namespace std;
using namespace Leap;

enum M_Status{Initialized,Connected,Disconnected,Exited,FocusGained,FocusLost};

//��������
class MusicListener : public Listener{
public:
	virtual void onInit(const Controller&);
	virtual void onConnect(const Controller&);
	virtual void onDisconnect(const Controller&);
	virtual void onExit(const Controller&);
	virtual void onFrame(const Controller&);
	virtual void onFocusGained(const Controller&);
	virtual void onFocusLost(const Controller&);
	M_Status m_status;

};

//��ʼ��
void MusicListener::onInit(const Controller& controller) {
	m_status=Initialized;
}

//���Ӻ����������������ƣ�
void MusicListener::onConnect(const Controller& controller) {
	m_status = Connected;
	controller.enableGesture(Gesture::TYPE_CIRCLE);
	controller.enableGesture(Gesture::TYPE_KEY_TAP);
	controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
	controller.enableGesture(Gesture::TYPE_SWIPE);
}

//�Ͽ�����
void MusicListener::onDisconnect(const Controller& controller) {
	m_status = Disconnected;
}

//�˳�����
void MusicListener::onExit(const Controller& controller) {
	m_status = Exited;
}

//��ȡ��Ϣ����
void MusicListener::onFrame(const Controller& controller) {
	// ��ȡ���µ�һ֡�����ҷ���һЩ������Ϣ
	const Frame frame = controller.frame();

	//��OpenGL����ʾ��
	EnterCriticalSection(&g_csStick);
	g_StickVector.clear();
	for (int i = 0; i<frame.fingers().count(); ++i)
	{
		SStick m_finger;
		m_finger.start_x = frame.fingers()[i].hand().palmPosition().x;
		m_finger.start_y = frame.fingers()[i].hand().palmPosition().y;
		m_finger.start_z = frame.fingers()[i].hand().palmPosition().z;
		m_finger.end_x = frame.fingers()[i].tipPosition().x;
		m_finger.end_y = frame.fingers()[i].tipPosition().y;
		m_finger.end_z = frame.fingers()[i].tipPosition().z;
		g_StickVector.push_back(m_finger);
	}
	LeaveCriticalSection(&g_csStick);

	// ��ȡ����
	const GestureList gestures = frame.gestures();
	for (int g = 0; g < gestures.count(); ++g) {
		Gesture gesture = gestures[g];
		switch (gesture.type()) {
		case Gesture::TYPE_SWIPE:
		{
			SwipeGesture swipe = gesture;
			break;
		}
		case Gesture::TYPE_KEY_TAP:
		{
			KeyTapGesture tap = gesture;
			EnterCriticalSection(&g_csMusic);
			//bool flag = g_pianoMelody->OccurGesture(tap);
			LeaveCriticalSection(&g_csMusic);
			break;
		}
		case Gesture::TYPE_SCREEN_TAP:
		{
			ScreenTapGesture screentap = gesture;
			break;
		}
		default:
			break;
		}
	}

	if (!frame.hands().isEmpty() || !gestures.isEmpty()) {
	}
}

void MusicListener::onFocusGained(const Controller& controller) {
	m_status = FocusGained;
}

void MusicListener::onFocusLost(const Controller& controller) {
	m_status = FocusLost;
}
