#include "global.c"

typedef int (*fp)(void);

int menu();
int cs();
int fork();
int kill();
int quit();
int ls();
int cd();
int pwd();
int mystat();
int make_dir();
int my_creat();
int rm_dir();
int my_chmod();
int my_touch();
int my_chown();
int my_chgrp();
int mylink();
int mysym();
int myunlink();
int ps();

int findCmd(char name[64])
{
	if(strcmp(name, "menu") == 0)
	{
			return 0;
	}
	if(strcmp(name, "cs") == 0)
	{
		return 1;
	}	
	if(strcmp(name, "fork") == 0)
	{
		return 2;
	}
	if(strcmp(name, "kill") == 0)
	{
		return 3;
	}
	if(strcmp(name, "quit") == 0)
	{
		return 4;
	}
	if(strcmp(name, "ls") == 0)
	{
		return 5;
	}
	if(strcmp(name, "cd") == 0)
	{
		return 6;
	}
	if(strcmp(name, "pwd") == 0)
	{
		return 7;
	}
	if(strcmp(name, "stat") == 0)
	{
		return 8;
	}
	if(strcmp(name, "mkdir") == 0)
	{
		return 9;
	}
	if(strcmp(name, "creat") == 0)
	{
		return 10;
	}
	if(strcmp(name, "rmdir") == 0)
	{
		return 11;
	}
	if(strcmp(name, "chmod") == 0)
	{
		return 12;
	}
	if(strcmp(name, "touch") == 0)
	{
		return 13;
	}
	if(strcmp(name, "chown") == 0)
	{
		return 14;
	}
	if(strcmp(name, "chgrp") == 0)
	{
		return 15;
	}
	if(strcmp(name, "link") == 0)
	{
		return 16;
	}
	if(strcmp(name, "symlink") == 0)
	{
		return 17;
	}
	if(strcmp(name, "unlink") == 0)
	{
		return 18;
	}
	if(strcmp(name, "ps") == 0)
	{
		return 19;
	}
	
	return -1;
}

int main(int argc, char *argv[])
{
  int i, cmd; 
  char line[128], cname[64];
  
  //initialiaze array of functions
  
  fp functions[20];
  functions[0] = menu;
  functions[1] = cs;
  functions[2] = fork;
  functions[3] = kill;
  functions[4] = quit;
  functions[5] = ls;
  functions[6] = cd;
  functions[7] = pwd;
  functions[8] = mystat;
  functions[9] = make_dir;
  functions[10] = my_creat;
  functions[11] = rm_dir;
  functions[12] = my_chmod;
  functions[13] = my_touch;
  functions[14] = my_chown;
  functions[15] = my_chgrp;
  functions[16] = mylink;
  functions[17] = mysym;
  functions[18] = myunlink;
  functions[19] = ps;

  init();
  
  if(!mount_root())
  {
	  return 0;
  }
  
  //set running to P0
  running = procs;
      
  while(1){
	//reset parameters
	  memset(line, 0, sizeof(line));
	  memset(cname, 0, sizeof(cname));
	  memset(pathname, 0, sizeof(pathname));
	  memset(parameter, 0, sizeof(parameter));
	  
      printf("P%d running: ", running->pid);
      
      if(running->uid == 0)
      {
		  printf("super: ");
	  }
	  else if(running->uid == 1)
	  {
		  printf("ordinary: ");
	  }
	  
      printf("input command : ");
      fgets(line, 128, stdin);
      line[strlen(line)-1] = 0;  // kill the \r char at end
      if (line[0]==0) continue;

      sscanf(line, "%s %s %64c", cname, pathname, parameter);

      cmd = findCmd(cname); // map cname to an index
      (*functions[cmd])();
  }
}

int stoi(char *str)
{
	
  int           result;
  int           puiss;

  result = 0;
  puiss = 1;
  while (('-' == (*str)) || ((*str) == '+'))
    {
      if (*str == '-')
        puiss = puiss * -1;
      str++;
    }
  while ((*str >= '0') && (*str <= '9'))
    {
      result = (result * 10) + ((*str) - '0');
      str++;
    }
  return (result * puiss);
}

//prints contents of used inodes
int minodep()
{
	  printf("\nMINODES\n");
      int i = 0;
      for(i = 0; i < 100; i++)
      {
		  if(minodes[i].refCount > 0)
		  {
			  printf("M[%d]  ino:%d  ref:%d  dirty:%d\n", i, minodes[i].ino, minodes[i].refCount, minodes[i].dirty);
		 }
	  }
	  
	  printf("\n");
}

//prints contents of the imaps and bmaps
int maps()
{
	int ibg = gp->bg_inode_bitmap;
	int bbg = gp->bg_block_bitmap;
	char ibuf[BLOCK_SIZE];
	char bbuf[BLOCK_SIZE];
	int dev = running->cwd->dev;
	int i;
	
	get_block(dev, ibg, ibuf);
	get_block(dev, bbg, bbuf);
	
	printf("INODES:->\n");
	for(i = 0; i < sp->s_inodes_count; i++)
	{
		//formats line a bit
		if((i % 32) == 0)
		{
			printf("\n");
		}
		
		else if((i % 8) == 0)
		{
			printf("\t");
		}
		
		
		//prints 0 or 1 based on taken or not
		if(tst_bit(ibuf, i) == 0)
		{
			printf("0");
		}
		
		else
		{
			printf("1");
		}
	}
	
	printf("BLOCKS:->\n");
	for(i = 0; i < sp->s_blocks_count; i++)
	{
		//formats line a bit
		if((i % 32) == 0)
		{
			printf("\n");
		}
		
		else if((i % 8) == 0)
		{
			printf("\t");
		}
		
		//prints 0 or 1 based on taken or not
		if(tst_bit(bbuf, i) == 0)
		{
			printf("0");
		}
		
		else
		{
			printf("1");
		}
	}
	
	printf("\n");
}

