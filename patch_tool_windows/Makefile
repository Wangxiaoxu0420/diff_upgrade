#*************************************************************************
#  **
#  ** File         : Makefile
#  ** Abstract     : This is the introduction to the document
#  ** Author       : wr
#  ** mail         : 791314247@q.com
#  ** Created Time : 2020年11月22日 星期日 11时58分06秒
#  ** copyright    : COPYRIGHT(c) 2020
#  **
#  ************************************************************************/

ifneq ($(V),1)
Q := @
else
Q :=
endif

################################以下项目需用户根据需要更改##########################
# 输出文件的名称，默认为main
TARGET  := bsdiff

# 输出文件夹，.hex .bin .elf放在此文件夹下，.o .d文件放在此文件的子目录Obj下(自动创建)
BUILD   := build

# 支持双系统编译，故需选当前系统，0为linux，1为windows
SYS     := 0

# 是否将debug信息编译进.elf文件，默认打开
DEBUG   := 1

# 若指定了windows系统，则需确认编译器的路径，若安装时以默认路径安装，则正确
ifeq ($(SYS), 1)
GCC_PATH = /mnt/d/MinGW/bin
endif
###################################用户修改结束###################################

# 编译器定义
ifdef GCC_PATH
SUFFIX = .exe
CC     = $(GCC_PATH)/gcc$(SUFFIX)
else
CC     = gcc
endif

#################### CFLAGS Config Start ##########################
#搜索所有的h文件,并输出携带-I的.h文件路径
C_INCLUDES := $(addprefix -I,$(subst ./,,$(sort $(dir $(shell find ./ -type f -iname "*.h")))))
CFLAGS = $(C_INCLUDES)

#标准
CFLAGS += -std=c99

#当开启DEBUG功能时携带DEBUG参数
ifeq ($(DEBUG), 1)
CFLAGS += -g
endif

#自动生成依赖文件
CFLAGS += -MMD -MP -MF"$(@:.o=.d)"
#################### CFLAGS Config End ##########################

C_SRC   := $(subst ./,,$(shell find ./ -type f -iname "*.c"))
OBJECTS := $(addprefix $(BUILD)/Obj/, $(notdir $(C_SRC:%.c=%.o)))
vpath %.c $(sort $(dir $(C_SRC)))


.PHONY : all clean printf commit

all : $(BUILD)/$(TARGET)$(SUFFIX)

$(BUILD)/$(TARGET)$(SUFFIX) : $(OBJECTS)
	$(Q)$(CC) -o $@ $^
	$(Q)echo complete the all.
	$(Q)mv $@ ./

$(BUILD)/Obj/%.o : %.c Makefile | $(BUILD)/Obj
	$(Q)echo "buid $(subst ./,,$<)"
	$(Q)$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD)/Obj:
	$(Q)mkdir -p $(BUILD)/Obj

clean :
	$(RM) -r build

printf:
	$(Q)echo $(info $(CFLAGS))

commit:
	$(Q)git add .
	$(Q)status='$(shell git status | grep "git pull")';\
	if test -n "$$status";then echo "Need to do git pull !";exit 10;fi
	$(Q)explain='$(shell read -p "Please input git commit explain:" explain;echo "$$explain")';\
	if test -z "$$explain";then git commit -m "Daily development submission"; \
	else git commit -m "$$explain";fi
	$(Q)git push
	$(Q)git status

-include $(wildcard *.d)
