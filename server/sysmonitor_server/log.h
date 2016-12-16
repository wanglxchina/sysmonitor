//+------------------------------------------------------------------------------------+
//+FileName : Log.h
//+Author :	wanglx
//+Version : 0.0.1
//+Date : 2015-04-28
//+Description : 日志操作封装类，单例模式实现
//+------------------------------------------------------------------------------------+
#pragma once
#include <stdio.h>
#include <Windows.h>
const int MAX_PATH_LENGTH = 255 * 2;
enum loglevel{ sys = 0, error, warning, info,debug};
enum LOG_LEVEL{ LOG_LEVEL_SYS = 0, LOG_LEVEL_ERROR,	LOG_LEVEL_WARNING,	LOG_LEVEL_INFO,	LOG_LEVEL_DEBUG};
enum logtarget { LOG_NONE_OUT = 0x00, LOG_FILE_OUT = 0x01, LOG_STD_OUT = 0x02, LOG_ALL_OUT = 0x01 | 0x02};

void log_printf(const int level, const wchar_t* str, ...);
void log_printf(const int level,const char* str,...);
void log_init(char* sPath,int target=LOG_ALL_OUT);


//////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CriticalSection
{
public:
	CriticalSection(void){ InitializeCriticalSection(&m_cs); };
	~CriticalSection(void){ DeleteCriticalSection(&m_cs); };
	void Lock(){ EnterCriticalSection(&m_cs); };
	void UnLock(){ LeaveCriticalSection(&m_cs); };
	struct LOCK{
		CriticalSection* m_pcs;
		LOCK(CriticalSection* pcs):m_pcs(pcs){ m_pcs->Lock(); };
		~LOCK(){ m_pcs->UnLock(); };
	};
private:
	CriticalSection(CriticalSection&);
	CriticalSection& operator=(const CriticalSection&);
	CRITICAL_SECTION m_cs;
};
class CLog
{
public:
	~CLog(void);
static CLog& GetIntance();
//+--------------------------------------------------+
//+param[in]
//+			sPath:log文件路径，bWriteToFile为true时有效
//+			bWriteToFile:是否写入指定文件
//+description:初始化日志
//+--------------------------------------------------+
void Init(char* sPath);
void UnInit();
//+--------------------------------------------------+
//+param[in]
//+			level:日志等级
//+			char:变长参数
//+description:写日志
//+--------------------------------------------------+
void WriteLogEx(const int outTarget,const int level,const char* str,...);
//+--------------------------------------------------+
//+param[in]
//+			level:日志等级
//+			char:日志说明
//+description:写日志
//+--------------------------------------------------+
void WriteLog(const int outTarget,const int level,const char* str);
//+--------------------------------------------------+
//+description:设置日志等级 
//+--------------------------------------------------+
void SetLogLevel(int nLevel) { m_nLevel = nLevel; };

protected:
	FILE* GetFilePointer(char* path);
	long GetFileSize( FILE *fp );
	long GetFileSizeEx( char* path );
	void CheckFile();
private:
	CLog(void);
	CLog& operator=(const CLog&);
	static unsigned int __stdcall ThreadStdFunc(void* param);
	static unsigned int __stdcall ThreadFileFunc(void* param);


	FILE* m_fp;
	int m_nLevel;
	int	m_nOutTarget;
	int m_nFileIndex;
	bool m_bStopLog;
	bool m_bFileFirstLine;
	char* m_pStdBuffer;
	char* m_pFileBuffer;
	int m_nStdBufferWritePos;
	int m_nFileBufferWritePos;
	HANDLE m_hStdThread;
	HANDLE m_hFileThread;
	char m_szFilePath[MAX_PATH_LENGTH];
	static char* LOG_LEVEL_CONTENT[]; 
	CriticalSection m_cs;
};
