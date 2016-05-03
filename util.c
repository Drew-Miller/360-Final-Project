#include "global.c"
//NOTES:

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd,(long)blk*BLOCK_SIZE, 0);
   read(fd, buf, BLOCK_SIZE);
}

int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd,(long)blk*BLOCK_SIZE, 0);
   write(fd, buf, BLOCK_SIZE);
}

//allows the user to pass in delimiter and an array to populate with the tokens
void tokenize(char *s, char *delim, char *array[])
{
	char* token = strtok(s, delim);

	int i = 0;

	while (token != NULL) 
	{
		array[i] = malloc(sizeof(token));
		strcpy(array[i],token);

    		token = strtok(NULL, delim);
		i++;
	}

	array[i] = '\0';
}

//passes in a device and returns the inode of the pathname requested
//NOTE: MAKE IT WORK FROM CWD
int getino(int *dev, char *pathname)
{
	char buf[BLOCK_SIZE];

	char gbuf[BLOCK_SIZE];
	get_block(*dev, 2, gbuf);
	gp = (GD *)gbuf;

	//if the pathname given is only root
	if(strcmp(pathname, "/") == 0)
	{
		return (gp->bg_inode_table - 8);
	}
	
	if(strcmp(pathname, "") == 0)
	{
		return (gp->bg_inode_table - 8);
	}
	
	//if the path is absolute, we set ip2 to the root instead of the cwd
	if(*pathname == '/')
	{
		//gets root inode block from ibg
		get_block(*dev, gp->bg_inode_table, buf);
		ip = (INODE *)buf + 1;
	}
	
	//else, use current directory
	else
	{
		//sets ip to cwd INODE
		ip = &running->cwd->INODE;
	}

	//create a copy of the array so we can use it in the tokenizer to be destroyed
	char *cpypath = malloc(strlen(pathname) + 1);
	strcpy(cpypath, pathname);

	//split the pathname into an array
	char *name[64];
	char *delim = "/";

	tokenize(cpypath, delim, name);

	//go through and find the correct items and return the inode
	int i = 0;
	int finalNode = -1;

	//this loop is iterated for each entry in the name array
	//we will first find out if we contain the name as a directory entry
	//if not, we go through all of the contains of the current directory to see if we have the name there
	//if we find the name, we then move to find the next name
	//if not, return -1
	bool found = true;
	
	while(name[i] && found)
	{
		found = false;
		char dbuf[BLOCK_SIZE];

		//if there is another entry in the name array, we HAVE to be in a directory
		if(!S_ISDIR(ip->i_mode))
		{
			return -1;
		}
		
		int block = 0;
		while(block < 15)
		{

			//write the start of the directory blocks into buf
			int b = ip->i_block[block];

			get_block(*dev, b, dbuf);

			//we get the start of the directory entries from the buffer
			dp = (DIR *)dbuf;

			//while we have an entry
			while(strlen(dp->name) > 0)
			{
				int j = 0;

				//removes vertical tabs from the directory entries name
				while(j < strlen(dp->name))
				{
					if(dp->name[j] == 12) { dp->name[j] = '\0'; }
					j++;
				}

				dp->name[j] = '\0';

				//if that entry matches the part of the path name we are looking for, we are here
				if(strcmp(name[i], dp->name) == 0)
				{
					found = true;

					int blk, offset, ino;
			
					//gets the inode of the entry
					ino = dp->inode;

					//find the block and offset
					blk = ((ino - 1)/8) + gp->bg_inode_table;
					offset = (ino - 1) % 8;

					//go to the entry
					get_block(*dev, blk, buf);

					ip = (INODE *)buf + offset;
				
					i++;

					//if there are absolutely no more entries at this point, we made it to the end
					if(!name[i])
					{
						return ino;
					}
				}

				//else move entries
				else
				{
					dp = (DIR *)((char *)dp + (dp->rec_len)); 
				}
			}
		
		block++;
		}
	}

	return -1;
}   


//returns 0 if not found, or 1 if found
int search(MINODE *mip, char *name)
{
	int i = 0;
	while(i < 12)
	{
		//if we are in use
		if(mip->INODE.i_block[i] != 0)
		{
			DIR *dirp;
			char dbuf[BLOCK_SIZE];
			get_block(mip->dev, mip->INODE.i_block[i], dbuf);
			dirp = (DIR *)dbuf;
			
			while(strlen(dirp->name) > 0)
			{
				//get the name and add a terminating character
				char dirname[dirp->name_len + 1];
				strncpy(dirname, dirp->name, dirp->name_len);
				dirname[dirp->name_len] = 0;
				
				
				if(strcmp(name, dirname) == 0) { return 1; }
				
				dirp = (DIR *)((char *)dirp + (dirp->rec_len));
			}
		}
		
		i++;
	}
	
	return 0;
}


MINODE *iget(int dev, int ino)
{
	int size = sizeof(minodes)/(sizeof(minodes[0]));
	
	//search for minode in mArray[];
	//if we have a match here, we increment ref count and return mArray
	int i = 0;
	while(i < (size))
	{
		if((minodes[i].dev == dev) && (minodes[i].ino == ino))
		{
			minodes[i].refCount++;
			return &minodes[i];
		}
		
		i++;
	}

	//else if we did not find it, add it to the array
	i = 0;
	while(i < size)
	{
		//first item in the array to be referenced
		if(minodes[i].refCount == 0)
		{
			char buf[BLOCK_SIZE];
			int blk, offset;

			blk = ((ino - 1)/8) + gp->bg_inode_table;
			offset = (ino - 1) % 8;

			INODE *mIn;

			get_block(dev, blk, buf);

			mIn = (INODE *)buf + offset;
			
			memcpy(&(minodes[i].INODE), mIn, sizeof(INODE));
			
			
			
			minodes[i].refCount = 1;
			minodes[i].ino = ino;
			minodes[i].dev = dev;
			minodes[i].dirty = 0;
			minodes[i].mounted = 0;
			minodes[i].mountptr = NULL;

			return &minodes[i];
		}

		i++;
	}

	return 0;
}

//should only take input in the form of &minodes[], so nothing strange should
//be happening
int iput(MINODE *mip)
{
	int i = 0;
	int size = sizeof(minodes) / (sizeof(minodes[0]));
	
	while(i < size)
	{
		//if we are working with the same dev and ino
		if((minodes[i].ino == mip->ino) && (minodes[i].dev == mip->dev))
		{
			minodes[i].refCount--;
			
			if(minodes[i].refCount > 0)
			{
				return 1;
			}

			if(minodes[i].dirty == 0)
			{
				return 1;
			}

			int ino = minodes[i].ino;
			int dev = minodes[i].dev;
			int blk, offset;
			blk = ((ino - 1) / 8) + gp->bg_inode_table;
			offset = (ino - 1) % 8;

			char buf[BLOCK_SIZE];

			INODE *oldInode;
			
			//ask about the buf changing contents
			get_block(dev, blk, buf);

			oldInode = (INODE *)buf + offset;
			*oldInode = mip->INODE;

			put_block(dev, blk, buf);
			
			//clears entry in minodes
			minodes[i] = (MINODE){0};	
			
			return ino;
		}
		
		i++;
	}

	return 0;
}

//give a parent DIR, and an ino, finds the myname string in the parent's data blocks.
//Copy name into myname
int findmyname(MINODE *parent, int myino, char *myname)
{
	DIR *dirp;
	char buf[BLOCK_SIZE];

	get_block(parent->dev, parent->INODE.i_block[0], buf);

	dirp = (DIR *)buf;

	//iterates through all of the dir entries in mip->INODE
	while(strlen(dirp->name) > 0)
	{
		//formatting string
		int i = 0;
		for(i = 0; i < strlen(dirp->name); i++)
		{
			if(dirp->name[i] == 12) { dirp->name[i] = '\0'; }
		}
	
		dirp->name[i] = '\0';

		//if we find a match on inodes
		if(myino == dirp->inode)
		{
			//copy the name into myname
			strcpy(myname, dirp->name);
			return 1;
		}

		//else if we did not, we need to move entries
		dirp = (DIR *)((char *)dirp + (dirp->rec_len));
	}

	return 0;
}

//for a DIR minode, extract the inumbers of . and ..
//Read in 0th data block. the inumbers are in the first two dir entries
int findino(MINODE *mip, int *myino, int *parentino)
{
	DIR *dirp;
	char buf[BLOCK_SIZE];

	//get the dir entries
	get_block(mip->dev, mip->INODE.i_block[0], buf);

	dirp = (DIR *)buf;

	//dot is us
	*myino= dirp->inode;

	dirp = (DIR *)((char *)dirp + (dirp->rec_len));

	//dot dot is our parent
	*parentino = dirp->inode;
}

//ALLOCATE FUNCTIONS
int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int incFreeInodes(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int incFreeBlocks(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
}

//allocate a FREE inode number
int ialloc(int dev)
{
  int  i;
  char buf[BLOCK_SIZE];
  char sbuf[BLOCK_SIZE];
  int bg = gp->bg_inode_bitmap;
  
  // read inode_bitmap block
  get_block(dev, bg, buf);
  get_block(dev, 1, sbuf);
  
  sp = (SUPER *)sbuf;
  
  for (i=0; i < sp->s_inodes_count; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, bg, buf);

	   char gbuf[BLOCK_SIZE];
	   get_block(dev, 2, gbuf);
	   gp = (GD *)gbuf;

       return i+1;
    }
  }
  return 0;
}

//allocate a FREE block number
int balloc(int dev)
{
  int  i;
  char buf[BLOCK_SIZE];
  char sbuf[BLOCK_SIZE];
  int bg = gp->bg_block_bitmap;
  
  // read inode_bitmap block
  get_block(dev, bg, buf);
  get_block(dev, 1, sbuf);
  
  sp = (SUPER *)sbuf;

  for (i=0; i < sp->s_blocks_count; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeBlocks(dev);

       put_block(dev, bg, buf);

       return i + 1;
    }
  }
  return 0;
}

//deallocate an inode number
int idealloc(int dev, int ino)
{
	char buf[BLOCK_SIZE];
	char gbuf[BLOCK_SIZE];
	
	get_block(dev, 2, gbuf);
	gp = (GD *)gbuf;
	
	get_block(dev, gp->bg_inode_bitmap, buf);

	//test if the block is in use
	if(tst_bit(buf, ino - 1) == 1)
	{
		clr_bit(buf, ino - 1);
		incFreeInodes(dev);
		
		put_block(dev, gp->bg_inode_bitmap, buf);
		
		return ino;	
	}
	
	put_block(dev, gp->bg_inode_bitmap, buf);

	return 0;
}

//deallocate a block number
int bdealloc(int dev, int bno)
{
	char buf[BLOCK_SIZE];
	char gbuf[BLOCK_SIZE];
	
	get_block(dev, 2, gbuf);
	
	gp = (GD *)gbuf;
	
	get_block(dev, gp->bg_block_bitmap, buf);

	//test if the block is in use
	if(tst_bit(buf, bno) == 1)
	{
		clr_bit(buf, bno);
		incFreeBlocks(dev);
		put_block(dev, gp->bg_block_bitmap, buf);
		
		return bno;	
	}
	
	put_block(dev, gp->bg_block_bitmap, buf);

	return 0;
}
