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
void BuildTrayIcon(HWND hwnd, DWORD act);
//修改托盘图标 
void ModifyTrayIcon(HWND hwnd);
//销毁系统托盘图标 
void DestroyTrayIcon(HWND hwnd);
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
BOOL startApp(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* ProxyExe1, BOOL show, BOOL uac);
// 停止应用
void stopApp(HWND hWnd, PROCESS_INFORMATION* process);
// 计划任务提权：首次UAC授权，后续静默启动
BOOL startAppElevated(HWND hWnd, PROCESS_INFORMATION* process, WCHAR* cmdLine, BOOL show, int appId);
void stopAppElevated(PROCESS_INFORMATION* process, int appId);
BOOL ScheduledTaskExists(LPCWSTR taskName);
// 发送CLOSE消息
BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam);
// 更新全局变量
void updateProxyText();
// 读取最新系统代理并同步到下拉框
void refreshSystemProxy(HWND hMainWnd);
// 创建指定磅值的宋体字体
HFONT MakeSongtiFont(HWND hRefWnd, int pt);
// 把96DPI下的设计像素值换算成当前DPI的像素值
int ScaleForDpi(HWND hRefWnd, int px96);
// 按对话框实际大小调整主窗口尺寸
void FitMainWindow(HWND hWnd);
// 模拟启动应用代理2
void clickStartApp2(HWND hdlg);
// 异步启动/停止任务(工作线程执行,避免UI卡顿)
BOOL LaunchProxyJob(HWND hdlg, int appId, int action, PROCESS_INFORMATION* process, const WCHAR* cmdLine, BOOL show, BOOL uac);
// 设置忙碌状态UI(状态文字/禁用按钮)
void SetJobBusyUI(HWND hdlg, int appId, BOOL starting);
void ErrorMessage(LPTSTR lpszFunction);