// ProxyUI.cpp : 定义应用程序的入口点。
// win32文档 https://docs.microsoft.com/en-us/windows/win32/api/

#include "stdafx.h"
#include "ProxyUI.h"
#include <ShellAPI.h>
#include <Shlwapi.h>
#include <comdef.h>
#include "commctrl.h"
#include <windows.h>
#include "wininet.h"
#include <atlstr.h>
#include <Commdlg.h>
#include <TlHelp32.h>
#include <sstream>
#include <string.h>
#include <strsafe.h>
#include <psapi.h>

#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
WCHAR proxyText[255] = { 0 }; // 代理变量
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

HWND hWndComboBox;
HWND hfDlg;

// 异步任务消息与常量
#define WM_APP_JOBDONE (WM_APP + 10)  // 工作线程完成后回发
#define JOB_START 1
#define JOB_STOP  2

// 异步任务上下文:启动/停止都放到工作线程执行,UI线程不再阻塞
struct AsyncJob {
	int appId;                    // 1 或 2
	int action;                   // JOB_START / JOB_STOP
	PROCESS_INFORMATION* process; // 指向 pro_info / pro_info2
	WCHAR cmdLine[MAX_PATH * 3];
	BOOL show;
	BOOL uac;
	HWND hdlg;                    // FORMVIEW 对话框,用于回发结果
	BOOL ok;                      // 执行结果
};
// 两个程序各自的忙碌标记,防止任务进行中重复触发
volatile LONG g_jobBusy1 = 0;
volatile LONG g_jobBusy2 = 0;

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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
      CW_USEDEFAULT, CW_USEDEFAULT, 660, 530, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   //当前进程id
   DWORD processId = GetCurrentProcessId();

   // 上次进程ID
   UINT pid = GetPrivateProfileInt(TEXT("ProxyUI"), TEXT("pid"), 0, iniFile);
   if (pid > 0) {
	   TCHAR inBuf[MAX_LOADSTRING];
	   GetPrivateProfileString(TEXT("ProxyUI"), TEXT("start"), TEXT("one"), inBuf, MAX_LOADSTRING, iniFile);
	   if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("one")) == 0) {
		   HANDLE lastProc = OpenProcess(PROCESS_QUERY_INFORMATION |
			   PROCESS_VM_READ, FALSE, pid);
		   if (lastProc != NULL) {
			   TCHAR LastPath[MAX_PATH];
			   if (GetModuleFileNameEx(lastProc, 0, LastPath, MAX_PATH)) {
				   DWORD dwExitCode;
				   GetExitCodeProcess(lastProc, &dwExitCode);
				   if (dwExitCode == STILL_ACTIVE) {
					   HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION |
						   PROCESS_VM_READ, FALSE, processId);
					   if (hProc != NULL) {
						   // 对比文件路径一样才提示多开，避免系统刚开机时文本里的pid被其它进程占用
						   TCHAR CurPath[MAX_PATH];
						   if (GetModuleFileNameEx(hProc, 0, CurPath, MAX_PATH)) {
							   if (wcscmp((const wchar_t*)LastPath, (const wchar_t*)CurPath) == 0) {
								   MessageBox(hWnd, TEXT("程序已打开，不能打开多个哦，如需要多开请在操作中切换启动模式"), TEXT("失败"), MB_OK);
								   return FALSE;
							   }
						   }
						   CloseHandle(hProc);
					   }
				   }
			   }
			   CloseHandle(lastProc);
		   }
	   }
   }
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

	// 恢复UAC复选框状态
	TCHAR uacBuf[MAX_LOADSTRING];
	GetPrivateProfileString(TEXT("ProxyUI"), TEXT("uac1"), TEXT(""), uacBuf, MAX_LOADSTRING, iniFile);
	CheckDlgButton(hdlg, IDC_UAC, (wcscmp((const wchar_t*)uacBuf, (const wchar_t*)TEXT("open")) == 0) ? BST_CHECKED : BST_UNCHECKED);

	// 程序运行状态
	if (pro_info.dwProcessId > 0 && g_jobBusy1 == 0) {
		HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
		SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
		HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START1);
		SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"重启");
	}
	if (pro_info2.dwProcessId > 0 && g_jobBusy2 == 0) {
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
    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMaxTrackSize.x = 800;
            lpMMI->ptMaxTrackSize.y = 600;
        }
        break;
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
		case WM_APP_JOBDONE:
			{
				// 工作线程完成:在UI线程刷新状态文字/按钮/托盘并解除忙碌
				AsyncJob* job = (AsyncJob*)wParam;
				int appId = job->appId;
				HWND hStatus = GetDlgItem(hdlg, (appId == 1) ? IDC_STATIC1 : IDC_STATIC2);
				HWND hStart  = GetDlgItem(hdlg, (appId == 1) ? IDC_PROXY_START1 : IDC_PROXY_START2);
				HWND hStop   = GetDlgItem(hdlg, (appId == 1) ? IDC_PROXY_STOP1 : IDC_PROXY_STOP2);
				if (job->action == JOB_START) {
					if (job->ok) {
						SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
						SendMessage(hStart, WM_SETTEXT, NULL, (LPARAM)L"重启");
					} else {
						SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"未运行");
						SendMessage(hStart, WM_SETTEXT, NULL, (LPARAM)L"启动");
					}
				} else {
					if (job->process->dwProcessId == 0) {
						SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"未运行");
						SendMessage(hStart, WM_SETTEXT, NULL, (LPARAM)L"启动");
					} else {
						SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
					}
				}
				EnableWindow(hStart, TRUE);
				EnableWindow(hStop, TRUE);
				BuildTrayIcon(GetParent(hdlg), NIM_MODIFY);
				InterlockedExchange((appId == 1) ? &g_jobBusy1 : &g_jobBusy2, 0);
				delete job;
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
					case IDC_SYSTEM_READ://读取最新系统代理
						{
							refreshSystemProxy(GetParent(hdlg));
						}
						break;
					case IDC_SYSTEM_SET://设置代理
						{
							HWND hMain = GetParent(hdlg);
							// 读取下拉框当前文本(选中的或手动输入的)
							GetWindowTextW(hWndComboBox, proxyText, _countof(proxyText));
							if (wcscmp((const wchar_t*)proxyText, (const wchar_t*)TEXT("")) == 0) {
								break;
							}
							if (wcscmp((const wchar_t*)proxyText, (const wchar_t*)CloseProxy) == 0) {
								if (DisableConnectionProxy(hMain, (LPWSTR)lanName)) {
									BuildTrayIcon(hMain, NIM_MODIFY);
									MessageBox(hMain, TEXT("已成功取消代理"), TEXT("成功"), MB_OK);
								}
								else {
									ErrorMessage(TEXT("取消代理失败，请尝试右键->以管理员身份运行"));
								}
							}
							else {
								// 手动输入的新代理若不在列表则添加为下拉项并写入ini
								if (SendMessageW(hWndComboBox, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)proxyText) == CB_ERR) {
									SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)proxyText);
									TCHAR listBuf[maxLen] = { 0 };
									GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), listBuf, maxLen, iniFile);
									TCHAR newList[maxLen] = { 0 };
									if (wcslen((const wchar_t*)listBuf) > 0) {
										_snwprintf_s(newList, _countof(newList), _TRUNCATE, L"%s|%s", listBuf, proxyText);
									}
									else {
										wcscpy_s(newList, _countof(newList), proxyText);
									}
									WritePrivateProfileString(TEXT("Server"), TEXT("List"), newList, iniFile);
								}
								if (SetConnectionOptions(hMain, (LPWSTR)lanName, (LPWSTR)proxyText)) {
									BuildTrayIcon(hMain, NIM_MODIFY);
									MessageBox(hMain, (LPCWSTR)proxyText, TEXT("代理设置如下"), MB_OK);
								}
								else {
									ErrorMessage(TEXT("代理设置失败，请尝试右键->以管理员身份运行"));
								}
							}
						}
						break;
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
								MessageBox(hdlg, TEXT("请选择程序"), TEXT("失败"), MB_OK);
								break;
							}
							// 先把变量写入
							WritePrivateProfileString(TEXT("Program"), TEXT("app1"), ProxyExe1, iniFile);

							WCHAR Params[MAX_PATH] = { 0 };
							GetDlgItemText(hdlg, IDC_EDIT2, (LPTSTR)Params, MAX_PATH);
							WritePrivateProfileString(TEXT("Program"), TEXT("param1"), Params, iniFile);

							// 把程序路径用双引号包起来,支持带空格路径,与参数合并成一条命令行
							WCHAR cmdLine1[MAX_PATH * 3] = { 0 };
							_snwprintf_s(cmdLine1, _countof(cmdLine1), _TRUNCATE, L"\"%s\" %s", ProxyExe1, Params);
							// 是否勾选后台
							UINT sta = IsDlgButtonChecked(hdlg, IDC_CHECK1);
							// 是否勾选 UAC 管理员权限
							UINT uac = IsDlgButtonChecked(hdlg, IDC_UAC);
							// 保存UAC状态到ini,供开机自启读取
							WritePrivateProfileString(TEXT("ProxyUI"), TEXT("uac1"), uac == BST_CHECKED ? TEXT("open") : TEXT(""), iniFile);
							// 放到工作线程执行,UI 不再卡顿
							SetJobBusyUI(hdlg, 1, TRUE);
							LaunchProxyJob(hdlg, 1, JOB_START, &pro_info, cmdLine1, sta == BST_UNCHECKED, uac == BST_CHECKED);
						}
						break;
					case IDC_PROXY_STOP1:
						{
							// 按启动时记录的 UAC 模式选择停止方式,避免两条路径重复提示授权
							TCHAR uacStopBuf[MAX_LOADSTRING] = { 0 };
							GetPrivateProfileString(TEXT("ProxyUI"), TEXT("uac1"), TEXT(""), uacStopBuf, MAX_LOADSTRING, iniFile);
							BOOL uacStop = (wcscmp((const wchar_t*)uacStopBuf, (const wchar_t*)TEXT("open")) == 0);
							// 放到工作线程执行,UI 不再卡顿
							SetJobBusyUI(hdlg, 1, FALSE);
							LaunchProxyJob(hdlg, 1, JOB_STOP, &pro_info, NULL, FALSE, uacStop);
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
							// 放到工作线程执行,UI 不再卡顿
							SetJobBusyUI(hdlg, 2, FALSE);
							LaunchProxyJob(hdlg, 2, JOB_STOP, &pro_info2, NULL, FALSE, FALSE);
						}
						break;
				}
			}
			break;
	}
	return 0;
}
#include <windows.h>
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
    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMaxTrackSize.x = 800;
            lpMMI->ptMaxTrackSize.y = 600;
        }
        break;
	case WM_CREATE: // 先于InitInstance方法被调用
		{
			// 先创建FORMVIEW，系统代理相关控件都在此dialog上
			hfDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)DlgProc);
			
			// 取IDC_PROXY_LIST的坐标后删除该占位控件
			HWND hProxyList = GetDlgItem(hfDlg, IDC_PROXY_LIST);
			RECT rcList;
			GetWindowRect(hProxyList, &rcList);
			MapWindowPoints(NULL, hfDlg, (LPPOINT)&rcList, 2);
			DestroyWindow(hProxyList);
			// 在IDC_PROXY_LIST原来的位置创建下拉框
			hWndComboBox = CreateWindowEx(0, L"COMBOBOX", L"下拉框",
				CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL | WS_VISIBLE | WS_CHILD,
				rcList.left, rcList.top, rcList.right - rcList.left, 300,
				hfDlg, (HMENU)IDC_PROXY_SERVER, hInst, NULL);
			// 下拉框和IDC_SWITCH都用宋体10pt
			HFONT hSongti10 = MakeSongtiFont(hfDlg, 10);
			SendMessageW(hWndComboBox, WM_SETFONT, (WPARAM)hSongti10, TRUE);
			SendMessageW(GetDlgItem(hfDlg, IDC_SWITCH), WM_SETFONT, (WPARAM)hSongti10, TRUE);
			
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
				if (wcscmp((const wchar_t*)token, (const wchar_t*)proxyText) == 0) {
					j = i;
				}
				i++;
				token = wcstok_s(NULL, L"|", &buffer);
			}

			if (j == 0 && wcscmp((const wchar_t*)proxyText, (const wchar_t*)TEXT("")) != 0) { //有设置代理，但不在已经配置的列表
				// 填充下拉菜单并选中
				SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)proxyText);
				SendMessageW(hWndComboBox, CB_SETCURSEL, i, (LPARAM)0);
			}
			else {
				// 设置选中
				SendMessageW(hWndComboBox, CB_SETCURSEL, j, (LPARAM)0);
			}
			//更新变量
			updateProxyText();
			

			// 自动开启服务
			WCHAR ProxyExe1[MAX_PATH] = { 0 };
			WCHAR Param1[MAX_PATH] = { 0 };
			WCHAR cmdLineAuto[MAX_PATH * 3] = { 0 };
			TCHAR autoBuf[MAX_LOADSTRING] = { 0 };
			GetPrivateProfileString(TEXT("ProxyUI"), TEXT("auto1"), TEXT(""), autoBuf, MAX_LOADSTRING, iniFile);
			// 从ini读取UAC状态（开机自启时对话框未显示，复选框状态不可靠）
			TCHAR uacBufAuto[MAX_LOADSTRING] = { 0 };
			GetPrivateProfileString(TEXT("ProxyUI"), TEXT("uac1"), TEXT(""), uacBufAuto, MAX_LOADSTRING, iniFile);
			BOOL uacAuto = (wcscmp((const wchar_t*)uacBufAuto, (const wchar_t*)TEXT("open")) == 0);
			if (wcscmp((const wchar_t*)autoBuf, (const wchar_t*)TEXT("open")) == 0) {
				GetPrivateProfileString(TEXT("Program"), TEXT("app1"), TEXT(""), ProxyExe1, MAX_PATH, iniFile);
				if (wcscmp((const wchar_t*)ProxyExe1, (const wchar_t*)TEXT("")) != 0) {
					GetPrivateProfileString(TEXT("Program"), TEXT("param1"), TEXT(""), Param1, MAX_PATH, iniFile);
					// 把程序路径用引号包裹，合并参数，避免缓冲区溢出
					_snwprintf_s(cmdLineAuto, _countof(cmdLineAuto), _TRUNCATE, L"\"%s\" %s", ProxyExe1, Param1);
					LaunchProxyJob(hfDlg, 1, JOB_START, &pro_info, cmdLineAuto, false, uacAuto);
			}
		}
		GetPrivateProfileString(TEXT("ProxyUI"), TEXT("auto2"), TEXT(""), autoBuf, MAX_LOADSTRING, iniFile);
		if (wcscmp((const wchar_t*)autoBuf, (const wchar_t*)TEXT("open")) == 0) {
			GetPrivateProfileString(TEXT("Program"), TEXT("app2"), TEXT(""), ProxyExe1, MAX_PATH, iniFile);
			if (wcscmp((const wchar_t*)ProxyExe1, (const wchar_t*)TEXT("")) != 0) {
				GetPrivateProfileString(TEXT("Program"), TEXT("param2"), TEXT(""), Param1, MAX_PATH, iniFile);
				_snwprintf_s(cmdLineAuto, _countof(cmdLineAuto), _TRUNCATE, L"\"%s\" %s", ProxyExe1, Param1);
				LaunchProxyJob(hfDlg, 2, JOB_START, &pro_info2, cmdLineAuto, false, false);
				}
			}
			// 显示dialog
			ShowWindow(hfDlg, SW_SHOW);
			// 显示托盘
			BuildTrayIcon(hWnd, NIM_ADD);
		}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
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
			case ID_OPENDIR:
				{
					ShellExecute(hWnd, TEXT("open"), TEXT("explorer.exe"), dirPath, NULL, SW_SHOWNORMAL);
				}
				break;
			case ID_OPENINI:
				{
					ShellExecute(hWnd, TEXT("open"), TEXT("notepad.exe"), iniFile, NULL, SW_SHOWNORMAL);
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
				if (pro_info.dwProcessId > 0 || pro_info2.dwProcessId > 0) {
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
			::SetForegroundWindow(hWnd);
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
    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMaxTrackSize.x = 800;
            lpMMI->ptMaxTrackSize.y = 600;
        }
        break;
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

void tipData(WCHAR *str, int len)
{
	wcscpy_s(str, len, TEXT("代理管理器:"));
	wcscat_s(str, len, TEXT("\n当前代理: "));
	if (wcscmp((const wchar_t*)proxyText, (const wchar_t*)TEXT("")) == 0) {
		wcscat_s(str, len, TEXT("无"));
	}
	else {
		wcscat_s(str, len, proxyText);
	}
	if (pro_info.dwProcessId > 0)  {
		wcscat_s(str, len, TEXT("\n程序1: 运行中"));
	}
	else {
		wcscat_s(str, len, TEXT("\n程序1: 未运行"));
	}
	if (pro_info2.dwProcessId > 0) {
		wcscat_s(str, len, TEXT("\n程序2: 运行中"));
	}
	else {
		wcscat_s(str, len, TEXT("\n程序2: 未运行"));
	}
}


// 托盘图标
// https://docs.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataa
void BuildTrayIcon(HWND hwnd, DWORD act)
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

	WCHAR str[255];
	tipData(str, _countof(str));
	lstrcpy(notifyIconData.szTip, str);
	notifyIconData.dwState =  NIS_SHAREDICON;//是否显示icon

	Shell_NotifyIcon(act, &notifyIconData);
}

//修改系统托盘图标 
void ModifyTrayIcon(HWND hwnd)
{
	if (!firstTray) {
		BuildTrayIcon(hwnd, NIM_MODIFY);
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
	
	WCHAR str[255];
	tipData(str, _countof(str));
	lstrcpy(notifyIconData.szTip, str);
	notifyIconData.dwState = NIS_SHAREDICON;//是否显示icon

	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	notifyIconData.dwInfoFlags = NIIF_NONE | NIIF_NOSOUND;//不播放声音
	notifyIconData.uTimeout = 1000/*提示超时毫秒,仅Windows 2000 和 Windows XP有效*/;
	lstrcpy(notifyIconData.szInfoTitle, TEXT("托盘最小化, 右键切换环境"));
	lstrcpy(notifyIconData.szInfo, TEXT("如需退出，请点击菜单\"操作->退出\"。"));

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
		// 如不做NULL判断，在windows7上Release模式会出现 "问题事件名称:	BEX 异常代码: c0000417"
		// Debug模式会出现corecrt_internal_string_templates.h Expression:(((source)))  != NULL
		if (ret &&  Option[0].Value.pszValue != NULL) {
			wcscpy_s(proxyText, _countof(proxyText), Option[0].Value.pszValue);
			//MessageBox(hWnd, (LPCWSTR)Option[0].Value.pszValue, TEXT("成功"), MB_OK);
		}
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
	SendMessage(hWndComboBox, CB_GETLBTEXT, idx_row, (LPARAM)proxyText);
}

// 创建指定磅值的宋体字体
HFONT MakeSongtiFont(HWND hRefWnd, int pt)
{
	HDC hdc = GetDC(hRefWnd);
	int h = -MulDiv(pt, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(hRefWnd, hdc);
	return CreateFont(h, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"宋体");
}

// 读取最新系统代理并同步到下拉框
void refreshSystemProxy(HWND hMainWnd)
{
	// 先默认为无代理，若系统未设代理则保持此值
	wcscpy_s(proxyText, _countof(proxyText), CloseProxy);
	// 重新读取当前系统代理到全局proxyText
	GetConnectProxy(hMainWnd, (LPWSTR)lanName);
	// 在下拉框里查找匹配项
	int count = (int)SendMessage(hWndComboBox, CB_GETCOUNT, 0, 0);
	int sel = -1;
	for (int k = 0; k < count; k++) {
		WCHAR item[maxLen] = { 0 };
		SendMessage(hWndComboBox, CB_GETLBTEXT, k, (LPARAM)item);
		if (wcscmp((const wchar_t*)item, (const wchar_t*)proxyText) == 0) {
			sel = k;
			break;
		}
	}
	if (sel < 0) {
		// 没有匹配项则新增一条并选中
		sel = (int)SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)proxyText);
	}
	SendMessage(hWndComboBox, CB_SETCURSEL, sel, 0);
	updateProxyText();
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

// 判断指定 PID 的进程是否仍在运行
static BOOL IsProcessRunning(DWORD pid)
{
	if (pid == 0) return FALSE;
	BOOL running = FALSE;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe32)) {
			do {
				if (pe32.th32ProcessID == pid) { running = TRUE; break; }
			} while (Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return running;
}

// 记录当前系统中指定进程名的所有 PID，返回数量。用于启动后对比找出新进程
static int SnapshotPidsByName(LPCWSTR procName, DWORD* pids, int maxPids)
{
	int n = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe32)) {
			do {
				if (_wcsicmp(pe32.szExeFile, procName) == 0) {
					if (n < maxPids) pids[n++] = pe32.th32ProcessID;
				}
			} while (Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return n;
}

// 启动应用（支持 UAC 提权）
BOOL startApp(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* ProxyExe1, BOOL show, BOOL uac)
{
	// app1 勾选 UAC 时走计划任务提权，其停止/重启由 startAppElevated 内部处理，避免重复停止和重复授权
	if (uac && process == &pro_info) {
		return startAppElevated(hWnd, process, ProxyExe1, show, 1);
	}

	// 普通启动：若已有进程，先停再开
	if ((*process).dwProcessId > 0) {
		// app1 可能上次是"提权/计划任务"方式启动，用 stopAppElevated 一并清理残留任务与进程；否则普通停止
		if (process == &pro_info) {
			stopAppElevated(process, 1);
		} else {
			stopApp(hWnd, process);
		}
		Sleep(300);
		if ((*process).dwProcessId > 0) {//停止失败
			return FALSE;
		}
	}

	ZeroMemory(process, sizeof(PROCESS_INFORMATION));

	// 以当前权限启动（子进程继承 ProxyUI 权限）
	STARTUPINFO sti;
	ZeroMemory(&sti, sizeof(STARTUPINFO));
	sti.cb = sizeof(sti);
	if (!show) {
		sti.dwFlags = STARTF_USESHOWWINDOW;
		sti.wShowWindow = SW_HIDE;
	}
	// ProxyExe1 已将程序路径用引号包裹，支持带空格路径，CreateProcess 可正确解析
	// dirPath 指定新进程工作目录，避免继承默认目录（如 C:\Windows\SysWOW64）导致相对路径找不到文件
	BOOL bRet = CreateProcess(NULL, ProxyExe1, NULL, NULL, FALSE, 0, NULL, dirPath, &sti, process);
	if (!bRet) {
		MessageBox(hWnd, TEXT("启动失败"), TEXT("失败"), MB_OK);
		return FALSE;
	}
	// 关闭子进程句柄
	CloseHandle((*process).hThread);
	CloseHandle((*process).hProcess);
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
	if ((*process).dwProcessId == 0) {
		//MessageBox(hWnd, TEXT("没有进程"), TEXT("失败"), MB_OK);
		return;
	}

	HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, (*process).dwProcessId);
	if (hProc == NULL) {
		// 打不开进程：先确认它是否真的还在。若已退出（PID 失效/被系统回收），
		// 直接清空并返回，避免对已不存在或被复用该 PID 的进程执行提权 taskkill 而误弹 UAC
		if (!IsProcessRunning((*process).dwProcessId)) {
			(*process).dwProcessId = 0;
			return;
		}
		// 可能因为权限不足无法打开管理员进程，尝试使用 taskkill 提权终止
		WCHAR pidStr[32] = { 0 };
		wsprintf(pidStr, L"/PID %lu /F", (*process).dwProcessId);
		SHELLEXECUTEINFO sei;
		ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		sei.hwnd = NULL;
		sei.lpVerb = L"runas";
		sei.lpFile = L"taskkill.exe";
		sei.lpParameters = pidStr;
		sei.nShow = SW_HIDE;
		if (ShellExecuteEx(&sei)) {
			if (sei.hProcess != NULL) {
				WaitForSingleObject(sei.hProcess, 5000);
				CloseHandle(sei.hProcess);
			}
		}
		// 验证是否确实被终止，未终止则保持 PID（避免误清零掩盖杀错进程的问题）
		if (!IsProcessRunning((*process).dwProcessId)) {
			(*process).dwProcessId = 0;
		}
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
		GetExitCodeProcess(hProc, &dwExitCode);
		TerminateProcess(hProc, dwExitCode);//终止进程
		// TerminateProcess 只是异步请求，进程不一定已被内核回收；持句柄等它真正退出再判断，
		// 否则过早的 IsProcessRunning 可能仍为真、dwProcessId 未清零，上层会误判"停止失败"而拒绝重启
		WaitForSingleObject(hProc, 2000);
	}

	// 验证是否被终止，仍在运行则保持 PID
	if (!IsProcessRunning((*process).dwProcessId)) {
		(*process).dwProcessId = 0;
	}
	CloseHandle(hProc);
}


// 模拟启动应用窗口2
void clickStartApp2(HWND hdlg)
{
	WCHAR ProxyExe2[MAX_PATH] = { 0 };
	GetDlgItemText(hdlg, IDC_PROXY_CMD2, (LPTSTR)ProxyExe2, MAX_PATH);
	if (wcscmp((const wchar_t*)ProxyExe2, (const wchar_t*)TEXT("")) == 0) {
		MessageBox(hdlg, TEXT("请选择程序"), TEXT("失败"), MB_OK);
		return;
	}
	WritePrivateProfileString(TEXT("Program"), TEXT("app2"), ProxyExe2, iniFile);

	WCHAR Params[MAX_PATH] = { 0 };
	GetDlgItemText(hdlg, IDC_EDIT4, (LPTSTR)Params, MAX_PATH);
	WritePrivateProfileString(TEXT("Program"), TEXT("param2"), Params, iniFile);

	// 保存当前选择的环境记录
	LRESULT idx_row;
	WCHAR selectText[255] = { 0 };
	HWND hComboBox = GetDlgItem(hdlg, IDC_SWITCH);
	idx_row = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
	SendMessage(hComboBox, CB_GETLBTEXT, idx_row, (LPARAM)selectText);
	WritePrivateProfileString(TEXT("Program"), TEXT("selected"), selectText, iniFile);

	// 把程序路径用双引号包起来合并参数,避免缓冲区问题
	WCHAR cmdLine2[MAX_PATH * 3] = { 0 };
	_snwprintf_s(cmdLine2, _countof(cmdLine2), _TRUNCATE, L"\"%s\" %s", ProxyExe2, Params);

	// 是否勾选后台
	UINT sta = IsDlgButtonChecked(hdlg, IDC_CHECK2);
	// 放到工作线程执行,UI 不再卡顿
	SetJobBusyUI(hdlg, 2, TRUE);
	LaunchProxyJob(hdlg, 2, JOB_START, &pro_info2, cmdLine2, sta == BST_UNCHECKED, FALSE);
}

// 设置为忙碌状态:更新状态文字并禁用启动/停止按钮
void SetJobBusyUI(HWND hdlg, int appId, BOOL starting)
{
	HWND hStatus = GetDlgItem(hdlg, (appId == 1) ? IDC_STATIC1 : IDC_STATIC2);
	HWND hStart  = GetDlgItem(hdlg, (appId == 1) ? IDC_PROXY_START1 : IDC_PROXY_START2);
	HWND hStop   = GetDlgItem(hdlg, (appId == 1) ? IDC_PROXY_STOP1 : IDC_PROXY_STOP2);
	PROCESS_INFORMATION* pi = (appId == 1) ? &pro_info : &pro_info2;
	LPCWSTR txt;
	if (!starting) {
		txt = L"停止中";
	} else {
		txt = (pi->dwProcessId > 0) ? L"重启中" : L"启动中";
	}
	SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)txt);
	EnableWindow(hStart, FALSE);
	EnableWindow(hStop, FALSE);
}

// 工作线程:执行耗时的启动/停止,完成后回发 WM_APP_JOBDONE 由UI线程刷新界面
static DWORD WINAPI AsyncJobProc(LPVOID param)
{
	AsyncJob* job = (AsyncJob*)param;
	if (job->action == JOB_START) {
		job->ok = startApp(job->hdlg, job->process, job->cmdLine, job->show, job->uac);
	} else {
		if (job->uac) {
			stopAppElevated(job->process, job->appId);
		} else {
			stopApp(job->hdlg, job->process);
		}
		job->ok = (job->process->dwProcessId == 0);
	}
	PostMessage(job->hdlg, WM_APP_JOBDONE, (WPARAM)job, 0);
	return 0;
}

// 发起异步任务;若该程序已有任务在跑则返回 FALSE(忽略本次)
BOOL LaunchProxyJob(HWND hdlg, int appId, int action, PROCESS_INFORMATION* process, const WCHAR* cmdLine, BOOL show, BOOL uac)
{
	volatile LONG* busy = (appId == 1) ? &g_jobBusy1 : &g_jobBusy2;
	if (InterlockedCompareExchange(busy, 1, 0) != 0) {
		return FALSE;  // 已有任务在进行
	}
	AsyncJob* job = new AsyncJob();
	job->appId = appId;
	job->action = action;
	job->process = process;
	job->show = show;
	job->uac = uac;
	job->hdlg = hdlg;
	job->ok = FALSE;
	job->cmdLine[0] = 0;
	if (cmdLine != NULL) {
		wcscpy_s(job->cmdLine, _countof(job->cmdLine), cmdLine);
	}
	HANDLE hThread = CreateThread(NULL, 0, AsyncJobProc, job, 0, NULL);
	if (hThread == NULL) {
		delete job;
		InterlockedExchange(busy, 0);
		return FALSE;
	}
	CloseHandle(hThread);
	return TRUE;
}

// 计划任务提权相关
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")

// 根据当前目录生成唯一任务名，确保不同目录的 ProxyUI 互不干扰
// 任务名格式: {倒数第2级目录}_{最后一级目录}_{4位hash}_App{d}，如 github_proxyui_a1b2_App1
void getElevatedTaskName(WCHAR* out, DWORD outSize, int appId)
{
	LPCWSTR dir = (LPCWSTR)dirPath;
	int len = (int)wcslen(dir);

	// 去除末尾的\，避免空尾段
	while (len > 0 && (dir[len - 1] == L'\\' || dir[len - 1] == L'/'))
		len--;

	// 找最后一级目录名（向后扫描两个\）
	LPCWSTR seg1 = NULL;  // 最后一级
	LPCWSTR seg2 = L"";   // 倒数第二级（可为空）
	int i = len - 1;
	while (i >= 0) {
		if (dir[i] == L'\\' || dir[i] == L'/') {
			if (seg1 == NULL) {
				seg1 = dir + i + 1;
			} else {
				seg2 = dir + i + 1;
				break;
			}
		}
		i--;
	}
	if (seg1 == NULL) seg1 = dir;  // 整个路径就是目录名

	// 大小写不敏感 hash，只用4位十六进制
	DWORD hash = 5381;
	LPCWSTR p = dir;
	while (*p) {
		hash = ((hash << 5) + hash) + (*p | 0x20);
		p++;
	}

	if (*seg2) {
		swprintf_s(out, outSize, L"%s_%s_%04X_App%d", seg2, seg1, hash & 0xFFFF, appId);
	} else {
		swprintf_s(out, outSize, L"%s_%04X_App%d", seg1, hash & 0xFFFF, appId);
	}
}

// 检查计划任务是否存在
BOOL ScheduledTaskExists(LPCWSTR taskName)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		CoUninitialize();
		return FALSE;
	}

	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
		IID_ITaskService, (void**)&pService);
	if (FAILED(hr)) {
		CoUninitialize();
		return FALSE;
	}

	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr)) {
		pService->Release();
		CoUninitialize();
		return FALSE;
	}

	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	pService->Release();
	if (FAILED(hr)) {
		CoUninitialize();
		return FALSE;
	}

	IRegisteredTask* pTask = NULL;
	hr = pRootFolder->GetTask(_bstr_t(taskName), &pTask);
	pRootFolder->Release();

	if (FAILED(hr) || pTask == NULL) {
		CoUninitialize();
		return FALSE;
	}

	pTask->Release();
	CoUninitialize();
	return TRUE;
}

// djb2 计算命令行 hash，用于判断重启时命令是否变化
static DWORD HashCmd(LPCWSTR s)
{
	DWORD h = 5381;
	while (*s) { h = ((h << 5) + h) + (DWORD)(*s); s++; }
	return h;
}

// 结束指定 PID 的进程：先尝试直接结束，权限不足再用 taskkill 提权；轮询等待其真正退出。返回是否已结束
static BOOL KillProcessByPid(DWORD pid)
{
	if (pid <= 1) return TRUE;
	if (!IsProcessRunning(pid)) return TRUE;  // 进程已退出，无需（提权）强杀，避免误弹 UAC
	HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
	if (hProc != NULL) {
		EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)pid);
		if (WaitForSingleObject(hProc, 1500) != WAIT_OBJECT_0) {
			DWORD ec = 0;
			GetExitCodeProcess(hProc, &ec);
			TerminateProcess(hProc, ec);
		}
		CloseHandle(hProc);
	} else {
		WCHAR pidStr[32] = { 0 };
		wsprintf(pidStr, L"/PID %lu /F", pid);
		SHELLEXECUTEINFO sei;
		ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		sei.lpVerb = L"runas";
		sei.lpFile = L"taskkill.exe";
		sei.lpParameters = pidStr;
		sei.nShow = SW_HIDE;
		if (ShellExecuteEx(&sei) && sei.hProcess != NULL) {
			WaitForSingleObject(sei.hProcess, 5000);
			CloseHandle(sei.hProcess);
		}
	}
	// 轮询等待进程真正退出（taskkill 是异步的，避免进程还没消失就误判失败导致"点两次"）
	for (int i = 0; i < 30 && IsProcessRunning(pid); i++) {
		Sleep(100);
	}
	return !IsProcessRunning(pid);
}

// 用计划任务实现提权（首次 UAC 授权，后续静默运行）
BOOL startAppElevated(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* cmdLine, BOOL show, int appId)
{
	WCHAR taskName[64];
	getElevatedTaskName(taskName, 64, appId);
	// hash 包含命令行与"后台"标志：命令或后台变化都会使 hash 不同，从而触发重建任务
	DWORD curHash = (HashCmd(cmdLine) << 1) | (show ? 1u : 0u);
	BOOL reuseTask = FALSE;

	// 若已有进程在跑（重启）：命令与后台都没变、且任务仍在，则重用任务，只结束旧进程（省去删/建任务的授权）
	if ((*process).dwProcessId > 0) {
		WCHAR hashKey[24] = { 0 }, hbuf[24] = { 0 }, curHashStr[24] = { 0 };
		swprintf_s(hashKey, L"elevhash%d", appId);
		swprintf_s(curHashStr, L"%08X", curHash);
		GetPrivateProfileString(TEXT("ProxyUI"), hashKey, TEXT(""), hbuf, 24, iniFile);
		if (wcscmp((const wchar_t*)hbuf, (const wchar_t*)curHashStr) == 0) {
			// 命令行+后台没变：先杀旧的管理员进程（这步需提权、会立即弹UAC），
			// 把耗时的计划任务COM查询挪到杀进程之后，避免"点击后卡几秒才弹UAC"
			if (!KillProcessByPid((*process).dwProcessId)) {
				return FALSE;  // 旧进程未能杀掉（如取消提权），保持原状避免多开
			}
			// 杀完再确认任务是否还在：在则复用（跳过重建），不在才落到下面重建路径
			if (ScheduledTaskExists(taskName)) {
				reuseTask = TRUE;
			}
		} else {
			// 命令/后台变化或任务丢失：完整停止（结束旧进程 + 删任务），随后重建
			stopAppElevated(process, appId);
			Sleep(300);
			if ((*process).dwProcessId > 0) {
				return FALSE;
			}
		}
	}
	// 开机首次启动时进程未运行：若计划任务已存在且命令行未变化，直接复用，
	// 用 COM 执行已有任务（无需 UAC），避免每次开机弹出授权框
	else if (ScheduledTaskExists(taskName)) {
		WCHAR hashKey[24] = { 0 }, hbuf[24] = { 0 }, curHashStr[24] = { 0 };
		swprintf_s(hashKey, L"elevhash%d", appId);
		swprintf_s(curHashStr, L"%08X", curHash);
		GetPrivateProfileString(TEXT("ProxyUI"), hashKey, TEXT(""), hbuf, 24, iniFile);
		if (wcscmp((const wchar_t*)hbuf, (const wchar_t*)curHashStr) == 0) {
			reuseTask = TRUE;
		}
	}

	ZeroMemory(process, sizeof(PROCESS_INFORMATION));

	// 分离程序路径和参数（支持带空格路径用引号包裹）
	WCHAR exePath[MAX_PATH] = { 0 };
	WCHAR cmdArgs[MAX_PATH * 2] = { 0 };
	if (cmdLine[0] == L'"') {
		// 路径被引号包裹，取引号内为路径，其后为参数
		WCHAR* closeQ = wcschr(cmdLine + 1, L'"');
		if (closeQ != NULL) {
			size_t pathLen = closeQ - (cmdLine + 1);
			if (pathLen >= MAX_PATH) pathLen = MAX_PATH - 1;
			wcsncpy_s(exePath, MAX_PATH, cmdLine + 1, pathLen);
			const WCHAR* argp = closeQ + 1;
			while (*argp == L' ') argp++;
			wcscpy_s(cmdArgs, MAX_PATH * 2, argp);
		} else {
			wcscpy_s(exePath, MAX_PATH, cmdLine + 1);
		}
	} else {
		WCHAR* spacePos = wcschr(cmdLine, L' ');
		if (spacePos != NULL) {
			size_t pathLen = spacePos - cmdLine;
			if (pathLen >= MAX_PATH) pathLen = MAX_PATH - 1;
			wcsncpy_s(exePath, MAX_PATH, cmdLine, pathLen);
			wcscpy_s(cmdArgs, MAX_PATH * 2, spacePos + 1);
		} else {
			wcscpy_s(exePath, MAX_PATH, cmdLine);
		}
	}

	// 命令首次设置或已变化：（重）创建计划任务；命令与后台都没变的重启会重用现有任务，跳过创建
	if (!reuseTask)
	{
		// dirPath 末尾带\，需先去掉
		WCHAR cleanDir[MAX_PATH];
		wcscpy_s(cleanDir, MAX_PATH, (LPCWSTR)dirPath);
		int cdl = (int)wcslen(cleanDir);
		while (cdl > 0 && cleanDir[cdl - 1] == L'\\') cleanDir[--cdl] = 0;

		// 用 VBS 脚本启动，通过 wscript.exe 执行（wscript 是 GUI 程序，无控制台窗口）
		WCHAR vbsPath[MAX_PATH];
		swprintf_s(vbsPath, MAX_PATH, L"%s\\_pl%d.vbs", cleanDir, appId);

		// VBS 引号转义：VBS 中两个双引号 "" 表示一个字面双引号
		WCHAR vbDir[MAX_PATH * 2] = { 0 }, vbExe[MAX_PATH * 2] = { 0 }, vbArgs[MAX_PATH * 4] = { 0 };
		WCHAR *d;
		for (const WCHAR *s = cleanDir; *s; s++) { d = vbDir + wcslen(vbDir); *d++ = *s; if (*s == L'"') *d++ = L'"'; *d = 0; }
		for (const WCHAR *s = exePath; *s; s++)  { d = vbExe + wcslen(vbExe); *d++ = *s; if (*s == L'"') *d++ = L'"'; *d = 0; }
		for (const WCHAR *s = cmdArgs; *s; s++)  { d = vbArgs + wcslen(vbArgs); *d++ = *s; if (*s == L'"') *d++ = L'"'; *d = 0; }

		int winStyle = show ? 1 : 0;  // 1=正常, 0=隐藏

		WCHAR vbs[4096];
		int vbsLen = swprintf_s(vbs, L"Set W=CreateObject(\"WScript.Shell\")\r\nW.CurrentDirectory=\"%s\"\r\nW.Run \"\"\"%s\"\" %s\",%d,0\r\n",
			vbDir, vbExe, vbArgs, winStyle);

		HANDLE hVbs = CreateFileW(vbsPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hVbs != INVALID_HANDLE_VALUE) {
			DWORD wr;
			WORD bom = 0xFEFF;
			WriteFile(hVbs, &bom, 2, &wr, NULL);
			WriteFile(hVbs, vbs, (DWORD)(vbsLen * sizeof(WCHAR)), &wr, NULL);
			CloseHandle(hVbs);
		}

		WCHAR schtasksCmd[2048];
		swprintf_s(schtasksCmd, L"/create /tn \"%s\" /tr \"wscript.exe //B //Nologo \\\"%s\\\"\" /sc once /st 00:00 /sd 2000/01/01 /rl highest /f",
			taskName, vbsPath);

		SHELLEXECUTEINFO sei;
		ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
		sei.cbSize = sizeof(SHELLEXECUTEINFO);
		sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
		sei.hwnd = hWnd;
		sei.lpVerb = L"runas";
		sei.lpFile = L"schtasks.exe";
		sei.lpParameters = schtasksCmd;
		sei.nShow = SW_HIDE;

		if (!ShellExecuteEx(&sei)) {
			DWORD err = GetLastError();
			if (err == ERROR_CANCELLED) {
				MessageBox(hWnd, TEXT("用户取消了UAC授权，计划任务未创建"), TEXT("提示"), MB_OK);
			} else {
				MessageBox(hWnd, TEXT("创建计划任务失败，可能需要管理员权限"), TEXT("失败"), MB_OK);
			}
			return FALSE;
		}
		if (sei.hProcess != NULL) {
			WaitForSingleObject(sei.hProcess, 30000);
			DWORD exitCode = 1;
			GetExitCodeProcess(sei.hProcess, &exitCode);
			CloseHandle(sei.hProcess);
			if (exitCode != 0) {
				WCHAR msg[512];
				swprintf_s(msg, L"创建计划任务失败 (schtasks exit=%d)\n命令: %s", exitCode, schtasksCmd);
				MessageBox(hWnd, msg, TEXT("失败"), MB_OK);
				return FALSE;
			}
		}

		// 再次确认任务创建成功
		if (!ScheduledTaskExists(taskName)) {
			MessageBox(hWnd, TEXT("计划任务创建后仍不存在"), TEXT("失败"), MB_OK);
			return FALSE;
		}
		// 记录本次命令+后台的 hash，供下次重启判断是否变化以决定能否重用任务
		WCHAR hkey[24] = { 0 }, hstr[24] = { 0 };
		swprintf_s(hkey, L"elevhash%d", appId);
		swprintf_s(hstr, L"%08X", curHash);
		WritePrivateProfileString(TEXT("ProxyUI"), hkey, hstr, iniFile);
	}

	// 启动前记录已存在的同名进程 PID，便于启动后区分出新进程（多开时不再认错 PID）
	WCHAR* preLastSlash = wcsrchr(exePath, L'\\');
	LPCWSTR procName = preLastSlash ? (preLastSlash + 1) : exePath;
	DWORD preExisting[256];
	int preCount = SnapshotPidsByName(procName, preExisting, 256);

	// 用 COM API 触发任务，不依赖 schtasks.exe（无控制台窗口）
	{
		HRESULT hr = E_FAIL;
		HRESULT hrInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (SUCCEEDED(hrInit)) {
			ITaskService* pService = NULL;
			hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
				IID_ITaskService, (void**)&pService);
			if (SUCCEEDED(hr)) {
				hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
				if (SUCCEEDED(hr)) {
					ITaskFolder* pRootFolder = NULL;
					hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
					if (SUCCEEDED(hr)) {
						IRegisteredTask* pRegTask = NULL;
						hr = pRootFolder->GetTask(_bstr_t(taskName), &pRegTask);
						if (SUCCEEDED(hr) && pRegTask != NULL) {
							IRunningTask* pRunning = NULL;
							hr = pRegTask->Run(_variant_t(), &pRunning);
							if (pRunning) pRunning->Release();
						}
						if (pRegTask) pRegTask->Release();
						pRootFolder->Release();
					}
				}
				pService->Release();
			}
			CoUninitialize();
		}

		if (FAILED(hr)) {
			MessageBox(hWnd, TEXT("执行计划任务失败"), TEXT("失败"), MB_OK);
			return FALSE;
		}
	}

	// 等待新进程出现：只接受不在 preExisting 中的新 PID，确保是本次启动的实例
	{
		int retry = 0;
		while (retry < 30) {
			Sleep(200);
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hSnapshot != INVALID_HANDLE_VALUE) {
				PROCESSENTRY32 pe32;
				pe32.dwSize = sizeof(PROCESSENTRY32);
				if (Process32First(hSnapshot, &pe32)) {
					do {
						if (_wcsicmp(pe32.szExeFile, procName) == 0 &&
							pe32.th32ProcessID != GetCurrentProcessId()) {
							BOOL isOld = FALSE;
							for (int k = 0; k < preCount; k++) {
								if (preExisting[k] == pe32.th32ProcessID) { isOld = TRUE; break; }
							}
							if (!isOld) {
								process->dwProcessId = pe32.th32ProcessID;
								CloseHandle(hSnapshot);
								goto foundPid;
							}
						}
					} while (Process32Next(hSnapshot, &pe32));
				}
				CloseHandle(hSnapshot);
			}
			retry++;
		}
	}
foundPid:

	if (process->dwProcessId == 0) {
		// 超时未找到新进程，视为失败
		return FALSE;
	}

	return TRUE;
}

// 停止通过计划任务启动的应用（结束进程 + 删除计划任务 + 删除 VBS，尽量合并到一次提权）
void stopAppElevated(PROCESS_INFORMATION* process, int appId)
{
	WCHAR taskName[64];
	getElevatedTaskName(taskName, 64, appId);
	DWORD pid = (*process).dwProcessId;

	// VBS 临时脚本路径（dirPath 末尾的 \ 先去掉）
	WCHAR cleanDir[MAX_PATH];
	wcscpy_s(cleanDir, MAX_PATH, (LPCWSTR)dirPath);
	int cdl = (int)wcslen(cleanDir);
	while (cdl > 0 && cleanDir[cdl - 1] == L'\\') cleanDir[--cdl] = 0;
	WCHAR vbsPath[MAX_PATH];
	swprintf_s(vbsPath, MAX_PATH, L"%s\\_pl%d.vbs", cleanDir, appId);

	// 1) 先尝试不提权结束进程（ProxyUI 未提权、进程也非提权时可成功）
	if (pid > 1) {
		HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
		if (hProc != NULL) {
			EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)pid);
			if (WaitForSingleObject(hProc, 2000) != WAIT_OBJECT_0) {
				DWORD dwExitCode = 0;
				GetExitCodeProcess(hProc, &dwExitCode);
				TerminateProcess(hProc, dwExitCode);
			}
			CloseHandle(hProc);
		}
	}

	// 2) 先尝试用 COM 停止并删除计划任务（有权限时生效，无窗口无提示）
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (SUCCEEDED(hr)) {
			ITaskService* pService = NULL;
			if (SUCCEEDED(CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
				IID_ITaskService, (void**)&pService))) {
				if (SUCCEEDED(pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()))) {
					ITaskFolder* pRoot = NULL;
					if (SUCCEEDED(pService->GetFolder(_bstr_t(L"\\"), &pRoot))) {
						IRegisteredTask* pTask = NULL;
						if (SUCCEEDED(pRoot->GetTask(_bstr_t(taskName), &pTask)) && pTask != NULL) {
							pTask->Stop(0);
							pTask->Release();
						}
						pRoot->DeleteTask(_bstr_t(taskName), 0);
						pRoot->Release();
					}
				}
				pService->Release();
			}
			CoUninitialize();
		}
	}

	// 3) 若进程或任务仍在，用「一次」提权命令一并结束进程、删除计划任务、删 VBS
	BOOL procAlive = (pid > 1 && IsProcessRunning(pid));
	BOOL taskAlive = ScheduledTaskExists(taskName);
	if (procAlive || taskAlive) {
		WCHAR elevCmd[1024];
		if (procAlive) {
			swprintf_s(elevCmd, L"/c taskkill /PID %lu /F & schtasks /delete /tn \"%s\" /f & del \"%s\"",
				pid, taskName, vbsPath);
		} else {
			swprintf_s(elevCmd, L"/c schtasks /delete /tn \"%s\" /f & del \"%s\"",
				taskName, vbsPath);
		}
		SHELLEXECUTEINFO sei = { sizeof(sei) };
		sei.fMask = SEE_MASK_NOCLOSEPROCESS;
		sei.hwnd = hfDlg;
		sei.lpVerb = L"runas";
		sei.lpFile = L"cmd.exe";
		sei.lpParameters = elevCmd;
		sei.nShow = SW_HIDE;
		if (ShellExecuteEx(&sei) && sei.hProcess != NULL) {
			WaitForSingleObject(sei.hProcess, 10000);
			CloseHandle(sei.hProcess);
		}
	} else {
		// 进程和任务都没了，顺手删掉不需要提权的 VBS 文件
		DeleteFileW(vbsPath);
	}

	// 轮询等待进程真正退出后再清零 PID（避免异步 kill 未完成就误判，导致"点两次"）
	for (int i = 0; i < 30 && IsProcessRunning((*process).dwProcessId); i++) {
		Sleep(100);
	}
	if (!IsProcessRunning((*process).dwProcessId)) {
		(*process).dwProcessId = 0;
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
