#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

//Block 0 = unused
//Block 1 = super
//Block 2 = start of Inodes

// Create boolean
#define TRUE 0
#define FALSE 1

// Root inode
#define ROOTINO 1 
// Block size
#define BSIZE 512 
// Number of direct links
#define NDIRECT 12
// Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))
// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)
// Bitmap bits per block
#define BPB           (BSIZE*8)
// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)
// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

// File system super block
struct superblock {
    uint size;			//fs size (blocks)
    uint nblocks;  		//number of blocks
    uint ninodes;  		//number of inodes
};

// On-disk inode structure
struct dinode {
    short type; 			//file type
    short major; 			//major device number
    short minor; 			//minor device number
    short nlink; 			//number of links to inode
    uint size; 				//size of file(bytes)
    uint addrs[NDIRECT+1];	//data block addresses
};

// Directory entry
struct dirent {
    ushort inum;		//inode number
    char name[DIRSIZ];	//directory name
};

// unused | superblock | inode table | bitmap (data) | data blocks

int main(int argc, char *argv[]) {
	char *filename = argv[1];

	if (argc != 2) {
		fprintf(stderr, "Usage: xcheck <file_system_image>\n");
		exit(1);
	}
    	int fd = open(filename, O_RDONLY);
    	// Check 0: bad file input
    	if (fd <= -1) {
        	fprintf(stderr, "image not found.\n");
        	return 1;
    	}

    	struct stat sbuf;
	
    	fstat(fd, &sbuf);
    
    	void *image = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    
    	struct superblock *sb = (struct superblock *)(image + BSIZE);
	
    	//inodes
	int blockAddr;



	// First inode block in list of inodes
    	struct dinode *inode = (struct dinode *)(image + 2*BSIZE);
	// Pointer to bitmap location
    	void *bitmap = (void *)(image + (sb->ninodes/IPB + 3)*BSIZE);
    	// List of used blocks
    	int usedBlocks[sb->size];
	// Set all used blocks to unused
        for(int i = 0; i < sb->size; i++ )
                usedBlocks[i] = 0;

        // Set used blocks to used in list
        for (int i = 0; i < (sb->ninodes/IPB + 3 + (sb->size/(BSIZE*8)+1)); i++)
                usedBlocks[i] = 1;
	
	// Number of links to inode in file system
    	int numLinks[sb->ninodes];
	for (int i = 0; i < sb->ninodes; i++)
                numLinks[i] = 0;
	
	// List of used inodes
    	int inodeUsed[sb->ninodes];
	// Does this inode found in a directory
    	int inodeDirectory[sb->ninodes];
	for (int i = 0; i < sb->ninodes; i++)
		inodeDirectory[i] = 0;


	//traverse each inodes
    	for (int i = 0; i < sb->ninodes; i++) {
		struct dirent *currDirectory;
		int type = inode->type;
		// Check 3: valid root inode
        	if (i == 1 && type != 1) {
        		fprintf(stderr, "ERROR: root directory does not exist.\n");
            		return 1; 
        	}
		// Check for valid file type
		if (!(type >= 0 && type <= 3)) {
			// Check 1: valid type
                        fprintf(stderr,"ERROR: bad inode.\n");
                        return 1;
		} else {
			// Make sure it is valid
			if (type != 0) {
//				printf("allocated inode %d\n", i);
				// If directory, check to make sure it is a valid directory
		        	if (type == 1) { //T_DIR
//		            		printf("file node %d\n", i);
					currDirectory = (struct dirent*)(image + inode->addrs[0] * BSIZE);
					// Check 4: valid directory format
		            		if (strcmp(currDirectory->name, ".") != 0) {
		                		fprintf(stderr, "ERROR: directory not properly formatted.\n");
		                		return 1;
		            		}
		            		currDirectory++;
		            		if (strcmp(currDirectory->name, "..") != 0) {
		                		fprintf(stderr, "ERROR: directory not properly formatted.\n");
		                		return 1;
		            		}
		            		// check 3: valid root inode
		            		if (i != 1 && currDirectory->inum == i) {
		                		fprintf(stderr, "ERROR: root directory does not exist.\n");
		                		return 1;
		            		}
					for (int k = 0; k < NDIRECT+1; k++) {
		                		struct dirent *currDir;	
						if (inode->addrs[k] != 0){
							if (k < NDIRECT) {
								currDir = (struct dirent *)(image + inode->addrs[k]*BSIZE);
								for (int z = 0; z < BSIZE/sizeof(struct dirent); z++) {
									if (currDir->inum != 0) {
										if (strcmp(currDir->name,".") != 0 && strcmp(currDir->name,"..") != 0) {
			                        					numLinks[currDir->inum]++;
											inodeDirectory[currDir->inum] = 1;
										}
									}
									currDir++;
			               				}
							} 
							else { //the last in indirect 
								uint *inCurr = (uint *)(image + inode->addrs[k]*BSIZE);
								//for each indirect pointer in block												
								for (int g = 0; g < 128; g++) {
									struct dirent *current = (struct dirent*)(image + inCurr[g]*BSIZE);
									for (int y = 0; y < BSIZE/sizeof(struct dirent); y++) {
										if (current->inum != 0) {
											if (strcmp(current->name,".") != 0 && strcmp(current->name,"..") != 0) {
                                                                         	        	numLinks[current->inum]++;
                                                                        			inodeDirectory[current->inum] = 1;
											}
										}
										current ++;
									}
								}

							}
		            			}
					}
				}
				//for directories and files 
		        	for (int k = 0; k < NDIRECT+1; k++) {
			
					blockAddr = inode->addrs[k];
					// Make sure address is valid
		            		if (blockAddr != 0) {
			            		// Check 2: bad address in inode
				            	if((blockAddr) < ((int)BBLOCK(sb->nblocks, sb->ninodes))+1 || blockAddr > (sb->size * BSIZE)){
				                	if (k < NDIRECT)
								fprintf(stderr, "ERROR: bad direct address in inode.\n");	
							else 
								fprintf(stderr, "ERROR: bad indirect address in inode.\n");
				                	return 1;
				            	}
						// Check 8: check used blocks are only used once
						if(usedBlocks[blockAddr] == 1){
							if (k < NDIRECT)
								fprintf(stderr, "ERROR: direct address used more than once.\n");
							else 
								fprintf(stderr, "ERROR: indirect address used more than once.\n");
							return 1;
						}
						usedBlocks[blockAddr] = 1;
					
			            		// Check 6: check used blocks in bitmap
			            		int bitmapLocation = (*((char*)bitmap + (blockAddr >> 3)) >> (blockAddr & 7)) & 1;
			            		if (bitmapLocation == 0) {
			                		fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
			                		return 1;
			            		}
					}
		        	}
				//there is an indirect link for this file 
		        	if (inode->size > BSIZE * NDIRECT) {
		            		int *indirect = (int *)(image + (blockAddr*BSIZE));
		            		for (int k = 0; k < BSIZE/4; k++) {
		               			int block = *(indirect + k);
						
						// Check if address is valid
		                		if (block != 0) {
			            			// Check 2: bad address in inode
			                		if (block < ((int)BBLOCK(sb->nblocks, sb->ninodes))+1) {
								fprintf(stderr, "ERROR: bad indirect address in inode.\n");
			                    			return 1;
			                		}
							// Check 8: check used blocks are only used once
			                		if (usedBlocks[block] == 1) {
								fprintf(stderr, "ERROR: indirect address used more than once.\n");
								return 1;
			                		}
		                    			usedBlocks[block] = 1;
							// Check 6: check used blocks in bitmap
				            		int bitmapLocation = (*((char*)bitmap + (block >> 3)) >> (block & 7)) & 1;
			                		if (bitmapLocation == 0) {
			                    			fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
			                    			return 1;
			                		}
		                		}
		            		}
		        	}

			}
			inode++;
		}
    }

   numLinks[1]++;
	
    // Check 7: check used blocks are marked bitmap and are used
    int block;
    for(block = 0; block < sb->size; block++) {
        int bitmapLocation = (*((char*)bitmap + (block >> 3)) >> (block & 7)) & 1;
        if (bitmapLocation == 1) {
            if (usedBlocks[block] == 0) {
                fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
                return 1;
            }
        }
    }


		
	
 /*       inode = (struct dinode *)(image + 2*BSIZE);
        for (int i = 0; i < sb->ninodes; i++) {
		printf("i is %d numLinks is %d and nlink is %d\n",i, numLinks[i], inode->nlink);
		inode++;
	}
*/

	inode = (struct dinode *)(image + 2*BSIZE);
    	for (int i = 0; i < sb->ninodes; i++) {
		
		
	int type = inode->type;
        	if (type != 0) {
            		inodeUsed[i] = 1;
        	}
		else {
			inodeUsed[i] = 0;
		}
		// Check 9: check used inodes reference a directory
       	
	if (i != 1 && inodeUsed[i] == 1 && inodeDirectory[i] == 0) {
            //printf("inode number is %d and type is %dn", i, inode->type);
		fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
            return 1;
        }
		// Check 10: check used inodes in directory are in inode table
        if (i != 1 && inodeDirectory[i] == 1 && inodeUsed[i] == 0) {
            
		//printf("inode number is %d\n and inode directory is %d", i, inodeDirectory[i]);		
		fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
            return 1;
        }
		// Check 12: No extra links allowed for directories
        if (i != 1 && type == 1 && numLinks[i] != 1) {
            fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
            return 1;
        }
		// Check 11: hard links to file match files reference count
        if (i != 1 && numLinks[i] != inode->nlink) {
		fprintf(stderr, "ERROR: bad reference count for file.\n");
            return 1;
	}
		inode++;
    }

    return 0;
}
