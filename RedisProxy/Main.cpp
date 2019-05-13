#include <Windows.h>
#include <mutex>
#include <condition_variable>
#include "MsgWorkerManager.h"
#include "MsgQueueManager.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "hiredis.lib")

std::mutex g_objExitMutex;
std::condition_variable g_objExitCond;

BOOL WINAPI ConsoleHandler(DWORD msgType)
{
    BOOL bRtn = FALSE;

    switch (msgType)
    {
        //CTRL+C
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        bRtn = TRUE;
        break;

    default:
        bRtn = FALSE;
        break;
    }

    if (bRtn)
    {
        printf(": ConsoleHandler msgType = %u\n", msgType);
        g_objExitCond.notify_one();
    }

    return bRtn;
}

// 加载socket动态链接库
bool LoadWindowsSocketLib()
{
    // 请求2.2版本的WinSock库
    WORD wVersionRequested = MAKEWORD(2, 2);

    // 接收Windows Socket的结构信息
    WSADATA wsaData;

    // 检查套接字库是否申请成功
    DWORD err = WSAStartup(wVersionRequested, &wsaData);
    if (0 != err) {
        return false;
    }

    // 检查是否申请了所需版本的套接字库
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        WSACleanup();
        return false;
    }
    return true;
}

// 释放socket动态链接库
void UnLoadWindowsSocketLib()
{
    WSACleanup();
}

int main()
{
    LoadWindowsSocketLib();

    // 绑定监听
    (void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);

    // 启动服务
    MsgWorkerManager::instance().Start();
    MsgQueueManager::instance().Start();

    // 等待事件
    std::unique_lock<std::mutex> lock(g_objExitMutex);
    g_objExitCond.wait(lock);

    // 停止服务
    MsgQueueManager::instance().Stop();
    MsgWorkerManager::instance().Stop();

    // 结束进程
    UnLoadWindowsSocketLib();

    system("pause");
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
