// ColorSpace.cpp : ����Ӧ�ó������ڵ㡣
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

// ȫ�ֱ���: 
HINSTANCE hInst;								// ��ǰʵ��
TCHAR szTitle[MAX_LOADSTRING];					// �������ı�
TCHAR szWindowClass[MAX_LOADSTRING];			// ����������

//�豸���
HWND						g_hWnd;
HDC							g_hDeviceDC;
HDC							g_hMemDC;
HBITMAP						g_hBMP;

int							viewW = 640;
int							viewH = 480;

BitmapFile					g_Bitmap;

// �˴���ģ���а����ĺ�����ǰ������: 
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

	// TODO:  �ڴ˷��ô��롣
	MSG msg{0};
	HACCEL hAccelTable;

	// ��ʼ��ȫ���ַ���
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_COLORSPACE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ִ��Ӧ�ó����ʼ��: 
	if (!InitInstance (hInstance, viewW, viewH, nCmdShow))
	{
		return FALSE;
	}

	if (!Initialize(viewW, viewH))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COLORSPACE));

	// ����Ϣѭ��: 
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
	//��ȡ��ĻDC���ڴ�DC
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

	//����DIB
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
	//�ڴ�DC��һ��λͼ
	HBITMAP hOldBackBitmap = (HBITMAP)::SelectObject(g_hMemDC, g_hBMP);

	//BitBlit hMemDC--->hDeviceDC, �ڴ�DC--->��ĻDC
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

	//��Ҫд����ļ�
	hFile = CreateFile(szBmpFile,
		GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}

	//д���ļ�ͷ
	int bfhSize = sizeof(BITMAPFILEHEADER);
	bSuccess = WriteFile(hFile, pbmfh, bfhSize, &dwBytesWrite, NULL);

	//д��λͼ��Ϣ
	dwInfoSize = pbmfh->bfOffBits - bfhSize;
	bSuccess = WriteFile(hFile, pbmi, dwInfoSize, &dwBytesWrite, NULL);

	//д��λͼ����
	dwBitsSize = pbmfh->bfSize - pbmfh->bfOffBits;
	bSuccess = WriteFile(hFile, pBits, dwBitsSize, &dwBytesWrite, NULL);
	CloseHandle(hFile);

	return TRUE;
}


//
//  ����:  MyRegisterClass()
//
//  Ŀ��:  ע�ᴰ���ࡣ
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
//   ����:  InitInstance(HINSTANCE, int)
//
//   Ŀ��:  ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int w, int h, int nCmdShow)
{
   g_hWnd;

   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

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
//  ����:  WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND	- ����Ӧ�ó���˵�
//  WM_PAINT	- ����������
//  WM_DESTROY	- �����˳���Ϣ������
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
		// �����˵�ѡ��: 
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
		// TODO:  �ڴ���������ͼ����...
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