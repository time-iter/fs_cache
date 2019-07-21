#pragma once
#include<sys/types.h>


/*func for debugging*/
#define V_DEBUG_PFUNC {printf("\nV_DEBUG_INFO: FUNC <%s> \n",__FUNCTION__);}
#define V_DEBUG_PFILE {printf("\nV_DEBUG_INFO: FILE <%s> \n",__FILE__);}
#define V_DEBUG_PLINE {printf("\nV_DEBUG_INFO: LINE <%d> \n",__LINE__);}



/*functions provide by vcache*/

void sem_init();
void clean();
/*vcreat: usage and parameters are as same as 'creat' in <fcntl.h>*/
int vcreat(const char *path, mode_t mode);

int vopen(const char *path, int oflag);

ssize_t vread(int fd, void *buf, size_t nbytes);

ssize_t vwrite(int fd, const void *buf, size_t nbytes);

off_t vlseek(int fd, off_t offset, int whence);

int vclose(int fd);