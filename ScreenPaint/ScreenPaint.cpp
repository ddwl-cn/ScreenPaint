#include <windows.h>
#include <vector>
#include <algorithm>
/*
可能的优化：
创建几张位图 各自绘制 互不影响 最后在叠加绘制到屏幕上 类似ps图层结构

一些不足：
在划线过程中鼠标快速移动会出现明显的 “折线” 原因可能是鼠标坐标获取不及时导致（鼠标坐标跳跃）
通过观察 windows 自带的画图工具同样也会出现这样的状况 但只有鼠标移动速度极快的时候才会出现

当然擦除同样也会出现 “跳跃” 的情况

有生之年在想办法吧~~
*/
void CreatCanvas(HDC hdc, HDC &hdcBuffer, HBITMAP &BitMap) {//为指定设备创建 画布
	hdcBuffer = CreateCompatibleDC(hdc);
	BitMap = CreateCompatibleBitmap(hdc, GetSystemMetrics(SM_CXSCREEN)*1.25, GetSystemMetrics(SM_CYSCREEN)*1.25);

	SelectObject(hdcBuffer, BitMap);

	BitBlt(hdcBuffer, 0, 0, GetSystemMetrics(SM_CXSCREEN)*1.25, GetSystemMetrics(SM_CYSCREEN)*1.25, hdc, 0, 0, SRCCOPY);//将背景拷贝到画布上
};
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevinstace, PSTR szCmdline, int iCmdShow) {
	/*变量声明*/
	WNDCLASS wc;
	static TCHAR *szAppName = TEXT("Screen_Paint");
	HWND hwnd = NULL;
	MSG msg;
	int nScreenWidth, nScreenHeight;
	/*获取屏幕尺寸*/
	nScreenWidth = GetSystemMetrics(SM_CXSCREEN) * 1.25;
	nScreenHeight = GetSystemMetrics(SM_CYSCREEN) * 1.25;
	/*窗口类属性填充 随后进行窗口注册*/
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_CROSS);				//设置鼠标为十字
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szAppName;
	/*失败则进行提示*/
	if (!RegisterClass(&wc)) {
		MessageBox(NULL, TEXT("该程序只能在windows NT下运行！"), szAppName, MB_ICONERROR);
		return 0;
	}
	/*进一步设置窗口样式*/
	hwnd = CreateWindowEx(WS_EX_LAYERED, szAppName,    //WS_EX_LAYERED 分层窗口（透明窗口）
		TEXT("屏幕画笔-Screen_Paint"),
		WS_POPUP, 0, 0, nScreenWidth, nScreenHeight,   //WS_POPUP 弹出风格的对话框 只有当对这个窗口的操作完成后才能进行其他操作
		NULL,
		NULL, hInstance,
		NULL);
	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);
	/*消息循环*/
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int nScreenWidth, nScreenHeight;
	long xPos, yPos;
	HDC hdc;
	static HDC hdcBuffer, auxiliaryBuffer;
	static HBITMAP BitMap, auxiliaryBitMap;
	static POINT pts[10000];
	static RECT erase = { 0,0,0,0 };
	static std::vector<HPEN> Pen;
	static HPEN hPenCur;
	static int i = 0;
	static int size = 20;
	static int leftDown = 0;
	static int rightDown = 0;

	/*至于为什么要乘1.25  是因为屏幕有放缩比例  我的屏幕放大125%*/
	nScreenWidth = GetSystemMetrics(SM_CXSCREEN) * 1.25;
	nScreenHeight = GetSystemMetrics(SM_CYSCREEN) * 1.25;

	switch (message) {
	case WM_CREATE:
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

		for (int i = 0; i <= 255; i += 255)
			for (int j = 0; j <= 255; j += 255)
				for (int k = 0; k <= 255; k += 255) {
					Pen.push_back(CreatePen(PS_SOLID, 3, RGB(i, j, k)));
				}

		hPenCur = Pen[4];//默认红色
		/*创建画布*/
		hdc = GetDC(NULL);
		CreatCanvas(hdc, hdcBuffer, BitMap);
		//CreatCanvas(hdc, auxiliaryBuffer, auxiliaryBitMap); 暂未用到
		ReleaseDC(NULL, hdc);
		return 0;
	case WM_KEYDOWN:
		/*换色 - 退出*/
		switch (wParam) {
		case 'D'://Dark-黑
			hPenCur = Pen[0];
			break;
		case 'B'://Blue-蓝
			hPenCur = Pen[1];
			break;
		case 'G'://Green-绿
			hPenCur = Pen[2];
			break;
		case 'C'://Cyan-青
			hPenCur = Pen[3];
			break;
		case 'R'://Red-红
			hPenCur = Pen[4];
			break;
		case 'P'://Pink-粉
			hPenCur = Pen[5];
			break;
		case 'Y'://Yellow-黄
			hPenCur = Pen[6];
			break;
		case 'W'://White-白
			hPenCur = Pen[7];
			break;
		case VK_ESCAPE://ESC-退出
			PostQuitMessage(0);
		}
		return 0;
	case WM_MOUSEMOVE:
		/*移动过程*/

		if (leftDown != 0) {//左键绘制
			xPos = LOWORD(lParam) * 1.25;
			yPos = HIWORD(lParam) * 1.25;
			pts[i].x = xPos;
			pts[i++].y = yPos;
			/*发送绘制消息 但不擦除背景*/
			InvalidateRect(hwnd, NULL, FALSE);
		}
		else if (rightDown != 0) {//右键擦除
			xPos = LOWORD(lParam) * 1.25;
			yPos = HIWORD(lParam) * 1.25;
			erase = { max(xPos - size, 0), max(yPos - size, 0), min(xPos + size, nScreenWidth), min(yPos + size, nScreenHeight) };
			InvalidateRect(hwnd, NULL, FALSE);
		}

		return 0;
	case WM_MOUSEWHEEL:
		if (GetAsyncKeyState(VK_CONTROL) & 0X8000) {//Ctrl + 滚轮 改变橡皮擦尺寸
			/*wParam 高位代表着滚轮滚动消息*/
			short val = HIWORD(wParam);
			if (val > 0)
				size = min(size + 2, 50);
			else if (val < 0)
				size = max(size - 2, 1);
		}
		return 0;
	case WM_LBUTTONDOWN://左键绘画
		/*开始操作*/
		leftDown = 1;
		i = 0;
		return 0;
	case WM_LBUTTONUP:
		/*结束操作*/
		leftDown = 0;
		return 0;
	case WM_RBUTTONDOWN://右键擦除
		rightDown = 1;
		return 0;
	case WM_RBUTTONUP:
		rightDown = 0;
		erase = { 0,0,0,0 };
		return 0;
	case WM_PAINT:
		/*绘制*/
		if (leftDown != 0) {//绘制
			hdc = GetDC(NULL);

			SelectObject(hdc, hPenCur);
			Polyline(hdc, pts, i);
			ReleaseDC(NULL, hdc);

			ValidateRect(hwnd, NULL);
		}
		else if (rightDown != 0) {//擦除
			hdc = GetDC(NULL);
			//BitBlt(hdc, erase.left, erase.top, erase.right, erase.bottom, hdcBuffer, 0, 0, SRCCOPY); //没太搞懂BitBlt 是怎么进行区域拷贝的  

			StretchBlt(hdc, erase.left, erase.top, erase.right - erase.left, erase.bottom - erase.top, hdcBuffer, erase.left, erase.top, erase.right - erase.left, erase.bottom - erase.top, SRCCOPY);

			ValidateRect(hwnd, NULL);
		}
		return 0;
	case WM_DESTROY:
		/*删除响应环境句柄和对象 并退出*/
		ReleaseDC(NULL, hdcBuffer);
		DeleteObject(BitMap);
		//ReleaseDC(NULL, auxiliaryBuffer);
		//DeleteObject(auxiliaryBitMap);
		for (int i = 0; i < Pen.size(); i++)
			DeleteObject(Pen[i]);
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}