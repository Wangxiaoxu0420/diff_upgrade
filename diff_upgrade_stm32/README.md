我将readme类似内容放在了我的博客：

[代码说明:https://blog.csdn.net/qq_35333978/article/details/128211763?spm=1001.2014.3001.5501](https://blog.csdn.net/qq_35333978/article/details/128211763?spm=1001.2014.3001.5501)

PS：很多人调不通99%都是因为申请内存那里有问题，要么malloc是自己写的、移植的有问题，要么就是给的堆内存不够。该套源码很多人用了很多工程都没问题的，基本无bug，如果调不通，注意以下几点：
1. 可以先用malloc测试，堆内存给20k以上，如果想缩减也是可以，需要修改源码每次还原的长度小一点，目前每次还原1k。
2. 检查堆、栈，如果调不通，可以先给比较大的值测试，基本不是堆就是栈的问题，源码是没有问题的，大量人测试过的。


/**
 * 重要的两个结构体
 */

 1. 用户需要手动赋值给该结构体，结构体定义在user_interface.h中，主要包含以下几个参数：
 typedef struct user_interface
{
    uint8_t *buf;       //buf       差分升级包
    uint32_t len;       //len       差分升级包长度
    uint32_t romaddr;   //romaddr   rom中当前版本代码存放的地址
    uint32_t dstaddr;   //dstaddr   还原升级包要写入的flash地址或RAM地址，一般为外部flash存储升级代码
    BUFCOPY bs_write;   //bs_write  差分升级包写入flash回调函数，例如：norflash_write，W25QXX_Write，memcpy等
    BUFCOPY bs_read;    //bs_read   差分升级包读取flash回调函数，例如：norflash_read，W25QXX_Read，memcpy等
    /* data */
}user_interface_t;

在自己的应用程序中，用户需要自己定义一个全局变量，
user_interface_t user = {0};

并赋值给该结构体，例如：
user.bs_read = W25QXX_Read;     //升级包读取函数，用与读取升级包写入是否完整，可以做MD5校验，md5已经包含在库里
user.bs_write = W25QXX_Write;   //升级包写入函数，用与写入升级包到flash，可以是norflash等其他写函数
user.buf = g_usart_rx_buf;      //差分升级包的地址，一般是串口接收到的buf，也可以是其他方式获取的
user.len = g_usart_rx_cnt;      //差分升级包的长度，一般是串口接收到的buf长度，也可以是其他方式获取的
user.romaddr = FLASH_APP1_ADDR; //rom中当前版本代码存放的地址，一般是固件的起始地址，例如STM32F103的0x08000000，如果有bootloader，则是后面APP的起始地址
user.dstaddr = 0xF00000;        //还原升级包要写入的flash地址或RAM地址，一般为外部flash存储升级代码，加入我使用16M外部flash，我可以将其设置为0xF00000，使用最后1M空间存储升级包，可以根据实际调整，放哪里都可以

之后调用差分升级函数即可完成差分升级：
diffupgrade( &user);

 2. 差分升级包头存储的格式，结构体定义在user_interface.h中，主要包含以下几个参数：
typedef struct upgrade_header
{
    uint8_t MD5[16];    /* MD5值校验 */
    uint32_t ih_crc;    /* CRC32 */
    uint32_t ih_size;   /* 升级文件大小 */
    uint32_t ih_flag;   /* 升级标志位 */
    uint32_t reserve;   /* 预留标志位 */
} upgrade_header_t; 

当前文件使用CRC32校验，也可以更换为MD5校验，把该结构体中ih_crc改为ih_md5即可。
ih_size是升级包的大小，ih_flag是升级标志位，reserve是预留标志位，目前没有用到，可以不用管。

升级包还原后，写入flash中时，会自动将该结构写入升级包最前面，等bootloader读取时，会自动解析该结构，并根据ih_flag判断是否需要还原升级包。

 3. 用户需要自己实现内存分配和释放函数
当前我使用了最简单的printf和malloc/free，如果需要使用其他的内存分配和释放函数，则需要修改源码，将malloc/free替换为自己的函数，例如：
#define bs_printf(...) printf(__VA_ARGS__)
#define bs_malloc(...) vmalloc(__VA_ARGS__)
#define bs_free(...) vfree(__VA_ARGS__)

 4. 用户提供的flash读写函数应该是如下类型：
typedef void (*BUFCOPY)(uint32_t dst_addr, uint32_t src_addr, uint32_t len);
其中第一个参数为目标地址，第二个参数为源地址，第三个参数为长度。
如果使用NORFLASH，则可以参考norflash_read和norflash_write函数，如果使用RAM，则可以参考memcpy函数。
如果使用W25QXX芯片，则可以参考W25QXX_Read和W25QXX_Write函数，如果使用其他芯片，则需要自己实现相应的读写函数。

如果用户的读写函数参数位置与给定类型不一致，则需要用户手动调整为该格式后，作为参数传递给差分升级函数。

