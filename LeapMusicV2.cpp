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

static int m_tune = 0;                  //用于控制音调
static bool key_keep[256] = { true };   //用于记录按键是否被按下

static int MyFingerDown = 0;           //用于记录LeapMotion中的手指按下的位置
static int MyFingerUp = 0;           //用于记录LeapMotion中的手指按键抬起的位置
static char *m_lrc[256];                         //用于存放歌词
static int lrc_numb=0;                 //用于记录歌词数目

GLfloat	cnt1=1.0f;				// 1st Counter Used To Move Text & For Coloring
GLfloat	cnt2=1.2f;				// 2nd Counter Used To Move Text & For Coloring

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// WndProc的声明


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


AUX_RGBImageRec *LoadBMP(char *Filename)				// 加载图像
{
	FILE *File = NULL;									// 文件句柄

	if (!Filename)										// 确保文件名已被给定
	{
		return NULL;									// 若未指定文件名则返回NULL
	}

	// 检查文件是否存在
	fopen_s(&File, Filename, "r");							
	if (File)							 
	{
		fclose(File);									// 关闭句柄
		return auxDIBImageLoad(Filename);				// 加载图像并返回指针
	}

	return NULL;										// 若加载失败则返回NULL
}

int LoadGLTextures()									// 加载图像并转化成纹理
{
	BOOL Status = FALSE;									// 状态标示符

	AUX_RGBImageRec *TextureImage[MAX_TEXTURE];                  // 为纹理开辟存储空间   

	memset(TextureImage, 0, sizeof(void *) * MAX_TEXTURE);           	// 将指针设为NULL

	if ((TextureImage[0] = LoadBMP("Resources/white.bmp"))
		&& (TextureImage[1] = LoadBMP("Resources/black.bmp"))
		&& (TextureImage[2] = LoadBMP("Resources/piano.bmp")))
	{
		Status = TRUE;                                           // 设置状态变量为TRUE   
		glGenTextures(MAX_TEXTURE, &texture[0]);                 // 返回唯一的纹理名字来标识纹理,保存在texture中   

		// 用图片数据产生纹理   
		for (int loop = 0; loop<MAX_TEXTURE; loop++)                // 循环处理MAX_TEXTURE张纹理   
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
			if (TextureImage[loop]->data)        // 如果纹理存在   
			{
				free(TextureImage[loop]->data);  // 释放纹理存储空间   
			}
			free(TextureImage[loop]);
		}
	}
	return Status;                                // 返回State   
}

// 重置OpenGL窗口大小
GLvoid ReSizeGLScene(GLsizei width, GLsizei height)
{
	if (height == 0)										// 判断高度是否为0（防止被清0）
	{
		height = 1;										// 若被清零，将高度重置为1
	}

	glViewport(0, 0, width, height);						// 将OpenGL场景尺寸设置成所在窗口大小

	glMatrixMode(GL_PROJECTION);						// 选择投影矩阵
	glLoadIdentity();									// 重置投影矩阵

	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);     // 设置视口大小

	glMatrixMode(GL_MODELVIEW);							// 选择模型观察矩阵
	glLoadIdentity();									// 重置模型观察矩阵
}

// 开始对OpenGL进行设置
int InitGL(GLvoid)
{
	//检测是否已加载纹理，若未加载则返回FALSE
	if (!LoadGLTextures())								
	{
		return FALSE;									
	}

	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);							// 启用阴影平滑
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// 设置背景颜色（此处为黑色）
	glClearDepth(1.0f);									// 设置深度缓存
	glEnable(GL_DEPTH_TEST);							// 启用深度测试
	glDepthFunc(GL_LEQUAL);								// 所做深度测试的类型

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// 对透视进行修正（此处为nicest，还有GL_FASTEST和GL_DONT_CARE）
	BuildFont();										// Build The Font

	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);		// 设置环绕光
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);		// 设置散射光
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);	// 放置光源
	glEnable(GL_LIGHT1);								// 激活光源

	m_lrc[0] = " ";
	m_lrc[1] = " ";
	m_lrc[2] = " ";
	m_lrc[3] = " ";
	return TRUE;										// 初始化OK
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

// 此处开始正式绘制图像
int DrawGLScene(GLvoid)
{

	//glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// 清除场景和深度缓存   
	glLoadIdentity();                                   // 重置当前矩阵   
	// 在此处添加代码进行绘制:
	glTranslatef(0.0f, 0.4f, -2.0f);
	glRotatef(30, 1.0f, 0.0f, 0.0f);

	//钢琴部分
	glColor3f(1, 1, 1);
	// 从左到右绘制七个白色按键
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glPushMatrix();
	glTranslatef(-0.8f, -0.5f, 0.0f);

	// 第一个白色按键   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['Z'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// 第二个白色按键   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['X'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// 第三个白色按键   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['C'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// 第四个白色按键   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['V'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// 第五个白色按键   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['B'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// 第六个白色按键   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['N'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	// 第七个白色按键   
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['M'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.2f, 0.1f, 0.8f);
	glPopMatrix();

	glPopMatrix();                      // 白色按键绘制完毕   

	//钢琴的其他部分
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

	//绘制黑键
	glColor3f(1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glPushMatrix();
	glTranslatef(-0.7f, -0.46f, -0.2f);

	//第一个黑键
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['S'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	//第二个黑键
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['D'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	//第三个黑键
	glTranslatef(0.4f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['G'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	//第四个黑键
	glTranslatef(0.2f, 0.0f, 0.0f);
	glPushMatrix();
	if (!key_keep['H'])
		glTranslatef(0.0f, -0.04f, 0.0f);
	glDrawCube(0.1f, 0.1f, 0.4f);
	glPopMatrix();

	//第五个黑键
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

	//绘制人手
	SStick *m_finger;
	GLUquadricObj *qobj = gluNewQuadric();
	EnterCriticalSection(&g_csStick);
	for (int i = 0; i < g_StickVector.size(); ++i)
	{
		m_finger = &g_StickVector[i];

		//手指
		glColor3f(0, 1, 1);
		glPushMatrix();
		DrawCylinder(m_finger);
		glPopMatrix();

		//手指尖(球)
		glColor3f(1, 0, 0);
		glPushMatrix();
		glTranslatef(m_finger->end_x, m_finger->end_y, m_finger->end_z);
		gluSphere(qobj, 0.003, 100, 100);
		glPopMatrix();
	}
	LeaveCriticalSection(&g_csStick);

	return TRUE;										// 一切正常
}

// 正常销毁窗口
GLvoid KillGLWindow(GLvoid)
{
	if (fullscreen)										// 判断是否处于全屏模式
	{
		ChangeDisplaySettings(NULL, 0);					// 若是，回到桌面
		ShowCursor(TRUE);								// 显示鼠标
	}

	if (hRC)											// 判断是否有OpenGL渲染描述表
	{
		if (!wglMakeCurrent(NULL, NULL))					// 能否释放DC和RC
		{
			MessageBox(NULL, "释放DC或RC失败", "关闭错误", MB_OK | MB_ICONINFORMATION);     //若不能释放则弹出窗口
		}

		if (!wglDeleteContext(hRC))						// 能否删除RC？
		{
			MessageBox(NULL, "释放RC失败", "关闭错误", MB_OK | MB_ICONINFORMATION);
		}
		hRC = NULL;										// 将RC设为NULL
	}

	if (hDC && !ReleaseDC(hWnd, hDC))					// 能否释放DC？
	{
		MessageBox(NULL, "释放DC失败", "关闭错误", MB_OK | MB_ICONINFORMATION);
		hDC = NULL;										// 将DC设为NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// 能否销毁窗口？
	{
		MessageBox(NULL, "释放窗口句柄失败", "关闭错误", MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;										// 将窗口句柄hWnd设为NULL
	}

	if (!UnregisterClass("OpenGL", hInstance))			// 能否注销类？
	{
		MessageBox(NULL, "不能注销窗口类", "关闭错误", MB_OK | MB_ICONINFORMATION);
		hInstance = NULL;									// 将hInstance设为NULL
	}
	KillFont();
}

//创建OpenGL窗口，返回布尔变量，函数参数依次为：标题、宽、高、色彩位数、全屏标志
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// 用来保存查找匹配（像素格式）的结果
	WNDCLASS	wc;						// 保存窗口类结构
	DWORD		dwExStyle;				// 存放扩展窗口风格
	DWORD		dwStyle;				// 存放窗口风格
	RECT		WindowRect;				// 取得举行左上角和右下角的坐标值
	WindowRect.left = (long)0;			// 将最左值设为0
	WindowRect.right = (long)width;		// 将最右值设为要求宽度
	WindowRect.top = (long)0;				// 将顶端值设为0
	WindowRect.bottom = (long)height;		// 将底端值设为要求高度

	fullscreen = fullscreenflag;			// 设置全局全屏标志

	hInstance = GetModuleHandle(NULL);				// 取得窗口实例
	wc.style = CS_HREDRAW | CS_HREDRAW | CS_OWNDC;	// CS_HREDRAW：水平方向变化时重画；CS_HREDRAW：竖直方向变化时重画；CS_OWNDC：为窗口创建私有的DC
	wc.lpfnWndProc = (WNDPROC)WndProc;					// WndProc处理消息
	wc.cbClsExtra = 0;									// 无额外窗口数据
	wc.cbWndExtra = 0;									// 无额外窗口数据
	wc.hInstance = hInstance;							// 设置实例
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);			// 装入缺省图标
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);			// 装入缺省鼠标指针
	wc.hbrBackground = NULL;									// GL背景为空
	wc.lpszMenuName = NULL;									// 菜单为空
	wc.lpszClassName = "OpenGL";								// 设置类名

	if (!RegisterClass(&wc))									// 尝试注册窗口类
	{
		MessageBox(NULL, "注册窗口类失败", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;											// 返回FALSE
	}

	if (fullscreen)												// 是否为全屏模式？
	{
		DEVMODE dmScreenSettings;								// 定义设备模式
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));	// 确保内存清空
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);		// 设置设备模式的大小
		dmScreenSettings.dmPelsWidth = width;				// 宽度为屏幕宽度
		dmScreenSettings.dmPelsHeight = height;				// 高度为屏幕高度
		dmScreenSettings.dmBitsPerPel = bits;					// 设置每像素对应的色彩深度
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// 尝试设置状态模式并返回结果。  注：CDS_FULLSCREEN移去了状态条
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			// 若模式设置失败，提供两个选项：退出或在窗口内运行
			if (MessageBox(NULL, "全屏模式在当前显卡上设置失败！\n是否使用窗口模式？", "Leap Music", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			{
				fullscreen = FALSE;		// 设置窗口模式（不在全屏下运行）
			}
			else
			{
				// 弹出窗口告知用户程序即将关闭
				MessageBox(NULL, "程序将被关闭", "错误", MB_OK | MB_ICONSTOP);
				return FALSE;									// 退出并返回FALSE
			}
		}
	}

	if (fullscreen)												// 检测是否仍处于全屏模式
	{
		dwExStyle = WS_EX_APPWINDOW;								// 设置扩展窗体风格
		dwStyle = WS_POPUP;										// 设置窗体风格
		ShowCursor(FALSE);										// 设定是否显示鼠标指针（这里不显示）
	}
	else                                                         //若不在全屏模式
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// 设置扩展窗体风格
		dwStyle = WS_OVERLAPPEDWINDOW;							// 设置窗体风格
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// 调整窗口至真是要求的大小

	// 创建窗口并检测窗口是否创建成功
	if (!(hWnd = CreateWindowEx            //以下为向CreateWindowEx函数传递的变量
		(dwExStyle,							// 扩展窗体风格
		"OpenGL",							// 类名
		title,								// 窗口标题
		dwStyle |							// 已选择的窗体风格
		WS_CLIPSIBLINGS |					// 必须的窗体风格
		WS_CLIPCHILDREN,					// 必须的窗体风格
		0, 0,								// 窗口位置
		WindowRect.right - WindowRect.left,	// 窗口宽度（调整后）
		WindowRect.bottom - WindowRect.top,	// 窗口高度（调整后）
		NULL,								// 无父窗口
		NULL,								// 无菜单
		hInstance,							// 实例
		NULL)))								// 不向WM_CREATE传递任何信息
	{
		KillGLWindow();								// 销毁窗口
		MessageBox(NULL, "窗口创建错误", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// 返回FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd =				// pfd告诉窗口我们所希望的像素格式
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// 上述格式描述符的大小
		1,											// 版本号
		PFD_DRAW_TO_WINDOW |						// 格式必须支持窗口
		PFD_SUPPORT_OPENGL |						// 格式必须支持OpenGL
		PFD_DOUBLEBUFFER,							// 必须支持双缓冲
		PFD_TYPE_RGBA,								// 需要RGB格式
		bits,										// 选定色彩深度
		0, 0, 0, 0, 0, 0,							// 忽略色彩位
		0,											// 无Alpha缓存
		0,											// 忽略Shift Bit
		0,											// 无累加缓存
		0, 0, 0, 0,									// 忽略聚集位
		16,											// 16位Z-缓存 (深度缓存)  
		0,											// 无蒙版缓存
		0,											// 无辅助缓存
		PFD_MAIN_PLANE,								// 主绘图层
		0,											// 预留
		0, 0, 0										// 忽略层遮罩
	};

	if (!(hDC = GetDC(hWnd)))							// 判断是否取得设备描述表
	{
		KillGLWindow();								// 若无，则销毁窗口
		MessageBox(NULL, "不能创建GL设备描述表", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// 返回FALSE
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))	// 判断Windows是否找到相应的像素格式
	{
		KillGLWindow();								// 销毁窗口
		MessageBox(NULL, "无法找到适合的像素格式", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// 返回FALSE
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd))		// 判断是否能设置像素格式
	{
		KillGLWindow();								// 销毁窗口
		MessageBox(NULL, "无法设置像素格式", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// 返回FALSE
	}

	if (!(hRC = wglCreateContext(hDC)))				// 能否获取渲染描述表
	{
		KillGLWindow();								// 销毁窗口
		MessageBox(NULL, "无法创建GL渲染描述表", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// 返回FALSE
	}

	if (!wglMakeCurrent(hDC, hRC))					// 尝试激活着色描述表
	{
		KillGLWindow();								// 销毁窗口
		MessageBox(NULL, "不能激活GL渲染描述表", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// 返回FALSE
	}

	ShowWindow(hWnd, SW_SHOW);						// 显示窗口
	SetForegroundWindow(hWnd);						// 提高优先级
	SetFocus(hWnd);									// 将键盘焦点设置至此窗口
	ReSizeGLScene(width, height);					// 设置透视GL屏幕

	if (!InitGL())									// 初始化新建的GL窗口
	{                                               // 若不成功
		KillGLWindow();								// 销毁窗口
		MessageBox(NULL, "窗口初始化失败", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// 返回FALSE
	}

	InitializeCriticalSection(&g_csStick);

	return TRUE;									// 成功则返回TRUE
}

// 处理窗口消息的函数
LRESULT CALLBACK WndProc
(HWND	hWnd,			// 窗口句柄
UINT	uMsg,			// 窗口消息
WPARAM	wParam,			// 附加消息内容（word型消息相应机制）
LPARAM	lParam)			// 附加消息内容（long型消息相应机制）
{
	switch (uMsg)									// 检查Windows消息
	{
	case WM_ACTIVATE:							// 监视窗口激活消息
	{
		if (!HIWORD(wParam))					// 检测最小化状态
		{
			active = TRUE;						// 程序处于激活状态
		}
		else
		{
			active = FALSE;						// 程序不再激活
		}

		return 0;								// 返回消息循环
	}

	case WM_SYSCOMMAND:							// 消息终端命令
	{
		switch (wParam)							// 检查系统调用
		{
		case SC_SCREENSAVE:					// 屏保要运行？
		case SC_MONITORPOWER:				// 显示器要进入节电模式？
			return 0;							// 阻止发生
		}
		break;									// 退出
	}

	case WM_CLOSE:								// 是否收到关闭指令？
	{
		PostQuitMessage(0);						// 发送退出消息
		return 0;								// 返回
	}

	case WM_KEYDOWN:							// 有键按下？
	{
		keys[wParam] = TRUE;					// 若有，将其值设为TRUE
		return 0;								// 返回
	}

	case WM_KEYUP:								// 有键放开？
	{
		keys[wParam] = FALSE;					// 若有，将其值设为FALSE
		return 0;								// 返回
	}

	case WM_SIZE:								// 窗口大小发生改变？
	{
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));  // LoWord为宽度, HiWord为高度
		return 0;								// 返回
	}
	}

	// 将所有未处理消息送至DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// 主函数：调用窗口创建实例、处理消息、监视人机交互
int WINAPI WinMain
(HINSTANCE	hInstance,			// 当前窗口实例
HINSTANCE	hPrevInstance,		// 前一窗口实例
LPSTR		lpCmdLine,			// 命令行参数
int			nCmdShow)			// 窗口显示状态
{
	MSG		msg;								    	// 用以监视是否有消息待处理
	BOOL	done = FALSE;								// 用来退出循环的布尔变量
	midi::CMIDIOutDevice m_OutDevice;
	midi::CMIDIInDevice m_InDevice;

	MusicListener m_listener;
	Controller m_controller;

	// 询问是在全屏模式下进行还是在窗口模式
	
		fullscreen = FALSE;							// 窗口模式
	

	try
	{
		// 若有MIDI输出设备则将其打开
		if (midi::CMIDIOutDevice::GetNumDevs() > 0)
		{
			m_OutDevice.Open(0);
		}
		// 若无则发出警告
		else
		{
			MessageBox(NULL, "无可用MIDI输出设备", "警告", MB_OK | MB_ICONWARNING);
		}

		// 若有MIDI输入设备则将其打开并开始记录
		if (midi::CMIDIInDevice::GetNumDevs() > 0)
		{
			m_InDevice.Open(0);
			// 开始接受MIDI消息
			m_InDevice.StartRecording();
		}
		// 若无则发出警告
		else
		{
			MessageBox(NULL, "无可用MIDI输入设备", "警告", MB_OK | MB_ICONWARNING);
		}

	}
	catch (const std::exception &ex)
	{
		MessageBox(NULL, ex.what(), "错误", MB_ICONSTOP | MB_OK);
	}

	// 创建OpenGL窗口
	if (!CreateGLWindow("Leap Music V2.0", 640, 480, 16, fullscreen))
	{
		return 0;									// 若失败则退出
	}

	while (!done)									// 保持循环直至done=TRUE
	{
		m_controller.addListener(m_listener);
		g_pianoMelody = new CPianoMelody;

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))	// 是否有消息待处理？
		{
			if (msg.message == WM_QUIT)				// 是否为退出消息？
			{
				done = TRUE;							// 若是，则done=TRUE
			}
			else									// 若否，则处理窗口消息
			{
				TranslateMessage(&msg);				// 翻译消息
				DispatchMessage(&msg);				// 发送消息
			}
		}
		else										// 若无消息
		{
			// 绘制屏幕、监视ESC键和来自DrawGLScene的退出消息
			if ((active && !DrawGLScene()) || keys[VK_ESCAPE])	// 程序是否处于激活状态？ ESC是否被按下？
			{
				done = TRUE;							// 发出退出信号
			}
			else									// 若处于激活且ESC没有被按下
			{
				SwapBuffers(hDC);					// 交换缓存(双缓存)

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
				if (keys['Q'])  //钢琴
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 1, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}
				if (keys['W'])  //电吉他
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 28, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}
				if (keys['E'])   //小提琴
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 41, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}
				if (keys['R'])   //笛子
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 74, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}
				if (keys['T'])   //枪声
				{
					midi::CShortMsg Msg(midi::PROGRAM_CHANGE, 0, 127, 0, 0);
					Msg.SendMsg(m_OutDevice);
				}

				if (keys[VK_F1])						// 判断F1键是否被按下
				{
					keys[VK_F1] = FALSE;					// 若是是对应键数组中F1值为FALSE
					KillGLWindow();						// 销毁当前窗口
					fullscreen = !fullscreen;				// Toggle Fullscreen / Windowed Mode
					// 重建OpenGL窗口
					if (!CreateGLWindow("Leap Music V1.0", 640, 480, 16, fullscreen))
					{
						return 0;						// 若未能建立则程序退出
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
					
					//停止
					//StopRec();
				}
			}
		}
	}

	// 关闭
	KillGLWindow();									// 销毁窗口
	return (msg.wParam);							// 退出程序
}
