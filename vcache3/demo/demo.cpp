#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include"vservice.h"
#include"vspace.h"

#define DEBUG

void my_cat(const char *path)
{
	int fd = vopen(path, O_RDONLY);
	if (fd < 2)
	{
		printf("can't open file: %s\n", path);
		fflush(stdout);
	}
	else
	{
#ifdef DEBUG
		printf("\n********DO CAT START************\n\n");
#endif // DEBUG

		char buf[1024] = { '\0' };
		int j = 0;
		int end = 0;
		while (end == 0)
		{
			memset(buf, '\0', sizeof(buf));
			vread(fd, buf, 1024);
			j = 0;
			while (j < 1024 && buf[j] != '\0')
			{
				printf("%c", buf[j]);
				fflush(stdout);
				j++;
			}

			if (j < 1024)
			{
				end = 1;
			}
		}

		vclose(fd);

#ifdef DEBUG
		printf("\n********DO CAT END**************\n\n");
#endif // DEBUG
	}

}

void  sys_cat(const char *path)
{
	char cmd[270];
	sprintf(cmd, "cat %s", path);
	//printf("%s",cmd);
	system(cmd);
}

void my_ls()
{
	system("ls -l");
}

void demo_read(int fd, void *buf, size_t nbytes)
{
	int rd = vread(fd, buf, nbytes);
	printf("\nnum:[%d]  read: %s\n", rd, buf);
}
void demo_write(int fd, const void *buf, size_t nbytes)
{
	int rd = vwrite(fd, buf, nbytes);
	printf("\nnum:[%d]  write: %s\n", rd, buf);
}
void clear()
{
	system("clear");
}
int demo_open(const char *path, int oflag)
{
	return vopen("test.txt", O_RDWR);
}
void demo_close(int fd)
{
	vclose(fd);
}

void demo_lseek(int fd, off_t offset, int whence)
{
	off_t pos = vlseek(fd, offset, whence);
	printf("fd:[%d] pos:<%d>\n", fd, pos);
}

void show_op()
{
	printf("OPTIONS:>\n");
	printf("op:	open\n");
	printf("rd:	read\n");
	printf("wr:	write\n");
	printf("cl:	close\n");
	printf("q :	quit\n");
}

void rwdemo()
{
	system("clear");
#ifdef DEBUG
	printf("********READ WRITE DEMO************\n\n");
#endif // DEBUG

	int fd1 = 0;
	int fd2 = 0;
	int fin = 0;
	char cmd[2048] = { '\0' };
	char options[8] = { '\0' };
	char para1[256] = { '\0' };
	char para2[1024] = { '\0' };
	while (1)
	{
		memset(cmd, '\0', sizeof(cmd));
		memset(options, '\0', sizeof(options));
		memset(para1, '\0', sizeof(para1));
		memset(para2, '\0', sizeof(para2));

		printf("demo@rwdemo > ");
		gets(cmd);
		sscanf(cmd, "%s", options);

		if (strcmp(options, "op") == 0 || strcmp(options, "open") == 0)
		{
			//char para1[256] = { '\0' };
			sscanf(cmd, "%s%s", options, para1);
			if (fd1 == 0)
			{

				fd1 = demo_open(para1, O_RDWR);
				printf("open: %s fd1:[%d]\n", para1, fd1);
			}
			else if (fd2 == 0)
			{
				fd2 = demo_open(para1, O_RDWR);
				printf("open: %s fd2:[%d]\n", para1, fd2);
			}
		}
		else if ((strcmp(options, "rd") == 0) || (strcmp(options, "read") == 0))
		{
			sscanf(cmd, "%s%s", options, para1);
			if (strcmp(para1, "fd1") == 0)
			{
				demo_read(fd1, para2, 1024);
			}
			else if (strcmp(para1, "fd2") == 0)
			{
				demo_read(fd2, para2, 1024);
			}
		}
		else if ((strcmp(options, "wr") == 0) || (strcmp(options, "write") == 0))
		{
			sscanf(cmd, "%s%s%s", options, para1, para2);
			if (strcmp(para1, "fd1") == 0)
			{
				demo_write(fd1, para2, strlen(para2));
			}
			else if (strcmp(para1, "fd2") == 0)
			{
				demo_write(fd2, para2, strlen(para2));
			}
		}
		else if ((strcmp(options, "cl") == 0) || (strcmp(options, "close") == 0))
		{
			sscanf(cmd, "%s%s", options, para1);
			if (strcmp(para1, "fd1") == 0)
			{
				demo_close(fd1);
				printf("close: fd1:[%d]\n", fd1);
			}
			else if (strcmp(para1, "fd2") == 0)
			{
				demo_close(fd2);
				printf("close: fd2:[%d]\n", fd2);
			}
		}
		else if ((strcmp(options, "cls") == 0) || (strcmp(options, "clear") == 0))
		{
			clear();
#ifdef DEBUG
			printf("********READ WRITE DEMO************\n\n");
#endif // DEBUG
		}
		else if ((strcmp(options, "cat") == 0))
		{
			sscanf(cmd, "%s%s", options, para1);
			my_cat(para1);
		}
		else if ((strcmp(options, "q") == 0) || (strcmp(options, "quit") == 0))
		{
			if (fd1 > 0)
			{
				demo_close(fd1);
			}
			if (fd2 > 0)
			{
				demo_close(fd2);
			}
			break;
		}
		else if ((strcmp(options, "show") == 0) || (strcmp(options, "s") == 0))
		{
			show_op();
		}
		else if ((strcmp(options, "ls") == 0) || (strcmp(options, "list") == 0))
		{
			my_ls();
		}
		else if ((strcmp(options, "pl") == 0) || (strcmp(options, "print_layout") == 0))
		{
			p_layout();
		}
		else if ((strcmp(options, "lseek") == 0) || (strcmp(options, "seek") == 0))
		{
			int offset = 0;
			sscanf(cmd, "%s%s%d%s", options, para1, offset, para2);

			if (strcmp(para1, "fd1") == 0)
			{
				if (strcmp(para2, "SEEK_CUR") == 0)
				{
					demo_lseek(fd1, offset, SEEK_CUR);
				}
				else if (strcmp(para2, "SEEK_SET") == 0)
				{
					demo_lseek(fd1, offset, SEEK_SET);
				}
				else if (strcmp(para2, "SEEK_END") == 0)
				{
					demo_lseek(fd1, offset, SEEK_END);
				}

			}
			else if (strcmp(para1, "fd2") == 0)
			{
				if (strcmp(para2, "SEEK_CUR") == 0)
				{
					demo_lseek(fd1, offset, SEEK_CUR);
				}
				else if (strcmp(para2, "SEEK_SET") == 0)
				{
					demo_lseek(fd1, offset, SEEK_SET);
				}
				else if (strcmp(para2, "SEEK_END") == 0)
				{
					demo_lseek(fd1, offset, SEEK_END);
				}
			}
		}
		else if ((strcmp(options, "sys_cat") == 0) || (strcmp(options, "scat") == 0))
		{
			sscanf(cmd, "%s%s", options, para1);
			sys_cat(para1);
		}
		else
		{
			/*pass*/
		}
	}

}

int main()
{
	printf("hello from vcache!\n");
	sem_init();
	V_DEBUG_PFUNC;
	V_DEBUG_PFILE;
	V_DEBUG_PLINE;
	//print_layout();
	//char buf[256];
	//char *s = "hahahahahahahahahaha";

	rwdemo();

	//my_cat("test.txt");

	/*int fd1 = vopen("test.txt", O_RDWR);
	printf("this is"
		" a test <%d>\n",fd1);
	int rd=vread(fd1, buf, 20);
	printf("%d : %s\n", rd, buf);
	memset(buf, '\0', 256);
	memcpy(buf, s, strlen(s)+1);
	int lo = strlen(s);
	rd = vwrite(fd1, buf,lo+1 );
	printf("%d : %s\n", rd, buf);
	my_cat("test.txt");
	int fd2 = vopen("test.txt", O_RDWR);
	printf("this is"
		" a test <%d>\n", fd2);
	memset(buf, '\0', 256);
	rd = vread(fd2, buf, 10);
	printf("%d : %s\n", rd, buf);
	vclose(fd1);
	vclose(fd2);*/
	clean();
	return 0;
}