#include "type.h"
/*
	BIG NOTE: Pecking Order
	
	RUNNING: the first proc we encounter. Points to an array of procs
	PROCS: the array of procs. Running points to one of these, and each proc
			contains an *cwd pointer to an minode in the minode array
			
	MINODES[]: array of minodes. each proc points to one of these
	INODE: each minode has an inode's data. They contain an inode, and an ino 
*/

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;
MINODE minodes[100];

//never actually use root to alter items. ONLY USE IT FOR BASIC INFO
MINODE *root;

//running proc
//PID is equal to position in array
PROC *running;
PROC procs[16];
char pathname[64], parameter[64];
