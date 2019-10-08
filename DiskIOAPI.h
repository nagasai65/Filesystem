#include<stdio.h>

void init(char *filename, int block_size);
void format(FILE *disk, int block_size);
void writeBlock(FILE *disk, void *buffer, unsigned int block_id);
void readBlock(FILE *disk, void *buffer, unsigned int block_id);