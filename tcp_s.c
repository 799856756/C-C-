#include<stdio.h>
#include<string.h>

#include<unistd.h>
#include"network.h"

void* start_run(void* arg)
{
    NetWork* nw = (NetWork*)arg;
    char buf[1024];
    for(;;)
    {
        nrecv(nw,buf,sizeof(buf));
        printf("server recv:%s\n",buf);
        nsend(nw,buf,strlen(buf)+1);
    }
}

int main()
{
    NetWork* nw = open_network('s',SOCK_STREAM,"192.168.43.114",6677);
    if(NULL == nw)
    {
        printf("open network return null!\n");
        return -1;
    }

    for(;;)
    {
        NetWork* clinw = accept_network(nw);
        if(NULL == clinw)
        {
            continue;
        }
        pthread_t pid;
        pthread_create(&pid,NULL,start_run,clinw);
        usleep(100);
    }
}
