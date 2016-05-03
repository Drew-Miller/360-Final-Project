#include "global.c"

//PROCS Functions
int menu(void)
{
	printf("*****************menu*****************\n");
	printf("mkdir\tcreat\tmount\tumount\trmdir\n");
	printf("cd\tls\tpwd\tstat\trm\n");
	printf("link\tunlink\tsymlink\chmod\tchown\ttouch\n");
	printf("open\tpfd\tlseek\trewind\tclose\n");
	printf("read\twrite\tcat\tcp\tmv\n");
	printf("cs\tfork\tps\tkill\tquit\n");
	printf("**************************************\n");
}

//returns the pid of the proc
int cs(void)
{
	int pid = running->pid;
	pid++;
	
	//if we have another ready process, 
	while(pid < (sizeof(procs)/(sizeof(procs[0]))))
	{
		if(procs[pid].status == 1)
		{
			running = &procs[pid];
			return pid + 1;
		}
		
		pid++;
	}
	
	//else return to super proc
	running = &procs;
	return 1;
}

//write fork(). uid and cwd gets carried over from current running proc
int fork(void)
{
	int i = 0;
	while(i < (sizeof(procs)/(sizeof(procs[0]))))
	{
			//find a new proc
			if(procs[i].status == 0)
			{				
				procs[i].uid = running->uid;
				procs[i].pid = i;
				procs[i].cwd = running->cwd;
				procs[i].status = 1;
								
				//return the pid of proc
				return procs[i].pid;
			}
			
			i++;
	}
	
	
	//upon failure
	return -1;
}

//write ps
int ps(void)
{
	int pid = running->pid;
	int initial = pid;
	
	printf("ps: show existing processes\n");
	
	while(pid < (sizeof(procs)/(sizeof(procs[0]))))
	{
		if(procs[pid].status == 1)
		{
			printf("P%d[%d] == > ", procs[pid].pid, procs[pid].uid);
	    }
	    
		pid++;
	}
	
	//loop back to print the first processes if we haven't started at 0
	if(initial != 0)
	{
		int i = 0;
		while(i < initial)
		{
			if(procs[i].status == 1)
			{
				printf("P%d[%d] == > ", procs[i].pid, procs[i].uid);
			}
			
			i++;
		}
	}
	
	printf("\n");
}

//kill
int kill()
{
	int pid = stoi(pathname);
	
	//cant kill original super, ordinary, or the running proc
	if((pid == 0) || (pid == 1) || (running->pid == pid))
	{
		return -1;
	} 
	
	//if process is already dead
	if(procs[pid].status == 0)
	{
		return 0;
	}
	
	//else actually kill process
	else
	{
		procs[pid].status = 0;
		return 1;
	}
}

int quit()
{
	//write all the dirty inodes back
	printf("writing all dirty blocks back\n");
	
	int i = 0;
	while(i < (sizeof(minodes)/(sizeof(minodes[0]))))
	{
		if(minodes[i].dirty == 1)
		{
			int ino = minodes[i].ino;
			int dev = minodes[i].dev;
			int blk, offset;
			blk = ((ino - 1) / 8) + gp->bg_inode_table;
			offset = (ino - 1) % 8;

			char buf[BLOCK_SIZE];

			INODE *oldInode;
			INODE *newInode = &minodes[i].INODE;
			
			//ask about the buf changing contents
			get_block(dev, blk, buf);
			oldInode = (INODE *)buf + offset;

			*oldInode = *newInode;

			put_block(dev, blk, buf);
			
			//clears entry in minodes
			minodes[i] = (MINODE){0};	
		}
		
		i++;	
	}
	
	printf("quitting. goodbye\n");
	exit(0);
}


int ls ()
{
  int ino = running->cwd->ino;
  int dev = running->cwd->dev;
  MINODE *mip = running->cwd;
  
  //if we have a pathname
  if (strcmp(pathname, "") != 0)
  {	  
	  //if we have an absoluate pathname, the device is the root
      if (*pathname == '/')
      {
             dev = root->dev;
	  }
	  	  	 
      ino = getino(&dev, pathname);      
      mip = iget(dev, ino);
  }
   



  int block = 0;
  for(block = 0; block < 15; block++)
  {
	 //use mip to print the datablocks of the inodes
    char dbuf[BLOCK_SIZE];	
  
    get_block(dev, mip->INODE.i_block[block], dbuf);
    dp = (DIR *)dbuf;
    
    while(strlen(dp->name) > 0)
    {
	  //formatting string
      int i = 0;
	  for(i = 0; i < strlen(dp->name); i++)
	  {
		  if(dp->name[i] == 12) { dp->name[i] = '\0'; }
	  }
	
	  //if we find a match
	  printf("%s\t", dp->name);

  	  dp = (DIR *)((char *)dp + (dp->rec_len));
    } 
  }
  
  printf("\n");  
   
   //deallocates the mip that we added for the pathname
   if(!pathname)
   {
	   iput(mip);
   }
   
   return ino;
}

int cd ()
{	
	 //if no path given, go to root
    if (strcmp(pathname, "") == 0)
    {	     
		 MINODE old;
		 
		 old = *(running->cwd);
		 		 
         running->cwd = iget(root->dev, root->ino);
              
         iput(&old);
         
         return root->ino;
    }
	
	//returns -1 on error
	int ino = getino(&(running->cwd->dev), pathname);	
	int dev = running->cwd->dev;
		
	if(ino > 0)
	{
		MINODE *new;
		new = iget(dev, ino);
		
		if(!S_ISDIR(new->INODE.i_mode))
		{
			printf("The requested path is not a directory\n");
		}
		
		//if we do have a directory
		else
		{
			iput(running->cwd);
			running->cwd = new;
		}
	}
	
	else
	{
		printf("path does not exist\n");
	}
			
	return ino;
}

int pwd()
{
	MINODE *minodep;
	minodep = running->cwd;
	

	DIR* dirp;
	int dev = running->cwd->dev;
	int blk, offset;
	int cwd = minodep->ino;
	int iblock = minodep->INODE.i_block[0];
	bool cont = true;
	int past = 0;	
	int bg = gp->bg_inode_table;
	
	char n1[EXT2_NAME_LEN];
	char n2[EXT2_NAME_LEN];

	if(running->cwd->ino == 2)
	{
		printf("/\n");
		return 2;
	}
	
	while(cont)
	{	
		char dbuf[BLOCK_SIZE];
		char ibuf[BLOCK_SIZE];	
					
		//Assume we have the current inode for the directory and the iblock int for the INODE dirs		
		//get the dir pointers
		get_block(dev, iblock, dbuf);	
		
		//move over one to dot dot
		dirp = (DIR *)dbuf;		
		dirp = (DIR *)((char *)dirp + (dirp->rec_len));
		past = cwd;
		cwd = dirp->inode;
								
		//else get the block and offset from the parent block to get the inode
		blk = ((cwd - 1) / 8) + bg;
		offset = (cwd - 1) % 8;
				
		//get parents inode
		get_block(dev, blk, ibuf);
				
		//give us access to the parent block (print names here) and the i_block for the next iteration
		INODE *iptr;
		iptr = (INODE *)ibuf + offset;
		iblock = iptr->i_block[0];
				
		get_block(dev, iblock, dbuf);
		dirp = (DIR *)dbuf;
				
		//since we cannot visit the parent of root, if we are at root lets just take it manually
		if(past == 2)
		{
			printf("%s\n", n2);
			return 1; 
		}
				
		//iterates through all directory entry from the parent to get a name
		while(strlen(dirp->name) > 0)
		{			
			    int i = 0;
				for(i = 0; i < strlen(dirp->name); i++)
				{
					if(dirp->name[i] == 12) { dirp->name[i] = '\0'; }
				}
	
				dirp->name[i] = '\0';
				
				if(dirp->inode == past)
				{		
					char dash[EXT2_NAME_LEN];
					dash[0] = '/';
					dash[1] = '\0';
														
					strncpy(n1, dirp->name, EXT2_NAME_LEN);
					strcat(dash, n1);
					strcat(dash, n2);
					strncpy(n2, dash, EXT2_NAME_LEN);
					
					break;
				}
				
				dirp = (DIR *)((char *)dirp + (dirp->rec_len));
		}
		
		 if(cwd == 0) {break;}
	}
	
	return 0;
}

int mystat()
{
	if(!pathname)
	{
		return 0;
	}
	

	int ino;
	ino = getino(&(running->cwd->dev), pathname);
	
	if(ino < 2)
	{
		return 0;
	}
	
	INODE *inoPtr;
	char ibuf[BLOCK_SIZE];
	int blk, offset;
	int bg = gp->bg_inode_table;
	
	blk = ((ino - 1)/8) + bg;
	offset = ((ino - 1)%8);

	get_block(running->cwd->dev, blk, ibuf);
	inoPtr = (INODE *)ibuf + offset;
	
	printf("MODE:%d\n", inoPtr->i_mode);
	
	printf("Stats: %s\n", pathname);
    printf("Size: \t\t%d bytes\n",inoPtr->i_size);
    printf("Links: \t%d\n",inoPtr->i_links_count);
    printf("inode: \t\t%d\n",ino);
    
	printf("Permissions: \t");
    printf( (S_ISDIR(inoPtr->i_mode)) ? "d" : "-");
    printf( (inoPtr->i_mode & S_IRUSR) ? "r" : "-");
    printf( (inoPtr->i_mode & S_IWUSR) ? "w" : "-");
    printf( (inoPtr->i_mode & S_IXUSR) ? "x" : "-");
    printf( (inoPtr->i_mode & S_IRGRP) ? "r" : "-");
    printf( (inoPtr->i_mode & S_IWGRP) ? "w" : "-");
    printf( (inoPtr->i_mode & S_IXGRP) ? "x" : "-");
    printf( (inoPtr->i_mode & S_IROTH) ? "r" : "-");
    printf( (inoPtr->i_mode & S_IWOTH) ? "w" : "-");
    printf( (inoPtr->i_mode & S_IXOTH) ? "x" : "-");
    printf("\n");
    printf("UID:%d\tGID:%d\n\n", inoPtr->i_uid, inoPtr->i_gid);
    
    return 1;
}


int make_dir()
{	
	char *papa, *base, *c1, *c2;
	
	c1 = strdup(pathname);
	c2 = strdup(pathname);
	papa = dirname(c1);
	base = basename(c2);	
	
	int dev = running->cwd->dev;
	MINODE *pmino;
	int pino = getino(&dev, papa);
	
	if(pino < 0) { return 0; }
	
	pmino = iget(dev, pino);
	
	//check to make sure child doesnt already exist
	DIR *dirp;
	char dbuf[BLOCK_SIZE];
	
	get_block(dev, pmino->INODE.i_block[0], dbuf);
	
	dirp = (DIR *)dbuf;
	
	int block = 0;
	while(block < 15)
	{
		get_block(dev, pmino->INODE.i_block[block], dbuf);
		dirp = (DIR *)dbuf;
		
		while(strlen(dirp->name) > 0)
		{
			//formatting string
			int i = 0;
			for(i = 0; i < strlen(dirp->name); i++)
			{
				if(dirp->name[i] == 12) { dirp->name[i] = '\0'; }
			}
	
			dirp->name[i] = '\0';

			//if we find a match
			if(strcmp(base, dirp->name) == 0)
			{
				printf("The directory already exists\n");
				iput(pmino);
				return dirp->inode;
			}

			dirp = (DIR *)((char *)dirp + (dirp->rec_len));
		} 
		
		block++;
	} 
	
	mymkdir(pmino, base);
	
	pmino->INODE.i_atime = time(0L);
	pmino->INODE.i_links_count++;
	pmino->dirty = 1;
	
	iput(pmino);
	
	return 1;
}


int mymkdir(MINODE *pip, char *name)
{
	int ino, blk, dev;
	dev = pip->dev;
	
	ino = ialloc(dev);
	blk = balloc(dev);
		
	MINODE *mip;
	
	mip = iget(dev, ino);
	mip->dirty = 1;
	
	mip->INODE.i_mode = 0x41ED;		// OR 040755: DIR type and permissions
    mip->INODE.i_uid  = running->cwd->INODE.i_uid;	// Owner uid 
    mip->INODE.i_gid  = running->cwd->INODE.i_gid;	// Group Id
    mip->INODE.i_size = BLOCK_SIZE;		// Size in bytes 
    mip->INODE.i_links_count = 2;	    // Links count=2 because of . and ..
    mip->INODE.i_atime = time(0L);
    mip->INODE.i_ctime = time(0L);
    mip->INODE.i_mtime = time(0L);  
    mip->INODE.i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
    mip->INODE.i_block[0] = blk;             // new DIR has one data block 
    
    int i = 1;
    while( i < 15)
    {
		mip->INODE.i_block[i] = 0;
		i++;
	}
 
    mip->dirty = 1;               // mark minode dirty
    
    //create . and ..
    DIR *dirp;
    char dbuf[BLOCK_SIZE];
    char *dot = ".";
    char *dotdot = "..";
    
    int ideal = 4*((8 + strlen(".") + 3)/4);
    get_block(dev, blk, dbuf);
    dirp = (DIR *)dbuf;
    dirp->inode = ino;
    dirp->rec_len = ideal;
    dirp->name_len = strlen(".");
    strncpy(dirp->name, dot, strlen(dot));
    
    dirp = (DIR *)((char *)dirp + (dirp->rec_len));
    
    dirp->inode = pip->ino;
    dirp->rec_len = BLOCK_SIZE - 12;
    dirp->name_len = strlen("..");
    strncpy(dirp->name, "..", strlen(".."));
    
    put_block(dev, blk, dbuf);    
    enter_name_dir(pip, ino, name);
        
    pip->INODE.i_links_count++;
    pip->INODE.i_atime = time(0L);
    pip->dirty = 1;
    
    iput(mip);                    // write INODE to disk
}


int enter_name_dir(MINODE *pip, int myino, char *name)
{	
	int i = 0;
	int dev = pip->dev;
	char dbuf[BLOCK_SIZE];
	DIR *dirp;
	
	//get to the ith entry where we first hit a 0 for ib[i]
	while(pip->INODE.i_block[i])
	{
		i++;
	}
	
	i--;
	
	//read that block in
	int i_block = pip->INODE.i_block[i];
	get_block(dev, i_block, dbuf);
	
	dirp = (DIR *)dbuf;
	
	//get to the last entry
	//remember, rec_len is the length in the block, the last one has a long rec_len
	int len = BLOCK_SIZE;
	while(len > 0)
	{
		len -= dirp->rec_len;
		dirp = (DIR *)((char *)dirp + (dirp->rec_len));
	}
	
	//now that we hit the last entry, we must now write our added directories content in there
	//we must make sure we do not write past the end of the block
	int need = 4*((8 + strlen(name) + 3)/4);
	int ideal = 4 * ((8 + strlen(dirp->name) + 3) / 4);
	len = dirp->rec_len;
	
	if((len - ideal) >= need)
	{
		//alter the previous dp so that it doesn't have an overly long rec_len
		dirp->rec_len = ideal;
		
		//move to the next entry. this is where we right the new data
		dirp = (DIR *)((char *)dirp + (dirp->rec_len));
		
		//what I do here is keep the previous length from the pervious last dp
		//and subtract the new size of the last entry and set this entry to fill the rest of the block
		dirp->rec_len = len - ideal;
		dirp->name_len = strlen(name);
		strncpy(dirp->name, name, strlen(name));
		dirp->inode = myino;

		//put_block(dev, i_block, dbuf);
	}
	
	else
	{
		//move forward a block, we will not fit in the last one
		i++;
				
		//get a new block number
		int newBlock = balloc(dev);
		pip->INODE.i_block[i] = newBlock;
		//also dont forget to increment the size of the inode one block
		pip->INODE.i_size += BLOCK_SIZE;
		
		//get the new block that our entry will be in
		char newbuf[BLOCK_SIZE];
		get_block(dev, newBlock, newbuf);
		dirp = (DIR *)newbuf;
		
		//now set all the data in the entry
		dirp->rec_len = BLOCK_SIZE;
		dirp->name_len = strlen(name);
		dirp->inode = myino;
		strncpy(dirp->name, name, strlen(name));
		
		put_block(dev, newBlock, newbuf);
	}
}

int my_creat()
{ 
	char *papa, *base, *c1, *c2;
	
	c1 = strdup(pathname);
	c2 = strdup(pathname);
	papa = dirname(c1);
	base = basename(c2);	
	
	int dev = running->cwd->dev;
	MINODE *pmino;
	int pino = getino(&dev, papa);
	
	if(pino < 0) { return 0; }
	
	pmino = iget(dev, pino);
	
	//check to make sure child doesnt already exist

	//if we find a match
	if(search(pmino, base))
	{
		printf("The file already exists\n");
		iput(pmino);
		return 1;
	}

	creatFile(pmino, base);
	
	pmino->INODE.i_atime = pmino->INODE.i_mtime = time(0L);
	pmino->dirty = 1;
	
	iput(pmino);
	
	return 1;
}

int creatFile(MINODE *pip, char *name)
{	
	int ino, blk, dev;
	dev = pip->dev;
	
	ino = ialloc(dev);
	blk = balloc(dev);
		
	MINODE *mip;
	
	mip = iget(dev, ino);
	mip->dirty = 1;
	mip->INODE.i_mode = 0x81A4;		// OR 040755: DIR type and permissions
    mip->INODE.i_uid  = running->cwd->INODE.i_uid;	// Owner uid 
    mip->INODE.i_gid  = running->cwd->INODE.i_gid;	// Group Id
    mip->INODE.i_size = 0;		// Size in bytes 
    mip->INODE.i_links_count = 2;	    // Links count=2 because of . and ..
    mip->INODE.i_atime = time(0L);
    mip->INODE.i_ctime = time(0L);
    mip->INODE.i_mtime = time(0L);  
    mip->INODE.i_blocks = 2; //LINUX: Blocks count in 512-byte chunks 
    
    int i = 0;
    while(i < 15) { mip->INODE.i_block[i] = 0; i++; }
	
	iput(mip);

	enter_name_file(pip, ino, name);
}
 
int enter_name_file(MINODE *pip, int myino, char *name)
{	
	int i = 0;
	int dev = pip->dev;
	char dbuf[BLOCK_SIZE];
	DIR *dirp;
	
	//get to the ith entry where we first hit a 0 for ib[i]
	while(pip->INODE.i_block[i])
	{
		i++;
	}
	
	i--;
	
	//read that block in
	int i_block = pip->INODE.i_block[i];
	get_block(dev, i_block, dbuf);
	dirp = (DIR *)dbuf;
	
	//get to the last entry
	//remember, rec_len is the length in the block, the last one has a long rec_len
	int len = 0;
	while(dirp->rec_len + len < BLOCK_SIZE)
	{
		len += dirp->rec_len;
		dirp = (DIR *)((char *)dirp + (dirp->rec_len));
	}
	
	//now that we hit the last entry, we must now write our added directories content in there
	//we must make sure we do not write past the end of the block
	//get the rec_len of the new block
	int new = 4*((8 + strlen(name) + 3)/4);
	//store the "ideal" rec_len of the old one. we will need it later
	int old = 4 * ((8 + strlen(dirp->name) + 4) / 4);
	//get the current rec_len so we can find out how much we trimmed
	int rec = dirp->rec_len;
		
	//if we can store the new directory here, continue
	if((rec - old) >= new)
	{		
		//alter the previous dp so that it doesn't have an overly long rec_len
		dirp->rec_len = old;
		
		//move to the next entry. this is where we right the new data
		dirp = (DIR *)((char *)dirp + old);
		
		//what I do here is keep the previous length from the pervious last dp
		//and subtract the new size of the last entry and set this entry to fill the rest of the block
		dirp->rec_len = rec - old;
		dirp->name_len = strlen(name);
		strncpy(dirp->name, name, strlen(name));
		dirp->inode = myino;

		put_block(dev, i_block, dbuf);
	}	
	
	else
	{
		//move forward a block, we will not fit in the last one
		i++;
				
		//get a new block number
		int newBlock = balloc(dev);
		pip->INODE.i_block[i] = newBlock;
		//also dont forget to increment the size of the inode one block
		pip->INODE.i_size += BLOCK_SIZE;
		
		//get the new block that our entry will be in
		char newbuf[BLOCK_SIZE];
		get_block(dev, newBlock, newbuf);
		dirp = (DIR *)newbuf;
		
		//now set all the data in the entry
		dirp->rec_len = BLOCK_SIZE;
		dirp->name_len = strlen(name);
		dirp->inode = myino;
		strncpy(dirp->name, name, strlen(name));
		
		put_block(dev, newBlock, newbuf);
	}
}

//returns the parents inode number of an minode
int findParentIno(MINODE *mip)
{
	DIR *dirp;
	char dbuf[BLOCK_SIZE];
	
	
	//read into block
	get_block(mip->dev, mip->INODE.i_block[0], dbuf);
	
	dirp = (DIR *)dbuf;
	
	//move to ..
	dirp = (DIR *)((char *)dirp + dirp->rec_len);
	
	return dirp->inode;
}

int rm_dir()
{
	int dev, i;
	
	//if no path, we do nothing
	if(strlen(pathname) == 0)
	{
		printf("NO PATH GIVEN\n");
		return 0;
	}
	
	//default the device is where we are working
	dev = running->cwd->dev;
	
	//if absolute, we need to set device to root->dev
	if(*pathname == '/')
	{
		dev = root->dev;
	}
	
	//get the inode of the directory to remove
	int ino = getino(&dev, pathname);
	
	//if we do not receive an inode
	if(ino <= 0) { return 0; }
	
	//get the minode of our directory
	MINODE *mcur;
	mcur = iget(dev, ino);
	
	//check if directory or not
	if(!S_ISDIR(mcur->INODE.i_mode))
	{
		printf("The path is not to a directory\n");
		iput(mcur);
		return 0;
	}
	
	//check if busy: WILL have us referencing it
	//so if we have more than one than someone else
	//is too
	if(mcur->refCount > 1)
	{
		printf("DIR is busy\n");
		iput(mcur);
		return 0;
	}
	
	if(empty_dir(mcur))
	{
		printf("DIR is not empty\n");
		iput(mcur);
		return 0;
	}
	
	//if we made it this far, we have a directory to remove
	//clear all of the iblocks
	for(i = 0; i < 12; i++)
	{
		//check if each block is in use
		if(mcur->INODE.i_block[i] != 0)
		{
			//if it IS, dealloc the block number
			bdealloc(mcur->dev, mcur->INODE.i_block[i]);
		}
	}
	
	//dealloc the inode of our directory
	idealloc(mcur->dev, mcur->ino);
	iput(mcur);
	
	int pino = findParentIno(mcur);
	MINODE *pip;
	pip = iget(mcur->dev, pino);
	
	char *removeName;
	removeName = basename(pathname);
	
	rm_child(pip, removeName);
	
	//edit the parent minode
	pip->INODE.i_links_count--;
	pip->INODE.i_atime = pip->INODE.i_mtime = time(0L);
	pip->dirty = 1;
	
	iput(pip);
	
	return 1;
}

int rm_child(MINODE *parent, char *name)
{
	 //go through each of the parents data blocks
     int i = 0;
     for(i = 0; i < 12; i++)
     {
		 //reset the entry count for each data block
		 int entry = 0;
		 
		 //if the block is not empty
		 if(parent->INODE.i_block[i] != 0)
		 {
			 DIR *dirp;
			 char dbuf[BLOCK_SIZE];
			 int totalLength = 0;
			 int prevLength = 0;
			 
			 get_block(parent->dev, parent->INODE.i_block[i], dbuf);
			 
			 //write the datablock to the directory pointer
			 dirp = (DIR *)dbuf;
			 
			 //iterate through all entries of dir
			 while(strlen(dirp->name) > 0)
			 {
				 //write the name in and format it
				 char namebuf[EXT2_NAME_LEN];
				 strncpy(namebuf, dirp->name, strlen(dirp->name));
				 namebuf[strlen(dirp->name)] = 0;
				 
				 //increment total length of the summated entries for this block
				 totalLength += dirp->rec_len;
				 
				 //check if the name is the same as the namebuffer
				 if(strcmp(namebuf, name) == 0)
				 {
					 //if not the first entry in the block
					 if(entry > 0)
					 {
						 //if we are not at the first entry, that means we have to write over entries
						 //or just extend the rec_len so the simulator does not know it is there
						 
						 //if we are at the very LAST entry, this is easy, because
						 //we do not need to shift any entries left
						 if(totalLength == BLOCK_SIZE)
						 {
							 //save the amount of length that we are deleting
							 int entryLen = dirp->rec_len;
							 
							 //move the dirp back ONE entry. (HINT: reason why prevLength is important)
							 dirp = (DIR *)((char *)dirp- prevLength);
							 
							 //add the entry length we removed to the previous entry so the total rec_lens
							 //of all of the entries in the dir are the same
							 dirp->rec_len += entryLen;
							 
							 parent->dirty = 1;
							 
							 //write the new buffer we edited back to the location in the parent's iblocks
							 put_block(parent->dev, parent->INODE.i_block[i], dbuf);
							 
							 return 1;
						 }
						 
						 //else we are not at the end of the block. must shift all entries left. ugh...
						 //save the length of this entry
						 int entryLen = dirp->rec_len;
						 
						 //get the next entry after the one thats deleted
						 DIR *next = (DIR *)((char *)dirp + (dirp->rec_len));
						 
						 //travel through each entry til the end
						 //we know we are at the end if the total length is the blocksize
						 while(totalLength + next->rec_len < BLOCK_SIZE)
						 {
							 totalLength += next->rec_len;
							 
							 int recLen = next->rec_len;
							 
							 //set all the entries from the previous inode to this inode
							 //name, inode, rec_len, name_len
							 strncpy(dirp->name, next->name, strlen(next->name));
							 dirp->inode = next->inode;
							 dirp->rec_len = next->rec_len;
							 dirp->name_len = strlen(next->name);
							 
							 //increment both dir pointers to the next entry.
							 next = (DIR *)((char *)next + recLen);
							 dirp = (DIR *)((char *)dirp + recLen);
						 }
						 
						 //do it one more time since we are at the last entry							 
							 
						 //set all the entries from the previous inode to this inode
						 //name, inode, rec_len, name_len
						 strncpy(dirp->name, next->name, strlen(next->name));
						 dirp->inode = next->inode;
						 
						 //since we took out this much from the entry previous, add it back on here
						 dirp->rec_len = next->rec_len + entryLen;
						 dirp->name_len = strlen(next->name);
						 
						 //write the buffer back
						 put_block(parent->dev, parent->INODE.i_block[i], dbuf);
						 
						 //mark parent as dirty
						 parent->dirty = 1;
						 
						 return 1;
					 }
					 
					 //we are the first entry in the block, deallocate the whole block
					 else
					 {
							//deallocate the block
							bdealloc(parent->dev, parent->INODE.i_block[i]);
							
							//create a block that is empty
							char clear[BLOCK_SIZE];
							//idk if needed, but I thought it might be a good idea to make sure it is all 0s
							memset(clear, 0, BLOCK_SIZE);
							//write that block to the section where the other block was
							put_block(parent->dev, parent->INODE.i_block[i], clear);
							
							//move all i_block entries
							while(i+1 < 12)
							{
								//set the current block to the next block
								parent->INODE.i_block[i] = parent->INODE.i_block[i+1];
								i++;
							}
							
							//last entry needs to be 0 since we cleared a previous entry
							parent->INODE.i_block[i] = 0;
							
							//decrease size and mark dirty;
							parent->INODE.i_size -= BLOCK_SIZE;
							parent->dirty = 1;
							
							return 1;
					 }
				 }
				 
				 //increment the entry number, we keep this because if it is not the first entry
				 //we have to shift all
				 prevLength = dirp->rec_len;
				 entry++;
				 dirp = (DIR *)((char *)dirp + (dirp->rec_len));
			 }
		 } 
	 }
	 
	 return 0;
}


//returns 0 for empty, 1 for not
int empty_dir(MINODE *mip)
{
	//if we have more than two links, we OBVIOUSLY have contents
	if(mip->INODE.i_links_count > 2) { return 0; }
	
	//if we have a link count of two, it is possible we have something here. like files and stuff
	if(mip->INODE.i_links_count == 2)
	{
		int i = 0;
		
		while(i < 12)
		{
			if(mip->INODE.i_block[i] != 0)
			{
				char dbuf[BLOCK_SIZE];
				int dev = mip->dev;
				
				//get the block from the i_block
				get_block(dev, mip->INODE.i_block[i], dbuf);
				
				//read the dbuf into a directory pointer
				DIR *dirp;
				dirp = (DIR *)dbuf;
				
				while(strlen(dirp->name) > 0)
				{
					//check to make sure that the directory does not have
					//the name . or ..
					if(strcmp(dirp->name, ".") != 0)
					{
						if(strcmp(dirp->name, "..") != 0)
						{
								return 1;
						}
					}
					
					dirp = (DIR *)((char *)dirp + (dirp->rec_len));
				}
			}
			
			i++;
		}
		
		//if we get here, we are emtpy, return 0
		return 0;
	}
	
	//else if SOMEHOW our directory has less than two links, it probably should be removed anyways, so return 0;
	else
	{
		return 0;
	}
	
	//should never get here, but why not add a safety check saying it is okay to remove it
	return 0;
}


//uses octal format eg. 777
int my_chmod()
{
	if(strlen(pathname) == 0)
	{
		printf("no path to file\n");
		return 0;
	}
	
	if(strlen(parameter) == 0 || (strlen(parameter) < 3))
	{
		printf("no permissions to edit\n");
		return 0;
	}
	
	int newmode;
	
	//gets mode as decimal
	sscanf(parameter, "%d", &newmode);
		
	MINODE *mip;
	int mino;
	
	mino = getino(&(root->dev), pathname); 
	mip = iget(root->dev, mino);
		
	int filetype = 0;
	
	//if we have a directory, we need to set the file type to the directory type
	if(S_ISDIR(mip->INODE.i_mode))
	{
		filetype = S_IFDIR;
	}
	
	//we have a regular file
	else
	{
		filetype = S_IFREG;
	}
	
	//example, 701 is USER READ WRITE EXEC and OTHER EXECUTE
	//read = 4, write = 2, exec = 1
	//first owner, next group, then other
	//CHECK IF DIR! IF IS ADD DIR BITS
	//ELSE ADD FILE BITS
	//set it to initial file type
	mip->INODE.i_mode = 0;
	mip->INODE.i_mode |= filetype;
	
	//divide the newmode up to find out what is what, then add the decimal version of the permissions
	//CHECK USER READ
	if(newmode - 400 > 0)
	{
		newmode -= 400;
		mip->INODE.i_mode += S_IRUSR;
	}
	
	//check user write
	if(newmode - 200 > 0)
	{
		newmode -= 200;
		mip->INODE.i_mode += S_IWUSR;
	}
	
	//check user execute
	if(newmode - 100 > 0)
	{
		newmode -= 100;
		mip->INODE.i_mode += S_IXUSR;
	}
	
	//CHECK GROUP
	//read write exec
	if(newmode - 40 > 0)
	{
		newmode -= 40;
		mip->INODE.i_mode += S_IRGRP;
	}
	
	if(newmode - 20 > 0)
	{
		newmode -= 20;
		mip->INODE.i_mode += S_IWGRP;
	}
	
	if(newmode - 10 > 0)
	{
		newmode -= 10;
		mip->INODE.i_mode += S_IXGRP;
	}
	
	//CHECK OTHER
	if(newmode - 4 > 0)
	{
		newmode -= 4;
		mip->INODE.i_mode += S_IROTH;
	}
	
	if(newmode - 2 > 0)
	{
		newmode -= 2;
		mip->INODE.i_mode += S_IWOTH;
	}
	
	if(newmode - 1 >= 0)
	{
		newmode -= 1;
		mip->INODE.i_mode += S_IXOTH;
	}
	
	mip->dirty = 1;
	iput(mip);
	
	return newmode;
}

//touches a_time and m_time of a file
int my_touch()
{
	//get the minode of the touched file
	int ino = getino(&(root->dev), pathname);
	
	if(ino <= 0)
	{
			return 0;
	}
	
	MINODE *mip;
	mip = iget(root->dev, pathname);
	
	mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	 
	return 1;
}

//changes owner of file
int my_chown()
{
	//: change INODE.i_uid to newOwner
	if(strlen(pathname) == 0)
	{
		return 0;
	}
	
	if(strlen(parameter) == 0)
	{
		return 0;
	}
	
	int o;
	sscanf(parameter, "%d", &o);
	
	int ino = getino(&(root->dev), pathname);
	
	if(ino <= 0)
	{
		return 0;
	}
	
	MINODE *mip;
	mip = iget(root->dev, ino);
	
	mip->INODE.i_uid = o;
	mip->dirty = 1;
	
	iput(mip);
}
 
//changes group of a file
int my_chgrp()
{
	//: change INODE.i_gid to newGroup
		//: change INODE.i_uid to newOwner
	if(strlen(pathname) == 0)
	{
		return 0;
	}
	
	if(strlen(parameter) == 0)
	{
		return 0;
	}
	
	int g;
	sscanf(parameter, "%d", &g);
	
	int ino = getino(&(root->dev), pathname);
	
	if(ino <= 0)
	{
		return 0;
	}
	
	MINODE *mip;
	mip = iget(root->dev, ino);
	
	mip->INODE.i_gid = g;
	mip->dirty = 1;
	
	iput(mip);
}


//link path1 to a new file in path2
int mylink()
{
	if(strlen(pathname) == 0)
	{
		printf("No original file path given\n");
		return 0;
	}
	
	if(strlen(parameter) == 0)
	{
		printf("No link file path given\n");
		return 0;
	}
	
	//get the device
	int dev = running->cwd->dev;
	
	//if absolute, use root
	if(*pathname == '/')
	{
		dev = root->dev;
	}
	
	//get the original inode
	int oino = getino(&dev, pathname);
	
	if(oino <= 0)
	{
		printf("Path to original does not exist\n");
		return 0;
	}
	
	//get the minode that is the original minode
	MINODE *omip;
	omip = iget(dev, oino);
	
	//since we cannot link a dir, we have to ensure we have a regular file
	if(!S_ISREG(omip->INODE.i_mode))
	{
		printf("Original file to link must be regular\n");
		return 0;
	}
		
	//get the base and dir name of the second path
	char *papa, *base, *c1, *c2;
	
	c1 = strdup(parameter);
	c2 = strdup(parameter);
	papa = dirname(c1);
	base = basename(c2);
	
	int ldirino = getino(&dev, papa);
	
	if(ldirino <= 0)
	{
		printf("Directory to place new file does not exist\n");
		return 0;
	}
	
	MINODE *dmip = iget(dev, ldirino);
	
	if(!S_ISDIR(dmip->INODE.i_mode))
	{
		printf("File to place new linked file is NOT a directory\n");
		return 0;
	}
	
	//get the directory entries from the parent folder so we can check whether or not the file name exists
	//already or not
	int i = 0;

	while(i < 12)
	{
		//if i_mode is being used, we enter this step, otherwise pass on
		if(dmip->INODE.i_block[i])
		{
			DIR *dirp;
			char dbuf[BLOCK_SIZE];
		
			get_block(dmip->dev, dmip->INODE.i_block[i], dbuf);
			dirp = (DIR *)dbuf;
			
			while(strlen(dirp->name) > 0)
			{
				//formatting string
				int i = 0;
				for(i = 0; i < strlen(dirp->name); i++)
				{
					if(dirp->name[i] == 12) { dirp->name[i] = '\0'; }
				}
	
				dirp->name[i] = '\0';

				//if we find a match
				if(strcmp(base, dirp->name) == 0)
				{
					printf("The file already exists\n");
					iput(dmip);
					iput(omip);
					return 0;
				}
				
				
				//go to the next entry
				dirp = (DIR *)((char *)dirp + dirp->rec_len);
			}
		}
		
		i++;
	}
		
	//find the last block being used
	i = 0;		
	while(dmip->INODE.i_block[i])
	{
		i++;
	}
	i--; 
	
	DIR *dirp;
	char dbuf[BLOCK_SIZE];
	
	//get the last block
	get_block(dev, dmip->INODE.i_block[i], dbuf);
	dirp = (DIR *)dbuf;
	int rec_len = 0;
		
	//move to the last entry in the directory entries
	while(dirp->rec_len + rec_len < BLOCK_SIZE)
	{
		rec_len += dirp->rec_len;
		
		dirp = (DIR *)((char *)dirp + dirp->rec_len);
	}
	
	//now we must find the length 
	int need = 4 * ((8 + strlen(base) + 3) / 4);
	int prev = 4 *((8 + strlen(dirp->name) + 3) / 4);
	rec_len = dirp->rec_len;
	
	//if we can still fit the entry in with the previous entry
	if((rec_len - prev) >= need)
	{		
		//set the previous entries rec_len to the new rec_len
		dirp->rec_len = prev;
		
		//move forward to the next entry
		dirp = (DIR *)((char *)dirp + prev);
		
		//allocate data for the new dir pointer. the ino is set to the linked file's inode
		dirp->rec_len = rec_len - prev;
		strncpy(dirp->name, base, strlen(base));
		dirp->name_len = strlen(base);
		dirp->inode = oino;
		
		dirp = (DIR *)((char *)dirp - prev);
		//printf("PREV STUFF:%d:%d:
		
		//put the block back
		put_block(dev, dmip->INODE.i_block[i], dbuf);
	}
	
	//else we need to get a newblock for the parent directory to put the new file in
	else
	{		
		//move to the next datablock
		i++;
		int block = balloc(dmip->dev);
		dmip->INODE.i_block[i] = block;
		
		//increment block_size of the directory minode
		dmip->INODE.i_size += BLOCK_SIZE;
		
		char dbuf[BLOCK_SIZE];
		DIR *mdp;
		
		get_block(dmip->dev, block, dbuf);
		mdp = (DIR *)dbuf;
		
		//set up the new directory entry
		//rec_len is the block_size because this the only entry
		dirp->rec_len = BLOCK_SIZE;
		dirp->name_len = strlen(base);
		strncpy(dirp->name, base, strlen(base));
		dirp->inode = oino;
		
		//put block back
		put_block(dmip->dev, block, dbuf);
	}
	
	//mark dirty, time, increment links, and iput things
	dmip->INODE.i_atime = dmip->INODE.i_mtime = time(0L);
	dmip->dirty = 1;
	
	omip->INODE.i_atime = omip->INODE.i_mtime = time(0L);
	omip->dirty = 1;
	omip->INODE.i_links_count++;
	
	iput(omip);
	iput(dmip);
	
	return 1;
}


//sym links path1 to the sym link of path2
int mysym()
{
	if(strlen(pathname) == 0)
	{
		printf("No original file path given\n");
		return 0;
	}
	
	if(strlen(parameter) == 0)
	{
		printf("No link file path given\n");
		return 0;
	}
	
	//get the device
	int dev = running->cwd->dev;
	
	//if absolute, use root
	if(*pathname == '/')
	{
		dev = root->dev;
	}
	
	//get the original inode
	int oino = getino(&dev, pathname);
	
	if(oino <= 0)
	{
		printf("Path to original does not exist\n");
		return 0;
	}
	
	//get the minode that is the original minode
	MINODE *omip;
	omip = iget(dev, oino);
	
	//since we cannot link a dir, we have to ensure we have a regular file
	if(!S_ISREG(omip->INODE.i_mode))
	{
		printf("Original file to link must be regular\n");
		return 0;
	}
		
	//get the base and dir name of the second path
	char *papa, *base, *c1, *c2;
	
	c1 = strdup(parameter);
	c2 = strdup(parameter);
	papa = dirname(c1);
	base = basename(c2);
	
	int ldirino = getino(&dev, papa);
	
	if(ldirino <= 0)
	{
		printf("Directory to place new file does not exist\n");
		return 0;
	}
	
	MINODE *dmip = iget(dev, ldirino);
	
	if(!S_ISDIR(dmip->INODE.i_mode))
	{
		printf("File to place new linked file is NOT a directory\n");
		return 0;
	}
	
	//get the directory entries from the parent folder so we can check whether or not the file name exists
	//already or not
	int i = 0;

	while(i < 12)
	{
		//if i_mode is being used, we enter this step, otherwise pass on
		if(dmip->INODE.i_block[i])
		{
			DIR *dirp;
			char dbuf[BLOCK_SIZE];
		
			get_block(dmip->dev, dmip->INODE.i_block[i], dbuf);
			dirp = (DIR *)dbuf;
			
			while(strlen(dirp->name) > 0)
			{
				//formatting string
				int i = 0;
				for(i = 0; i < strlen(dirp->name); i++)
				{
					if(dirp->name[i] == 12) { dirp->name[i] = '\0'; }
				}
	
				dirp->name[i] = '\0';

				//if we find a match
				if(strcmp(base, dirp->name) == 0)
				{
					printf("The file already exists\n");
					iput(dmip);
					iput(omip);
					return 0;
				}
				
				
				//go to the next entry
				dirp = (DIR *)((char *)dirp + dirp->rec_len);
			}
		}
		
		i++;
	}
		
	//find the last block being used
	i = 0;		
	while(dmip->INODE.i_block[i])
	{
		i++;
	}
	i--; 
	
	DIR *dirp;
	char dbuf[BLOCK_SIZE];
	
	//get the last block
	get_block(dev, dmip->INODE.i_block[i], dbuf);
	dirp = (DIR *)dbuf;
	int rec_len = 0;
		
	//move to the last entry in the directory entries
	while(dirp->rec_len + rec_len < BLOCK_SIZE)
	{
		rec_len += dirp->rec_len;
		
		dirp = (DIR *)((char *)dirp + dirp->rec_len);
	}
	
	//now we must find the length 
	int need = 4 * ((8 + strlen(base) + 3) / 4);
	int prev = 4 *((8 + strlen(dirp->name) + 3) / 4);
	rec_len = dirp->rec_len;
	int newIno = ialloc(dev);
	
	//if we can still fit the entry in with the previous entry
	if((rec_len - prev) >= need)
	{		
		//set the previous entries rec_len to the new rec_len
		dirp->rec_len = prev;
		
		//move forward to the next entry
		dirp = (DIR *)((char *)dirp + prev);
		
		//allocate data for the new dir pointer. the ino is set to the linked file's inode
		dirp->rec_len = rec_len - prev;
		strncpy(dirp->name, base, strlen(base));
		dirp->name_len = strlen(base);
		dirp->inode = newIno;
		
		dirp = (DIR *)((char *)dirp - prev);
		//printf("PREV STUFF:%d:%d:
		
		//put the block back
		put_block(dev, dmip->INODE.i_block[i], dbuf);
	}
	
	//else we need to get a newblock for the parent directory to put the new file in
	else
	{		
		//move to the next datablock
		i++;
		int block = balloc(dmip->dev);
		dmip->INODE.i_block[i] = block;
		
		//increment block_size of the directory minode
		dmip->INODE.i_size += BLOCK_SIZE;
		
		char dbuf[BLOCK_SIZE];
		DIR *mdp;
		
		get_block(dmip->dev, block, dbuf);
		mdp = (DIR *)dbuf;
		
		//set up the new directory entry
		//rec_len is the block_size because this the only entry
		dirp->rec_len = BLOCK_SIZE;
		dirp->name_len = strlen(base);
		strncpy(dirp->name, base, strlen(base));
		dirp->inode = newIno;
		
		//put block back
		put_block(dmip->dev, block, dbuf);
	}
	
	//mark dirty, time, increment links, and iput things
	dmip->INODE.i_atime = dmip->INODE.i_mtime = time(0L);
	dmip->dirty = 1;
	
	omip->INODE.i_atime = omip->INODE.i_mtime = time(0L);
	omip->dirty = 1;
	omip->INODE.i_links_count++;
	
	//get the new inode back
	MINODE *sym;
	sym = iget(dev, newIno);
	
	//change the mode to symbolic link
	sym->INODE.i_mode = 0xA1FF;
	
	//is the relative path possible...too bad for now
	strcpy((char *)(sym->INODE.i_block), pathname);
	
	//since i_blocks are used for ints usually
	//but they are used to hold the name of the symlinc
	//in this inode, so the size is the length of the path
	sym->INODE.i_size = strlen(pathname);
	sym->dirty = 1;
	
	//put back all minodes being used
	iput(sym);
	iput(omip);
	iput(dmip);
	
}


char *myreadlink()
{
	printf("in read:%s\n", pathname);
	
	int dev = running->cwd->ino;
	
	if(*pathname == '/')
	{
		dev = root->dev;
	}
	
		printf("in read:%s\n", pathname);
	
	char *papa, *base;
	papa = dirname(pathname);
	base = basename(pathname);
	
	printf("p:%s\nb%s\n", papa, base);
	
	int ino = getino(&dev, papa);
	
		printf("in read:%s\t%s\nino:%d\n", base, papa, ino);

	
	if(ino <= 0)
	{
			printf("filepath does not exist\n");
			return 0;
	}
	
	
	printf("PATH:%s\tino:%d\n", pathname, ino);
	
	//get minode from path
	MINODE *mip = iget(dev, ino);
	
	printf("before\n");
	//if we have a symlink
	if(mip->INODE.i_mode == 0xA1FF)
	{
		printf("SYMLINK TO:%s\n", (char *)mip->INODE.i_block);
		return (char *)mip->INODE.i_block;
	}
	
	
	else
	{
		printf("Not a symlink\n");
		return 0;
	}
	
	return 0;
}


//removes or unlinks a file
int myunlink()
{
	//if no pathname given
	if(pathname[0] == 0)
	{
		printf("no pathname given\n");
	}
	
	//get the proper dev
	int dev = running->cwd->dev;
	if(*pathname == '/')
	{
		dev = root->dev;
	}
	
	int ino = getino(&dev, pathname);
	
	if(ino <= 0)
	{
		return 0;
	}
	
	MINODE *mip;
	mip = iget(dev, ino);
	
	//make sure were dealing with an actual file and not a dir
	if(S_ISDIR(mip->INODE.i_mode))
	{
		printf("The path requested is not a directory\n");
		iput(mip);
		return 0;
	}
	
	//special case for symlinks since they do not use datablocks
	int sym = 0;
	
	if(mip->INODE.i_mode == 0xA1FF)
	{
		sym = 1;
		iput(mip);
	} 
	
	//decrement the link count by 1
	mip->INODE.i_links_count--;
		
	//if we have no links to this file, we can remove all of the datablocks from the file
	if(mip->INODE.i_links_count == 0)
	{		
		//bdealloc all blocks
		if(!sym)
		{
			truncate(&(mip->INODE), dev);
		}
		
		idealloc(dev, mip->ino);
	}
		
	//mark it dirty
	mip->dirty = 1;
	iput(mip);
	
	//get the base file name and the parent dir name
	char *base, *papa;
	base = basename(pathname);
	papa = dirname(pathname);
		
	//get the parent's inode
	MINODE *pip;
	int pino = getino(&dev, papa);
	pip = iget(dev, pino);
	
	//remove file
	rm_child(pip, base);
	
	//change parent directory's stats
	pip->INODE.i_atime = pip->INODE.i_mtime = time(0L);
	pip->dirty = 1;
	iput(pip);
	
	return 1;
}

//bdeallocs all direct blocks in an inode
int truncate(INODE *ino, int dev)
{
		int i = 0;
		while(i < 14)
		{
			//direct blocks
			if(i < 12)
			{
				//deallocate the i_block directly
				if(ino->i_block[i] != 0)
				{
					bdealloc(dev, ino->i_block[i]);
				}
			}
			
			//indirect
			if(i == 12)
			{
				if(ino->i_block[12] != 0)
				{
					int *p;
					char ibuf[BLOCK_SIZE];
					
					//get the block which contains pointers to block numbers
					get_block(dev, ino->i_block[12], ibuf);
					//point p to the start. for every ACTUAL integer, we deallocate that block
					p = (int *)ibuf;
					
					int count = 0;
					//there is a max possible of 256 blocks pointed to in each indirect block.
					while(count < 256)
					{
						//if p contains an integer, deallocate it
						if(*p != 0)
						{
							bdealloc(dev, *p);
						}
						
						count++;
						p++;
					}
				}
			}
			
			
			//double indirect
			if(i == 13)
			{
				//here we are with a block that can contain 256 indirect blocks which EACH can contain another 256 direct blocks
				if(ino->i_block[13] != 0)
				{
					int *p1;
					char iibuf[BLOCK_SIZE];
					
					//read the double indirect block into a buffer
					get_block(dev, ino->i_block[13], iibuf);
					p1 = (int *)iibuf;
					
					//go through each indirect block in the double indirect block
					int count1 = 0;
					while(count1 < 256)
					{
						//if we have an indirect block, go through each direct block in there
						if(*p1 != 0)
						{
							int *p2;
							char ibuf[BLOCK_SIZE];
							
							get_block(dev, *p1, ibuf);
							p2 = (int *)ibuf;
							
							int count2 = 0;
							while(count2 < 256)
							{
								if(*p2 != 0)
								{
									bdealloc(dev, *p2);
								}
								
								p2++;
								count2++;
							}
						}
						
						p1++;
						count1++;
					}
				}
			}
			
			i++;
		}
		
		return 1;
}



