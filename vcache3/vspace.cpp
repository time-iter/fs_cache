#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"vspace.h"
#include"vdefine.h"

/*size of nvm data structure*/
extern const int log_size = sizeof(struct cache_log);
extern const int entry_size = sizeof(struct cache_entry);
extern const int index_size = sizeof(struct cache_index);
extern const int addr_size = sizeof(struct cache_index*);

/*cache block*/
//const int cblock_size = 1024 * 1024; /*block size is 1Mb*/
extern int cbit_map[BIT_MAP] = { 0 };
extern const int bitmap_size = sizeof(cbit_map);

/********************************************************************/
/************Very Important Data****************/
/************The layout of nvm space************/
/*************|-----------------|***************/
/*************|    Index Table  |***************/
/*************|-----------------|***************/
/*************|    Entry Table  |***************/
/*************|-----------------|***************/
/*************|      BitMap     |***************/
/*************|-----------------|***************/
/*************|    Cache Block  |***************/
/*************|-----------------|***************/
/*************|        Log      |***************/
/*************|-----------------|***************/
const unsigned int Total = 200 * 1024 * 1024;	/*200MB*/

/*alloc nvm*/
const char *nvm = (char*)malloc(Total);
/*start address of each part*/
const char* Index_Start = nvm + 0x0;
extern const char* Entry_Start = nvm + INDEX_NUM * index_size;
const char* Bitmap_Start = Entry_Start + ENTRY_NUM * entry_size;
extern const char* Log_Start = nvm + (Total - LOG_NUM * log_size);
const char* Block_Start = Log_Start - \
(Log_Start - Bitmap_Start - cblock_size) / cblock_size * cblock_size;
/****************************************************************/

const int Block_Num = (int)(Log_Start - Block_Start) / cblock_size;

/***************************************************************/
/*functions for management of nvm cache*/

void p_layout()
/*print address of each part*/
{
	printf("Total Size:     %d MB\n", Total / 1024 / 1024);
	printf("Index_Start:    %p\n", Index_Start);
	printf("Index_Size:     %.3f KB\n", (float)(Entry_Start - Index_Start) / 1024);
	printf("Entry_Start:    %p\n", Entry_Start);
	printf("Entry_Size:     %.3f KB\n", (float)(Bitmap_Start - Entry_Start) / 1024);
	printf("Bitmap_Start:   %p\n", Bitmap_Start);
	printf("Block_Start:    %p\n", Block_Start);
	printf("Block_Size:     %.3f KB\n", (float)(Log_Start - Block_Start) / 1024);
	printf("Block_num:      %d\n", Block_Num);
	printf("Log_Start:      %p\n", Log_Start);
	printf("Log_Size:       %.3f KB\n", (float)(nvm + Total - Log_Start) / 1024);
	printf("\n***************The layout of nvm space*************\n"
		"***************************************************\n"
		"***%p|-----------------|***************\n"
		"*****************|    Index Table  |***************\n"
		"***%p|-----------------|***************\n"
		"*****************|    Entry Table  |***************\n"
		"***%p|-----------------|***************\n"
		"*****************|      BitMap     |***************\n"
		"***%p|-----------------|***************\n"
		"*****************|    Cache Block  |***************\n"
		"***%p|-----------------|***************\n"
		"*****************|        Log      |***************\n"
		"*****************|-----------------|***************\n"
		"***************************************************\n\n",
		Index_Start, Entry_Start, Bitmap_Start, Block_Start, Log_Start);
}

struct cache_entry * alloc_entry()
{
	int i = 1;

	struct cache_entry *tmpentry = (struct cache_entry*)malloc(entry_size);
	memset(tmpentry, '\0', entry_size);

	memcpy(tmpentry, Entry_Start, entry_size);
	/*find the first empty entry*/
	while (i <= ENTRY_NUM && (tmpentry->path[0] != '\0'))
	{
		if (i >= ENTRY_NUM) break;
		memset(tmpentry, '\0', entry_size);
		memcpy(tmpentry, Entry_Start + i * entry_size, entry_size);
		i++;
	}

	/*find: success >empty*/
	if (tmpentry->path[0] == '\0')
	{
		free(tmpentry);
		return (struct cache_entry*)(Entry_Start + (i - 1)*entry_size);
	}
	/*find: fail*/
	else
	{
		/*exchange happens*/
	}
}

struct cache_entry ** find_entry(char *path, struct cache_entry* entry[])
{
	int i = 1, j = 0;

	struct cache_entry *tmpentry = (struct cache_entry*)malloc(entry_size);
	memset(tmpentry, '\0', entry_size);

	memcpy(tmpentry, Entry_Start, entry_size);
	/*search in entry*/
	while (i <= ENTRY_NUM)
	{
		if (tmpentry->path[0] != '\0')
		{
			if (strcmp(tmpentry->path, path) == 0)
			{
				entry[j++] = (struct cache_entry*)(Entry_Start + (i - 1)*entry_size);
			}
		}
		if (i >= ENTRY_NUM) break;
		memset(tmpentry, '\0', entry_size);
		memcpy(tmpentry, Entry_Start + i * entry_size, entry_size);
		i++;
	}

	free(tmpentry);
	return entry;
}

struct cache_index * alloc_index()
{
	int i = 1;
	struct cache_index *tmpindex = (struct cache_index*)malloc(index_size);
	memset(tmpindex, '\0', index_size);

	memcpy(tmpindex, Index_Start, index_size);

	/*find the first empty index*/
	while (i <= INDEX_NUM && (tmpindex->fd != 0))
	{
		if (i >= INDEX_NUM) break;
		memset(tmpindex, '\0', index_size);
		memcpy(tmpindex, Index_Start + i * index_size, index_size);
		i++;
	}

	/*find: success*/
	if (tmpindex->fd == 0)
	{
		free(tmpindex);
		return (struct cache_index*)(Index_Start + (i - 1)*index_size);
	}
	/*find: fail*/
	/*unfinished*******/
	/******************/
	else
	{
		/*error, unused*/
		/*need redisign*/
	}
}

struct cache_index * find_index(int fd)
{
	int i = 1, j = 0;

	struct cache_index *tmpindex = (struct cache_index*)malloc(index_size);
	memset(tmpindex, '\0', index_size);

	memcpy(tmpindex, Index_Start, index_size);

	/*search in index*/
	while (i <= INDEX_NUM)
	{

		if (tmpindex->fd != 0 && tmpindex->fd == fd)
		{
			return (struct cache_index*)(Index_Start + (i - 1)*index_size);
		}
		if (i >= INDEX_NUM) break;
		memset(tmpindex, '\0', index_size);

		memcpy(tmpindex, Index_Start + i * index_size, index_size);
		i++;
	}


	free(tmpindex);
	return NULL;
}

void release_block(struct cache_block *block)
{
	struct cache_block *tmpblock = (struct cache_block*)malloc(cblock_size);

	memset(tmpblock, '\0', cblock_size);

	memcpy(cbit_map, Bitmap_Start, bitmap_size);
	int num = (block - (struct cache_block*)Block_Start) / cblock_size;
	cbit_map[num] = 0;
	memcpy((int *)Bitmap_Start, cbit_map, bitmap_size);
	memcpy(block, tmpblock, cblock_size);

	free(tmpblock);
}

/*replace happens when block table is full*/
void cache_replace()
{

	struct cache_entry *tmpentry = (struct cache_entry*)malloc(entry_size);
	struct cache_entry *entry[ENTRY_NUM];


	int i = 0;
	int found = -1;
	while (found == -1)
	{
		for (i = 0; i < ENTRY_NUM; i++)
		{
			memset(tmpentry, '\0', entry_size);
			memcpy(tmpentry, Entry_Start + i * entry_size, entry_size);

			if (tmpentry->path[0] != '\0' && tmpentry->a == 0 && tmpentry->m == 0)
			{
				/*replace*/
				found = i;
				break;
			}
		}

		if (found == -1)
		{
			for (i = 0; i < ENTRY_NUM; i++)
			{
				memset(tmpentry, '\0', entry_size);
				memcpy(tmpentry, Entry_Start + i * entry_size, entry_size);

				if (tmpentry->path[0] != '\0' && tmpentry->a == 0)
				{
					/*replace*/
					found = i;
					break;
				}
				else if (tmpentry->path[0] != '\0' && tmpentry->a != 0)
				{
					tmpentry->a = 0;
					memcpy((struct cache_entry*)(Entry_Start + i * entry_size), tmpentry, entry_size);
				}
			}
		}

		if (found != -1)
		{
			memset(tmpentry, '\0', entry_size);
			memcpy(tmpentry, Entry_Start + found * entry_size, entry_size);

			memset(entry, '\0', sizeof(entry));
			find_entry(tmpentry->path, entry);

			int max_a = tmpentry->a;
			int max_m = tmpentry->m;

			if (entry[0] != NULL)
			{
				int m = 0;
				for (m = 0; entry[m] != NULL && (entry[m]->buffer_addr == tmpentry->buffer_addr); m++)
				{
					if (entry[m]->a > max_a)
					{
						max_a = entry[m]->a;
					}
					if (entry[m]->m > max_m)
					{
						max_m = entry[m]->m;
					}

				}
				for (m = 0; entry[m] != NULL && (entry[m]->buffer_addr == tmpentry->buffer_addr); m++)
				{
					entry[m]->a = max_a;
					entry[m]->m = max_m;
				}

			}

			if (max_a == tmpentry->a && max_m == tmpentry->m)
			{
				/*pass*/
			}
			else
			{
				tmpentry->a = max_a;
				tmpentry->m = max_m;
				found = -1;
			}
		}

	}


	memset(tmpentry, '\0', entry_size);
	memcpy(tmpentry, Entry_Start + found * entry_size, entry_size);

	release_block(tmpentry->buffer_addr);

	//tmpentry->offset = 0;
	tmpentry->size = 0;
	tmpentry->buffer_addr = 0x0;
	memcpy((struct cache_entry*)(Entry_Start + found * entry_size), tmpentry, entry_size);

	free(tmpentry);
}

struct cache_block * alloc_block()
{
	int i = 0;
	memcpy(cbit_map, Bitmap_Start, bitmap_size);
	for (; cbit_map[i] == 1; i++);

	/*find an empty block*/
	if (i < Block_Num)
	{
		cbit_map[i] = 1;
		memcpy((int *)Bitmap_Start, cbit_map, bitmap_size);
		return (struct cache_block*)(Block_Start + i * cblock_size);
	}
	/*can't find empty block*/
	/*unfinished*/
	else
	{
		/*exchange happens*/
		cache_replace();
	}
}

struct cache_log * alloc_log()
{

	int fd = open("vcache.log", O_APPEND);
	if (fd < 2)
	{
		mode_t mode = (S_IRUSR | S_IWUSR);
		fd = open("vcache.log", O_CREAT | O_WRONLY, mode);
	}
	int i = 1;
	struct cache_log *tmplog = (struct cache_log*)malloc(log_size);
AGN:memset(tmplog, '\0', log_size);

	memcpy(tmplog, Log_Start, log_size);

	while (i <= LOG_NUM && tmplog->time != 0)
	{
		if (i >= LOG_NUM) break;
		memset(tmplog, '\0', log_size);

		memcpy(tmplog, Log_Start + i * log_size, log_size);
		i++;
	}

	/*find: success*/
	if (tmplog->time == 0)
	{
		free(tmplog);
		close(fd);
		return (struct cache_log*)(Log_Start + (i - 1)*log_size);
	}
	/*find: fail*/
	/******************/
	else
	{
		/*write to log file*/
		int j = 0;
		for (j = 0; j < LOG_NUM; j++)
		{
			memcpy(tmplog, Log_Start + j * log_size, log_size);
			if (tmplog->commit == 1)
			{
				write(fd, tmplog, log_size);
			}
			else
			{
				break;
			}
		}
		goto AGN;
	}

}
