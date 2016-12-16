#include "stdafx.h"
#include "ProcessInfoOutput.h"
#include "util.h"
#include "pugixml/pugixml.hpp"
#include "log.h"
#include <direct.h>
#include <io.h>
#include <atlconv.h>
#include "messagetypes.h"
CProcessInfoOutput::CProcessInfoOutput(std::string appName,int syncTrigger) :
m_syncDataToFileStopFlag(false),
m_isSyncThreadWork(false),
m_flushFlag(false),
m_appName(appName),
m_writeToFileTrigger(syncTrigger)
{
	pthread_mutex_init(&m_processInfoMutex, NULL);
}


CProcessInfoOutput::~CProcessInfoOutput()
{
	pthread_mutex_destroy(&m_processInfoMutex);
}

bool CProcessInfoOutput::Open( const std::string path)
{
	if (m_appName.empty())
	{
		log_printf(LOG_LEVEL_WARNING, "%s:appname should not be empty!\n",__FUNCTION__);
		return false;
	}
	m_path = path;
	if (m_path.empty())
	{
		char tmp[255] = { 0 };
		_getcwd(tmp, sizeof(tmp));
		m_path = tmp;
	}
	m_path = m_path + "\\appinfo";
	if (_access(m_path.c_str(), 0) == -1 && !CreateDirectoryA(m_path.c_str(), NULL))
	{
		//illega path
		log_printf(LOG_LEVEL_WARNING, "%s:illega path->%s,lasterror:%d!\n", __FUNCTION__,m_path.c_str(),GetLastError());
		return false;
	}
	m_path = m_path + "\\" + m_appName;
	if (_access(m_path.c_str(), 0) == -1 && !CreateDirectoryA(m_path.c_str(), NULL))
	{
		//illega path
		log_printf(LOG_LEVEL_WARNING, "%s:illega path->%s,lasterror:%d!\n", __FUNCTION__, m_path.c_str(), GetLastError());
		return false;
	}
	m_path = m_path + "\\" + m_appName + " " + GetCurrentTime_hhmmss() + ".xml";
	if (pthread_create(&m_syncFileThreadId, NULL, WriteDataToFileProc, this) != 0)
	{
		log_printf(LOG_LEVEL_WARNING, "%s:create thread failed!\n", __FUNCTION__);
		return false;
	}
	return true;
}

bool CProcessInfoOutput::Write(const process_info_t& info)
{
	bool ret = m_isSyncThreadWork;
	pthread_mutex_lock(&m_processInfoMutex);
	m_infoQueue.push(info);
	pthread_mutex_unlock(&m_processInfoMutex);
	return ret;
}

bool CProcessInfoOutput::Flush()
{
	pthread_mutex_lock(&m_processInfoMutex);
	m_flushFlag = true;
	pthread_mutex_unlock(&m_processInfoMutex);
	Sleep(200);
	return true;
}
bool CProcessInfoOutput::Close()
{
	bool ret = false;
	if (m_isSyncThreadWork)
	{
		m_syncDataToFileStopFlag = true;
		pthread_join(m_syncFileThreadId, NULL);
		ret = true;
	}
	
	return ret;
}

void CProcessInfoOutput::WriteDataToFile()
{
	//创建一个新的xml文件
	pugi::xml_document doc;
	pugi::xml_node decl = doc.append_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "utf-8";
	pugi::xml_node root = doc.append_child("root");
	pugi::xml_node app = root.append_child(MYW2A(APP).c_str());
	app.append_child(pugi::node_pcdata).set_value(m_appName.c_str());
	pugi::xml_node node_list = root.append_child(MYW2A(NODE_LIST).c_str());
	doc.save_file(m_path.c_str());

	m_isSyncThreadWork = true;
	while (!m_syncDataToFileStopFlag)
	{
		pthread_mutex_lock(&m_processInfoMutex);
		int bufCount = m_infoQueue.size();
		pthread_mutex_unlock(&m_processInfoMutex);
		if (bufCount >= m_writeToFileTrigger || m_flushFlag)
		{
			std::queue<process_info_t> infoQueue;
			pthread_mutex_lock(&m_processInfoMutex);
			infoQueue = m_infoQueue;
			while (!m_infoQueue.empty())
			{
				m_infoQueue.pop();
			}
			pthread_mutex_unlock(&m_processInfoMutex);
			pugi::xml_document doc2;
			pugi::xml_parse_result result = doc2.load_file(m_path.c_str());
			if (!result)
			{
				log_printf(loglevel::error, "%s:load %s failed,description:%s\n", __FUNCTION__, m_path.c_str(), result.description());
				Sleep(100);
				continue;
			}
			node_list = doc2.child("root").child(MYW2A(NODE_LIST).c_str());

			while (infoQueue.size())
			{
				process_info_t info = infoQueue.front();
				infoQueue.pop();

				pugi::xml_node node = node_list.append_child(MYW2A(APP).c_str());
				node.append_child(MYW2A(TIME).c_str()).append_child(pugi::node_pcdata).set_value(info.timeStr.c_str());
				node.append_child(MYW2A(CPU).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.fCpu).c_str());
				node.append_child(MYW2A(MEM).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.nPhyMemory).c_str());
				node.append_child(MYW2A(VMEM).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.nVirMemory).c_str());
				node.append_child(MYW2A(NETUP).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.nNetUpKb).c_str());
				node.append_child(MYW2A(NETDOWN).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.nNetDownkb).c_str());
				node.append_child(MYW2A(IOWRITE).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.nIoWrite).c_str());
				node.append_child(MYW2A(IOREAD).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.nIoRead).c_str());
				node.append_child(MYW2A(THREADS).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.nThreadsCount).c_str());
				node.append_child(MYW2A(HANDLES).c_str()).append_child(pugi::node_pcdata).set_value(ConvertToString(info.nHandleCount).c_str());
			}
			doc2.save_file(m_path.c_str());
		}
		Sleep(100);
	}
	m_isSyncThreadWork = true;
}
void* CProcessInfoOutput::WriteDataToFileProc(void* arg)
{
	CProcessInfoOutput* s = (CProcessInfoOutput*)arg;
	s->WriteDataToFile();
	return NULL;
}