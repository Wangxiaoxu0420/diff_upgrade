#ifndef MD5_H
#define MD5_H

// MD5ѹ������4��ѭ����ʹ�õ����ɺ�����ÿ�ֲ�ͬ
#define F(b, c, d) (((b) & (c)) | ((~b) & (d)))
#define G(b, c, d) (((b) & (d)) | ((c) & (~d)))
#define H(b, c, d) ((b) ^ (c) ^ (d))
#define I(b, c, d) ((c) ^ ((b) | (~d)))

// ѭ������
#define LEFTROTATE(num, n) (((num) << n) | ((num >> (32 - n))))

//���ⲿ����buf�����ص��������������ڴ濽����Ҳ������flash������MD5ֵ���㲻Ӧ����buf��Դ��ֻ���Ŀ�����ַ�ͳ��ȣ����������MD5ֵ
typedef void (*BUFCOPY)(uint32_t dst_addr, uint32_t src_addr, uint32_t len);
int MD5(uint32_t file_base, uint32_t filesize, uint8_t *result, BUFCOPY callback );

#endif
