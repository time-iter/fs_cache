#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<time.h>
#include <sys/stat.h>
#include"vcontent.h"
#include"vspace.h"
#include"vdefine.h"
#include"vlog.h"

/*size of nvm data structure*/
/*assignment in file <vspace.c/cpp>*/
extern const int log_size;
extern const int entry_size;
extern const int index_size;

extern const char* Entry_Start;

/*functions provide for content managment*/

struct cache_entry * regist_entry(char *path, int oflag)
{
	struct cache_entry *rtmpentry = (struct cache_entry*)malloc(entry_size);
	memset(rtmpentry, '\0', entry_size);
	struct cache_entry *entry[ENTRY_NUM];
	memset(entry, '\0', sizeof(entry));

	find_entry(path, entry);

	/*find at least one*/
	if (entry[0]->path != NULL)
	{
		/*already exist*/
		if (strcmp(entry[0]->path, path) == 0)
		{
			rtmpentry->r_cnt = entry[0]->r_cnt;
			rtmpentry->w_cnt = entry[0]->w_cnt;
		}
	}

	strncpy(rtmpentry->path, path, (strlen(path) < PATH_LEN ? strlen(path) : PATH_LEN));
	struct cache_entry *tentry;
	tentry = alloc_entry();

	if ((oflag ^ O_RDONLY) == 0 || (oflag ^ O_RDWR) == 0)
	{
		rtmpentry->r_cnt++;
	}
	if ((oflag ^ O_WRONLY) == 0 || (oflag ^ O_RDWR) == 0)
	{
		rtmpentry->w_cnt++;
	}

	memcpy(tentry, rtmpentry, entry_size);

	/*modify entry before the new alloc one*/
	unsigned short rcnt = rtmpentry->r_cnt;
	unsigned short wcnt = rtmpentry->w_cnt;
	int i = 0;

	while (entry[i]->path != NULL)
	{
		memset(rtmpentry, '\0', entry_size);
		memcpy(rtmpentry, entry[i], entry_size);

		if (rtmpentry->r_cnt != rcnt || rtmpentry->w_cnt != wcnt)
		{
			rtmpentry->r_cnt = rcnt;
			rtmpentry->w_cnt = wcnt;
			memcpy(entry[i], rtmpentry, entry_size);
		}

		i++;
	}


	free(rtmpentry);
	return tentry;
}

struct cache_index * regist_index(int fd, struct cache_entry *entry, int oflag)
{
	struct cache_index *ritmpindex = (struct cache_index*)malloc(index_size);
	memset(ritmpindex, '\0', index_size);
	ritmpindex->fd = fd;
	ritmpindex->addr[0] = entry;
	if ((oflag ^ O_RDONLY) == 0 || (oflag ^ O_RDWR) == 0)
	{
		ritmpindex->read = 1;
	}
	if ((oflag ^ O_WRONLY) == 0 || (oflag ^ O_RDWR) == 0)
	{
		ritmpindex->write = 1;
	}
	struct cache_index *index = alloc_index();
	memcpy(index, ritmpindex, index_size);

	free(ritmpindex);
	return index;
}

ssize_t read_to_buff(struct cache_entry *entry, struct cache_index *index)
{
	off_t oldpos;
	/*record pos before pre_read*/
	oldpos = lseek(index->fd, 0, SEEK_CUR);

	struct cache_block *block = alloc_block();
	entry->buffer_addr = block;
	entry->offset = oldpos;

	/***record time*/
	struct stat statbuf;
	struct timespec times[2];

	fstat(index->fd, &statbuf);

	times[0] = statbuf.st_atim;
	times[1] = statbuf.st_mtim;

	/*do sys read*/
	ssize_t rd = read(index->fd, block, cblock_size);

	/* restore time*/
	futimens(index->fd, times);

	/*restore offset*/
	lseek(index->fd, oldpos, SEEK_SET);

	entry->size = rd;

	return rd;
}

ssize_t pre_read(struct cache_index *index, off_t offset)
{
	struct cache_index *prtmpindex = (struct cache_index *)malloc(index_size);
	struct cache_entry *prtmpentry = (struct cache_entry *)malloc(entry_size);
	memset(prtmpentry, '\0', entry_size);
	memset(prtmpindex, '\0', index_size);

	memcpy(prtmpindex, index, index_size);

	struct cache_entry *entry[ENTRY_NUM];
	memcpy(prtmpentry, prtmpindex->addr[0], entry_size);
	find_entry(prtmpentry->path, entry);

	int flag = 0;	/*used to mark whether the buffer exist or not*/
	/*find at least one*/
	if (entry[0]->path != NULL)
	{
		int i = 0;
		while (entry[i]->path != NULL)
		{
			if (entry[i]->offset == offset && entry[i]->buffer_addr != NULL)
			{
				int j = 0;
				/*search in direct address*/
				for (; j < 3; j++)
				{
					if (prtmpindex->addr[j] == NULL)
					{
						/*create a new entry*/
						struct cache_entry *tentry = alloc_entry();
						memset(prtmpentry, '\0', entry_size);

						memcpy(prtmpentry, tentry, entry_size);
						prtmpindex->addr[j] = tentry;
						/*write after modify*/
						memcpy(index, prtmpindex, index_size);

						strncpy(prtmpentry->path, entry[0]->path, strlen(entry[0]->path));
						prtmpentry->r_cnt = entry[0]->r_cnt;
						prtmpentry->w_cnt = entry[0]->w_cnt;
					}
					else
					{
						memset(prtmpentry, '\0', entry_size);
						memcpy(prtmpentry, prtmpindex->addr[j], entry_size);
					}
					if (prtmpentry->buffer_addr == NULL && entry[i] != prtmpindex->addr[j])
					{
						prtmpentry->buffer_addr = entry[i]->buffer_addr;
						prtmpentry->offset = offset;
						prtmpentry->size = entry[i]->size;
						/*write after modify*/
						memcpy(prtmpindex->addr[j], prtmpentry, entry_size);

						free(prtmpindex);
						free(prtmpentry);
						return prtmpentry->size;
					}
				}
#ifdef USE_INA
				/*search in indirect address*/

				if (prtmpindex->iaddr == NULL)
				{
					/*alloc indirect address block*/
					struct cache_block *addr_block = alloc_block();
					prtmpindex->iaddr = addr_block;
					/*write after modify*/
					memcpy(index, prtmpindex, index_size);
				}

				struct cache_block *iaddr_block = (struct cache_block *)malloc(cblock_size);
				memcpy(iaddr_block, prtmpindex->iaddr, cblock_size);
				for (j = 0; j < ENTRY_NUM - 3; j++)
				{
					if (iaddr_block->addr_item[j] == NULL)
					{
						/*create a new entry*/
						struct cache_entry *tentry = alloc_entry();
						memset(prtmpentry, '\0', entry_size);
						memcpy(prtmpentry, tentry, entry_size);
						iaddr_block->addr_item[j] = tentry;

						strncpy(prtmpentry->path, entry[0]->path, strlen(entry[0]->path));
						prtmpentry->r_cnt = entry[0]->r_cnt;
						prtmpentry->w_cnt = entry[0]->w_cnt;
					}

					if (prtmpentry->buffer_addr == NULL)
					{
						prtmpentry->buffer_addr = entry[i]->buffer_addr;
						prtmpentry->offset = offset;
						prtmpentry->size = entry[i]->size;
						/*write after modify*/
						memcpy(iaddr_block->addr_item[j], prtmpentry, entry_size);
						memcpy(prtmpindex->iaddr, iaddr_block, cblock_size);

						free(prtmpindex);
						free(prtmpentry);
						free(iaddr_block);
						return prtmpentry->size;
					}
				}
#endif
			}/*end: if (entry[i]->offset == offset)*/

			i++;
		}/*end: while (entry[i]->path != NULL)*/
	}

	/*not exist in cache*/
	{/*start ELSE*/
		int i = 0;
		/*search in direct address*/
		for (; i < 3; i++)
		{
			if (prtmpindex->addr[i] == NULL)
			{
				/*create a new entry*/
				struct cache_entry *tentry = alloc_entry();
				memset(prtmpentry, '\0', entry_size);
				memcpy(prtmpentry, tentry, entry_size);
				prtmpindex->addr[i] = tentry;
				/*write after modify*/
				memcpy(index, prtmpindex, index_size);

				strncpy(prtmpentry->path, entry[0]->path, strlen(entry[0]->path));
				prtmpentry->r_cnt = entry[0]->r_cnt;
				prtmpentry->w_cnt = entry[0]->w_cnt;
			}
			else
			{
				memset(prtmpentry, '\0', entry_size);
				memcpy(prtmpentry, prtmpindex->addr[i], entry_size);
			}
			/*addr[i] buff empty*/
			if (prtmpentry->buffer_addr == NULL)
			{
				ssize_t rdsize = read_to_buff(prtmpentry, prtmpindex);

				/*write after modify*/
				memcpy(prtmpindex->addr[i], prtmpentry, entry_size);

				free(prtmpindex);
				free(prtmpentry);
				return rdsize;
			}
		}/*end: for (; i < 3; i++)*/


#ifdef USE_INA
		/*search in indirect address*/
		if (prtmpindex->iaddr == NULL)
		{
			/*alloc indirect address block*/
			struct cache_block *addr_block = alloc_block();
			prtmpindex->iaddr = addr_block;

			/*write after modify*/
			memcpy(index, prtmpindex, index_size);
		}

		struct cache_block *iaddr_block = (struct cache_block *)malloc(cblock_size);
		memcpy(iaddr_block, prtmpindex->iaddr, cblock_size);

		for (i = 0; i < ENTRY_NUM - 3; i++)
		{
			if (iaddr_block->addr_item[i] == NULL)
			{
				/*create a new entry*/
				struct cache_entry *tentry = alloc_entry();
				memset(prtmpentry, '\0', entry_size);

				memcpy(prtmpentry, tentry, entry_size);
				iaddr_block->addr_item[i] = tentry;

				strncpy(prtmpentry->path, entry[0]->path, strlen(entry[0]->path));
				prtmpentry->r_cnt = entry[0]->r_cnt;
				prtmpentry->w_cnt = entry[0]->w_cnt;
			}

			if (prtmpentry->buffer_addr == NULL)
			{
				ssize_t rdsize = read_to_buff(prtmpentry, prtmpindex);

				/*write after modify*/
				memcpy(iaddr_block->addr_item[i], prtmpentry, entry_size);
				memcpy(prtmpindex->iaddr, iaddr_block, cblock_size);

				free(prtmpindex);
				free(prtmpentry);
				free(iaddr_block);
				return rdsize;
			}
		}


#endif

	}/*end ELSE*/

	free(prtmpindex);
	free(prtmpentry);
	return -1;	/*index address error or the index hasn't registed*/
}

struct cache_index * verify_fd(int fd, struct cache_index *index)
{
	return index = find_index(fd);
}

int search_cache(struct search_body *s_body, off_t curpos, size_t nbytes)
{
	struct cache_index *index = s_body->index;
	struct cache_entry *entry[ENTRY_NUM];


	memset(s_body->fentry, '\0', ENTRY_NUM * sizeof(struct cache_entry*));

	struct cache_entry *tmpentry = (struct cache_entry*)malloc(entry_size);
	struct cache_index *tmpindex = (struct cache_index*)malloc(index_size);
	int count = nbytes;		/*record buff size when search in index address*/
	off_t pos = curpos;


	memset(tmpindex, '\0', index_size);


	memcpy(tmpindex, index, index_size);
	int i = 0;
	int j = 0;

	int iaddrflag = 0;
	/*search in index address*/
	/*search in direct address*/
	for (; i < 3; i++)
	{
		if (tmpindex->addr[i] != NULL)
		{
			memset(tmpentry, '\0', entry_size);
			memcpy(tmpentry, tmpindex->addr[i], entry_size);
			if ((pos >= tmpentry->offset && pos <= (tmpentry->offset + cblock_size - 1))\
				|| ((pos + count - 1) >= tmpentry->offset && (pos + count - 1) <= (tmpentry->offset + cblock_size - 1))\
				|| (pos < tmpentry->offset && (pos + count) >= (tmpentry->offset + tmpentry->size)))
			{
				/*find all of buff in cache*/
				if ((tmpentry->offset + tmpentry->size) >= (pos + count) && pos >= tmpentry->offset)
				{
					entry[j++] = tmpindex->addr[i];

					memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

					free(tmpindex);
					free(tmpentry);
					return 0;		/*means lack how many bytes in cache*/
				}
				/*lack right*/
				else if (pos >= tmpentry->offset && (pos + count) > (tmpentry->offset + tmpentry->size))
				{
					entry[j++] = tmpindex->addr[i];

					memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

					count = count - (tmpentry->offset + tmpentry->size - pos);

					pos = tmpentry->offset + tmpentry->size;
				}
				/*lack left*/
				else if (pos < tmpentry->offset && (pos + count) <= (tmpentry->offset + tmpentry->size))
				{
					entry[j++] = tmpindex->addr[i];

					memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

					count = count - (pos + count - tmpentry->offset);
				}
				else if (pos < tmpentry->offset && (pos + count) >(tmpentry->offset + tmpentry->size))
				{
					//entry[j++] = tmpindex->addr[i];

					//memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));
					/*not exist*/
				}
			}
		}
		else
		{
			break;
		}

	}
#ifdef USE_INA
	/*search in indirect address*/
	if (tmpindex->iaddr != NULL)
	{
		iaddrflag = 1;
		struct cache_block *iaddr_block = (struct cache_block *)malloc(cblock_size);
		memset(iaddr_block, '\0', cblock_size);
		memcpy(iaddr_block, tmpindex->iaddr, cblock_size);


		for (i = 0; i < ENTRY_NUM - 3; i++)
		{
			if (iaddr_block->addr_item[i] != NULL)
			{
				memset(tmpentry, '\0', entry_size);
				memcpy(tmpentry, iaddr_block->addr_item[i], entry_size);

				if ((pos >= tmpentry->offset && pos <= (tmpentry->offset + cblock_size - 1))\
					|| ((pos + count - 1) >= tmpentry->offset && (pos + count - 1) <= (tmpentry->offset + cblock_size - 1))\
					|| (pos < tmpentry->offset && (pos + count) >= (tmpentry->offset + tmpentry->size)))
				{
					/*find all of buff in cache*/
					if ((tmpentry->offset + tmpentry->size) >= (pos + count) && pos >= tmpentry->offset)
					{
						entry[j++] = iaddr_block->addr_item[i];

						memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

						free(tmpindex);
						free(tmpentry);
						return 0;		/*means lack how many bytes in cache*/
					}
					/*lack right*/
					else if (pos >= tmpentry->offset && (pos + count) > (tmpentry->offset + tmpentry->size))
					{
						entry[j++] = iaddr_block->addr_item[i];

						memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

						count = count - (tmpentry->offset + tmpentry->size - pos);

						pos = tmpentry->offset + tmpentry->size;
					}
					/*lack left*/
					else if (pos < tmpentry->offset && (pos + count) <= (tmpentry->offset + tmpentry->size))
					{
						entry[j++] = iaddr_block->addr_item[i];

						memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

						count = count - (pos + count - tmpentry->offset);
					}
					else if (pos < tmpentry->offset && (pos + count) >(tmpentry->offset + tmpentry->size))
					{
						//entry[j++] = tmpindex->addr[i];

						//memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));
						/*not exist*/
					}
				}
			}
			else
			{
				break;
			}
		}

		free(iaddr_block);
	}

#endif

	/*search all caches*/
	struct cache_entry *findentry[ENTRY_NUM];
	int k = 0;
	int m = 0;
	memset(tmpentry, '\0', entry_size);
	memcpy(tmpentry, tmpindex->addr[0], entry_size);
	find_entry(tmpentry->path, findentry);

	if (findentry[0]->path[0] != '\0')
	{
		k = 0;
		int flag = 0;
		while (findentry[k] != NULL && findentry[k]->path[0] != '\0')
		{
			flag = 0;
			memset(tmpentry, '\0', entry_size);
			memcpy(tmpentry, findentry[k], entry_size);

			for (m = 0; m < j; m++)
			{
				if (findentry[k] == entry[m])
				{
					flag = 1;
					break;
				}
			}

			if (flag == 0)
			{
				if ((pos >= tmpentry->offset && pos <= (tmpentry->offset + cblock_size - 1))\
					|| ((pos + count - 1) >= tmpentry->offset && (pos + count - 1) <= (tmpentry->offset + cblock_size - 1))\
					|| (pos < tmpentry->offset && (pos + count) >= (tmpentry->offset + tmpentry->size)))
				{
					/*find all of buff in cache*/
					if ((tmpentry->offset + tmpentry->size) >= (pos + count) && pos >= tmpentry->offset)
					{
						entry[j++] = findentry[k];

						/*alloc new entry and add it to index*/
						struct cache_entry *newentry = alloc_entry();

						/*iaddr not used*/
						if (iaddrflag == 0)
						{
							if (i < 3)
							{
								tmpindex->addr[i] = newentry;
								memcpy(index, tmpindex, index_size);
							}
							else
							{
								/*alloc indirect address block*/
								struct cache_block *addr_block = alloc_block();
								tmpindex->iaddr = addr_block;
								/*write after modify*/
								memcpy(index, tmpindex, index_size);
								iaddrflag = 1;
							}
						}
#ifdef USE_INA
						if (iaddrflag == 1)
						{
							struct cache_block *iaddr_block = (struct cache_block *)malloc(cblock_size);
							memset(iaddr_block, '\0', cblock_size);
							memcpy(iaddr_block, tmpindex->iaddr, cblock_size);

							int m = 0;
							for (; m < ENTRY_NUM - 3; m++)
							{
								if (iaddr_block->addr_item[m] == NULL)
								{
									iaddr_block->addr_item[m] = newentry;
									break;
								}
							}
							free(iaddr_block);
						}

#endif
						/*copy entry and add it to index*/
						memcpy(newentry, tmpentry, entry_size);

						memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

						free(tmpindex);
						free(tmpentry);
						return 0;		/*means lack how many bytes in cache*/
					}
					else if (pos >= tmpentry->offset && (pos + count) > (tmpentry->offset + tmpentry->size))
					{
						entry[j++] = findentry[k];

						/*alloc new entry and add it to index*/
						struct cache_entry *newentry = alloc_entry();

						/*iaddr not used*/
						if (iaddrflag == 0)
						{
							if (i < 3)
							{
								tmpindex->addr[i] = newentry;
								/*write after modify*/
								memcpy(index, tmpindex, index_size);
							}
							else
							{
								/*alloc indirect address block*/
								struct cache_block *addr_block = alloc_block();
								tmpindex->iaddr = addr_block;
								/*write after modify*/
								memcpy(index, tmpindex, index_size);
								iaddrflag = 1;
							}
						}
#ifdef USE_INA
						if (iaddrflag == 1)
						{
							struct cache_block *iaddr_block = (struct cache_block *)malloc(cblock_size);
							memset(iaddr_block, '\0', cblock_size);
							memcpy(iaddr_block, tmpindex->iaddr, cblock_size);

							int m = 0;
							for (; m < ENTRY_NUM - 3; m++)
							{
								if (iaddr_block->addr_item[m] == NULL)
								{
									iaddr_block->addr_item[m] = newentry;
									break;
								}
							}
							free(iaddr_block);
						}

#endif

						memcpy(newentry, tmpentry, entry_size);

						memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

						count = count - (tmpentry->offset + tmpentry->size - pos);

						pos = tmpentry->offset + tmpentry->size;
					}
					else if (pos < tmpentry->offset && (pos + count) <= (tmpentry->offset + tmpentry->size))
					{
						entry[j++] = findentry[k];

						/*alloc new entry and add it to index*/
						struct cache_entry *newentry = alloc_entry();

						/*iaddr not used*/
						if (iaddrflag == 0)
						{
							if (i < 3)
							{
								tmpindex->addr[i] = newentry;
								/*write after modify*/
								memcpy(index, tmpindex, index_size);
							}
							else
							{
								/*alloc indirect address block*/
								struct cache_block *addr_block = alloc_block();
								tmpindex->iaddr = addr_block;
								/*write after modify*/
								memcpy(index, tmpindex, index_size);
								iaddrflag = 1;
							}
						}
#ifdef USE_INA
						if (iaddrflag == 1)
						{
							struct cache_block *iaddr_block = (struct cache_block *)malloc(cblock_size);
							memset(iaddr_block, '\0', cblock_size);
							memcpy(iaddr_block, tmpindex->iaddr, cblock_size);

							int m = 0;
							for (; m < ENTRY_NUM - 3; m++)
							{
								if (iaddr_block->addr_item[m] == NULL)
								{
									iaddr_block->addr_item[m] = newentry;
									break;
								}
							}
							free(iaddr_block);
						}

#endif

						memcpy(newentry, tmpentry, entry_size);


						memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));

						count = count - (pos + count - tmpentry->offset);
					}
					else if (pos < tmpentry->offset && (pos + count) >(tmpentry->offset + tmpentry->size))
					{
						//entry[j++] = tmpindex->addr[i];

						//memcpy(s_body->fentry, entry, ENTRY_NUM * sizeof(struct cache_entry*));
						/*not exist*/
					}
				}
			}



			k++;
		}
	}

	free(tmpindex);
	free(tmpentry);
	return count;
}

ssize_t direct_2_ubuf(int fd, void *buf, size_t nbytes)
{
	return read(fd, buf, nbytes);
}

ssize_t read_2_ubuf(void *buf, off_t curpos, size_t nbytes, struct search_body *s_body)
{

	int pos = curpos;
	int tmpos = curpos;
	int count = nbytes;
	int tmpcount = nbytes;
	int ccount = 0;

	struct stat statbuf;
	struct timespec times[2];

	struct cache_entry *tmpentry = (struct cache_entry*)malloc(entry_size);
	struct cache_entry *entry[ENTRY_NUM];
	memset(entry, '\0', sizeof(entry));
	memcpy(entry, s_body->fentry, sizeof(entry));
	struct cache_log *log;
	int i = 0;
	int k = 0;
	while (s_body->fentry[i] != NULL)
	{
		memset(tmpentry, '\0', entry_size);
		memcpy(tmpentry, s_body->fentry[i], entry_size);

		log = 0x0;

		tmpos = tmpentry->offset;
		tmpcount = tmpentry->size;

		/*lack left*/
		if (tmpos > pos)
		{
			ccount = (pos + count - tmpos) > tmpcount ? tmpcount : (pos + count - tmpos);
			memcpy(buf + (tmpos - pos), tmpentry->buffer_addr, ccount);

			count -= ccount;

			log = generate_log(entry[i], 0);
#ifdef V_DEBUG
			printf("READ: target hit\n");
			fflush(stdout);
#endif
		}
		else
		{
			ccount = (tmpos + tmpcount - pos) > count ? count : (tmpos + tmpcount - pos);
			memcpy(buf, tmpentry->buffer_addr, ccount);

			pos += ccount;
			count -= ccount;

			log = generate_log(entry[i], 0);
#ifdef V_DEBUG
			printf("READ: target hit\n");
			fflush(stdout);
#endif
		}

		/*accessed*/
		tmpentry->a = 1;
		memcpy(s_body->fentry[i], tmpentry, entry_size);


		memset(entry, '\0', sizeof(entry));

		find_entry(tmpentry->path, entry);

		k = 0;
		while (entry[k] != NULL)
		{
			if (entry[k]->buffer_addr == tmpentry->buffer_addr)
			{
				entry[k]->a = 1;
			}
			k++;
		}

		commit_log(log);

		i++;
	}

	if (count > 0)
	{
		log = 0x0;
		char tmpbuff[count + 1];
		memset(tmpbuff, '\0', sizeof(tmpbuff));

		lseek(s_body->index->fd, pos, SEEK_SET);

		fstat(s_body->index->fd, &statbuf);

		times[0] = statbuf.st_atim;
		times[1] = statbuf.st_mtim;

		ssize_t rd = read(s_body->index->fd, tmpbuff, count);

		lseek(s_body->index->fd, curpos, SEEK_SET);

		if (rd > 0)
		{
			memcpy(buf + (pos - curpos), tmpbuff, rd);
			count -= rd;
		}
		else
		{
			/* restore time*/
			//futimens(s_body->index->fd, times);
		}
	}
	else
	{
		/*modify access time*/;
		times[0].tv_nsec = UTIME_NOW;
		times[1].tv_nsec = UTIME_OMIT;

		futimens(s_body->index->fd, times);
	}

	return nbytes - count;
}

ssize_t write_2_cbuf(const void *buf, off_t curpos, size_t nbytes, struct search_body * s_body)
{

	/*write through*/
	write(s_body->index->fd, buf, nbytes);
	fsync(s_body->index->fd);

	int pos = curpos;
	int tmpos = curpos;
	int count = nbytes;
	int tmpcount = nbytes;
	int ccount = 0;

	struct stat statbuf;
	struct timespec times[2];


	struct cache_entry *tmpentry = (struct cache_entry*)malloc(entry_size);
	struct cache_entry *entry[ENTRY_NUM];
	memset(entry, '\0', sizeof(entry));
	memcpy(entry, s_body->fentry, sizeof(entry));
	struct cache_log *log;

	int i = 0;
	int k = 0;
	/*modify cache*/
	while (s_body->fentry[i] != NULL)
	{
		memset(tmpentry, '\0', entry_size);
		memcpy(tmpentry, s_body->fentry[i], entry_size);

		log = 0x0;

		tmpos = tmpentry->offset;
		tmpcount = tmpentry->size;

		/*lack left*/
		if (tmpos > pos)
		{
			log = generate_log(entry[i], 1);
			ccount = (pos + count - tmpos) > tmpcount ? (pos + count - tmpos) : (pos + count - tmpos);
			memcpy(tmpentry->buffer_addr, buf + (tmpos - pos), ccount);

			count -= ccount;

#ifdef V_DEBUG
			printf("WRITE: target hit\n");
			fflush(stdout);
#endif
		}
		else
		{
			log = generate_log(entry[i], 1);
			ccount = (tmpos + tmpcount - pos) > count ? count : (tmpos + tmpcount - pos);
			//memcpy(tmpentry->buffer_addr, buf, ccount);


			count -= ccount;

			if (count > 0 && tmpcount < cblock_size)
			{
				ccount += (cblock_size - tmpcount) > count ? count : (cblock_size - tmpcount);
				memcpy(tmpentry->buffer_addr + pos - tmpos, buf, ccount);


				pos += ccount;
				count -= ccount;
			}
			pos += ccount;

#ifdef V_DEBUG
			printf("WRITE: target hit\n");
			fflush(stdout);
#endif
		}

		/*modified*/
		tmpentry->m = 1;
		memcpy(s_body->fentry[i], tmpentry, entry_size);

		memset(entry, '\0', entry_size);

		find_entry(tmpentry->path, entry);

		k = 0;
		while (entry[k] != NULL)
		{
			if (entry[k]->buffer_addr == tmpentry->buffer_addr)
			{
				entry[k]->m = 1;
			}
			k++;
		}

		commit_log(log);

		i++;
	}

	return nbytes;
}

ssize_t direct_2_cbuf(int fd, const void *buf, size_t nbytes)
{
	ssize_t wr = write(fd, buf, nbytes);
	fsync(fd);
	return wr;
}

int clean_up(struct cache_index *index)
{
	struct cache_index *tmpindex = (struct cache_index*)malloc(index_size);
	struct cache_entry *tmpentry = (struct cache_entry*)malloc(entry_size);
	struct cache_entry *entry[ENTRY_NUM];

	char path[PATH_LEN] = { '\0' };

	memset(tmpindex, '\0', index_size);


	memcpy(tmpindex, index, index_size);

	memcpy(path, tmpindex->addr[0]->path, PATH_LEN);

	int i = 0;
	int j = 0;

	/*remove entries*/
	for (i = 0; i < 3; i++)
	{
		if (tmpindex->addr[i] != NULL)
		{
			memset(tmpentry, '\0', entry_size);
			memcpy(tmpentry, tmpindex->addr[i], entry_size);

			if (tmpentry->r_cnt > tmpindex->read || tmpentry->w_cnt > tmpindex->write)
			{
				memset(tmpentry, '\0', entry_size);
				memcpy(tmpindex->addr[i], tmpentry, entry_size);
			}
			else
			{
				/*release cache*/
				release_block(tmpentry->buffer_addr);
				memset(tmpentry, '\0', entry_size);
				memcpy(tmpindex->addr[i], tmpentry, entry_size);
			}
		}
		else
		{
			break;
		}

	}

#ifdef USE_INA

	if (tmpindex->iaddr != NULL)
	{
		struct cache_block *iaddr_block = (struct cache_block *)malloc(cblock_size);
		memset(iaddr_block, '\0', cblock_size);
		memcpy(iaddr_block, tmpindex->iaddr, cblock_size);

		for (j = 0; j < ENTRY_NUM - 3; j++)
		{
			if (iaddr_block->addr_item[j] != NULL)
			{
				memset(tmpentry, '\0', entry_size);
				memcpy(tmpentry, iaddr_block->addr_item[j], entry_size);

				if (tmpentry->r_cnt > tmpindex->read || tmpentry->w_cnt > tmpindex->write)
				{
					memset(tmpentry, '\0', entry_size);
					memcpy(iaddr_block->addr_item[j], tmpentry, entry_size);
				}
				else
				{
					/*release cache*/
					release_block(tmpentry->buffer_addr);
					memset(tmpentry, '\0', entry_size);
					memcpy(iaddr_block->addr_item[j], tmpentry, entry_size);
				}
			}
			else
			{
				break;
			}
		}

		/*release inaddr block*/
		release_block(tmpindex->iaddr);

		free(iaddr_block);
	}

#endif


	/*update read and write count of the file*/
	memset(entry, '\0', sizeof(entry));
	find_entry(path, entry);

	for (j = 0; entry[j] != NULL; j++)
	{
		memset(tmpentry, '\0', entry_size);
		memcpy(tmpentry, entry[j], entry_size);
		tmpentry->r_cnt -= tmpindex->read;
		tmpentry->w_cnt -= tmpindex->write;

		memcpy(entry[j], tmpentry, entry_size);
	}

	/*remove index*/
	memset(tmpindex, '\0', index_size);
	memcpy(index, tmpindex, index_size);

	free(tmpindex);
	free(tmpentry);
	return 0;
}


off_t vvlseek(int fd, off_t offset, int whence)
{
	return lseek(fd, offset, whence);
}

int vvcreat(const char *path, mode_t mode)
{
	return creat(path, mode);
}

ssize_t do_read(struct ioqueue *iread, struct cache_index *index, void *buf, size_t nbytes)
{

	// ioqueue *found[MAX_LIST];
	//find_same(tmpindex->fd, 0, found);


	//if (iohead->next == iread)
	{
		struct search_body s_body;
		memset(&s_body, '\0', sizeof(s_body));

		s_body.index = index;

		off_t cur_pos = lseek(index->fd, 0, SEEK_CUR);


		search_cache(&s_body, cur_pos, nbytes);

		if (s_body.fentry[0] != NULL)
		{


			/*read to user buff*/
			ssize_t rdsize;
			rdsize = read_2_ubuf(buf, cur_pos, nbytes, &s_body);

			/*restore offset*/
			lseek(index->fd, cur_pos + rdsize, SEEK_SET);

			return rdsize;
		}
		else
		{

			/*directly read from file*/
			ssize_t rdsize;
			rdsize = direct_2_ubuf(index->fd, buf, nbytes);
			pre_read(index, cur_pos);
			return rdsize;
		}
	}
	return 0;
}

ssize_t do_write(struct ioqueue *owrite, struct cache_index *tmpindex, const void *buf, size_t nbytes)
{
	struct search_body s_body;
	memset(&s_body, '\0', sizeof(s_body));

	s_body.index = tmpindex;

	off_t cur_pos = lseek(tmpindex->fd, 0, SEEK_CUR);


	search_cache(&s_body, cur_pos, nbytes);

	if (s_body.fentry[0] != NULL)
	{

		/*write to cache buff*/
		ssize_t wrsize;
		wrsize = write_2_cbuf(buf, cur_pos, nbytes, &s_body);


		return wrsize;
	}
	else
	{

		/*directly write to file*/
		ssize_t wrsize;
		wrsize = direct_2_cbuf(tmpindex->fd, buf, nbytes);

		return wrsize;
	}
}