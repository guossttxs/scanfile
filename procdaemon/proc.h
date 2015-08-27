#ifndef PROC_H
#define PROC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

/*监控的目录*/
#define PATH_NAME "/home/usr"
#define DEME_LOGNAME "/tmp/demeon.log"
#define PROC_LOGNAME "/home/guoss/filechange.log"
/*定义存储监控目录下的文件结构体*/
struct file_attr
{
     char filename[102];
     struct stat file_stat;
	 int file_exist;
};
typedef struct file
{
     struct file_attr data;
     struct file *next;
}FILE_S;


/*函数声明*/
void daemonize();
void sys_err(char*);
char *gettime();
void w_write();
void scan_dir(char *,void(*)(char *));
void scan_file(char *);
void monitoring();
void fileListinit();
FILE_S* fileListadd(struct file_attr);
void fileListdel(char *);
int fileListfind(char*);
void fileList_DELfind();
#endif
