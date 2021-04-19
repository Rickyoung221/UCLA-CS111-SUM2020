#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "ext2_fs.h"

int fs_image;

struct ext2_super_block sb;
struct ext2_group_desc group;
struct ext2_dir_entry dir;

__u32 BLOCKSIZE;
__u32 inodeSize;
__u32 numBlocks;
__u32 numInodes;
__u32 block_bitmap;
__u32 inode_bitmap;
__u32 firstBlockInode;


void Pread(int fd,void *buf, size_t count, off_t offset){
    int ret =  pread(fd, buf, count, offset);
    if (ret < 0){
        fprintf(stderr, "Eorro occurs at pread \n");
        exit(1);
    }
}


char* format_time(__u32 time) {
    char* formattedDate = malloc(sizeof(char)*32); //random value
    time_t rawtime = time;
    struct tm* info = gmtime(&rawtime);
    strftime(formattedDate, 32, "%m/%d/%y %H:%M:%S", info);
    return formattedDate;
}


bool is_block_free(int block_no){    // bit of 1 means "used" and 0 means 'free'
    if (block_no == 0)
        return false; 

    int block_bitmap_offset = block_bitmap * BLOCKSIZE;
    int bit_to_read = (block_no -1 )% 8;        
    uint8_t byte_to_read = (block_no-1) / 8;  

    unsigned char read;
    Pread(fs_image, &read, sizeof(char), block_bitmap_offset + byte_to_read);

    int bit_value = ((read >> bit_to_read) & 1);
    if(bit_value == 0){
        return true;
    }
    else{
        return false;
    }
}

bool is_inode_free(int inode_no){    // bit of 1 means "used" and 0 means 'free'
    int inode_bitmap_offset = inode_bitmap * BLOCKSIZE;
    int bit_to_read = (inode_no - 1) % 8;        
    uint8_t byte_to_read = (inode_no - 1) / 8;  

    unsigned char read;
    Pread(fs_image, &read, sizeof(char), inode_bitmap_offset + byte_to_read);

    int bit_value = ((read >> bit_to_read) & 1);
    if(bit_value == 0){
        return true;
    }
    else{
        return false;
    }
}

void print_free_block_entries() {
    __u32 i;
    // scan through the block bitmap 
    for(i = 0; i<numBlocks; i++){
        if(is_block_free(i)){
            fprintf(stdout, "BFREE,%d\n", i); 
        }
    }
}

void print_free_inode_entries(){
    uint32_t i;
    // scan through and inode bitmap 
    for(i = 1; i <= numInodes; i++){
        if(is_inode_free(i)){
            fprintf(stdout, "IFREE,%d\n", i);
        }
    }
}

void print_directory_entries(uint32_t starting, uint32_t parentNum)
{
    uint32_t current = starting;
    while(current < starting + BLOCKSIZE)
    {
        Pread(fs_image, &dir, sizeof(struct ext2_dir_entry), current);
        uint32_t parent = parentNum;
        uint32_t logical = current - starting;
        uint32_t inodeNumber = dir.inode;
        uint32_t entryLength = dir.rec_len;
        uint32_t nameLength = dir.name_len;
        current += entryLength;
        if(inodeNumber == 0)
            continue;
        fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,",
            parent, //parent inode number (decimal) ... the I-node number of the directory that contains this entry
            logical, //logical byte offset (decimal) of this entry within the directory
            inodeNumber, // inode number of the referenced file (decimal)
            entryLength, // entry length (decimal)
            nameLength); // name length (decimal)

        // name (string, surrounded by single-quotes)
        fprintf(stdout, "'");
        for(uint32_t i = 0; i < nameLength; i++)
            fprintf(stdout, "%c", dir.name[i]);
        fprintf(stdout, "'\n");
    }
}

void print_inode_directory_entries(uint32_t inode_offset, int parentNum)
{
    // directory
    int buffer;
    int OFFSET; 
    for(uint32_t j = 0; j < 12; j++)
    {
        Pread(fs_image, &buffer, 4, inode_offset + 40 + j * 4);
        if(buffer != 0)
        {
            OFFSET = BLOCKSIZE * buffer;
            print_directory_entries(OFFSET, parentNum);
        }   
    }

    int indir_1;
    int indir_2;
    int indir_3;
    // single indirectory 
    Pread(fs_image, &indir_1, 4, inode_offset + 40 + 48);
    if(indir_1 != 0)
    {
        for(uint32_t j = 0; j < 256; j++)
        {
            Pread(fs_image,&buffer, 4, indir_1 * BLOCKSIZE + j * 4);
            OFFSET = BLOCKSIZE * buffer;
            if(buffer != 0)
            {
                print_directory_entries(OFFSET, parentNum);           
            }   
        }
    }   

    //doubly indirectory 
    Pread(fs_image, &indir_2, 4, inode_offset + 40 + 52);
    if(indir_2 != 0)
    {
        for(uint32_t k = 0; k < 256; k++)
        {
            Pread(fs_image, &indir_1, 4, indir_2 * BLOCKSIZE + k * 4);
            if(indir_1 != 0)
            {
                for(uint32_t j = 0; j < 256; j++)
                {
                    
                    Pread(fs_image, &buffer, 4, indir_1 * BLOCKSIZE + j * 4);
                    OFFSET = BLOCKSIZE * buffer;
                    if(buffer != 0)
                        print_directory_entries(OFFSET, parentNum);
                }
            }
        }
    }        

    // triple indirectory 
    Pread(fs_image,&indir_3, 4, inode_offset + 40 + 56);
    if(indir_3 != 0)
    {
        for(uint32_t m = 0; m < 256; m++)
        {
            Pread(fs_image, &indir_2, 4, indir_3 * BLOCKSIZE + m * 4);
            if(indir_2 != 0)
            {
                for(uint32_t k = 0; k < 256; k++)
                {
                    Pread(fs_image, &indir_1, 4, indir_2 * BLOCKSIZE + k * 4);
                    if(indir_1 != 0)
                    {
                        for(uint32_t j = 0; j < 256; j++)
                        {
                            Pread(fs_image, &buffer, 4, indir_1 * BLOCKSIZE + j * 4);
                            OFFSET = BLOCKSIZE * buffer;
                            if(buffer != 0)
                                print_directory_entries(OFFSET, parentNum);
                        }
                    }
                }
            }
        }
    }
}


char get_file_type(struct ext2_inode inode){
        char fileType = '?';
        if(S_ISDIR(inode.i_mode)){
            fileType = 'd';
        }
        else if(S_ISREG(inode.i_mode)){
            fileType = 'f';
        }
        else if(S_ISLNK(inode.i_mode)){
            fileType = 's';
        }
        return fileType;
}

void  print_indirect_block_references(int inodeNumber, struct ext2_inode inode){
    uint32_t blockValue;
    if(inode.i_block[12] != 0){   // direct block
        uint32_t block_num_L1 = inode.i_block[12];
        uint32_t blockOffset = block_num_L1 * BLOCKSIZE;
        for(uint32_t i = 0; i < 256; i++) // "there would be 256 indirect blocks per doubly-indirect block"
        {
            Pread(fs_image, &blockValue, sizeof(uint32_t), blockOffset + (4*i) );
            if(blockValue)
                fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n",
                 inodeNumber, // I-node number of the owning file (decimal)
                 1,           // (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
                 12 + i,      // logical block offset (decimal) 
                 block_num_L1, // block number of the (1, 2, 3) indirect block being scanned (decimal) 
                 blockValue); // block number of the referenced block (decimal)
        }
    }

    if(inode.i_block[13] != 0){   // doubly indirect block
        uint32_t block_num_L2 = inode.i_block[13];
        uint32_t block_offset_L1 = block_num_L2 * BLOCKSIZE;
        uint32_t blockOffset;
        uint32_t block_value_L1;

        for(uint32_t i = 0; i < 256; i++)
        {
            Pread(fs_image, &block_value_L1, sizeof(blockValue), block_offset_L1 + i * 4);

            blockOffset = block_value_L1 * BLOCKSIZE;
            if(block_value_L1 != 0)
            {
                fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", 
                    inodeNumber,
                    2, 
                    12 + 256 + i * 256, 
                    block_num_L2, 
                    block_value_L1);

                for(uint32_t j = 0; j < 256; j++)
                {
                    Pread(fs_image, &blockValue, sizeof(blockValue), blockOffset + j * 4);
                    if(blockValue)
                        fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", 
                            inodeNumber, 
                            1, 
                            12 + 256 + i * 256 + j, 
                            block_value_L1, 
                            blockValue);
                }
            }
        }
    }

    if(inode.i_block[14] != 0){   // triply indirect block 
        uint32_t block_num_3 = inode.i_block[14];
        uint32_t block_offset_L3 = block_num_3 * BLOCKSIZE;
        uint32_t num_scaned_blocks = 12 + 256 + 256 * 256;
        for(uint32_t i = 0; i < 256; i++)
        {
            //Read each entry in L3
            uint32_t block_offset_L2, block_number_L2;
            Pread(fs_image, &block_number_L2, sizeof(block_number_L2), block_offset_L3 + i * 4);
            block_offset_L2 = block_number_L2 * BLOCKSIZE;
            if(block_number_L2 != 0)
            {
                fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", 
                    inodeNumber, 
                    3, 
                    num_scaned_blocks + i * (BLOCKSIZE * BLOCKSIZE / 8), 
                    block_num_3, 
                    block_number_L2);

                for(uint32_t j = 0; j < 256; j++)
                {
                    //Read each entry in L2
                    uint32_t block_Offset_L1, block_number_L1;
                    Pread(fs_image, &block_number_L1, sizeof(block_number_L1), block_offset_L2 + j * 4);
                    block_Offset_L1 = block_number_L1 * BLOCKSIZE;
                    if(block_number_L1 != 0)
                    {
                        fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", 
                            inodeNumber, 
                            2, 
                            num_scaned_blocks + i * (BLOCKSIZE * BLOCKSIZE / 8) + j * (256),
                            block_number_L2, 
                            block_number_L1);
                        for(uint32_t k = 0; k < 256; k++)
                        {
                            uint32_t dataBlockNumber;
                            Pread(fs_image, &dataBlockNumber, sizeof(dataBlockNumber), block_Offset_L1 + k * 4);
                            if(dataBlockNumber)
                                fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", 
                                    inodeNumber, 
                                    1, 
                                    num_scaned_blocks + i *(BLOCKSIZE * BLOCKSIZE / 8) + j * (256) + k, 
                                    block_number_L1, 
                                    dataBlockNumber);
                        }
                    }
                }
            }
        }
    }
}


void print_inode_summary() {
    // bit value 1 is "used", 0 is "free"
    uint32_t i = 0;
    uint32_t j = 0;
    char fileType = '?';

    for(i=0; i < numInodes; i++){

        if(is_inode_free(i)) // We don't print the csv for free inode 
            continue;

        // scan through all inode entries 
        struct ext2_inode inode;
        Pread(fs_image, &inode, inodeSize, 1024 + (firstBlockInode-1) * BLOCKSIZE + i * sizeof(struct ext2_inode));
     
        if(inode.i_mode == 0 || inode.i_links_count == 0){
            continue;
        }
        fileType = get_file_type(inode);
        if (fileType == 'd'){
            print_inode_directory_entries(1024 +  (firstBlockInode - 1) * (BLOCKSIZE) + i * sizeof(struct ext2_inode), i+1);
        }

        uint16_t imode = inode.i_mode & 0xFFF; 
        uint16_t owner = inode.i_uid;
        uint16_t group = inode.i_gid;
        uint16_t linksCount = inode.i_links_count;
        char* changeDate = format_time(inode.i_ctime);
        char* modificationDate = format_time(inode.i_mtime);
        char* accessDate = format_time(inode.i_atime);
        uint32_t fileSize = inode.i_size;
        uint32_t numBlocks = inode.i_blocks; 

        fprintf(stdout, "INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%d,%d",
         i+1, //inode number (decimal)
         fileType,    // file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
         imode,       // mode (low order 12-bits, octal ... suggested format "%o")
         owner,       // owner (decimal)
         group,       // group (decimal)       
         linksCount,    // link count (decimal)
         changeDate,    // time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
         modificationDate, // modification time (mm/dd/yy hh:mm:ss, GMT)
         accessDate,    // time of last access (mm/dd/yy hh:mm:ss, GMT)
         fileSize,  // file size (decimal)
         numBlocks);    //number of (512 byte) blocks of disk space (decimal) taken up by this file
        

        if(!(fileType == 's' && fileSize < 60))
            for(j = 0; j < 15; j++){
                fprintf(stdout, ",%u", inode.i_block[j]);
            }
        else
            fprintf(stdout, ",%u", inode.i_block[0]);
        fprintf(stdout,"\n");

        if(!(fileType == 's' && fileSize < 60))
        {
            print_indirect_block_references(i+1, inode);
        }        
    }  
} 


void print_inode_summaryy() {

    uint32_t i = 0;
    uint32_t j = 0;
    char fileType = '?';

    for(i = 0; i < numInodes; i++){

        if(is_inode_free(i))
            continue;

        // scan through all inode entries 
        struct ext2_inode inode;
        Pread(fs_image, &inode, inodeSize, 1024 +  (firstBlockInode - 1) * (BLOCKSIZE) + i * sizeof(struct ext2_inode));
      
        if(inode.i_mode == 0 || inode.i_links_count == 0){
            continue;
        }

        fileType = get_file_type(inode);
        if (fileType == 'd'){
            print_inode_directory_entries(1024 +  (firstBlockInode - 1) * (BLOCKSIZE) + i * sizeof(struct ext2_inode), i + 1);
        }
        uint16_t imode = inode.i_mode & 0xFFF; //
        uint16_t owner = inode.i_uid;
        uint16_t group = inode.i_gid;
        uint16_t linksCount = inode.i_links_count;
        char* changeDate = format_time(inode.i_ctime);
        char* modificationDate = format_time(inode.i_mtime);
        char* accessDate = format_time(inode.i_atime);
        uint32_t fileSize = inode.i_size;
        uint32_t numBlocks = inode.i_blocks; 

        fprintf(stdout, "INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%d,%d",
         i+1, //inode number (decimal)
         fileType,    // file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
         imode,       // mode (low order 12-bits, octal ... suggested format "%o")
         owner,       // owner (decimal)
         group,       // group (decimal)       
         linksCount,    // link count (decimal)
         changeDate,    // time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
         modificationDate, // modification time (mm/dd/yy hh:mm:ss, GMT)
         accessDate,    // time of last access (mm/dd/yy hh:mm:ss, GMT)
         fileSize,  // file size (decimal)
         numBlocks);    //number of (512 byte) blocks of disk space (decimal) taken up by this file

         //fprintf(stdout, "ERROR\n");
        if(!(fileType == 's' && fileSize < 60)){
            for(j = 0; j < 15; j++){
                fprintf(stdout, ",%u", inode.i_block[j]);
            }
        }
        fprintf(stdout,"\n");

        if(!(fileType == 's' && fileSize < 60)){
            print_indirect_block_references(i+1, inode);
        }
    }
} //first non-reserved inode is 11 -> check later

void print_superblock_summary() {
    fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", 
        sb.s_blocks_count,  //total number of blocks (decimal)  1024
        sb.s_inodes_count,  //total number of i-nodes (decimal) 1000
        BLOCKSIZE,  //block size (in bytes, decimal)     1024
        inodeSize,  //i-node size (in bytes, decimal)    128
        sb.s_blocks_per_group, //blocks per group (decimal)    8192
        sb.s_inodes_per_group, //i-nodes per group (decimal)   1000
        sb.s_first_ino); //first non-reserved i-node (decimal) 11 
}// slice 8 


void print_group_summary() { 
    fprintf(stdout, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
     0,             // group number (decimal, starting from zero)
     sb.s_blocks_count,     // total number of blocks in this group (decimal), there is only one group 
     sb.s_inodes_count,     // total number of i-nodes in this group (decimal), threr is only one group 
     group.bg_free_blocks_count, // number of free blocks (decimal)  
     group.bg_free_inodes_count, // number of free i-nodes (decimal)
     group.bg_block_bitmap,  // 3 block number of free block bitmap (block id)for this group (decimal)
     group.bg_inode_bitmap,  // 4 block number of free i-node bitmap for this group (decimal)
     group.bg_inode_table); // 5 block number of first block of i-nodes in this group (decimal)
}


void open_file_system(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "%s\n", "Incorrect arguments");
        exit(1);
    }
    fs_image = -1; 
    if((fs_image = open(argv[1], O_RDONLY)) < 0)
    {
        fprintf(stderr, "%s%s\n", argv[1] , " is not a legal image\n" );
        exit(2);
    }

    int superblock_offset = 1024; 
    // SuperBlock located after the boot block, starting at 1024 bytes offset from the beginning of the disk. 
    int block_group_descriptor_offset = 1024 + sizeof(struct ext2_super_block);
    //"The block group descriptor table starts on the first block following the superblock.""
    //https://www.nongnu.org/ext2-doc/ext2.html#block-group-descriptor-table

    Pread(fs_image, &sb, sizeof(struct ext2_super_block), superblock_offset);
    Pread(fs_image, &group, sizeof(struct ext2_group_desc), block_group_descriptor_offset);
    BLOCKSIZE = 1024 << sb.s_log_block_size;  // calculate block size in bytes  1024 
    inodeSize = sb.s_inode_size;              // 128

    numBlocks = sb.s_blocks_count;
    numInodes = sb.s_inodes_count;
    block_bitmap = group.bg_block_bitmap;
    inode_bitmap = group.bg_inode_bitmap;
    firstBlockInode = group.bg_inode_table;

}

int main(int argc, char** argv){

    open_file_system(argc, argv);
    print_superblock_summary();
    print_group_summary();
    print_free_block_entries();
    print_free_inode_entries();
    print_inode_summaryy();
  
    exit(0);
}










