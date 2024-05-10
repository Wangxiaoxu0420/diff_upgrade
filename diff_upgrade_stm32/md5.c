#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <md5.h>

// T��32λ�֣�һ����64��Ԫ�أ���Ӧ64�ε�����Ҳ��Ϊ�ӷ�����
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

// 64�ε���������õ���ѭ����λ��sֵ
static const uint32_t S[64] = { 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
						 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
						 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
						 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };

// �������ߺ���
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

// MD5������
int MD5(uint32_t file_base, uint32_t filesize, uint8_t *result, BUFCOPY callback ) {
	uint8_t buffer[64];
	uint8_t  temp[512];
	size_t count = 0, offset, i; // count���ڼ�¼�ܳ��ȣ���λ��ʱ����Ҫ�õ�
	uint32_t X[16];
	int flag = 0;
	int len;

	// MD������CV�������ڻ���������
	uint32_t A, B, C, D;
	// ��ʼ����IV������С�˴洢��Intel x86ϵ��ԭ���Ͳ�����Little Endian��ʽ�洢��
	A = 0x67452301;
	B = 0xEFCDAB89;
	C = 0x98BADCFE;
	D = 0X10325476;

	//�������ļ�size����ʱѭ����д
	while(filesize)
	{
	
		memset(buffer, 0, sizeof(buffer));
		// fread�������ض�ȡ�Ĵ������趨ÿ�ζ�ȡһ���ַ����Ϳ���֪���ַ�������
		if(filesize > 64) 
		{
			callback((uint32_t)buffer, file_base+count, 64);
			len = 64;
		}
		else{
			callback((uint32_t)buffer, file_base+count, filesize);
			len = filesize;
		}
		// �����ļ��ܳ���

		count += len;
		//������һ�ζ�����filesize����64�ֽڣ�����Ҫ���в�λ������
		// ����ȡ�ļ���ĩβʱ����ζ����Ҫ���в�λ�����ˣ���ʱ������len���ܲ���512bit��Ҳ���ܸպõ���512bit
		if (filesize<=64) {
			flag = 1;

			// ��Ϊǡ�õ���448bit���У�����new_lenֱ�ӵ���len+1
			int new_len;
			for (new_len = len + 1; new_len % 64 != 56; new_len++)
				;

			memcpy(temp, buffer, len);

			// ���1000...0
			temp[len] = 0x80;
			for (offset = len + 1; offset < new_len; offset++)
				temp[offset] = 0;

			// ��ĩβ�ٸ����ܳ���count�ĵ�64λ�����������count��λ��byte������Ҫ����8
			int2byte(count * 8, temp + new_len);
			int2byte(count >> 29, temp + new_len + 4); //�ο����������룬count>>29�൱��count*8>>32�������Ա���ֵ���
			len = new_len;
		}

		// ��Ȼÿ��ֻ��ȡ512bit�����ǻ��ǲ��������ķ�ʽ�����Է�ֹ����һ�����ڲ�λ���¿��ܳ��ֵ� len > 512bit ���������ʱ��Ҫ�������ˣ�
		for (offset = 0; offset < len; offset += 64) {
			// ������βʱ�����ǰѲ�λ������ݴ�����temp�У�Ϊ�˴����ͳһ����temp�е����ݱ��浽buffer��
			if (flag == 1) {
				memcpy(buffer, temp + offset, 64);
			}

			// ����512λ��ÿ32λ���飬��X[k]ʱ���õ�
			for (int i = 0; i < 16; i++) {
				X[i] = byte2int(buffer + i * 4);
			}

			uint32_t a, b, c, d, temp, g, k;
			a = A;
			b = B;
			c = C;
			d = D;

			// ��ѭ���������֣�ÿ��16�ε�������64�ε���
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
		//���ƫ��64�ֽ�
		if(filesize <= 64) break;
		filesize -=64;
	
	}

	// ��128λ�����ս��ת��Ϊ�ֽ���ʽ
	int2byte(A, result);
	int2byte(B, result + 4);
	int2byte(C, result + 8);
	int2byte(D, result + 12);

	return 1;
}
