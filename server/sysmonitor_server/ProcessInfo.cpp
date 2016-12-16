#include "stdafx.h"
#include "ProcessInfo.h"
#include <process.h>
#include <iostream>
#include "log.h"
const int PROCESS_INFO_BUFFER_LENGTH = 1024 * 1024;
const int ProcessBasicInformation = 0;
using namespace std;
void FormatAppName(std::string& appName)
{
	if (appName.find(".exe") !=  std::string::npos)
	{
		appName = appName.substr(0, appName.length() - 4);
	}
}
CProcessInfo::CProcessInfo():
m_isMonitorWork(false)
{
	m_nLastTotalCPU = 0;
	m_stopMonitorFlag = false;
	m_pProcessInfoBuf = new CHAR[PROCESS_INFO_BUFFER_LENGTH];
	InitializeCriticalSection(&m_mutex_lock);
}


CProcessInfo::~CProcessInfo()
{
	if (m_isMonitorWork)
	{
		StopMonitor();
	}
	delete[] m_pProcessInfoBuf;
	DeleteCriticalSection(&m_mutex_lock);
}

CProcessInfo& CProcessInfo::GetInstance()
{
	static CProcessInfo instance;
	return instance;
}
//开始监视系统资源
bool CProcessInfo::StartMonitor()
{
	if (m_isMonitorWork)
	{
		return false;
	}
	m_stopMonitorFlag = false;
	m_isMonitorWork = true;
	m_updateProcessInfoThreadId = (HANDLE)_beginthreadex(NULL, 0, UpdateProcessInfoThreadProc, this, 0, NULL);
	if (!m_updateProcessInfoThreadId)
	{
		m_isMonitorWork = false;
		return false;
	}
	return true;
}
//停止监视系统资源
bool CProcessInfo::StopMonitor()
{
	m_stopMonitorFlag = true;
	WaitForSingleObject(m_updateProcessInfoThreadId, 2000);
	return !m_isMonitorWork;
}

bool CProcessInfo::AddMonitorProcess(std::string processName,bool storage)
{
	bool ret = false;
	FormatAppName(processName);
	EnterCriticalSection(&m_mutex_lock);
	if (m_processInfoMap.find(processName) == m_processInfoMap.end())
	{
		process_info_t info;
		memset(&info, 0, sizeof(info));
		m_processInfoMap.insert(std::make_pair(processName, info));
		
		ret = true;
	}
	if (storage && m_processInfoOutputMap.find(processName) == m_processInfoOutputMap.end())
	{
		std::shared_ptr<CProcessInfoOutput> out = std::make_shared<CProcessInfoOutput>(processName);
		if (!out->Open())
		{
			log_printf(LOG_LEVEL_ERROR, "%s:<%s> open output failed!\n", __FUNCTION__, processName);
		}
		else
		{
			m_processInfoOutputMap.insert(std::make_pair(processName, out));
		}
	}
	LeaveCriticalSection(&m_mutex_lock);
	return ret;
}

bool CProcessInfo::DelMonitorProcess(std::string processName)
{
	bool ret = false;
	FormatAppName(processName);
	EnterCriticalSection(&m_mutex_lock);
	auto iter = m_processInfoMap.find(processName);
	if (iter != m_processInfoMap.end())
	{
		m_processInfoMap.erase(iter);
		ret = true;
	}
	auto it = m_processInfoOutputMap.find(processName);
	if (it != m_processInfoOutputMap.end())
	{
		it->second->Flush();
		it->second->Close();
		m_processInfoOutputMap.erase(it);
	}
	LeaveCriticalSection(&m_mutex_lock);
	return ret;
}

void CProcessInfo::RefreashProcessInfo()
{
	m_isMonitorWork = true;
	while (!m_stopMonitorFlag)
	{
		//取得函数地址
		typedef long(__stdcall *SystemFunction)(DWORD, PVOID, ULONG, ULONG*);
		static SystemFunction NtQuerySystemInformation = (SystemFunction)
			GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQuerySystemInformation");

		if (0 == NtQuerySystemInformation)
		{
			break;
		}

		//查询所有process的信息
		long lRet = NtQuerySystemInformation(5,
			m_pProcessInfoBuf,
			PROCESS_INFO_BUFFER_LENGTH - 1,
			0);
		if (lRet != 0)
		{
			break;
		}

		DWORD dwOffset = 0;
		BOOL bLeft = TRUE;
		__int64 totalProcessCPU = 0;
		do {
			PROCESSINFO& pi = *(PROCESSINFO*)(m_pProcessInfoBuf + dwOffset);
			if (0 == pi.dwOffset)
			{
				bLeft = FALSE;
			}
			totalProcessCPU += (__int64)pi.KernelTime.QuadPart + (__int64)pi.UserTime.QuadPart;
			dwOffset += pi.dwOffset;
		} while (bLeft);
		if (m_nLastTotalCPU == totalProcessCPU)
		{
			++totalProcessCPU;
		}
		bLeft = TRUE;
		dwOffset = 0;
		do {
			PROCESSINFO& pi = *(PROCESSINFO*)(m_pProcessInfoBuf + dwOffset);
			if (0 == pi.dwOffset)
			{
				bLeft = FALSE;
			}
			CHAR processName[255] = { 0 };
			WideCharToMultiByte(CP_ACP, 0, pi.ProcessName.Buffer, -1, processName, sizeof(processName), NULL, NULL);
			EnterCriticalSection(&m_mutex_lock);
			std::string appName = processName;
			FormatAppName(appName);
			auto iter = m_processInfoMap.find(appName);
			if (iter != m_processInfoMap.end())
			{
				iter->second.nPid = pi.dwProcessID;
				iter->second.nTid = pi.ThreadSysInfo[0].dwThreadId;
				iter->second.nPhyMemory = int(pi.dwWorkingSet / 1024);
				iter->second.nVirMemory = int(pi.dwPageFileUsage / 1024);
				__int64 nTime = (__int64)pi.UserTime.QuadPart +	(__int64)pi.KernelTime.QuadPart - iter->second.nLastTime;
				iter->second.fCpu = int(100 * nTime / (totalProcessCPU - m_nLastTotalCPU));
				iter->second.nRefreshTime = totalProcessCPU;
				iter->second.nLastTime = (__int64)pi.UserTime.QuadPart + (__int64)pi.KernelTime.QuadPart;
				iter->second.nThreadsCount = pi.dwThreadsCount;
				iter->second.nHandleCount = pi.dwHandleCount;
				iter->second.timeStr = GetCurrentTime_hhmmss_sp();

				auto itout = m_processInfoOutputMap.find(appName);
				if (itout != m_processInfoOutputMap.end())
				{
					itout->second->Write(iter->second);
				}
				log_printf(LOG_LEVEL_INFO, "[%s_pid:%d] [%s_cpu:%d] [%s_mem:%d]kB [%s_vmem:%d]KB [%s_handle:%d] [%s_thread:%d]\n",
					appName.c_str(), iter->second.nPid, appName.c_str(), iter->second.fCpu, appName.c_str(), iter->second.nPhyMemory,
					appName.c_str(), iter->second.nVirMemory, appName.c_str(), iter->second.nHandleCount, appName.c_str(),
					iter->second.nThreadsCount);
			}
			LeaveCriticalSection(&m_mutex_lock);
			dwOffset += pi.dwOffset;
		} while (bLeft);
		m_nLastTotalCPU = totalProcessCPU;
		Sleep(1000);
	}
	m_isMonitorWork = false;
}

unsigned _stdcall CProcessInfo::UpdateProcessInfoThreadProc(void * pParam)
{
	CProcessInfo* s = (CProcessInfo*)pParam;
	s->RefreashProcessInfo();
	return 0;
}
bool CProcessInfo::GetProcessInfo(std::string processName,process_info_t& info)
{
	bool ret = false;
	FormatAppName(processName);
	EnterCriticalSection(&m_mutex_lock);
	auto iter = m_processInfoMap.find(processName);
	if (iter != m_processInfoMap.end())
	{
		info = iter->second;
		ret = true;
	}
	LeaveCriticalSection(&m_mutex_lock);
	return ret;
}

void CProcessInfo::GetPorcessList(std::vector<std::string>& appVec)
{
	EnterCriticalSection(&m_mutex_lock);
	for (auto iter:m_processInfoMap)
	{
		appVec.push_back(iter.first);
	}
	LeaveCriticalSection(&m_mutex_lock);
}