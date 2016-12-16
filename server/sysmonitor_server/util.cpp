#include "stdafx.h"
#include "util.h"
#include <atlconv.h>
std::string GetCurrentTime_hhmmssmmm()
{
	std::string timeStr;
	SYSTEMTIME st;
	GetLocalTime(&st);
	char tmp[50] = { 0 };
	sprintf_s(tmp, sizeof(tmp),"%d-%02d-%02d %02d:%02d:%02d:%003d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	timeStr = tmp;
	return timeStr;
}

std::string GetCurrentTime_hhmmss()
{
	std::string timeStr;
	SYSTEMTIME st;
	GetLocalTime(&st);
	char tmp[50] = { 0 };
	sprintf_s(tmp, sizeof(tmp), "%d-%02d-%02d %02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	timeStr = tmp;
	return timeStr;
}

std::string GetCurrentTime_hhmmss_sp()
{
	std::string timeStr;
	SYSTEMTIME st;
	GetLocalTime(&st);
	char tmp[50] = { 0 };
	sprintf_s(tmp, sizeof(tmp), "%d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	timeStr = tmp;
	return timeStr;
}

std::string MyW2A(std::wstring str)
{
	USES_CONVERSION;
	std::string ret = W2A(str.c_str());
	return ret;
}
std::wstring MyA2W(std::string str)
{
	USES_CONVERSION;
	std::wstring ret = A2W(str.c_str());
	return ret;
}