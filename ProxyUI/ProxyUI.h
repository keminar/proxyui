#pragma once

#include "resource.h"

// �˴���ģ���а����ĺ�����ǰ������: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//FORMVIEW �ص���Ϣ
LRESULT CALLBACK DlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
//�����Զ����� 
BOOL SetAutoRun(HWND hwnd, LPWSTR params);
//�رտ����Զ����� 
BOOL SetNoAutoRun(HWND hwnd);
//����ͼ�� 
void BuildTrayIcon(HWND hwnd);
//�޸�����ͼ�� 
void ModifyTrayIcon(HWND hwnd);

// ���ô���
BOOL SetConnectionOptions(HWND hWnd, LPWSTR conn_name, LPWSTR proxy_full_addr);
// ȡ������
BOOL DisableConnectionProxy(HWND hWnd, LPWSTR conn_name);
// ��ѯ����
BOOL GetConnectProxy(HWND hWnd, LPWSTR conn_name);
// ��ʼ����ֵ
void initFormData(HWND hdlg);
// ѡ���ļ�
void selectApplication(HWND hWnd, int nIDDlgItem);
// ����Ӧ��
BOOL startApp(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* ProxyExe1, BOOL show);
// ֹͣӦ��
void stopApp(HWND hWnd, PROCESS_INFORMATION* process);
// ����CLOSE��Ϣ
BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam);
// ����ȫ�ֱ���
void updateProxyText();
// ģ������Ӧ�ô���2
void clickStartApp2(HWND hdlg);