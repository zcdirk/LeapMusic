#include "stdafx.h"
#include "LeapMusicV2.h"
#include <stdlib.h>
#include <windows.h>		// windows的头文件
#include <stdio.h>			// 输入输出的头文件
#include <gl\gl.h>			// OpenGL32库
#include <gl\glu.h>			// GLu32库
#include <gl\glaux.h>		// Glaux库
#include <GL/glut.h>
#include <stdio.h>

HDC			hDC = NULL;		// OpenGL渲染描述表句柄
HGLRC		hRC = NULL;		// 窗口着色描述句柄
HWND		hWnd = NULL;		// 保存窗口的句柄
HINSTANCE	hInstance;		// 保存程序实例句柄

bool	keys[256];			// 保存键盘按键的数组
bool	active = TRUE;		// 窗口活动标志（缺省为TURE）
bool	fullscreen = TRUE;	// 全屏标志（缺省为全屏模式）
bool	light;				// 灯光标志

GLfloat LightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[] = { 0.0f, 0.0f, 2.0f, 1.0f };

GLuint	filter;				// 决定用哪个滤镜
GLuint	texture[3];			// 存储3个纹理
GLuint base;

//画圆柱
void DrawCylinder(SStick *m_finger)
{
	glLineWidth(3);
	glBegin(GL_LINES);
	glVertex3f(m_finger->start_x, m_finger->start_y, m_finger->start_z);
	glVertex3f(m_finger->end_x, m_finger->end_y, m_finger->end_z);
	glEnd();
}

// 绘制一个带纹理的长方体   
void glDrawCube(GLfloat width, GLfloat height, GLfloat deep)
{
	glBegin(GL_QUADS);
	// 前面   
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, -height / 2, deep / 2);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, -height / 2, deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, deep / 2);
	// 后面   
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-width / 2, -height / 2, -deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-width / 2, height / 2, -deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(width / 2, height / 2, -deep / 2);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(width / 2, -height / 2, -deep / 2);
	// 顶面   
	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, -deep / 2);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, height / 2, deep / 2);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, height / 2, deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, -deep / 2);
	// 底面   
	glNormal3f(0.0f, -1.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-width / 2, -height / 2, -deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(width / 2, -height / 2, -deep / 2);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(width / 2, -height / 2, deep / 2);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-width / 2, -height / 2, deep / 2);
	// 右面   
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, -height / 2, -deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, -deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(width / 2, height / 2, deep / 2);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(width / 2, -height / 2, deep / 2);
	// 左面   
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, -height / 2, -deep / 2);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-width / 2, -height / 2, deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-width / 2, height / 2, deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, -deep / 2);
	glEnd();
}

GLvoid BuildFont(GLvoid)								// Build Our Bitmap Font
{
	HFONT	font;										// Windows Font ID
	HFONT	oldfont;									// Used For Good House Keeping

	base = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(-24,							// Height Of Font
		0,								// Width Of Font
		0,								// Angle Of Escapement
		0,								// Orientation Angle
		FW_BOLD,						// Font Weight
		FALSE,							// Italic
		FALSE,							// Underline
		FALSE,							// Strikeout
		ANSI_CHARSET,					// Character Set Identifier
		OUT_TT_PRECIS,					// Output Precision
		CLIP_DEFAULT_PRECIS,			// Clipping Precision
		ANTIALIASED_QUALITY,			// Output Quality
		FF_DONTCARE | DEFAULT_PITCH,		// Family And Pitch
		"Courier New");					// Font Name

	oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, base);				// Builds 96 Characters Starting At Character 32
	SelectObject(hDC, oldfont);							// Selects The Font We Want
	DeleteObject(font);									// Delete The Font
}

