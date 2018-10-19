#include "sdk_cfg.h"
#include "lxfat.h"

#define FAT_DEBUG
#ifdef FAT_DEBUG
#define fat_printf  printf
#else
#define fat_printf(...)
#endif

#define FAT_MIN(x,y)    ((x)>(y)?(y):(x))
#define CHECK_CLUST_END(x) (((x)&0x0ffffff8) == 0x0ffffff8)
#define CHECK_RES(a)  do{if(a){fat_printf("%s %d res %d\n", __func__, __LINE__, a); return a;}}while(0)

static u16 ld_word_func(u8 *p)
{
    return (tu16)p[1] << 8 | p[0];
}

static u32 ld_dword_func(u8 *p)
{
    return (u32)p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0];
}
static void st_word_func(u8 *ptr, u16 val)
{
    ptr[0] = val;
    ptr[1] = val >> 8;
}

static void st_dword_func(u8 *ptr, u32 val)
{
    ptr[0] = val;
    ptr[1] = val >> 8;
    ptr[2] = val >> 16;
    ptr[3] = val >> 24;
}
//********************************************************************************************
//                      FAT COMM FUNC
//
//
//********************************************************************************************
u32 fat_strlen(const char *str, char endchar)
{
    u8 cnt;
    for (cnt = 0; (*str != endchar) && (*str != '\0'); str++, cnt++) ;
    return cnt;
}

#if 0
u32 fat_strcmp(const char *str1, const char *str2, char endchar)
{
    u32 str_cnt;

    for (str_cnt = 1; *str1 == *str2; str_cnt++, str1++, str2++) {
        if ((*str1 == '\0') || (*str1 == endchar)) {
            return 0;
        }
    }
    if ((*str1 == '\0') && (*str2 == endchar)) {
        return 0;
    }
    if ((*str2 == '\0') && (*str1 == endchar)) {
        return 0;
    }
    return str_cnt;
}
#else
u32 fat_strcmp(const char *str1, const char *str2, char uch)
{
	u32 scnt = 0;
    while (1) {
		scnt++;
        if (((*str1 == '\0')||(*str1 == uch)) && ((*str2 == '\0')||(*str2 == uch))) {			//同时结束
            return 0;
        }
    	/* if ((*str1 == uch) && (*str2 == uch)) {			//同时结束 */
        /*     return 0; */
        /* } */
        /*  */
        if ((*str1 == '\0') || (*str2 == '\0')||(*str1 == uch) || (*str2 == uch)) {	//有一方提前结束
            return scnt;
        }

		if (*str1 != *str2) {						//比较不相等
			if ((*str2 != '?')&& (*str1 != '?')) {
				if ((*str2 != '*')&& (*str1 != '*')) {
					return scnt;
				}else{
					if(*str1 == '*'){
                        if((*(str1+1) == '\0')||(*(str1+1) == uch)){
                            return 0;
                        }
                        if(*(str1+1) != *str2){
                           str2++; 
                           continue;
                        }else{
                           str1++; 
                        }
                    }

                    if(*str2 == '*'){
                        if((*(str2+1) == '\0')||(*(str2+1) == uch)){
                            return 0;
                        }
                        if(*(str2+1) != *str1){
                           str1++; 
                           continue;
                        }else{
                           str2++; 
                        }
                    }
                }
			}
		}
        str1++;
        str2++;
    }
}
#endif

void fat_uppercase(char *str)
{
    while(*str){
       if((*str>='a')&&(*str<='z')){
           *str -= 0x20; 
       }
       str++;
    }
}

//检测是否有小写字符
static u16 check_lowercase(char *str)
{
    u16 cnt = 0;
    while(*str != '\0'){
       cnt++;
       if((*str>='a')&&(*str<='z')){
           return cnt;
       }
       str++;
    }
    return 0;
}


struct _FAT_TIME {
    u16 sec :5;
    u16 min :6;
    u16 hour:5;
};

struct _FAT_DATE {
    u16 day :5;
    u16 mon :4;
    u16 year:7;
};

static u16 get_cur_time()
{
    struct _FAT_TIME time;
   u8 hour, min, sec; 
   hour = 10;
   min  = 30;
   sec  = 30;
   time.hour = hour;
   time.min  = min;
   time.sec  = sec/2;
   /* fat_printf("fat time size %d\n", sizeof(struct _FAT_TIME)); */
   return *(u16 *)&time;
}

static u16 get_cur_date()
{
   struct _FAT_DATE date;
   u8 mon, day; 
   u16 year;

   year = 2018;
   mon  = 10;
   day  = 7;

   date.year = year - 1980;
   date.mon  = mon;
   date.day  = day;
   /* fat_printf("fat date size %d\n", sizeof(struct _FAT_DATE)); */
   return *(u16 *)&date;

    return 0;    
}

//********************************************************************************************
//                      FAT DISK  FUNC
//
//
//********************************************************************************************
static u32 fat_disk_read(FAT_HDL *fs, u8 *buffer, u32 sector, u32 len)
{
    FAT_DEV_INFO *dev_api = &fs->dev_api;

    if (dev_api == NULL) {
        return FS_DISK_ERR;
    }

    if (dev_api->dev_block_size > FAT_SECTOR_SIZE) {
        return FS_DISK_ERR;
    }

    if (dev_api->dev_block_size != FAT_SECTOR_SIZE) {
        len = FAT_SECTOR_SIZE / dev_api->dev_block_size * len;
    }


    if (0 != dev_api->dev_read(dev_api->dev, buffer, sector, len)) {
        return FS_DISK_ERR;
    }

    return FS_NO_ERR;
}


static u32 fat_disk_write(FAT_HDL *fs, u8 *buffer, u32 sector, u32 len)
{
#if FAT_WRITE_EN
    FAT_DEV_INFO *dev_api = &fs->dev_api;

    printf("1");
    if (dev_api == NULL) {
        return FS_DISK_ERR;
    }

    printf("2");
    if (dev_api->dev_block_size > FAT_SECTOR_SIZE) {
        return FS_DISK_ERR;
    }
    printf("3");
    if (sector < fs->fs_start_sec) { //不在文件系统管理内，不可写
        return FS_ACCESS_ERR;
    }

    printf("4");
    if (dev_api->dev_block_size != FAT_SECTOR_SIZE) {
        len = FAT_SECTOR_SIZE / dev_api->dev_block_size * len;
    }

    if (0 != dev_api->dev_write(dev_api->dev, buffer, sector, len)) {
        return FS_DISK_ERR;
    }

    printf("5");
#endif
    return FS_NO_ERR;
}

#define LOCK_WIN(w)     do{(w)->flag |= WIN_BUSY;}while(0)
#define UNLOCK_WIN(w)   do{(w)->flag &= ~WIN_BUSY;}while(0)
#define CHECK_WIN(w,r)    do{if((w)->flag&WIN_BUSY)return r;}while(0)
FAT_WIN scan_win[FAT_PATH_DEPTH];
//获取一块可用的WIN
FAT_WIN *alloc_win()
{
   u8 win_cnt; 
   FAT_WIN *win;
   for(win = scan_win, win_cnt = 0; win_cnt<sizeof(scan_win)/sizeof(FAT_WIN);win++, win_cnt++){
       if(!(win->flag&WIN_USE)){ 
           win->flag |= WIN_USE;
           return win; 
       }
   }
   return NULL;
}

void free_win(FAT_WIN *win)
{
    if(win){
        if(win->flag|WIN_USE){ 
           win->flag &= ~WIN_USE;
       }
    } 
}

static u32 fat_syn_win(FAT_HDL *fs, FAT_WIN *win)
{
#if FAT_WRITE_EN
    u32 sector = win->sector;
    if ((fs != win->fs)||(win->flag & DIRTY)) {
        fat_printf("syn win %x\n", sector);
        if(win->fs){
            if (FS_NO_ERR != fat_disk_write(win->fs, win->buffer, sector, 1)) {
                return FS_DISK_ERR;
            }
        }
        win->flag &= ~DIRTY;
        win->fs = fs;
    }
#endif
    return FS_NO_ERR;
}

static u32 fat_move_win(FAT_HDL *fs, FAT_WIN *win, u32 sector)
{
    if ((fs != win->fs)||(win->sector != sector)) {
        u32 ret;

        ret = fat_syn_win(fs, win);
        CHECK_RES(ret);

        fat_printf("mov win %x %x\n", win->sector, sector);
        if (FS_NO_ERR != fat_disk_read(fs, win->buffer, sector, 1)) {
            return FS_DISK_ERR;
        }
        win->sector = sector;
        win->fs = fs;
    }
    return FS_NO_ERR;
}

u32 fat_syn_fs(FAT_HDL *fs)
{
    u32 ret;
    FAT_WIN *win = fs->fs_win;
    fat_printf("fs win flag %x\n", win->flag);
    CHECK_WIN(win, FS_BUSY);
    LOCK_WIN(win);
    ret = fat_syn_win(fs, win);
    UNLOCK_WIN(win);
    return ret;
}
/*
   s32 fat_win_write(FAT_WIN *win, u32 offset, u8 *buffer, u32 size)
   {
   if(offset + size > FAT_SECTOR_SIZE){
       return FS_INPUT_PARM_ERR;
    }

    memcpy(&win->buffer[offset], buffer, size);

}
*/

//********************************************************************************************
//                      FAT FS OPEN FUNC
//
//
//********************************************************************************************
#define BPB_OFFSET          11

enum {
    NO_FS = 0x00,
    FAT12,
    FAT16,
    FAT32,
    EXFAT,
};

u32 check_fat(u8 *dbr)
{
    u8 *bpb;
    u8 string[9];
    if ((dbr[510] != 0x55) || (dbr[511] != 0xaa)) {
        return NO_FS;
    }
    memset(string, 0x00, 9);
    memcpy(string, &dbr[3], 8);
    fat_printf("OS VERS:%s\n", string);

    bpb = &dbr[BPB_OFFSET];

    memset(string, 0x00, 9);
    memcpy(string, &bpb[71], 8);
    fat_printf("FAT ID:%s\n", string);

    memset(string, 0x00, 9);
    memcpy(string, &bpb[60], 8);
    fat_printf("LABEL:%s\n", string);

    fat_printf("let");
    putchar(bpb[53]);

    return FAT32;
}

u32 fat_open_fs(FAT_HDL *fs, FAT_DEV_INFO *dev_info, u32 boot_start)
{
    u32 ret;
    u8 *dbr;
    u8 *bpb;

    if (fs == NULL) {
        return FS_HDL_ERR;
    }
    if (dev_info == NULL) {
        return FS_INPUT_PARM_ERR;
    }

    memset(fs, 0x00, sizeof(FAT_HDL));
    memcpy(&fs->dev_api, dev_info, sizeof(FAT_DEV_INFO));
    fs->fs_start_sec = boot_start;
    fs->fs_win = alloc_win();
    if(fs->fs_win == NULL){
       return FS_WIN_ERR; 
    }
    ret = fat_move_win(fs, fs->fs_win, fs->fs_start_sec);
    if (ret != FS_NO_ERR) {
        goto __open_fs_err;
    }
    dbr = &fs->fs_win->buffer[0];
    if (check_fat(dbr) != FAT32) {
        ret = FS_NO_EXIST;
        goto __open_fs_err;
    }

    bpb = &dbr[BPB_OFFSET];
    fs->sector_512size      = ld_word_func(bpb) / 512;
    fs->clust_nsector       = bpb[2];
    fs->fat_tab_start       = fs->fs_start_sec + ld_word_func(&bpb[3]);
    fs->fat_tab_num         = bpb[5];
    fs->total_sector        = ld_dword_func(&bpb[21]);
    fs->fat_tab_nsector     = ld_dword_func(&bpb[25]);
    fs->root_dir_clust      = ld_dword_func(&bpb[33]);
    fs->fs_info_sector      = ld_word_func(&bpb[37]);
    fs->data_start_sector   = fs->fat_tab_start + fs->fat_tab_nsector * fs->fat_tab_num;
    fs->next_clust          = 2;
    fs->fs_dir.depth        = 1;


    fat_ld_dir(fs, &fs->fs_dir.dir[0], fs->root_dir_clust);
    fat_printf("extern flag 0x%x\n", ld_word_func(&bpb[29]));
    fat_printf("sys ver 0x%x\n", ld_word_func(&bpb[31]));
    fat_printf("hidden sector num %d\n", ld_dword_func(&bpb[17]));
    fat_printf("fs->sclust_nsector %d\n", fs->clust_nsector);
    fat_printf("fs->fat_tab_sector %d\n", fs->fat_tab_start);
    fat_printf("fs->fat_tab_num %d\n", fs->fat_tab_num);
    fat_printf("fs size %dM\n", fs->total_sector / 2048);
    fat_printf("fs->root_dir_clust %d\n", fs->root_dir_clust);
    fat_printf("fs->data_start_sector %d\n", fs->data_start_sector);

    return FS_NO_ERR;
__open_fs_err:
    free_win(fs->fs_win);
    return ret;
}


void close_fs(FAT_HDL *fs)
{ 
    fat_syn_fs(fs);
    free_win(fs->fs_win);
    fs->fs_win = NULL;
}

//********************************************************************************************
//                      FAT SECTER AND SCLUSET FUNC
//
//
//********************************************************************************************
/*
u32 fat_getsector_byclust(FAT_HDL *fs, u32 sclust)
{
    u32 sector;
    if (sclust >= 2) {
        sector = fs->data_start_sector + (sclust - 2) * fs->sclust_nsector;
        return sector;
    }
    return 0;
}
*/
//获取簇对应的物理扇区号
u32 get_sector_byclust(FAT_HDL *fs, u32 sclust)
{
    u32 sector;
    if (sclust < 2) {
        return 0;
    }
    sector = fs->data_start_sector + fs->clust_nsector * (sclust - 2);
    return sector;
}
//寻找当前簇对应的下个簇号
u32 fat_next_clust(FAT_HDL *fs, u32 cur_clust)
{
    u32 fat_tab_sector;
    u32 offset_insector;
    u32 sclust = 0;
    FAT_WIN *win = fs->fs_win;

    fat_tab_sector = fs->fat_tab_start + (cur_clust >> 7);
    offset_insector = (cur_clust << 2) & 0x1ff;
    CHECK_WIN(win, 0);
    LOCK_WIN(win);
    if(FS_NO_ERR == fat_move_win(fs, win, fat_tab_sector)){
        memcpy(&sclust, &fs->fs_win->buffer[offset_insector], 4);
    }
    UNLOCK_WIN(win);
    return sclust;
}
//从FAT表获取一个空簇
u32 fat_find_empty_clust(FAT_HDL *fs)
{
    u32 fat_tab_sector;
    u32 offset_insector;
    u32 cur_clust, next_clust;
    FAT_WIN *win;
    u32 ret;

    win = fs->fs_win;
    cur_clust = fs->next_clust;

    //寻找一个空簇
    do {
        cur_clust++;
        if (cur_clust > (fs->fat_tab_nsector << 7)) { //是否超出FAT表
            cur_clust = 2;
        }
        if (cur_clust == fs->next_clust) { //已经查询过所有簇
            return 0;
        }

        fat_tab_sector = fs->fat_tab_start + (cur_clust >> 7);
        offset_insector = (cur_clust << 2) & 0x1ff;
        CHECK_WIN(win, 0);
        LOCK_WIN(win);
        ret = fat_move_win(fs, win, fat_tab_sector);
        if (FS_NO_ERR == ret) {
            memcpy(&next_clust, &fs->fs_win->buffer[offset_insector], 4);
        }
        UNLOCK_WIN(win);

        if (ret != FS_NO_ERR) {
            return 0;
        }
    } while (next_clust);

    fs->next_clust = cur_clust;
    return cur_clust;
}

//更新当前簇对应的FAT表信息
u32 fat_update_clust_node(FAT_HDL *fs, u32 cur_clust, u32 node)
{
#if FAT_WRITE_EN
    u32 fat_tab_sector;
    u32 offset_insector;
    FAT_WIN *win;
    u32 ret;
    u8 ftb_cnt;

    win = fs->fs_win;

    fat_tab_sector = fs->fat_tab_start + (cur_clust >> 7);
    offset_insector = (cur_clust << 2) & 0x1ff;
    CHECK_WIN(win, 0);
    LOCK_WIN(win);
    for(ftb_cnt = 0; ftb_cnt<fs->fat_tab_num;ftb_cnt++){
        fat_tab_sector = fs->fat_tab_start + fs->fat_tab_nsector*ftb_cnt + (cur_clust >> 7);
        ret = fat_move_win(fs, win, fat_tab_sector);
        if (FS_NO_ERR == ret) {
            memcpy(&fs->fs_win->buffer[offset_insector], &node, 4);
            win->flag |= DIRTY;
        }
    }
    UNLOCK_WIN(win);

    return ret;
#else
    return FS_ACCESS_ERR;
#endif
}

//清除指定簇的数据
u32 clear_clust_data(FAT_HDL *fs, u32 cur_clust)
{
    u32 sec_cnt;
    FAT_WIN *win;
    u32 ret;

    win = fs->fs_win;
    sec_cnt = fs->clust_nsector;
    CHECK_WIN(win, FS_BUSY);
    LOCK_WIN(win);
    ret = fat_syn_win(fs, win);
    if (ret) {
        UNLOCK_WIN(win);
        return ret;
    }

    win->sector = get_sector_byclust(fs, cur_clust);
    memset(win->buffer, 0x00, FAT_SECTOR_SIZE);
    do {
        if (FS_NO_ERR != fat_disk_write(fs, win->buffer, win->sector, 1)) {
            UNLOCK_WIN(win);
            ret = FS_DISK_ERR;
            return ret;
        }
        win->sector++;
        sec_cnt--;
    } while (sec_cnt);
    UNLOCK_WIN(win);
    return FS_NO_ERR;
}

//按当前簇创建指定个数的簇链
u32 fat_creat_clust_link(FAT_HDL *fs, u32 cur_clust, u32 sclust_num)
{
    u32 next_clust;
    u32 ret = FS_NO_ERR;

    fat_printf("cur sclust %x  nclust %d\n", cur_clust, sclust_num);
    //寻找簇链末尾
    while (sclust_num) {
        //获取当前簇的下个簇
        next_clust = fat_next_clust(fs, cur_clust);
        if (next_clust == 0) {
            return FS_CLUST_ERR;
        }
        if ((next_clust > 1) && (next_clust < 0x0fffffef)) { //分配簇的范围，正常
            cur_clust = next_clust;
        } else { //当前簇是最后一个簇
            break;
        }
        sclust_num--;
    }
    //为簇链添加新节点
    while (sclust_num) {
        next_clust = fat_find_empty_clust(fs);
        if (0 == next_clust) {
            ret = FS_IS_FULL;
            break;
        }
        ret = fat_update_clust_node(fs, cur_clust, next_clust);
        if (ret) {
            break;
        }
        cur_clust = next_clust;
        ret = clear_clust_data(fs, cur_clust);
        if (ret) {
            break;
        }
        sclust_num--;
    }

    fat_update_clust_node(fs, cur_clust, 0x0fffffff); //添加结束标志
    return ret;
}
//删除簇链
u32 fat_rm_clust_link(FAT_HDL *fs, u32 st_clust)
{
    u32 next_clust, cur_clust;
    u32 ret = FS_NO_ERR;

    cur_clust = st_clust;
    fat_printf("rm cluset:");
    do {
        fat_printf("->%x", cur_clust);
        next_clust = fat_next_clust(fs, cur_clust);
        ret = fat_update_clust_node(fs, cur_clust, 0x0); //将当前簇设为空闲
        if (ret) {
            break;
        }
    } while ((next_clust > 2) && (next_clust < 0x0fffffef));
    fat_printf("\n");
    return ret;
}

#if DIR_ACCELE
//加载簇链到缓存中
u32 ld_clust_link(FAT_HDL *fs, CLUST_LINK_CACHE *clc, u32 sclust, u16 cluset_num)
{
    u32 next_clust, cur_clust;
    u32 ret = FS_NO_ERR;

    fat_printf("cur sclust %x  \n",  sclust);
    /* clc->ptr = 0; */
    //寻找簇链末尾
    clc->len = clc->ptr;
    clc->cache[clc->len] = 0x10000000|cur_clust;  //起始簇号标志
    clc->len++; 
    for(cur_clust = sclust; clc->len< DIR_ACCELE_BUF_L; ) {
        //获取当前簇的下个簇
        next_clust = fat_next_clust(fs, cur_clust);
        if (next_clust == 0) {
            return FS_CLUST_ERR;
        }
        if ((next_clust > 1) && (next_clust < 0x0fffffef)) { //分配簇的范围，正常
            cur_clust = next_clust;
        } else { //当前簇是最后一个簇
            break;
        }
        clc->cache[clc->len] = cur_clust;
        clc->len++; 
    }
    return FS_NO_ERR;
}

u32 ld_st_clc(PATH_DIR *p_dir)
{
    u8 depth_cnt;
    p_dir->clc.len = 0;
    for(depth_cnt = 0; depth_cnt<FAT_PATH_DEPTH; depth_cnt++){
    p_dir->clc.dir_ptr[p_dir->depth] = 0; 
    }
}

#endif
//********************************************************************************************
//                      FAT DIR FUNC
//
//
//********************************************************************************************
//加载目录
void fat_ld_dir(FAT_HDL *fs, FAT_DIR *dir, u32 sclust)
{
    if (sclust < 2) {
        sclust = fs->root_dir_clust;
    }
    dir->st_clust = sclust;
    dir->cur_clust = sclust;
    dir->cur_nsector = 0;
    dir->FDI_index = 255;
    fat_printf("ld dir %x\n", sclust);
#if (FAT_TINY == 0)
    memset(dir->path, 0x00, FAT_PATH_LEN);
#endif
}

static u32 fat_get_FDI(FAT_HDL *fs, FAT_DIR *dir, u8 *FDI)
{
    u32 sector;
    FAT_WIN *win;

    /* fat_printf("dir cluset %d\n", dir->cur_clust); */
    sector = get_sector_byclust(fs, dir->cur_clust);
    if (!sector) {
        return FS_CLUST_ERR;
    }
    sector += dir->cur_nsector;

    win = fs->fs_win;
    CHECK_WIN(win, FS_BUSY);
    LOCK_WIN(win);
    //fat_printf("dir sector %d\n", sector);
    if (FS_NO_ERR != fat_move_win(fs, fs->fs_win, sector)) {
        UNLOCK_WIN(win);
        return FS_DISK_ERR;
    }

    //  fat_printf_buf(win->buffer, 512);
    // FDI = &win->buffer[dir->FDI_index<<5];
    memcpy(FDI, &win->buffer[dir->FDI_index << 5], 32);
    UNLOCK_WIN(win);
    return FS_NO_ERR;
}

//寻找当前目录项的下个FDI
static u32 fat_next_FDI(FAT_HDL *fs, FAT_DIR *dir, u8 *FDI)
{
    /* FAT_WIN *win; */
    dir->FDI_index++;
    if (dir->FDI_index == FAT_SECTOR_SIZE / 32) {
        dir->cur_nsector++;
        dir->FDI_index = 0;
    }
    if (dir->cur_nsector == fs->clust_nsector) {
        u32 sclust;
        sclust = fat_next_clust(fs, dir->cur_clust);
        if ((sclust > 1) && (sclust < 0x0fffffef)) { //分配簇的范围，正常
            dir->cur_clust = sclust;
            dir->cur_nsector = 0;
            dir->FDI_index = 0;
        } else {
            return FS_DIR_END;
        }
    }

    return fat_get_FDI(fs, dir, FDI);
}
//往当前目录项编号写入FDI
static u32 fat_write_FDI(FAT_HDL *fs, FAT_DIR *dir, u8 *FDI)
{
    FAT_WIN *win;
    u32 sector;
    
    fat_printf(" %s \n", __func__);
    win = fs->fs_win;
    sector = get_sector_byclust(fs, dir->cur_clust);
    if (!sector) {
        return FS_CLUST_ERR;
    }
    sector += dir->cur_nsector;

    CHECK_WIN(win, FS_BUSY);
    LOCK_WIN(win);
    //fat_printf("dir sector %d\n", sector);
    if (FS_NO_ERR != fat_move_win(fs, fs->fs_win, sector)) {
        UNLOCK_WIN(win);
        return FS_DISK_ERR;
    }

    //  fat_printf_buf(win->buffer, 512);
    // FDI = &win->buffer[dir->FDI_index<<5];
    memcpy(&win->buffer[dir->FDI_index << 5], FDI, 32);
    win->flag |= DIRTY;
    fat_printf("win flag %x\n", win->flag);
    UNLOCK_WIN(win);
    return FS_NO_ERR;
}
//将目录信息转换到FDI
static void dir_info_to_FDI(FILE_DIR_INFO *file_info, u8 *FDI)
{
    u8 str_len;
    str_len = fat_strlen((const char *)file_info->name, '.');
    if (str_len > 8) {
        str_len  = 8;
    }
    //复制文件名到FDI
    memcpy(&FDI[0], file_info->name, str_len);
    memset(&FDI[str_len], 0x20, 8 - str_len);
    if (file_info->attr & AM_DIR) {
        memset(&FDI[8], 0x20, 3);
    } else {
        memcpy(&FDI[8], file_info->name + str_len + 1, 3);
    }
    FDI[11] = file_info->attr;
    FDI[20] = file_info->st_clust >> 16 & 0xff;
    FDI[21] = file_info->st_clust >> 24 & 0xff;
    FDI[26] = file_info->st_clust & 0xff;
    FDI[27] = file_info->st_clust >> 8 & 0xff;
    st_dword_func(&FDI[28], file_info->file_size);
    fat_printf("up dir attr 0x%x st_clust 0x%x file size %d\n", file_info->attr, file_info->st_clust, file_info->file_size);
}
//寻找下一个文件目录项
u32 fat_dir_next(FAT_HDL *fs, FAT_DIR *dir, FILE_DIR_INFO *file_info)
{
    /* u32 sector; */
    u8 FDI[32];
    u8 *name;
    u32 ret;

//_func_enty:
    //寻找一个有效的热目录项
    for(;;){
        ret = fat_next_FDI(fs, dir, FDI);
        if (ret != FS_NO_ERR) {
            return ret;
        }

        switch (FDI[0]) {
        case 0x0:
            return FS_DIR_END;
        case 0xE5:  //被删除的目录项
            continue;
        default :
            break;
        }

        if (FDI[11] != AM_LFN) { //不是长文件名项
            break;
        }
    }


    name = file_info->name;
    memset(name + 8, 0x00, sizeof(file_info->name) - 8);
    memcpy(name, FDI, 8);
    while ((*name != 0) && (*name != 0x20)) {
        name++;
    }
    if (!(FDI[11]&AM_DIR)) { //不是子目录
        *name++ = '.';
        memcpy(name, &FDI[8], 3); //扩展名
        name += 3;
    }
    *name = '\0';

    file_info->attr = FDI[11];
    file_info->st_clust = ld_word_func(&FDI[20]);
    file_info->st_clust = (file_info->st_clust << 16) + ld_word_func(&FDI[26]);
    file_info->file_size = ld_dword_func(&FDI[28]);

    fat_printf("file name %s\n", file_info->name);
    fat_printf("file size %d\n", file_info->file_size);
    return FS_NO_ERR;
}
//寻找指定目录项，根据file info 的name
u32 fat_find_dir(FAT_HDL *fs, FAT_DIR *dir, FILE_DIR_INFO *file_info)
{
    FILE_DIR_INFO dir_info;
    while (FS_NO_ERR == fat_dir_next(fs, dir, &dir_info)) {
        if (!fat_strcmp((const char *)file_info->name, (char *)dir_info.name, 0)) { //名字匹配
            memcpy(file_info, &dir_info, sizeof(FILE_DIR_INFO));
            return FS_NO_ERR;
        }
    }
    return FS_NO_FILE;
}

//创建一个目录项
u32 creat_dir_entry(FAT_HDL *fs, FAT_DIR *dir, FILE_DIR_INFO *file_info)
{
    u32 ret;
    u8 FDI[32];

    //寻找当前目录下一个可用的空目录项
    fat_ld_dir(fs, dir, dir->st_clust);//重新加载目录起始位置
    do {
        ret = fat_next_FDI(fs, dir, FDI);
        /* printf("ret %d\n", ret); */
        switch (ret) {
        case FS_NO_ERR:
            /* put_buf(FDI,32); */
            switch (FDI[0]) {
            case 0://无效FDI可使用
            case 0xe5:
                break;
            default:
                continue;
            }
            break;
        case FS_DIR_END:
            //为簇链增加节点
            ret = fat_creat_clust_link(fs, dir->cur_clust, 1);
            CHECK_RES(ret);
            continue;
        default :
            return ret;
        }

        memset(FDI, 0x00, 32);
        dir_info_to_FDI(file_info, FDI);
        st_word_func(&FDI[14], get_cur_time());
        st_word_func(&FDI[16], get_cur_date());
        st_word_func(&FDI[24], get_cur_date());
        st_word_func(&FDI[22], get_cur_time());

        ret = fat_write_FDI(fs, dir, FDI);
        break; 
    } while (1);
    extern void stack_check();
    stack_check();

    return ret;
}

//更新目录信息到磁盘
u32 fat_updata_dir(FAT_HDL *fs, FAT_DIR *dir, FILE_DIR_INFO *file_info)
{
    u8 FDI[32];
    u32 res;

     //先从磁盘读出原来的FDI
    res = fat_get_FDI(fs, dir, FDI);
    CHECK_RES(res);
    //更新FDI
    dir_info_to_FDI(file_info, FDI);
    st_word_func(&FDI[24], get_cur_date());
    st_word_func(&FDI[22], get_cur_time());
    //写入FDI
    return fat_write_FDI(fs, dir, FDI);
}


u32 fat_open_dir_bypath(FAT_HDL *fs, PATH_DIR *p_dir, const char *path)
{
    FILE_DIR_INFO file_info;
    FAT_DIR *dir;
    if (*path == '/') { //根目录开始
        fat_ld_dir(fs, &p_dir->dir[0], fs->root_dir_clust);
        p_dir->depth = 0;
#if DIR_ACCELE 
        p_dir->clc.len = 0;
        /* p_dir->clc.ptr = p_dir->clc.len; */
        p_dir->clc.dir_ptr[p_dir->depth] = p_dir->clc.len; 
#endif
        path++;
    }
    dir = &p_dir->dir[p_dir->depth];
#if DIR_ACCELE 
    dir->clc_cache = p_dir->clc.cahce[p_dir->clc.len];
    dir->clc_len = DIR_ACCELE_BUF_L - p_dir->clc_len;
    dir->clc_ptr = 0;
#endif
    if (p_dir->depth == FAT_PATH_DEPTH) {
        return FS_DIR_LIMI;
    } else {
        const char *p = path;
        bool is_file = false;
        while ((*p != '\0') && (*p != '/')) {
            if (*p++ == '.') { //可能是文件
                is_file = true;
            }
        }
        if (*p == '\0') {
            /* if (is_file == true) { //路径最后是文件 */
                return FS_NO_ERR;
            /* }else{ */
            /*     return FS_NO_FILE; */
            /* } */
        }
    }

    while (FS_NO_ERR == fat_dir_next(fs, dir, &file_info)) {
        if (file_info.attr & AM_DIR) { //改文件时子目录
            if (!fat_strcmp(path, (char *)file_info.name, '/')) { //名字匹配
                fat_printf("next dir %s\n", file_info.name);
                while (1) {
                    switch (*path) {
                        case '\0':   //路径末尾
                            if(p_dir->depth == FAT_PATH_DEPTH - 1){
                                return FS_DIR_LIMI;
                            }
                            fat_ld_dir(fs, &p_dir->dir[++p_dir->depth], file_info.st_clust);
#if DIR_ACCELE 
                            dir->clc_cache = p_dir->clc.cahce[ptr];
                            dir->clc_len = 0;
#endif
                            return FS_NO_ERR;
                        case '/':  //还有后续目录
                            if(p_dir->depth == FAT_PATH_DEPTH - 1){
                                return FS_DIR_LIMI;
                            }
                            fat_ld_dir(fs, &p_dir->dir[++p_dir->depth], file_info.st_clust);
                            if (*++path == '\0') { //后续目录为空，也是路径末尾
                                return FS_NO_ERR;
                            }
                            return fat_open_dir_bypath(fs, p_dir, path); //递推下层目录
                        default:
                            path++;
                            break;
                    }
                }
            }
        }
    }
    return FS_NO_DIR;
}
/*  */
/* u32 fat_open_dir(FAT_HDL *fs, const char *path) */
/* { */
/*     return fat_open_dir_bypath(fs, &fs->fs_dir, path); */
/* } */


static u32 fat_creat_file(FAT_HDL *fs, PATH_DIR *p_dir, FILE_DIR_INFO *file_info, char *path)
{
    u32 ret;
    char *file_name;
    u16 name_len;
    FAT_DIR *dir;
    /* FAT_DIR dir_tmp; */

    /* if (dir == NULL) { */
    /*     dir = &dir_tmp; */
    /*     memcpy(dir, &fs->fs_dir, sizeof(FAT_DIR)); //使用文件系统当前目录 */
    /* } */
    /* dir = &p_dir->dir[p_dir->depth - 1]; */
    /* fat_ld_dir(fs, dir, dir->st_clust); */

    ret = fat_open_dir_bypath(fs, p_dir, path);
    CHECK_RES(ret);
    dir = &p_dir->dir[p_dir->depth];

    file_info->file_size = 0;
    // file_info->st_clust = 0;
    //file_info->attr  = 0;
    //寻找文件名位置
    for (file_name = path; *path; path++) {
        if (*path == '/') {
            file_name = path + 1;
        }
    }
    name_len = fat_strlen(file_name, '.');
    if (!name_len) {
        fat_printf("%s %d\n", __func__, __LINE__);
        return FS_PATH_ERR;
    }
    fat_printf("crear file name %s\n", file_name);


    fat_ld_dir(fs, dir, dir->st_clust);//重新加载目录起始位置
    if (name_len > 8) { //文件名太长，需要使用编号
#if FAT_LFN_EN
        FILE_DIR_INFO find_file;
        char cmp_name[13];
        char index_num;
        u16 name_index = 0;
        //是否有文件与需要创建的文件重名
        memset(cmp_name, 0x00, 13);
        memcmp(cmp_name, file_name, 8);
        memcmp(cmp_name, file_name + name_len, 4);
        cmp_name[6] = '~';
        cmp_name[7] = '*';

        while (FS_NO_ERR == fat_dir_next(fs, &dir, &find_file)) {
            if (!fat_strcmp(cmp_name, (char *)dir_info.name, 0)) { //名字匹配
                index_num = find_file.name[7];
                if ((index_num >= '0') && (index_num <= '9')) {
                    index_num -= '0';
                    name_index |= BIT(index_num);
                }
            }
        }
        for (index_num = 0; index_num < 10; index_num++) {
            if (!(name_index | BIT(index_num))) {
                break;
            }
        }
        if (index_num == 10) {
            return FS_FILE_EXIST;
        }
        memcmp(file_info->name, cmp_name, 13);
        file_info->name[7] = index_num + '0';
#else
        return FS_PATH_ERR;
#endif
    } else {
        FILE_DIR_INFO find_file;
        //是否有文件与需要创建的文件重名
        memset(&find_file, 0x00, sizeof(FILE_DIR_INFO));
        memcmp(find_file.name, file_name, name_len + 4);
        ret = fat_find_dir(fs, dir, &find_file);
        if (ret == FS_NO_ERR) {
            fat_printf("name repeat\n");
            return FS_FILE_EXIST;
        }
        memset(file_info->name, 0x00, sizeof(file_info->name));
        memcpy(file_info->name, file_name, name_len + 4);
    }
    //不支持小写创建
    if(check_lowercase((char *)file_info->name)){
       return FS_PATH_ERR; 
    }
    /* fat_uppercase((char *)file_info->name);  */

    return creat_dir_entry(fs, dir, file_info);
}

//创建一个目录
u32 fat_mkdir(FAT_HDL *fs, PATH_DIR *p_dir, const char *path)
{
    u32 ret;
    FAT_DIR *dir;
    FILE_DIR_INFO file_info;

    if(p_dir == NULL){
        p_dir = &fs->fs_dir;
    }
    /* memcpy(dir, &fs->fs_dir, sizeof(FAT_DIR)); //使用文件系统当前目录 */
    if (*path == '/') { //根目录开始
        fat_ld_dir(fs, &p_dir->dir[0], fs->root_dir_clust);
        path++;
        p_dir->depth = 0;
    }
    dir = &p_dir->dir[p_dir->depth];
    fat_ld_dir(fs, dir, dir->st_clust);
    ret = fat_open_dir_bypath(fs, p_dir, path);
    CHECK_RES(ret);

    dir = &p_dir->dir[p_dir->depth];

    

    //寻找路径中的目录是否存在
    while (1) {
        ret = fat_dir_next(fs, dir, &file_info);
        if (ret) {
            break;
        }

        fat_printf("next dir %s:%s\n",path, file_info.name);
        if (file_info.attr & AM_DIR) { //改文件时子目录
            if (!fat_strcmp(path, (char *)file_info.name, '/')) { //名字匹配
                fat_printf("cmp sucess\n");
                return FS_FILE_EXIST;
            }
        }
    }
    if (ret == FS_DIR_END) {
        char filename[FAT_PATH_LEN];
        u8 namecnt;
        //u32 cur_sector;
        //拷贝文件名
        for (namecnt = 0; (*path != '/') && (*path != '\0'); path++, namecnt++) {
            if (namecnt == FAT_PATH_LEN - 1) {
                return FS_PATH_ERR;
            }
            filename[namecnt] = *path;
        }
        path++;
        filename[namecnt + 1] = '\0';
        //创建新目录
        memset(&file_info, 0x00, sizeof(FILE_DIR_INFO));
        file_info.st_clust = fat_find_empty_clust(fs);
        file_info.attr = AM_DIR;
        if (file_info.st_clust == 0) {
            return FS_IS_FULL;
        }
        ret = fat_creat_file(fs, p_dir, &file_info, filename);
        if (ret) {
            return ret;
        }
        if (fat_update_clust_node(fs, file_info.st_clust, 0x0fffffff)) { //添加结束标志
            return FS_CLUST_ERR;
        }

        //创建当前目录和上级目录
        FAT_WIN *win = fs->fs_win;
        u8 *FDI;
        CHECK_WIN(win, FS_BUSY);
        LOCK_WIN(win);
        ret = fat_syn_win(fs, win);
        if (ret) {
            UNLOCK_WIN(win);
            goto path_end;
            //   return ret;
        }
        win->sector = get_sector_byclust(fs, file_info.st_clust);
        memset(win->buffer, 0x00, FAT_SECTOR_SIZE);
        FDI = win->buffer;
        memset(&FDI[0], 0x20, 11);
        FDI[0] = '.';
        FDI[11] = AM_DIR;
        FDI[20] = file_info.st_clust >> 16 & 0xff;
        FDI[21] = file_info.st_clust >> 24 & 0xff;
        FDI[26] = file_info.st_clust & 0xff;
        FDI[27] = file_info.st_clust >> 8 & 0xff;
        st_word_func(&FDI[14], get_cur_time());
        st_word_func(&FDI[16], get_cur_date());
        st_word_func(&FDI[24], get_cur_date());
        st_word_func(&FDI[22], get_cur_time());


        FDI = &win->buffer[32];
        memset(&FDI[0], 0x20, 11);
        FDI[0] = '.';
        FDI[1] = '.';
        FDI[11] = AM_DIR;
        FDI[21] = dir->st_clust >> 24 & 0xff;
        FDI[26] = dir->st_clust & 0xff;
        FDI[27] = dir->st_clust >> 8 & 0xff;
        st_word_func(&FDI[14], get_cur_time());
        st_word_func(&FDI[16], get_cur_date());
        st_word_func(&FDI[24], get_cur_date());
        st_word_func(&FDI[22], get_cur_time());


        //清除当前簇的数据
/* u32 clear_clust_data(FAT_HDL *fs, u32 cur_clust) */
        u8 sec_cnt = 0;
        if (FS_NO_ERR != fat_disk_write(fs, win->buffer, win->sector, 1)) {
            UNLOCK_WIN(win);
            ret = FS_DISK_ERR;
            goto path_end;
        }
        win->sector++;
        sec_cnt++;
        memset(win->buffer, 0x00, FAT_SECTOR_SIZE);
        do {
            if (FS_NO_ERR != fat_disk_write(fs, win->buffer, win->sector, 1)) {
                UNLOCK_WIN(win);
                ret = FS_DISK_ERR;
                goto path_end;
            }
            win->sector++;
            sec_cnt++;
        } while (sec_cnt == fs->clust_nsector);
        UNLOCK_WIN(win);
        //加载当前目录
        fat_ld_dir(fs, dir, file_info.st_clust);
    } else {
        return ret;
    }

path_end:
    //创建过程失败，需要删除簇链
    if (FS_NO_ERR != ret) {
        fat_rm_clust_link(fs, file_info.st_clust);
    }
    return ret;
}

//打开文件
u32 fat_open_file(FAT_HDL *fs, FILE_HDL *f_hdl, char *path, u32 access)
{
    FAT_DIR *dir;
    FILE_DIR_INFO file_info;
    char *filename;
    bool find_file_flag = false;
    u32 ret;
    char *cp = path;

    memset(f_hdl, 0x00, sizeof(FILE_HDL));
    if (*path != '/') { //相对路径
        memcpy(&f_hdl->p_dir, &fs->fs_dir, sizeof(PATH_DIR));
    }
    if (fat_open_dir_bypath(fs, &f_hdl->p_dir, path)) {
        return FS_NO_DIR;
    }
    dir = &f_hdl->p_dir.dir[f_hdl->p_dir.depth];

    //找到文件名位置
    filename = cp;
    while (*cp != '\0') {
        if (*cp == '/') {
            filename = cp + 1;
        }
        cp++;
    }

#if !FAT_WRITE_EN
    if (access & (FAT_CREATE_NEW | FAT_CREATE_ALWAYS | FAT_WRITE)) { //只读文件系统不允许写
        return FS_ACCESS_ERR;
    }
#endif

    while (FS_NO_ERR == fat_dir_next(fs, dir, &file_info)) {
        if (!(file_info.attr & AM_DIR)) { //是文件
            fat_printf("file_name %s:%s\n", file_info.name, filename);
            if (!fat_strcmp(filename, (char *)file_info.name, 0)) { //名字匹配
                fat_printf("open %s sucess\n", filename);
                memcpy(&f_hdl->file_info, &file_info, sizeof(FILE_DIR_INFO));
                f_hdl->fs = fs;
                f_hdl->cur_clust = file_info.st_clust;
                f_hdl->access = access;
                find_file_flag = true;
                break;
            }
        }
    }
    if (find_file_flag == true) {
#if FAT_WRITE_EN
        //文件已经存在
        if (access & FAT_CREATE_ALWAYS) {
            ret = fat_rm_clust_link(fs, f_hdl->cur_clust);
            f_hdl->cur_clust = 0;
            f_hdl->file_info.file_size = 0;
            f_hdl->file_info.st_clust = 0;
            ret = fat_updata_dir(fs, dir, &f_hdl->file_info);
            CHECK_RES(ret);
            ret = fat_syn_fs(fs);
            CHECK_RES(ret);
        } else if (access & FAT_CREATE_NEW) {
            return FS_FILE_EXIST;
        } else
#endif
        {
            /* memcpy(&f_hdl->dir, &dir, sizeof(FAT_DIR)); */
            /* return FS_NO_ERR; */
            ret = FS_NO_ERR;
        }
    } else {
        //文件不存在
#if FAT_WRITE_EN
        if (access & (FAT_CREATE_NEW | FAT_CREATE_ALWAYS)) {
            memset(&file_info, 0x00, sizeof(FAT_DEV_INFO));
            file_info.attr = AM_ARC;
            fat_printf("creat file %s\n", path);
            ret = fat_creat_file(fs, &f_hdl->p_dir, &file_info, path);
            CHECK_RES(ret);
            /* memset(f_hdl, 0x00, sizeof(FILE_HDL)); */
            memcpy(&f_hdl->file_info, &file_info, sizeof(FILE_DIR_INFO));
            f_hdl->fs = fs;
            f_hdl->cur_clust = file_info.st_clust;
            f_hdl->access = access;
            ret = fat_syn_fs(fs);
            CHECK_RES(ret);
        } else 
#endif
        {
            return FS_NO_FILE;
        }
    }
    f_hdl->data_win = alloc_win();
    if(f_hdl->data_win == NULL){
        return FS_WIN_ERR;
    }
    /* memcpy(&f_hdl->dir, &dir, sizeof(FAT_DIR)); */
    return ret;
}

u32 fat_syn_file(FILE_HDL *f_hdl)
{
    FAT_HDL *fs; 
    u32 ret;
    FAT_WIN *win;
    FAT_DIR *dir;

    fs = f_hdl->fs;
    ret = fat_syn_fs(fs);
    CHECK_RES(ret);

#if FAT_TINY
#else
    win = f_hdl->data_win; 
    CHECK_WIN(win, FS_BUSY);
    LOCK_WIN(win);
    ret = fat_syn_win(fs, win);
    UNLOCK_WIN(win);
    CHECK_RES(ret);
#endif
   dir = &f_hdl->p_dir.dir[f_hdl->p_dir.depth];
   f_hdl->file_info.file_size = f_hdl->ptr; 
   ret = fat_updata_dir(fs, dir, &f_hdl->file_info);
   CHECK_RES(ret);

   ret = fat_syn_win(fs, fs->fs_win);
   f_hdl->file_flag &= ~F_DIR_UPDATE;
   return ret;
}

u32 fat_close_file(FILE_HDL *f_hdl)
{
    u32 ret = FS_NO_ERR;
    if(f_hdl->file_flag&F_DIR_UPDATE){ 
        ret = fat_syn_file(f_hdl);
    }
    free_win(f_hdl->data_win);
    f_hdl->data_win = NULL;
    return FS_NO_ERR;
}
/*  */
/* u32 fat_file_close(FILE_HDL *f_hdl) */
/* { */
/*     fat_syn_file(f_hdl); */
/* } */
//删除文件
u32 fat_rm_dirdot(FAT_HDL *fs, FAT_DIR *dir)
{
    u8 FDI[32];
    u32 ret, st_clust;
    u8 attr;

    ret = fat_get_FDI(fs, dir, FDI);
    CHECK_RES(ret); 

    st_clust = ld_word_func(&FDI[20]);
    st_clust = (st_clust<<16) + ld_word_func(&FDI[26]);
    attr = FDI[11];
    if(attr == AM_DIR){  //删除的是目录
       return FS_ACCESS_ERR; 
    }else{
        
    }
 
    ret = fat_rm_clust_link(fs, st_clust);
    CHECK_RES(ret); 

    FDI[0] = 0xE5;  //目录项无效标志
    ret =  fat_write_FDI(fs, dir, FDI);
    CHECK_RES(ret);
    return fat_syn_win(fs, fs->fs_win);
}


u32 fat_file_del(FILE_HDL *f_hdl)
{
    u32 ret;
    ret =  fat_rm_dirdot(f_hdl->fs, &f_hdl->p_dir.dir[f_hdl->p_dir.depth]);
    CHECK_RES(ret);
    free_win(f_hdl->data_win);
    f_hdl->data_win = NULL;
    memset(f_hdl,0x00, sizeof(FILE_HDL));
    return FS_NO_ERR;
}

s32 fat_file_read(FILE_HDL *f_hdl, u8 *buffer, u32 len)
{
    u16 sec_offset;
    u32 read_cnt, cur_sector, read_len;
    /* u32 *cur_clust; */
    FAT_HDL *fs = f_hdl->fs;
    FAT_WIN *win;
    u16 sec_size = fs->sector_512size << 9;

#if FAT_TINY
    win = fs->fs_win;
#else
    win = f_hdl->data_win;
#endif
    /* cur_clust  = &f_hdl->cur_clust; */
    for (read_cnt = 0; read_cnt < len;) {
        // sec_offset  = f_hdl->ptr%FAT_SECTOR_SIZE;
        cur_sector =  get_sector_byclust(fs, f_hdl->cur_clust) + f_hdl->cur_nsector;
        if (cur_sector == 0) {
            return FS_CLUST_ERR;
        }
        sec_offset = f_hdl->ptr & 0x1ff;
        read_len   = FAT_MIN(f_hdl->file_info.file_size - f_hdl->ptr, len - read_cnt);
        fat_printf("read len %d read cnt %d ptr %d file_size %d\n", read_len, read_cnt, f_hdl->ptr, f_hdl->file_info.file_size);
        if (read_len == 0) { //已经完成读取
            return read_cnt;
        }

        if (sec_offset == 0) { //一个扇区的头
            //u16 nclust;
            u32 nsector_max, sector_cnt, nsector, cur_clust, next_clust;
            u8 sclust_nsector, cur_nsector;

            if (f_hdl->cur_nsector == fs->clust_nsector) { //读完一个簇
                next_clust = fat_next_clust(fs, f_hdl->cur_clust);
                if (next_clust == 0) {
                    return read_cnt;
                }
                f_hdl->cur_nsector = 0;
                f_hdl->cur_clust = next_clust;
                if (CHECK_CLUST_END(f_hdl->cur_clust)) { //到达簇链尾
                    return read_cnt;
                }
            }

            if (read_len >= sec_size) { //可读整块扇区
                sclust_nsector = fs->clust_nsector;
                cur_nsector = f_hdl->cur_nsector;
                cur_clust = f_hdl->cur_clust;
                nsector_max = read_len >> 9; //最多可连续读取的扇区数
                for (sector_cnt = 0; sector_cnt < nsector_max;) {
                    if (cur_nsector == sclust_nsector) { //满一个簇
                        next_clust = fat_next_clust(fs, cur_clust);
                        if (next_clust != cur_clust + 1) { //簇不是连续的
                            break;
                        }
                        cur_clust = next_clust;
                        cur_nsector = 0;
                    }
                    nsector = FAT_MIN(nsector_max - sector_cnt, sclust_nsector - cur_nsector);
                    //  nsector = FAT_MIN(nsector, sclust_nsector - cur_nsector);
                    sector_cnt += nsector;
                    cur_nsector += nsector;
                }
                fat_printf("mult sec read %d\n", sector_cnt);
                if (FS_NO_ERR != fat_disk_read(fs, buffer, cur_sector, sector_cnt)) { ///连续读多个扇区
                    return read_cnt;
                }
                ///更新文件信息
                f_hdl->cur_clust = cur_clust;
                f_hdl->cur_nsector = cur_nsector;
                f_hdl->ptr +=  sector_cnt * sec_size;
                read_cnt += sector_cnt * sec_size;
                buffer += sector_cnt * sec_size;
                continue;
            }
        }

        {
            //不可整块读的情况，用到win
            u32 ret;
            read_len = FAT_MIN(FAT_SECTOR_SIZE - sec_offset, read_len);
            CHECK_WIN(win, read_cnt);
            LOCK_WIN(win);
            ret = fat_move_win(fs, win, cur_sector);
            if (ret != FS_NO_ERR) {
                UNLOCK_WIN(win);
                return read_cnt;
            }
            memcpy(buffer, &win->buffer[sec_offset], read_len);
            UNLOCK_WIN(win);
            read_cnt += read_len;
            f_hdl->ptr += read_len;
            buffer += read_len;

            if (!(f_hdl->ptr % sec_size)) { //读完了一个扇区
                f_hdl->cur_nsector++;
            }
        }
    }
    return read_cnt;
}

s32 fat_file_write(FILE_HDL *f_hdl, u8 *buffer, u32 len)
{
    u16 sec_offset;
    u32 write_cnt, cur_sector, write_len;
    //u32 *cur_clust;
    FAT_HDL *fs = f_hdl->fs;
    FAT_WIN *win;
    u16 sec_size = fs->sector_512size << 9;
    u32 ret;

#if !FAT_WRITE_EN
    if (f_hdl->access & (FAT_CREATE_NEW | FAT_CREATE_ALWAYS | FAT_WRITE)) { //只读文件系统不允许写
        return -FS_ACCESS_ERR;
    }
#endif

#if FAT_TINY
    win = fs->fs_win;
#else
    win = f_hdl->data_win;
#endif
    if(f_hdl->cur_clust == 0){
        if(f_hdl->ptr == 0){ //is null file
            u32 sclust;
            sclust = fat_find_empty_clust(fs);
            if(sclust == 0){
                return -FS_IS_FULL; 
            }
            ret = fat_update_clust_node(fs, sclust, 0x0fffffff); //添加结束标志
            CHECK_RES(-ret);
            f_hdl->file_info.st_clust = sclust;
            f_hdl->cur_clust = sclust;
        }
    }
    f_hdl->file_flag |= F_DIR_UPDATE;

    for (write_cnt = 0; write_cnt < len;) {
        cur_sector =  get_sector_byclust(fs, f_hdl->cur_clust) + f_hdl->cur_nsector;
        if (cur_sector == 0) {
            return -FS_CLUST_ERR;
        }
        sec_offset = f_hdl->ptr & 0x1ff;
        write_len   = len - write_cnt ;//FAT_MIN(f_hdl->dir_info.file_size - f_hdl->ptr, len - read_cnt);
        fat_printf("write len %d write cnt %d ptr %d file_size %d\n", write_len, write_cnt, f_hdl->ptr, f_hdl->file_info.file_size);
        if (write_len == 0) { //已经写完成
            return write_cnt;
        }

        if (sec_offset == 0) { //一个扇区的头
            u32 nsector_max, sector_cnt, nsector, cur_clust, next_clust;
            u8 sclust_nsector, cur_nsector;
            if (f_hdl->cur_nsector == fs->clust_nsector) { //写完一个簇
                next_clust = fat_next_clust(fs, f_hdl->cur_clust);
                if (next_clust == 0) {
                    return write_cnt;
                }
                if (CHECK_CLUST_END(next_clust)) { //到达簇链尾
                    u32 nclust;
                    nclust = (write_len>>9)/fs->clust_nsector; 
                    if(nclust == 0){
                       nclust = 1; 
                    }
                    ret = fat_creat_clust_link(fs, f_hdl->cur_clust, nclust);
                    CHECK_RES(ret);
                    continue;
                }
                f_hdl->cur_nsector = 0;
                f_hdl->cur_clust = next_clust;
            }

            if (write_len >= sec_size) { //可写整块扇区
                sclust_nsector = fs->clust_nsector;
                cur_nsector = f_hdl->cur_nsector;
                cur_clust = f_hdl->cur_clust;

                nsector_max = write_len>>9; //最多可连续读取的扇区数
                //calculate mult write size
                for (sector_cnt = 0; sector_cnt < nsector_max;) {
                    if (cur_nsector == sclust_nsector) { //满一个簇
                        next_clust = fat_next_clust(fs, cur_clust);
                        if (next_clust != cur_clust + 1) { //簇不是连续的
                            break;
                        }
                        cur_clust = next_clust;
                        cur_nsector = 0;
                    }
                    nsector = FAT_MIN(nsector_max - sector_cnt, sclust_nsector - cur_nsector);
                    sector_cnt += nsector;
                    cur_nsector += nsector;
                }
                fat_printf("mult sec write %d\n", sector_cnt);
                if (FS_NO_ERR != fat_disk_write(fs, buffer, cur_sector, sector_cnt)) { ///连续读多个扇区
                    return write_cnt;
                }
                ///update file info
                f_hdl->cur_clust = cur_clust;
                f_hdl->cur_nsector = cur_nsector;
                f_hdl->ptr +=  sector_cnt * sec_size;
                write_cnt += sector_cnt * sec_size;
                buffer += sector_cnt * sec_size;
                continue;
            }
        }

        {
            //不可整块读的情况，用到win
            write_len = FAT_MIN(FAT_SECTOR_SIZE - sec_offset, write_len);
            CHECK_WIN(win, write_cnt);
            LOCK_WIN(win);
            ret = fat_move_win(fs, win, cur_sector);
            if (ret != FS_NO_ERR) {
                UNLOCK_WIN(win);
                return write_cnt;
            }
            memcpy(&win->buffer[sec_offset], buffer, write_len);
            win->flag |= DIRTY;
            UNLOCK_WIN(win);
            write_cnt += write_len;
            f_hdl->ptr += write_len;
            buffer += write_len;

            if (!(f_hdl->ptr % sec_size)) { //读完了一个扇区
                f_hdl->cur_nsector++;
            }
        }
    }
    return write_cnt;
}

//指定文件指针
u32 fat_file_seek(FILE_HDL *f_hdl, s32 seek, u8 mode)
{
    u32 tag_seek;
    u16 sector_size = f_hdl->fs->sector_512size << 9;
    switch (mode) {
    case FAT_SEEK_HEAD:
        tag_seek = seek;
        break;
    case FAT_SEEK_TAIL:
        if (seek < 0) {
            if ((-seek) > f_hdl->file_info.file_size) {
                tag_seek = 0;
                break;
            }
        }
        tag_seek = f_hdl->file_info.file_size + seek;
        break;
    case FAT_SEEK_CUR:
        if (seek < 0) {
            if ((-seek) > f_hdl->ptr) {
                tag_seek = 0;
                break;
            }
        }
        tag_seek = f_hdl->ptr + seek;
        break;
    default:
        return FS_INPUT_PARM_ERR;
    }
    if (tag_seek > f_hdl->file_info.file_size) {
#if FAT_WRITE_EN
        if (f_hdl->access & FAT_WRITE_ADD) { //追加文件
            //追加数据
        } else
#endif
        {
            return FS_FILE_END;
        }
    }
    if (tag_seek < f_hdl->ptr) {
        u32 offsize = f_hdl->ptr - tag_seek;
        u32 used_culst_size = f_hdl->cur_nsector * sector_size + f_hdl->ptr % sector_size;
        fat_printf("offsize %d used_culst_size %d\n", offsize, used_culst_size);
        if (offsize > used_culst_size) { //已经超出当前簇的范围
            fat_printf("restar file ptr\n");
            f_hdl->ptr = 0;
            f_hdl->cur_nsector = 0;
            f_hdl->cur_clust = f_hdl->file_info.st_clust;
        } else {
            fat_printf("seek in clust\n");
            offsize = used_culst_size - offsize;  //对簇起始的偏移
            f_hdl->cur_nsector = offsize / sector_size;
            f_hdl->ptr = tag_seek;
            return FS_NO_ERR;
        }
    }

    u32 *ptr = &f_hdl->ptr;
    u32 seek_unit;
    u16 sclust_nsector = f_hdl->fs->clust_nsector;
    for (; *ptr < tag_seek;) {
        if (f_hdl->cur_nsector == sclust_nsector) {
            u32 next_clust = fat_next_clust(f_hdl->fs, f_hdl->cur_clust);
            if (CHECK_CLUST_END(next_clust)) {
                return FS_CLUST_ERR;
            }
            f_hdl->cur_clust = next_clust;
            f_hdl->cur_nsector = 0;
        }
        seek_unit = FAT_MIN(sector_size, sector_size - *ptr % sector_size);
        seek_unit = FAT_MIN(seek_unit, tag_seek - *ptr);
        if (seek_unit == 0) {
            return FS_NO_ERR;
        }
        *ptr += seek_unit;
        if (*ptr % sector_size == 0) {
            f_hdl->cur_nsector++;
        }
    }
    return FS_NO_ERR;
}

u32 fat_file_tell(FILE_HDL *f_hdl)
{
    return f_hdl->ptr;
}

//格式化fat32
u32 fat_format(FAT_DEV_INFO *dev_api, u32 st_sec, u32 nsec)
{
    u8 dbr[512];
    u8 *bpb;
    u32 nsector;

    if (dev_api == NULL) {
        return FS_DISK_ERR;
    }

    memset(dbr, 0x00, 512);
    dbr[510] = 0x55;
    dbr[511] = 0xaa;
    bpb = &dbr[BPB_OFFSET];

    st_word_func(bpb, FAT_SECTOR_SIZE);     //sector size
    bpb[2] = 0x8;                           //nsec sclust per
    st_word_func(&bpb[3], 32);              //reserved sector
    bpb[5] = 2;                             //num of fat table 
    st_dword_func(&bpb[21], nsec);          //total sector
    st_dword_func(&bpb[21], (nsec>>7)+1);   // num sector for fat table
    st_dword_func(&bpb[33], 2);             //root dir sclust
    st_word_func(&bpb[37], 1);              //fs info sector
    st_word_func(&bpb[39], 6);              //dir backup sector
    memcpy(&bpb[60], "NO_NAME", 8);         //label
    memcpy(&bpb[71], "FAT32", 6);           //fs ID

    if (FS_NO_ERR != dev_api->dev_write(dev_api->dev, dbr, st_sec, 1)) {
        return FS_DISK_ERR;
    }

    if (FS_NO_ERR != dev_api->dev_write(dev_api->dev, dbr, st_sec + 6, 1)) {
        return FS_DISK_ERR;
    }

    //clear fat table and root dir
    st_sec += 32; 
    nsector = ((nsec>>7)+1)*2 + 8;
    memset(dbr, 0x0, 512);
    st_dword_func(&bpb[0], 0x0ffffff8);      
    st_dword_func(&bpb[4], 0xffffffff);             
    st_dword_func(&bpb[8], 0x0fffffff);             
    if (FS_NO_ERR != dev_api->dev_write(dev_api->dev, dbr, st_sec, 1)) {
        return FS_DISK_ERR;
    }
    st_sec++; 
    nsector--;

    memset(dbr, 0x0, 512);
    for(; nsector; nsector--, st_sec++){
        if (FS_NO_ERR != dev_api->dev_write(dev_api->dev, dbr, st_sec, 1)) {
            return FS_DISK_ERR;
        }
    }
    return FS_NO_ERR; 
}

