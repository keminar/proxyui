#pragma once

#include "resource.h"

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//FORMVIEW 回调消息
LRESULT CALLBACK DlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
//开机自动运行 
BOOL SetAutoRun(HWND hwnd, LPWSTR params);
//关闭开机自动运行 
BOOL SetNoAutoRun(HWND hwnd);
//托盘图标 
void BuildTrayIcon(HWND hwnd);
//修改托盘图标 
void ModifyTrayIcon(HWND hwnd);

// 设置代理
BOOL SetConnectionOptions(HWND hWnd, LPWSTR conn_name, LPWSTR proxy_full_addr);
// 取消代理
BOOL DisableConnectionProxy(HWND hWnd, LPWSTR conn_name);
// 查询代理
BOOL GetConnectProxy(HWND hWnd, LPWSTR conn_name);
// 初始化表单值
void initFormData(HWND hdlg);
// 选择文件
void selectApplication(HWND hWnd, int nIDDlgItem);
// 启动应用
BOOL startApp(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* ProxyExe1, BOOL show);
// 停止应用
void stopApp(HWND hWnd, PROCESS_INFORMATION* process);
// 发送CLOSE消息
BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam);
// 更新全局变量
void updateProxyText();
// 模拟启动应用代理2
void clickStartApp2(HWND hdlg);