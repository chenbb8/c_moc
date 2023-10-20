#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "str_list.h"

#define NAME_PARAM_MAX              1024
#define SIGNAL_SLOT_NUMB_MAX        1024

typedef struct {
    char name[NAME_PARAM_MAX];
    char param[NAME_PARAM_MAX];
} signal_slot_info_t;

signal_slot_info_t slot_info[SIGNAL_SLOT_NUMB_MAX];//预处理所记录的slot信息
signal_slot_info_t signal_info[SIGNAL_SLOT_NUMB_MAX];//预处理所记录的signal信息
int slot_numb = 0;//当前有多少slot信息
int signal_numb = 0;//当前有多少signal信息

#define LINE_MAX 1024*1024/*一行最多几个字节*/


/**
* @brief 初始化cmoc模块
*/
void cmoc_init(void)
{
    slot_numb = 0;
    signal_numb = 0;
}

typedef enum {
    AXTRACT_S_INIT,//起始
    AXTRACT_S_KEY,//找到关键字
    AXTRACT_S_NAME,//找到函数名
} extract_s_t;

void cmoc_extract_keyword(char *src, const char *key, signal_slot_info_t *info, int *numb)
{
    assert(src != NULL);
    assert(key != NULL);
    assert(info != NULL);
    assert(numb != NULL);

    extract_s_t ext_state = AXTRACT_S_INIT;
    int len = strlen(src);
    int info_len = strlen(key);
    int i;
    char *pread = src;

    if ((len == 0)||(info_len == 0)) {
        return;
    }

    for (i = 0; i < len; ++i)
    {
        switch (ext_state)
        {
            case AXTRACT_S_INIT:
                if ( (*pread != ' ') && (*pread != '\t') )
                {
                    if (memcmp(pread, key, info_len) == 0)
                    {
                        pread += info_len;
                        if ( (*pread == ' ') && (*pread == '\t') )//关键字后应该接空格或者tab
                        {
                            ext_state = AXTRACT_S_KEY;//已经找到关键字
                        }
                        else {
                            return;
                        }
                    }
                    else {
                        return;
                    }
                }
                else {
                        pread++;
                }
                break;
            case AXTRACT_S_KEY:
                if ( (*pread != ' ') && (*pread != '\t') )
                {
                    if (memcmp(pread, key, info_len) == 0)
                    {
                        ext_state = AXTRACT_S_KEY;//已经找到关键字
                        pread += info_len;
                    }
                    else {
                        pread++;
                    }
                }
                else {
                        pread++;
                }
                break;
            default:
                break;
        }
    }

}

/**
* @brief 预处理文件，提取出关键字
* @param[in] file_name 需要预处理的文件
* @return 是否有检测到相应关键字
*/
bool cmoc_preprocess_file(const char *file_name)
{
    char line_str[LINE_MAX];
    char *pstr;

    FILE* fp = fopen(file_name, "r");

    if (fp == NULL)
	{
		perror("fopen fail:");
		exit(-1);
	}

	while (fgets(line_str, LINE_MAX, fp) != NULL)
    {
//        pstr =
        printf("%s", line_str);
    }
	return true;
}

/**
* @brief 将预处理提取的关键字，生成对应的文件
* @param[in] save_dir 保存生成文件的路径
*/
void cmoc_generation_and_save(const char *save_dir);



