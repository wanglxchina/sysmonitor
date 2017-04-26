// rest_server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ProcessInfo.h"
#include "log.h"
#include "Configuration.h"
#include <direct.h>
#include <iostream>

int main()
{
	//1.初始化log系统
	char curPath[MAX_PATH_LENGTH] = { 0 };
	if (!_getcwd(curPath,sizeof(curPath)))
	{
		std::cout << "get current exe path failed,lasterror:" << GetLastError() << std::endl;
	}
	log_init(curPath);

	//2.从本地加载监视列表并设置监视
	std::map<std::string, bool> appMap;
	CConfiguration::GetInstance().GetDefaultProcessList(appMap);
	for (auto iter:appMap)
	{
		CProcessInfo::GetInstance().AddMonitorProcess(iter.first, iter.second);
	}

	//3.开启rest接口，允许外部控制
	int serverPort = CConfiguration::GetInstance().GetServerPort();
	utility::string_t addr(U("http://*:"));
	addr = addr + WConvertToString(serverPort);
	on_init_rest(addr);

	//4.开启资源监测线程
	if (!CProcessInfo::GetInstance().StartMonitor())
	{
		log_printf(LOG_LEVEL_ERROR, "process stat start monitor failed!\n");
		return 0;
	}

	//5.hold住主线程
	while (true)
	{
		Sleep(1000);
	}
	//6.退出释放资源
	CProcessInfo::GetInstance().StopMonitor();
	on_shutdown_rest();
    return 0;
}

