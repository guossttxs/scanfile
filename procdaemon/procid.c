#include "proc.h"

/*链表头节点*/
FILE_S * head;

/*errno错误处理函数*/
void sys_err(char *str)
{
	perror(str);
	exit(-1);
}

/*获取时间函数*/
char * gettime()
{
	time_t t;
	time(&t);
	return ctime(&t); 
}

/*向守护目录的日志中写入日志*/
void w_write(int num,char* name)
{
	char buf[1024];
	switch (num)
	{
		case 1:
			sprintf(buf,"%s: creat a new file--%s\n",gettime(),name);
			break;
		case 2:
			sprintf(buf,"%s: del a file--%s\n",gettime(),name);
			break;
		case 3:
			sprintf(buf,"%s: file mtime attr  is changed--%s\n",gettime(),name);
		case 4:
			sprintf(buf,"%s: file ctime attr  is changed--%s\n",gettime(),name);
			break;
		default:
			break;
	}
	/*向日志中写入操作*/
	int fd = open(PROC_LOGNAME,O_WRONLY|O_CREAT|O_APPEND,0777);
	if(fd == -1)
	{
		sys_err("open");
	}
	write(fd,buf,strlen(buf)+1);
	close(fd);
}


/*定义存储监控目录下文件的链表*/
/*链表初始化*/
void fileListinit()
{
	head = (FILE_S*)malloc(sizeof(FILE_S));
	if(head == NULL)
	{
		printf("head NULL");
		return ;
	}
	head->next = NULL;
}

/*链表插入数据*/
FILE_S* fileListadd(struct file_attr buf)
{
	FILE_S * cur = (FILE_S*)malloc(sizeof(FILE_S));
	if(cur == NULL)
	{
		return NULL;
	}
	cur->data = buf;
	cur->next = head->next;

	head->next = cur;
	return head;
}

#if 1
/*链表删除数据*/
void fileListdel(char* name)
{
	FILE_S* cur = head->next;
	FILE_S* ptr = head;
	while(cur->next != NULL)
	{
		if(strcmp(name,cur->data.filename) == 0)
		{
			ptr->next = cur->next;
			free(cur);
			return ;
		}
		cur = cur->next;
		ptr = ptr->next;
	}
	return ;
}
#endif

/*链表查找数据并判断数据属性*/
int fileListfind(char* name)
{
	FILE_S* cur = head;
	cur = cur->next;
	while(cur != NULL)
	{
		if(strcmp(name,cur->data.filename) == 0)
		{
			cur->data.file_exist = 1;
			struct stat buf;
			stat(name,&buf);
			if((((buf.st_mtime)!=(cur->data.file_stat.st_mtime))))
			{
				cur->data.file_stat.st_mtime = buf.st_mtime;
				w_write(3,name);
				return 0;
			}
			else if((((buf.st_ctime)!=(cur->data.file_stat.st_ctime))))
			{
				cur->data.file_stat.st_ctime = buf.st_ctime;
				w_write(4,name);
				return 0;
			}
			else
			{
				return 0;
			}

		}
		cur = cur->next;
	}
	w_write(1,name);
	return 2;
}

/*通过exist的值来判断文件是否被删除,若变历的时候为0,表示没有监控到,也就是被删除*/
void fileList_DELfind()
{
	FILE_S* cur = head;
	cur = cur->next;
	while(cur != NULL)
	{
		if(cur->data.file_exist == 1)
		{
			cur->data.file_exist = 0;
		}
		else
		{
			w_write(2,cur->data.filename);
			fileListdel(cur->data.filename);
		}
		cur = cur->next;
	}
	return ;
}


/*扫描目录下文件,并将文件添加到结构体中*/
/*如果是目录就遍历*/
void scan_dir(char* path,void(*scandir)(char *))
{
	DIR *dir;
	struct dirent * ret;
	char name[1024];

	dir = opendir(path);
	if(dir == NULL)
	{
		sys_err("opendir");
	}
	while((ret = readdir(dir))!=NULL)
	{
		if(strcmp(ret->d_name,".")==0 || strcmp(ret->d_name,"..")==0)
		{
			continue;
		}
		else
		{
			sprintf(name,"%s/%s",path,ret->d_name);
			(*scandir)(name);
		}
	}
	closedir(dir);
}

/*文件变历加到链表*/
void scan_file(char* path)
{
	struct stat buf;
	struct file_attr attr;
	int sf = stat(path,&buf);
	if(sf == -1)
	{
		sys_err("stat");
	}
	if((buf.st_mode & S_IFMT) == S_IFDIR)
	{
		scan_dir(path,scan_file);
	}
	/*便利文件添加到链表中*/
	strcpy(attr.filename,path);
	attr.file_stat = buf;
	attr.file_exist = 0;
	fileListadd(attr);
}

/*在文件变历加到链表后，开始监控目录*/
void scan_file_dir(char* path)
{
	struct stat buf;
	struct file_attr attr;
	int sf = stat(path,&buf);
	if(sf == -1)
	{
		sys_err("stat");
	}
	if((buf.st_mode & S_IFMT) == S_IFDIR)
	{
		scan_dir(path,scan_file_dir);
	}
	/*如果文件在链表中不存在就添加到fileList中*/
	//	printf("%s\n",path);
	if((fileListfind(path)) == 2)
	{
		strcpy(attr.filename,path);
		attr.file_stat = buf;
		attr.file_exist = 1;
		fileListadd(attr);
	}
}

/*监控目录函数*/
void monitoring()
{
	sleep(5);
	scan_file_dir(PATH_NAME);
	fileList_DELfind();
}

/*守护进程函数*/
void daemonize()
{
	pid_t pid;

	/*创建子进程,并退出主进程*/
	pid = fork();
	if(pid < 0)
	{
		sys_err("fork");
	}
	else if(pid > 0)
	{
		exit(1);
	}
	/*子进程生成新会话*/
	setsid();
	/*将进程的工作目录改到/目录下面*/
	if((chdir("/")) < 0)
	{
		sys_err("chdir");
	}
	/*更改umask的值,防止进程创建的文件受到权限限制*/
	umask(0);
	/*因为会话被改变,所以已经失去控制终端,文件描述符也就失去意义*/
	/*将0 和 1 指向/dev/null*/
	close(0);
	open("/dev/null",O_RDWR);
	dup2(0,1);
	dup2(0,2);
}

int main()
{
	fileListinit(head);
	scan_file(PATH_NAME);

	daemonize();
	while(1)
	{
		/*守护进程工作*/
		/*监控文件目录操作*/
		monitoring();
	}

	return 0;
}
