#pragma once
#include "util.h"
#define DEFAULT_SERVER_PORT 38889
class CConfiguration
{
public:
	~CConfiguration();
	static CConfiguration& GetInstance();
	bool AddDefaultProcess(std::string appName,bool storage=true);
	bool DelDefaultProcess(std::string appName);
	void GetDefaultProcessList(std::map<std::string, bool>& appMap);

	void SetServerPort(int port);
	int GetServerPort();

	void SetStoragePath(std::string path);
	std::string GetStoragePath();

protected:
	CConfiguration();
	CConfiguration& operator=(const CConfiguration& obj);
	void SyncDataToFile();
	static void* ThreadProc(void* arg);
	bool read_config();
	std::string					m_path;
	pthread_t					m_syncThreadId;
	pthread_mutex_t				m_mutex;
	HANDLE						m_event;
	std::map<std::string, bool> m_appMap;
	int							m_serverPort;
	std::string					m_storagePath;
	bool						m_stopThread;
	bool						m_threadWork;
};

