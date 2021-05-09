// ProxyUI.cpp : 定义应用程序的入口点。
//

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

#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
WCHAR ProxyText[255] = { 0 }; // 代理变量
WCHAR lanName[255] = { 0 }; // 网络连接
WCHAR filePath[MAX_PATH];
CString dirPath;


PROCESS_INFORMATION pro_info; //进程信息 
PROCESS_INFORMATION pro_info2; //进程信息  

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")

#define WM_CLICKBIT (WM_USER + 1)

HWND hWndComboBox, hWndBtn1;
HWND hfDlg;

#define CloseProxy TEXT("无代理")
#define RegRun L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RegName L"ProxyUI"
#define maxLen 1024

BOOL SetConnectionOptions(HWND hWnd, LPWSTR conn_name, LPWSTR proxy_full_addr);
BOOL DisableConnectionProxy(HWND hWnd, LPWSTR conn_name);

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

   //得到程序本身路径
   GetModuleFileName(NULL, filePath, MAX_PATH);
   dirPath = filePath;
   dirPath = dirPath.Left(dirPath.ReverseFind(TEXT('\\'))) + "\\";

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 660, 530, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
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
void startApp(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* ProxyExe1, BOOL show)
{
	// 检查进程是否在
	if ((*process).hProcess > 0) {
		MessageBox(hWnd, TEXT("先停止再启动"), TEXT("失败"), MB_OK);
		return;
	}

	STARTUPINFO sti; //启动信息   
	ZeroMemory(process, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&sti, sizeof(STARTUPINFO));
	sti.cb = sizeof(sti);
	if (!show) {
		sti.dwFlags = STARTF_USESHOWWINDOW;
		sti.wShowWindow = SW_HIDE;
	}
	//第二个参数是第一个命令要接收的参数
	BOOL bRet = FALSE;
	bRet = CreateProcess(NULL, ProxyExe1, NULL, NULL, FALSE, 0, NULL, NULL, &sti, process);
	if (!bRet)
	{
		MessageBox(hWnd, TEXT("启动失败"), TEXT("失败"), MB_OK);
		return;
	}
}
void writeIni(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString)
{
	TCHAR inBuf[maxLen];
	CString iniFile;
	iniFile = dirPath + L"ProxyUI.ini";
	//GetPrivateProfileString(lpAppName, lpKeyName, TEXT(""), inBuf, maxLen, iniFile);
	//if (strcmp((const char*)inBuf, "") == 0) {//无文件，创建文件
		WritePrivateProfileString(lpAppName, lpKeyName, lpString, iniFile);
	//}
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

void stopApp(HWND hWnd, PROCESS_INFORMATION* process)
{
	// 检查进程是否在
	if ((*process).hProcess == 0) {
		MessageBox(hWnd, TEXT("没有进程"), TEXT("失败"), MB_OK);
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
}

//FORMVIEW 回调消息
LRESULT CALLBACK DlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hdlg, &ps);

			//默认值
			HWND hCmd1;
			CString iniFile;
			TCHAR inBuf1[maxLen];
			iniFile = dirPath + L"ProxyUI.ini";

			GetPrivateProfileString(TEXT("Program"), TEXT("app1"), TEXT(""), inBuf1, maxLen, iniFile);
			hCmd1 = GetDlgItem(hdlg, IDC_PROXY_CMD1);
			SendMessage(hCmd1, WM_SETTEXT, NULL, (LPARAM)inBuf1);

			GetPrivateProfileString(TEXT("Program"), TEXT("param1"), TEXT(""), inBuf1, maxLen, iniFile);
			hCmd1 = GetDlgItem(hdlg, IDC_EDIT2);
			SendMessage(hCmd1, WM_SETTEXT, NULL, (LPARAM)inBuf1);
			

			GetPrivateProfileString(TEXT("Program"), TEXT("app2"), TEXT(""), inBuf1, maxLen, iniFile);
			hCmd1 = GetDlgItem(hdlg, IDC_PROXY_CMD2);
			SendMessage(hCmd1, WM_SETTEXT, NULL, (LPARAM)inBuf1);

			GetPrivateProfileString(TEXT("Program"), TEXT("param2"), TEXT(""), inBuf1, maxLen, iniFile);
			hCmd1 = GetDlgItem(hdlg, IDC_EDIT4);
			SendMessage(hCmd1, WM_SETTEXT, NULL, (LPARAM)inBuf1);

			// 默认选中
			CheckDlgButton(hdlg, IDC_CHECK1, BST_CHECKED);
			CheckDlgButton(hdlg, IDC_CHECK2, BST_CHECKED);
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
					if (strcmp((const char*)ProxyExe1, "") == 0) {
						MessageBox(hdlg, TEXT("先选择程序"), TEXT("失败"), MB_OK);
						break;
					}
					WCHAR Params[MAX_PATH] = { 0 };
					GetDlgItemText(hdlg, IDC_EDIT2, (LPTSTR)Params, MAX_PATH);

					lstrcat(ProxyExe1, TEXT(" "));
					lstrcat(ProxyExe1, Params);
					// 是否选中后台
					UINT sta = IsDlgButtonChecked(hdlg, IDC_CHECK1);
					startApp(hdlg, &pro_info, ProxyExe1, sta == BST_UNCHECKED);
					writeIni(TEXT("Program"), TEXT("app1"), ProxyExe1);
					writeIni(TEXT("Program"), TEXT("param1"), Params);
					HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
					SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
				}
				break;
				case IDC_PROXY_STOP1:
				{
					stopApp(hdlg, &pro_info);
					HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
					SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"未运行");
				}
				break;
				case IDC_PROXY_FILE2:
				{
					selectApplication(hdlg, IDC_PROXY_CMD2);
				}
				break;
				case IDC_PROXY_START2:
				{
					WCHAR ProxyExe2[MAX_PATH] = { 0 };
					GetDlgItemText(hdlg, IDC_PROXY_CMD2, (LPTSTR)ProxyExe2, MAX_PATH);
					if (strcmp((const char*)ProxyExe2, "") == 0) {
						MessageBox(hdlg, TEXT("先选择程序"), TEXT("失败"), MB_OK);
						break;
					}
					WCHAR Params[MAX_PATH] = { 0 };
					GetDlgItemText(hdlg, IDC_EDIT4, (LPTSTR)Params, MAX_PATH);

					lstrcat(ProxyExe2, TEXT(" "));
					lstrcat(ProxyExe2, Params);
					//MessageBox(hdlg, ProxyExe2, TEXT("失败"), MB_OK);

					// 是否选中后台
					UINT sta = IsDlgButtonChecked(hdlg, IDC_CHECK1);
					startApp(hdlg, &pro_info2, ProxyExe2, sta == BST_UNCHECKED);
					writeIni(TEXT("Program"), TEXT("app2"), ProxyExe2);
					writeIni(TEXT("Program"), TEXT("param2"), Params);
					HWND hStatus = GetDlgItem(hdlg, IDC_STATIC2);
					SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"运行中");
				}
				break;
				case IDC_PROXY_STOP2:
				{
					stopApp(hdlg, &pro_info2);
					HWND hStatus = GetDlgItem(hdlg, IDC_STATIC2);
					SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"未运行");
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
	case WM_CREATE:
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
 
			// 添加默认代理
			SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)CloseProxy);

			TCHAR inBuf[maxLen];
			CString iniFile;
			iniFile = dirPath + L"ProxyUI.ini";
			GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), inBuf, maxLen, iniFile);
			if (strcmp((const char*)inBuf, "") == 0) {//无文件，创建文件
				WritePrivateProfileString(L"Server", L"List", L"http=127.0.0.1:3000;https=127.0.0.1:3000|http=127.0.0.1:8888;https=127.0.0.1:8888", iniFile);
				GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), inBuf, maxLen, iniFile);
			}
			// 分割字符串，并添加Items
			wchar_t *buffer;
			wchar_t *token = wcstok_s(inBuf, L"|", &buffer);
			while (token) {
				SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)token);
				token = wcstok_s(NULL, L"|", &buffer);
			}

			// 选中第一个值
			SendMessageW(hWndComboBox, CB_SETCURSEL, 0, (LPARAM)0);
			updateProxyText();
			
			// 插入FORMVIEW
			hfDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)DlgProc);
			ShowWindow(hfDlg, SW_SHOW);
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
			case IDC_PROXY_OK:
				{
					if (strcmp((const char*)ProxyText, (const char*)CloseProxy) == 0) {
						DisableConnectionProxy(hWnd, (LPWSTR)lanName);
						MessageBox(hWnd, TEXT("已成功取消代理"), TEXT("成功"), MB_OK);
					}
					else {
						SetConnectionOptions(hWnd, (LPWSTR)lanName, (LPWSTR)ProxyText);
						MessageBox(hWnd, (LPCWSTR)ProxyText, TEXT("代理设置如下"), MB_OK);
					}
				}
				break;
			case IDM_START:
				{
					BOOL AutoStart = false;
					AutoStart = SetAutoRun(hWnd);
					if (AutoStart) {
						MessageBox(hWnd, TEXT("已设为开机自动运行"), TEXT("成功"), MB_OK);
					}
					else {
						MessageBox(hWnd, TEXT("设置开机自运行失败"), TEXT("失败"), MB_OK);
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
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
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
        PostQuitMessage(0);
        break;
	case WM_CLOSE://关闭窗口时最小化到托盘
		BuildTrayIcon(hWnd);
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_CLICKBIT:
	{
		switch (lParam)
		{
		case WM_LBUTTONUP://托盘图标还原窗口
			ShowWindow(hWnd, SW_SHOWNORMAL);
			DestroyTrayIcon(hWnd);
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
BOOL SetAutoRun(HWND hwnd)
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
		//wcslen 返回的是字符串中的字符数, 在 UNICODE 编码中，一个字符占2个字节
		if (RegSetValueEx(hRegKey, RegName, 0, REG_SZ, (BYTE*)filePath, (DWORD)wcslen(filePath) * 2) != ERROR_SUCCESS) {
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

//最小化到托盘 
void BuildTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA notifyIconData;
	ZeroMemory(&notifyIconData, sizeof(notifyIconData));
	notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	notifyIconData.hIcon = LoadIcon((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		MAKEINTRESOURCE(IDI_PROXYUI));
	notifyIconData.uID = IDI_PROXYUI;
	notifyIconData.hWnd = hwnd;
	notifyIconData.uCallbackMessage = WM_CLICKBIT; //自定义消息
	lstrcpy(notifyIconData.szTip, TEXT("ProxyUI"));

	notifyIconData.dwState = NIS_SHAREDICON;
	notifyIconData.uTimeout = 1000/*超时毫秒数*/;
	notifyIconData.dwInfoFlags = NIIF_NONE;
	lstrcpy(notifyIconData.szInfoTitle, TEXT("托盘最小化;-)："));
	lstrcpy(notifyIconData.szInfo, TEXT("如需退出，请点击\"操作->退出\"。 嘿嘿~~~"));

	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
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


// 设置代理
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
