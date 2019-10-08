#include "includeheaders.h"
#include "structures.h"

static void error(char *msg)
{
	printf("%s\nPress ANY KEY to EXIT:",msg);
	getchar();
	exit(0);
}

void readBlock(FILE *disk, void *buffer, unsigned int block_id)
{
	fseek(disk, 0, SEEK_SET);
	fseek(disk, block_id * BLOCK_SIZE, SEEK_SET);
	if (fread(buffer, BLOCK_SIZE, 1, disk) == NULL)
	{
		error("Error Reading a block from Disk");
	}
}
void writeBlock(FILE *disk, void * buffer, unsigned int block_id)
{
	fseek(disk, block_id * BLOCK_SIZE, SEEK_SET);
	if (fwrite(buffer, BLOCK_SIZE, 1, disk)==NULL)
	{
		error("Error Writing a block to Disk");
	}
}
void format(FILE *disk, int block_size)
{
	struct MetaData *metadata;
	metadata = (struct MetaData*)malloc(sizeof(struct MetaData));
	metadata->magicnum = MAGICNUM;
	metadata->block_size = block_size;
	metadata->noof_blocks = NOOFBLOCKS;
	metadata->noof_empty_blocks = NOOFBLOCKS - NOOF_METABLOCKS;
	metadata->noof_meta_blocks = NOOF_METABLOCKS;
	metadata->noof_files = 0;
	for (int i = 0; i < NOOFBLOCKS - NOOF_METABLOCKS; i++)
	{
		metadata->free_blocks_list[i] = 1;
	}
	void *buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
		error("Unable to allocate Memory");
	memcpy(buffer, metadata, sizeof(struct MetaData));
	writeBlock(disk, buffer, 0);
	free(buffer);
	free(metadata);
}
void validateFS(FILE *disk, struct MetaData *metadata, int block_size)
{
	//checking MagicNUM
	if (metadata->magicnum != MAGICNUM || metadata->block_size != block_size)
	{
		printf("Error in File System\nDo you want to Format it(y/n):");
		char choice;
		choice = getchar();
		if (choice != 'y' && choice != 'Y')
			error("Dobbey ayithey");
		format(disk, block_size);
	}
}
void init(char *filename, int block_size)
{
	FILE *disk;
	disk = fopen(filename, "rb+");
	if (disk == NULL)
	{
		error("Unable to open Disk");
	}
	void *buffer;
	buffer = malloc(BLOCK_SIZE);
	if (buffer == NULL)
	{
		error("Unable to allocate memory");
	}
	readBlock(disk, buffer, 0);
	struct MetaData *metadata;
	metadata = (struct MetaData*)malloc(sizeof(struct MetaData));
	memcpy(metadata, buffer, sizeof(struct MetaData));
	validateFS(disk, metadata, block_size);
	free(metadata);
	free(buffer);
	fclose(disk);
}