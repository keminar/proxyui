// ProxyUI.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "ProxyUI.h"
#include <ShellAPI.h>
#include <Shlwapi.h>
#include "commctrl.h"
#include <windows.h>
#include "wininet.h"

#define MAX_LOADSTRING 100

// ȫ�ֱ���: 
HINSTANCE hInst;                                // ��ǰʵ��
WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING];            // ����������
WCHAR ProxyText[255] = { 0 }; // �������
WCHAR lanName[255] = { 0 }; // ��������

// �˴���ģ���а����ĺ�����ǰ������: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")

#define WM_CLICKBIT (WM_USER + 1)

HWND hWndComboBox, hWndBtn1;
#define IDC_PROXY_SERVER 100
#define IDC_PROXY_OK 101

#define CloseProxy TEXT("�޴���")
#define RegRun L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RegName L"ProxyUI"
#define IniFile L".\\ProxyUI.ini"

BOOL SetConnectionOptions(HWND hWnd, LPWSTR conn_name, LPWSTR proxy_full_addr);
BOOL DisableConnectionProxy(HWND hWnd, LPWSTR conn_name);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: �ڴ˷��ô��롣

    // ��ʼ��ȫ���ַ���
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PROXYUI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // ִ��Ӧ�ó����ʼ��: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROXYUI));

    MSG msg;

    // ����Ϣѭ��: 
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
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
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
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 630, 500, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

// ����ȫ�ֱ���
void updateProxyText()
{
	int idx_row;
	idx_row = SendMessage(hWndComboBox, CB_GETCURSEL, 0, 0);
	SendMessage(hWndComboBox, CB_GETLBTEXT, idx_row, (LPARAM)ProxyText);
}
//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND  - ����Ӧ�ó���˵�
//  WM_PAINT    - ����������
//  WM_DESTROY  - �����˳���Ϣ������
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		{
			CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"ϵͳ����",
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				10, 10, 100, 30,
				hWnd, NULL, NULL, NULL);
			hWndComboBox = CreateWindowEx(WS_EX_STATICEDGE, L"COMBOBOX", L"������",
				CBS_DROPDOWN | CBS_HASSTRINGS | WS_VISIBLE | WS_CHILD | WS_BORDER,
				120, 10, 350, 500, hWnd, (HMENU)IDC_PROXY_SERVER, NULL, NULL);
			hWndBtn1 = CreateWindowEx(WS_EX_STATICEDGE, L"BUTTON", L"ȷ��",
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				480, 10, 100, 30,
				hWnd, (HMENU)IDC_PROXY_OK, NULL, NULL);
 
			//  ADD 2 ITEMS
			SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)CloseProxy);

			TCHAR inBuf[255];
			GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), inBuf, 255,IniFile);
			if (strnlen_s((const char*)inBuf, 255) == 0) {//���ļ��������ļ�
				WritePrivateProfileString(L"Server", L"List", L"http=127.0.0.1:3000;https=127.0.0.1:3000|http=127.0.0.1:8888;https=127.0.0.1:8888", IniFile);
				GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), inBuf, 255, IniFile);
			}
			// �ָ��ַ����������Items
			wchar_t *buffer;
			wchar_t *token = wcstok_s(inBuf, L"|", &buffer);
			while (token) {
				SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)token);
				token = wcstok_s(NULL, L"|", &buffer);
			}

			//  SEND THE CB_SETCURSEL MESSAGE TO DISPLAY AN INITIAL ITEM IN SELECTION FIELD
			SendMessageW(hWndComboBox, CB_SETCURSEL, 0, (LPARAM)0);
			updateProxyText();
		}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // �����˵�ѡ��: 
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
						MessageBox(hWnd, TEXT("�ѳɹ�ȡ������"), TEXT("�ɹ�"), MB_OK);
					}
					else {
						SetConnectionOptions(hWnd, (LPWSTR)lanName, (LPWSTR)ProxyText);
						MessageBox(hWnd, (LPCWSTR)ProxyText, TEXT("������������"), MB_OK);
					}
				}
				break;
			case IDM_START:
				{
					BOOL AutoStart = false;
					AutoStart = SetAutoRun(hWnd);
					if (AutoStart) {
						MessageBox(hWnd, TEXT("����Ϊ�����Զ�����"), TEXT("�ɹ�"), MB_OK);
					}
					else {
						MessageBox(hWnd, TEXT("���ÿ���������ʧ��"), TEXT("ʧ��"), MB_OK);
					}
				}
				break;
			case IDM_STOP:
			{
				BOOL NoAutoStart = false;
				NoAutoStart = SetNoAutoRun(hWnd);
				if (NoAutoStart) {
					MessageBox(hWnd, TEXT("��ȡ�������Զ�����"), TEXT("�ɹ�"), MB_OK);
				}
				else {
					MessageBox(hWnd, TEXT("��ǰû�п�������"), TEXT("ʧ��"), MB_OK);
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
            // TODO: �ڴ˴����ʹ�� hdc ���κλ�ͼ����...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_CLOSE://�رմ���ʱ��С��������
		BuildTrayIcon(hWnd);
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_CLICKBIT:
	{
		switch (lParam)
		{
		case WM_LBUTTONUP://����ͼ�껹ԭ����
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

// �����ڡ������Ϣ�������
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

//�����Զ����� 
BOOL SetAutoRun(HWND hwnd)
{
	//�õ�������·��
	WCHAR sthPath[MAX_PATH];
	GetModuleFileName(NULL, sthPath, MAX_PATH);
	//MessageBox(NULL, sthPath, "path", MB_OK);

	WCHAR str[MAX_PATH];
	HKEY hRegKey;
	BOOL bResult;
	lstrcpy(str, RegRun);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, str, 0, KEY_ALL_ACCESS, &hRegKey) != ERROR_SUCCESS) {
		bResult = FALSE;
	}
	else {
		if (RegSetValueEx(hRegKey, RegName, 0, REG_SZ, (BYTE*)sthPath, lstrlen(sthPath) + 1) != ERROR_SUCCESS) {
			bResult = FALSE;
		}
		else {
			bResult = TRUE;
		}
		RegCloseKey(hRegKey);
	}

	return bResult;
}

//�رտ����Զ����� 
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

//��С�������� 
void BuildTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA notifyIconData;
	ZeroMemory(&notifyIconData, sizeof(notifyIconData));
	notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	notifyIconData.hIcon = LoadIcon((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
		MAKEINTRESOURCE(IDI_PROXYUI));
	notifyIconData.uID = IDI_PROXYUI;
	notifyIconData.hWnd = hwnd;
	notifyIconData.uCallbackMessage = WM_CLICKBIT; //�Զ�����Ϣ
	lstrcpy(notifyIconData.szTip, TEXT("ProxyUI"));

	notifyIconData.dwState = NIS_SHAREDICON;
	notifyIconData.uTimeout = 1000/*��ʱ������*/;
	notifyIconData.dwInfoFlags = NIIF_NONE;
	lstrcpy(notifyIconData.szInfoTitle, TEXT("������С��;-)��"));
	lstrcpy(notifyIconData.szInfo, TEXT("�����˳�������\"����->�˳�\"�� �ٺ�~~~"));

	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

//����ϵͳ����ͼ�� 
void DestroyTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA notifyIconData;
	ZeroMemory(&notifyIconData, sizeof(notifyIconData));
	notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.uID = IDI_PROXYUI;
	notifyIconData.hWnd = hwnd;
	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
}


// ���ô���
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
		MessageBox(hWnd, TEXT("failed to allocat memory in SetConnectionOptions()"), TEXT("ʧ��"), MB_OK);
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

// ȡ������
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
		MessageBox(hWnd, TEXT("failed to allocat memory in DisableConnectionProxy()"), TEXT("ʧ��"), MB_OK);
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
