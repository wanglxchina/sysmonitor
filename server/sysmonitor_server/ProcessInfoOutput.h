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
	int   fCpu;				//cpuռ����
	int   nPhyMemory;	//�����ڴ�MB
	int   nVirMemory;    //�����ڴ�MB
	int   nPid;				   //process id
	int   nTid;				   //thread id
	int   nThreadsCount;
	int   nHandleCount;
	int   nNetUpKb;
	int   nNetDownkb;
	int   nIoWrite;
	int   nIoRead;
	std::string timeStr;
	__int64 nLastTime;		//�ϴε�CPUʱ��
	__int64 nRefreshTime;	//�ϴμ�¼ʱ�䣬���ڱ�Ǹý����Ƿ��Ѿ�����
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

