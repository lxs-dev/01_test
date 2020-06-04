#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>


#define FILE "/dev/lxs_key"
int fd;

int main()
{

	int i;
	int oflage;
	int val;
	int size;
	struct pollfd fds[1];
		
   	fd = open(FILE, O_RDWR|O_NONBLOCK);
	if(fd<0)
		{
		printf("open file failed \n");
		return 0;
	}

	for(i=0;i<10;i++)
		{
			if(read(fd,&val,4)==4)
				{
				printf("val: %d \n", val);
			}else
				{
				printf("get button : -1\n");
			}
	}
	
	oflage = fcntl(fd,F_GETFL);
	fcntl(fd,F_SETFL,oflage & ~O_NONBLOCK);
	
	 while (1)
	 {
	 	size = read(fd, &val, 4);
		printf("size: %d \n", size );
	//	printf("sizel: %d \n", val);
	
		if(read(fd, &val, 4) == 4 )
		{
			printf("while val: %d \n", val);
		}else
		{
			printf("while get button : -1\n");
		}
	 }

	 return 0;
}



