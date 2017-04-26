#include "stdafx.h"
#include "log.h"
#include "messagetypes.h"
#include "ProcessInfo.h"
#include "Configuration.h"
#include <atlconv.h>
using namespace std;
using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;
class RestServer
{
public:
	RestServer() {}
	RestServer(utility::string_t url);
	~RestServer();

	pplx::task<void> open() { return m_listener.open();  }
	pplx::task<void> close() { return m_listener.close(); }

private:
	
	void handle_get(http_request message);
	void handle_put(http_request message);
	void handle_post(http_request message);
	void handle_delete(http_request message);
	http_listener m_listener;
};


RestServer::RestServer(utility::string_t url) : m_listener(url)
{
	m_listener.support(methods::GET, std::bind(&RestServer::handle_get, this, std::placeholders::_1));
	m_listener.support(methods::PUT, std::bind(&RestServer::handle_put, this, std::placeholders::_1));
	m_listener.support(methods::POST, std::bind(&RestServer::handle_post, this, std::placeholders::_1));
	m_listener.support(methods::DEL, std::bind(&RestServer::handle_delete, this, std::placeholders::_1));
}


RestServer::~RestServer()
{
}

void RestServer::handle_get(http_request message)
{
	log_printf(loglevel::debug, message.to_string().c_str());
	auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
	if (paths.empty()|| paths[0] != APP || paths.size() >= 3 )
	{
		message.reply(status_codes::BadRequest);
		return;
	}
	USES_CONVERSION;
	http_response response(status_codes::OK);
	response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
	if (paths.size() == 1)
	{
		std::vector<std::string> appVec;
		CProcessInfo::GetInstance().GetPorcessList(appVec);
		web::json::value result = web::json::value::array(appVec.size());
		size_t index = 0;
		for (auto iter:appVec)
		{
			result[index++] = web::json::value::string(A2W(iter.c_str()));
		}
		response.set_body(result);
	}
	else 
	{
		process_info_t info;
		if (CProcessInfo::GetInstance().GetProcessInfo(W2A(paths[1].c_str()),info))
		{
			web::json::value result = web::json::value::object();
			result[TIME] = web::json::value::string(A2W(info.timeStr.c_str()));
			result[CPU] = web::json::value::number(info.fCpu);
			result[MEM] = web::json::value::number(info.nPhyMemory);
			result[VMEM] = web::json::value::number(info.nVirMemory);
			result[HANDLES] = web::json::value::number(info.nHandleCount);
			result[THREADS] = web::json::value::number(info.nThreadsCount);
			result[NETUP] = web::json::value::number(info.nNetUpKb);
			result[NETDOWN] = web::json::value::number(info.nNetDownkb);
			result[IOWRITE] = web::json::value::number(info.nIoWrite);
			result[IOREAD] = web::json::value::number(info.nIoRead);
			response.set_body(result);
		}
		else
		{
			response.set_status_code(status_codes::NotFound);
		}
	}
	message.reply(response);
}

void RestServer::handle_put(http_request message)
{
	log_printf(loglevel::debug, message.to_string().c_str());
	auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
	message.reply(status_codes::BadRequest);
}

void RestServer::handle_post(http_request message)
{
	log_printf(loglevel::debug, message.to_string().c_str());
	auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
	if (paths.size() != 1 || paths[0] != APP)
	{
		message.reply(status_codes::BadRequest);
		return;
	}
// 	std::map<utility::string_t, utility::string_t> query = uri::split_query(uri::decode(message.request_uri().query()));
// 	auto appName = query.find(APP_NAME);
// 	auto storage = query.find(APP_STOREAGE);
	pplx::task<web::json::value> v = message.extract_json();
	const web::json::value& app = v.get();
	try
	{
		utility::string_t appName = app.at(APP_NAME).as_string();
		bool storage = app.at(APP_STOREAGE).as_bool();
		if (appName.empty())
		{
			message.reply(status_codes::BadRequest);
			return;
		}
		USES_CONVERSION;
		CProcessInfo::GetInstance().AddMonitorProcess(W2A(appName.c_str()), storage);
		CConfiguration::GetInstance().AddDefaultProcess(W2A(appName.c_str()), storage);
		message.reply(status_codes::Accepted);
	}
	catch (web::json::json_exception e)
	{
		log_printf(loglevel::error, "%s parse request error,%s\n", __FUNCTION__, e.what());
		message.reply(status_codes::BadRequest);
	}

}

void RestServer::handle_delete(http_request message)
{
	log_printf(loglevel::debug, message.to_string().c_str());
	auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
	if (paths.size() != 1 || paths[0] != APP)
	{
		message.reply(status_codes::BadRequest);
		return;
	}
}

std::unique_ptr<RestServer> g_restServer;
void on_init_rest(const string_t& address)
{
	uri_builder uri(address);
	uri.append_path(U("sysmonitor"));
	auto addr = uri.to_uri().to_string();
	g_restServer = std::unique_ptr<RestServer>(new RestServer(addr));
	try
	{
		g_restServer->open().wait();
		log_printf(loglevel::info, L"start listen on %s\n", addr.c_str());
	}
	catch (web::http::http_exception e)
	{
		log_printf(loglevel::error, "%s open rest server failed! error:%s\nFATAL ERROR\n", __FUNCTION__, e.what());
	}
	catch (std::exception e)
	{
		log_printf(loglevel::error, "%s open rest server failed! error:%s\nFATAL ERROR\n", __FUNCTION__, e.what());
	}
	return;
}
void on_shutdown_rest()
{
	g_restServer->close().wait();
	return;
}