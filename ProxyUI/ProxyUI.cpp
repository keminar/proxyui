// ProxyUI.cpp : 定义应用程序的入口点。
// win32文档 https://docs.microsoft.com/en-us/windows/win32/api/

#include "stdafx.h"
#include "ProxyUI.h"
#include <ShellAPI.h>
#include <Shlwapi.h>
#include "commctrl.h"
#include <windows.h>
#include "wininet.h"
#include <atlstr.h>
#include <Commdlg.h>
#include <TlHelp32.h>
#include <sstream>
#include <string.h>
#include <strsafe.h>

#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
WCHAR ProxyText[255] = { 0 }; // 代理变量
WCHAR lanName[255] = { 0 }; // 网络连接
WCHAR filePath[MAX_PATH];//程序路径
CString dirPath; //程序所在目录
CString iniFile; //ini文件路径
WCHAR startCmdLine[MAX_PATH]; //启动参数
BOOL initFormFlag = FALSE; //初始化Form标记
BOOL firstTray = TRUE; //首次最小化

PROCESS_INFORMATION pro_info; //进程信息 
PROCESS_INFORMATION pro_info2; //进程信息  

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")

#define WM_CLICKBIT (WM_USER + 1)

HWND hWndComboBox, hWndBtn1;
HWND hfDlg;

#define CloseProxy TEXT("无代理")
#define RegRun L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RegName L"ProxyUI"
#define IniName L"ProxyUI.ini"
//读取最大长度
#define maxLen 1024

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PROXYUI, szWindowClass, MAX_LOADSTRING);

	//得到程序本身路径
	GetModuleFileName(NULL, filePath, MAX_PATH);
	dirPath = filePath;
	dirPath = dirPath.Left(dirPath.ReverseFind(TEXT('\\'))) + "\\";
	iniFile = dirPath + IniName;
	wcscpy_s(startCmdLine, _countof(startCmdLine), lpCmdLine);

    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROXYUI));

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROXYUI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PROXYUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 660, 530, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // 上次进程ID
   UINT pid = GetPrivateProfileInt(TEXT("ProxyUI"), TEXT("pid"), 0, iniFile);
   if (pid > 0) {
	   TCHAR inBuf[MAX_LOADSTRING];
	   GetPrivateProfileString(TEXT("ProxyUI"), TEXT("start"), TEXT("one"), inBuf, MAX_LOADSTRING, iniFile);
	   if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("one")) == 0) {
		   HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		   if (hProc != NULL) {
			   DWORD dwExitCode;
			   GetExitCodeProcess(hProc, &dwExitCode);
			   CloseHandle(hProc);
			   if (dwExitCode == STILL_ACTIVE)
			   {
				   MessageBox(hWnd, TEXT("程序已打开，不能打开多个哦，如需要多开请在操作中切换启动模式"), TEXT("失败"), MB_OK);
				   return FALSE;
			   }
		   }
	   }
   }

   DWORD processId = GetCurrentProcessId();//当前进程id
	// 先把变量写入
   TCHAR str[0x20];
   memset(str, 0, 0x20);
   wsprintf(str, TEXT("%d"), processId);
   WritePrivateProfileString(TEXT("ProxyUI"), TEXT("pid"), (LPCWSTR)str, iniFile);

   // 检查开机最小化
   if (wcscmp((const wchar_t*)startCmdLine, (const wchar_t*)TEXT("-mini")) == 0) {
	   ShowWindow(hWnd, SW_HIDE);
   }
   else
   {
	   ShowWindow(hWnd, nCmdShow);
   }
   UpdateWindow(hWnd);
   return TRUE;
}

// 初始化表单值
void initFormData(HWND hdlg)
{
	HWND hCmd1;
	TCHAR inBuf1[MAX_PATH];
	TCHAR inBuf2[MAX_PATH];
	if (initFormFlag) {
		return;
	}
	GetPrivateProfileString(TEXT("Program"), TEXT("app1"), TEXT(""), inBuf1, MAX_PATH, iniFile);
	hCmd1 = GetDlgItem(hdlg, IDC_PROXY_CMD1);
	SendMessage(hCmd1, WM_SETTEXT, NULL, (LPARAM)inBuf1);

	GetPrivateProfileString(TEXT("Program"), TEXT("param1"), TEXT(""), inBuf1, MAX_PATH, iniFile);
	hCmd1 = GetDlgItem(hdlg, IDC_EDIT2);
	SendMessage(hCmd1, WM_SETTEXT, NULL, (LPARAM)inBuf1);


	GetPrivateProfileString(TEXT("Program"), TEXT("app2"), TEXT(""), inBuf1, MAX_PATH, iniFile);
	hCmd1 = GetDlgItem(hdlg, IDC_PROXY_CMD2);
	SendMessage(hCmd1, WM_SETTEXT, NULL, (LPARAM)inBuf1);

	GetPrivateProfileString(TEXT("Program"), TEXT("param2"), TEXT(""), inBuf1, MAX_PATH, iniFile);
	hCmd1 = GetDlgItem(hdlg, IDC_EDIT4);
	SendMessage(hCmd1, WM_SETTEXT, NULL, (LPARAM)inBuf1);

	// 环境切换
	HWND hComboBox = GetDlgItem(hdlg, IDC_SWITCH);
	GetPrivateProfileString(TEXT("Program"), TEXT("switch"), TEXT(""), inBuf1, MAX_PATH, iniFile);
	if (wcscmp((const wchar_t*)inBuf1, (const wchar_t*)TEXT("")) == 0) {//无数据，初始化模板
		WritePrivateProfileString(L"Program", L"switch", L"online|test", iniFile);
		WritePrivateProfileString(L"Env", L"online", L"-c online.yaml", iniFile);
		WritePrivateProfileString(L"Env", L"test", L"-c test.yaml", iniFile);
		GetPrivateProfileString(TEXT("Program"), TEXT("switch"), TEXT(""), inBuf1, MAX_PATH, iniFile);
	}
	// 上次选中的环境
	GetPrivateProfileString(TEXT("Program"), TEXT("selected"), TEXT(""), inBuf2, MAX_PATH, iniFile);
	SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
	// 分割字符串，并添加Items
	wchar_t *buffer;
	wchar_t *token = wcstok_s(inBuf1, L"|", &buffer);
	int i = 0;
	int j = 0;
	while (token) {
		SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)token);
		if (wcscmp((const wchar_t*)token, (const wchar_t*)inBuf2) == 0) {
			j = i;
		}
		i++;
		token = wcstok_s(NULL, L"|", &buffer);
	}
	SendMessageW(hComboBox, CB_SETCURSEL, j, (LPARAM)0);
	// 重画高度
	RECT rect;
	GetClientRect(hComboBox, &rect);
	MapDialogRect(hComboBox, &rect);
	SetWindowPos(hComboBox, 0, 0, 0, rect.right, (i + 1) * rect.bottom, SWP_NOMOVE);

	// 默认选中后台打开进程
	CheckDlgButton(hdlg, IDC_CHECK1, BST_CHECKED);
	CheckDlgButton(hdlg, IDC_CHECK2, BST_CHECKED);

	// 程序运行状态
	if (pro_info.hProcess > 0) {
		HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
		SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
		HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START1);
		SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"重启");
	}
	if (pro_info2.hProcess > 0) {
		HWND hStatus = GetDlgItem(hdlg, IDC_STATIC2);
		SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
		HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START2);
		SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"重启");
	}
	initFormFlag = true;
}

//FORMVIEW 回调消息
LRESULT CALLBACK DlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_SHOWWINDOW://这里初始化化，托盘右键才能取到值
			{
				initFormData(hdlg);
			}
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hdlg, &ps);
				initFormData(hdlg);
				EndPaint(hdlg, &ps);
			}
			break;
		case WM_COMMAND:
			{
				int wmId = LOWORD(wParam);
				// 分析菜单选择: 
				switch (wmId)
				{
					case IDC_PROXY_FILE1:
						{
							selectApplication(hdlg, IDC_PROXY_CMD1);
						}
						break;
					case IDC_PROXY_START1:
						{
							WCHAR ProxyExe1[MAX_PATH] = { 0 };
							GetDlgItemText(hdlg, IDC_PROXY_CMD1, (LPTSTR)ProxyExe1, MAX_PATH);
							if (wcscmp((const wchar_t*)ProxyExe1, (const wchar_t*)TEXT("")) == 0) {
								MessageBox(hdlg, TEXT("先选择程序"), TEXT("失败"), MB_OK);
								break;
							}
							// 先把变量写入
							WritePrivateProfileString(TEXT("Program"), TEXT("app1"), ProxyExe1, iniFile);

							WCHAR Params[MAX_PATH] = { 0 };
							GetDlgItemText(hdlg, IDC_EDIT2, (LPTSTR)Params, MAX_PATH);
							WritePrivateProfileString(TEXT("Program"), TEXT("param1"), Params, iniFile);

							lstrcat(ProxyExe1, TEXT(" "));
							lstrcat(ProxyExe1, Params);
							// 是否选中后台
							UINT sta = IsDlgButtonChecked(hdlg, IDC_CHECK1);
							BOOL ret = startApp(hdlg, &pro_info, ProxyExe1, sta == BST_UNCHECKED);
							if (ret) {
								HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
								SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
								HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START1);
								SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"重启");
							}

						}
						break;
					case IDC_PROXY_STOP1:
						{
							stopApp(hdlg, &pro_info);
							if (pro_info.hProcess == 0) {
								HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
								SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"未运行");
								HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START1);
								SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"启动");
							}
						}
						break;
					case IDC_PROXY_FILE2:
						{
							selectApplication(hdlg, IDC_PROXY_CMD2);
						}
						break;
					case IDC_SWITCH:
						{
							switch (HIWORD(wParam))
							{
							case CBN_SELCHANGE:
								// 读取选中
								LRESULT idx_row;
								WCHAR selectText[MAX_LOADSTRING] = { 0 };
								HWND hComboBox = GetDlgItem(hdlg, IDC_SWITCH);
								idx_row = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
								SendMessage(hComboBox, CB_GETLBTEXT, idx_row, (LPARAM)selectText);

								// 获取配置的参数
								WCHAR params[MAX_PATH] = { 0 };
								GetPrivateProfileString(TEXT("Env"), selectText, TEXT(""), params, MAX_PATH, iniFile);
								// 设置到参数输入框
								HWND hEdit4 = GetDlgItem(hdlg, IDC_EDIT4);
								SendMessage(hEdit4, WM_SETTEXT, NULL, (LPARAM)params);
								break;
							}
						}
						break;
					case IDC_PROXY_START2:
						{
							clickStartApp2(hdlg);
						}
						break;
					case IDC_PROXY_STOP2:
						{
							stopApp(hdlg, &pro_info2);
							if (pro_info2.hProcess == 0) {
								HWND hStatus = GetDlgItem(hdlg, IDC_STATIC2);
								SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"未运行");
								HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START2);
								SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"启动");
							}
						}
						break;
				}
			}
			break;
	}
	return 0;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE: // 先于InitInstance方法被调用
		{
			CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"  系统代理",
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				10, 10, 100, 30,
				hWnd, NULL, NULL, NULL);
			hWndComboBox = CreateWindowEx(WS_EX_STATICEDGE, L"COMBOBOX", L"下拉框",
				CBS_DROPDOWN | CBS_HASSTRINGS | WS_VISIBLE | WS_CHILD | WS_BORDER,
				120, 10, 350, 500, hWnd, (HMENU)IDC_PROXY_SERVER, NULL, NULL);
			hWndBtn1 = CreateWindowEx(WS_EX_STATICEDGE, L"BUTTON", L"确定",
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				480, 10, 100, 30,
				hWnd, (HMENU)IDC_PROXY_OK, NULL, NULL);
 
			// 添加默认代理选项
			SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)CloseProxy);

			TCHAR inBuf[maxLen];
			GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), inBuf, maxLen, iniFile);
			if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("")) == 0) {//无文件，创建文件
				WritePrivateProfileString(L"Server", L"List", L"http=127.0.0.1:3000;https=127.0.0.1:3000|http=127.0.0.1:8888;https=127.0.0.1:8888", iniFile);
				GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), inBuf, maxLen, iniFile);
			}

			//获取系统设置
			GetConnectProxy(hWnd, (LPWSTR)lanName);
			// 分割字符串，并添加Items
			wchar_t *buffer;
			wchar_t *token = wcstok_s(inBuf, L"|", &buffer);
			int i = 1;
			int j = 0;
			while (token) {
				SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)token);
				if (wcscmp((const wchar_t*)token, (const wchar_t*)ProxyText) == 0) {
					j = i;
				}
				i++;
				token = wcstok_s(NULL, L"|", &buffer);
			}

			if (j == 0 && wcscmp((const wchar_t*)ProxyText, (const wchar_t*)TEXT("")) != 0) { //有设置代理，但不在已经配置的列表
				// 填充下拉菜单并选中
				SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)ProxyText);
				SendMessageW(hWndComboBox, CB_SETCURSEL, i, (LPARAM)0);
			}
			else {
				// 设置选中
				SendMessageW(hWndComboBox, CB_SETCURSEL, j, (LPARAM)0);
			}
			//更新变量
			updateProxyText();
			
			// 插入FORMVIEW
			hfDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)DlgProc);
			// 自动开启服务
			WCHAR ProxyExe1[MAX_PATH] = { 0 };
			WCHAR Param1[MAX_PATH] = { 0 };
			TCHAR autoBuf[MAX_LOADSTRING] = { 0 };
			GetPrivateProfileString(TEXT("ProxyUI"), TEXT("auto1"), TEXT(""), autoBuf, MAX_LOADSTRING, iniFile);
			if (wcscmp((const wchar_t*)autoBuf, (const wchar_t*)TEXT("open")) == 0) {
				GetPrivateProfileString(TEXT("Program"), TEXT("app1"), TEXT(""), ProxyExe1, MAX_PATH, iniFile);
				if (wcscmp((const wchar_t*)ProxyExe1, (const wchar_t*)TEXT("")) != 0) {
					GetPrivateProfileString(TEXT("Program"), TEXT("param1"), TEXT(""), Param1, MAX_PATH, iniFile);
					lstrcat(ProxyExe1, TEXT(" "));
					lstrcat(ProxyExe1, Param1);
					startApp(hfDlg, &pro_info, ProxyExe1, false);
				}
			}
			GetPrivateProfileString(TEXT("ProxyUI"), TEXT("auto2"), TEXT(""), autoBuf, MAX_LOADSTRING, iniFile);
			if (wcscmp((const wchar_t*)autoBuf, (const wchar_t*)TEXT("open")) == 0) {
				GetPrivateProfileString(TEXT("Program"), TEXT("app2"), TEXT(""), ProxyExe1, MAX_PATH, iniFile);
				if (wcscmp((const wchar_t*)ProxyExe1, (const wchar_t*)TEXT("")) != 0) {
					GetPrivateProfileString(TEXT("Program"), TEXT("param2"), TEXT(""), Param1, MAX_PATH, iniFile);
					lstrcat(ProxyExe1, TEXT(" "));
					lstrcat(ProxyExe1, Param1);
					startApp(hfDlg, &pro_info2, ProxyExe1, false);
				}
			}
			// 显示dialog
			ShowWindow(hfDlg, SW_SHOW);
			// 显示托盘
			BuildTrayIcon(hWnd);
		}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
			case IDC_PROXY_SERVER:
				{
					switch (HIWORD(wParam))
					{
					case CBN_SELCHANGE:
						updateProxyText();
						break;
					}
				}
				break;
			case IDC_PROXY_OK://设置代理
				{
					if (wcscmp((const wchar_t*)ProxyText, (const wchar_t*)CloseProxy) == 0) {
						if (DisableConnectionProxy(hWnd, (LPWSTR)lanName)) {
							MessageBox(hWnd, TEXT("已成功取消代理"), TEXT("成功"), MB_OK);
						}
						else {
							ErrorMessage(TEXT("取消代理失败，请尝试右键->以管理员身份运行"));
						}
					}
					else {
						if (SetConnectionOptions(hWnd, (LPWSTR)lanName, (LPWSTR)ProxyText)) {
							MessageBox(hWnd, (LPCWSTR)ProxyText, TEXT("代理设置如下"), MB_OK);
						}
						else {
							ErrorMessage(TEXT("代理设置失败，请尝试右键->以管理员身份运行"));
						}
					}
				}
				break;
			case IDM_START:
				{
					BOOL AutoStart = false;
					AutoStart = SetAutoRun(hWnd, TEXT(""));
					if (AutoStart) {
						MessageBox(hWnd, TEXT("已设为开机自动运行"), TEXT("成功"), MB_OK);
					}
					else {
						ErrorMessage(TEXT("设置开机自运行失败，请尝试右键->以管理员身份运行"));
					}
				}
				break;
			case IDM_START_MINI:
				{
					BOOL AutoStart = false;
					AutoStart = SetAutoRun(hWnd, TEXT(" -mini"));
					if (AutoStart) {
						MessageBox(hWnd, TEXT("已设为开机自动运行并最小化窗口"), TEXT("成功"), MB_OK);
					}
					else {
						ErrorMessage(TEXT("设置开机自运行失败，请尝试右键->以管理员身份运行"));
					}
				}
				break;
			case IDM_STOP:
				{
					BOOL NoAutoStart = false;
					NoAutoStart = SetNoAutoRun(hWnd);
					if (NoAutoStart) {
						MessageBox(hWnd, TEXT("已取消开机自动运行"), TEXT("成功"), MB_OK);
					}
					else {
						MessageBox(hWnd, TEXT("当前没有开机启动"), TEXT("失败"), MB_OK);
					}
				}
				break;
			case ID_AUTO1:
				{
					TCHAR inBuf[MAX_LOADSTRING];
					GetPrivateProfileString(TEXT("ProxyUI"), TEXT("auto1"), TEXT(""), inBuf, MAX_LOADSTRING, iniFile);
					if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("open")) == 0) {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("auto1"), TEXT(""), iniFile);
						MessageBox(hWnd, TEXT("已关闭自动运行代理程序1"), TEXT("成功"), MB_OK);
					}
					else {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("auto1"), TEXT("open"), iniFile);
						MessageBox(hWnd, TEXT("开启软件后自动运行代理程序1"), TEXT("成功"), MB_OK);
					}
				}
				break;
			case ID_AUTO2:
				{
					TCHAR inBuf[MAX_LOADSTRING];
					GetPrivateProfileString(TEXT("ProxyUI"), TEXT("auto2"), TEXT(""), inBuf, MAX_LOADSTRING, iniFile);
					if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("open")) == 0) {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("auto2"), TEXT(""), iniFile);
						MessageBox(hWnd, TEXT("已关闭自动运行代理程序2"), TEXT("成功"), MB_OK);
					}
					else {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("auto2"), TEXT("open"), iniFile);
						MessageBox(hWnd, TEXT("开启软件后自动运行代理程序2"), TEXT("成功"), MB_OK);
					}
				}
				break;
			case IDM_START_MOD:
				{
					TCHAR inBuf[MAX_LOADSTRING];
					GetPrivateProfileString(TEXT("ProxyUI"), TEXT("start"), TEXT("one"), inBuf, MAX_LOADSTRING, iniFile);
					if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("one")) == 0) {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("start"), TEXT("multi"), iniFile);
						MessageBox(hWnd, TEXT("已切换到多开模式，当前程序可以同时打开多次"), TEXT("成功"), MB_OK);
					}
					else {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("start"), TEXT("one"), iniFile);
						MessageBox(hWnd, TEXT("已切换到单开模式，当前程序只能打开一次"), TEXT("成功"), MB_OK);
					}
				}
				break;
			case ID_GITHUB:
				ShellExecute(hWnd, TEXT("open"), TEXT("https://github.com/keminar/proxyui"), NULL, NULL, SW_SHOWNORMAL);
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
				// 检查进程是否在
				if (pro_info.hProcess > 0 || pro_info2.hProcess > 0) {
					MessageBox(hWnd, TEXT("先停止启动的程序再退出"), TEXT("失败"), MB_OK);
					break;
				}
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		DestroyTrayIcon(hWnd);
        PostQuitMessage(0);
        break;
	case WM_CLOSE://关闭窗口时最小化到托盘
		ModifyTrayIcon(hWnd);
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_CLICKBIT://点击托盘图标
	{
		switch (lParam)
		{
		case WM_LBUTTONUP://托盘图标还原窗口
			ShowWindow(hWnd, SW_SHOWNORMAL);
			break;
		case WM_RBUTTONDOWN:
			POINT pt;
			int menu_rtn;//用于接收菜单选项返回值
			GetCursorPos(&pt);//取鼠标坐标
			::SetForegroundWindow(hWnd);
			HMENU hMenu;
			hMenu = CreatePopupMenu();

			TCHAR inBuf1[MAX_PATH];
			GetPrivateProfileString(TEXT("Program"), TEXT("switch"), TEXT(""), inBuf1, MAX_PATH, iniFile);
			if (wcscmp((const wchar_t*)inBuf1, (const wchar_t*)TEXT("")) != 0) {//有多个环境切换
				// 当前环境
				LRESULT idx_row;
				HWND hComboBox = GetDlgItem(hfDlg, IDC_SWITCH);
				idx_row = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
				AppendMenu(hMenu, MFS_DISABLED, 0, TEXT("快速切换环境"));
				// 创建菜单
				wchar_t *buffer;
				wchar_t *token = wcstok_s(inBuf1, L"|", &buffer);
				int i = 0;
				while (token) {
					i++;
					if (i == idx_row+1) {
						AppendMenu(hMenu, MF_CHECKED, i, token);
					}
					else {
						AppendMenu(hMenu, MF_UNCHECKED, i, token);
					}
					AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
					token = wcstok_s(NULL, L"|", &buffer);
				}

				menu_rtn = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hWnd, NULL);
				// 选中菜单
				if (menu_rtn >= 1 && menu_rtn <= i) {
					// 设置下拉值
					SendMessageW(hComboBox, CB_SETCURSEL, menu_rtn - 1, (LPARAM)0);
					// 防止消息是异步的还没有更新，不用PostMessage模拟消息驱动，直接取值并赋值
					WCHAR selectText[MAX_LOADSTRING] = { 0 };
					SendMessage(hComboBox, CB_GETLBTEXT, menu_rtn - 1, (LPARAM)selectText);

					// 获取配置的参数
					WCHAR params[MAX_PATH] = { 0 };
					GetPrivateProfileString(TEXT("Env"), selectText, TEXT(""), params, MAX_PATH, iniFile);
					// 设置到参数输入框
					HWND hEdit4 = GetDlgItem(hfDlg, IDC_EDIT4);
					SendMessage(hEdit4, WM_SETTEXT, NULL, (LPARAM)params);
					
					// 开启应用
					clickStartApp2(hfDlg);
				}
			}
			break;
		default:
			break;
		}
	}
	break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//开机自动运行 
BOOL SetAutoRun(HWND hwnd, LPWSTR params)
{
	//MessageBox(NULL, filePath, TEXT("path"), MB_OK);

	WCHAR str[MAX_PATH];
	HKEY hRegKey;
	BOOL bResult;
	lstrcpy(str, RegRun);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, str, 0, KEY_ALL_ACCESS, &hRegKey) != ERROR_SUCCESS) {
		bResult = FALSE;
	}
	else {
		WCHAR fileRun[MAX_PATH];
		wcscpy_s(fileRun, _countof(fileRun), filePath);
		wcscat_s(fileRun, params);
		//wcslen 返回的是字符串中的字符数, 在 UNICODE 编码中，一个字符占2个字节
		if (RegSetValueEx(hRegKey, RegName, 0, REG_SZ, (BYTE*)fileRun, (DWORD)wcslen(fileRun) * 2) != ERROR_SUCCESS) {
			bResult = FALSE;
		}
		else {
			bResult = TRUE;
		}
		RegCloseKey(hRegKey);
	}

	return bResult;
}

//关闭开机自动运行 
BOOL SetNoAutoRun(HWND hwnd)
{
	WCHAR str[MAX_PATH];
	HKEY hRegKey;
	BOOL bResult;
	lstrcpy(str, RegRun);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, str, 0, KEY_ALL_ACCESS, &hRegKey) != ERROR_SUCCESS) {
		bResult = FALSE;
	}
	else {
		if (RegDeleteValue(hRegKey, RegName) != ERROR_SUCCESS) {
			bResult = FALSE;
		}
		else {
			bResult = TRUE;
		}
		RegCloseKey(hRegKey);
	}
	return bResult;
}

// 托盘图标
// https://docs.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataa
void BuildTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA notifyIconData;
	ZeroMemory(&notifyIconData, sizeof(notifyIconData));
	notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.hIcon = LoadIcon((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		MAKEINTRESOURCE(IDI_PROXYUI));
	notifyIconData.uID = IDI_PROXYUI;
	notifyIconData.hWnd = hwnd;
	notifyIconData.uCallbackMessage = WM_CLICKBIT; //自定义消息
	lstrcpy(notifyIconData.szTip, TEXT("ProxyUI"));
	notifyIconData.dwState =  NIS_SHAREDICON;//是否显示icon

	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

//修改系统托盘图标 
void ModifyTrayIcon(HWND hwnd)
{
	if (!firstTray) {
		return;
	}
	NOTIFYICONDATA notifyIconData;
	ZeroMemory(&notifyIconData, sizeof(notifyIconData));
	notifyIconData.cbSize = sizeof(notifyIconData);
	
	notifyIconData.hIcon = LoadIcon((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		MAKEINTRESOURCE(IDI_PROXYUI));
	notifyIconData.uID = IDI_PROXYUI;
	notifyIconData.hWnd = hwnd;
	notifyIconData.uCallbackMessage = WM_CLICKBIT; //自定义消息
	lstrcpy(notifyIconData.szTip, TEXT("ProxyUI"));
	notifyIconData.dwState = NIS_SHAREDICON;//是否显示icon

	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	notifyIconData.dwInfoFlags = NIIF_NONE | NIIF_NOSOUND;//不播放声音
	notifyIconData.uTimeout = 1000/*提示超时毫秒,仅Windows 2000 和 Windows XP有效*/;
	lstrcpy(notifyIconData.szInfoTitle, TEXT("托盘最小化;-)："));
	lstrcpy(notifyIconData.szInfo, TEXT("如需退出，请点击\"操作->退出\"。 嘿嘿~~~"));

	Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
	firstTray = FALSE;
}

//销毁系统托盘图标 
void DestroyTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA notifyIconData;
	ZeroMemory(&notifyIconData, sizeof(notifyIconData));
	notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.uID = IDI_PROXYUI;
	notifyIconData.hWnd = hwnd;
	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
}

// 设置代理 https://docs.microsoft.com/en-us/windows/win32/wininet/setting-and-retrieving-internet-options
BOOL SetConnectionOptions(HWND hWnd, LPWSTR conn_name, LPWSTR proxy_full_addr)
{
	//conn_name: active connection name. 
	//proxy_full_addr : eg "210.78.22.87:8000"
	INTERNET_PER_CONN_OPTION_LIST list;
	BOOL    bReturn;
	DWORD   dwBufSize = sizeof(list);
	// Fill out list struct.
	list.dwSize = sizeof(list);
	// NULL == LAN, otherwise connectoid name.
	list.pszConnection = conn_name;
	// Set three options.
	list.dwOptionCount = 2;//3;
	list.pOptions = new INTERNET_PER_CONN_OPTION[2/*3*/];
	// Make sure the memory was allocated.
	if (NULL == list.pOptions)
	{
		// Return FALSE if the memory wasn't allocated.
		MessageBox(hWnd, TEXT("failed to allocat memory in SetConnectionOptions()"), TEXT("失败"), MB_OK);
		return FALSE;
	}
	// Set flags.
	list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
	list.pOptions[0].Value.dwValue = PROXY_TYPE_DIRECT |
		PROXY_TYPE_PROXY;

	// Set proxy name.
	list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
	list.pOptions[1].Value.pszValue = proxy_full_addr;//"http://proxy:80";

	/*
	// Set proxy override.
	list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
	list.pOptions[2].Value.pszValue = "local";
	*/

	// Set the options on the connection.
	bReturn = InternetSetOption(NULL,
		INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize);

	// Free the allocated memory.
	delete[] list.pOptions;

	InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
	InternetSetOption(NULL, INTERNET_OPTION_REFRESH, NULL, 0);
	return bReturn;
}

// 取消代理
BOOL DisableConnectionProxy(HWND hWnd, LPWSTR conn_name)
{
	//conn_name: active connection name. 
	INTERNET_PER_CONN_OPTION_LIST list;
	BOOL    bReturn;
	DWORD   dwBufSize = sizeof(list);
	// Fill out list struct.
	list.dwSize = sizeof(list);
	// NULL == LAN, otherwise connectoid name.
	list.pszConnection = conn_name;
	// Set three options.
	list.dwOptionCount = 1;
	list.pOptions = new INTERNET_PER_CONN_OPTION[list.dwOptionCount];
	// Make sure the memory was allocated.
	if (NULL == list.pOptions)
	{
		// Return FALSE if the memory wasn't allocated.
		MessageBox(hWnd, TEXT("failed to allocat memory in DisableConnectionProxy()"), TEXT("失败"), MB_OK);
		return FALSE;
	}
	// Set flags.
	list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
	list.pOptions[0].Value.dwValue = PROXY_TYPE_DIRECT;
	// Set the options on the connection.
	bReturn = InternetSetOption(NULL,
		INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize);
	// Free the allocated memory.
	delete[] list.pOptions;
	InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
	InternetSetOption(NULL, INTERNET_OPTION_REFRESH, NULL, 0);
	return bReturn;
}

// 查询代理
BOOL GetConnectProxy(HWND hWnd, LPWSTR conn_name)
{
	BOOL ret;
	INTERNET_PER_CONN_OPTION_LIST list;
	INTERNET_PER_CONN_OPTION* Option = new INTERNET_PER_CONN_OPTION[1];

	DWORD   dwBufSize = sizeof(list);
	list.dwSize = sizeof(list);
	list.pszConnection = conn_name; // NULL;
	list.dwOptionCount = 1;
	list.pOptions = Option;

	list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
	list.pOptions[0].Value.dwValue = 0;

	ret = InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufSize);
	if (ret && Option[0].Value.dwValue != PROXY_TYPE_DIRECT) {
		list.pOptions[0].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
		list.pOptions[0].Value.pszValue = 0;

		ret = InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufSize);

		wcscpy_s(ProxyText, _countof(ProxyText), Option[0].Value.pszValue);
		//MessageBox(hWnd, (LPCWSTR)Option[0].Value.pszValue, TEXT("成功"), MB_OK);
	}

	// Free the allocated memory.
	delete[] list.pOptions;
	return ret;
}


// 更新全局变量
void updateProxyText()
{
	LRESULT idx_row;
	idx_row = SendMessage(hWndComboBox, CB_GETCURSEL, 0, 0);
	SendMessage(hWndComboBox, CB_GETLBTEXT, idx_row, (LPARAM)ProxyText);
}

// 选择文件
void selectApplication(HWND hWnd, int nIDDlgItem)
{
	OPENFILENAME opfn;
	WCHAR strFilename[MAX_PATH];//存放文件名
	//初始化
	ZeroMemory(&opfn, sizeof(OPENFILENAME));
	opfn.lStructSize = sizeof(OPENFILENAME);//结构体大小
	//设置过滤
	opfn.lpstrFilter = L"所有文件\0*.*\0可执行文件\0*.exe\0";
	//默认过滤器索引设为1
	opfn.nFilterIndex = 1;
	//文件名的字段必须先把第一个字符设为 \0
	opfn.lpstrFile = strFilename;
	opfn.lpstrFile[0] = '\0';
	opfn.nMaxFile = sizeof(strFilename);
	//设置标志位，检查目录或文件是否存在
	opfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	//opfn.lpstrInitialDir = NULL;
	// 显示对话框让用户选择文件
	if (GetOpenFileName(&opfn))
	{
		//在文本框中显示文件路径
		HWND hEdt = GetDlgItem(hWnd, nIDDlgItem);
		SendMessage(hEdt, WM_SETTEXT, NULL, (LPARAM)strFilename);
	}
}

// 启动应用
BOOL startApp(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* ProxyExe1, BOOL show)
{
	// 检查进程是否在则先停再开
	if ((*process).hProcess > 0) {
		stopApp(hWnd, process);
		Sleep(500);
		if ((*process).hProcess > 0) {//停止失败
			return FALSE;
		}
	}

	STARTUPINFO sti; //启动信息   
	ZeroMemory(process, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&sti, sizeof(STARTUPINFO));
	sti.cb = sizeof(sti);
	if (!show) {
		sti.dwFlags = STARTF_USESHOWWINDOW;
		sti.wShowWindow = SW_HIDE;
	}
	//dirPath指定新进程的工作路径，解决开机自启动工作路径是C:\Windows\SysWOW64，子进程配置相对路径时找不到配置文件的bug
	BOOL bRet = FALSE;
	bRet = CreateProcess(NULL, ProxyExe1, NULL, NULL, FALSE, 0, NULL, dirPath, &sti, process);
	if (!bRet)
	{
		MessageBox(hWnd, TEXT("启动失败"), TEXT("失败"), MB_OK);
		return FALSE;
	}
	return TRUE;
}

// 发送CLOSE消息
BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD dwID;
	GetWindowThreadProcessId(hwnd, &dwID);
	if (dwID == (DWORD)lParam) {
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}
	return TRUE;
}

// 停止应用
void stopApp(HWND hWnd, PROCESS_INFORMATION* process)
{
	// 检查进程是否在
	if ((*process).hProcess == 0) {
		//MessageBox(hWnd, TEXT("没有进程"), TEXT("失败"), MB_OK);
		return;
	}

	HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, (*process).dwProcessId);
	if (hProc == NULL) {
		return;
	}

	//bat文件采用这种可以关闭, bat的还要写在前面
	// 发送CLOSE消息关闭，针对bat有效
	EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)(*process).dwProcessId);

	/*
	// 另一种发送CLOSE方法
	std::wostringstream oss;
	oss.str(_T(""));
	oss << _T("/PID ");
	oss << pro_info.dwProcessId;
	std::wstring strCmd = oss.str();
	ShellExecute(NULL, _T("OPEN"), _T("taskkill.exe"), strCmd.c_str(), _T(""), SW_HIDE);
	*/

	//不加停顿bat会关不掉
	///Sleep(1000);
	// 等待Milliseconds，如果不行杀进程，对exe有效果
	if (WaitForSingleObject(hProc, 1000) != WAIT_OBJECT_0) {
		//exe文件采用这种可以关闭
		DWORD dwExitCode = 0;
		// 获取子进程的退出码 
		GetExitCodeProcess((*process).hProcess, &dwExitCode);
		TerminateProcess((*process).hProcess, dwExitCode);//终止进程
	}

	// 关闭子进程的主线程句柄 
	CloseHandle((*process).hThread);
	// 关闭子进程句柄 
	CloseHandle((*process).hProcess);
	(*process).hProcess = 0;
	CloseHandle(hProc);
}


// 开启代理2
void clickStartApp2(HWND hdlg)
{
	WCHAR ProxyExe2[MAX_PATH] = { 0 };
	GetDlgItemText(hdlg, IDC_PROXY_CMD2, (LPTSTR)ProxyExe2, MAX_PATH);
	if (wcscmp((const wchar_t*)ProxyExe2, (const wchar_t*)TEXT("")) == 0) {
		MessageBox(hdlg, TEXT("先选择程序"), TEXT("失败"), MB_OK);
		return;
	}
	WritePrivateProfileString(TEXT("Program"), TEXT("app2"), ProxyExe2, iniFile);

	WCHAR Params[MAX_PATH] = { 0 };
	GetDlgItemText(hdlg, IDC_EDIT4, (LPTSTR)Params, MAX_PATH);
	WritePrivateProfileString(TEXT("Program"), TEXT("param2"), Params, iniFile);

	// 环境选择记录下来
	LRESULT idx_row;
	WCHAR selectText[255] = { 0 };
	HWND hComboBox = GetDlgItem(hdlg, IDC_SWITCH);
	idx_row = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
	SendMessage(hComboBox, CB_GETLBTEXT, idx_row, (LPARAM)selectText);
	WritePrivateProfileString(TEXT("Program"), TEXT("selected"), selectText, iniFile);

	// 拼接命令
	lstrcat(ProxyExe2, TEXT(" "));
	lstrcat(ProxyExe2, Params);
	//MessageBox(hdlg, ProxyExe2, TEXT("失败"), MB_OK);

	// 是否选中后台
	UINT sta = IsDlgButtonChecked(hdlg, IDC_CHECK2);
	BOOL ret = startApp(hdlg, &pro_info2, ProxyExe2, sta == BST_UNCHECKED);
	if (ret) {
		HWND hStatus = GetDlgItem(hdlg, IDC_STATIC2);
		SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
		HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START2);
		SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"重启");
	}
}

// 显示错误
void ErrorMessage(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s \n错误码 %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("错误"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	//ExitProcess(dw);
}
