#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>

// Root inode
#define RTINODE 1 
// Directory size
#define DIRECTORYSIZE 14
// Block size
#define NUMDIRECT 12
// Inodes per block.
#define BLOCKSIZE 512 


// Super block
struct sup_block {
    uint size;					//size blocks
    uint numberOfBlocks;  		//# blocks
    uint numberOfIndoes;  		//# inodes
};

// Directory entry
struct directoryEntry {
    ushort inodeNumber;		//inode number
    char directoryName[DIRECTORYSIZE];	//directory name
};

// On-disk inode structure
struct ondiskinode {
    short type; 						//file type
    short majorDeviceNumber; 			//majorDeviceNumber
    short minorDeviceNumber; 			//minorDeviceNumber
    short numberOfLinks; 				//# of links to inode
    uint size; 							//size of file in bytes
    uint dataBlockAddress[NUMDIRECT+1];	//data block addresses
};


// Number of direct links
#define INODEPERBLOCK (BLOCKSIZE / sizeof(struct ondiskinode))
// Bitmap bits per block
#define BITMAPBITSPERBLOCK           (BLOCKSIZE*8)
// Block containing bit for block b
#define BBITBLOCK(b, numberOfIndoes) (b/BITMAPBITSPERBLOCK + (numberOfIndoes)/INODEPERBLOCK + 3)
// Block containing inode i
#define INODEIBLOCK(i)     ((i) / INODEPERBLOCK + 2)



//Main Function
int main(int argc, char *argv[]) 
{
	char *nameOfFile = argv[1];
	int blkAdr;// for inodes

	if (argc != 2) 
	{
		fprintf(stderr, "Usage: fcheck <file_system_image>\n");
		exit(1);
	}
    int fileimage = open(nameOfFile, O_RDONLY);
    // Check 0: checks for bad file inputs
    if (fileimage <= -1) 
	{
        fprintf(stderr, "image not found.\n");
        return 1;
    }

    struct stat shBuffer;
    fstat(fileimage, &shBuffer);
    void *image = mmap(NULL, shBuffer.st_size, PROT_READ, MAP_PRIVATE, fileimage, 0);
    struct sup_block *su_block = (struct sup_block *)(image + BLOCKSIZE);
	



	// Fnumber 1 inode block that have list of inodes
    struct ondiskinode *inode = (struct ondiskinode *)(image + BLOCKSIZE+BLOCKSIZE);
	// bitmap location on disk pointer
    void *bitmap = (void *)((su_block->numberOfIndoes/INODEPERBLOCK + 3)*BLOCKSIZE+image);
    // for a list that has all used blocks
	int xlsize=su_block->size;
    int blockinuse[xlsize];
	// lets set all used blocks as unused
	int x = 0;
    while(x < xlsize)
	{
        blockinuse[x] = 0;
		x++;
	}

    // used blocks set as unused inside a list
	int y = 0;
	int dlt=su_block->numberOfIndoes/INODEPERBLOCK;
	int prox1=xlsize/(BLOCKSIZE*8);
    while ( y < (dlt + (prox1+1)+3))
	{
        blockinuse[y] = 1;
		y++;
	}
	
	// capture links in file system to the inodes and their number
	int sl=su_block->numberOfIndoes;
    int numberOfLinks[sl];
	int z = 0;
	while ( z < su_block->numberOfIndoes)
	{
        numberOfLinks[z] = 0;
		z++;
	}
	
	// used inodes
    int usedInodes[su_block->numberOfIndoes];
	// check if the indoe is in directory
    int inodeDir[su_block->numberOfIndoes];
	int i = 0;
	while ( i < su_block->numberOfIndoes)
	{
		inodeDir[i] = 0;
		i++;
	}

	//go through each inode
	i = 0;
    while (i < su_block->numberOfIndoes) 
	{
		int ntype = inode->type;
		struct directoryEntry *DirCurr;
		// check for root inode is valid
    	if ( ntype != 1 && i == 1) 
		{
        	fprintf(stderr, "ERROR: root directory does not exist.\n");
            return 1; 
        }
		// make sure file type is valid
		if (!(ntype <= 3 && ntype >= 0)) 
		{
			// make sure valid type
                fprintf(stderr,"ERROR: bad inode.\n");
                return 1;
		} else 
		{
			// ensure its validity
			if (ntype != 0) 
			{
				// ensure valid directory
		        if (ntype == 1) 
				{ 
					DirCurr = (struct directoryEntry*)(inode->dataBlockAddress[0] * BLOCKSIZE + image);
					// ensure valid dir format
		            if (strcmp(DirCurr->directoryName, ".") != 0) 
					{
		                fprintf(stderr, "ERROR: directory not properly formatted.\n");
		                return 1;
		            }
		            DirCurr++;
		            if (strcmp(DirCurr->directoryName, "..") != 0) 
					{
		                fprintf(stderr, "ERROR: directory not properly formatted.\n");
		                return 1;
		            }
		            // check for valid root inode
		            if (i != 1 && DirCurr->inodeNumber == i) 
					{
		                fprintf(stderr, "ERROR: root directory does not exist.\n");
		                return 1;
		            }
					int j = 0;
					while (j < NUMDIRECT+1) 
					{
		                struct directoryEntry *thisDirectoryEntry;	
						if (inode->dataBlockAddress[j] != 0)
						{
							if (!(j < NUMDIRECT))
							{ //final of the indirect
								uint *currIn = (uint *)(inode->dataBlockAddress[j]*BLOCKSIZE+ image);
								//loop through indirect pointer in block
								int k = 0;												
								while (k < 128) 
								{
									struct directoryEntry *forThisDirectoryEntry = (struct directoryEntry*)( currIn[k]*BLOCKSIZE + image);
									int q = 0;
									while (q < BLOCKSIZE/sizeof(struct directoryEntry)) 
									{
										if (forThisDirectoryEntry->inodeNumber != 0) 
										{
											if (strcmp(forThisDirectoryEntry->directoryName,"..") != 0 && strcmp(forThisDirectoryEntry->directoryName,".") != 0) 
											{
                                                numberOfLinks[forThisDirectoryEntry->inodeNumber]++;
                                                inodeDir[forThisDirectoryEntry->inodeNumber] = 1;
											}
										}
										forThisDirectoryEntry ++;
										q++;
									}
									k++;
								}

							} 
							else 
							{
								thisDirectoryEntry = (struct directoryEntry *)(inode->dataBlockAddress[j]*BLOCKSIZE+ image);
								int z = 0;
								while ( z < BLOCKSIZE/sizeof(struct directoryEntry)) 
								{
									if (thisDirectoryEntry->inodeNumber != 0) 
									{
										if (strcmp(thisDirectoryEntry->directoryName,"..") != 0 && strcmp(thisDirectoryEntry->directoryName,".") != 0) 
										{
			                        		numberOfLinks[thisDirectoryEntry->inodeNumber]++;
											inodeDir[thisDirectoryEntry->inodeNumber] = 1;
										}
									}
									thisDirectoryEntry++;
									z++;
			               		}
							}
							
		            	}
						j++;
					}
				}
				//loop through directorys and files
				int j = 0;
		        while (j < NUMDIRECT+1) 
				{
					blkAdr = inode->dataBlockAddress[j];
					// ensure the address validity
		            if (blkAdr != 0) 
					{
			            // check inode for bad address 
				        if((blkAdr) < ((int)BBITBLOCK(su_block->numberOfBlocks, su_block->numberOfIndoes))+1 || blkAdr > (su_block->size * BLOCKSIZE))
						{
				            if (j < NUMDIRECT)
								fprintf(stderr, "ERROR: bad direct address in inode.\n");	
							else 
								fprintf(stderr, "ERROR: bad indirect address in inode.\n");
				            return 1;
				        }
						// check for single use of used blocks 
						if(blockinuse[blkAdr] == 1)
						{
							if (j < NUMDIRECT)
								fprintf(stderr, "ERROR: direct address used more than once.\n");
							else 
								fprintf(stderr, "ERROR: indirect address used more than once.\n");
							return 1;
						}
						blockinuse[blkAdr] = 1;
					
			            // check bitmap for used blocks 
			            int bitmapLocation = (*((char*)bitmap + (blkAdr >> 3)) >> (blkAdr & 7)) & 1;
			            if (bitmapLocation == 0) 
						{
			                fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
			                return 1;
			            }
					}
					j++;
		        }

				//found an indirect link 
		        if (inode->size > BLOCKSIZE * NUMDIRECT) 
				{
		            int *linkindirect = (int *)(image + (blkAdr*BLOCKSIZE));
					j = 0;
		            while (j < BLOCKSIZE/4) 
					{
		               	int block = *(linkindirect + j);
						
						// ensure the address validity
		                if (block != 0) 
						{
			            	// check  bad address 
			                if (block < ((int)BBITBLOCK(su_block->numberOfBlocks, su_block->numberOfIndoes))+1) 
							{
								fprintf(stderr, "ERROR: bad indirect address in inode.\n");
			                    return 1;
			                }
							// check single use of used blocks 
			                if (blockinuse[block] == 1) 
							{
								fprintf(stderr, "ERROR: indirect address used more than once.\n");
								return 1;
			                }
		                    blockinuse[block] = 1;
							// check bitmap for used blocks 
				            int bitmapLocation = (*((char*)bitmap + (block >> 3)) >> (block & 7)) & 1;
			                if (bitmapLocation == 0) 
							{
			                    fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
			                    return 1;
			                }
		                }
						j++;
		            }
		        }

			}
			inode++;
		}
		i++;
    }

   numberOfLinks[1]++;
	
    // check if used blocks are marked 
    int q = 0;
    while(q < su_block->size) {
        int bitmapLocation = (*((char*)bitmap + (q >> 3)) >> (q & 7)) & 1;
        if (bitmapLocation == 1) {
            if (blockinuse[q] == 0) {
                fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
                return 1;
            }
        }
		q++;
    }

	inode = (struct ondiskinode *)(image + 2*BLOCKSIZE);
	i = 0;
    while (i < su_block->numberOfIndoes) 
	{
		int ntype = inode->type;
        if (ntype == 0) 
		{
            usedInodes[i] = 0;
        }
		else {
			usedInodes[i] = 1;
		}
		// check for reference to a directory
		if (i != 1 && usedInodes[i] == 1 && inodeDir[i] == 0) 
		{
			fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
            return 1;
        }
		// check inode table
        if (i != 1 && inodeDir[i] == 1 && usedInodes[i] == 0) 
		{
			fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
            return 1;
        }
		// make sure extra not links allowed for the directories
        if (i != 1 && ntype == 1 && numberOfLinks[i] != 1) 
		{
            fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
            return 1;
        }
		// make sure hard links to file match files reference count
        if (i != 1 && numberOfLinks[i] != inode->numberOfLinks) 
		{
			fprintf(stderr, "ERROR: bad reference count for file.\n");
            return 1;
		}
		i++;
		inode++;
    }

    return 0;
}
