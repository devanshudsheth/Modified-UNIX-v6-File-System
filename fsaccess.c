/*******************************************************************************************************
*	Group Members : 1. Devanshu Sheth - dds160030
*			2. Srichandrakiran Gottipati - sxg166430
*	
*	Implemented this program using putty on csgrads1@utdallas.edu
*	write the program on csgrads1 using vi fsaccess.c 
*	compile using cc fsaccess.c
*	Execute with ./a.out
*	Read the readme for other details.
*  Thank you.
*
*******************************************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>


//GLOBAL CONSTANTS
#define BLOCK_SIZE 512
#define ISIZE 32
#define inode_alloc 0100000        //Flag value to allocate an inode
#define pfile 000000               //To define file as a plain file
#define lfile 010000               //To define file as a large file
#define directory 040000           //To define file as a Directory
#define max_array 256
#define null_size 512


//GLOBAL VARIABLES
int fd ;                //file descriptor
int rootfd;
const char *rootname;
unsigned short chainarray[max_array];



// super block 
typedef struct {
unsigned short isize;
unsigned short fsize;
unsigned short nfree;
unsigned short free[100];
unsigned short ninode;
unsigned short inode[100];
char flock;
char ilock;
char fmod;
unsigned short time[2];
} fs_super;//instance of superblock
fs_super super;


//inode

typedef struct {
unsigned short flags;
char nlinks;
char uid;
char gid;
char size0;
unsigned short size1;
unsigned short addr[8];
unsigned short actime[2];
unsigned short modtime[2];
} fs_inode; //instance of inode

fs_inode initial_inode;//for initial inode

// directory
typedef struct 
{
        unsigned short inode;
        char filename[13];
        }dir;
dir newdir;		// instance of directory
dir dir1;   //for duplicates



//functions in order

void chainblocks( unsigned short total_blocks);
void create_rootdir();
int initializefs(char* path, unsigned short total_blocks,unsigned short total_inodes);
unsigned short allocatefreedblock();
unsigned short allocateinode();
void read_block_int(unsigned short *dest, unsigned short bno);
void write_block_int(unsigned short *dest, unsigned short bno);
void copy_inode(fs_inode current_inode, unsigned int new_inode);
void freeblock(unsigned short block);
void update_rootdir(const char *pathname , unsigned short inode_number);
void cpin(const char *pathname1 , const char *pathname2);
void out_smallfile(const char *pathname1 , const char *pathname2 , int num_block);
void out_largefile(const char *pathname1 , const char *pathname2 , int num_blocks);
void cpout(const char *pathname1 , const char *pathname2);
void largefile(const char *pathname1 , const char *pathname2 , int blocks_allocated);
void smallfile(const char *pathname1 , const char *pathname2 , int blocks_allocated);
void makedirectory(const char *pathname);
void update_rootdir(const char *pathname , unsigned short inode_number);
void display_files();


// Data blocks chaining procedure
 void chainblocks(unsigned short total_blocks)
	 {
	 unsigned short emptybuffer[256];
	 unsigned short linked_blockcount;
	 unsigned short split_blocks = total_blocks/100;
	 unsigned short remaining_blocks = total_blocks%100;
	 unsigned short idx = 0;
	 int i=0;
	 for (idx=0; idx <= 255; idx++)
		 {
		 emptybuffer[idx] = 0;
		 chainarray[idx] = 0;
		 }

	 //chaining for chunks of blocks 100 blocks at a time
	 for (linked_blockcount=0; linked_blockcount < split_blocks; linked_blockcount++)
		 {
		 chainarray[0] = 100;
	
		 for (i=0;i<100;i++)
			 {
			 if((linked_blockcount == (split_blocks - 1)) && (remaining_blocks == 0))
				 {
				 if ((remaining_blocks == 0) && (i==0))
					 {
					 if ((linked_blockcount == (split_blocks - 1)) && (i==0))
						 {
						 chainarray[i+1] = 0;
						 continue;
						 }
					 }
				 }
			 chainarray[i+1] = i+(100*(linked_blockcount+1))+(super.isize + 2 );
			 }			

		 write_block_int(chainarray, 2+super.isize+100*linked_blockcount);

		 for (i=1; i<=100;i++)
			 write_block_int(emptybuffer, 2+super.isize+i+ 100*linked_blockcount);
		 }

	//chaining for remaining blocks
	chainarray[0] = remaining_blocks;
	chainarray[1] = 0;
	
	for (i=1;i<=remaining_blocks;i++)
		 chainarray[i+1] = 2+super.isize+i+(100*linked_blockcount);

	 write_block_int(chainarray, 2+super.isize+(100*linked_blockcount));

	 for (i=1; i<=remaining_blocks;i++)
		 write_block_int(chainarray, 2+super.isize+1+i+(100*linked_blockcount));
	 }


 //function to create root directory and its inode.
 void create_rootdir()
 	{
 	rootname = "root";
 	rootfd = creat(rootname, 0600);
 	rootfd = open(rootname , O_RDWR | O_APPEND);
 	unsigned int i = 0;
 	unsigned short num_bytes;
 	unsigned short dblock = allocatefreedblock();
 	
	for (i=0;i<14;i++)
		 newdir.filename[i] = 0;

 	newdir.filename[0] = '.';                       //root directory's file name is .
 	newdir.filename[1] = '\0';
 	newdir.inode = 1;                                       // root directory's inode number is 1.
	
	 initial_inode.flags = inode_alloc | directory | 000077;
	 initial_inode.nlinks = 2;
	 initial_inode.uid = '0';
	 initial_inode.gid = '0';
	 initial_inode.size0 = '0';
	 initial_inode.size1 = ISIZE;
	 initial_inode.addr[0] = dblock;
	
	 for (i=1;i<8;i++)
		 initial_inode.addr[i] = 0;

	initial_inode.actime[0] = 0;
	initial_inode.modtime[0] = 0;
	initial_inode.modtime[1] = 0;

	copy_inode(initial_inode, 0);
	lseek(rootfd , 512 , SEEK_SET);
	write(rootfd , &initial_inode , 32);
	lseek(rootfd, dblock*BLOCK_SIZE, SEEK_SET);

	//filling 1st entry with .
	num_bytes = write(rootfd, &newdir, 16);
	if((num_bytes) < 16)
		 printf("\n Error in writing root directory \n ");

	//filling net entry with ..
	newdir.filename[0] = '.';
	newdir.filename[1] = '.';
	newdir.filename[2] = '\0';
	num_bytes = write(rootfd, &newdir, 16);
	if((num_bytes) < 16)
		 printf("\n Error in writing root directory\n ");
	close(rootfd);
	}




// file system initilization
int initializefs(char* path, unsigned short total_blocks,unsigned short total_inodes)
	{
	printf("\nV6 File System by Devanshu and Sri Chandra Kiran \n");
	char buffer[BLOCK_SIZE];
	int num_bytes;
	if(((total_inodes*32)%512) == 0)
		super.isize = ((total_inodes*32)/512);
	else
		super.isize = ((total_inodes*32)/512)+1;	

	super.fsize = total_blocks;

	unsigned short i = 0;
	
	if((fd = open(path,O_RDWR|O_CREAT,0600))== -1)          
		{
		printf("\n open() failed with error [%s]\n",strerror(errno));
		return 1;
		}
	//assigning superblock values
	for (i = 0; i<100; i++)
		super.free[i] =  0;			

	super.nfree = 0;
	super.ninode = 100;
	
	for (i=0; i < 100; i++)
		super.inode[i] = i;		

	super.flock = 'f'; 					
	super.ilock = 'i';					
	super.fmod = 'f';
	super.time[0] = 0000;
	super.time[1] = 1970;
	lseek(fd,BLOCK_SIZE,SEEK_SET);
	lseek(fd,0,SEEK_SET);
	write(fd, &super, 512);

	if((num_bytes =write(fd,&super,BLOCK_SIZE)) < BLOCK_SIZE)
		{
		printf("\n error in writing the super block\n");
		return 0;
		}

	for (i=0; i<BLOCK_SIZE; i++)  
		buffer[i] = 0;

	lseek(fd,1*512,SEEK_SET);

	for (i=0; i < super.isize; i++)
		write(fd,buffer,BLOCK_SIZE);

	// chaining data blocks
	chainblocks(total_blocks);	

	for (i=0; i<100; i++) 
		{
		super.free[super.nfree] = i+2+super.isize; //get free block
		++super.nfree;
		}

	create_rootdir();
	return 1;
	}

// get a free data block.
unsigned short allocatefreedblock()
	{
	unsigned short block;
	block = super.free[--super.nfree];
	super.free[super.nfree] = 0;
	
	if (super.nfree == 0)
		{
		int n=0;
		read_block_int(chainarray, block);
		super.nfree = chainarray[0];
		for(n=0; n<100; n++)
			super.free[n] = chainarray[n+1];
		}
	return block;
	}

// allocate inode
unsigned short allocateinode()
	{
	unsigned short inumber;
	unsigned int i = 0;
	inumber = super.inode[--super.ninode];
	return inumber;
	}



//Read integer array from the required block
void read_block_int(unsigned short *dest, unsigned short bno)
	{
	int flag=0;
	if (bno > super.isize + super.fsize )
		flag = 1;

	else
		{			
		lseek(fd,bno*BLOCK_SIZE,SEEK_SET);
		read(fd, dest, BLOCK_SIZE);
		}
	}


//Write integer array to the required block
void write_block_int(unsigned short *dest, unsigned short bno)
	{
	int flag1, flag2;
	int num_bytes;
	
	if (bno > super.isize + super.fsize )
		flag1 = 1;		
	else
		{
		lseek(fd,bno*BLOCK_SIZE,0);
		num_bytes=write(fd, dest, BLOCK_SIZE);

		if((num_bytes) < BLOCK_SIZE)
			flag2=1;		
		}
	if (flag2 == 1)
		{
		//problem with block
		}
	}

//Write to an inode given the inode number	
void copy_inode(fs_inode current_inode, unsigned int new_inode)
	{
	int num_bytes;
	lseek(fd,2*BLOCK_SIZE + new_inode*ISIZE,0);
	num_bytes=write(fd,&current_inode,ISIZE);

	if((num_bytes) < ISIZE)
		printf("\n Error in inode number : %d\n", new_inode);		
	}


//free data blocks and initialize free array
void freeblock(unsigned short block)       
	{
	super.free[super.nfree] = block;
	++super.nfree;
	}

//Function to update root directory
void update_rootdir(const char *pathname , unsigned short inode_number)
	{
	int i;
	dir ndir;
	int size;
	ndir.inode = inode_number;
	strncpy(ndir.filename, pathname , 14);
	size = write(rootfd , &ndir , 16);
	}



//Function to copy file contents from external file to v6-file
void cpin(const char *pathname1 , const char *pathname2)
	{
	struct stat stats;
	int blksize , blocks_allocated , req_blocks;
	int filesize;
	stat(pathname1 , &stats);
	blksize = stats.st_blksize;
	blocks_allocated = stats.st_blocks;
	filesize = stats.st_size;
	req_blocks = filesize / 512;
	
	if(blocks_allocated <= 8)
		{
		// externalfile is small file
		printf("small file , %d\n" , blocks_allocated);
		smallfile(pathname1 , pathname2 , blocks_allocated); 
		}
	else
		{
		//externalfile is largefile
		printf("Large file , %d\n" , blocks_allocated);
		largefile(pathname1 , pathname2 , blocks_allocated); 
		}
	printf("cpin complete\n");
	}


//Copying from a Small File
void smallfile(const char *pathname1 , const char *pathname2 , int blocks_allocated)
	{
	int fdes , fd1 ,i ,j,k,l;
	unsigned short size;
	unsigned short inode_number;
	inode_number = allocateinode();
	struct stat s1;
	stat(pathname1 , &s1);
	size = s1.st_size;
	unsigned short buff[100];
	fs_inode inode;
 	fs_super sb;
	sb.isize = super.isize;
	sb.fsize = super.fsize;
	sb.nfree = 0;
		for(l=0; l<100; l++)
		{
		sb.free[sb.nfree] = l+2+sb.isize ;
		sb.nfree++;
		}

	inode.flags = inode_alloc | pfile | 000077; 
	inode.size0 =0;
	inode.size1 = size;
	blocks_allocated = size/512;
	fdes = creat(pathname2, 0775);
	fdes = open(pathname2 , O_RDWR | O_APPEND);
	lseek(fdes , 0 , SEEK_SET);
	write(fdes ,&sb , 512);
	update_rootdir(pathname2 , inode_number);
	unsigned short bl;
	for(i=0; i<blocks_allocated; i++)
		{
		inode.addr[i] = sb.free[i];
		sb.free[i] = 0;
		}
	close(fdes);
	lseek(fdes , 512 , SEEK_SET);
	write(fdes , &inode , 32);
	close(fdes);
	unsigned short buf[256];
	fd1 = open(pathname1 ,O_RDONLY);
	fdes = open(pathname2 , O_RDWR | O_APPEND);
	for(j =0; j<=blocks_allocated; j++)
		{
		lseek(fd1 , 512*j , SEEK_SET);
		read(fd1 ,&buf , 512);
		lseek(fdes , 512*(inode.addr[j]) , SEEK_SET);
		write(fdes , &buf , 512);
		}
	printf("Small file copied\n");
	close(fdes);
	close(fd1);
	fd = open(pathname2 , O_RDONLY); 
	
	}

// Copying from a Large File
void largefile(const char *pathname1 , const char *pathname2 , int blocks_allocated)
	{
	int size ,fdes1 ,fd1 ,i ,j ,h ,z ,x ,v , icount , c ,d ,m, w ,e ,t , l,f,sizer;
	unsigned short buff[100];
	unsigned short b;
	unsigned short fb;
	unsigned short i_number;
	i_number = allocateinode();
	struct stat s1;
	stat(pathname1 , &s1);
	size = s1.st_size;
	blocks_allocated = size/512;
	fs_super sb;
	sb.isize = super.isize;
	sb.fsize = super.fsize;
	sb.nfree = 0;
	for(l=0; l<100; l++)
		{
		sb.free[sb.nfree] = l+2+sb.isize;
		sb.nfree++;
		}
		
	fs_inode inode1;
	inode1.flags = inode_alloc | lfile | 000077; 
	fdes1 = creat(pathname2, 0775);
	fdes1 = open(pathname2 , O_RDWR | O_APPEND);
	unsigned short block1;
	lseek(fdes1 , 0 , SEEK_SET);
	write(fdes1 ,&sb , 512);
	for(i=0; i<8; i++)
		{
		block1 = allocatefreedblock(); // Get a Free Block
		inode1.addr[i] = block1;
		}
	close(fdes1);
	fdes1 = open(pathname2 , O_RDWR | O_APPEND);
	lseek(fdes1 , 512 , SEEK_SET);
	write(fdes1 , &inode1 , 32);
	close(fdes1);
	if(blocks_allocated > 67336)
		{
		printf("\nExternal File too large\n");
		}
	else
		{
		if(blocks_allocated < 2048)
			{
			unsigned short bf[8][256];
			unsigned short bz[8][256];
			unsigned short block2;
			int k =8;
			for(j =0; j<8; j++)
				{
				if(k < blocks_allocated)
					{
					fdes1 = open(pathname2 , O_RDWR | O_APPEND);
					for(m=0; m<256; m++)
						{
						if(m > blocks_allocated)
							break;
						block2 = allocatefreedblock();
						bf[j][m] = block2;

						}
					close(fdes1);
					for(e=0; e<256; e++)
						{
						fdes1 = open(pathname2 , O_RDWR | O_APPEND);
						lseek(fdes1 , ((inode1.addr[j])*512)+(2*e) , SEEK_SET);
						write(fdes1 , &bf[j][e] , 2);
						close(fdes1);
						}
					}  
				else
					break;
				k = k + 256;
				}
			unsigned short buffer[256];
			unsigned short buf[256][256];
			int m=0;
			int r=0;
			int count =0;
			fd1 = open(pathname1 , O_RDONLY);
			for(h=0; h<=blocks_allocated; h++)
				{
				if(r<8)
					{
					lseek(fd1 , h*512 , SEEK_SET);
					for(x=0; x<256; x++)
						{
						if(count > blocks_allocated)
							break;
						read(fd1 , &buffer , 512);
						fdes1 = open(pathname2 , O_RDWR | O_APPEND);
						lseek(fdes1 , 512*(buf[r][x]) , SEEK_SET);
						write(fdes1 , &buffer , 512);
						close(fdes1);
						count++;
						}
					}
				r++;
				}
			close(fd1);
			}
		else
			{
			unsigned short bf[8][256];
			unsigned short bz[256];
			unsigned int block2;
			int k =7;
			int r=0;
			fdes1 = open(pathname2 , O_RDWR | O_APPEND);
			for(j =0; j<7; j++)
				{
				if(k < blocks_allocated)
					{
					fdes1 = open(pathname2 , O_RDWR | O_APPEND);
					for(m=0; m<256; m++)
						{
						block2 = allocatefreedblock();
						bf[j][m] = block2;
						r++;
						}
					close(fdes1);
					fdes1 = open(pathname2 , O_RDWR | O_APPEND);
					for(e=0; e<256; e++)
						{
						lseek(fdes1 , ((inode1.addr[j])*512)+ 2*e , SEEK_SET);
						write(fdes1 , &bf[j][e] , 2);
						}
					close(fdes1);
					}  
				else
					break;
				k = k + 256;
				}

			//changed after the long quiz
			//code for double indirect block
			unsigned short longbuff[256][256];
			fdes1 = open(pathname2 , O_RDWR | O_APPEND);
			for(c=0; c<256; c++)
				{
				lseek(fdes1 , bf[7][c]*512 , SEEK_SET);
				for(d=0; d<256;d++)
					{
					if(r > blocks_allocated)
						break;
					block2 = allocatefreedblock(sb);
					longbuff[c][d] = block2; 
					r++; 
					}
				for(t=0; t<256; t++)
					{
					lseek(fdes1 , (bf[7][c]*512)+2*t , SEEK_SET);
					write(fdes1 , &longbuff[c][t] , 2);
					}
				}
			close(fdes1);
			unsigned short buffer[256];
			unsigned short bu;
			int s=0;
			int k1=0;
			fd1 = open(pathname1 , O_RDONLY);
			for(h=0; h<=blocks_allocated; h++)
				{
				if(k1<7)
					{
					lseek(fd1 , h*512 , SEEK_SET);
					read(fd1 , &buffer , 512);
		
					for(x=0; x<256; x++)
						{
						fdes1 = open(pathname2 , O_RDWR | O_APPEND);
						lseek(fdes1 , (bf[k1][x])*512 , SEEK_SET);
						write(fdes1 , &buffer , 512);
						close(fdes1);
						s++;
						}
					k1++;
					}
				else
					{
					lseek(fd1 , h*512 , SEEK_SET);
					read(fd1 , &buffer , 512);
					unsigned short dilbuf[256];
					fdes1 = open(pathname2 , O_RDWR | O_APPEND);
					for(v=0; v<256; v++)
						{
						for(w=0; w<256; w++)
							{
							if(s > blocks_allocated)
								break;
							lseek(fdes1 , longbuff[v][w]*512 , SEEK_SET);
							write(fdes1 , &buffer , 512);
							s++;
							}
						}
					close(fdes1);
					}
				}
			close(fd1);
			}
		}	

	}
// cpout Copying from v-6file System into an external File
void cpout(const char *pathname1 , const char *pathname2)
	{
	struct stat stats;
	int blocksize , blocks_allocated , num_blocks;
	int filesize;
	stat(pathname1 , &stats);
	blocksize = stats.st_blksize;
	blocks_allocated = stats.st_blocks;
	filesize = stats.st_size;
	num_blocks = filesize /512;
	
	if(blocks_allocated < 8)
		{
		//v6-file is small file
		printf("Small file , %d\n" , num_blocks);
		out_smallfile(pathname1 , pathname2 , num_blocks);  
		}
	else
		{
		//v6-file is large file
		printf("Largefile , %d\n" , num_blocks);
		out_largefile(pathname1 , pathname2 ,  num_blocks);  
		}
	printf("cpout complete\n");
	}

//Copying Small file using cpout
void out_smallfile(const char *pathname1 , const char *pathname2 , int blocks_allocated)
	{
	int numblocks = blocks_allocated;
	int fdes , fd1 , i;
	unsigned short buffer[256];
	unsigned short addr[8];
	fdes = open(pathname1 , O_RDONLY);
	lseek(fdes , 520 , SEEK_SET);
	read(fdes , &addr , 16);
	close(fdes);
	fdes = open(pathname1 , O_RDONLY);
	fd1 = creat(pathname2, 0775);
	fd1 = open(pathname2, O_RDWR | O_APPEND);
	for(i =0 ; i<numblocks; i++)
		{
		lseek(fdes , i*512 , SEEK_SET);
		read(fdes , &buffer , 512);
		lseek(fd1 , 512*i , SEEK_SET);
		write(fd1 , &buffer , 512);
		}
	close(fd1);
	close(fdes);
	}

//Copying a large file using cpout
void out_largefile(const char *pathname1 , const char *pathname2 , int blocks_allocated)
	{
	int numblocks = blocks_allocated;
	int fdes1 , fd1 , i ,j ,k ,x ,m ,l ,r;
	unsigned short buffer[256];
	unsigned short buff[8][256];
	unsigned short buf[256][256];
	unsigned short addr[8];
	fd1 = creat(pathname2, 0775);
	for(i=0; i<numblocks; i++)
		{
		fdes1 = open(pathname1 , O_RDONLY);
		lseek(fdes1 , i*512 , SEEK_SET);
		read(fdes1 , &buffer , 512);
		fd1 = open(pathname2 , O_RDWR | O_APPEND);
		lseek(fd1 , i*512 , SEEK_SET);
		write(fd1 , &buffer , 512);
		close(fd1);
		}
	close(fdes1);
	}  

// make directory
void makedirectory(const char *pathname)
	{
	int filedes , i ,j;
	filedes = creat(pathname, 0666);
	filedes = open(pathname , O_RDWR | O_APPEND);
	fs_inode inode2;
	unsigned short inum;
	inum = allocateinode();
	inode2.flags = inode_alloc | directory | 000077; 
	inode2.nlinks = 2;
	inode2.uid =0;
	inode2.gid =0;
	for(i=0; i<8; i++)
		{
		inode2.addr[i] = 0;
		}
	unsigned short bk;
	bk = allocatefreedblock();
	inode2.addr[0] = bk;
	lseek(filedes , 512 , SEEK_SET);
	write(filedes , &inode2 , 32);
	dir direct;
	direct.inode = inum;
	strncpy(direct.filename, "." , 14);
	lseek(filedes , inode2.addr[0]*512 , SEEK_SET);
	write(filedes , &direct , 16);
	strncpy(direct.filename, ".." , 14);
	lseek(filedes , (inode2.addr[0]*512 + 16) , SEEK_SET);
	write(filedes , &direct , 16);
	close(filedes);
	update_rootdir(pathname , inum);
	printf("directory created\n");
	}

// display all files
void display_files()
	{  
	int i , size;
	fs_inode i_node;
	dir out;
	unsigned short buffer;
	rootfd = open(rootname , O_RDWR | O_APPEND);
	lseek(rootfd , 520 , SEEK_SET);
	size = read(rootfd , &i_node.addr[0] , 2);
	for(i=0; i<10; i++)
		{
		lseek(rootfd , (120*512)+(16*i) , SEEK_SET);
		size = read(fd , &out , 16);
		printf("%d , %s\n" , out.inode , out.filename);
		close(rootfd);
		}
	}


int offset_set(int block)
	{
	int offset =0;
	return block * BLOCK_SIZE + offset;
	}


//MAIN
int main(int argc, char *argv[])
	{	
	char c;
	int i=0;
	char *tmp = (char *)malloc(sizeof(char) * 200);
	char *cmd1 = (char *)malloc(sizeof(char) * 200);
	signal(SIGINT, SIG_IGN);
	int fs_initcheck = 0;    // bit to check if file system is initialized
	char *parser, cmd[256];
	unsigned short n = 0, j , k;
	char buffer1[BLOCK_SIZE];
	unsigned short num_bytes;
	char *name_dir;
	char *cpoutextern;
	char *cpoutsource;
	unsigned int bno =0, inode_no=0;
	char *cpinextern;
	char *cpinsource;
	char *file_path;
	char *filename;
	char *num1, *num2, *num3, *num4;

	
	printf("\n\nInitialize file system by using initfs <<name of your v6filesystem>> << total blocks>> << total inodes>>\n");
	while(1)
		{
		printf("\nEnter command\n");
		scanf(" %[^\n]s", cmd);
		parser = strtok(cmd," ");
		if(strcmp(parser, "initfs")==0)
			{
			file_path = strtok(NULL, " ");
			num1 = strtok(NULL, " ");
			num2 = strtok(NULL, " ");
			if(access(file_path,X_OK) != -1)
				{
				if( fd = open(file_path, O_RDWR) == -1)
					{
					printf("File system exists, open failed");
					return 1;
					}
				access(file_path, R_OK |W_OK | X_OK);
				printf("Filesystem already exists.\n");
				printf("Same file system to be used\n");
				fs_initcheck=1;
				}
			else
				{
				if (!num1 || !num2)
					printf("Enter all arguments in form initfs v6filesystem 5000 400.\n");
				else
					{
					bno = atoi(num1);
					inode_no = atoi(num2);
					if(initializefs(file_path,bno, inode_no))
						{
						printf("File System has been Initialized \n");
						fs_initcheck = 1;
						}
					else
						{
						printf("Error: File system initialization error.\n");
						}
					}
				}
			parser = NULL;
			printf("\n");
			}
		else if(strcmp(parser, "mkdir")==0)
			{
			if(fs_initcheck == 0)
				printf("File System has not been initialized.\n");
			else
				{
				name_dir = strtok(NULL, " ");
				if(!name_dir)
					{
					printf("Enter the command in form mkdir v6-dir.\n");
					}
				else
					{
					unsigned int dirinum = allocateinode();
					if(dirinum < 0)
						{
						printf("insufficient inodes \n");
						exit(1);
						}
					if(access(name_dir, R_OK) == 0)
						{
						printf("Directory already exists\n");
						}
					else
						makedirectory(name_dir);
					}
				}
			parser = NULL;
			printf("\n");
			}
		else if(strcmp(parser, "cpin")==0)
			{
			if(fs_initcheck == 0)
				printf("File System has not been initialized.\n\n");
			else
				{
				cpinsource = strtok(NULL, " ");
				cpinextern = strtok(NULL, " ");
				if(!cpinsource || !cpinextern )
					printf("Enter the command in the form cpin externalfile v6-file \n");
				else if((cpinsource) && (cpinextern ))
					{
					cpin(cpinsource,cpinextern);
					//printf("Enter ls command to check");
					}
				}
			parser = NULL;
			printf("\n");
			}
		else if(strcmp(parser, "cpout")==0)
			{
			if(fs_initcheck == 0)
				printf("File system has not been initialized.\n");
			else
				{
				cpoutsource = strtok(NULL, " ");
				cpoutextern = strtok(NULL, " ");
				if(!cpoutsource || !cpoutextern )
					printf("Enter command in the form cpout v6-file externalfile\n");
				else if((cpinsource) && (cpinextern ))
					{
					cpout(cpoutsource,cpoutextern);
					//printf("Enter ls command to check");
					}
				}
			parser = NULL;
			printf("\n");
			}
		else if (strcmp(parser, "rm")==0)
			{
			if (fs_initcheck == 0)
				printf("File system has not been initialized.\n");
			else
			{
			FILE *fp;
			filename = strtok(NULL, " ");
			fp = fopen( filename, "w");
			fclose(fp);
			int ret = remove(filename);
			if( ret == 0 )
				{
				
				printf("File deleted\n");
				//printf("Enter ls command to check\n");
				}
			}
			parser = NULL;
			printf("\n");
			}
		else if(strcmp(parser , "ls") ==0)
			{
			display_files();
			fd=open(num1,O_RDWR,0600);
			lseek(fd, 2 * BLOCK_SIZE  * sizeof(fs_inode), SEEK_SET);
			read(fd, &initial_inode, sizeof(initial_inode));
			dir temp_entry;
			int idx = 0;
			while(idx < initial_inode.size0 / sizeof(dir))
				{
				if(idx % (BLOCK_SIZE / sizeof(dir)) == 0)
					{
					lseek(fd, offset_set(initial_inode.addr[idx / (BLOCK_SIZE / sizeof(dir))]), SEEK_SET);
					}
				read(fd, &temp_entry, sizeof(temp_entry));
				printf("%-d %s \n", temp_entry.inode, temp_entry.filename);
				idx++;
				}
			printf("\n");

			}
		else if(strcmp(parser, "q")==0)
			{
			printf("\nEXITING FILE SYSTEM NOW....\n");
			lseek(fd,BLOCK_SIZE,0);
			if((num_bytes =write(fd,&super,BLOCK_SIZE)) < BLOCK_SIZE)
				{
				printf("\nerror in writing the super block\n");
				}
			lseek(fd,BLOCK_SIZE,0);
			return 0;
			}
		else
			{
			printf("\nInvalid command\n ");
			printf("\n");
			}
		}
	}
