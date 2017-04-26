// rest_server.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "ProcessInfo.h"
#include "log.h"
#include "Configuration.h"
#include <direct.h>
#include <iostream>

int main()
{
	//1.��ʼ��logϵͳ
	char curPath[MAX_PATH_LENGTH] = { 0 };
	if (!_getcwd(curPath,sizeof(curPath)))
	{
		std::cout << "get current exe path failed,lasterror:" << GetLastError() << std::endl;
	}
	log_init(curPath);

	//2.�ӱ��ؼ��ؼ����б����ü���
	std::map<std::string, bool> appMap;
	CConfiguration::GetInstance().GetDefaultProcessList(appMap);
	for (auto iter:appMap)
	{
		CProcessInfo::GetInstance().AddMonitorProcess(iter.first, iter.second);
	}

	//3.����rest�ӿڣ������ⲿ����
	int serverPort = CConfiguration::GetInstance().GetServerPort();
	utility::string_t addr(U("http://*:"));
	addr = addr + WConvertToString(serverPort);
	on_init_rest(addr);

	//4.������Դ����߳�
	if (!CProcessInfo::GetInstance().StartMonitor())
	{
		log_printf(LOG_LEVEL_ERROR, "process stat start monitor failed!\n");
		return 0;
	}

	//5.holdס���߳�
	while (true)
	{
		Sleep(1000);
	}
	//6.�˳��ͷ���Դ
	CProcessInfo::GetInstance().StopMonitor();
	on_shutdown_rest();
    return 0;
}

