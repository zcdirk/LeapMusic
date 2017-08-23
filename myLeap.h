#include "stdafx.h"
#include "LeapMusicV1.h"
#include <stdlib.h>
#include <windows.h>		// windows的头文件
#include <stdio.h>			// 输入输出的头文件
#include <Leap.h>
#include <LeapMath.h>

using namespace Leap;
using namespace std;
//extern CPianoMelody		*g_pianoMelody;
//extern vector<SStick>	g_StickVector;
static bool				g_bStartBackgroundMusic = true;
class PianoListener : public Listener {
public:
	virtual void onInit(const Controller&);
	virtual void onConnect(const Controller&);
	virtual void onDisconnect(const Controller&);
	virtual void onExit(const Controller&);
	virtual void onFrame(const Controller&);
	virtual void onFocusGained(const Controller&);
	virtual void onFocusLost(const Controller&);
};

void PianoListener::onInit(const Controller& controller) {
	std::cout << "Initialized" << std::endl;
}

void PianoListener::onConnect(const Controller& controller) {
	std::cout << "Connected" << std::endl;
	controller.enableGesture(Gesture::TYPE_CIRCLE);
	controller.enableGesture(Gesture::TYPE_KEY_TAP);
	controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
	controller.enableGesture(Gesture::TYPE_SWIPE);
}

void PianoListener::onDisconnect(const Controller& controller) {
	//Note: not dispatched when running in a debugger.
	std::cout << "Disconnected" << std::endl;
}

void PianoListener::onExit(const Controller& controller) {
	std::cout << "Exited" << std::endl;
}

void PianoListener::onFrame(const Controller& controller) {

}

void PianoListener::onFocusGained(const Controller& controller) {
	std::cout << "Focus Gained" << std::endl;
}

void PianoListener::onFocusLost(const Controller& controller) {
	std::cout << "Focus Lost" << std::endl;
}
