#pragma once
#include "pthread.h"
#include<string>
#include <map>
#include <sstream>
#include "stdafx.h"
#ifdef USE_UNIT_TEST
#define UNITTEST(x) friend class x##_Test
#else
#define UNITTEST(x)
#endif
#if defined(WIN32)
#include <Windows.h>
#define time_interval_ms(begin,end,base) (int)((end.QuadPart-begin.QuadPart)* 1000 / base.QuadPart)
#else
#define Sleep(x) usleep(x*1000)
typedef struct timeval LARGE_INTEGER;
#define QueryPerformanceCounter(st) gettimeofday(st,NULL)
#define QueryPerformanceFrequency(base) gettimeofday(base,NULL)
#define time_interval_ms(stbegin,stend,stbase) (int)((stend.tv_sec*1000000+stend.tv_usec - stbegin.tv_sec*1000000-stbegin.tv_usec)/1000)
#endif

//yyyy-mm-dd hh:mm:ss:mmm
std::string GetCurrentTime_hhmmssmmm();
//yyyy-mm-dd hh:mm:ss
std::string GetCurrentTime_hhmmss_sp();
//yyyy-mm-dd hhmmss
std::string GetCurrentTime_hhmmss();

template<typename T>
std::string ConvertToString(T _value)
{
	std::stringstream ss;
	ss << _value;
	return ss.str();
}
template<typename T>
utility::string_t WConvertToString(T _value)
{
	utility::ostringstream_t ss;
	ss << _value;
	return ss.str();
}
#define MYW2A(x) MyW2A(x)
#define MYA2W(x) MyA2W(x)
std::string MyW2A(std::wstring str);
std::wstring MyA2W(std::string str);
//////////////////////////////////////////////////////////////////////////
typedef void (*ThreadCallbackFunction) (void *);
class ThreadBase
{
public:
	ThreadBase():
	m_threadCount(0)
	{
		pthread_mutex_init(&m_thread_mutex,NULL);
	}
	~ThreadBase()
	{
		pthread_mutex_destroy(&m_thread_mutex);
	}
	int pthread_create(pthread_t * tid, const pthread_attr_t * attr, ThreadCallbackFunction start, void *arg)
	{
		int ret = -1;
		pthread_mutex_lock(&m_thread_mutex);
		switch (m_threadCount)
		{
			case 0:
			{
				m_fn[0] = start;
				ret = ::pthread_create(tid, attr, BaseThreadProc, arg);
			}
			break;
			case 1:
			{
				m_fn[1] = start;
				ret = ::pthread_create(tid, attr, BaseThreadProc_1, arg);
			}
			break;
			case 2:
			{
				m_fn[2] = start;
				ret = ::pthread_create(tid, attr, BaseThreadProc_2, arg);
			}
			break;
		}
		pthread_mutex_unlock(&m_thread_mutex);
		if (start != NULL && tid != NULL)
		{
			ret = -1;
		}
		if (ret ==0)
		{
			++m_threadCount;
		}
		return ret;
	}
protected:
	
private:
	static void* BaseThreadProc(void* arg)
	{
		m_fn[0](arg);
		return NULL;
	}
	static void* BaseThreadProc_1(void* arg)
	{
		m_fn[0](arg);
		return NULL;
	}
	static void* BaseThreadProc_2(void* arg)
	{
		m_fn[0](arg);
		return NULL;
	}
	static ThreadCallbackFunction m_fn[3];
	int m_threadCount;
	pthread_mutex_t m_thread_mutex;
};