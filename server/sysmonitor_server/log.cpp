#include "stdafx.h"
#include "log.h"
#include <Windows.h>
#include <process.h>
#include <shlwapi.h>
#include <assert.h>
#include <io.h>
#include <direct.h>
#include <atlconv.h>
#pragma warning(disable:4996)
char* CLog::LOG_LEVEL_CONTENT[] ={"[ SYS ]","[ERROR]","[ WARN]","[ INFO]","[DEBUG]"}; 
const int LOG_STD_BUFFER_SIZE = 5 * 1024 * 1024;
const int LOG_FILE_BUFFER_SIZE = 5 * 1024 * 1024;
const int MAX_LOG_FILE_SIZE = 50 * 1024 * 1024;
const int MAX_FILE_LOG_COUNT = 10;
int LOG_OUT_TARGET = LOG_ALL_OUT;
CLog::CLog(void):
m_nOutTarget(LOG_ALL_OUT)
{
	m_fp = NULL;
	m_nLevel = 4;
	m_hStdThread = NULL;
	m_hFileThread = NULL;
	m_nStdBufferWritePos = 0;
	m_nFileBufferWritePos = 0;
	memset(m_szFilePath,0,MAX_PATH_LENGTH);
	m_pStdBuffer = NULL;
	m_pFileBuffer = NULL;	
	m_bStopLog = false;
	m_bFileFirstLine = true;
	m_nFileIndex = 0;
}

CLog::~CLog(void)
{
	
}
CLog& CLog::GetIntance()
{
	static CLog instance;
	return instance;
}
void CLog::Init(char* sPath)
{
	CriticalSection::LOCK lock(&m_cs);
	m_bStopLog = false;
	m_pStdBuffer = (char*)malloc(LOG_STD_BUFFER_SIZE);
	m_pFileBuffer = (char*)malloc(LOG_FILE_BUFFER_SIZE);
	assert(m_pStdBuffer);
	assert(m_pFileBuffer);	
	strcat(sPath,"\\log\\");
	memcpy(m_szFilePath,sPath,MAX_PATH_LENGTH);
	m_fp = GetFilePointer(sPath);
	if (m_fp != NULL)
	{
		m_hFileThread = (HANDLE)_beginthreadex(NULL,0,ThreadFileFunc,this,0,NULL);
	}
	m_hStdThread = (HANDLE)_beginthreadex(NULL,0,ThreadStdFunc,this,0,NULL);
	
}

FILE* CLog::GetFilePointer(char* path)
{
	char szFileName[MAX_PATH_LENGTH] = {0};
	char szTmpPath[MAX_PATH_LENGTH] = {0};
	if (NULL!=path && strlen(path)!=0)
	{
		CreateDirectoryA(path,NULL);
		sprintf(szTmpPath,"%s\\",path);
	}
	while( m_nFileIndex < MAX_FILE_LOG_COUNT )
	{
		sprintf_s(szFileName,MAX_PATH_LENGTH,"%slog_%04d.txt",szTmpPath,m_nFileIndex);
		if ( access(szFileName,0) < 0 || GetFileSizeEx(szFileName) < MAX_LOG_FILE_SIZE )
		{
			break;
		}
		++m_nFileIndex;
	}
	if ( m_nFileIndex >= MAX_FILE_LOG_COUNT )
	{
		m_nFileIndex = 0;
		sprintf_s(szFileName,MAX_PATH_LENGTH,"%sgui_log_%04d.txt",szTmpPath,m_nFileIndex);
		remove(szFileName);
	}
	
	FILE* fp = NULL;
	fp = fopen(szFileName,"a+");
	if ( fp == NULL )
	{
		int nLastErr = GetLastError();
		WriteLogEx(LOG_STD_OUT,LOG_LEVEL_ERROR,"Open log file %s Failed! Next will not write log to file!lasterror:%d\n",szFileName,nLastErr);
	}
	return fp;
}

long CLog::GetFileSizeEx( char* path )
{
	int size = 0;
	FILE* fp = NULL;
	fp = fopen(path,"r");
	if ( fp != NULL )
	{
		size = GetFileSize(fp);
		fclose(fp);
	}
	return size;
}

long CLog::GetFileSize( FILE *fp )
{
	long int save_pos;
	long size_of_file;

	/* Save the current position. */
	save_pos = ftell( fp );

	/* Jump to the end of the file. */
	fseek( fp, 0L, SEEK_END );

	/* Get the end position. */
	size_of_file = ftell( fp );

	/* Jump back to the original position. */
	fseek( fp, save_pos, SEEK_SET );

	return( size_of_file );
}

void CLog::CheckFile()
{
	if ( GetFileSize(m_fp) > MAX_LOG_FILE_SIZE )
	{
		fclose(m_fp);
		m_fp = NULL;
		++m_nFileIndex;
		if ( m_nFileIndex >= MAX_FILE_LOG_COUNT )
		{
			m_nFileIndex = 0;
		}
		m_fp = GetFilePointer(m_szFilePath);
	}
}

void CLog::WriteLogEx(const int outTarget,const int level,const char* str,...)
{
	char szInfo[MAX_PATH_LENGTH]={0};
	va_list arg_ptr;
	va_start(arg_ptr,str);
	_vsnprintf_s(szInfo,sizeof(szInfo)-1,str,arg_ptr);

	WriteLog(outTarget,level,szInfo);

}
void CLog::WriteLog(const int outTarget,const int level,const char* str)
{
	if (level > m_nLevel)
	{
		return;
	}
	SYSTEMTIME st;
	GetLocalTime(&st);
	char szLog[MAX_PATH_LENGTH]={0};
	sprintf_s(szLog,MAX_PATH_LENGTH,"%s_%d/%02d/%02d_%02d:%02d:%02d:%03d : %s",LOG_LEVEL_CONTENT[level],st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds,str);
	int nLen = strlen(szLog);
	CriticalSection::LOCK lock(&m_cs);
	if ( outTarget&LOG_FILE_OUT && m_pFileBuffer+m_nFileBufferWritePos+nLen < m_pFileBuffer+LOG_FILE_BUFFER_SIZE)
	{
		memcpy(m_pFileBuffer+m_nFileBufferWritePos,szLog,nLen);
		m_nFileBufferWritePos += nLen;
	}
	if (outTarget&LOG_STD_OUT && m_pStdBuffer+m_nStdBufferWritePos+nLen < m_pStdBuffer+LOG_STD_BUFFER_SIZE)
	{
		memcpy(m_pStdBuffer+m_nStdBufferWritePos,szLog,nLen);
		m_nStdBufferWritePos += nLen;
	}
}
unsigned int __stdcall CLog::ThreadStdFunc(void* param)
{
	CLog* pLog = (CLog*)param;
	int nLogSize = 0;
	char* pBuf = (char*)malloc(LOG_STD_BUFFER_SIZE);
	ZeroMemory(pBuf, LOG_STD_BUFFER_SIZE);
	
	while(!pLog->m_bStopLog)
	{
		pLog->m_cs.Lock();
		memcpy(pBuf,pLog->m_pStdBuffer,pLog->m_nStdBufferWritePos);
		nLogSize = pLog->m_nStdBufferWritePos;
		pLog->m_nStdBufferWritePos = 0;
		pLog->m_cs.UnLock();

		if (nLogSize>0)
		{
			int nWrite = fwrite(pBuf, 1, nLogSize, stdout);
		}
		Sleep(1 * 1000);
	}
	free(pBuf);
	return 0;
}
unsigned int __stdcall CLog::ThreadFileFunc(void* param)
{
	CLog* pLog = (CLog*)param;
	int nLogSize = 0;
	char* pBuf = (char*)malloc(LOG_FILE_BUFFER_SIZE);
	ZeroMemory(pBuf, LOG_FILE_BUFFER_SIZE);

	while(!pLog->m_bStopLog)
	{
		ZeroMemory(pBuf, LOG_FILE_BUFFER_SIZE);
		pLog->m_cs.Lock();
		memcpy(pBuf,pLog->m_pFileBuffer,pLog->m_nFileBufferWritePos);
		nLogSize = pLog->m_nFileBufferWritePos;
		pLog->m_nFileBufferWritePos = 0;
		pLog->m_cs.UnLock();
		if (NULL == pLog->m_fp)
		{
			break;
		}
		if (nLogSize>0)
		{
			if ( pLog->m_bFileFirstLine && pLog->GetFileSize(pLog->m_fp) > 0 )
			{
				fwrite("\n\n\n",1,3,pLog->m_fp);
			}
			pLog->m_bFileFirstLine = false;
			fwrite(pBuf, 1, nLogSize, pLog->m_fp);
#ifdef _DEBUG
			OutputDebugStringA(pBuf);
#endif
		}
		fflush(pLog->m_fp);
		pLog->CheckFile();
		Sleep(1 * 1000);
	}
	free(pBuf);
	return 0;
}
void CLog::UnInit()
{
	m_bStopLog = true;
	if (WaitForSingleObject(m_hFileThread,500)!=WAIT_OBJECT_0)
	{
		TerminateThread(m_hFileThread,-1);
	}
	if (WaitForSingleObject(m_hStdThread,500)!=WAIT_OBJECT_0)
	{
		TerminateThread(m_hStdThread,-1);
	}
	CloseHandle(m_hStdThread);
	CloseHandle(m_hFileThread);
	if (NULL != m_fp)
	{
		fclose(m_fp);
		m_fp = NULL;
	}
}
void log_printf(const int level, const wchar_t* str, ...)
{
	USES_CONVERSION;
	wchar_t szInfo[MAX_PATH_LENGTH] = { 0 };
	va_list arg_ptr;
	va_start(arg_ptr, str);
	_vsnwprintf_s(szInfo, sizeof(szInfo) - 1, str, arg_ptr);

	CLog::GetIntance().WriteLog(LOG_OUT_TARGET, level, W2A(szInfo));
}
void log_printf(const int level,const char* str,...)
{
	char szInfo[MAX_PATH_LENGTH]={0};
	va_list arg_ptr;
	va_start(arg_ptr,str);
	_vsnprintf_s(szInfo,sizeof(szInfo)-1,str,arg_ptr);

	CLog::GetIntance().WriteLog(LOG_OUT_TARGET,level,szInfo);
}
void log_init(char* sPath, int target)
{
	char tmp[MAX_PATH_LENGTH] = { 0 };
	memcpy(tmp, sPath, strlen(sPath));
	if (strlen(tmp)<=0)
	{
		_getcwd(tmp, sizeof(tmp));
	}
	LOG_OUT_TARGET = target;
	CLog::GetIntance().Init(tmp);
}