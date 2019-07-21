#pragma once

#define PATH_LEN 256
#define INDEX_NUM 512
#define ENTRY_NUM 512
#define LOG_NUM 512
#define BIT_MAP 512
#define MAX_LIST 128

#define USE_INA  /*if defined, indirect address will be used*/
#define V_DEBUG  /*if defined, all debug info will print to screen*/

const int cblock_size = 1024 * 1024; /*cache block size is 1MB*/
