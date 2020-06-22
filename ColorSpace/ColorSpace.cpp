// ColorSpace.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "ColorSpace.h"
#include <Windows.h>
#include <fstream>
#include <stdio.h>

#define MAX_LOADSTRING 100
#define BITMAP_WIDTH 4096
#define BITMAP_HEIGHT 4096
#define BITMAP_DATASIZE (BITMAP_WIDTH * BITMAP_HEIGHT * 3)


typedef struct BitmapFile
{
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	BYTE *lpData;
};

struct ColorRGB
{
	BYTE r, g, b;
};

// 全局变量: 
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

//设备相关
HWND						g_hWnd;
HDC							g_hDeviceDC;
HDC							g_hMemDC;
HBITMAP						g_hBMP;

int							viewW = 640;
int							viewH = 480;

BitmapFile					g_Bitmap;

// 此代码模块中包含的函数的前向声明: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int, int, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

//
bool				Initialize(int w, int h);
void				Destroy();

void				InitialBitmap();
void				SplitRGB();
void				SaveBitmap(char *);
BOOL				DibSeqSave(PTSTR szBmpFile, PBITMAPFILEHEADER pbmfh, PBITMAPINFO pbmi, PBYTE pBits);


void				Draw();

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO:  在此放置代码。
	MSG msg{0};
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_COLORSPACE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance (hInstance, viewW, viewH, nCmdShow))
	{
		return FALSE;
	}

	if (!Initialize(viewW, viewH))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COLORSPACE));

	// 主消息循环: 
	while (WM_QUIT != msg.message)
	{
		Draw();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Destroy();

	return (int) msg.wParam;
}

bool Initialize(int w, int h)
{
	bool result = true;
	//获取屏幕DC和内存DC
	g_hDeviceDC = GetDC(g_hWnd);
	result = g_hDeviceDC != NULL;
	if (!result)
	{
		return result;
	}

	g_hMemDC = CreateCompatibleDC(g_hDeviceDC);
	result = g_hMemDC != NULL;
	if (!result)
	{
		return result;
	}

	//创建DIB
	InitialBitmap();

	BITMAPINFO bmInfo;
	memcpy(&bmInfo.bmiHeader, &g_Bitmap.bih, sizeof(BITMAPINFOHEADER));

	g_hBMP = CreateDIBSection(g_hDeviceDC, &bmInfo, DIB_RGB_COLORS, (void**)&g_Bitmap.lpData , NULL, 0);
	result = g_hBMP != NULL;
	if (!result)
	{
		return result;
	}

	SplitRGB();
	SaveBitmap("test1.bmp");

	return result;
}

void Destroy()
{
	DeleteObject(g_hBMP);
	DeleteObject(g_hMemDC);

	ReleaseDC(NULL, g_hDeviceDC);
}

void InitialBitmap()
{
	int bfhSize = sizeof(BITMAPFILEHEADER);
	int bihSize = sizeof(BITMAPINFOHEADER);

	memset(&g_Bitmap.bfh, 0, bfhSize);
	g_Bitmap.bfh.bfType = 0x4D42;
	g_Bitmap.bfh.bfOffBits = bfhSize + bihSize;
	g_Bitmap.bfh.bfSize = g_Bitmap.bfh.bfOffBits + BITMAP_DATASIZE;

	memset(&g_Bitmap.bih, 0, bihSize);
	g_Bitmap.bih.biSize = bihSize;
	g_Bitmap.bih.biWidth = BITMAP_WIDTH;
	g_Bitmap.bih.biHeight = BITMAP_HEIGHT;
	g_Bitmap.bih.biPlanes = 1;
	g_Bitmap.bih.biBitCount = 24;
}

void Draw()
{
	//内存DC绑定一块位图
	HBITMAP hOldBackBitmap = (HBITMAP)::SelectObject(g_hMemDC, g_hBMP);

	//BitBlit hMemDC--->hDeviceDC, 内存DC--->屏幕DC
	bool result = BitBlt(g_hDeviceDC, 0, 0, viewW, viewH, g_hMemDC, 0, 0, SRCCOPY);
	if (!result) return;

	//clear
	SelectObject(g_hMemDC, hOldBackBitmap);
}

void SplitRGB()
{
	if (g_Bitmap.lpData == NULL)
	{
		g_Bitmap.lpData = (BYTE*)malloc(BITMAP_DATASIZE);
		memset(g_Bitmap.lpData, 0, BITMAP_DATASIZE);
	}

	int pixBytes = sizeof(ColorRGB);
	int rowBytes = BITMAP_WIDTH * pixBytes;

	BYTE* data = g_Bitmap.lpData;

	for (int row = 0; row < BITMAP_HEIGHT; row++)
	{
		int redRow = row / 256;
		BYTE blue = row % 256;

		for (int col = 0; col < BITMAP_WIDTH; col++)
		{
			int redCol = col / 256;

			BYTE red = redRow * 16 + redCol;
			BYTE green = (col % 256);

			data[0] = blue;
			data[1] = green;
			data[2] = red;
			data += pixBytes;
		}
	}
}

void SaveBitmap(char *pFilename)
{
	std::ofstream ofs("test1.bmp", std::ios_base::binary);
	ofs.write((char*)(&g_Bitmap.bfh), sizeof(BITMAPFILEHEADER));
	ofs.write((char*)(&g_Bitmap.bih), sizeof(BITMAPINFOHEADER));
	ofs.write((char*)(g_Bitmap.lpData), BITMAP_DATASIZE);
	ofs.close();

	//FILE *pFile = fopen(pFilename, "wb");
	//if (pFile == NULL)
	//	return;
	//fwrite(&g_Bitmap.bfh, sizeof(BITMAPFILEHEADER), 1, pFile);
	//fwrite(&g_Bitmap.bih, sizeof(BITMAPINFOHEADER), 1, pFile);
	//fwrite(g_Bitmap.lpData, BITMAP_DATASIZE, 1, pFile);
	//fclose(pFile);
}

BOOL DibSeqSave(PTSTR szBmpFile, PBITMAPFILEHEADER pbmfh, PBITMAPINFO pbmi, PBYTE pBits)
{
	BOOL		bSuccess;
	DWORD		dwBytesWrite, dwInfoSize, dwBitsSize;
	HANDLE		hFile;

	//打开要写入的文件
	hFile = CreateFile(szBmpFile,
		GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}

	//写入文件头
	int bfhSize = sizeof(BITMAPFILEHEADER);
	bSuccess = WriteFile(hFile, pbmfh, bfhSize, &dwBytesWrite, NULL);

	//写入位图信息
	dwInfoSize = pbmfh->bfOffBits - bfhSize;
	bSuccess = WriteFile(hFile, pbmi, dwInfoSize, &dwBytesWrite, NULL);

	//写入位图数据
	dwBitsSize = pbmfh->bfSize - pbmfh->bfOffBits;
	bSuccess = WriteFile(hFile, pBits, dwBitsSize, &dwBytesWrite, NULL);
	CloseHandle(hFile);

	return TRUE;
}


//
//  函数:  MyRegisterClass()
//
//  目的:  注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COLORSPACE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_COLORSPACE);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数:  InitInstance(HINSTANCE, int)
//
//   目的:  保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int w, int h, int nCmdShow)
{
   g_hWnd;

   hInst = hInstance; // 将实例句柄存储在全局变量中

   RECT rc = RECT{ 0, 0, w, h };
   g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	   CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

   if (!g_hWnd)
   {
      return FALSE;
   }

   ShowWindow(g_hWnd, nCmdShow);
   UpdateWindow(g_hWnd);

   return TRUE;
}

//
//  函数:  WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择: 
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO:  在此添加任意绘图代码...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}