// ProxyUI.cpp : ����Ӧ�ó������ڵ㡣
// win32�ĵ� https://docs.microsoft.com/en-us/windows/win32/api/

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

// ȫ�ֱ���: 
HINSTANCE hInst;                                // ��ǰʵ��
WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING];            // ����������
WCHAR ProxyText[255] = { 0 }; // �������
WCHAR lanName[255] = { 0 }; // ��������
WCHAR filePath[MAX_PATH];//����·��
CString dirPath; //��������Ŀ¼
CString iniFile; //ini�ļ�·��
WCHAR startCmdLine[MAX_PATH]; //��������
BOOL initFormFlag = FALSE; //��ʼ��Form���
BOOL firstTray = TRUE; //�״���С��

PROCESS_INFORMATION pro_info; //������Ϣ 
PROCESS_INFORMATION pro_info2; //������Ϣ  

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")

#define WM_CLICKBIT (WM_USER + 1)

HWND hWndComboBox, hWndBtn1;
HWND hfDlg;

#define CloseProxy TEXT("�޴���")
#define RegRun L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RegName L"ProxyUI"
#define IniName L"ProxyUI.ini"
//��ȡ��󳤶�
#define maxLen 1024

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

	//�õ�������·��
	GetModuleFileName(NULL, filePath, MAX_PATH);
	dirPath = filePath;
	dirPath = dirPath.Left(dirPath.ReverseFind(TEXT('\\'))) + "\\";
	iniFile = dirPath + IniName;
	wcscpy_s(startCmdLine, _countof(startCmdLine), lpCmdLine);

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
      CW_USEDEFAULT, CW_USEDEFAULT, 660, 530, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // �ϴν���ID
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
				   MessageBox(hWnd, TEXT("�����Ѵ򿪣����ܴ򿪶��Ŷ������Ҫ�࿪���ڲ������л�����ģʽ"), TEXT("ʧ��"), MB_OK);
				   return FALSE;
			   }
		   }
	   }
   }

   DWORD processId = GetCurrentProcessId();//��ǰ����id
	// �Ȱѱ���д��
   TCHAR str[0x20];
   memset(str, 0, 0x20);
   wsprintf(str, TEXT("%d"), processId);
   WritePrivateProfileString(TEXT("ProxyUI"), TEXT("pid"), (LPCWSTR)str, iniFile);

   // ��鿪����С��
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

// ��ʼ����ֵ
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

	// �����л�
	HWND hComboBox = GetDlgItem(hdlg, IDC_SWITCH);
	GetPrivateProfileString(TEXT("Program"), TEXT("switch"), TEXT(""), inBuf1, MAX_PATH, iniFile);
	if (wcscmp((const wchar_t*)inBuf1, (const wchar_t*)TEXT("")) == 0) {//�����ݣ���ʼ��ģ��
		WritePrivateProfileString(L"Program", L"switch", L"online|test", iniFile);
		WritePrivateProfileString(L"Env", L"online", L"-c online.yaml", iniFile);
		WritePrivateProfileString(L"Env", L"test", L"-c test.yaml", iniFile);
		GetPrivateProfileString(TEXT("Program"), TEXT("switch"), TEXT(""), inBuf1, MAX_PATH, iniFile);
	}
	// �ϴ�ѡ�еĻ���
	GetPrivateProfileString(TEXT("Program"), TEXT("selected"), TEXT(""), inBuf2, MAX_PATH, iniFile);
	SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
	// �ָ��ַ����������Items
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
	// �ػ��߶�
	RECT rect;
	GetClientRect(hComboBox, &rect);
	MapDialogRect(hComboBox, &rect);
	SetWindowPos(hComboBox, 0, 0, 0, rect.right, (i + 1) * rect.bottom, SWP_NOMOVE);

	// Ĭ��ѡ�к�̨�򿪽���
	CheckDlgButton(hdlg, IDC_CHECK1, BST_CHECKED);
	CheckDlgButton(hdlg, IDC_CHECK2, BST_CHECKED);

	// ��������״̬
	if (pro_info.hProcess > 0) {
		HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
		SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"������");
		HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START1);
		SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"����");
	}
	if (pro_info2.hProcess > 0) {
		HWND hStatus = GetDlgItem(hdlg, IDC_STATIC2);
		SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"������");
		HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START2);
		SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"����");
	}
	initFormFlag = true;
}

//FORMVIEW �ص���Ϣ
LRESULT CALLBACK DlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_SHOWWINDOW://�����ʼ�����������Ҽ�����ȡ��ֵ
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
				// �����˵�ѡ��: 
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
								MessageBox(hdlg, TEXT("��ѡ�����"), TEXT("ʧ��"), MB_OK);
								break;
							}
							// �Ȱѱ���д��
							WritePrivateProfileString(TEXT("Program"), TEXT("app1"), ProxyExe1, iniFile);

							WCHAR Params[MAX_PATH] = { 0 };
							GetDlgItemText(hdlg, IDC_EDIT2, (LPTSTR)Params, MAX_PATH);
							WritePrivateProfileString(TEXT("Program"), TEXT("param1"), Params, iniFile);

							lstrcat(ProxyExe1, TEXT(" "));
							lstrcat(ProxyExe1, Params);
							// �Ƿ�ѡ�к�̨
							UINT sta = IsDlgButtonChecked(hdlg, IDC_CHECK1);
							BOOL ret = startApp(hdlg, &pro_info, ProxyExe1, sta == BST_UNCHECKED);
							if (ret) {
								HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
								SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"������");
								HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START1);
								SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"����");
							}

						}
						break;
					case IDC_PROXY_STOP1:
						{
							stopApp(hdlg, &pro_info);
							if (pro_info.hProcess == 0) {
								HWND hStatus = GetDlgItem(hdlg, IDC_STATIC1);
								SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"δ����");
								HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START1);
								SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"����");
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
								// ��ȡѡ��
								LRESULT idx_row;
								WCHAR selectText[MAX_LOADSTRING] = { 0 };
								HWND hComboBox = GetDlgItem(hdlg, IDC_SWITCH);
								idx_row = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
								SendMessage(hComboBox, CB_GETLBTEXT, idx_row, (LPARAM)selectText);

								// ��ȡ���õĲ���
								WCHAR params[MAX_PATH] = { 0 };
								GetPrivateProfileString(TEXT("Env"), selectText, TEXT(""), params, MAX_PATH, iniFile);
								// ���õ����������
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
								SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"δ����");
								HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START2);
								SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"����");
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
	case WM_CREATE: // ����InitInstance����������
		{
			CreateWindowEx(WS_EX_STATICEDGE, L"STATIC", L"  ϵͳ����",
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
 
			// ���Ĭ�ϴ���ѡ��
			SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)CloseProxy);

			TCHAR inBuf[maxLen];
			GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), inBuf, maxLen, iniFile);
			if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("")) == 0) {//���ļ��������ļ�
				WritePrivateProfileString(L"Server", L"List", L"http=127.0.0.1:3000;https=127.0.0.1:3000|http=127.0.0.1:8888;https=127.0.0.1:8888", iniFile);
				GetPrivateProfileString(TEXT("Server"), TEXT("List"), TEXT(""), inBuf, maxLen, iniFile);
			}

			//��ȡϵͳ����
			GetConnectProxy(hWnd, (LPWSTR)lanName);
			// �ָ��ַ����������Items
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

			if (j == 0 && wcscmp((const wchar_t*)ProxyText, (const wchar_t*)TEXT("")) != 0) { //�����ô����������Ѿ����õ��б�
				// ��������˵���ѡ��
				SendMessageW(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)ProxyText);
				SendMessageW(hWndComboBox, CB_SETCURSEL, i, (LPARAM)0);
			}
			else {
				// ����ѡ��
				SendMessageW(hWndComboBox, CB_SETCURSEL, j, (LPARAM)0);
			}
			//���±���
			updateProxyText();
			
			// ����FORMVIEW
			hfDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)DlgProc);
			// �Զ���������
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
			// ��ʾdialog
			ShowWindow(hfDlg, SW_SHOW);
			// ��ʾ����
			BuildTrayIcon(hWnd);
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
			case IDC_PROXY_OK://���ô���
				{
					if (wcscmp((const wchar_t*)ProxyText, (const wchar_t*)CloseProxy) == 0) {
						if (DisableConnectionProxy(hWnd, (LPWSTR)lanName)) {
							MessageBox(hWnd, TEXT("�ѳɹ�ȡ������"), TEXT("�ɹ�"), MB_OK);
						}
						else {
							ErrorMessage(TEXT("ȡ������ʧ�ܣ��볢���Ҽ�->�Թ���Ա�������"));
						}
					}
					else {
						if (SetConnectionOptions(hWnd, (LPWSTR)lanName, (LPWSTR)ProxyText)) {
							MessageBox(hWnd, (LPCWSTR)ProxyText, TEXT("������������"), MB_OK);
						}
						else {
							ErrorMessage(TEXT("��������ʧ�ܣ��볢���Ҽ�->�Թ���Ա�������"));
						}
					}
				}
				break;
			case IDM_START:
				{
					BOOL AutoStart = false;
					AutoStart = SetAutoRun(hWnd, TEXT(""));
					if (AutoStart) {
						MessageBox(hWnd, TEXT("����Ϊ�����Զ�����"), TEXT("�ɹ�"), MB_OK);
					}
					else {
						ErrorMessage(TEXT("���ÿ���������ʧ�ܣ��볢���Ҽ�->�Թ���Ա�������"));
					}
				}
				break;
			case IDM_START_MINI:
				{
					BOOL AutoStart = false;
					AutoStart = SetAutoRun(hWnd, TEXT(" -mini"));
					if (AutoStart) {
						MessageBox(hWnd, TEXT("����Ϊ�����Զ����в���С������"), TEXT("�ɹ�"), MB_OK);
					}
					else {
						ErrorMessage(TEXT("���ÿ���������ʧ�ܣ��볢���Ҽ�->�Թ���Ա�������"));
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
			case ID_AUTO1:
				{
					TCHAR inBuf[MAX_LOADSTRING];
					GetPrivateProfileString(TEXT("ProxyUI"), TEXT("auto1"), TEXT(""), inBuf, MAX_LOADSTRING, iniFile);
					if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("open")) == 0) {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("auto1"), TEXT(""), iniFile);
						MessageBox(hWnd, TEXT("�ѹر��Զ����д������1"), TEXT("�ɹ�"), MB_OK);
					}
					else {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("auto1"), TEXT("open"), iniFile);
						MessageBox(hWnd, TEXT("����������Զ����д������1"), TEXT("�ɹ�"), MB_OK);
					}
				}
				break;
			case ID_AUTO2:
				{
					TCHAR inBuf[MAX_LOADSTRING];
					GetPrivateProfileString(TEXT("ProxyUI"), TEXT("auto2"), TEXT(""), inBuf, MAX_LOADSTRING, iniFile);
					if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("open")) == 0) {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("auto2"), TEXT(""), iniFile);
						MessageBox(hWnd, TEXT("�ѹر��Զ����д������2"), TEXT("�ɹ�"), MB_OK);
					}
					else {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("auto2"), TEXT("open"), iniFile);
						MessageBox(hWnd, TEXT("����������Զ����д������2"), TEXT("�ɹ�"), MB_OK);
					}
				}
				break;
			case IDM_START_MOD:
				{
					TCHAR inBuf[MAX_LOADSTRING];
					GetPrivateProfileString(TEXT("ProxyUI"), TEXT("start"), TEXT("one"), inBuf, MAX_LOADSTRING, iniFile);
					if (wcscmp((const wchar_t*)inBuf, (const wchar_t*)TEXT("one")) == 0) {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("start"), TEXT("multi"), iniFile);
						MessageBox(hWnd, TEXT("���л����࿪ģʽ����ǰ�������ͬʱ�򿪶��"), TEXT("�ɹ�"), MB_OK);
					}
					else {
						WritePrivateProfileString(TEXT("ProxyUI"), TEXT("start"), TEXT("one"), iniFile);
						MessageBox(hWnd, TEXT("���л�������ģʽ����ǰ����ֻ�ܴ�һ��"), TEXT("�ɹ�"), MB_OK);
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
				// �������Ƿ���
				if (pro_info.hProcess > 0 || pro_info2.hProcess > 0) {
					MessageBox(hWnd, TEXT("��ֹͣ�����ĳ������˳�"), TEXT("ʧ��"), MB_OK);
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
            // TODO: �ڴ˴����ʹ�� hdc ���κλ�ͼ����...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		DestroyTrayIcon(hWnd);
        PostQuitMessage(0);
        break;
	case WM_CLOSE://�رմ���ʱ��С��������
		ModifyTrayIcon(hWnd);
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_CLICKBIT://�������ͼ��
	{
		switch (lParam)
		{
		case WM_LBUTTONUP://����ͼ�껹ԭ����
			ShowWindow(hWnd, SW_SHOWNORMAL);
			break;
		case WM_RBUTTONDOWN:
			POINT pt;
			int menu_rtn;//���ڽ��ղ˵�ѡ���ֵ
			GetCursorPos(&pt);//ȡ�������
			::SetForegroundWindow(hWnd);
			HMENU hMenu;
			hMenu = CreatePopupMenu();

			TCHAR inBuf1[MAX_PATH];
			GetPrivateProfileString(TEXT("Program"), TEXT("switch"), TEXT(""), inBuf1, MAX_PATH, iniFile);
			if (wcscmp((const wchar_t*)inBuf1, (const wchar_t*)TEXT("")) != 0) {//�ж�������л�
				// ��ǰ����
				LRESULT idx_row;
				HWND hComboBox = GetDlgItem(hfDlg, IDC_SWITCH);
				idx_row = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
				AppendMenu(hMenu, MFS_DISABLED, 0, TEXT("�����л�����"));
				// �����˵�
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
				// ѡ�в˵�
				if (menu_rtn >= 1 && menu_rtn <= i) {
					// ��������ֵ
					SendMessageW(hComboBox, CB_SETCURSEL, menu_rtn - 1, (LPARAM)0);
					// ��ֹ��Ϣ���첽�Ļ�û�и��£�����PostMessageģ����Ϣ������ֱ��ȡֵ����ֵ
					WCHAR selectText[MAX_LOADSTRING] = { 0 };
					SendMessage(hComboBox, CB_GETLBTEXT, menu_rtn - 1, (LPARAM)selectText);

					// ��ȡ���õĲ���
					WCHAR params[MAX_PATH] = { 0 };
					GetPrivateProfileString(TEXT("Env"), selectText, TEXT(""), params, MAX_PATH, iniFile);
					// ���õ����������
					HWND hEdit4 = GetDlgItem(hfDlg, IDC_EDIT4);
					SendMessage(hEdit4, WM_SETTEXT, NULL, (LPARAM)params);
					
					// ����Ӧ��
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
		//wcslen ���ص����ַ����е��ַ���, �� UNICODE �����У�һ���ַ�ռ2���ֽ�
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

// ����ͼ��
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
	notifyIconData.uCallbackMessage = WM_CLICKBIT; //�Զ�����Ϣ
	lstrcpy(notifyIconData.szTip, TEXT("ProxyUI"));
	notifyIconData.dwState =  NIS_SHAREDICON;//�Ƿ���ʾicon

	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

//�޸�ϵͳ����ͼ�� 
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
	notifyIconData.uCallbackMessage = WM_CLICKBIT; //�Զ�����Ϣ
	lstrcpy(notifyIconData.szTip, TEXT("ProxyUI"));
	notifyIconData.dwState = NIS_SHAREDICON;//�Ƿ���ʾicon

	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	notifyIconData.dwInfoFlags = NIIF_NONE | NIIF_NOSOUND;//����������
	notifyIconData.uTimeout = 1000/*��ʾ��ʱ����,��Windows 2000 �� Windows XP��Ч*/;
	lstrcpy(notifyIconData.szInfoTitle, TEXT("������С��;-)��"));
	lstrcpy(notifyIconData.szInfo, TEXT("�����˳�������\"����->�˳�\"�� �ٺ�~~~"));

	Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
	firstTray = FALSE;
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

// ���ô��� https://docs.microsoft.com/en-us/windows/win32/wininet/setting-and-retrieving-internet-options
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

// ��ѯ����
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
		//MessageBox(hWnd, (LPCWSTR)Option[0].Value.pszValue, TEXT("�ɹ�"), MB_OK);
	}

	// Free the allocated memory.
	delete[] list.pOptions;
	return ret;
}


// ����ȫ�ֱ���
void updateProxyText()
{
	LRESULT idx_row;
	idx_row = SendMessage(hWndComboBox, CB_GETCURSEL, 0, 0);
	SendMessage(hWndComboBox, CB_GETLBTEXT, idx_row, (LPARAM)ProxyText);
}

// ѡ���ļ�
void selectApplication(HWND hWnd, int nIDDlgItem)
{
	OPENFILENAME opfn;
	WCHAR strFilename[MAX_PATH];//����ļ���
	//��ʼ��
	ZeroMemory(&opfn, sizeof(OPENFILENAME));
	opfn.lStructSize = sizeof(OPENFILENAME);//�ṹ���С
	//���ù���
	opfn.lpstrFilter = L"�����ļ�\0*.*\0��ִ���ļ�\0*.exe\0";
	//Ĭ�Ϲ�����������Ϊ1
	opfn.nFilterIndex = 1;
	//�ļ������ֶα����Ȱѵ�һ���ַ���Ϊ \0
	opfn.lpstrFile = strFilename;
	opfn.lpstrFile[0] = '\0';
	opfn.nMaxFile = sizeof(strFilename);
	//���ñ�־λ�����Ŀ¼���ļ��Ƿ����
	opfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	//opfn.lpstrInitialDir = NULL;
	// ��ʾ�Ի������û�ѡ���ļ�
	if (GetOpenFileName(&opfn))
	{
		//���ı�������ʾ�ļ�·��
		HWND hEdt = GetDlgItem(hWnd, nIDDlgItem);
		SendMessage(hEdt, WM_SETTEXT, NULL, (LPARAM)strFilename);
	}
}

// ����Ӧ��
BOOL startApp(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* ProxyExe1, BOOL show)
{
	// �������Ƿ�������ͣ�ٿ�
	if ((*process).hProcess > 0) {
		stopApp(hWnd, process);
		Sleep(500);
		if ((*process).hProcess > 0) {//ֹͣʧ��
			return FALSE;
		}
	}

	STARTUPINFO sti; //������Ϣ   
	ZeroMemory(process, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&sti, sizeof(STARTUPINFO));
	sti.cb = sizeof(sti);
	if (!show) {
		sti.dwFlags = STARTF_USESHOWWINDOW;
		sti.wShowWindow = SW_HIDE;
	}
	//dirPathָ���½��̵Ĺ���·���������������������·����C:\Windows\SysWOW64���ӽ����������·��ʱ�Ҳ��������ļ���bug
	BOOL bRet = FALSE;
	bRet = CreateProcess(NULL, ProxyExe1, NULL, NULL, FALSE, 0, NULL, dirPath, &sti, process);
	if (!bRet)
	{
		MessageBox(hWnd, TEXT("����ʧ��"), TEXT("ʧ��"), MB_OK);
		return FALSE;
	}
	return TRUE;
}

// ����CLOSE��Ϣ
BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD dwID;
	GetWindowThreadProcessId(hwnd, &dwID);
	if (dwID == (DWORD)lParam) {
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}
	return TRUE;
}

// ֹͣӦ��
void stopApp(HWND hWnd, PROCESS_INFORMATION* process)
{
	// �������Ƿ���
	if ((*process).hProcess == 0) {
		//MessageBox(hWnd, TEXT("û�н���"), TEXT("ʧ��"), MB_OK);
		return;
	}

	HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, (*process).dwProcessId);
	if (hProc == NULL) {
		return;
	}

	//bat�ļ��������ֿ��Թر�, bat�Ļ�Ҫд��ǰ��
	// ����CLOSE��Ϣ�رգ����bat��Ч
	EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)(*process).dwProcessId);

	/*
	// ��һ�ַ���CLOSE����
	std::wostringstream oss;
	oss.str(_T(""));
	oss << _T("/PID ");
	oss << pro_info.dwProcessId;
	std::wstring strCmd = oss.str();
	ShellExecute(NULL, _T("OPEN"), _T("taskkill.exe"), strCmd.c_str(), _T(""), SW_HIDE);
	*/

	//����ͣ��bat��ز���
	///Sleep(1000);
	// �ȴ�Milliseconds���������ɱ���̣���exe��Ч��
	if (WaitForSingleObject(hProc, 1000) != WAIT_OBJECT_0) {
		//exe�ļ��������ֿ��Թر�
		DWORD dwExitCode = 0;
		// ��ȡ�ӽ��̵��˳��� 
		GetExitCodeProcess((*process).hProcess, &dwExitCode);
		TerminateProcess((*process).hProcess, dwExitCode);//��ֹ����
	}

	// �ر��ӽ��̵����߳̾�� 
	CloseHandle((*process).hThread);
	// �ر��ӽ��̾�� 
	CloseHandle((*process).hProcess);
	(*process).hProcess = 0;
	CloseHandle(hProc);
}


// ��������2
void clickStartApp2(HWND hdlg)
{
	WCHAR ProxyExe2[MAX_PATH] = { 0 };
	GetDlgItemText(hdlg, IDC_PROXY_CMD2, (LPTSTR)ProxyExe2, MAX_PATH);
	if (wcscmp((const wchar_t*)ProxyExe2, (const wchar_t*)TEXT("")) == 0) {
		MessageBox(hdlg, TEXT("��ѡ�����"), TEXT("ʧ��"), MB_OK);
		return;
	}
	WritePrivateProfileString(TEXT("Program"), TEXT("app2"), ProxyExe2, iniFile);

	WCHAR Params[MAX_PATH] = { 0 };
	GetDlgItemText(hdlg, IDC_EDIT4, (LPTSTR)Params, MAX_PATH);
	WritePrivateProfileString(TEXT("Program"), TEXT("param2"), Params, iniFile);

	// ����ѡ���¼����
	LRESULT idx_row;
	WCHAR selectText[255] = { 0 };
	HWND hComboBox = GetDlgItem(hdlg, IDC_SWITCH);
	idx_row = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
	SendMessage(hComboBox, CB_GETLBTEXT, idx_row, (LPARAM)selectText);
	WritePrivateProfileString(TEXT("Program"), TEXT("selected"), selectText, iniFile);

	// ƴ������
	lstrcat(ProxyExe2, TEXT(" "));
	lstrcat(ProxyExe2, Params);
	//MessageBox(hdlg, ProxyExe2, TEXT("ʧ��"), MB_OK);

	// �Ƿ�ѡ�к�̨
	UINT sta = IsDlgButtonChecked(hdlg, IDC_CHECK2);
	BOOL ret = startApp(hdlg, &pro_info2, ProxyExe2, sta == BST_UNCHECKED);
	if (ret) {
		HWND hStatus = GetDlgItem(hdlg, IDC_STATIC2);
		SendMessage(hStatus, WM_SETTEXT, NULL, (LPARAM)L"������");
		HWND hBtn = GetDlgItem(hdlg, IDC_PROXY_START2);
		SendMessage(hBtn, WM_SETTEXT, NULL, (LPARAM)L"����");
	}
}

// ��ʾ����
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
		TEXT("%s \n������ %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("����"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	//ExitProcess(dw);
}
