#pragma once
#include <unistd.h>
#include<fcntl.h>
#include"vdefine.h"

/*data structure of nvm*/

/*the index table for entry list */
struct cache_index
{
	int fd;		/*file descriptor*/
	unsigned short read;
	unsigned short write;
	struct cache_entry * addr[3];		/*direct adddress*/
	struct cache_block * iaddr;		/*indriect address*/
};

/*each cache_entry record a cache item in nvm*/
struct cache_entry
{
	char path[PATH_LEN];		/*file path*/
	off_t offset;
	ssize_t size;
	struct cache_block *buffer_addr;
	unsigned short r_cnt;		/*read count*/
	unsigned short w_cnt;		/*write count*/
	unsigned short a;		/*accessed*/
	unsigned short m;		/*modified*/
};

/*log entry*/
struct cache_log
{
	unsigned long time;
	struct cache_block *buffer_bf;
	struct cache_block *buffer_af;
	unsigned short entry_num;
	unsigned short commit;
	unsigned short read;
	unsigned short write;
};


/*data block*/
struct cache_block
{
	union
	{
		char block[cblock_size];
		struct cache_entry *addr_item[cblock_size / sizeof(struct cache_entry*)];
	};

};


/*functions provide for vcontent*/

/*print address of each part*/
void p_layout();

struct cache_entry * alloc_entry();

struct cache_entry ** find_entry(char *path, struct cache_entry* entry[]);

struct cache_index * alloc_index();

struct cache_index * find_index(int fd);

struct cache_block * alloc_block();

void release_block(struct cache_block *block);

void cache_replace();

struct cache_log * alloc_log();
