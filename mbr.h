#ifndef _MBR_H_
#define _MBR_H_

#define PART_INFO_OFFSET        446
#define MBR_TAG_OFFSET          510

struct _dev_part {
    u8 active_flag;         //引导指示符 0x80 为操作系统引导 通常为0
    u8 start_head;
    u16 start_cyl_secter; //前6位开始扇区，后10位开始柱面
    u8 sys_id;             //文件系统ID
    u8 end_head;
    u16 end_cyl_secter; //前6位结束扇区，后10位结束柱面
    u32 start_secter;
    u32 totle_sector;
};

typedef struct _mbr_info {
    void *dev;
    u32  dev_block_size;
    struct _dev_part dev_part[4];
    u8  part_num;
} _mbr_info;

enum {
    MBR_NO_ERR = 0x00,
    MBR_DEV_OP_ERR,
    MBR_PTR_NULL,
    MBR_TAG_NO_MATCH,
    MBR_NO_PART,
};

int __mbr_scan(void *dev, _mbr_info *mbr_info);
#endif
