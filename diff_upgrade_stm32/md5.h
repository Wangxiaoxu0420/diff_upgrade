#ifndef MD5_H
#define MD5_H

// MD5压缩函数4轮循环中使用的生成函数，每轮不同
#define F(b, c, d) (((b) & (c)) | ((~b) & (d)))
#define G(b, c, d) (((b) & (d)) | ((c) & (~d)))
#define H(b, c, d) ((b) ^ (c) ^ (d))
#define I(b, c, d) ((c) ^ ((b) | (~d)))

// 循环左移
#define LEFTROTATE(num, n) (((num) << n) | ((num >> (32 - n))))

//由外部传入buf拷贝回调函数，可以是内存拷贝，也可以是flash拷贝，MD5值计算不应关心buf来源，只关心拷贝地址和长度，输出计算后的MD5值
typedef void (*BUFCOPY)(uint32_t dst_addr, uint32_t src_addr, uint32_t len);
int MD5(uint32_t file_base, uint32_t filesize, uint8_t *result, BUFCOPY callback );

#endif
