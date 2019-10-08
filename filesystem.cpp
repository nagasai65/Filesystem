#include "includeheaders.h"
#include "structures.h"
#include "DiskIOAPI.h"
char diskname[50];
void error(char *msg)
{
	printf("%s\npress ANY KEY to EXIT:", msg);
	getchar();
	exit(0);
}
char check(char *str1, char *str2)
{
	while (str1[0] == str2[0] && str1[0]!='\0')
	{
		str1++;
		str2++;
	}
	if (str1[0] == '\0' && str2[0] == '\0')
		return 1;
	return 0;
}
void seperateString(char *input_string, char *str1, char *str2)
{
	int index = 0;
	while (input_string[0] != ' ' && input_string[0] != '\0')
	{
		str1[index++] = input_string[0];
		input_string++;
	}
	str1[index] = '\0';
	if (input_string[0] == '\0')
	{
		str2[0] = '\0';
		return;
	}
	index = 0;
	input_string++;
	while (input_string[0] != '\0')
	{
		str2[index++] = input_string[0];
		input_string++;
	}
	str2[index] = '\0';
}
void mountFun(char *filename, int block_size)
{
	init(filename, block_size);
	sprintf(diskname, "%s", filename);
}
void formatFun(unsigned int blocksize)
{
	FILE *disk = fopen(diskname, "rb+");
	if (disk == NULL)
		error("Unable to open Disk");
	format(disk, blocksize);
	fclose(disk);
}
struct MetaData* getMetaData()
{
	FILE *disk = fopen(diskname, "rb+");
	if (disk == NULL)
		error("Unable to open disk");
	void *buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
		error("UNable to allocate memory");
	readBlock(disk, buffer, 0);
	struct MetaData *metadata = (struct MetaData*)malloc(sizeof(struct MetaData));
	if (metadata == NULL)
		error("Unable to allocate Memory");
	memcpy(metadata, buffer, sizeof(struct MetaData));
	free(buffer);
	fclose(disk);
	return metadata;
}
unsigned int getNoofFilesCanInBlock()
{
	return BLOCK_SIZE / sizeof(struct INODE);
}
void listFiles(FILE *disk, struct MetaData *metadata)
{
	if (metadata->noof_files == 0)
	{
		printf("Dont have Files\n");
		return;
	}
	void *buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
		error("Unable to allocate memory");
	int blockno, i;
	unsigned int nooffiles = metadata->noof_files;
	INODE *inode = (struct INODE*)malloc(sizeof(struct INODE));
	if (inode == NULL)
		error("Unable to allocate memory");
	printf("Total %d file(s)\n", nooffiles);
	blockno = 1;
	unsigned int perblockcount = getNoofFilesCanInBlock();
	while (nooffiles)
	{
		readBlock(disk, buffer, blockno);
		i = 0;

		while (i != perblockcount && nooffiles) 
		{
			memcpy(inode, (char*)buffer + sizeof(struct INODE)*i, sizeof(struct INODE));
			printf("%s\n", inode->filename);
			i++;
			nooffiles--;
		}
		blockno++;
	}
	free(inode);
	free(buffer);
}
void LSFun()
{
	FILE *disk = fopen(diskname, "rb+");
	if (disk == NULL)
		error("Unable to open Disk");
	struct MetaData* metadata = getMetaData();
	listFiles(disk, metadata);
	free(metadata);
	fclose(disk);
}
long long int getFileSize(FILE *file)
{
 	fseek(file, 0L, SEEK_END);
	return ftell(file);
}
//returns noof blocks required for this file
unsigned int getNoofBlocksReq(long long int filesize)
{
	if (filesize%BLOCK_SIZE)
		return filesize / BLOCK_SIZE + 1;
	return filesize / BLOCK_SIZE;
}
//returns -1 when there are no available blocks
//otherwise returns first free block
unsigned int getStartingFreeBlock(struct MetaData *metadata, unsigned int noofblocks)
{
	if (metadata->noof_empty_blocks < noofblocks)
		return -1;
	for (int i = 0; i < metadata->noof_blocks - metadata->noof_meta_blocks; i++)
	{
		if (metadata->free_blocks_list[i])
			return metadata->noof_meta_blocks + i;
	}
}
void writeFileStruct(FILE *disk,struct INODE *inode, unsigned int nooffiles)
{
	unsigned int perblockcount,blocknum;
	perblockcount = getNoofFilesCanInBlock();
	blocknum = nooffiles / perblockcount + 1; //+1 b/c metadata has one block
	void *buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
		error("Unable to allocate memory");
	readBlock(disk, buffer, blocknum);
	int move = sizeof(struct INODE)*(nooffiles % perblockcount);
	memcpy((char*)buffer + move , inode, sizeof(struct INODE));
	writeBlock(disk, buffer, blocknum);
	free(buffer);
}
void copyFileToFS(FILE *disk, FILE *src, struct MetaData *metadata,char *destfile,long long int filesize,unsigned int noofblocksreq)
{
	struct INODE *inode = (struct INODE*)malloc(sizeof(struct INODE));
	if (inode == NULL)
		error("Unable to allocate memory");

	unsigned int freeblockid = getStartingFreeBlock(metadata, noofblocksreq);
	sscanf(destfile, "%s", inode->filename);
	inode->filesize = filesize;
	inode->noofblocks = noofblocksreq;
	inode->starting_block = freeblockid;
	writeFileStruct(disk,inode, metadata->noof_files);
	void *buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
		error("Unable to allocate memory");
	fseek(src, 0, SEEK_SET);
	while (filesize)
	{
		if (filesize >= BLOCK_SIZE)
		{
			fread(buffer, BLOCK_SIZE, 1, src);
			filesize -= BLOCK_SIZE;
		}
		else
		{
			fread(buffer, filesize, 1, src);
			filesize = 0;
		}
		
		writeBlock(disk, buffer, freeblockid);
		metadata->free_blocks_list[freeblockid - metadata->noof_meta_blocks] = 0;
		metadata->noof_empty_blocks--;
		freeblockid = getStartingFreeBlock(metadata, 1);
	}
	metadata->noof_files++;
	printf("Copied\n");
	free(buffer);
	free(inode);
	return;
}
void COPYTOFSFun(char *srcfile,char *destfile)
{
	FILE *src = fopen(srcfile, "rb");
	if (src == NULL)
		error("Unable to read source file");
	long long int filesize = getFileSize(src);
	unsigned int noofblocksreq = getNoofBlocksReq(filesize);
	FILE *disk = fopen(diskname, "rb+");
	if (disk == NULL)
		error("Cant open Disk");
	struct MetaData *metadata = getMetaData();
	copyFileToFS(disk, src, metadata, destfile, filesize, noofblocksreq);
	void *buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
		error("Unable to allocate memory");
	memcpy(buffer, metadata, sizeof(struct MetaData));
	writeBlock(disk, buffer, 0);
	free(metadata);
	free(buffer);
	fclose(disk);
	fclose(src);
}
struct INODE* checkFileInDisk(FILE *disk, char *filename)
{
	struct MetaData* metadata = getMetaData();
	if (metadata->noof_files == 0)
		return NULL;
	void *buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
		error("Unable to allocate memory");
	struct INODE *inode = (struct INODE*)malloc(sizeof(struct INODE));
	if (inode == NULL)
		error("Unable to allocate memory");
	unsigned int nooffiles = metadata->noof_files;
	unsigned int blockno = 1;
	int i,perblockcount;
	perblockcount = getNoofFilesCanInBlock();

	while (nooffiles)
	{
		i = 0;
		readBlock(disk, buffer, blockno);

		while (i != perblockcount && nooffiles)
		{
			memcpy(inode,(char*) buffer + (i*sizeof(struct INODE)), sizeof(struct INODE));
			if (check(inode->filename, filename))
			{
				free(buffer);
				free(metadata);
				return inode;
			}
			i++;
			nooffiles--;
		}
		blockno++;
	}
	free(buffer);
	free(metadata);
	free(inode);
	return NULL;
}
void writeBlockToFile(FILE *disk, FILE *dest, unsigned int  blockNo, unsigned int size)
{
	void *buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
	{
		error("Unable to allocate memory");
	}
	readBlock(disk, buffer, blockNo);
	fwrite(buffer, size, 1, dest);
	free(buffer);
}
void copyFileFromFS(FILE *disk, struct INODE *inode, char *destfile)
{
	unsigned int noofblocks, blockno, filesize;
	noofblocks = inode->noofblocks;
	blockno = inode->starting_block;
	filesize = inode->filesize;
	FILE *dest = fopen(destfile, "w");
	if (dest == NULL)
	{
		printf("Unable to Create dest file");
		return;
	}
	while (noofblocks)
	{
		if (filesize >= BLOCK_SIZE)
		{
			writeBlockToFile(disk, dest, blockno, BLOCK_SIZE);
			filesize -= BLOCK_SIZE;
			blockno++;
		}
		else
		{
			writeBlockToFile(disk, dest, blockno, filesize);
		}
		noofblocks--;
	}
	fclose(dest);
}
void COPYFROMFSFun(char *srcfile, char *destfile)
{
	FILE *disk = fopen(diskname, "rb+");
	if (disk == NULL)
		error("Unable to open Disk");
	struct INODE *inode;
	inode = checkFileInDisk(disk,srcfile);
	if (inode == NULL)
	{
		printf("Src File Specified Not Found\n");
	}
	else
	{
		copyFileFromFS(disk, inode, destfile);
	}
	free(inode);
	fclose(disk);
}
char callTargetFunction(char *command, char *str)
{
	char *str1;
	str1 = (char*)malloc(50);
	if (str1 == NULL)
		error("unable to allocate memory");

	if (check(command, "MOUNT"))
	{
		seperateString(str, str1, command);
		unsigned int bs = 0;
		while (command[0] != '\0')
		{
			bs = bs * 10 + (command[0] - '0');
			command++;
		}
		mountFun(str1, bs);
	}
	if (check(command, "FORMAT"))
	{
		unsigned int blocksize = 0;
		while (str[0] != '\0')
		{
			blocksize = blocksize * 10 + (str[0] - '0');
			str++;
		}
		formatFun(blocksize);
	}
	if (check(command, "LS"))
	{
		LSFun();
	}
	if (check(command, "COPYTOFS"))
	{
		seperateString(str, str1, command);
		COPYTOFSFun(str1,command);
	}
	if (check(command, "COPYFROMFS"))
	{
		seperateString(str, str1, command);
		COPYFROMFSFun(str1, command);
	}
	if (check(command, "exit"))
		return 1;
	free(str1);
	return 0;
}
char tokenize(char *input_string)
{
	char *str1, *str2;
	str1 = (char*)malloc(50);
	str2 = (char*)malloc(50);
	if (str1 == NULL || str2 == NULL)
		error("Unable to allocate Memory");
	seperateString(input_string, str1, str2);
	char returnval = callTargetFunction(str1, str2);
	free(str1);
	free(str2);
	return returnval;
}
int main()
{
	char *input_command;
	input_command = (char*)malloc(100);
	if (input_command == NULL)
		error("Unable to allocate Memory");

	while (1)
	{
		printf(">");
		gets(input_command);
		if (tokenize(input_command))
			break;
	}
	free(input_command);
	return 0;
}