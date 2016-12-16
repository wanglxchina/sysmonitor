/***********************************************************************
FILE	:ProcessInfoOutput.h
DATE	:2016-12-07
AUTHOR	:wanglx
DECRIPE	:write process_info_t to xml file by appname
***********************************************************************/
#pragma once
#include <string>
#include <queue>
#include <map>
#include "pthread.h"
#include "util.h"

struct process_info_t
{
	int   fCpu;				//cpu占用率
	int   nPhyMemory;	//物理内存MB
	int   nVirMemory;    //虚拟内存MB
	int   nPid;				   //process id
	int   nTid;				   //thread id
	int   nThreadsCount;
	int   nHandleCount;
	int   nNetUpKb;
	int   nNetDownkb;
	int   nIoWrite;
	int   nIoRead;
	std::string timeStr;
	__int64 nLastTime;		//上次的CPU时间
	__int64 nRefreshTime;	//上次记录时间，用于标记该进程是否已经不再
};

class CProcessInfoOutput
{
public:
	CProcessInfoOutput(std::string appName,int syncTrigger=60);
	~CProcessInfoOutput();
	bool Open(const std::string path="");
 	bool Write(const process_info_t& info);
	bool Flush();
	bool Close();
protected:
	void WriteDataToFile();
	static void* WriteDataToFileProc(void* arg);
	std::string m_path;
	std::string m_appName;
	std::queue<process_info_t> m_infoQueue;
	pthread_t m_syncFileThreadId;
	pthread_mutex_t m_processInfoMutex;
	int  m_writeToFileTrigger;
	bool m_syncDataToFileStopFlag;
	bool m_isSyncThreadWork;
	bool m_flushFlag;
	UNITTEST(CProcessInfoOutput);
};

