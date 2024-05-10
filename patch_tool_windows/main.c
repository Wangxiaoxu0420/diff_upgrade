/* LzmaUtil.c -- Test application for LZMA compression
2021-11-01 : Igor Pavlov : Public domain */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <LzmaDec.h>

#include "vFile.h"
#include "Alloc.h"

#include "LzmaLib.h"
#include "7zFile.h"

#include "bsdiff.h"
#include "bspatch.h"

#define BigtoLittle16(x)    ((((uint16_t)(x) & 0xff00) >> 8) | (((uint16_t)(x) & 0x00ff) << 8))
#define BigtoLittle32(x)    ((((uint32_t)(x) & 0xff000000) >> 24) | \
                            (((uint32_t)(x) & 0x00ff0000) >> 8) | \
                            (((uint32_t)(x) & 0x0000ff00) << 8) | \
                            (((uint32_t)(x) & 0x000000ff) << 24))

#define IH_MAGIC   0x56190527   /* Image Magic Number       */
#define IH_NMLEN   (32 - 4)     /* Image Name Length        */
#define IH_NMLEN   (32 - 4)     /* Image Name Length        */
#define BIN_HEAD   "ENDSLEY/BSDIFF43"

typedef struct image_header
{
    uint32_t ih_magic;  /* Image Header Magic Number */
    uint32_t ih_hcrc;   /* Image Header CRC Checksum 差分包包头校验 */
    uint32_t ih_time;   /* Image Creation Timestamp */
    uint32_t ih_size;   /* Image Data Size 差分包的大小 */
    uint32_t ih_load;   /* Data Load Address 上一版本旧文件的大小 */
    uint32_t ih_ep;     /* Entry Point Address 要升级的新文件的大小 */
    uint32_t ih_dcrc;   /* Image Data CRC Checksum 新文件的CRC */
    uint8_t  ih_os;     /* Operating System */
    uint8_t  ih_arch;   /* CPU architecture */
    uint8_t  ih_type;   /* Image Type */
    uint8_t  ih_comp;   /* Compression Type */
    uint8_t  ih_name[IH_NMLEN]; /* Image Name */
    uint32_t ih_ocrc;   /* Old Image Data CRC Checksum 上一版本旧文件的CRC */
} image_header_t;

static image_header_t header;


const unsigned int crc32tab[] =
{
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
    0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
    0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
    0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
    0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
    0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
    0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
    0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
    0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
    0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
    0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
    0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
    0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
    0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
    0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
    0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
    0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
    0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
    0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
    0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
    0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
    0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
    0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
    0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
    0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
    0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
    0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
    0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
    0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
    0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
    0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
    0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
    0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
    0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
    0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
    0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
    0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
    0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
    0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
    0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
    0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
    0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
    0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
    0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
    0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
    0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
    0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
    0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
    0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
    0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
    0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
    0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};


unsigned int crc32(const unsigned char *buf, unsigned int size)
{
    unsigned int i, crc;
    crc = 0xFFFFFFFF;

    for (i = 0; i < size; i++)
    {
        crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

static void PrintHelp(char *buffer)
{
    strcat(buffer,
           "lzma: <e|d> inputFile outputFile\n"
           "  e: encode file\n"
           "  d: decode file\n"
           "bsdiff: <b|p> inputOldFile inputNewFile outputFile\n"
           "  b: diff file\n"
           "  p: pach file\n"
           "run: bsdiff inputOldFile inputNewFile outputFile\n"
          );
}

static int PrintError(char *buffer, const char *message)
{
    strcat(buffer, "\nError: ");
    strcat(buffer, message);
    strcat(buffer, "\n");
    return 1;
}

static int PrintUserError(char *buffer)
{
    return PrintError(buffer, "Incorrect command");
}

static int PrintError_WRes(char *buffer, const char *message, WRes wres)
{
    strcat(buffer, "\nError: ");
    strcat(buffer, message);
    sprintf(buffer + strlen(buffer), "\nSystem error code: %d", (unsigned)wres);
// #ifndef _WIN32
    {
        const char *s = strerror(wres);
        if (s)
            sprintf(buffer + strlen(buffer), " : %s", s);
    }
// #endif
    strcat(buffer, "\n");
    return 1;
}


long fsize(FILE *fp)
{
    long n;
    fpos_t fpos;
    fgetpos(fp, &fpos);
    fseek(fp, 0, SEEK_END);
    n = ftell(fp);
    fsetpos(fp, &fpos);
    return n;
}



int lzmaProcess(int numArgs, const char *args[], char *rs)
{
    char c;
    int res;
    int encodeMode;
    unsigned long long Dsize;
    unsigned long long Lsize;
    FILE *fp;

    if (numArgs < 3 || numArgs > 4 || strlen(args[1]) != 1)
    {
        printf("lzmaProcess numArgs err, numArgs = %d\n", numArgs);
        return PrintUserError(rs);
    }

    c = args[1][0];
    encodeMode = (c == 'e' || c == 'E');
    if (!encodeMode && c != 'd' && c != 'D')
    {
        printf("lzmaProcess encodeMode err.");
        return PrintUserError(rs);
    }

    if (!encodeMode)  /* 压缩文件 */
    {
        fp = fopen(args[2], "rb+");  /* 打开待压缩的文件 */
        if (fp == NULL)
            return PrintError_WRes(rs, "Cannot open input file", -1);

        Dsize = fsize(fp);  /* 获取待压缩的文件大小 */
        printf("lzma file size:%llu\n", Dsize);
        char *Dfile = malloc(Dsize);  /* 创建待压缩的文件内存 */

        fread(Dfile, 1, Dsize, fp);  /* 将待压缩的文件写入内存 */

        char *Lfile = malloc(Dsize);  /* 申请压缩内存 */

        Lsize = Dsize;
        res = LzmaCompress(Lfile, &Lsize, Dfile, Dsize);  /* 压缩到内存 */
        if (res != SZ_OK)
        {
            printf("lzma failed to compress, res:%d", res);
            return res;
        }
        free(Dfile);
        printf("lzma success, new file size:%llu\n", Lsize);
        fclose(fp);

        fp = fopen(args[3], "wb+");  /* 创建一个空文件 */
        if (fp == NULL)
            return PrintError_WRes(rs, "Cannot open or creat file", -1);

        fwrite(Lfile, 1, Lsize, fp);  /* 写入压缩文件 */
        fclose(fp);
        free(Lfile);
    }
    else  /* 解压文件 */
    {
        fp = fopen(args[2], "rb+");
        if (fp == NULL)
            return PrintError_WRes(rs, "Cannot open or creat file", -1);

        long Esize = fsize(fp);  /* 获取待解压的文件大小 */
        printf("Pack size:%lu\n", Esize);

        char *Efile = malloc(Esize);  /* 创建待解压的文件内存 */
        fread(Efile, 1, Esize, fp);  /* 将待解压的文件写入内存 */

        char Probs[LZMA_PROPS_SIZE + 8];  /* 放入文件头 */

        memcpy(Probs, Efile, sizeof(Probs)); /* 获取文件头 */
        Esize -= sizeof(Probs);

        // for (int i = 0; i < sizeof(Probs); i++)
        // {
        //     printf("%02X ", (unsigned char)Probs[i]);
        // }

        UInt64 unpack_size = 0;
        for (int i = 0; i < 8; i++)
        {
            unpack_size |= (unsigned char)Probs[LZMA_PROPS_SIZE + i] << (i * 8);
        }

        CLzmaDec state;
        size_t inPos = 0, inSize = 0, outPos = 0;
        ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
        ELzmaStatus status;

        Byte inBuf[1024];
        Byte outBuf[1024];

        LzmaDec_Construct(&state);
        LzmaDec_Allocate(&state, Probs, LZMA_PROPS_SIZE, &g_Alloc);
        LzmaDec_Init(&state);
        vFile *vpf = vfopen(Efile + sizeof(Probs), Esize);

        fp = fopen(args[3], "wb+");
        if (fp == NULL)
            return PrintError_WRes(rs, "Cannot open or creat file", -1);

        for (;;)
        {
            // printf("inPos:%lu ", inPos);
            if (inPos == inSize)
            {
                inSize = sizeof(inBuf);
                inSize = vfread(vpf, inBuf, inSize);
                inPos = 0;
                // printf("inSize:%lu ", inSize);
            }
            {
                SizeT inProcessed = inSize - inPos;
                SizeT outProcessed = sizeof(outBuf) - outPos;
                ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
                ELzmaStatus status;

                if (outProcessed > unpack_size)
                {
                    outProcessed = (SizeT)unpack_size;
                    finishMode = LZMA_FINISH_END;
                }

                res = LzmaDec_DecodeToBuf(&state, outBuf + outPos, &outProcessed,
                                          inBuf + inPos, &inProcessed, finishMode, &status);
                inPos += inProcessed;
                outPos += outProcessed;
                unpack_size -= outProcessed;

                if (outPos > 0)
                {
                    // printf("outPos:%lu\n", outPos);
                    fwrite(outBuf, 1, outPos, fp);  /* 写入结果文件 */
                    fseek(fp, 0, SEEK_END);
                }
                outPos = 0;

                if (res != SZ_OK)
                {
                    printf("Unpack failed, exit:%d\n", res);
                    break;
                }
                if (inProcessed == 0 && outProcessed == 0)
                {

                    if (status != LZMA_STATUS_FINISHED_WITH_MARK)
                    {
                        printf("Unpack success, new file size: %lu\n", ftell(fp));
                        res = SZ_ERROR_DATA;
                    }
                    break;
                }
            }
        }
        free(Efile);
        fclose(fp);
    }

    return res;
}

int bsdiff_write(struct bsdiff_stream *stream, const void *buffer, int size)
{
    FILE *pf = (FILE *) stream->opaque;
    // printf("write size:%u\n", size);
    fwrite((void *)buffer, 1, size, pf);
    fseek(pf, 0, SEEK_END);
    return 0;
}

int bsdiffProcess(int numArgs, const char *args[], char *rs)
{
    char c;
    int res;
    int encodeMode;
    long oldSize;
    long newSize;
    long Lsize;
    FILE *oldFp;
    FILE *newFp;
    int lzmaFlag = 0;

    if (numArgs < 3 || numArgs > 5)
    {
        printf("bsdiffProcess numArgs err, numArgs = %d\n", numArgs);
        return PrintUserError(rs);
    }

    c = args[1][0];
    encodeMode = (c == 'b');
    if (!encodeMode && c != 'p')
    {
        printf("bsdiffProcess encodeMode err.");
        return PrintUserError(rs);
    }

    if (strstr(args[1], "bsdiff") != NULL)
    {
        lzmaFlag = 1;
    }

    if (encodeMode)  /* 差分文件 */
    {
        /* 处理待差分的原文件写到内存 */
        oldFp = fopen(args[2], "rb+");
        if (oldFp == NULL)
            return PrintError_WRes(rs, "Cannot open input file", -1);
        oldSize = fsize(oldFp);  /* 获取原文件的大小 */
        char *oldFile = malloc(oldSize);  /* 创建待压缩的文件内存 */
        printf("%s file size:%lu\n", args[2], oldSize);
        fread(oldFile, 1, oldSize, oldFp);  /* 将待压缩的文件写入内存 */
        {
            /* 组织文件头 */
            header.ih_load = oldSize;  /* 上一版本旧文件的大小 */
            header.ih_ocrc = BigtoLittle32(crc32(oldFile, oldSize));   /* 原文件CRC大端放入 */
        }
        fclose(oldFp);

        /* 处理待差分的新文件写到内存 */
        newFp = fopen(args[3], "rb+");  /* 打开待差分的原文件 */
        if (newFp == NULL)
        {
            free(oldFile);
            return PrintError_WRes(rs, "Cannot open input file", -1);
        }
        newSize = fsize(newFp);  /* 获取新文件的大小 */
        char *newFile = malloc(newSize);  /* 创建待压缩的文件内存 */
        printf("%s file size:%lu\n", args[3], newSize);
        fread(newFile, 1, newSize, newFp);  /* 将待压缩的文件写入内存 */
        {
            /* 组织文件头 */
            header.ih_ep = newSize;  /* 要升级的新文件的大小 */
            header.ih_dcrc = BigtoLittle32(crc32(newFile, newSize));  /* 新文件CRC大端放入 */
        }
        fclose(newFp);

        /* 开始差分,创建新文件 */
        FILE *fp = fopen(args[4], "wb+");
        if (fp == NULL)
        {
            free(oldFile);
            free(newFile);
            return PrintError_WRes(rs, "Cannot open or creat file", -1);
        }

        struct bsdiff_stream bsStream;
        bsStream.opaque = fp;
        bsStream.malloc = MyAlloc;
        bsStream.free = MyFree;
        bsStream.write = bsdiff_write;
        res = bsdiff(oldFile, oldSize, newFile, newSize, &bsStream);
        free(oldFile);
        free(newFile);
        if (res)
        {
            printf("bsdiff failed, exit:%d\n", res);
            return  res;
        }
        long pachSize = fsize(fp);
        printf("%s pach file size:%lu\n", args[4], pachSize);
        fclose(fp);

        /* 差分后需要压缩 */
        if (lzmaFlag)
        {
            unsigned char bin_head[16 + 8];  /* strlen(BIN_HEAD)+要升级的文件长度 */
            const char *lzargs[4] = {NULL};
            lzargs[0] = NULL;
            lzargs[1] = "d";
            lzargs[2] = args[4];  /* pach文件名 */
            lzargs[3] = "bstemp.bin";
            res = lzmaProcess(sizeof(lzargs) / sizeof(char *), lzargs, rs);
            if (res != 0)
            {
                printf("lzmaProcess failed, exit:%d\n", res);
                return res;
            }

            newFp = fopen("bstemp.bin", "rb+");  /* 打开待插入的差分文件 */
            if (newFp == NULL)
            {
                return PrintError_WRes(rs, "Cannot open newPachFile file", -1);
            }
            pachSize = fsize(newFp);
            printf("pachSize: %ld\n", pachSize);

            /* 在压缩好的差分文件上添加文件头 */
            long headSize = sizeof(image_header_t) + sizeof(bin_head);
            char *newPachFile = malloc(pachSize + headSize); /* 创建待插入的文件内存 */
            /* 组织文件头 */
            header.ih_magic = IH_MAGIC;
            header.ih_size = BigtoLittle32(pachSize + sizeof(bin_head));
            header.ih_os = 0;
            header.ih_arch = 0;
            header.ih_type = 0;
            header.ih_comp = 0;
            header.ih_hcrc = 0;
            memset(header.ih_name, 0, IH_NMLEN);
            header.ih_hcrc = BigtoLittle32(crc32((uint8_t *)&header, sizeof(image_header_t)));
            /* 插入内存 */
            memcpy(newPachFile, (uint8_t *)&header, sizeof(image_header_t));

            /* 插入bin文件头 */
            memcpy(bin_head, "ENDSLEY/BSDIFF43", 16);
            offtout(header.ih_ep, bin_head + 16);
            memcpy(newPachFile + sizeof(image_header_t), bin_head, sizeof(bin_head));

            /* 处理待差分的新文件写到内存 */
            fread(newPachFile + headSize, 1, pachSize, newFp);
            fseek(newFp, 0, SEEK_SET);
            fwrite((void *)newPachFile, 1, pachSize + headSize, newFp);
            pachSize = fsize(newFp);
            fclose(newFp);
            free(newPachFile);
            
            remove(args[4]); /* 删除未压缩的文件 */
            rename("bstemp.bin", args[4]); /* 将临时文件重命名为用户指定的文件名 */
            printf("old_bin len:%u, crc:0X%08X\n", header.ih_load, BigtoLittle32(header.ih_ocrc));
            printf("new_bin len:%u, crc:0X%08X\n", header.ih_ep, BigtoLittle32(header.ih_dcrc));
            printf("patchsize+header = %d\n", BigtoLittle32(header.ih_size));
        }
    }
    else  /* 还原 */
    {
        printf("not support yet.");
    }
    return res;
}



int main(int numArgs, const char *args[])
{
    char rs[1000] = { 0 };
    int res;

    if (numArgs == 1)
    {
        PrintHelp(rs);
        fputs(rs, stdout);
        return 0;
    }

    if (args[1][0] == 'b' || args[1][0] == 'p')
        res = bsdiffProcess(numArgs, args, rs);
    else
        res = lzmaProcess(numArgs, args, rs);
    fputs(rs, stdout);
    return res;
}
