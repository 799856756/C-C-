#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getch.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "network.h"

int main(int argc,char* argv[])
{
	if(argc < 1)
	{
		printf("cmd error\n");
		return -1;
	}

	NetWork* nw = open_network('c',SOCK_STREAM,argv[1],21);
	if(NULL == nw)
	{
		printf("open network socket null\n");
		return -1;
	}

	char buf[1024] = {};
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	printf("请输入用户名:");
	char username[20] = {};
	gets(username);
	bzero(buf,sizeof(buf));
	sprintf(buf,"USER %s\n",username);
	nsend(nw,buf,strlen(buf));
	nrecv(nw,buf,sizeof(buf));

	printf("请输入密码:");
	char password[20] = {};
	char p;
	int count = 0;
	while(p != 10 && count < 20)
	{
		p = getch();
		if(p == 127)
		{
			if(count > 0) count--;
			continue;
		}
		password[count++] = p;
	}

	bzero(buf,sizeof(buf));
	sprintf(buf,"PASS %s\n",password);
	printf("\n");
	nsend(nw,buf,strlen(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	if(0 == strncmp("230",buf,3))
	{
		sprintf(buf,"SYST\n");
		nsend(nw,buf,strlen(buf));
		bzero(buf,sizeof(buf));
		nrecv(nw,buf,sizeof(buf));
		puts(buf);

		sprintf(buf,"OPTS UTF8 ON\n");
		nsend(nw,buf,strlen(buf));
		bzero(buf,sizeof(buf));
		nrecv(nw,buf,sizeof(buf));
		puts(buf);

		sprintf(buf,"PWD\n");
		nsend(nw,buf,strlen(buf));
		bzero(buf,sizeof(buf));
		nrecv(nw,buf,sizeof(buf));
		puts(buf);
		
		for(;;)
		{
			sprintf(buf,"PASV\n");
			nsend(nw,buf,strlen(buf));
			bzero(buf,sizeof(buf));
			nrecv(nw,buf,sizeof(buf));
			//puts(buf);
			
			unsigned char ip1,ip2,ip3,ip4,port1,port2;
			sscanf(strchr(buf,'(')+1,"%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",&ip1,&ip2,&ip3,&ip4,&port1,&port2);;

			sprintf(buf,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);
			NetWork* data_nw = open_network('c',SOCK_STREAM,buf,port1*256+port2);
			//printf("connect success fd = %d\n",data_nw->fd);

			char cmd[20] = {};
			printf("ftp>");
			gets(cmd);
			if(strncmp(cmd,"list",4) == 0)
			{
				bzero(buf,sizeof(buf));
				sprintf(buf,"LIST -al\n");
				
				nsend(nw,buf,strlen(buf));
				bzero(buf,sizeof(buf));
				nrecv(nw,buf,sizeof(buf));
				puts(buf);
				
				
				int ret = 0;
				bzero(buf,sizeof(buf));
				while(ret = nrecv(data_nw,buf,sizeof(buf)))
				{
					printf("%s",buf);
					bzero(buf,sizeof(buf));
				}
				
				close_network(data_nw);
				printf("\n");

				bzero(buf,sizeof(buf));
				nrecv(nw,buf,sizeof(buf));
				puts(buf);
			}
			else if(strncmp(cmd,"get",3) == 0)
			{
				bzero(buf,sizeof(buf));
				if(strlen(cmd) == 3)
				{
					printf("wrong cmd!\n");
					continue;
				}
				char* str = strstr(cmd," ")+1;
				if(str[0] == ' ')
				{
					printf("wrong cmd!\n");
					continue;
				}

				sprintf(buf,"RETR %s\n",str);
				nsend(nw,buf,strlen(buf));

				bzero(buf,sizeof(buf));
				nrecv(nw,buf,sizeof(buf));
				puts(buf);
				
				bzero(buf,sizeof(buf));
				sprintf(buf,"%s",str);
				int ret = 0;
				int fd = open(buf,O_WRONLY|O_CREAT|O_TRUNC,0644);
				if(0 > fd)
				{
					perror("open");
					return -1;
				}

				bzero(buf,sizeof(buf));
				while(ret = nrecv(data_nw,buf,sizeof(buf)))
				{
					write(fd,buf,ret);
				}
				close(fd);

				close_network(data_nw);
				printf("\n");
				bzero(buf,sizeof(buf));
				nrecv(nw,buf,sizeof(buf));
				puts(buf);
			}
			else if(strncmp(cmd,"put",3) == 0)
			{
				bzero(buf,sizeof(buf));
				if(strlen(cmd) == 3)
				{
					printf("wrong cmd!\n");
					continue;
				}
				char* str = strstr(cmd," ")+1;
				if(str[0] == ' ')
				{
					printf("wrong cmd!\n");
					continue;
				}

				sprintf(buf,"STOR %s\n",str);
				nsend(nw,buf,strlen(buf));

				bzero(buf,sizeof(buf));
				nrecv(nw,buf,sizeof(buf));
				puts(buf);
				
				bzero(buf,sizeof(buf));
				sprintf(buf,"%s",str);
				int ret = 0;
				int fd = open(buf,O_RDONLY);
				if(fd < 0)
				{
					perror("open");
					return -1;
				}
	
	
				bzero(buf,sizeof(buf));
				while(ret = read(fd,buf,sizeof(buf)))
				{
					nsend(data_nw,buf,strlen(buf));
					bzero(buf,sizeof(buf));
				}	
				close(fd);

				close_network(data_nw);
				printf("\n");
				bzero(buf,sizeof(buf));
				nrecv(nw,buf,sizeof(buf));
				puts(buf);
			}
			else if(strncmp(cmd,"cd",2) == 0)
			{
				bzero(buf,sizeof(buf));
				if(strlen(cmd) == 2)
				{
					printf("wrong cmd!\n");
					continue;
				}
				char* str = strstr(cmd," ")+1;
				if(str[0] == ' ')
				{
					printf("wrong cmd!\n");
					continue;
				}
				
				sprintf(buf,"CWD %s\n",str);
				nsend(nw,buf,strlen(buf));
				
				bzero(buf,sizeof(buf));
				nrecv(nw,buf,sizeof(buf));
				puts(buf);

				if(strncmp(buf,"250",3) == 0)
				{
					bzero(buf,sizeof(buf));
					sprintf(buf,"PWD\n");
					nsend(nw,buf,strlen(buf));

					bzero(buf,sizeof(buf));
					nrecv(nw,buf,sizeof(buf));
					puts(buf);
				}
				close_network(data_nw);
				
			}
			else if(strncmp(cmd,"mkdir",5) == 0)
			{
				bzero(buf,sizeof(buf));
				if(strlen(cmd) == 5)
				{
					printf("wrong cmd!\n");
					continue;
				}
				char* str = strstr(cmd," ")+1;
				if(str[0] == ' ')
				{
					printf("wrong cmd!\n");
					continue;
				}
				
				sprintf(buf,"MKD %s\n",str);
				nsend(nw,buf,strlen(buf));
				
				bzero(buf,sizeof(buf));
				nrecv(nw,buf,sizeof(buf));
				puts(buf);

				if(strncmp(buf,"257",3) == 0)
				{
					bzero(buf,sizeof(buf));
					sprintf(buf,"PWD\n");
					nsend(nw,buf,strlen(buf));

					bzero(buf,sizeof(buf));
					nrecv(nw,buf,sizeof(buf));
					puts(buf);
				}
				close_network(data_nw);
			}
			else if(strcmp(cmd,"bye") == 0)
			{
				printf("通信结束\n");				
				close_network(data_nw);
				break;
			}
		}	
	}
	else
	{
		printf("帐号名或密码错误\n");
	}
	close_network(nw);
}
