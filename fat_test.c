
#include "sdk_cfg.h"
#include "fs.h"
#include "dev_manage.h"
#include "lxfat.h"
#include "mbr.h"
#include "key.h"
#include "msg.h"

/////FAT  API //////////////////////////
#if 1
static u32 fs_dev_read_api(void *hdev, u8 *buf, u32 addr, u32 len)
{
    if (512 * len != dev_read(hdev, buf, addr, len)) {
        return 1;
    }
    return 0;
}

static u32 fs_dev_write_api(void *hdev, u8 *buf, u32 addr, u32 len)
{
    put_buf(buf, len*512);
    if (512*len != dev_write(hdev, buf, addr, len)) {
        return 1;
    }
    return 0;
}

extern u32 STACK_START[];
extern u32 STACK_END[];
void stack_check()
{
    u32 check_tmp;
    u32 used_size;
    used_size = (u32)&check_tmp - (u32)STACK_END; 
    printf("stack start %x   stack end %x used_size %d\n", STACK_START, STACK_END, used_size);
}

#define ADKEY_FAT_SHORT		\
                        /*00*/    MSG_FS_FLIE_OP,\
                        /*01*/    NO_MSG,\
                        /*02*/    NO_MSG,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    NO_MSG,

#define ADKEY_FAT_LONG		\
                        /*00*/    MSG_FS_WRITE_OP,\
                        /*01*/    MSG_FS_FILE_MK,\
                        /*02*/    NO_MSG,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    MSG_CHANGE_WORKMODE,

#define ADKEY_FAT_HOLD		\
                        /*00*/    NO_MSG,\
                        /*01*/    NO_MSG,\
                        /*02*/    NO_MSG,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    NO_MSG,

#define ADKEY_FAT_LONG_UP	\
                        /*00*/    NO_MSG,\
                        /*01*/    NO_MSG,\
                        /*02*/    NO_MSG,\
                        /*03*/    NO_MSG,\
                        /*04*/    NO_MSG,\
                        /*05*/    NO_MSG,\
                        /*06*/    NO_MSG,\
                        /*07*/    NO_MSG,\
                        /*08*/    NO_MSG,\
                        /*09*/    NO_MSG,

const u16 task_fat_ad_table[4][KEY_REG_AD_MAX] = {
    /*短按*/	    {ADKEY_FAT_SHORT},
    /*长按*/		{ADKEY_FAT_LONG},
    /*连按*/		{ADKEY_FAT_HOLD},
    /*长按抬起*/	{ADKEY_FAT_LONG_UP},
};

const KEY_REG task_fat_key = {
#if (KEY_AD_RTCVDD_EN||KEY_AD_VDDIO_EN)
    ._ad = task_fat_ad_table,
#endif
};

static _mbr_info mbr;
static FAT_HDL fs_hd;
static FILE_HDL file_hdl;
static u8 fat_data_buffer1[124];
static char filename[] = "/0000.fat";
static u16 namecnt = 0;
static FAT_HDL *fs_p = NULL;
static FILE_HDL *f_p = NULL;
char *get_filename()
{
    filename[1] = namecnt%10000/1000 + '0'; 
    filename[2] = namecnt%1000/100 + '0'; 
    filename[3] = namecnt%100/10 + '0'; 
    filename[4] = namecnt%10 + '0'; 
    return filename;
}
//static u8 fat_data_buffer[1024 * 10];
FAT_HDL *__fs_open(void *dev, u32 boot)
{
    FAT_DEV_INFO dev_info;

    memset(&fs_hd, 0x00, sizeof(FAT_HDL));

    dev_info.dev_block_size = 512;
    dev_info.dev            = dev;
    dev_info.dev_read       = fs_dev_read_api;
    dev_info.dev_write      = fs_dev_write_api;

    if (fat_open_fs(&fs_hd, &dev_info, boot)) {
        return NULL; 
    }

    printf("open fs succ\n");
    return &fs_hd;
}

void __fs_open_file() 
{
    u32 res;
    if(fs_p == NULL){
        printf("no open fs\n");
       return; 
    }
    res = fat_open_file(&fs_hd, &file_hdl, "/111/222/333/444/555/666/777/LXFAT.TXT", 0);
     if (!res) {
     /* if (!fat_open_file(&fs_hd, &file_hdl, get_filename(), 0)) { */
        printf("file size %d\n", file_hdl.file_info.file_size);
        while (124 == fat_file_read(&file_hdl, fat_data_buffer1, 124)) {
            printf("%s", fat_data_buffer1);
        }
    }else{
        printf("open err %d\n", res);
        return;
    }
    f_p = &file_hdl;
}

void __fs_write_file()
{
    u32 ret;
    if(fs_p == NULL){
        printf("no open fs\n");
        return; 
    }

open_file:
    ret = fat_open_file(&fs_hd, &file_hdl, get_filename(), FAT_CREATE_NEW);
    if(ret){
        printf("open file  %x\n", ret);
        if(ret == FS_FILE_EXIST){
            namecnt++; 
            goto open_file;
        }
    }
     f_p = &file_hdl;
}

void __fs_mk_dir()
{
    u32 ret;
    if(fs_p == NULL){
        printf("no open fs\n");
        return; 
    }

    ret = fat_mk_dir(&fs_hd, NULL, "/0001");
    if(ret){
       printf("open dir %d\n", ret); 
    }
}

void __fs_del()
{
    u32 ret;
    if(fs_p == NULL){
        printf("no open fs\n");
        return; 
    }
    if(f_p == NULL){
        printf("no open file\n");
        return;
    }

    ret = fat_file_del(&file_hdl);
    if(ret){
       printf("open dir %d\n", ret); 
    }
}

#include "wdt.h"
void fat_loop()
{
    u32 msg_error;
    int msg;
    key_msg_table_reg(&task_fat_key);

    while(1){
        clear_wdt();

        msg_error = task_get_msg(0, 1, &msg);

        if (NO_MSG == msg) {
            continue;
        }
        if((msg != MSG_HALF_SECOND)&&(msg != MSG_ONE_SECOND)){
            printf("fat mgs %x\n", msg);
        }
        switch(msg){
            case MSG_SD0_ONLINE:
                puts("MSG_SD0_ONLINE\n");
                if (dev_online_mount(sd0)) {
                    if (__mbr_scan(sd0, &mbr)) {
                        printf("mbr scan err\n");
                    }
                    if(mbr.part_num){
                        fs_p = __fs_open(sd0, mbr.dev_part[0].start_secter);
                    }
                }
                break;

            case MSG_SD0_OFFLINE:
                puts("MSG_SD0_OFFLINE\n");
                dev_offline_unmount(sd0);
                fs_p = NULL;
                break;

            case MSG_SD1_ONLINE:
                puts("MSG_SD1_ONLINE\n");
                if (dev_online_mount(sd1)) {
                    if (__mbr_scan(sd1, &mbr)) {
                        printf("mbr scan err\n");
                    }
                    if(mbr.part_num){
                        fs_p = __fs_open(sd1, mbr.dev_part[0].start_secter);
                    }

                }
                break;

            case MSG_SD1_OFFLINE:
                puts("MSG_SD1_OFFLINE\n");
                dev_offline_unmount(sd1);
                fs_p = NULL;
                break;

            case MSG_HUSB_ONLINE:
                puts("MSG_HUSB_ONLINE\n");
                if (dev_online_mount(usb)) {
                    if (__mbr_scan(usb, &mbr)) {
                        printf("mbr scan err\n");
                    }
                    if(mbr.part_num){
                        fs_p = __fs_open(usb, mbr.dev_part[0].start_secter);
                    }
                }

                break;

            case MSG_HUSB_OFFLINE:
                puts("MSG_HUSB_OFFLINE\n");
                dev_offline_unmount(usb);
                break;


            case MSG_FS_FLIE_OP:
                __fs_open_file(); 
                break;
            case MSG_FS_FILE_MK:
                __fs_mk_dir();
                break;
            case MSG_FS_WRITE_OP:
                __fs_write_file();
                break;
            case MSG_FS_DEL:
                __fs_del();
                break;
            case MSG_CHANGE_WORKMODE:
                return;
                break;
            default:
                break;
        }

    }
}
#endif
#if 0
void fs_FAT_test(void *dev)
{
    FAT_DEV_INFO dev_info;

    if (__mbr_scan(dev, &mbr)) {
        printf("mbr scan err\n");
    }

    memset(&fs_hd, 0x00, sizeof(FAT_HDL));

    dev_info.dev_block_size = 512;
    dev_info.dev            = dev;
    dev_info.dev_read       = fs_dev_read_api;
    dev_info.dev_write      = fs_dev_write_api;

    if (!fat_open_fs(&fs_hd, &dev_info, mbr.dev_part[0].start_secter)) {
        FAT_DIR dir;
        FILE_HDL file_hdl;
        fs_hd.file_totle =  dir_scan_test(&fs_hd, fs_hd.root_dir_sclust);
        printf("file_totle %d\n", fs_hd.file_totle);
        printf("opend dir %d\n", fat_open_dir_bypath(&fs_hd, &dir, "/111/222/333/444"));
        if (!fat_open_file(&fs_hd, &file_hdl, "/111/222/333/444/555/666/777/LXFAT.TXT", 0)) {
            printf("file size %d\n", file_hdl.file_info.file_size);
            while (fat_file_read(&file_hdl, fat_data_buffer1, 124)) {
                printf("%s", fat_data_buffer1);
                //      printf("file tell %d\n", fat_file_tell(&file_hdl));
                //     fat_file_seek(&file_hdl, -97, FAT_SEEK_CUR);
                //     printf("file tell %d\n", fat_file_tell(&file_hdl));

            }
            printf("file tell %d\n", fat_file_tell(&file_hdl));
            fat_file_seek(&file_hdl, -(1024 * 30), FAT_SEEK_CUR);
            printf("file tell %d\n", fat_file_tell(&file_hdl));

            printf("read size %d\n", fat_file_read(&file_hdl, fat_data_buffer, 10240));
            printf("%s", fat_data_buffer);
        }
    }
}
#endif

#if 0
u32 dir_scan_test(FAT_HDL *fs, u32 sclust)
{
    FAT_DIR dir;
    FILE_DIR_INFO file_info;
    u32 file_totle = 0;
    fat_ld_dir(fs, &dir, sclust);
    while (FS_NO_ERR == fat_dir_next(fs, &dir, &file_info)) {
        if (file_info.attr & AM_DIR) { //改文件时子目录
            if (!(file_info.attr & AM_SYS)) {
                if (file_info.name[0] == '.') {
                    if ((file_info.name[1] == '.') || (file_info.name[1] == '\0')) {
                        continue;
                    }
                }
                file_totle += dir_scan_test(fs, file_info.st_sclust);
            }
        }
        file_totle++;
    }

    return file_totle;
}
#endif
