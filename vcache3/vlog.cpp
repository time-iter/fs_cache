#include<time.h>
#include<stdlib.h>
#include<string.h>
#include"vspace.h"
#include"vlog.h"

/*size of nvm data structure*/
/*assignment in file <vspace.c/cpp>*/
extern const int log_size;
extern const int entry_size;
extern const char* Entry_Start;
extern const char* Log_Start;


struct cache_log *generate_log(struct cache_entry *entry, int type)
{
	struct cache_log *log = alloc_log();
	struct cache_log *tmplog = (struct cache_log*)malloc(log_size);
	memset(tmplog, '\0', log_size);
	time_t now;
	time(&now);

	if (log != NULL)
	{
		tmplog->time = (long)now;
		if (type == 0)
		{
			tmplog->read = 1;
		}
		else
		{
			tmplog->write = 1;
		}
		tmplog->buffer_bf = entry->buffer_addr;
		tmplog->buffer_af = entry->buffer_addr;

		tmplog->entry_num = (entry - (struct cache_entry*)Entry_Start) / entry_size;

	}
	else
	{
		return NULL;
	}

	if (tmplog->write == 1)
	{

		struct cache_block *buf_addr = entry->buffer_addr;
		struct cache_block *block = alloc_block();
		if (block != NULL && buf_addr != block)
		{
			memcpy(block, entry->buffer_addr, cblock_size);
			tmplog->buffer_bf = block;
			tmplog->buffer_af = entry->buffer_addr;
		}
	}

	memcpy(log, tmplog, log_size);

	free(tmplog);
	return log;
}

int commit_log(struct cache_log *log)
{
	return log->commit = 1;
}

void recovery()
{
	int i = 0;
	struct cache_log *log = (struct cache_log *)malloc(log_size);

	for (i = 0; i < LOG_NUM; i++)
	{
		memset(log, '\0', log_size);
		memcpy(log, Log_Start + i * log_size, log_size);


		if (log->time != 0 && log->commit == 0)
		{
			/*need recovery*/
			if (log->write == 1 && log->buffer_af != log->buffer_bf)
			{
				struct cache_entry *entry = (struct cache_entry*)malloc(entry_size);
				memset(entry, '\0', entry_size);
				memcpy(entry, Entry_Start + entry_size * log->entry_num, entry_size);

				struct cache_block *block = (struct cache_block*)malloc(cblock_size);
				memset(block, '\0', cblock_size);
				memcpy(block, log->buffer_bf, cblock_size);
				memcpy(log->buffer_af, block, cblock_size);

				log->commit = -1;

				memcpy((struct cache_log*)(Log_Start + i * log_size), log, log_size);

				free(entry);
				free(block);
			}
			/*commit but not recovery*/
			else
			{
				commit_log((struct cache_log*)(Log_Start + i * log_size));
			}
		}
	}

	free(log);
}


void block_recovery(struct cache_entry *entry)
{
	int entry_num = (entry - (struct cache_entry*)Entry_Start) / entry_size;

	int i = 0;
	struct cache_log *log = (struct cache_log *)malloc(log_size);
	int found = 0;

	for (i = 0; i < LOG_NUM; i++)
	{
		memset(log, '\0', log_size);
		memcpy(log, Log_Start + i * log_size, log_size);

		if (log->entry_num == entry_num)
		{
			found = (log - (struct cache_log*)Log_Start) / log_size;
		}
	}

	{
		memset(log, '\0', log_size);
		memcpy(log, Log_Start + found * log_size, log_size);


		if (log->time != 0 && log->commit == 0)
		{
			/*need recovery*/
			if (log->write == 1 && log->buffer_af != log->buffer_bf)
			{
				struct cache_entry *entry = (struct cache_entry*)malloc(entry_size);
				memset(entry, '\0', entry_size);
				memcpy(entry, Entry_Start + entry_size * log->entry_num, entry_size);

				struct cache_block *block = (struct cache_block*)malloc(cblock_size);
				memset(block, '\0', cblock_size);
				memcpy(block, log->buffer_bf, cblock_size);
				memcpy(log->buffer_af, block, cblock_size);

				log->commit = -1;

				memcpy((struct cache_log*)(Log_Start + i * log_size), log, log_size);

				free(entry);
				free(block);
			}
			/*commit but not recovery*/
			else
			{
				commit_log((struct cache_log*)(Log_Start + i * log_size));
			}
		}
	}

	free(log);

}