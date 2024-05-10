### 根目录下先编译出目标文件main
`make`

### LZMA压缩文件
`./main d test.bin destin.bin` 

test.bin为要压缩的文件，destin.bin为即将压缩后的文件，文件名可自己随意定义


### LZMA解压文件
`./main e destin.bin encode.bin`

destin.bin为要解压的文件，encode.bin为即将解压后的文件，文件名可自己随意定义

/** 2024.05.09 by wangxiaoxu */
命令说明：
lzma: <e|d> inputFile outputFile
  e: encode file
  d: decode file
bsdiff: <b|p> inputOldFile inputNewFile outputFile
  b: diff file
  p: pach file
run: bsdiff inputOldFile inputNewFile outputFile

当前只使用该命令，其他命令不太好用，pach不支持，e/d可以用，压缩文件的，bsdiff可以用，直接打包为差分文件的
bsdiff: <bsdiff> inputOldFile inputNewFile outputFile
打包文件，outputfile是输出的差分文件，可以直接使用

