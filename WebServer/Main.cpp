// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoop.h"
#include "Server.h"
#include "base/Logging.h"
#include <getopt.h>
#include <string>

int main(int argc, char *argv[])
{
    int threadNum = 4;
    int port = 80;
    std::string logPath = "./WebServer.log";

    // parse args
    // 解析参数
    int opt;
    const char *str = "t:l:p:"; //选项字符串，单个":"表示该选项有且必须有参数
    while ((opt = getopt(argc, argv, str))!= -1)
    {
        switch (opt)
        {
            case 't': // -t 线程数
            {
                threadNum = atoi(optarg);
                break;
            }
            case 'l': // -l 日志文件路径
            {
                logPath = optarg;
                if (logPath.size() < 2 || optarg[0] != '/')
                {
                    printf("logPath should start with \"/\"\n");
                    abort();
                }
                break;
            }
            case 'p': // -p 端口号
            {
                port = atoi(optarg);
                break;
            }
            default: break;
        }
    }
    Logger::setLogFileName(logPath);
    // STL库在多线程上应用
    #ifndef _PTHREADS
        LOG << "_PTHREADS is not defined !";
    #endif
    EventLoop mainLoop; // baseLoop只用来接受新连接
    Server myHTTPServer(&mainLoop, threadNum, port); // 通过自定义的Server类来处理http
    myHTTPServer.start(); // 开启线程池，初始化接收器，在baseLoop中注册接受器Channel
    mainLoop.loop(); // 开启主事件循环，用来接受新连接，然后把新连接交给子线程的事件循环处理
    return 0;
}
