// sysmonitor_server_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "gtest/gtest.h"
int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
	system("pause");
	return 0;
}

