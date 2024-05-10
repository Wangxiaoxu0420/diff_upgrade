#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <md5.h>

// T表，32位字，一共有64个元素，对应64次迭代，也成为加法常数
static const uint32_t T[64] = { 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
						0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
						0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
						0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
						0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
						0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
						0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
						0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
						0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
						0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
						0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
						0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
						0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
						0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
						0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
						0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

// 64次迭代运算采用的左循环移位的s值
static const uint32_t S[64] = { 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
						 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
						 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
						 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };

// 两个工具函数
static void int2byte(uint32_t val, uint8_t *bytes)
{
	bytes[0] = (uint8_t)val;
	bytes[1] = (uint8_t)(val >> 8);
	bytes[2] = (uint8_t)(val >> 16);
	bytes[3] = (uint8_t)(val >> 24);
}

static uint32_t byte2int(const uint8_t *bytes)
{
	return (uint32_t)bytes[0]
		| ((uint32_t)bytes[1] << 8)
		| ((uint32_t)bytes[2] << 16)
		| ((uint32_t)bytes[3] << 24);
}

// MD5主函数
int MD5(uint32_t file_base, uint32_t filesize, uint8_t *result, BUFCOPY callback ) {
	uint8_t buffer[64];
	uint8_t  temp[512];
	size_t count = 0, offset, i; // count用于记录总长度，补位的时候需要用到
	uint32_t X[16];
	int flag = 0;
	int len;

	// MD缓冲区CV，迭代在缓冲区进行
	uint32_t A, B, C, D;
	// 初始向量IV，采用小端存储（Intel x86系列原本就采用了Little Endian方式存储）
	A = 0x67452301;
	B = 0xEFCDAB89;
	C = 0x98BADCFE;
	D = 0X10325476;

	//当升级文件size还有时循环读写
	while(filesize)
	{
	
		memset(buffer, 0, sizeof(buffer));
		// fread函数返回读取的次数，设定每次读取一个字符，就可以知道字符长度了
		if(filesize > 64) 
		{
			callback((uint32_t)buffer, file_base+count, 64);
			len = 64;
		}
		else{
			callback((uint32_t)buffer, file_base+count, filesize);
			len = filesize;
		}
		// 更新文件总长度

		count += len;
		//如果最后一次读到的filesize不足64字节，则需要进行补位操作了
		// 当读取文件到末尾时，意味着需要进行补位操作了，此时读到的len可能不足512bit，也可能刚好等于512bit
		if (filesize<=64) {
			flag = 1;

			// 因为恰好等于448bit不行，所以new_len直接等于len+1
			int new_len;
			for (new_len = len + 1; new_len % 64 != 56; new_len++)
				;

			memcpy(temp, buffer, len);

			// 填充1000...0
			temp[len] = 0x80;
			for (offset = len + 1; offset < new_len; offset++)
				temp[offset] = 0;

			// 在末尾再附加总长度count的低64位，由于这里的count单位是byte，所以要乘以8
			int2byte(count * 8, temp + new_len);
			int2byte(count >> 29, temp + new_len + 4); //参考了其他代码，count>>29相当于count*8>>32，但可以避免值溢出
			len = new_len;
		}

		// 虽然每次只读取512bit，但是还是采用这样的方式，可以防止最后的一次由于补位导致可能出现的 len > 512bit 的情况（此时就要分两次了）
		for (offset = 0; offset < len; offset += 64) {
			// 读到结尾时，我们把补位后的数据存在了temp中，为了处理的统一，将temp中的数据保存到buffer上
			if (flag == 1) {
				memcpy(buffer, temp + offset, 64);
			}

			// 保存512位的每32位分组，在X[k]时会用到
			for (int i = 0; i < 16; i++) {
				X[i] = byte2int(buffer + i * 4);
			}

			uint32_t a, b, c, d, temp, g, k;
			a = A;
			b = B;
			c = C;
			d = D;

			// 主循环，共四轮，每轮16次迭代，共64次迭代
			for (i = 0; i < 64; i++) {
				if (i < 16) {
					g = F(b, c, d);
					k = i;
				}
				else if (i < 32) {
					g = G(b, c, d);
					k = (1 + 5 * i) % 16;
				}
				else if (i < 48) {
					g = H(b, c, d);
					k = (5 + 3 * i) % 16;
				}
				else {
					g = I(b, c, d);
					k = (7 * i) % 16;
				}
				temp = d;
				d = c;
				c = b;
				b = b + LEFTROTATE((a + g + X[k] + T[i]), S[i]);
				a = temp;
			}

			A += a;
			B += b;
			C += c;
			D += d;

		}
		//向后偏移64字节
		if(filesize <= 64) break;
		filesize -=64;
	
	}

	// 把128位的最终结果转化为字节形式
	int2byte(A, result);
	int2byte(B, result + 4);
	int2byte(C, result + 8);
	int2byte(D, result + 12);

	return 1;
}
