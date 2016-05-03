#include "global.c"

//initiates system
int init(void)
{
	//(1). 2 PROCs, P0 with uid=0, P1 with uid=1, all PROC.cwd = 0
	//status = 1 because they are ready to run
	PROC P0, P1;
	procs[0].uid = 0;
	procs[0].pid = 0;
	procs[0].cwd = 0;
	procs[0].status = 1;
	
	procs[1].uid = 1;
	procs[1].pid = 1;
	procs[1].cwd = 0;
	procs[1].status = 1;
	
	//set all unused procs to status 0
	int i = 2;
	while(i < sizeof(procs)/(sizeof(procs[0]))) 
	{
		procs[i].status = 0;
		i++;
	}

	running = &procs[0];
	
	//(2). MINODE minode[100]; all with refCount=0
    int n = sizeof(minodes)/sizeof(minodes[0]);
    i = 0;
    while(i < n)
	{
		minodes[i].INODE = (INODE){0};
		minodes[i].dev = 0;
		minodes[i].ino = 0;
		minodes[i].refCount = 0;
		minodes[i].dirty = 0;
		minodes[i].mounted = 0;
		minodes[i].mountptr = NULL;
		
		i++;
	}
	
	//(3). MINODE *root = 0;
    root = NULL; 
}

//mounts filesystem to root
int mount_root(void)
{
	/*
	  open device for RW
      read SUPER block to verify it's an EXT2 FS
   
      root = iget(dev, 2);    get root inode
    
      Let cwd of both P0 and P1 point at the root minode (refCount=3)
          P0.cwd = iget(dev, 2); 
          P1.cwd = iget(dev, 2);
	 */
	 
	 printf("mounting root\n");
	 printf("enter rootdev name:  ");

	 //gets path
	 char device[128];
	 fgets(device, sizeof(device), stdin);	 

	 int fd;	 
	 device[strlen(device) - 1] = 0;
	 fd = open(device, O_RDWR);
	 
	 //upon opening
	 if(fd > -1)
	 {
		 //check magic number
		 char sbuf[BLOCK_SIZE];
		 get_block(fd, 1, sbuf);
		 sp = (SUPER *)sbuf;
		 
		 char gbuf[BLOCK_SIZE];
		 get_block(fd, 2, gbuf);
		 gp = (GD *)gbuf;
		 
		 if(sp->s_magic == 0xef53)
		 {
			 printf("mount : %s mounted on /\n", device);
			 printf("SUPER magic=0x%0x\t", sp->s_magic);
			 printf("bmap=%0x\t", gp->bg_block_bitmap);
			 printf("imap=%0x\t", gp->bg_inode_bitmap);
			 printf("iblock=%0x\t", gp->bg_inode_table);
		 }
		 
		 else
		 {
			 printf("file system is not ext2. exiting...\n");
			 return 0;
		 }
		 
		 printf("\n");
		 
		 printf("mounted root\n");
		 root = iget(fd, 2);
		 root->refCount--;
		 
		 //create p0 p1
		 procs[0].cwd = iget(fd, 2);
		 procs[1].cwd = iget(fd, 2);
		 
		 printf("creating P0 super, and P1 user\n");
		 		 
		 return 1;
	 }
	 
	 //didnt open
	 else
	 {
		 printf("panic : can't open root device\n");
		 return 0;
	 }
}
