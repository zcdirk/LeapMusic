#pragma once

#include "resource.h"

#define MAX_TEXTURE 3

using namespace std;

#include <vector>

//为手掌建立的结构体
struct SStick
{
	float start_x;
	float start_y;
	float start_z;
	float end_x;
	float end_y;
	float end_z;
};

CRITICAL_SECTION g_csStick;   
vector<SStick> g_StickVector;
CRITICAL_SECTION g_csMusic;