/**
 * @file user_interface.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-11-09
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "user_interface.h"
#include "bs_type.h"
#include "md5.h"

static BUFCOPY bs_write;
static BUFCOPY bs_read;


/**
 * @brief 将差分文件写入用户的flash,用户自己决定是否在写之前擦除
 *
 * @param addr
 * @param p
 * @param len
 * @return int
 */
int bs_flash_write(uint32_t addr, const void *p, uint32_t len)
{
    bs_write((uint32_t)p, addr, len);
    if (0)
    {
        printf("norflash_write err");
        return 1;  /* 写入flash失败返回错误码 */
    }
    return 0;
}

/**
 * @brief 差分升级
 * 
 * @param buf   差分升级包
 * @param len   差分升级包长度
 * @param romaddr  rom中当前版本代码存放的地址
 * @param dstaddr  还原升级包要写入的flash地址或RAM地址，一般为外部flash存储升级代码
 * @param copy_func  差分升级包写入flash回调函数，例如：norflash_write，W25QXX_Write，memcpy等
 * @return int  0: 成功，其他: 失败原因
 * 
 * @example ret = diffupgrade( g_usart_rx_buf, g_usart_rx_cnt, FLASH_APP1_ADDR, 0xF00000, W25QXX_Write);
 */
int diffupgrade( user_interface_t *user)
{
    image_header_t *recv_head = (image_header_t *)user->buf;
    upgrade_header_t upgrade_head;

    uint32_t recv_hcrc;  /* 接收到的文件头CRC */
    uint32_t calculation_crc;  /* 计算出来的文件头CRC */

    bs_write = user->bs_write;//注册回调函数
    bs_read =  user->bs_read;//注册回调函数
    recv_hcrc = BigtoLittle32(recv_head->ih_hcrc);
    recv_head->ih_hcrc = 0;
    calculation_crc = crc32((uint8_t *)recv_head, sizeof(image_header_t));
    //如果差分包头校验不通过立即返回失败
    if (recv_hcrc != calculation_crc)
        return 1; 

    recv_head->ih_hcrc = recv_hcrc;
    recv_head->ih_time = BigtoLittle32(recv_head->ih_time);
    recv_head->ih_size = BigtoLittle32(recv_head->ih_size);
    recv_head->ih_dcrc = BigtoLittle32(recv_head->ih_dcrc);
    recv_head->ih_ocrc = BigtoLittle32(recv_head->ih_ocrc);
    /* 差分升级包 */
    if (crc32((uint8_t *)user->romaddr, recv_head->ih_load) != recv_head->ih_ocrc)
    {
        printf("file oldcrc err,calcrc:0X%08X, ih_oldbin_crc:0X%08X,", 
        crc32((uint8_t *)user->romaddr, 
        recv_head->ih_load), recv_head->ih_ocrc);
        return 1;  /* 本地应用校验失败 */
    }
    //写入flash的时候，要提前预留出一个升级包头的位置
    recv_hcrc = iap_patch((uint8_t *)user->romaddr, recv_head->ih_load, 
                        (uint8_t *)(user->buf + sizeof(image_header_t)),  
                        recv_head->ih_size, user->dstaddr + sizeof(upgrade_header_t));
    //校验新文件大小和还原后的是否一致
    if (recv_hcrc != recv_head->ih_ep)
    {
        printf("iap_patch len err.");
        printf("iap_patch len: %lu, new_len: %lu", recv_hcrc, recv_head->ih_ep);
        return 1;
    }

    printf("recv_hcrc is %x\r\n",     recv_hcrc);
    uint8_t md5buf[16];
    MD5(user->dstaddr + sizeof(upgrade_header_t),recv_head->ih_ep,md5buf,bs_read);
    printf("md5buf is [");
    int i;
    for(i = 0; i < 16; i++)
        printf("%x ", md5buf[i]);
    printf("]\r\n");

    memcpy(recv_head->MD5, md5buf, 16);
    //把升级文件CRC32校验和升级文件大小和升级标志写入flash中
    upgrade_head.ih_crc = recv_hcrc;
    upgrade_head.ih_size = recv_head->ih_ep;
    upgrade_head.ih_flag = 0x5A5A5A5A;
    bs_write((uint32_t)&upgrade_head, user->dstaddr, sizeof(upgrade_header_t));



    return 0;
}



