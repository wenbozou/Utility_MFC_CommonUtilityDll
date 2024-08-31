﻿
// CommonUtilitySDKDemoDlg.h: 头文件
//

#pragma once
#include "CurlDownloader.h"

// CCommonUtilitySDKDemoDlg 对话框
class CCommonUtilitySDKDemoDlg : public CDialogEx
{
// 构造
public:
	CCommonUtilitySDKDemoDlg(CWnd* pParent = nullptr);	// 标准构造函数
	virtual ~CCommonUtilitySDKDemoDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_COMMONUTILITYSDKDEMO_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
	void DownloadStart();

private:
	CCurlDownloader *m_pDownloader;
	unsigned int m_dwThreadID;	// 接收线程的ID
	HANDLE m_hThread;				// 线程句柄
};
