#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include "str_list.h"

#define NAME_PARAM_MAX              1024
#define SIGNAL_SLOT_NUMB_MAX        1024

#if defined(__linux__)
#define NL "\n"
#else
#define NL "\r\n"
#endif

#define CMOC_VER "0.0.4"

enum {
    CMOC_LABEL_NONE = STRLIST_LABEL_USER,
    CMOC_LABEL_COMMA,//逗号
    CMOC_LABEL_FUNC_NAME,//函数名
    CMOC_LABEL_FUNC_PARAM_TYPE,//函数参数：类型
    CMOC_LABEL_FUNC_PARAM_NAME,//函数参数：参数名
    CMOC_LABEL_FUNC_PARAM_TYPE_NAME,//函数参数：类型+参数名
};

typedef struct {
    char name[NAME_PARAM_MAX];
    strlist_t param_list;
} signal_slot_info_t;

signal_slot_info_t slot_info[SIGNAL_SLOT_NUMB_MAX];//预处理所记录的slot信息
signal_slot_info_t signal_info[SIGNAL_SLOT_NUMB_MAX];//预处理所记录的signal信息
int slot_numb = 0;//当前有多少slot信息
int signal_numb = 0;//当前有多少signal信息

extern const char *signal_define;
extern const char *signal_declaration;
extern const char *slot_define;
extern const char *slot_declaration;

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

/**
* @brief 查找对应的字符串
* @param[in] src 需要被查找的字符串
* @param[in] keyword 需要匹配的关键字符串
* @return
*   @retval NULL 没有找到匹配的字符串
*   @retval 非NULL指针 在src中匹配到的字符串的地址
*/
static const char *cmoc_find_keyword(const char *src, const char *keyword)
{
    assert(src != NULL);
    assert(keyword != NULL);

    int len_src = strlen(src);
    int len_keyword = strlen(keyword);
    int i;
    const char *pread = src;

    if ((len_src == 0)||(len_keyword == 0)||(len_src < len_keyword))
    {
        return NULL;
    }
    for (i = 0; i < (len_src+1-len_keyword); ++i,++pread)
    {
        if (memcmp(pread, keyword, len_keyword) == 0)
        {
            return pread;
        }
    }
    return NULL;
}

/**
* @brief 在字符串开始的地方查找对应的字符串(忽略空格与TAB)
* @param[in] src 需要被查找的字符串
* @param[in] keyword 需要匹配的关键字符串
* @return
*   @retval NULL 没有找到匹配的字符串
*   @retval 非NULL指针 在src中匹配到的字符串的地址
*/
static const char *cmoc_find_keyword_first(const char *src, const char *keyword)
{
    assert(src != NULL);
    assert(keyword != NULL);

    int len_src = strlen(src);
    int len_keyword = strlen(keyword);
    int i;
    const char *pread = src;

    if ((len_src == 0)||(len_keyword == 0)||(len_src < len_keyword))
    {
        return NULL;
    }
    for (i = 0; i < (len_src+1-len_keyword); ++i,++pread)
    {
        if ( (*pread != ' ') && (*pread != '\t') )//首个非空格与tab的字符串是否为keyword
        {
            if (memcmp(pread, keyword, len_keyword) == 0)
            {
                return pread;
            }
            return NULL;
        }
    }
    return NULL;
}

/**
* @brief 将参数字符串只保留类型，并转换成参数链表
* @param[out] param_list 转换出来的参数链表
* @param[in] param_str 需要转换的参数字符串
* @param[in] size 参数字符串的大小
* @return 一共有几个参数 0代表void
*/
static int cmoc_param_str_2_list(strlist_t *param_list, const char *param_str, int size)
{
    strlist_t split;
    strlist_t *pos;
    const char *comma;//逗号的指针
    int numb = 0;

    strlist_head_init(&split);

    if (size == 0)
    {
        return 0;
    }
    //将参数转换成split链表以方便后面的处理
    strlist_append_str(&split, param_str, size);
    //循环获取最后一个节点，并以搜索到的第一个','符为界，分割链表
    while (pos = strlist_last(&split), comma = cmoc_find_keyword(pos->str, ","), comma!=NULL)
    {
        strlist_split_point(pos, comma, 1, CMOC_LABEL_COMMA);
    }
    //遍历链表
    strlist_for_each_entry(pos, &split)
    {
        if (pos->label != CMOC_LABEL_COMMA)
        {
            int len = strlen(pos->str);
            char curr_param[200];//应该不会有超过200字节的类型名字
            int write_idx = 0;
            int read_idx = 0;
            char *read_start = pos->str;//读取的起始指针


            if (len == 0) {
                continue;
            }
            //删除前面的空格
            for (read_idx = 0; read_idx < len; ++read_idx)
            {
                if (!((read_start[read_idx] == ' ')||(read_start[read_idx] == '\t')))
                {
                    break;//碰到了第一个不为空格或tab的字符
                }
            }
            len -= read_idx;
            if (len == 0) {
                continue;
            }
            read_start += read_idx;
            //先判断是否为类似"struct type * name"这样有"struct"的格式
            if (memcmp(read_start, "struct", strlen("struct")) == 0)
            {
                char ch = read_start[strlen("struct")];
                if ( (ch == ' ')||(ch == '\t') )
                {
                    strcpy(&curr_param[write_idx], "struct ");
                    len -= strlen("struct ");
                    if (len == 0) {
                        continue;
                    }
                    write_idx += strlen("struct ");
                    read_start += strlen("struct ");

                    //再次删除前面的空格
                    for (read_idx = 0; read_idx < len; ++read_idx)
                    {
                        if (!((read_start[read_idx] == ' ')||(read_start[read_idx] == '\t')))
                        {
                            break;//碰到了第一个不为空格或tab的字符
                        }
                    }
                    len -= read_idx;
                    if (len == 0) {
                        continue;
                    }
                    read_start += read_idx;
                }
            }
            //取得类型名 从这开始就要开始就要写入到param_list了
            for (read_idx = 0; read_idx < len; ++read_idx)
            {
                if ((read_start[read_idx] == ' ')||(read_start[read_idx] == '\t'))
                {
                    break;
                }
            }
            memcpy(&curr_param[write_idx], read_start, read_idx);
            curr_param[write_idx+read_idx] = '\0';
            len -= read_idx;
            if (len == 0) {
                if (strcmp(curr_param, "void") != 0) {//void类型不记录
                    strlist_append_str(param_list, curr_param, strlen(curr_param));
                    numb++;
                }
                continue;
            }
            write_idx += read_idx;
            read_start += read_idx;
            //查找是否还有‘*’符
            for (read_idx = 0; read_idx < len; ++read_idx)
            {
                if (!((read_start[read_idx] == ' ')||(read_start[read_idx] == '\t')))
                {
                    if (read_start[read_idx] == '*') {
                        curr_param[write_idx++] = '*';
                    }
                    else {
                        break;
                    }
                }
            }
            curr_param[write_idx] = '\0';
            if (strcmp(curr_param, "void") != 0) {//void类型不记录
                strlist_append_str(param_list, curr_param, strlen(curr_param));
                numb++;
            }
        }
    }
    strlist_destruct(&split);//释放split
    return numb;
}

/**
* @brief 将参数链表转换成参数字符串,包含类型和参数名
* @param[out] param_str 转换出来的参数字符串
* @param[in] size_str 参数字符串所在缓冲区的大小
* @param[in] param_list 需要转换的参数链表
* @return param_str的起始地址
*/
static char* cmoc_param_list_2_str(char *param_str, int size_str, strlist_t *param_list)
{
    uint8_t idx = 0;
    strlist_t *pos;
    assert(size_str > 1);

    if (list_empty(&param_list->list))
    {
        strcpy(param_str, "void");
    }
    else
    {
        param_str[0] = '\0';
        size_str -= 1;//最后的'\0'不存放东西
        strlist_for_each_entry(pos, param_list)
        {
            int len = strlen(param_str);
            if (len >= size_str) {
                break;
            }
            if (list_is_first(&pos->list, &param_list->list))
            {
                snprintf(param_str+len, size_str-len, "%s p%d", pos->str, idx);
            }
            else
            {
                snprintf(param_str+len, size_str-len, ", %s p%d", pos->str, idx);
            }
            idx++;
        }
    }
    return param_str;
}

/**
* @brief 将参数链表转换成参数字符串,只包含参数名
* @param[out] param_str 转换出来的参数字符串
* @param[in] size_str 参数字符串所在缓冲区的大小
* @param[in] param_list 需要转换的参数链表
* @return param_str的起始地址
*/
static char* cmoc_param_list_2_str_no_type(char *param_str, int size_str, strlist_t *param_list)
{
    uint8_t idx = 0;
    strlist_t *pos;
    assert(size_str > 1);

    param_str[0] = '\0';
    size_str -= 1;//最后的'\0'不存放东西
    strlist_for_each_entry(pos, param_list)
    {
        int len = strlen(param_str);
        if (len >= size_str) {
            break;
        }
//        if (list_is_first(&pos->list, &param_list->list))
//        {
//            snprintf(param_str+len, size_str-len, "p%d", idx);
//        }
//        else
        {
            snprintf(param_str+len, size_str-len, ", p%d", idx);
        }
        idx++;
    }
    return param_str;
}

/**
* @brief 提取关键字对应的函数
* @param[in] src 需要被处理的字符串
* @param[in] keyword 需要匹配的关键字符串
* @param[out] info 存放提取出来的函数info
* @param[in,out] numb 当前有效的info数量
* @return 是否有对应函数
*   @retval true 找到对应函数
*   @retval false 没找到对应函数
*/
static bool cmoc_extract_keyword(const char *src, const char *keyword, signal_slot_info_t *info, int *numb)
{
    assert(src != NULL);
    assert(keyword != NULL);
    assert(info != NULL);
    assert(numb != NULL);

    int len = strlen(src);
    int key_len = strlen(keyword);

    if ((len == 0)||(key_len == 0)) {
        return false;
    }

    //查找关键字signal或者slot
    const char *pkey = cmoc_find_keyword_first(src, keyword);//在字符串开始的地方查找对应的字符串(忽略空格与TAB)
    if (pkey == NULL) {
        return false;
    }
    //关键字后应该是空格或者tab
    if ( !((pkey[key_len] == ' ')||(pkey[key_len] == '\t')) )
    {
        return false;
    }

    //查找'('
    const char *left = cmoc_find_keyword(pkey, "(");
    if (left == NULL) {
        return false;
    }

    //查找')'
    const char *right = cmoc_find_keyword(left, ")");
    if (right == NULL) {
        return false;
    }

    //截取函数名
    const char *name_read = left-1;
    while ((*name_read==' ')||(*name_read=='\t'))//首先忽略'('前的空格
    {
        if (--name_read <= pkey+key_len)
        {
            return false;//从pkey到'('之间全是空格
        }
    }
    const char *name_end = name_read;//指向函数名的最后一个字符
    while (1)
    {
        if ((*name_read==' ')||(*name_read=='\t'))//函数名前应该是空格
        {
            break;
        }
        if (--name_read < pkey+key_len)//超出范围
        {
            return false;
        }
    }
    memcpy(info[*numb].name, name_read+1, name_end-name_read);
    info[*numb].name[name_end-name_read] = '\0';

    //截取参数
    strlist_head_init(&info[*numb].param_list);
    if (right-left != 1)
    {
        cmoc_param_str_2_list(&info[*numb].param_list, left+1, right-left-1);
    }
    (*numb)++;
    return true;
}

/**
* @brief 预处理文件，提取出关键字
* @param[in] file_name 需要预处理的文件
* @param[in] include_path 查找头文件的路径
* @return 是否有检测到相应关键字
*/
bool cmoc_preprocess_file(const char *file_name, const char *include_path)
{
    char *line_str;
    bool ret = false;
    bool is_popen = true;
    FILE* fp;

    //有simplecpp.exe则提前预处理，没有则直接读取
    if (access("./simplecpp.exe", F_OK) != 0)
    {
        fp = fopen(file_name, "rb");/*注意需要以rb+或者wb+的方式打开文件，因为在windows环境下，不带b的版本可能会在读写文件的时候，遇到\n自动增减一个/r*/

        if (fp == NULL)
        {
            perror("fopen fail:");
            return 0;
        }
        is_popen = false;
    }
    else
    {
        #define INCLUDE_PATH_SIZE (1024*1024+100)
        char *include_str = malloc(INCLUDE_PATH_SIZE);
        if (include_str == NULL) {
            perror("malloc fail:");
            exit(-1);
        }
        if (include_path != NULL)
        {
            snprintf(include_str, INCLUDE_PATH_SIZE, ".\\simplecpp.exe %s %s", include_path, file_name);
        }
        else
        {
            snprintf(include_str, INCLUDE_PATH_SIZE, ".\\simplecpp.exe %s", file_name);
        }
        fp = popen(include_str, "r");
        free(include_str);

        if (fp == NULL)
        {
            perror("popen fail:");
            return 0;
        }
    }

    line_str = malloc(LINE_MAX);
    if (line_str == NULL) {
        perror("malloc fail:");
		exit(-1);
    }

	while (fgets(line_str, LINE_MAX, fp) != NULL)
    {
//        pstr =
//        printf("%s", line_str);
        if (cmoc_extract_keyword(line_str, "slot", slot_info, &slot_numb)) {
            ret = true;
        }
        if (cmoc_extract_keyword(line_str, "signal", signal_info, &signal_numb)) {
            ret = true;
        }
    }
    free(line_str);

    if (is_popen)
    {
        pclose(fp);
    }
    else
    {
        fclose(fp);
    }

    //TEST:打印检索出来的signal和slot
//    int i;
//    char param_str[NAME_PARAM_MAX];
//    for (i = 0; i < slot_numb; i++)
//    {
//        printf("slot %2d: void %s(", i, slot_info[i].name);
//        printf("%s", cmoc_param_list_2_str(param_str, NAME_PARAM_MAX, &slot_info[i].param_list));
//        printf(")\n");
//    }
//
//    for (i = 0; i < signal_numb; i++)
//    {
//        printf("signal %2d: void %s(", i, signal_info[i].name);
//        printf("%s", cmoc_param_list_2_str(param_str, NAME_PARAM_MAX, &signal_info[i].param_list));
//        printf(")\n");
//    }

	return ret;
}

/**
* @brief 用函数info生成字符链表
* @param[out] out_list 转换成出来的字符链表
* @param[in] info 需要转换的函数info
* @param[in] template_str 转换用到的模板
*/
static void cmoc_info_2_strlist(strlist_t *out_list, const signal_slot_info_t* info, const char* template_str)
{
    assert(out_list != NULL);
    assert(info != NULL);
    assert(template_str != NULL);

    strlist_t *pos;
    int size = strlen(template_str);
    const char *keyword;
    char param_str[NAME_PARAM_MAX];

    //将模板转换成out_list链表以方便后面的处理
    strlist_append_str(out_list, template_str, size);
    //标记name
    strlist_for_each_entry_fix(pos, out_list)
    {
        keyword = cmoc_find_keyword(pos->str, "$NAME");
        if (keyword!=NULL)
        {
            strlist_split_point(pos, keyword, sizeof("$NAME")-1, CMOC_LABEL_FUNC_NAME);
        }
    }
    //标记 参数类型-参数名
    strlist_for_each_entry_fix(pos, out_list)
    {
        keyword = cmoc_find_keyword(pos->str, "$PARAM_TYPE_NAME");
        if (keyword!=NULL)
        {
            strlist_split_point(pos, keyword, sizeof("$PARAM_TYPE_NAME")-1, CMOC_LABEL_FUNC_PARAM_TYPE_NAME);
        }
    }
    //标记 参数名
    strlist_for_each_entry_fix(pos, out_list)
    {
        keyword = cmoc_find_keyword(pos->str, "$PARAM_NAME");
        if (keyword!=NULL)
        {
            strlist_split_point(pos, keyword, sizeof("$PARAM_NAME")-1, CMOC_LABEL_FUNC_PARAM_NAME);
        }
    }
    //替换name
    strlist_replace_keyword(out_list, CMOC_LABEL_FUNC_NAME, info->name);
    //替换 参数类型-参数名
    strlist_replace_keyword(out_list, CMOC_LABEL_FUNC_PARAM_TYPE_NAME, cmoc_param_list_2_str(param_str, NAME_PARAM_MAX, (void*)&info->param_list));
    //替换 参数名
    strlist_replace_keyword(out_list, CMOC_LABEL_FUNC_PARAM_NAME, cmoc_param_list_2_str_no_type(param_str, NAME_PARAM_MAX, (void*)&info->param_list));
}

/**
* @brief 合并路径和文件名
* @param[out] target_str 保存生成文件的路径+文件名的缓冲区
* @param[in] target_size_max 输出缓冲区的大小
* @param[in] dir 文件的路径
* @param[in] file_name 文件名
* @return 合并好的路径和文件名字符串
*/
static char *cmoc_path_name(char *target_str, int target_size_max, const char *dir, const char *file_name)
{
    assert(target_str!=NULL);
    assert(file_name!=NULL);
    assert(strlen(file_name) < 50);

    int dir_len = 0;

    if ((dir != NULL) && (dir_len = strlen(dir),(dir_len < target_size_max-50)))
    {
        strcpy(target_str, dir);
        if (target_str[dir_len-1]=='\\'||target_str[dir_len-1]=='/')
        {
            strcpy(&target_str[dir_len], file_name);
        }
        else
        {
            strcpy(&target_str[dir_len-1], file_name);
        }
    }
    else{
        strcpy(target_str, file_name);
    }
    return target_str;
}

/**
* @brief 在文件中标记当前的信息
* @param[in] pf 需要标记的文件的句柄
*/
static void cmoc_info_mask(FILE* pf)
{
    const char *cstr;
    char mask_per[NAME_PARAM_MAX];
    char param_str[NAME_PARAM_MAX];

    cstr = "/*"NL"INFO_MASK_START,CMOC_VER="CMOC_VER NL;
    fwrite(cstr, 1, strlen(cstr), pf);
    for (int i = 0; i < signal_numb; ++i)
    {
        snprintf(mask_per, NAME_PARAM_MAX, "signal:%s(%s)"NL, signal_info[i].name, cmoc_param_list_2_str(param_str, NAME_PARAM_MAX, &signal_info[i].param_list));
        fwrite(mask_per, 1, strlen(mask_per), pf);
    }
    for (int i = 0; i < slot_numb; ++i)
    {
        snprintf(mask_per, NAME_PARAM_MAX, "slot:%s(%s)"NL, slot_info[i].name, cmoc_param_list_2_str(param_str, NAME_PARAM_MAX, &slot_info[i].param_list));
        fwrite(mask_per, 1, strlen(mask_per), pf);
    }
    cstr = "INFO_MASK_START"NL"*/"NL NL NL;
    fwrite(cstr, 1, strlen(cstr), pf);
}

/**
* @brief 判断是否需要生成文件
* @param[in] save_dir 保存生成文件的路径
* @return 是否需要生成文件
*/
bool cmoc_check_need_save(const char *save_dir)
{
    char target_name[1024];
    const char *cstr;
    char mask_per[NAME_PARAM_MAX];
    char param_str[NAME_PARAM_MAX];
    FILE* pf;
    int match_slot = 0;
    int match_signal = 0;
    int unmatch_numb = 0;
    int i;

    cmoc_path_name(target_name, sizeof target_name, save_dir, "signal_slot_dec.h");
    if (access(target_name, F_OK) != 0)
    {
        return true;
    }
    cmoc_path_name(target_name, sizeof target_name, save_dir, "signal_slot_def.h");
    if (access(target_name, F_OK) != 0)
    {
        return true;
    }
    pf = fopen(target_name, "r");
    if (pf == NULL)
	{
		perror("fopen fail:");
		exit(-1);
	}

    char *line_str;
    bool isMaskStart = false;


    line_str = malloc(LINE_MAX);
    if (line_str == NULL) {
        perror("malloc fail:");
		exit(-1);
    }
    while (fgets(line_str, LINE_MAX, pf) != NULL)
    {
        if (!isMaskStart)
        {
            cstr = "INFO_MASK_START,CMOC_VER="CMOC_VER;
            if (memcmp(line_str, cstr, strlen(cstr)) == 0) {
                isMaskStart = true;
            }
            continue;
        }

        cstr = "INFO_MASK_END";
        if (memcmp(line_str, cstr, strlen(cstr)) == 0) {
            break;
        }
        //匹配signal
        cstr = "signal:";
        if (memcmp(line_str, cstr, strlen(cstr)) == 0)
        {
            for (i = 0; i < signal_numb; ++i)
            {
                snprintf(mask_per, NAME_PARAM_MAX, "signal:%s(%s)", signal_info[i].name, cmoc_param_list_2_str(param_str, NAME_PARAM_MAX, &signal_info[i].param_list));
                if (memcmp(line_str, mask_per, strlen(mask_per)) == 0)
                {
                    match_signal++;
                    break;
                }
            }
            if (i >= signal_numb) {//没匹配的
                unmatch_numb++;
            }
            continue;
        }
        //匹配slot
        cstr = "slot:";
        if (memcmp(line_str, cstr, strlen(cstr)) == 0)
        {
            for (i = 0; i < slot_numb; ++i)
            {
                snprintf(mask_per, NAME_PARAM_MAX, "slot:%s(%s)", slot_info[i].name, cmoc_param_list_2_str(param_str, NAME_PARAM_MAX, &slot_info[i].param_list));
                if (memcmp(line_str, mask_per, strlen(mask_per)) == 0)
                {
                    match_slot++;
                    break;
                }
            }
            if (i >= slot_numb) {//没匹配的
                unmatch_numb++;
            }
            continue;
        }
    }
    free(line_str);

    if ((match_signal==signal_numb)&&(match_slot==slot_numb)&&(unmatch_numb==0))
    {
        return false;
    }
    return true;
}

/**
* @brief 将预处理提取的函数列表，生成对应的文件
* @param[in] save_dir 保存生成文件的路径
*/
void cmoc_generation_and_save(const char *save_dir)
{
    int i;
    char target_name[1024];
    const char *cstr;
    FILE* pf;

//    printf("%s", signal_define);

    strlist_t *pos;
    strlist_t gen_list;

    //保存定义文件
    cmoc_path_name(target_name, sizeof target_name, save_dir, "signal_slot_def.h");
    pf = fopen(target_name, "wb+");
    if (pf == NULL)
	{
		perror("fopen fail:");
		exit(-1);
	}

    cstr = "#ifndef _SIGNAL_SLOT_DEF_H_"NL"#define _SIGNAL_SLOT_DEF_H_"NL NL"#include \"signal_slot.h\""NL NL NL;
    fwrite(cstr, 1, strlen(cstr), pf);

    cmoc_info_mask(pf);//在文件中标记当前的信息

    strlist_head_init(&gen_list);

    for (i = 0; i < signal_numb; i++)
    {
        cmoc_info_2_strlist(&gen_list, &signal_info[i], signal_define);
        strlist_for_each_entry(pos, &gen_list)
        {
            if (pos->str != NULL) {
                fwrite(pos->str, 1, strlen(pos->str), pf);
                //printf("%s", pos->str);
            }
        }
        strlist_destruct(&gen_list);
    }

    for (i = 0; i < slot_numb; i++)
    {
        cmoc_info_2_strlist(&gen_list, &slot_info[i], slot_define);
        strlist_for_each_entry(pos, &gen_list)
        {
            if (pos->str != NULL) {
                fwrite(pos->str, 1, strlen(pos->str), pf);
                //printf("%s", pos->str);
            }
        }
        strlist_destruct(&gen_list);
    }

    cstr = "#endif"NL;
    fwrite(cstr, 1, strlen(cstr), pf);
    fflush(pf);
    fclose(pf);

    //保存声明文件
    cmoc_path_name(target_name, sizeof target_name, save_dir, "signal_slot_dec.h");
    pf = fopen(target_name, "wb+");
    if (pf == NULL)
	{
		perror("fopen fail:");
		exit(-1);
	}

    cstr = "#ifndef _SIGNAL_SLOT_DEC_H_"NL"#define _SIGNAL_SLOT_DEC_H_"NL NL NL;
    fwrite(cstr, 1, strlen(cstr), pf);

    strlist_head_init(&gen_list);

    for (i = 0; i < slot_numb; i++)
    {
        cmoc_info_2_strlist(&gen_list, &slot_info[i], slot_declaration);
        strlist_for_each_entry(pos, &gen_list)
        {
            if (pos->str != NULL) {
                fwrite(pos->str, 1, strlen(pos->str), pf);
                //printf("%s", pos->str);
            }
        }
        strlist_destruct(&gen_list);
    }

    for (i = 0; i < signal_numb; i++)
    {
        cmoc_info_2_strlist(&gen_list, &signal_info[i], signal_declaration);
        strlist_for_each_entry(pos, &gen_list)
        {
            if (pos->str != NULL) {
                fwrite(pos->str, 1, strlen(pos->str), pf);
                //printf("%s", pos->str);
            }
        }
        strlist_destruct(&gen_list);
    }
    cstr = "#endif"NL;
    fwrite(cstr, 1, strlen(cstr), pf);
    fflush(pf);
    fclose(pf);
}

const char *signal_define = \
"/*define signal: void $NAME($PARAM_TYPE_NAME)*/"NL\
"typedef void (*$NAME_t)($PARAM_TYPE_NAME);"NL\
"void $NAME($PARAM_TYPE_NAME);"NL\
"struct _signal_slot_node $NAME_head = {"NL\
"    .self_node = USER_LIST_HEAD_INIT($NAME_head.self_node),"NL\
"    .signal_node = USER_LIST_HEAD_INIT($NAME_head.signal_node),"NL\
"    .func = $NAME,"NL\
"    .signals = NULL,"NL\
"};"NL\
"void $NAME($PARAM_TYPE_NAME)"NL\
"{"NL\
"    SIGNAL_SEND($NAME$PARAM_NAME);"NL\
"}" NL NL NL;


const char *signal_declaration = \
"/*declaration signal: void $NAME($PARAM_TYPE_NAME)*/"NL\
"void $NAME($PARAM_TYPE_NAME);"NL\
"extern struct _signal_slot_node $NAME_head;"NL NL NL;

const char *slot_define = \
"/*define slot: void $NAME($PARAM_TYPE_NAME)*/"NL\
"void $NAME($PARAM_TYPE_NAME);"NL\
"struct _signal_slot_node $NAME_head = { "NL\
"    .self_node = USER_LIST_HEAD_INIT($NAME_head.self_node), "NL\
"    .signal_node = USER_LIST_HEAD_INIT($NAME_head.signal_node), "NL\
"    .func = $NAME, "NL\
"    .signals = NULL, "NL\
"};"NL NL NL;

const char *slot_declaration = \
"/*declaration slot: void $NAME($PARAM_TYPE_NAME)*/"NL\
"void $NAME($PARAM_TYPE_NAME);"NL\
"extern struct _signal_slot_node $NAME_head;"NL NL NL;

