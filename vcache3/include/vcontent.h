#pragma once
#include<unistd.h>
#include"vdefine.h"

struct search_body
{
	struct cache_index *index;
	struct cahce_entry *fentry[ENTRY_NUM];
};

/*record the order of io requests*/
struct ioqueue
{
	pid_t pid;
	int pathnum;
	int fd;
	int type;	/*read:0 write:1*/
	int enable;
	struct ioqueue *next;
};

struct pathlist
{
	char path[PATH_LEN];
	int fd[PATH_LEN];
};

/*functions provide by content management*/

struct cache_entry *regist_entry(char *path, int oflag);

struct cache_index * regist_index(int fd, struct cache_entry *entry, int oflag);

ssize_t pre_read(struct cache_index *index, off_t offset);

struct cache_index * verify_fd(int fd, struct cache_index *index);

int search_cache(struct search_body *s_body, off_t curpos, size_t nbytes);

ssize_t read_2_ubuf(void *buf, off_t curpos, size_t nbytes, struct search_body *s_body);

ssize_t direct_2_ubuf(int fd, void *buf, size_t nbytes);

ssize_t write_2_cbuf(const void *buf, off_t curpos, size_t nbytes, struct search_body * s_body);

ssize_t direct_2_cbuf(int fd, const void *buf, size_t nbytes);

int clean_up(struct cache_index *index);

off_t vvlseek(int fd, off_t offset, int whence);

int vvcreat(const char *path, mode_t mode);

ssize_t do_read(struct ioqueue *iread, struct cache_index *index, void *buf, size_t nbytes);

ssize_t do_write(struct ioqueue *owrite, struct cache_index *tmpindex, const void *buf, size_t nbytes);