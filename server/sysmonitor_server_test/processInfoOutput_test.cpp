#include "stdafx.h"
#include "gtest/gtest.h"
#include "../sysmonitor_server/ProcessInfoOutput.h"
#include <vector>
#include "../sysmonitor_server/log.h"
#include "../sysmonitor_server/pugixml/pugixml.hpp"
#include "../sysmonitor_server/util.h"
#include "../sysmonitor_server/messagetypes.h"
using namespace testing;
class CProcessInfoOutput_Test :public testing::Test
{
protected:
	virtual void SetUp()
	{
		log_init("D://Test",LOG_NONE_OUT);
		_output = new CProcessInfoOutput("system");
	}
	virtual void TearDown()
	{
	//	remove(_output->m_path.c_str());
		delete _output;
	}
	CProcessInfoOutput* _output;

	int GetOutPutNodeCount();
};
int CProcessInfoOutput_Test::GetOutPutNodeCount()
{
	int ret = 0;
	pugi::xml_document doc;
	if (doc.load_file(_output->m_path.c_str()))
	{
		ret = -1;
		return ret;
	}
	pugi::xml_node node_list = doc.child("root").child(MYW2A(NODE_LIST).c_str());
	for (auto node = node_list.child("node");node;node = node.next_sibling())
	{
		++ret;
	}
	return ret;
}
TEST_F(CProcessInfoOutput_Test, writeTestSuccess)
{
	int count = 86400;
	process_info_t info;
	info.timeStr = "2016-12-07 00:00:00";
	info.fCpu = 50;
	info.nIoRead = 100000;
	info.nIoWrite = 900000;
	info.nPhyMemory = 1024000000;
	info.nVirMemory = 2048000000;
	info.nNetUpKb = 109000000;
	info.nNetDownkb = 80000000;
	info.nThreadsCount = 1000;
	info.nHandleCount = 0;
	std::string outputPath = "D://Test";
	ASSERT_TRUE(_output->Open(outputPath));
	Sleep(200);
	while (++info.nHandleCount < count)
	{
		EXPECT_TRUE(_output->Write(info));
		Sleep(1);
	}
	
	Sleep(500);
	info.timeStr = "2016-12-07 00:00:01";
	EXPECT_TRUE(_output->Write(info));
	EXPECT_TRUE(_output->Flush());
	EXPECT_TRUE(_output->Close());
	EXPECT_EQ(count, GetOutPutNodeCount());
}

TEST_F(CProcessInfoOutput_Test, writeTestFailure)
{
	process_info_t info;
	std::string outputPath = "D://Test";
	EXPECT_FALSE(_output->Write(info));
	EXPECT_TRUE(_output->Flush());
	EXPECT_FALSE(_output->Close());
}

TEST_F(CProcessInfoOutput_Test, illegaPathTest)
{
	std::string outputPath = "K://";
	EXPECT_FALSE(_output->Open(outputPath));
}