#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <fcntl.h>
#include"vcontent.h"
#include"vspace.h"

/*****************************************/
/*****************************************/
/*semaphore operate function area :BEGIN*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int ban_io_rd = 0;
int ban_io_rw = 0;

struct sembuf sem_lock;
struct sembuf sem_unlock;


union semun {
	int              val;    /* Value for SETVAL */
	struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;  /* Array for GETALL, SETALL */
};


static int sem_id = 0;

static int set_semvalue()
{

	union semun sem_union;

	sem_union.val = 1;
	if (semctl(sem_id, 0, SETVAL, sem_union) == -1)
	{
		return 0;
	}
	return 1;
}

static void del_semvalue()
{

	union semun sem_union;

	if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
	{
		fprintf(stderr, "Failed to delete semaphore\n");
	}
}

static int semaphore_p()
{

	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;//P()
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1)
	{
		fprintf(stderr, "semaphore_p failed\n");
		return 0;
	}
	return 1;
}

static int semaphore_v()
{

	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = 1;//V()
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1)
	{
		fprintf(stderr, "semaphore_v failed\n");
		return 0;
	}
	return 1;
}

/*senaphore init here*/
void sem_init()
{
	sem_id = semget((key_t)1234, 1, 0666 | IPC_CREAT);
	set_semvalue();
}

/*semaphore operate function area :END*/
/************************************************/
/**********************************************/
void clean()
{
	del_semvalue();
}

/************************************************/
/************************************************/
/*io queue operate function : BEGIN*/
struct pathlist path_list[MAX_LIST];

const int item_size = sizeof(struct ioqueue);
struct ioqueue *iohead = (struct ioqueue*)malloc(item_size);
struct ioqueue *iotail = iohead;

struct ioqueue *add_queue(pid_t pid, int fd, int type)
{
	if (ban_io_rd == 0)
	{
		struct ioqueue *ioele = (struct ioqueue*)malloc(item_size);
		memset(ioele, '\0', sizeof(ioele));

		ioele->fd = fd;
		ioele->pid = pid;
		ioele->type = type;
		ioele->enable = 0;
		ioele->next = 0x0;

		int i = 0;
		int j = 0;
		int found = 0;
		for (; i < MAX_LIST && found == 0; i++)
		{
			for (j = 0; j < MAX_LIST; j++)
			{
				if (path_list[i].fd[j] == fd)
				{
					ioele->pathnum = i;
					found = 1;
					break;
				}
			}
		}

		iotail->next = ioele;
		iotail = ioele;

		if (i < MAX_LIST)
		{
			return iotail;
		}
		else
		{
			free(ioele);
			return NULL;
		}
	}
	else
		return NULL;

}

struct ioqueue *find(struct ioqueue *tofind, pid_t pid, int fd, int type)
{
	struct ioqueue *find_ele = iohead->next;
	struct ioqueue *find_ele_pre = iohead;
	while (find_ele != NULL)
	{
		if (find_ele->fd == fd && find_ele->pid == pid && find_ele->type == type && find_ele == tofind)
		{
			return find_ele_pre;
		}
		else
		{
			find_ele_pre = find_ele;
			find_ele = find_ele->next;
		}
	}
	return NULL;
}
/*need del directly by queue element*/
/*need optimize*/
void del_queue(struct ioqueue *todel, pid_t pid, int fd, int type)
{
	struct ioqueue *find_ele_pre = find(todel, pid, fd, type);
	if (find_ele_pre->next != NULL && find_ele_pre->next == todel)
	{
		struct ioqueue *find_ele = find_ele_pre->next;

		find_ele_pre->next = find_ele->next;

		if (find_ele->next == NULL)
		{
			iotail = find_ele_pre;
		}

		free(find_ele);


	}
	else
	{
		/*pass*/
	}
}
struct ioqueue ** find_same(int fd, int type, struct ioqueue *found[])
{
	struct ioqueue *tmp = iohead->next;
	int pathnum = -1;
	while (tmp != NULL)
	{
		if (tmp->fd == fd && tmp->type == type)
		{
			pathnum = tmp->pathnum;
			break;
		}

		tmp = tmp->next;
	}
	if (tmp != NULL)
	{
		int i = 0;
		tmp = iohead->next;
		while (tmp != NULL)
		{
			if (tmp->pathnum == pathnum && tmp->enable == 1)
			{
				found[i++] = tmp;
			}
			tmp = tmp->next;
		}
	}
	return found;
}

void set_read_enable(int fd, int type)
{
	struct ioqueue *tmp = iohead->next;
	int pathnum = -1;
	while (tmp != NULL)
	{
		if (tmp->fd == fd && tmp->type == type)
		{
			pathnum = tmp->pathnum;
			break;
		}

		tmp = tmp->next;
	}

	if (tmp != NULL)
	{
		int writeflag = 0;

		tmp = iohead->next;
		while (tmp != NULL)
		{
			if (tmp->pathnum == pathnum)
			{
				if (tmp->type == 1)	/*write*/
				{
					writeflag = 1;
					break;
				}
				if (writeflag == 0 && tmp->enable == 0)
				{
					tmp->enable = 1;	/*read enable*/
				}
			}
			tmp = tmp->next;
		}
	}
}

int write_enable(int fd, int type)
{
	struct ioqueue *tmp = iohead->next;
	int pathnum = -1;
	while (tmp != NULL)
	{
		if (tmp->fd == fd && tmp->type == type)
		{
			pathnum = tmp->pathnum;
			break;
		}

		tmp = tmp->next;
	}

	if (tmp != NULL)
	{
		//int readflag = 0;
		tmp = iohead->next;
		while (tmp != NULL)
		{
			if (tmp->pathnum == pathnum)
			{
				if (tmp->type == 0)
				{
					return 0;
				}
				if (tmp->type == type)
				{
					return 1;
				}
			}
			tmp = tmp->next;
		}
	}
}

/*io queue operate function : END*/
/************************************************/


/************************************************/
/*functions for service*/

int vcreat(const char *path, mode_t mode)
{

	return vvcreat(path, mode);
}

int vopen(const char *path, int oflag)
{
	/*get path of the file*/
	char tmpbuf[PATH_LEN] = { '\0' };
	struct cache_entry *vtmpentry = 0x0;
	struct cache_index *vtmpindex = 0x0;
	if (getcwd(tmpbuf, PATH_LEN - strlen(path) - 2) != NULL)
	{
		char ch[1];
		ch[0] = '/';
		strncat(tmpbuf, ch, 1);
		strncat(tmpbuf, path, strlen(path));
		vtmpentry = regist_entry(tmpbuf, oflag);
	}
	else
	{
		return -1;	/*path error*/
	}

	int fd;
	/*do sys_open*/
	fd = open(path, oflag);

	if (fd > 2)
	{
		vtmpindex = regist_index(fd, vtmpentry, oflag);
	}
	else
	{
		/*need release entry*/
		return -2;	/*open error*/
	}
	if ((oflag ^ O_RDONLY) == 0 || (oflag ^ O_RDWR) == 0)
	{
		/*need optimize*/
		pre_read(vtmpindex, 0);
	}
	//printf("%s\n", vtmpentry->buffer_addr);

	/*regist pathlist*/
	int i = 0;
	int j = 0;
	int found = 0;
	for (i = 0; i < MAX_LIST; i++)
	{
		if (strcmp(vtmpentry->path, path_list[i].path) == 0)
		{
			found = 1;
			break;
		}
	}
	if (found == 1)
	{
		for (j = 0; j < MAX_LIST; j++)
		{
			if (path_list[i].fd[j] == 0)
			{
				path_list[i].fd[j] = fd;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < MAX_LIST; i++)
		{
			if (path_list[i].path[0] == '\0')
			{
				strncpy(path_list[i].path, vtmpentry->path, strlen(vtmpentry->path));
				path_list[i].fd[0] = fd;
				break;
			}
		}
	}

	if (i < MAX_LIST)
	{
		return fd;
	}
	else
	{
		return -1;
	}

}

ssize_t vread(int fd, void *buf, size_t nbytes)
{
#ifdef V_DEBUG
	printf("**********CACHE READ*************\n");
	printf("read: <%d> pos:<%d>\n", fd, lseek(fd, 0, SEEK_CUR));
	printf("**********CACHE READ*************\n");
#endif // V_DEBUG


	memset(buf, '\0', sizeof(buf));

	struct cache_entry *tmpentry = 0x0;
	struct cache_index *tmpindex = 0x0;


	tmpindex = verify_fd(fd, tmpindex);

	if (tmpindex != NULL && tmpindex->fd == fd && tmpindex->read == 1)
	{
		/*add to ioqueue*/
		pid_t pid = getpid();
		struct ioqueue *iread = 0x0;

		while (1)
		{
			iread = add_queue(pid, fd, 0);
			if (iread != NULL) break;
		}

		struct flock lock;
		//int set = 0;
		while (1)
		{
		SET:	memset(&lock, '\0', sizeof(lock));
			fcntl(fd, F_GETLK, &lock);
			if (lock.l_type == F_RDLCK && ban_io_rw == 0)
			{
				/*do read*/

				ssize_t rdsize = 0;
				if (iread->enable == 1)
				{
					rdsize = do_read(iread, tmpindex, buf, nbytes);
				}

				del_queue(iread, pid, fd, 0);

				return rdsize;
			}
			else if (lock.l_type == F_UNLCK)
			{
				/*get semaphore*/
				if (semaphore_p())
				{
					/*SET LOCK*/
					int ret;
					do
					{
						lock.l_type = F_RDLCK;
						lock.l_len = 0;
						lock.l_start = 0;
						lock.l_whence = SEEK_SET;
						lock.l_pid = getpid();

						/*lock io*/
						ban_io_rw = 1;
						ret = fcntl(fd, F_SETLK, &lock);
					} while (ret < 0);

					/*enable read*/
					set_read_enable(fd, 0);

					/*unlock io*/
					ban_io_rw = 0;
				}
				else
				{
					goto SET;
				}
				/*release*/
				semaphore_v();

				/*do read*/

				ssize_t rdsize = do_read(iread, tmpindex, buf, nbytes);
				del_queue(iread, pid, fd, 0);


				/*unlock  file*/

				/*lock read queue*/
				if (ban_io_rd == 0)
				{
					ban_io_rd = 1;
				}

				struct ioqueue *findrd[MAX_LIST];

				while (1)
				{
					memset(findrd, '\0', sizeof(findrd));
					find_same(fd, 0, findrd);
					if (findrd[0] == NULL) break;
				}

				/*unlock*/
				fcntl(fd, F_SETLK, &lock);

				/*release read queue*/
				if (ban_io_rd == 1)
				{
					ban_io_rd = 0;
				}
#ifdef V_DEBUG
				printf("**********CACHE READ*************\n");
				printf("read: <%d> pos:<%d>  get:<%d>\n", fd, lseek(fd, 0, SEEK_CUR), rdsize);
				printf("**********CACHE READ*************\n");
#endif
				return rdsize;
			}
			else
			{
				/*wait*/
			}
		}


	}

	return 0;
}

ssize_t vwrite(int fd, const void *buf, size_t nbytes)
{
#ifdef V_DEBUG
	printf("**********CACHE WRITE*************\n");
	printf("write: <%d> pos:<%d>\n ", fd, lseek(fd, 0, SEEK_CUR));
	printf("**********CACHE WRITE*************\n");
#endif // V_DEBUG



	struct cache_index *tmpindex = 0x0;
	struct cahce_entry *tmpentry = 0x0;

	tmpindex = verify_fd(fd, tmpindex);

	if (tmpindex != NULL && tmpindex->fd == fd && tmpindex->write == 1)
	{
		/*add to ioqueue*/
		pid_t pid = getpid();
		struct ioqueue *owrite;

		while (1)
		{
			owrite = add_queue(pid, fd, 1);
			if (owrite != NULL) break;
		}

		struct flock lock;

		while (1)
		{
		STE:	memset(&lock, '\0', sizeof(lock));
			fcntl(fd, F_GETLK, &lock);

			if (lock.l_type == F_UNLCK && write_enable(fd, 1))
			{
				if (semaphore_p())
				{
					/*SET LOCK*/
					int ret;
					do
					{
						lock.l_type = F_WRLCK;
						lock.l_len = 0;
						lock.l_start = 0;
						lock.l_whence = SEEK_SET;
						lock.l_pid = getpid();

						ret = fcntl(fd, F_SETLK, &lock);
					} while (ret < 0);

				}
				else
				{
					goto STE;
				}

				/*do write*/
				ssize_t wrsize = 0;
				wrsize = do_write(owrite, tmpindex, buf, nbytes);

				del_queue(owrite, pid, fd, 1);

				/*release semaphore*/
				semaphore_v();
#ifdef V_DEBUG
				printf("**********CACHE WRITE*************\n");
				printf("write: <%d> pos:<%d>  put:<%d>\n ", fd, lseek(fd, 0, SEEK_CUR), wrsize);
				printf("**********CACHE WRITE*************\n");
#endif // V_DEBUG


				return wrsize;
			}
			else if (lock.l_type == F_WRLCK)
			{
				/*WAIT*/
				/*only one process permitted*/
			}
			else if (lock.l_type == F_RDLCK)
			{
				/*lock read queue*/
				//ban_io_rd = 1;
			}

		}
	}

	return 0;
}

off_t vlseek(int fd, off_t offset, int whence)
{
	return vvlseek(fd, offset, whence);
}

int vclose(int fd)
{

	struct cache_entry *tmpentry = 0x0;
	struct cache_index *tmpindex = 0x0;


	tmpindex = verify_fd(fd, tmpindex);

	/*found*/
	if (tmpindex != NULL && tmpindex->fd == fd)
	{
		/*record path*/
		char path[PATH_LEN] = { '\0' };

		memcpy(path, tmpindex->addr[0]->path, PATH_LEN);

		/*clean index and entries*/
		clean_up(tmpindex);

		/*clean pathlist*/

		int i = 0;
		int j = 0;
		int found = 0;
		for (i = 0; i < MAX_LIST; i++)
		{
			if (strcmp(path, path_list[i].path) == 0)
			{
				found = 1;
				break;
			}
		}
		if (found == 1)
		{
			for (j = 0; j < MAX_LIST; j++)
			{
				if (path_list[i].fd[j] == fd)
				{
					path_list[i].fd[j] = 0;
					break;
				}
			}

			for (j = 0; j < MAX_LIST; j++)
			{
				if (path_list[i].fd[j] != 0)
				{
					break;
				}
			}
			/*path has no fd reserved*/
			if (j >= MAX_LIST)
			{
				memset(path_list[i].path, '\0', PATH_LEN);
			}

		}
		else
		{
			/*error: not define*/

			return -1;
		}

	}

	return 0;
}