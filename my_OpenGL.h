#include "stdafx.h"
#include "LeapMusicV2.h"
#include <stdlib.h>
#include <windows.h>		// windows��ͷ�ļ�
#include <stdio.h>			// ���������ͷ�ļ�
#include <gl\gl.h>			// OpenGL32��
#include <gl\glu.h>			// GLu32��
#include <gl\glaux.h>		// Glaux��
#include <GL/glut.h>
#include <stdio.h>

HDC			hDC = NULL;		// OpenGL��Ⱦ��������
HGLRC		hRC = NULL;		// ������ɫ�������
HWND		hWnd = NULL;		// ���洰�ڵľ��
HINSTANCE	hInstance;		// �������ʵ�����

bool	keys[256];			// ������̰���������
bool	active = TRUE;		// ���ڻ��־��ȱʡΪTURE��
bool	fullscreen = TRUE;	// ȫ����־��ȱʡΪȫ��ģʽ��
bool	light;				// �ƹ��־

GLfloat LightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[] = { 0.0f, 0.0f, 2.0f, 1.0f };

GLuint	filter;				// �������ĸ��˾�
GLuint	texture[3];			// �洢3������
GLuint base;

//��Բ��
void DrawCylinder(SStick *m_finger)
{
	glLineWidth(3);
	glBegin(GL_LINES);
	glVertex3f(m_finger->start_x, m_finger->start_y, m_finger->start_z);
	glVertex3f(m_finger->end_x, m_finger->end_y, m_finger->end_z);
	glEnd();
}

// ����һ��������ĳ�����   
void glDrawCube(GLfloat width, GLfloat height, GLfloat deep)
{
	glBegin(GL_QUADS);
	// ǰ��   
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, -height / 2, deep / 2);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, -height / 2, deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, deep / 2);
	// ����   
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-width / 2, -height / 2, -deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-width / 2, height / 2, -deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(width / 2, height / 2, -deep / 2);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(width / 2, -height / 2, -deep / 2);
	// ����   
	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-width / 2, height / 2, -deep / 2);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-width / 2, height / 2, deep / 2);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, height / 2, deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, -deep / 2);
	// ����   
	glNormal3f(0.0f, -1.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-width / 2, -height / 2, -deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(width / 2, -height / 2, -deep / 2);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(width / 2, -height / 2, deep / 2);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-width / 2, -height / 2, deep / 2);
	// ����   
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(width / 2, -height / 2, -deep / 2);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(width / 2, height / 2, -deep / 2);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(width / 2, height / 2, deep / 2);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(width / 2, -height / 2, deep / 2);
	// ����   
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

