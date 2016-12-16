#include "stdafx.h"
#include "Configuration.h"
#include "pugixml/pugixml.hpp"
#include "log.h"
#include <direct.h>
CConfiguration::CConfiguration():
	m_serverPort(DEFAULT_SERVER_PORT),
	m_storagePath(""),
	m_stopThread(false),
	m_threadWork(false)
{
	char tmp[255] = { 0 };
	_getcwd(tmp, sizeof(tmp));
	m_path = m_path + tmp + "\\configuration.xml";
	pthread_mutex_init(&m_mutex, NULL);
	m_event = CreateEvent(NULL, TRUE, FALSE, L"configuration");
	read_config();
	if (pthread_create(&m_syncThreadId,NULL,ThreadProc,this) != 0)
	{
		log_printf(LOG_LEVEL_ERROR, "create sync configuration thread failed!\n");
	}
}

CConfiguration::~CConfiguration()
{
	if (m_threadWork)
	{
		m_stopThread = true;
		SetEvent(m_event);
		pthread_join(m_syncThreadId, NULL);
	}
	m_threadWork = false;
	pthread_mutex_destroy(&m_mutex);
	CloseHandle(m_event);
}
CConfiguration& CConfiguration::GetInstance()
{
	static CConfiguration instance;
	return instance;
}
bool CConfiguration::AddDefaultProcess(std::string appName, bool storage)
{
	bool ret = false;
	pthread_mutex_lock(&m_mutex);
	if (m_appMap.find(appName) == m_appMap.end())
	{
		m_appMap.insert(std::make_pair(appName, storage));
		ret = true;
		SetEvent(m_event);
	}
	pthread_mutex_unlock(&m_mutex);
	return ret;
}
bool CConfiguration::DelDefaultProcess(std::string appName)
{
	bool ret = false;
	pthread_mutex_lock(&m_mutex);
	auto iter = m_appMap.find(appName);
	if (iter != m_appMap.end())
	{
		m_appMap.erase(iter);
		ret = true;
		SetEvent(m_event);
	}
	pthread_mutex_unlock(&m_mutex);
	return ret;
}
void CConfiguration::GetDefaultProcessList(std::map<std::string, bool>& appMap)
{
	pthread_mutex_lock(&m_mutex);
	appMap = m_appMap;
	pthread_mutex_unlock(&m_mutex);
}

void CConfiguration::SetServerPort(int port)
{
	pthread_mutex_lock(&m_mutex);
	m_serverPort = port;
	pthread_mutex_unlock(&m_mutex);
	SetEvent(m_event);
}
int CConfiguration::GetServerPort()
{
	int port = 0;
	pthread_mutex_lock(&m_mutex);
	port = m_serverPort;
	pthread_mutex_unlock(&m_mutex);
	return port;
}

void CConfiguration::SetStoragePath(std::string path)
{
	pthread_mutex_lock(&m_mutex);
	m_storagePath = path;
	pthread_mutex_unlock(&m_mutex);
	SetEvent(m_event);
}
std::string CConfiguration::GetStoragePath()
{
	std::string path;
	pthread_mutex_lock(&m_mutex);
	path = m_storagePath;
	pthread_mutex_unlock(&m_mutex);
	return path;
}

void CConfiguration::SyncDataToFile()
{
	m_threadWork = true;
	while (!m_stopThread)
	{
		WaitForSingleObject(m_event,INFINITE);
		ResetEvent(m_event);
		pugi::xml_document doc;
		pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "utf-8";
		doc.append_child("server_port").append_child(pugi::node_pcdata).set_value(ConvertToString(m_serverPort).c_str());
		doc.append_child("storage_path").append_child(pugi::node_pcdata).set_value(m_storagePath.c_str());
		pugi::xml_node applist = doc.append_child("applist");

		std::map<std::string, bool> appMap;
		pthread_mutex_lock(&m_mutex);
		appMap = m_appMap;
		pthread_mutex_unlock(&m_mutex);
		for (auto iter:appMap)
		{
			pugi::xml_node app = applist.append_child("app");
			app.append_child("name").append_child(pugi::node_pcdata).set_value(iter.first.c_str());
			app.append_child("storage").append_child(pugi::node_pcdata).set_value(iter.second ? "1" : "0");
		}
		remove(m_path.c_str());
		doc.save_file(m_path.c_str());
		Sleep(10);
	}
	m_threadWork = false;
}
void* CConfiguration::ThreadProc(void* arg)
{
	CConfiguration* s = (CConfiguration*)arg;
	s->SyncDataToFile();
	return NULL;
}

bool CConfiguration::read_config()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(m_path.c_str());
	if (!result)
	{
		log_printf(loglevel::error, "%s %s failed,load default,description:%s\n", __FUNCTION__, m_path.c_str(), result.description());
		SetEvent(m_event);
		return false;
	}
	m_serverPort = doc.child("server_port").text().as_int();
	m_path = doc.child("path").text().as_string();
	pugi::xml_object_range<pugi::xml_named_node_iterator> app_range = doc.child("applist").children("app");
	for (auto iter = app_range.begin();iter != app_range.end();++iter)
	{
		std::string appName = iter->child("name").text().as_string();
		std::string storageStr = iter->child("storage").text().as_string();
		bool storage = storageStr == "1" ? true : false;
		pthread_mutex_lock(&m_mutex);
		m_appMap.insert(std::make_pair(appName, storage));
		pthread_mutex_unlock(&m_mutex);
	}
	return true;
}