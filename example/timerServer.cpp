#include <string.h>
#include "ConfigFile.h"
#include "TcpServer.h" 

TcpServer *server;
void buz1(EventLoop* loop, void* usr_data)
{
    FILE* fp = (FILE*)usr_data;
    fprintf(fp, "once display ts %ld\n", time(NULL));
    fflush(fp);
}

void buz2(EventLoop* loop, void* usr_data)
{
    FILE* fp = (FILE*)usr_data;
    fprintf(fp, "always display ts %ld\n", time(NULL));
    fflush(fp);
}

int main(void)
{
    EventLoop loop;
 
    ConfigFile::setPath("./reactor.ini");
    std::string ip = ConfigFile::instance()->GetString("reactor", "ip", "0.0.0.0");
    short port = ConfigFile::instance()->GetNumber("reactor", "port", 8888);

    server = new TcpServer(&loop, ip.c_str(), port);
    FILE* fp = fopen("output.log", "w");
    loop.RunAfter(buz1, fp, 10);
    loop.RunEvery(buz2, fp, 1, 500);

    loop.EventProcess();

    fclose(fp);
    return 0;
}