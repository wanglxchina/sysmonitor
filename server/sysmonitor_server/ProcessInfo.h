/***********************************************************************
FILE	:ProcessInfo.h
DATE	:2016-12-07
AUTHOR	:wanglx
DECRIPE	:this file is only for windows,get app resouce info by NtQuerySystemInformation
***********************************************************************/
#pragma once
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
#include <vector>
#include <map>
#include <windows.h>
#include <string>
#include <memory>
#include "ProcessInfoOutput.h"
typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaxLength;
	PWSTR Buffer;
} UNICODE_STRING;

typedef struct _THREAD_INFO
{
	LARGE_INTEGER CreateTime;
	DWORD dwUnknown1;
	DWORD dwStartAddress;
	DWORD StartEIP;
	DWORD dwOwnerPID;
	DWORD dwThreadId;
	DWORD dwCurrentPriority;
	DWORD dwBasePriority;
	DWORD dwContextSwitches;
	DWORD Unknown;
	DWORD WaitReason;
}THREADINFO, *PTHREADINFO;

typedef struct _PROCESS_INFO
{
	DWORD dwOffset;
	DWORD dwThreadsCount;
	DWORD dwUnused1[6];
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ProcessName;
	DWORD dwBasePriority;
	DWORD dwProcessID;
	DWORD dwParentProcessId;
	DWORD dwHandleCount;
	DWORD dwUnused3[2];
	DWORD dwVirtualBytesPeak;
	DWORD dwVirtualBytes;
	ULONG dwPageFaults;
	DWORD dwWorkingSetPeak;
	DWORD dwWorkingSet;
	DWORD dwQuotaPeakPagedPoolUsage;
	DWORD dwQuotaPagedPoolUsage;
	DWORD dwQuotaPeakNonPagedPoolUsage;
	DWORD dwQuotaNonPagedPoolUsage;
	DWORD dwPageFileUsage;
	DWORD dwPageFileUsagePeak;
	DWORD dCommitCharge;
	THREADINFO ThreadSysInfo[1];
} PROCESSINFO, *PPROCESSINFO;



//组件资源占用
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
class CProcessInfo
{
public:
	~CProcessInfo();
	static CProcessInfo& GetInstance();
	//开始监视系统资源
	bool StartMonitor();
	//停止监视系统资源
	bool StopMonitor();
	//添加监视进程
	bool AddMonitorProcess(std::string processName,bool storage=false);
	//删除对指定进程的监视
	bool DelMonitorProcess(std::string processName);
	//获取指定进程的信息
	bool GetProcessInfo(std::string processName, process_info_t& info);
	//获取当前监视进程列表
	void GetPorcessList(std::vector<std::string>& appVec);
protected:
	CProcessInfo();
	CProcessInfo& operator=(const CProcessInfo& obj);
	void RefreashProcessInfo();
	static unsigned _stdcall UpdateProcessInfoThreadProc(void * pParam);
	HANDLE m_updateProcessInfoThreadId;
	bool m_stopMonitorFlag;
	bool m_isMonitorWork;
	LPSTR m_pProcessInfoBuf;
	std::map<std::string, process_info_t> m_processInfoMap;
	std::map<std::string, std::shared_ptr<CProcessInfoOutput>> m_processInfoOutputMap;
	CRITICAL_SECTION m_mutex_lock;
	__int64 m_nLastTotalCPU;
};