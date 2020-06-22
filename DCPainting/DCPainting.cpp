// DCPainting.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <wingdi.h>
#include <math.h>
#include <vector>
#include <stdio.h>
#include "DCPainting.h"

#define MAX_LOADSTRING 100
#define _RGB(r,g,b)	(((r) << 16) | ((g) << 8) | (b))			// Convert to RGB
#define _BITS_PER_PIXEL	32										// Color depth

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;

enum dataformat
{
	R32G32B32_FORMAT,
	R32G32B32A32_FORMAT
};

enum primitivetype
{
	POINT_PRIMITIVE,
	LINE_PRIMITIVE,
	TRIANGLE_PRIMITIVE,
	TRIANGLELIST_PRIMITIVE
};

enum  state
{
	CULLBACKFACE = 1,
	DEPTHTEST = 2,
	ZWRITE = 4,
	TEXTURE = 8,
	LIGHTING = 16,
	BLEND = 32
};

enum lighttype
{
	AMBIENT_LIGHT,
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT
};

enum texturemapmode
{
	MODULATE,
};

enum blendfactor
{
	ZERO,
	ONE,
	SRC_COLOR,
	SRC_ONEMINUS_COLOR,
	SRC_ALPHA,
	SRC_ONEMINUS_ALPHA,
	DST_COLOR,
	DST_ONEMINUS_COLOR,
	DST_ALPHA,
	DST_ONEMINUS_ALPHA
};

enum blendoperation
{
	ADD,
	SUBTRACT,
	REVERSESUBTRACT,
	MIN,
	MAX
};

typedef struct bitmapfile
{
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	DWORD dwSize;
	LPDWORD lpData;

} _bitmapfile, *_lpbitmapfile;

struct vector4
{
	float x, y, z, w;

	vector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f)
		:x(x), y(y), z(z), w(w)
	{}
};

typedef struct colorrgba
{
	float r, g, b, a;
};

struct texture
{
	int width, height, widthBytes;;
	dataformat format;
	uint *data;
};

struct texcoord
{
	float u, v;
};

struct light
{
	lighttype type;
	vector4 pos;
	vector4 dir;
	colorrgba color;
	float intensity;
	bool enable;
};

typedef struct vertex
{
	vector4 pos;
	colorrgba color;
	texcoord uv;
	vector4 normal;
}vsoutput;

struct vertexbuffer
{
	int bytes;
	void *subdata = NULL;
};

struct vertexlayout
{
	char *semanticname;
	dataformat format;
	int aligendbyteoffset;
};

struct triangle
{
	vertex v1, v2, v3;
};

typedef struct matrix
{
	float m0, m1, m2, m3,
	m4, m5, m6, m7,
	m8, m9, m10, m11,
	m12, m13, m14, m15;
};

typedef struct fragment
{
	int x, y;
	float z;
	colorrgba color;
	texcoord uv;
};

// 全局变量: 
HINSTANCE					hInst;								// 当前实例
TCHAR						szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR						szWindowClass[MAX_LOADSTRING];			// 主窗口类名

//设备相关
HWND						hWnd;
HDC							hDeviceDC;
HDC							hMemDC;
HBITMAP						hBackBitmap;

//颜色，深度缓冲
uint						*pFrameBuffer = NULL;
float						*pZBuffer = NULL;

vertexlayout				*pInputLayout = NULL;
int							inputLayoutNum = 0;
vertexbuffer				*pVertexBuffer = NULL;
vsoutput					*pVSOut = NULL;
int							vsOutputBytes = 0;

int							curPrimitiveType = TRIANGLE_PRIMITIVE;
std::vector<triangle>		outputPrimitive;

std::vector<fragment>		fragments;

int							renderState;
light						lights[8];

int							viewW = 640;
int							viewH = 480;

//世界、视图、投影变换矩阵
matrix						world;
matrix						view;
matrix						projection;

//纹理
texture						*pTexture;

//窗口相关 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int, int, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

//初始化device
bool				Initialize(int w, int h);
void				Destrory();

//clear bg color
void				ClearBG(uchar r = 255, uchar g = 255, uchar b = 255);

//input layout
void				SetInputLayout(vertexlayout[], int);

//vs stage
bool				VertexShaderStage(vertexbuffer*);
bool				VertexShader(vertex*);
int					CVVCulling(vector4);

//primitive assemebly
bool				PrimitiveAssembly(vsoutput* v, int vertexcount, unsigned short indices[], int indexCount, texcoord*);
void				SetPrimitiveType(int);
bool				BackFaceCulling(triangle*);

//rasterization
bool				RasterizationStage();
void				RasterTriangle(triangle *t);
int					SplitTriangle(triangle *input, triangle *output);
void				VertexInterpolate(vertex *v1, vertex *v2, vertex *newv, float t);

//fs stage
bool				FragmentShaderStage();

//om stage
bool				OutMergeStage();

//
bool				Pipeline(vertexbuffer*, int vcount, unsigned short*, int icount, texcoord*);


//视图、投影变换（右手系）
matrix				CreateView(vector4 eye, vector4 look, vector4 up);
matrix				CreatePerspective(float l, float r, float t, float b, float n, float f);
matrix				CreateOrtho(float l, float r, float t, float b, float n, float f);

vector4				WorldTransform(vector4 v);
vector4				ViewTransform(vector4 v);
vector4				ProjectionTransform(vector4 v);
vector4				ScreenTransform(vector4 v);

//主渲染函数
void				Render();
bool				DrawTriangleCone();
bool				DrawBox();
bool				DrawBox2();

//向量基本运算 
vector4				Normalized(vector4);
vector4				operator-(vector4, vector4);
vector4				operator*(float f, vector4 r);
vector4				CrossProduct(vector4, vector4);
float				DotProduct(vector4, vector4);

//矩阵基本运算
matrix				Identity();
matrix				Mul(matrix, matrix);
vector4				Mul(vector4, matrix);


//其他函数
void				RoundVertex(vertex*);
_lpbitmapfile		LoadBitmap(LPTSTR lpszBitmapFile);

void				SetRenderState(int);
bool				SetTexture(_bitmapfile);
bool				SetLight(int, light, bool);
void				SetBlend(blendfactor, blendfactor, blendoperation);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO:  在此放置代码。
	MSG msg = { 0 };
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_DCPAINTING, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance(hInstance, viewW, viewH, nCmdShow))
	{
		return FALSE;
	}

	if (!Initialize(viewW, viewH))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DCPAINTING));

	// 主消息循环: 
	while (WM_QUIT != msg.message)
	{
		Render();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Destrory();
	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DCPAINTING));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int w, int h, int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

	RECT rc = RECT{ 0, 0, w, h };
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

float rotateAngle = 0.0f;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RIGHT:
			rotateAngle += 0.1f;
			break;
		case VK_LEFT:
			rotateAngle -= 0.1f;
			break;
		default:
			break;
		}

		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

matrix CreateView(vector4 eye, vector4 look, vector4 up)
{
	vector4 n = Normalized(eye - look);
	vector4 r = Normalized(CrossProduct(up, n));
	vector4 v = Normalized(CrossProduct(n, r));

	matrix view;
	view.m0 = r.x; view.m1 = v.x; view.m2 = n.x; view.m3 = 0.0f;
	view.m4 = r.y; view.m5 = v.y; view.m6 = n.y; view.m7 = 0.0f;
	view.m8 = r.z; view.m9 = v.z; view.m10 = n.z; view.m11 = 0.0f;
	view.m12 = DotProduct(-1.0f * eye, r);
	view.m13 = DotProduct(-1.0f * eye, v);
	view.m14 = DotProduct(-1.0f * eye, n);
	view.m15 = 1.0f;

	return view;
}

matrix CreatePerspective(float l, float r, float t, float b, float n, float f)
{
	matrix p = { 0.0f };
	float t1 = 2 * n;
	float t2 = r - l;
	float t3 = t - b;
	float t4 = f - n;

	//x-->(-1, 1), y-->(-1, 1), z-->(-1, 1)
	p.m0 = t1 / t2;				p.m1 = 0.0f;				p.m2 = 0.0f;				p.m3 = 0.0f;
	p.m4 = 0.0f;				p.m5 = t1 / t3;				p.m6 = 0.0f;				p.m7 = 0.0f;
	p.m8 = (r + l) / t2;		p.m9 = (t + b) / t3;		p.m10 = -(f + n) / t4;		p.m11 = -1.0f;
	p.m12 = 0.0f;				p.m13 = 0.0f;				p.m14 = -2 * f * n / t4;	p.m15 = 0.0f;

	return p;
}

matrix CreatePerspective(float fovy, float aspect, float zn, float zf)
{
	float fax = 1.0f / (float)tan(fovy * 0.5f);

	matrix p = { 0.0f };
	p.m0 = (float)(fax / aspect);
	p.m5 = fax;
	p.m10 = zf / (zf - zn);
	p.m14 = -zn * zf / (zf - zn);
	p.m11 = 1.0f;

	return p;
}

matrix CreateOrtho(float l, float r, float t, float b, float n, float f)
{
	matrix o = { 0.0f };
	float t1 = r - l;
	float t2 = t - b;
	float t3 = f - n;

	o.m0 = 2.0f / t1;		o.m1 = 0.0f;			o.m2 = 0.0f;			o.m3 = 0.0f;
	o.m4 = 0.0f;			o.m5 = 2.0f / t2;		o.m6 = 0.0f;			o.m7 = 0.0f;
	o.m8 = 0.0f;			o.m9 = 0.0f;			o.m10 = -2.0f / t3;		o.m11 = 0.0f;
	o.m12 = (r + l) / t1;	o.m13 = (t + b) / t2;	o.m14 = (f + n) / t3;	o.m15 = 1.0f;

	return o;
}

vector4 operator-(vector4 l, vector4 r)
{
	return vector4(l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w);
}

vector4 operator*(float f, vector4 r)
{
	return vector4(f * r.x, f * r.y, f * r.z, f * r.w);
}

vector4 operator+(vector4 l, vector4 r)
{
	return vector4(l.x + r.x, l.y + r.y, l.z + r.z, l.w);
}

void operator+=(vector4 &l, vector4 r)
{
	l.x += r.x;
	l.y += r.y;
	l.z += r.z;
}

colorrgba operator*(colorrgba &l, colorrgba &r)
{
	colorrgba color = { l.r * r.r, l.g * r.g, l.b * r.b, l.a * r.a };
	return color;
}

colorrgba operator+(colorrgba &l, colorrgba &r)
{
	colorrgba color = { l.r + r.r, l.g + r.g, l.b + r.b, l.a + r.a };
	return color;
}

colorrgba operator-(colorrgba &l, colorrgba &r)
{
	colorrgba color = { l.r - r.r, l.g - r.g, l.b - r.b, l.a - r.a };
	return color;
}

vector4 Normalized(vector4 v)
{
	float magnitude = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	return vector4(v.x / magnitude, v.y / magnitude, v.z / magnitude, 0.0f);
}

vector4 CrossProduct(vector4 l, vector4 r)
{
	return vector4(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x, 0.0f);
}

float DotProduct(vector4 l, vector4 r)
{
	return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
}

matrix Identity()
{
	matrix identity = { 0.0f };
	identity.m0 = 1.0f; identity.m1 = 0.0f; identity.m2 = 0.0f; identity.m3 = 0.0f;
	identity.m4 = 0.0f; identity.m5 = 1.0f; identity.m6 = 0.0f; identity.m7 = 0.0f;
	identity.m8 = 0.0f; identity.m9 = 0.0f; identity.m10 = 1.0f; identity.m11 = 0.0f;
	identity.m12 = 0.0f; identity.m13 = 0.0f; identity.m14 = 0.0f; identity.m15 = 1.0f;

	return identity;
}

matrix Mul(matrix l, matrix r)
{
	matrix mulResult = Identity();

	return mulResult;
}

vector4 Mul(vector4 v, matrix m)
{
	vector4 r = {};
	vector4 tmp = { m.m0, m.m4, m.m8, m.m12 };
	r.x = DotProduct(v, tmp);

	tmp = { m.m1, m.m5, m.m9, m.m13 };
	r.y = DotProduct(v, tmp);

	tmp = { m.m2, m.m6, m.m10, m.m14 };
	r.z = DotProduct(v, tmp);

	tmp = { m.m3, m.m7, m.m11, m.m15 };
	r.w = DotProduct(v, tmp);

	return r;
}

matrix RotateX(float angle)
{
	matrix rotateX = Identity();

	rotateX.m5 = cos(angle);
	rotateX.m9 = -sin(angle);

	rotateX.m6 = sin(angle);
	rotateX.m10 = cos(angle);

	return rotateX;
}

matrix RotateY(float angle)
{
	matrix rotateY = Identity();

	rotateY.m0 = cos(angle);
	rotateY.m8 = -sin(angle);

	rotateY.m2 = sin(angle);
	rotateY.m10 = cos(angle);

	return rotateY;
}

matrix RotateZ(float angle)
{
	matrix rotateZ = Identity();

	rotateZ.m0 = cos(angle);
	rotateZ.m4 = -sin(angle);

	rotateZ.m1 = sin(angle);
	rotateZ.m5 = cos(angle);

	return rotateZ;
}

matrix RotateV(vector4 v, float rad)
{
	matrix rotateV = Identity();

	return rotateV;
}

bool Initialize(int w, int h)
{
	//获取屏幕DC， 创建内存DC， 创建位图

	//位图头结构
	BITMAPINFO bmInfo;
	memset(&bmInfo.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmInfo.bmiHeader.biWidth = w;
	bmInfo.bmiHeader.biHeight = h;
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = 32;

	//获取屏幕DC和内存DC
	hDeviceDC = GetDC(hWnd);
	if (!hDeviceDC)
	{
		return false;
	}

	hMemDC = CreateCompatibleDC(hDeviceDC);
	if (!hMemDC)
	{
		return false;
	}

	//创建位图
	BYTE *pBMData;
	hBackBitmap = CreateDIBSection(hDeviceDC, &bmInfo, DIB_RGB_COLORS, (void**)&pBMData, NULL, 0);
	pFrameBuffer = (UINT*)pBMData;

	int len = viewH * viewW * sizeof(float);
	pZBuffer = (float*)malloc(len);
	for (int i = 0; i < viewH * viewH; i++)
	{
		pZBuffer[i] = 1000.f;
	}

	if (!hBackBitmap)
	{
		return false;
	}

	light light0 = { AMBIENT_LIGHT, {}, {}, { 1.0f, 0, 0, 1.0f }, 0.2f };
	light light1 = { DIRECTIONAL_LIGHT, {}, { 0.0f, 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.3f };
	light light2 = { POINT_LIGHT, { -4.0f, 4.0f, 0.0f, 1.0f }, {}, { 1.0f, 1.0f, 0, 1.0f } };

	bool result = SetLight(0, light0, true);
	result &= SetLight(1, light1, true);
	result &= SetLight(2, light2, true);
	if (!result)
	{
		return false;
	}

	//设置纹理	
	_lpbitmapfile bmp = LoadBitmap(L"test.bmp");
	result = SetTexture(*bmp);

	//设置初始世界、视图以及投影
	world = Identity();
	vector4 eyePos = vector4(0.0f, 0.0f, 15.0f, 1.0f);
	vector4 lookPos = vector4(0.0f, 0.0f, 0.0f, 1.0f);
	vector4 upVector = vector4(0.0f, 1.0f, 0.0f, 0.0f);
	view = CreateView(eyePos, lookPos, upVector);

	float left = -5.0f, right = 5.0f, bottom = -5.0f, top = 5.0f, n = 1.5f, f = 100.f;
	projection = CreatePerspective(left, right, top, bottom, n, f);

	return true;
}

void Destrory()
{
	if (pVSOut)
	{
		free(pVSOut);
	}
	pVSOut = NULL;
	pVertexBuffer = NULL;

	if (pInputLayout)
	{
		free(pInputLayout);
	}
	pInputLayout = NULL;

	outputPrimitive.clear();
	fragments.clear();

	if (pZBuffer)
	{
		free(pZBuffer);
	}
	pZBuffer = NULL;

	DeleteObject(hBackBitmap);
	DeleteObject(hMemDC);

	ReleaseDC(NULL, hDeviceDC);
}

void ClearBG(uchar r, uchar g, uchar b)
{
	for (int i = 0; i < viewH; i++)
	{
		for (int j = 0; j < viewW; j++)
		{
			int index = i * viewW + j;
			pFrameBuffer[index] = (r << 16) | (g << 8) | (b);
			pZBuffer[index] = 100;
		}
	}
}

void UpdateTransform()
{
	static float t = 0.0f;
	static DWORD dwTimeStart = 0;
	DWORD dwTimeCur = GetTickCount();
	if (dwTimeStart == 0)
		dwTimeStart = dwTimeCur;
	t = (dwTimeCur - dwTimeStart) / 1000.0f;

	//world = RotateY(rotateAngle);

	matrix mat = RotateY(t);
	vector4 eyePos = vector4(0.0f, 10.0f, 13.0f, 1.0f);
	vector4 lookPos = vector4(0.0f, 0.0f, 0.0f, 1.0f);
	vector4 upVector = vector4(0.0f, 1.0f, 0.0f, 0.0f);

	eyePos = Mul(eyePos, mat);
	upVector = Mul(upVector, mat);
	view = CreateView(eyePos, lookPos, upVector);
}

void Render()
{
	//内存DC绑定一块位图
	HBITMAP hOldBackBitmap = (HBITMAP)::SelectObject(hMemDC, hBackBitmap);

	//擦除背景
	ClearBG(255, 255, 255);

	//Draw on hMemDC,  画到内存DC上
	UpdateTransform();

	bool result = true;

	result = DrawTriangleCone();
	if (!result) return;

	//result = DrawBox2();
	//if (!result) return;

	result = DrawBox();
	if (!result) return;


	result = DrawBox();
	if (!result) return;


	//BitBlit hMemDC--->hDeviceDC, 内存DC--->屏幕DC
	result = BitBlt(hDeviceDC, 0, 0, viewW, viewH, hMemDC, 0, 0, SRCCOPY);
	if (!result) return;

	//clear
	SelectObject(hMemDC, hOldBackBitmap);
}

void ComputeNormal(vertexbuffer *buffer, int vcount, ushort *indices, int icount)
{
	vertex* p = (vertex*)buffer->subdata;
	int *sharedArr = (int*)malloc(sizeof(int)* vcount);
	memset(sharedArr, 0, vcount * sizeof(int));

	int trinum = icount / 3;
	for (int i = 0; i < trinum; i++)
	{
		vector4 v0 = (p[indices[i * 3 + 1]]).pos - (p[indices[i * 3]]).pos;
		vector4 v1 = (p[indices[i * 3 + 2]]).pos - (p[indices[i * 3 + 1]]).pos;
		vector4 n = CrossProduct(v0, v1);

		(p[indices[i * 3]]).normal += n;
		(p[indices[i * 3 + 1]]).normal += n;
		(p[indices[i * 3 + 2]]).normal += n;

		sharedArr[indices[i * 3]]++;
		sharedArr[indices[i * 3 + 1]]++;
		sharedArr[indices[i * 3 + 2]]++;
	}

	for (int i = 0; i < vcount; i++)
	{
		p[i].normal = (1.0f / sharedArr[i]) * p[i].normal;
		p[i].normal = Normalized(p[i].normal);
		p[i].normal.w = 0.0f;
	}

	free(sharedArr);
	sharedArr = NULL;
}

bool DrawBox2()
{
	SetRenderState(DEPTHTEST | ZWRITE | CULLBACKFACE | LIGHTING);
	SetPrimitiveType(TRIANGLE_PRIMITIVE);
	vertex vertices[] =
	{
		{ vector4{ -5.0f, -5.0f, -9.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ 5.0f, -5.0f, -9.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ 5.0f, 5.0f, -9.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ -5.0f, 5.0f, -9.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ 5.0f, -5.0f, -11.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ 5.0f, 5.0f, -11.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ -5.0f, -5.0f, -11.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ -5.0f, 5.0f, -11.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} }
	};

	vertexbuffer buffer = {};
	buffer.subdata = vertices;
	buffer.bytes = sizeof(vertex)* 8;

	unsigned short indices[] =
	{
		0, 1, 2,
		0, 2, 3,
		1, 4, 5,
		1, 5, 2,
		4, 6, 7,
		4, 7, 5,
		6, 0, 3,
		6, 3, 7,
		6, 4, 1,
		6, 1, 0,
		3, 2, 5,
		3, 5, 7
	};

	ComputeNormal(&buffer, 8, indices, 36);

	bool result = Pipeline(&buffer, 8, indices, 36, NULL);
	SetRenderState(0);

	return result;
}

bool DrawBox()
{
	SetRenderState(DEPTHTEST | CULLBACKFACE | LIGHTING);
	SetPrimitiveType(TRIANGLE_PRIMITIVE);
	SetBlend(SRC_ALPHA, SRC_ONEMINUS_ALPHA, ADD);

	vertex vertices[] =
	{
		{ vector4{ -5.0f, -5.0f, -3.0f }, colorrgba{ 0, 0, 1.0f, 0.5f }, texcoord{}, vector4{} },
		{ vector4{ 5.0f, -5.0f, -3.0f }, colorrgba{ 0, 1.0f, 0, 0.5f }, texcoord{}, vector4{} },
		{ vector4{ 5.0f, 5.0f, -3.0f }, colorrgba{ 1.0f, 0, 0, 0.5f }, texcoord{}, vector4{} },
		{ vector4{ -5.0f, 5.0f, -3.0f }, colorrgba{ 1.0f, 1.0f, 0, 0.5f }, texcoord{}, vector4{} },
		{ vector4{ 5.0f, -5.0f, -7.0f }, colorrgba{ 0, 0, 1.0f, 0.5f }, texcoord{}, vector4{} },
		{ vector4{ 5.0f, 5.0f, -7.0f }, colorrgba{ 0, 1.0f, 0, 0.5f }, texcoord{}, vector4{} },
		{ vector4{ -5.0f, -5.0f, -7.0f }, colorrgba{ 1.0f, 0, 0, 0.5f }, texcoord{}, vector4{} },
		{ vector4{ -5.0f, 5.0f, -7.0f }, colorrgba{ 1.0f, 1.0f, 0, 0.5f }, texcoord{}, vector4{} }
	};

	vertexbuffer buffer = {};
	buffer.subdata = vertices;
	buffer.bytes = sizeof(vertex)* 8;

	unsigned short indices[] =
	{
		0, 1, 2,
		0, 2, 3,
		1, 4, 5,
		1, 5, 2,
		4, 6, 7,
		4, 7, 5,
		6, 0, 3,
		6, 3, 7,
		6, 4, 1,
		6, 1, 0,
		3, 2, 5,
		3, 5, 7
	};

	ComputeNormal(&buffer, 8, indices, 36);

	bool result = Pipeline(&buffer, 8, indices, 36, NULL);
	SetRenderState(0);

	return result;
}

bool DrawTriangleCone()
{
	SetRenderState(DEPTHTEST | ZWRITE | CULLBACKFACE | TEXTURE | LIGHTING);
	SetPrimitiveType(TRIANGLE_PRIMITIVE);

	vertex vertices[] =
	{
		{ vector4{ 0.0f, 4.0f, 2.0f }, colorrgba{ 0, 0, 1.0f, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ 4.0f, -4.0f, 4.0f }, colorrgba{ 0, 1.0f, 0, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ -4.0f, -4.0f, 4.0f }, colorrgba{ 1.0f, 0, 0, 1.0f }, texcoord{}, vector4{} },
		{ vector4{ 0.0f, -4.0f, 0.0f }, colorrgba{ 1.0f, 1.0f, 0, 1.0f }, texcoord{}, vector4{} }
	};

	vertexbuffer buffer = {};
	buffer.subdata = vertices;
	buffer.bytes = sizeof(vertex)* 4;

	unsigned short indices[] =
	{
		0, 2, 1,
		0, 3, 2,
		0, 1, 3,
		1, 2, 3
	};

	texcoord uvlist[] =
	{
		{ 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f },
		{ 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f },
		{ 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f },
		{ 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f },
	};

	ComputeNormal(&buffer, 4, indices, 12);

	bool result = Pipeline(&buffer, 4, indices, 12, uvlist);
	SetRenderState(0);

	return result;
}

bool Pipeline(vertexbuffer *buffer, int vcount, ushort *indices, int icount, texcoord *uvlist)
{
	bool result = VertexShaderStage(buffer);
	if (!result) return result;

	outputPrimitive.clear();
	result = PrimitiveAssembly(pVSOut, vcount, indices, icount, uvlist);
	if (!result) return result;

	fragments.clear();
	result = RasterizationStage();
	if (!result) return result;

	result = FragmentShaderStage();
	if (!result) return result;

	result = OutMergeStage();
	return result;
}

vector4 WorldTransform(vector4 v)
{
	return Mul(v, world);
}

vector4 ViewTransform(vector4 v)
{
	return Mul(v, view);
}

vector4 ProjectionTransform(vector4 v)
{
	return Mul(v, projection);
}

vector4 ScreenTransform(vector4 v)
{
	int halfW = viewW / 2;
	int halfH = viewH / 2;

	v.x = halfW * v.x + halfW;
	v.y = halfH * v.y + halfH;

	return v;
}

bool VertexShaderStage(vertexbuffer *pBuf)
{
	if (vsOutputBytes != pBuf->bytes)
	{
		if (pVSOut)
		{
			free(pVSOut);
			pVSOut = NULL;
		}

		pVSOut = (vsoutput*)malloc(pBuf->bytes);
		if (!pVSOut) return false;

		vsOutputBytes = pBuf->bytes;
	}

	memcpy(pVSOut, pBuf->subdata, vsOutputBytes);

	bool result = true;
	for (int i = 0, n = pBuf->bytes / sizeof(vsoutput); i < n; i++)
	{
		result &= VertexShader(&pVSOut[i]);
	}

	return result;
}

bool VertexShader(vsoutput *v)
{
	//世界、视图、投影变换
	vector4 r = WorldTransform(v->pos);
	r = ViewTransform(r);
	r = ProjectionTransform(r);

	//per vertex lighting
	if (renderState &LIGHTING)
	{
		vector4 vec = Normalized(lights[1].dir);
		vector4 normal = Normalized(WorldTransform(v->normal));

		float intensity = DotProduct(vec, normal);
		if (intensity < 0.0f) intensity = 0.0f;
		if (intensity > 1.0f) intensity = 1.0f;

		v->color.r *= lights[1].color.r * intensity;
		v->color.g *= lights[1].color.g * intensity;
		v->color.b *= lights[1].color.b * intensity;
	}

	if (CVVCulling(r) != 0) return false;

	v->pos = r;
	return true;
}

int CVVCulling(vector4 v)
{
	float w = v.w;
	int inCVV = 0;
	if (v.x < -w) inCVV |= 1;	//在左平面的左侧
	if (v.x > w) inCVV |= 2;	//在有平面的右侧
	if (v.y < -w) inCVV |= 4;	//下平面的下部
	if (v.y > w) inCVV |= 8;	//上平面的上部
	if (v.z < -w) inCVV |= 16;	//超出近平面
	if (v.z > w) inCVV |= 32;	//超出远平面

	return inCVV;
}

bool BackFaceCulling(triangle* t)
{
	if (!(renderState & CULLBACKFACE)) return false;

	vector4 p0 = t->v1.pos, p1 = t->v2.pos, p2 = t->v3.pos;
	p0 = (1.0f / p0.w) * p0;
	p1 = (1.0f / p1.w) * p1;
	p2 = (1.0f / p2.w) * p2;

	vector4 v0 = p1 - p0;
	vector4 v1 = p2 - p1;

	//vector4 sufNormal = Normalized(CrossProduct(v0, v1));
	//vector4 eyeLook = {0.0f, 0.0f, 1.0f, 0.0f};					//wrong:{ view.m2, view.m6, view.m10, 0.0f };

	return v0.x * v1.y - v0.y * v1.x <= 0.0f;
}

void SetInputLayout(vertexlayout layout[], int num){ }

bool PrimitiveAssembly(vsoutput* v, int vertexCount, unsigned short *indices, int indexCount, texcoord* uvlist)
{
	switch (curPrimitiveType)
	{
	case POINT_PRIMITIVE:
		break;
	case TRIANGLE_PRIMITIVE:
	{
		int num = indexCount / 3;

		if (num > 0)
		{
			for (int i = 0; i < num; i++)
			{
				triangle t;
				t.v1 = v[indices[i * 3]];
				t.v2 = v[indices[i * 3 + 1]];
				t.v3 = v[indices[i * 3 + 2]];

				if (BackFaceCulling(&(t)))
					continue;

				vector4 v = t.v1.pos;
				v.x /= v.w;
				v.y /= v.w;
				t.v1.pos = ScreenTransform(v);
				RoundVertex(&(t.v1));

				v = t.v2.pos;
				v.x /= v.w;
				v.y /= v.w;
				t.v2.pos = ScreenTransform(v);
				RoundVertex(&(t.v2));

				v = t.v3.pos;
				v.x /= v.w;
				v.y /= v.w;
				t.v3.pos = ScreenTransform(v);
				RoundVertex(&(t.v3));

				if (uvlist)
				{
					t.v1.uv = uvlist[i * 3];
					t.v2.uv = uvlist[i * 3 + 1];
					t.v3.uv = uvlist[i * 3 + 2];
				}
				outputPrimitive.push_back(t);
			}
		}
	}
		break;
	case TRIANGLELIST_PRIMITIVE:
		break;
	default:
		break;
	}

	return true;
}

void SetPrimitiveType(int type)
{
	curPrimitiveType = type;
}

void RoundVertex(vertex* v)
{
	v->pos.x = (int)(v->pos.x + 0.5f);
	v->pos.y = (int)(v->pos.y + 0.5f);
}

bool RasterizationStage()
{
	switch (curPrimitiveType)
	{
	case POINT_PRIMITIVE:
		break;
	case TRIANGLE_PRIMITIVE:
	{
		for (int i = 0, n = outputPrimitive.size(); i < n; i++)
		{
			triangle output[] = { {}, {} };
			int num = SplitTriangle(&outputPrimitive[i], output);

			for (int i = 0; i < num; i++)
			{
				RasterTriangle(&output[i]);
			}
		}
	}
		break;
	case TRIANGLELIST_PRIMITIVE:
		break;
	default:
		break;
	}

	return true;
}

void VertexInterpolate(vertex *v1, vertex *v2, vertex *newv, float t)
{
	newv->pos.x = v1->pos.x * t + (1 - t) * v2->pos.x;
	newv->pos.y = v1->pos.y * t + (1 - t) * v2->pos.y;

	//深度的倒数满足线性插值
	float w = t * (1 / -v1->pos.w) + (1 - t) / -v2->pos.w;
	w = 1.0f / w;
	newv->pos.w = -w;

	//texcoord
	newv->uv.u = (v1->uv.u * t / -v1->pos.w + v2->uv.u * (1 - t) / -v2->pos.w) * w;
	newv->uv.v = (v1->uv.v * t / -v1->pos.w + v2->uv.v * (1 - t) / -v2->pos.w) * w;

	//vertex color
	newv->color.r = (v1->color.r * t / -v1->pos.w + v2->color.r * (1 - t) / -v2->pos.w) * w;
	newv->color.g = (v1->color.g * t / -v1->pos.w + v2->color.g * (1 - t) / -v2->pos.w) * w;
	newv->color.b = (v1->color.b * t / -v1->pos.w + v2->color.b * (1 - t) / -v2->pos.w) * w;
	newv->color.a = (v1->color.a * t / -v1->pos.w + v2->color.a * (1 - t) / -v2->pos.w) * w;

	RoundVertex(newv);
}

int SplitTriangle(triangle *input, triangle *output)
{
	vertex p0, p1, p2, p;
	p0 = input->v1;
	p1 = input->v2;
	p2 = input->v3;

	//p0, p1, p2依Y值从大大小
	if (p1.pos.y > p0.pos.y) { p = p1; p1 = p0; p0 = p; }
	if (p2.pos.y > p0.pos.y) { p = p2; p2 = p0; p0 = p; }
	if (p2.pos.y > p1.pos.y) { p = p2; p2 = p1; p1 = p; }

	//三点共线的情形暂不处理
	{
	}

	//平顶三角形
	if (p0.pos.y == p1.pos.y || p1.pos.y == p2.pos.y)
	{
		//不用拆分，直接拷贝三角形
		int len = sizeof(vertex);
		memcpy(&output->v1, &p0, len);
		memcpy(&output->v2, &p1, len);
		memcpy(&output->v3, &p2, len);

		return 1;
	}

	/*从p1点发出水平射线拆分三角形.
	*/
	vertex v = {};
	float t = (p1.pos.y - p2.pos.y) / (p0.pos.y - p2.pos.y);
	VertexInterpolate(&p0, &p2, &v, t);

	if (p1.pos.x <= p2.pos.x)
	{
		output[0].v1 = p0;
		output[0].v2 = p1;
		output[0].v3 = v;

		output[1].v1 = v;
		output[1].v2 = p1;
		output[1].v3 = p2;
	}
	else
	{
		output[0].v1 = p0;
		output[0].v2 = v;
		output[0].v3 = p1;

		output[1].v1 = p1;
		output[1].v2 = v;
		output[1].v3 = p2;
	}

	return 2;
}

void RasterTriangle(triangle *t)
{
	vertex p0 = t->v1, p1 = t->v2, p2 = t->v3, p;

	//v1的Y坐标最大，扫描线从上往下
	int stepY = p0.pos.y - p2.pos.y;
	int scanY = p0.pos.y;

	for (int i = 0; i < stepY; scanY--, i++)
	{
		//获取扫描线与三角形的两个交点
		vertex intersect1 = {};
		vertex intersect2 = {};
		float ty = (scanY - p2.pos.y) / stepY;

		if (scanY != p0.pos.y && scanY != p2.pos.y)
		{
			if (p0.pos.y == p1.pos.y)	//平顶三角形
			{
				VertexInterpolate(&p0, &p2, &intersect1, ty);
				VertexInterpolate(&p1, &p2, &intersect2, ty);
			}
			else
			{
				VertexInterpolate(&p0, &p1, &intersect1, ty);
				VertexInterpolate(&p0, &p2, &intersect2, ty);
			}
		}
		else if (scanY == p0.pos.y)
		{	
			if (scanY == p1.pos.y)		//平顶三角形
			{
				intersect1 = p0;
				intersect2 = p1;
			}
			else						//平底三角形
			{
				intersect1 = p0;
				intersect2 = p0;
			}
		}
		else
		{
			if (scanY != p1.pos.y)		//平顶三角形
			{
				intersect1 = p2;
				intersect2 = p2;
			}
			else						//平底三角形
			{
				intersect1 = p1;
				intersect2 = p2;
			}
		}

		if (intersect1.pos.x > intersect2.pos.x)
		{
			p = intersect2;
			intersect2 = intersect1;
			intersect1 = p;
		}

		int stepX = intersect2.pos.x - intersect1.pos.x + 1;
		//if (stepX <= 1)
		//{

		//}
		//else
		{
			for (int j = 0; j < stepX; j++)
			{
				float tx = 1.0f * j / (stepX - 1);

				VertexInterpolate(&intersect1, &intersect2, &p, tx);
				fragments.push_back({ p.pos.x, p.pos.y, p.pos.w, p.color, p.uv });
			}
		}
	}
}

bool FragmentShaderStage()
{
	if (renderState & TEXTURE)
	{
		int w = pTexture->width;
		int h = pTexture->height;
		int bytes = 0;

		switch (pTexture->format)
		{
		case R32G32B32_FORMAT:
			bytes = 3;
			break;
		default:
			break;
		}

		for (int i = 0; i < fragments.size(); i++)
		{
			int u = fragments[i].uv.u * (w - 1);
			int v = fragments[i].uv.v * (h - 1);
			uint color = pTexture->data[v * w + u];

			//Texture Modulate(color * texture color)
			fragments[i].color.r = (float)((uchar)(color >> 16)) / 255.0f;
			fragments[i].color.g = (float)((uchar)(color >> 8)) / 255.0f;
			fragments[i].color.b = (float)((uchar)color) / 255.0f;
			fragments[i].color.a = 1.0f;
		}
	}

	return true;
}

void SetRenderState(int state)
{
	if (state == 0)
		renderState = 0;
	else
		renderState |= state;
}

colorrgba ComputeFactor(colorrgba &factor, blendfactor bf)
{
	switch (bf)
	{
	case ZERO:
		factor.r = 0.0f; factor.g = 0.0f; factor.b = 0.0f; factor.a = 0.0f;
		break;
	case ONE:
		factor.r = 1.0f; factor.g = 1.0f; factor.b = 1.0f; factor.a = 1.0f;
		break;
	case SRC_COLOR:
	case DST_COLOR:
		break;
	case SRC_ONEMINUS_COLOR:
	case DST_ONEMINUS_COLOR:
		factor.r = 1.0f - factor.r;
		factor.g = 1.0f - factor.g;
		factor.b = 1.0f - factor.b;
		factor.a = 1.0f - factor.a;
		break;
	case SRC_ALPHA:
	case DST_ALPHA:
		factor.r = factor.a; factor.g = factor.a; factor.b = factor.a;
		break;
	case SRC_ONEMINUS_ALPHA:
	case DST_ONEMINUS_ALPHA:
		factor.r = 1.0f - factor.a;
		factor.g = 1.0f - factor.a;
		factor.b = 1.0f - factor.a;
		factor.a = 1.0f - factor.a;
		break;
	default:
		break;
	}

	return factor;
}


blendfactor srcFactor;
blendfactor dstFactor;
colorrgba(*BlendFunc)(colorrgba&, colorrgba&);

colorrgba Add(colorrgba &src, colorrgba &dst)
{
	colorrgba sf = src, df = dst;
	if (srcFactor == DST_ALPHA
		|| srcFactor == DST_COLOR
		|| srcFactor == DST_ONEMINUS_ALPHA
		|| srcFactor == DST_ONEMINUS_COLOR)
	{
		sf = df;
	}
	ComputeFactor(sf, srcFactor);
	
	if (dstFactor == SRC_ALPHA
		|| dstFactor == SRC_COLOR
		|| dstFactor == SRC_ONEMINUS_ALPHA
		|| dstFactor == SRC_ONEMINUS_COLOR
		)
	{
		df = sf;
	}
	ComputeFactor(df, dstFactor);

	return (src * sf) + (dst * df);
}

colorrgba Subtract(colorrgba &src, colorrgba &dst)
{
	colorrgba sf = ComputeFactor(src, srcFactor);
	colorrgba df = ComputeFactor(dst, dstFactor);

	return (dst * df) - (src * sf);
}

void SetBlend(blendfactor sf, blendfactor df, blendoperation op)
{
	renderState |= BLEND;

	srcFactor = sf;
	dstFactor = df;

	switch (op)
	{
	case ADD:
		BlendFunc = Add;
		break;
	case SUBTRACT:
		BlendFunc = Subtract;
		break;
	case REVERSESUBTRACT:
		break;
	case MIN:
		break;
	case MAX:
		break;
	default:
		break;
	}
}

bool OutMergeStage()
{
	uchar r, g, b, a;

	for each (fragment f in fragments)
	{
		colorrgba dst = f.color;
		int index = (int)f.y * viewW + (int)f.x;
		if (renderState & DEPTHTEST)
		{
			float z = pZBuffer[index];

			if (f.z <= pZBuffer[index])
			{
				if (renderState & ZWRITE) pZBuffer[index] = f.z;

				if (renderState & BLEND)
				{
					dst.r = (uchar)(pFrameBuffer[index] >> 16) / 255.0f;
					dst.g = (uchar)(pFrameBuffer[index] >> 8) / 255.0f;
					dst.b = (uchar)(pFrameBuffer[index]) / 255.0f;
					dst.a = (uchar)(pFrameBuffer[index] >> 24) / 255.0f;

					dst = (*BlendFunc)(f.color, dst);
				}

				r = (uchar)(dst.r * 255);
				g = (uchar)(dst.g * 255);
				b = (uchar)(dst.b * 255);
				a = (uchar)(dst.a * 255);
				pFrameBuffer[index] = (a << 24) | (r << 16) | (g << 8) | (b);
			}
		}
		else
		{
			if (renderState & ZWRITE) pZBuffer[index] = f.z;

			dst = f.color;
			if (renderState & BLEND)
			{
				dst.r = (uchar)(pFrameBuffer[index] >> 16) / 255.0f;
				dst.g = (uchar)(pFrameBuffer[index] >> 8) / 255.0f;
				dst.b = (uchar)(pFrameBuffer[index]) / 255.0f;

				dst = BlendFunc(f.color, dst);
			}

			r = (uchar)(dst.r * 255);
			g = (uchar)(dst.g * 255);
			b = (uchar)(dst.b * 255);
			pFrameBuffer[index] = (r << 16) | (g << 8) | (b);
		}
	}

	return true;
}

bool SetTexture(_bitmapfile bmp)
{
	if (pTexture) free(pTexture);

	pTexture = (texture*)malloc(sizeof(texture));
	if (!pTexture) return false;
	pTexture->data = NULL;

	pTexture->width = bmp.bih.biWidth;
	pTexture->height = bmp.bih.biHeight;

	int num = bmp.bih.biWidth * bmp.bih.biHeight * 4;

	pTexture->data = (uint*)malloc(num);
	if (!pTexture->data) return false;
	memcpy(pTexture->data, bmp.lpData, num);

	return true;
}

bool SetLight(int index, light l, bool enable)
{
	if (index > 7) return false;

	lights[index] = l;
	return true;
}

_lpbitmapfile LoadBitmap(LPTSTR lpszBitmapFile)
{
	_lpbitmapfile bitmapFile = NULL;

	// Check for valid bitmap file path
	if (lpszBitmapFile != NULL)
	{
		// Open bitmap file
		FILE* file = _tfopen(lpszBitmapFile, _T("rb"));
		if (file != NULL)
		{
			bitmapFile = new _bitmapfile;

			// Read BITMAPFILEHEADER info
			fread(&bitmapFile->bfh, 1, sizeof(BITMAPFILEHEADER), file);

			// Read BITMAPINFOHEADER info
			fread(&bitmapFile->bih, 1, sizeof(BITMAPINFOHEADER), file);

			// Calculate pitch
			int iBpp = bitmapFile->bih.biBitCount >> 3;
			int iPitch = iBpp * bitmapFile->bih.biWidth;
			while ((iPitch & 3) != 0)
				iPitch++;

			// Check for bit-depth (8, 16, 24 and 32bpp only)
			if (bitmapFile->bih.biBitCount >= 8)
			{
				RGBQUAD lpPalette[256];
				if (bitmapFile->bih.biBitCount == 8)
				{
					// Calculate palette entries
					DWORD dwPaletteEntries = bitmapFile->bih.biClrUsed;
					if (dwPaletteEntries == 0)
						dwPaletteEntries = 256;

					// Read palette info
					fread(lpPalette, dwPaletteEntries, sizeof(RGBQUAD), file);
				}

				// Read image data
				DWORD dwSize = iPitch * bitmapFile->bih.biHeight;
				LPBYTE lpData = (LPBYTE)malloc(dwSize*sizeof(BYTE));
				fread(lpData, dwSize, sizeof(BYTE), file);

				// Create texture bitmap
				long _bpp = (_BITS_PER_PIXEL >> 3);
				long _pitch = bitmapFile->bih.biWidth * _bpp;
				bitmapFile->dwSize = bitmapFile->bih.biHeight * _pitch;
				bitmapFile->lpData = (LPDWORD)malloc(bitmapFile->dwSize*sizeof(DWORD));

				// Convert bitmap to 32bpp
				DWORD dwDstHorizontalOffset;
				DWORD dwDstVerticalOffset = 0;
				DWORD dwDstTotalOffset;
				DWORD dwSrcHorizontalOffset;
				DWORD dwSrcVerticalOffset = 0;
				DWORD dwSrcTotalOffset;
				for (long i = 0; i < bitmapFile->bih.biHeight; i++)
				{
					dwDstHorizontalOffset = 0;
					dwSrcHorizontalOffset = 0;
					for (long j = 0; j < bitmapFile->bih.biWidth; j++)
					{
						// Update destination total offset
						dwDstTotalOffset = dwDstVerticalOffset + dwDstHorizontalOffset;

						// Update source total offset
						dwSrcTotalOffset = dwSrcVerticalOffset + dwSrcHorizontalOffset;

						// Update bitmap
						switch (bitmapFile->bih.biBitCount)
						{
						case 8:
						{
								  BYTE red = lpPalette[lpData[dwSrcTotalOffset]].rgbRed;
								  BYTE green = lpPalette[lpData[dwSrcTotalOffset]].rgbGreen;
								  BYTE blue = lpPalette[lpData[dwSrcTotalOffset]].rgbBlue;
								  bitmapFile->lpData[dwDstTotalOffset >> 2] = _RGB(red, green, blue);
						}
							break;

						case 16:
						{
								   LPWORD lpSrcData = (LPWORD)lpData;
								   BYTE red = (lpSrcData[dwSrcTotalOffset >> 1] & 0x7C00) >> 10;
								   BYTE green = (lpSrcData[dwSrcTotalOffset >> 1] & 0x03E0) >> 5;
								   BYTE blue = lpSrcData[dwSrcTotalOffset >> 1] & 0x001F;
								   bitmapFile->lpData[dwDstTotalOffset >> 2] = _RGB(red, green, blue);
						}
							break;

						case 24:
						{
								   bitmapFile->lpData[dwDstTotalOffset >> 2] = _RGB(lpData[dwSrcTotalOffset + 2], lpData[dwSrcTotalOffset + 1], lpData[dwSrcTotalOffset]);
						}
							break;

						case 32:
						{
								   LPDWORD lpSrcData = (LPDWORD)lpData;
								   bitmapFile->lpData[dwDstTotalOffset >> 2] = lpSrcData[dwSrcTotalOffset >> 2];
						}
							break;
						}

						// Update destination horizontal offset
						dwDstHorizontalOffset += _bpp;

						// Update source horizontal offset
						dwSrcHorizontalOffset += iBpp;
					}

					// Update destination vertical offset
					dwDstVerticalOffset += _pitch;

					// Update source vertical offset
					dwSrcVerticalOffset += iPitch;
				}

				// Free image data
				free(lpData);
			}

			// Close .BMP file
			fclose(file);
		}
	}

	return bitmapFile;
}