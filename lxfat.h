#ifndef _LXFAT_H
#define _LXFAT_H

#define FAT_WRITE_EN        1 
#define FAT_TINY            0
#define FAT_LFN_EN          0
#define DIR_ACCELE          0   //目录加速,需要更多BUFF 

#define FAT_SECTOR_SIZE     512
#define FAT_PATH_LEN        128
#define FAT_PATH_DEPTH      9      //支持最大目录深度
#define DIR_ACCELE_BUF_L    128
typedef struct _FAT_DEV_INFO {
    void *dev;
    u32(*dev_read)(void *dev, u8 *buffer, u32 addr, u32 len);
    u32(*dev_write)(void *dev, u8 *buffer, u32 addr, u32 len);
    u32 dev_block_size;
} FAT_DEV_INFO;

//以下是WIN的标志
#define DIRTY                BIT(0)
#define WIN_BUSY             BIT(1)
#define WIN_USE              BIT(7)

/* File attribute bits for directory entry */
#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define	AM_VOL	0x08	/* Volume label */
#define AM_LFN	0x0F	/* LFN entry */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */
#define AM_FCH	0x80    /* exFAT下,文件簇连续标志 */

#define FAT_SEEK_HEAD    0
#define FAT_SEEK_TAIL    1
#define FAT_SEEK_CUR     2

typedef struct _FAT_HDL FAT_HDL;


typedef struct _FAT_WIN {
    FAT_HDL     *fs;
    u32         sector;
    u8          buffer[FAT_SECTOR_SIZE];
    u8          flag;
} FAT_WIN;

typedef struct _FAT_DIR {
    u32 st_clust;
    u32 cur_clust;
#if DIR_ACCELE 
    u32 *clcu_cache;
    u16 clc_len;
    u16 clc_ptr;
#endif
#if !FAT_TINY
    u8 path[FAT_PATH_LEN];
#endif
    u8 cur_nsector;
    u8 FDI_index;
} FAT_DIR;

typedef struct _CLUST_LINK_CACHE
{
    u32 cache[DIR_ACCELE_BUF_L]; //缓存簇链
    u16 dir_ptr[FAT_PATH_DEPTH];  //每个目录对应的簇缓存起始
    u16 len;
    // u16 ptr;
}CLUST_LINK_CACHE;

typedef struct _PATH_DIR
{
    FAT_DIR dir[FAT_PATH_DEPTH];
#if DIR_ACCELE 
    CLUST_LINK_CACHE clc;
#endif
    u8 depth;
}PATH_DIR;

struct _FAT_HDL {
    u32 fs_start_sec;      //文件系统起始物理
    u32 data_start_sector;
    u32 fat_tab_start;
    u32 fat_tab_nsector;
    u32 next_clust;
    u32 total_sector;      //文件系统管理的总扇区数
    u32 fat_tab_total;
    u32 root_dir_clust;
    u32 fs_info_sector;    //文件系统信息扇区
    u32 file_totle;
    FAT_DEV_INFO dev_api;
    FAT_WIN *fs_win;
    PATH_DIR fs_dir;         //文件系统目录，作为当前目录使用
    u16 sector_512size;
    u8 clust_nsector;     //每簇扇区数
    u8 fat_tab_num;
};


typedef struct _FILE_DIR_INFO {
    u32 st_clust;
    u32 file_size;
    u8 attr;
    u8 name[13];
} FILE_DIR_INFO;

/* File access control and file status flags (FIL.flag) */
#define	FAT_OPEN_EXISTING	0x00
// #if FAT_WRITE_EN
#define FAT_WRITE_ADD       0x02            //文件末尾追加数据
#define	FAT_WRITE			0x04            //是否允许写文件
#define	FAT_CREATE_NEW		0x08            //文件不存在时创建
#define	FAT_CREATE_ALWAYS	0x10            //无论文件中否存在，均创建
// #endif
typedef struct _FILE_HDL {
    u32 cur_clust;
    u32 ptr;
    u32 access;
    FILE_DIR_INFO file_info;
   // FAT_DIR dir;
    PATH_DIR p_dir;
    FAT_HDL *fs;
#if !FAT_TINY
    FAT_WIN *data_win;
#endif
    u8 cur_nsector;
} FILE_HDL;


enum {
    FS_NO_ERR = 0x00,
    FS_DISK_ERR,
    FS_BUSY,
    FS_INPUT_PARM_ERR,
    FS_NO_EXIST,
    FS_HDL_ERR,
    FS_DIR_END,
    FS_FILE_END,
    FS_CLUST_ERR,
    FS_NO_DIR,
    FS_NO_FILE,
    FS_DIR_LIMI,    //超出目录深度
    FS_IS_FULL,
    FS_ACCESS_ERR,
    FS_PATH_ERR,
    FS_FILE_EXIST,
    FS_WIN_ERR,
};


u32 fat_syn_fs(FAT_HDL *fs);
u32 fat_open_fs(FAT_HDL *fs, FAT_DEV_INFO *dev_info, u32 boot_start);
void fat_ld_dir(FAT_HDL *fs, FAT_DIR *dir, u32 clust);
u32 fat_dir_next(FAT_HDL *fs, FAT_DIR *dir, FILE_DIR_INFO *file_info);
u32 fat_open_dir_bypath(FAT_HDL *fs, PATH_DIR *p_dir, const char *path);
//u32 fat_open_dir(FAT_HDL *fs, const char *path);
u32 fat_mk_dir(FAT_HDL *fs, PATH_DIR *p_dir, const char *path);
u32 fat_open_file(FAT_HDL *fs, FILE_HDL *f_hdl, char *path, u32 access);
u32 fat_rm(FAT_HDL *fs, FAT_DIR *dir);
s32 fat_file_read(FILE_HDL *f_hdl, u8 *buffer, u32 len);
s32 fat_file_write(FILE_HDL *f_hdl, u8 *buffer, u32 len);
u32 fat_file_del(FILE_HDL *f_hdl);
u32 fat_syn_file(FILE_HDL *f_hdl);
u32 fat_file_seek(FILE_HDL *f_hdl, s32 seek, u8 mode);
u32 fat_file_tell(FILE_HDL *f_hdl);
u32 fat_format(FAT_DEV_INFO *dev_api, u32 st_sec, u32 nsec);

#endif
