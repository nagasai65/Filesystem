#ifndef structures_h
#define structures_h
#include"includeheaders.h"
struct INODE
{
	char filename[FILE_NAME_LENGTH];
	long long int filesize;
	unsigned int noofblocks;
	unsigned int starting_block;
};
struct MetaData
{
	int magicnum;
	unsigned int block_size;
	unsigned int noof_blocks;
	unsigned int noof_empty_blocks;
	unsigned int noof_meta_blocks;
	unsigned int noof_files;
	char free_blocks_list[NOOFBLOCKS - NOOF_METABLOCKS];
};
#endif