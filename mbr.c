
#include "sdk_cfg.h"
#include "mbr.h"
#include "dev_manage.h"

#define MBR_DEBUG
#ifdef MBR_DEBUG
#define mbr_printf          printf
#define mbr_printf_buf      printf_buf
#else
#define mbr_printf(...)
#define mbr_printf_buf(...)
#endif

#define SYS_BIG_EDIAN           0
#if SYS_BIG_EDIAN
#else
#endif

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
/* //从小端模式数组获取数据 */
/* u32 get_num_by_little_end_arr(u8 *arr, u8 len) */
/* { */
/*     u32 ret = 0; */
/*     if (len > 4) { */
/*         return 0; */
/*     } */
/*     while (len--) { */
/*         ret |= arr[len]; */
/*         if (len == 0) { */
/*             break; */
/*         } */
/*         ret <<= 8; */
/*     } */
/*     return ret; */
/* } */
/*  */
/*  */
/* //从大端模式数组获取数据 */
/* u32 get_num_by_big_end_arr(u8 *arr, u8 len) */
/* { */
/*     u32 ret = 0; */
/*     if (len > 4) { */
/*         return 0; */
/*     } */
/*     while (len--) { */
/*         ret |= *arr++; */
/*         if (len == 0) { */
/*             break; */
/*         } */
/*         ret <<= 8; */
/*     } */
/*     return ret; */
/* } */
/*  */
#include "fs.h"
static int mbr_read(void *dev, u8 *buffer)
{
    return read_api(dev, buffer, 0);
}
static int mbr_write(void *dev, u8 *buffer)
{
    return write_api(dev, buffer, 0);
}

bool check_dpt(u8 *dpt_buf)
{
    bool mbr;
    u32 start_sec, nsec;
    u8 dpt_cnt;
    mbr = true;
    start_sec = ld_dword_func(&dpt_buf[8]);
    nsec = ld_dword_func(&dpt_buf[12]);

    if ((dpt_buf[0] != 0) && (dpt_buf[0] != 0x80)) {
        mbr = false;
    }

    if (dpt_buf[4] == 0) {
        mbr = false;
    }

    if (start_sec == 0) {
        mbr = false;
    }

    if (nsec == 0) {
        mbr = false;
    }
    return mbr;
}


int __mbr_scan(void *dev, _mbr_info *mbr_info)
{
    u8  mbr_buf[512];

    if (mbr_info == NULL) {
        return MBR_PTR_NULL;
    }

    memset(mbr_info, 0x00, sizeof(_mbr_info));
    mbr_info->dev = dev;
    if (DEV_ERR_NONE != dev_io_ctrl(dev, DEV_GET_BLOCK_SIZE, &mbr_info->dev_block_size)) {
        return MBR_DEV_OP_ERR;
    }
    mbr_printf("DEV_GET_BLOCK_SIZE %d\n", mbr_info->dev_block_size);
    if (mbr_read(dev, mbr_buf)) {
        return MBR_DEV_OP_ERR;
    }
    mbr_printf_buf(mbr_buf, 512);
    //校验标记
    if ((mbr_buf[MBR_TAG_OFFSET] != 0x55) || (mbr_buf[MBR_TAG_OFFSET + 1] != 0xaa)) {
        return MBR_TAG_NO_MATCH;
    }
    //   memcpy(mbr_info->dev_part, &mbr_buf[PART_INFO_OFFSET], sizeof(struct _dev_part)*4);

    int cnt = 0;
    u8 *part_p = &mbr_buf[PART_INFO_OFFSET];
    for (; cnt < 4; cnt++, part_p += 16) {
        mbr_printf("**************part %d **********\n", cnt);
        /* if (((part_p[0] == 0x80) || (part_p[0] == 0x00)) && (part_p[4])) { // 校验分区有效性 */
        if(true == check_dpt(part_p)){ // check dpt exist
            mbr_info->dev_part[mbr_info->part_num].active_flag = part_p[0];
            mbr_info->dev_part[mbr_info->part_num].start_head = part_p[1];
            mbr_info->dev_part[mbr_info->part_num].start_cyl_secter = ld_word_func(part_p + 2);
            mbr_info->dev_part[mbr_info->part_num].sys_id = part_p[4];
            mbr_info->dev_part[mbr_info->part_num].end_head = part_p[5];
            mbr_info->dev_part[mbr_info->part_num].end_cyl_secter = ld_word_func(part_p + 6);
            mbr_info->dev_part[mbr_info->part_num].start_secter = ld_dword_func(part_p + 8);
            mbr_info->dev_part[mbr_info->part_num].totle_sector = ld_dword_func(part_p + 12);
            mbr_printf("sys id %x\n", mbr_info->dev_part[mbr_info->part_num].sys_id);
            mbr_printf("part start sec 0x%x\n", mbr_info->dev_part[mbr_info->part_num].start_secter);
            mbr_printf("part totle sec %d\n", mbr_info->dev_part[mbr_info->part_num].totle_sector);
            mbr_printf("part size: %dM\n", (mbr_info->dev_part[mbr_info->part_num].totle_sector / 1024)*mbr_info->dev_block_size / 1024);
            mbr_info->part_num++;
        }
    }

    if (mbr_info->part_num) {
        return 0;
    } else {
        return MBR_NO_PART;
    }
}

//在MBR上创建一个分区
u32 mbr_creat_part(void *dev, u8 fs_type, u32 nsector)
{
    u8  mbr_buf[512];
    u8 *part_p = &mbr_buf[PART_INFO_OFFSET];
    u8 *null_part =  NULL;
    u32 end_sector;
    u32 st_sector = 32; 
    u32 total_sector;

    if (dev == NULL) {
        return MBR_PTR_NULL;
    }

    if (DEV_ERR_NONE != dev_io_ctrl(dev, DEV_GET_BLOCK_NUM, &total_sector)) {
        return MBR_DEV_OP_ERR;
    }
 
    if (mbr_read(dev, mbr_buf)) {
        return MBR_DEV_OP_ERR;
    }
    //校验标记
    if ((mbr_buf[MBR_TAG_OFFSET] != 0x55) || (mbr_buf[MBR_TAG_OFFSET + 1] != 0xaa)) {
        memset(mbr_buf, 0x00, 512);
    }

    u8 cnt;
    for (cnt = 0; cnt < 4; cnt++, part_p += 16) {
        if(false == check_dpt(part_p)){ // check dpt exist
            if(null_part == NULL){
                null_part = part_p;
            }
        }else{
           end_sector =  ld_dword_func(part_p + 8) + ld_dword_func(part_p + 12);
           if(end_sector > st_sector){
               st_sector = end_sector;
           }
        }
    }
    if(null_part == NULL){
        return MBR_PTR_NULL;    
    }
    if(nsector>(total_sector - end_sector)){
        nsector = total_sector - st_sector;
    }

    null_part[0] = 0x00; 
    null_part[4] = fs_type; 
    st_dword_func(&null_part[8], st_sector);
    st_dword_func(&null_part[12], nsector);
    if(mbr_write(dev, mbr_buf)){
        return MBR_DEV_OP_ERR; 
    }
    return MBR_NO_ERR;
}
//static u8 mbr_buf_test[512 * 4];
#if 0
void mbr_test(void *dev)
{
    mbr_scan(dev, &mbr);

#if 0
    mbr_printf("8800-------------------\n");
    read_api(dev, mbr_buf_test, 0x01/* + mbr.dev_part[0].start_secter*/);
    mbr_printf_buf(mbr_buf_test, 512);
    mbr_printf("8801-------------------\n");
    read_api(dev, mbr_buf_test, 0x8800/* + mbr.dev_part[0].start_secter*/);
    mbr_printf_buf(mbr_buf_test, 512);

    mbr_printf("8802-------------------\n");
    read_api(dev, mbr_buf_test, 0x8802/* + mbr.dev_part[0].start_secter*/);
    mbr_printf_buf(mbr_buf_test, 512);

    mbr_printf("8803-------------------\n");
    read_api(dev, mbr_buf_test, 0x8803/* + mbr.dev_part[0].start_secter*/);
    mbr_printf_buf(mbr_buf_test, 512);
    mbr_printf("8804-------------------\n");
    read_api(dev, mbr_buf_test, 32768 + mbr.dev_part[0].start_secter + 3);
    mbr_printf_buf(mbr_buf_test, 512);

    while (1);
#endif
}
#endif


