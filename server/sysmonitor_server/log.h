//+------------------------------------------------------------------------------------+
//+FileName : Log.h
//+Author :	wanglx
//+Version : 0.0.1
//+Date : 2015-04-28
//+Description : ��־������װ�࣬����ģʽʵ��
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
//+			sPath:log�ļ�·����bWriteToFileΪtrueʱ��Ч
//+			bWriteToFile:�Ƿ�д��ָ���ļ�
//+description:��ʼ����־
//+--------------------------------------------------+
void Init(char* sPath);
void UnInit();
//+--------------------------------------------------+
//+param[in]
//+			level:��־�ȼ�
//+			char:�䳤����
//+description:д��־
//+--------------------------------------------------+
void WriteLogEx(const int outTarget,const int level,const char* str,...);
//+--------------------------------------------------+
//+param[in]
//+			level:��־�ȼ�
//+			char:��־˵��
//+description:д��־
//+--------------------------------------------------+
void WriteLog(const int outTarget,const int level,const char* str);
//+--------------------------------------------------+
//+description:������־�ȼ� 
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
