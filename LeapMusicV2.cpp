// LeapMusic a music performance system
// Created by Chi Zhang
//

#include "stdafx.h"
#include "LeapMusicV2.h"
#include <windows.h>
#include <my_OpenGL.h>
#include <m_leap.h>
#include <m_MIDI.h>        
#include <m_music.h>
#include <m_record.h>
#include <fstream>

using namespace std;
using namespace midi;

static int m_tune = 0;                  //���ڿ�������
static bool key_keep[256] = { true };   //���ڼ�¼�����Ƿ񱻰���

static int MyFingerDown = 0;           //���ڼ�¼LeapMotion�е���ָ���µ�λ��
static int MyFingerUp = 0;           //���ڼ�¼LeapMotion�е���ָ����̧���λ��
static char *m_lrc[256];                         //���ڴ�Ÿ��
static int lrc_numb=0;                 //���ڼ�¼�����Ŀ

GLfloat	cnt1=1.0f;				// 1st Counter Used To Move Text & For Coloring
GLfloat	cnt2=1.2f;				// 2nd Counter Used To Move Text & For Coloring

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// WndProc������


GLvoid KillFont(GLvoid)									// Delete The Font List
{
	glDeleteLists(base, 96);							// Delete All 96 Characters
}

GLvoid glPrint(static char *fmt, ...)					// Custom GL "Print" Routine
{
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf_s(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(base - 32);								// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}


AUX_RGBImageRec *LoadBMP(char *Filename)				// ����ͼ��
{
	FILE *File = NULL;									// �ļ����

	if (!Filename)										// ȷ���ļ����ѱ�����
	{
		return NULL;									// ��δָ���ļ����򷵻�NULL
	}

	// ����ļ��Ƿ����
	fopen_s(&File, Filename, "r");							
	if (File)							 
	{
		fclose(File);									// �رվ��
		return auxDIBImageLoad(Filename);				// ����ͼ�񲢷���ָ��
	}

	return NULL;										// ������ʧ���򷵻�NULL
}

int LoadGLTextures()									// ����ͼ��ת��������
{
	BOOL Status = FALSE;									// ״̬��ʾ��

	AUX_RGBImageRec *TextureImage[MAX_TEXTURE];                  // Ϊ�����ٴ洢�ռ�   

	memset(TextureImage, 0, sizeof(void *) * MAX_TEXTURE);           	// ��ָ����ΪNULL

	if ((TextureImage[0] = LoadBMP("Resources/white.bmp"))
		&& (TextureImage[1] = LoadBMP("Resources/black.bmp"))
		&& (TextureImage[2] = LoadBMP("Resources/piano.bmp")))
	{
		Status = TRUE;                                           // ����״̬����ΪTRUE   
		glGenTextures(MAX_TEXTURE, &texture[0]);                 // ����Ψһ��������������ʶ����,������texture��   

		// ��ͼƬ���ݲ�������   
		for (int loop = 0; loop<MAX_TEXTURE; loop++)                // ѭ������MAX_TEXTURE������   
		{
			glBindTexture(GL_TEXTURE_2D, texture[loop]);
			glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[loop]->sizeX, TextureImage[loop]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[loop]->data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
	for (int loop = 0; loop<MAX_TEXTURE; loop++)
	{
		if (TextureImage[loop])
		{
			if (TextureImage[loop]->data)        // ����������   
			{
				free(TextureImage[loop]->data);  // �ͷ�����洢�ռ�   
			}
			free(TextureImage[loop]);
		}
	}
	return Status;                                // ����State   
}

// ����OpenGL���ڴ�С
GLvoid ReSizeGLScene(GLsizei width, GLsizei height)
{
	if (height == 0)										// �жϸ߶��Ƿ�Ϊ0����ֹ����0��
	{
		height = 1;										// �������㣬���߶�����Ϊ1
	}

	glViewport(0, 0, width, height);						// ��OpenGL�����ߴ����ó����ڴ��ڴ�С

	glMatrixMode(GL_PROJECTION);						// ѡ��ͶӰ����
	glLoadIdentity();									// ����ͶӰ����

	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);     // �����ӿڴ�С

	glMatrixMode(GL_MODELVIEW);							// ѡ��ģ�͹۲����
	glLoadIdentity();									// ����ģ�͹۲����
}

// ��ʼ��OpenGL��������
int InitGL(GLvoid)
{
	//����Ƿ��Ѽ���������δ�����򷵻�FALSE
	if (!LoadGLTextures())								
	{
		return FALSE;									
	}

	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);							// ������Ӱƽ��
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// ���ñ�����ɫ���˴�Ϊ��ɫ��
	glClearDepth(1.0f);									// ������Ȼ���
	glEnable(GL_DEPTH_TEST);							// ������Ȳ���
	glDepthFunc(GL_LEQUAL);								// ������Ȳ��Ե�����

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// ��͸�ӽ����������˴�Ϊnicest������GL_FASTEST��GL_DONT_CARE��
	BuildFont();										// Build The Font

	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);		// ���û��ƹ�
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);		// ����ɢ���
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);	// ���ù�Դ
	glEnable(GL_LIGHT1);								// �����Դ

	m_lrc[0] = " ";
	m_lrc[1] = " ";
	m_lrc[2] = " ";
	m_lrc[3] = " ";
	return TRUE;										// ��ʼ��OK
}

void addlrc(char *note)
{
	ofstream outfile("lrc.txt",ios::app); 
	outfile << note << "  ";
	outfile.close();
	lrc_numb++;
	if (lrc_numb == 1)
	{
		m_lrc[0] = note;
		m_lrc[1] = " ";
		m_lrc[2] = " ";
		m_lrc[3] = " ";
	}
	else if (lrc_numb == 2)
		m_lrc[1] = note;
	else if (lrc_numb == 3)
		m_lrc[2] = note;
	else if (lrc_numb == 4)
	{
		m_lrc[3] = note;
		lrc_numb = 0;
	}
}

// �˴���ʼ��ʽ����ͼ��
int DrawGLScene(GLvoid)
{

	//glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// �����������Ȼ���   
	glLoadIdentity();                                   // ���õ�ǰ����   
	// �ڴ˴���Ӵ�����л���:
	glTranslatef(0.0f, 0.4f, -2.0f);
	glRotatef(30, 1.0f, 0.0f, 0.0f);

	//���ٲ���
	glColor3f(1, 1, 1);
	// �����һ����߸���ɫ����
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glPushMatrix();
	glTranslatef(-0.8f, -0.5f, 0.0f);

	// ��һ����ɫ����   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['Z'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// �ڶ�����ɫ����   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['X'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// ��������ɫ����   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['C'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// ���ĸ���ɫ����   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['V'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// �������ɫ����   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['B'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// ��������ɫ����   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['N'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// ���߸���ɫ����   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['M'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	glPopMatrix();                      // ��ɫ�����������   

	//���ٵ���������
	//glBindTexture(GL_TEXTURE_2D, texture[2]);
	glColor3f(0.75, 0.55, 0.35);
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -0.8f);
	glTranslatef(0.0f, -0.65f, 0.0f);
	glDrawCube(1.8f, 0.6f, 0.3f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.75f, -0.8f, 0.0f);
	glDrawCube(0.1f, 0.9f, 0.8f);
	glTranslatef(1.5f, 0.0f, 0.0f);
	glDrawCube(0.1f, 0.9f, 0.8f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -1.3f, -0.4f);
	glDrawCube(1.8f, 0.1f, 1.7f);
	glPopMatrix();

	//���ƺڼ�
	glColor3f(1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glPushMatrix();
	glTranslatef(-0.7f, -0.46f, -0.2f);

	//��һ���ڼ�
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['S'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	//�ڶ����ڼ�
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['D'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	//�������ڼ�
	glTranslatef(0.4f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['G'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	//���ĸ��ڼ�
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['H'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	//������ڼ�
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['J'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	glPopMatrix();


	glTranslatef(-0.4f, 0.0f, 0.0f);						// Move One Unit Into The Screen
	// Pulsing Colors Based On Text Position
	glColor3f(1.0f, 1.0f, 1.0f);
	for (int k = 0; k < 4; k++)
	{ 
		// Position The Text On The Screen
		glRasterPos2f(-0.2f + 0.05f*float(cos(cnt1))+k*0.4, 0.32f*float(sin(cnt2)));
		glPrint(m_lrc[k], cnt1);	// Print GL Text To The Screen
	}

	//��������
	SStick *m_finger;
	GLUquadricObj *qobj = gluNewQuadric();
	EnterCriticalSection(&g_csStick);
	for (int i = 0; i < g_StickVector.size(); ++i)
	{
		m_finger = &g_StickVector[i];

		//��ָ
		glColor3f(0, 1, 1);
		glPushMatrix();
		DrawCylinder(m_finger);
		glPopMatrix();

		//��ָ��(��)
		glColor3f(1, 0, 0);
		glPushMatrix();
		glTranslatef(m_finger->end_x, m_finger->end_y, m_finger->end_z);
		gluSphere(qobj, 0.003, 100, 100);
		glPopMatrix();
	}
	LeaveCriticalSection(&g_csStick);

	return TRUE;										// һ������
}

// �������ٴ���
GLvoid KillGLWindow(GLvoid)
{
	if (fullscreen)										// �ж��Ƿ���ȫ��ģʽ
	{
		ChangeDisplaySettings(NULL, 0);					// ���ǣ��ص�����
		ShowCursor(TRUE);								// ��ʾ���
	}

	if (hRC)											// �ж��Ƿ���OpenGL��Ⱦ������
	{
		if (!wglMakeCurrent(NULL, NULL))					// �ܷ��ͷ�DC��RC
		{
			MessageBox(NULL, "�ͷ�DC��RCʧ��", "�رմ���", MB_OK | MB_ICONINFORMATION);     //�������ͷ��򵯳�����
		}

		if (!wglDeleteContext(hRC))						// �ܷ�ɾ��RC��
		{
			MessageBox(NULL, "�ͷ�RCʧ��", "�رմ���", MB_OK | MB_ICONINFORMATION);
		}
		hRC = NULL;										// ��RC��ΪNULL
	}

	if (hDC && !ReleaseDC(hWnd, hDC))					// �ܷ��ͷ�DC��
	{
		MessageBox(NULL, "�ͷ�DCʧ��", "�رմ���", MB_OK | MB_ICONINFORMATION);
		hDC = NULL;										// ��DC��ΪNULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// �ܷ����ٴ��ڣ�
	{
		MessageBox(NULL, "�ͷŴ��ھ��ʧ��", "�رմ���", MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;										// �����ھ��hWnd��ΪNULL
	}

	if (!UnregisterClass("OpenGL", hInstance))			// �ܷ�ע���ࣿ
	{
		MessageBox(NULL, "����ע��������", "�رմ���", MB_OK | MB_ICONINFORMATION);
		hInstance = NULL;									// ��hInstance��ΪNULL
	}
	KillFont();
}

//����OpenGL���ڣ����ز���������������������Ϊ�����⡢���ߡ�ɫ��λ����ȫ����־
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// �����������ƥ�䣨���ظ�ʽ���Ľ��
	WNDCLASS	wc;						// ���洰����ṹ
	DWORD		dwExStyle;				// �����չ���ڷ��
	DWORD		dwStyle;				// ��Ŵ��ڷ��
	RECT		WindowRect;				// ȡ�þ������ϽǺ����½ǵ�����ֵ
	WindowRect.left = (long)0;			// ������ֵ��Ϊ0
	WindowRect.right = (long)width;		// ������ֵ��ΪҪ����
	WindowRect.top = (long)0;				// ������ֵ��Ϊ0
	WindowRect.bottom = (long)height;		// ���׶�ֵ��ΪҪ��߶�

	fullscreen = fullscreenflag;			// ����ȫ��ȫ����־

	hInstance = GetModuleHandle(NULL);				// ȡ�ô���ʵ��
	wc.style = CS_HREDRAW | CS_HREDRAW | CS_OWNDC;	// CS_HREDRAW��ˮƽ����仯ʱ�ػ���CS_HREDRAW����ֱ����仯ʱ�ػ���CS_OWNDC��Ϊ���ڴ���˽�е�DC
	wc.lpfnWndProc = (WNDPROC)WndProc;					// WndProc������Ϣ
	wc.cbClsExtra = 0;									// �޶��ⴰ������
	wc.cbWndExtra = 0;									// �޶��ⴰ������
	wc.hInstance = hInstance;							// ����ʵ��
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);			// װ��ȱʡͼ��
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);			// װ��ȱʡ���ָ��
	wc.hbrBackground = NULL;									// GL����Ϊ��
	wc.lpszMenuName = NULL;									// �˵�Ϊ��
	wc.lpszClassName = "OpenGL";								// ��������

	if (!RegisterClass(&wc))									// ����ע�ᴰ����
	{
		MessageBox(NULL, "ע�ᴰ����ʧ��", "����", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;											// ����FALSE
	}

	if (fullscreen)												// �Ƿ�Ϊȫ��ģʽ��
	{
		DEVMODE dmScreenSettings;								// �����豸ģʽ
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));	// ȷ���ڴ����
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);		// �����豸ģʽ�Ĵ�С
		dmScreenSettings.dmPelsWidth = width;				// ���Ϊ��Ļ���
		dmScreenSettings.dmPelsHeight = height;				// �߶�Ϊ��Ļ�߶�
		dmScreenSettings.dmBitsPerPel = bits;					// ����ÿ���ض�Ӧ��ɫ�����
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// ��������״̬ģʽ�����ؽ����  ע��CDS_FULLSCREEN��ȥ��״̬��
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			// ��ģʽ����ʧ�ܣ��ṩ����ѡ��˳����ڴ���������
			if (MessageBox(NULL, "ȫ��ģʽ�ڵ�ǰ�Կ�������ʧ�ܣ�\n�Ƿ�ʹ�ô���ģʽ��", "Leap Music", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			{
				fullscreen = FALSE;		// ���ô���ģʽ������ȫ�������У�
			}
			else
			{
				// �������ڸ�֪�û����򼴽��ر�
				MessageBox(NULL, "���򽫱��ر�", "����", MB_OK | MB_ICONSTOP);
				return FALSE;									// �˳�������FALSE
			}
		}
	}

	if (fullscreen)												// ����Ƿ��Դ���ȫ��ģʽ
	{
		dwExStyle = WS_EX_APPWINDOW;								// ������չ������
		dwStyle = WS_POPUP;										// ���ô�����
		ShowCursor(FALSE);										// �趨�Ƿ���ʾ���ָ�루���ﲻ��ʾ��
	}
	else                                                         //������ȫ��ģʽ
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// ������չ������
		dwStyle = WS_OVERLAPPEDWINDOW;							// ���ô�����
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// ��������������Ҫ��Ĵ�С

	// �������ڲ���ⴰ���Ƿ񴴽��ɹ�
	if (!(hWnd = CreateWindowEx            //����Ϊ��CreateWindowEx�������ݵı���
		(dwExStyle,							// ��չ������
		"OpenGL",							// ����
		title,								// ���ڱ���
		dwStyle |							// ��ѡ��Ĵ�����
		WS_CLIPSIBLINGS |					// ����Ĵ�����
		WS_CLIPCHILDREN,					// ����Ĵ�����
		0, 0,								// ����λ��
		WindowRect.right - WindowRect.left,	// ���ڿ�ȣ�������
		WindowRect.bottom - WindowRect.top,	// ���ڸ߶ȣ�������
		NULL,								// �޸�����
		NULL,								// �޲˵�
		hInstance,							// ʵ��
		NULL)))								// ����WM_CREATE�����κ���Ϣ
	{
		KillGLWindow();								// ���ٴ���
		MessageBox(NULL, "���ڴ�������", "����", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// ����FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd =				// pfd���ߴ���������ϣ�������ظ�ʽ
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// ������ʽ�������Ĵ�С
		1,											// �汾��
		PFD_DRAW_TO_WINDOW |						// ��ʽ����֧�ִ���
		PFD_SUPPORT_OPENGL |						// ��ʽ����֧��OpenGL
		PFD_DOUBLEBUFFER,							// ����֧��˫����
		PFD_TYPE_RGBA,								// ��ҪRGB��ʽ
		bits,										// ѡ��ɫ�����
		0, 0, 0, 0, 0, 0,							// ����ɫ��λ
		0,											// ��Alpha����
		0,											// ����Shift Bit
		0,											// ���ۼӻ���
		0, 0, 0, 0,									// ���Ծۼ�λ
		16,											// 16λZ-���� (��Ȼ���)  
		0,											// ���ɰ滺��
		0,											// �޸�������
		PFD_MAIN_PLANE,								// ����ͼ��
		0,											// Ԥ��
		0, 0, 0										// ���Բ�����
	};

	if (!(hDC = GetDC(hWnd)))							// �ж��Ƿ�ȡ���豸������
	{
		KillGLWindow();								// ���ޣ������ٴ���
		MessageBox(NULL, "���ܴ���GL�豸������", "����", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// ����FALSE
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))	// �ж�Windows�Ƿ��ҵ���Ӧ�����ظ�ʽ
	{
		KillGLWindow();								// ���ٴ���
		MessageBox(NULL, "�޷��ҵ��ʺϵ����ظ�ʽ", "����", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// ����FALSE
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd))		// �ж��Ƿ����������ظ�ʽ
	{
		KillGLWindow();								// ���ٴ���
		MessageBox(NULL, "�޷��������ظ�ʽ", "����", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// ����FALSE
	}

	if (!(hRC = wglCreateContext(hDC)))				// �ܷ��ȡ��Ⱦ������
	{
		KillGLWindow();								// ���ٴ���
		MessageBox(NULL, "�޷�����GL��Ⱦ������", "����", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// ����FALSE
	}

	if (!wglMakeCurrent(hDC, hRC))					// ���Լ�����ɫ������
	{
		KillGLWindow();								// ���ٴ���
		MessageBox(NULL, "���ܼ���GL��Ⱦ������", "����", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// ����FALSE
	}

	ShowWindow(hWnd, SW_SHOW);						// ��ʾ����
	SetForegroundWindow(hWnd);						// ������ȼ�
	SetFocus(hWnd);									// �����̽����������˴���
	ReSizeGLScene(width, height);					// ����͸��GL��Ļ

	if (!InitGL())									// ��ʼ���½���GL����
	{                                               // �����ɹ�
		KillGLWindow();								// ���ٴ���
		MessageBox(NULL, "���ڳ�ʼ��ʧ��", "����", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// ����FALSE
	}

	InitializeCriticalSection(&g_csStick);

	return TRUE;									// �ɹ��򷵻�TRUE
}

// ��������Ϣ�ĺ���
LRESULT CALLBACK WndProc
(HWND	hWnd,			// ���ھ��
UINT	uMsg,			// ������Ϣ
WPARAM	wParam,			// ������Ϣ���ݣ�word����Ϣ��Ӧ���ƣ�
LPARAM	lParam)			// ������Ϣ���ݣ�long����Ϣ��Ӧ���ƣ�
{
	switch (uMsg)									// ���Windows��Ϣ
	{
	case WM_ACTIVATE:							// ���Ӵ��ڼ�����Ϣ
	{
		if (!HIWORD(wParam))					// �����С��״̬
		{
			active = TRUE;						// �����ڼ���״̬
		}
		else
		{
			active = FALSE;						// �����ټ���
		}

		return 0;								// ������Ϣѭ��
	}

	case WM_SYSCOMMAND:							// ��Ϣ�ն�����
	{
		switch (wParam)							// ���ϵͳ����
		{
		case SC_SCREENSAVE:					// ����Ҫ���У�
		case SC_MONITORPOWER:				// ��ʾ��Ҫ����ڵ�ģʽ��
			return 0;							// ��ֹ����
		}
		break;									// �˳�
	}

	case WM_CLOSE:								// �Ƿ��յ��ر�ָ�
	{
		PostQuitMessage(0);						// �����˳���Ϣ
		return 0;								// ����
	}

	case WM_KEYDOWN:							// �м����£�
	{
		keys[wParam] = TRUE;					// ���У�����ֵ��ΪTRUE
		return 0;								// ����
	}

	case WM_KEYUP:								// �м��ſ���
	{
		keys[wParam] = FALSE;					// ���У�����ֵ��ΪFALSE
		return 0;								// ����
	}

	case WM_SIZE:								// ���ڴ�С�����ı䣿
	{
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));  // LoWordΪ���, HiWordΪ�߶�
		return 0;								// ����
	}
	}

	// ������δ������Ϣ����DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// �����������ô��ڴ���ʵ����������Ϣ�������˻�����
int WINAPI WinMain
(HINSTANCE	hInstance,			// ��ǰ����ʵ��
HINSTANCE	hPrevInstance,		// ǰһ����ʵ��
LPSTR		lpCmdLine,			// �����в���
int			nCmdShow)			// ������ʾ״̬
{
	MSG		msg;								    	// ���Լ����Ƿ�����Ϣ������
	BOOL	done = FALSE;								// �����˳�ѭ���Ĳ�������
	midi::CMIDIOutDevice m_OutDevice;
	midi::CMIDIInDevice m_InDevice;

	MusicListener m_listener;
	Controller m_controller;

	// ѯ������ȫ��ģʽ�½��л����ڴ���ģʽ
	
		fullscreen = FALSE;							// ����ģʽ
	

	try
	{
		// ����MIDI����豸�����
		if (midi::CMIDIOutDevice::GetNumDevs() > 0)
		{
			m_OutDevice.Open(0);
		}
		// �����򷢳�����
		else
		{
			MessageBox(NULL, "�޿���MIDI����豸", "����", MB_OK | MB_ICONWARNING);
		}

		// ����MIDI�����豸����򿪲���ʼ��¼
		if (midi::CMIDIInDevice::GetNumDevs() > 0)
		{
			m_InDevice.Open(0);
			// ��ʼ����MIDI��Ϣ
			m_InDevice.StartRecording();
		}
		// �����򷢳�����
		else
		{
			MessageBox(NULL, "�޿���MIDI�����豸", "����", MB_OK | MB_ICONWARNING);
		}

	}
	catch (const std::exception &ex)
	{
		MessageBox(NULL, ex.what(), "����", MB_ICONSTOP | MB_OK);
	}

	// ����OpenGL����
	if (!CreateGLWindow("Leap Music V2.0", 640, 480, 16, fullscreen))
	{
		return 0;									// ��ʧ�����˳�
	}

	while (!done)									// ����ѭ��ֱ��done=TRUE
	{
		m_controller.addListener(m_listener);
		g_pianoMelody = new CPianoMelody;

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))	// �Ƿ�����Ϣ������
		{
			if (msg.message == WM_QUIT)				// �Ƿ�Ϊ�˳���Ϣ��
			{
				done = TRUE;							// ���ǣ���done=TRUE
			}
			else									// ������������Ϣ
			{
				TranslateMessage(&msg);				// ������Ϣ
				DispatchMessage(&msg);				// ������Ϣ
			}
		}
		else										// ������Ϣ
		{
			// ������Ļ������ESC��������DrawGLScene���˳���Ϣ
			if ((active && !DrawGLScene()) || keys[VK_ESCAPE])	// �����Ƿ��ڼ���״̬�� ESC�Ƿ񱻰��£�
			{
				done = TRUE;							// �����˳��ź�
			}
			else									// �����ڼ�����ESCû�б�����
			{
				SwapBuffers(hDC);					// ��������(˫����)

				if ((keys['Z'] && key_keep['Z'])||(MyFingerDown==1))
				{
					key_keep['Z'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 36+12*m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("C");
				}
				if ((!keys['Z']) || (MyFingerUp == 1))
				{
					key_keep['Z'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 36 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['S'] && key_keep['S']) || (MyFingerDown == 2))
				{
					key_keep['S'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 37 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("C#");
				}
				if ((!keys['S']) || (MyFingerUp == 2))
				{
					key_keep['S'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 37 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['X'] && key_keep['X']) || (MyFingerDown == 3))
				{
					key_keep['X'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 38 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("D");
				}
				if ((!keys['X']) || (MyFingerUp == 3))
				{
					key_keep['X'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 38 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['D'] && key_keep['D']) || (MyFingerDown == 4))
				{
					key_keep['D'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 39 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("D#");
				}
				if ((!keys['D']) || (MyFingerUp == 4))
				{
					key_keep['D'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 39 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['C'] && key_keep['C']) || (MyFingerDown == 5))
				{
					key_keep['C'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 40 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("E");
				}
				if ((!keys['C']) || (MyFingerUp == 5))
				{
					key_keep['C'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 40 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['V'] && key_keep['V']) || (MyFingerDown == 6))
				{
					key_keep['V'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 41 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("F");
				}
				if ((!keys['V']) || (MyFingerUp == 6))
				{
					key_keep['V'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 41 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['G'] && key_keep['G']) || (MyFingerDown == 7))
				{
					key_keep['G'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 42 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("F#");
				}
				if ((!keys['G']) || (MyFingerUp == 7))
				{
					key_keep['G'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 42 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['B'] && key_keep['B']) || (MyFingerDown == 8))
				{
					key_keep['B'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 43 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("G");
				}
				if ((!keys['B']) || (MyFingerUp == 8))
				{
					key_keep['B'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 43 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['H'] && key_keep['H']) || (MyFingerDown == 9))
				{
					key_keep['H'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 44 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("G#");
				}
				if ((!keys['H']) || (MyFingerUp == 9))
				{
					key_keep['H'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 44 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['N'] && key_keep['N']) || (MyFingerDown == 10))
				{
					key_keep['N'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 45 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("A");
				}
				if ((!keys['N']) || (MyFingerUp == 10))
				{
					key_keep['N'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 45 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['J'] && key_keep['J']) || (MyFingerDown == 11))
				{
					key_keep['J'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 46 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("A#");
				}
				if ((!keys['J']) || (MyFingerUp == 11))
				{
					key_keep['J'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 46 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if ((keys['M'] && key_keep['M']) || (MyFingerDown == 12))
				{
					key_keep['M'] = false;
					midi::CShortMsg ShortMsg(midi::NOTE_ON, 0, 47 + 12 * m_tune, 127, 0);
					ShortMsg.SendMsg(m_OutDevice);
					addlrc("B");
				}
				if ((!keys['M']) || (MyFingerUp == 12))
				{
					key_keep['M'] = true;
					midi::CShortMsg ShortMsg(midi::NOTE_OFF, 0, 47 + 12 * m_tune, 0, 0);
					ShortMsg.SendMsg(m_OutDevice);
				}
				if (keys['1'])
				{
					m_tune = 0;
				}
				if (keys['2'])
				{
					m_tune = 1;
				}
				if (keys['3'])
				{
					m_tune = 2;
				}
				if (keys['4'])
				{
					m_tune = 3;
				}
				if (keys['Q'])  //����
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 1, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}
				if (keys['W'])  //�缪��
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 28, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}
				if (keys['E'])   //С����
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 41, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}
				if (keys['R'])   //����
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 74, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}
				if (keys['T'])   //ǹ��
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 127, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}

				if (keys[VK_F1])						// �ж�F1���Ƿ񱻰���
				{
					keys[VK_F1] = FALSE;					// �����Ƕ�Ӧ��������F1ֵΪFALSE
					KillGLWindow();						// ���ٵ�ǰ����
					fullscreen = !fullscreen;				// Toggle Fullscreen / Windowed Mode
					// �ؽ�OpenGL����
					if (!CreateGLWindow("Leap Music V1.0", 640, 480, 16, fullscreen))
					{
						return 0;						// ��δ�ܽ���������˳�
					}
				}
				if (keys[VK_SPACE]&&key_keep[VK_SPACE])
				{
					key_keep[VK_SPACE] = false;
					//StartRec(_T("yaya.mp3"));
				}
				if (!keys[VK_SPACE])
				{ 
					key_keep[VK_SPACE] = true;
					
					//ֹͣ
					//StopRec();
				}
			}
		}
	}

	// �ر�
	KillGLWindow();									// ���ٴ���
	return (msg.wParam);							// �˳�����
}
