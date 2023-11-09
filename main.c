#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <assert.h>
#include <io.h>
#include <sys/stat.h>
#include "str_list.h"
#include "c_moc.h"
#include "conf-c/conf.h"

#define CONFIG_NAME "cmoc_conf.txt"

#define STR2(S) #S

/**
* @brief 查找目录下的对应扩展名的文件
* @param[in] path 需要查找的目录
* @param[in] extension 需要查找的文件扩展名
* @param[out] filelist 查找到的文件列表
*/
void search_file(const char* path, const char* extension, strlist_t *filelist)
{
    struct _finddata_t filefind;
    char* tail = "\\*.*";
    char* endstr;
    char final[1000];
    char no_shash_dir[1000];

    if ((path == NULL)||(strlen(path)==0)) {
        perror("path error:\n" __FILE__ " @ " STR2(__LINE__));
		exit(-1);
    }
    assert(extension!=NULL);
    assert(filelist!=NULL);

    strcpy(no_shash_dir,path);
    //去掉最后的//
    endstr = strrchr(no_shash_dir, '\\');
    if (endstr != NULL)
    {
        if (strcmp(endstr, "\\")==0)
        {
            *endstr = '\0';
        }
    }
    strcpy(final, no_shash_dir);
    strcat(final, tail);

    int done = 0, handle;
    if((handle = _findfirst(final, &filefind)) != -1)
    {
        while(!(done = _findnext(handle, &filefind)))
        {
            if(strcmp(filefind.name, "..") == 0)
                continue;
            if((_A_SUBDIR != filefind.attrib))              // 不是目录
            {
                endstr = strrchr(filefind.name, '.');
                if(endstr!=NULL && strcmpi(strrchr(filefind.name, '.'), extension)==0)  //指定文件的后缀
                {
                    char fileName[1000];

                    snprintf(fileName, sizeof fileName, "%s\\%s", no_shash_dir, filefind.name);
                    strlist_append_str(filelist, fileName, strlen(fileName));
                }
            }
        }
        _findclose(handle);
    }
}

/**
* @brief 递归查找目录下的各个子目录的路径名
* @param[in] path 需要查找的目录
* @param[out] sublist 查找到的子目录路径列表
*/
void search_dir_recursive(const char* path, strlist_t *sublist)
{
    struct _finddata_t filefind;
    char* tail = "\\*.*";
    char* endstr;
    char final[1000];
    char no_shash_dir[1000];

    if ((path == NULL)||(strlen(path)==0)) {
        perror("path error:\n" __FILE__ " @ " STR2(__LINE__));
		exit(-1);
    }
    assert(sublist!=NULL);

    strcpy(no_shash_dir,path);
    //去掉最后的//
    endstr = strrchr(no_shash_dir, '\\');
    if (endstr != NULL)
    {
        if (strcmp(endstr, "\\")==0)
        {
            *endstr = '\0';
        }
    }
    strcpy(final, no_shash_dir);
    strcat(final, tail);

    int done = 0, handle;
    if((handle = _findfirst(final, &filefind)) != -1)
    {
        while(!(done = _findnext(handle, &filefind)))
        {
            if(strcmp(filefind.name, "..") == 0)
                continue;
            if((_A_SUBDIR == filefind.attrib))              // 是目录
            {
                // printf("[Dir]:\t%s\n", filefind.name);
                char out[1000];
                strcpy(out,no_shash_dir);
                strcat(out,"\\");
                strcat(out,filefind.name);
                strlist_append_str(sublist, out, strlen(out));
                search_dir_recursive(out, sublist);// 递归遍历子目录
            }
        }
        _findclose(handle);
    }
}

/**
* @brief 替换路径中的'/'为'\'
* @param[in,out] path_list 需要替换斜杠的路径列表
*/
void replace_slash(strlist_t *path_list)
{
    strlist_t *pos;
    strlist_for_each_entry(pos, path_list)
    {
        for (int i; pos->str[i] != '\0'; ++i)
        {
            if (pos->str[i] == '/') {
                pos->str[i] = '\\';
            }
        }
    }
}

/**
* @brief 创建默认的配置文件
*/
void config_creat(void)
{
    CONF_CREATER *conf; //需要的数据结构

    //初始化数据结构
    //出错则返回NULL
    if((conf=conf_creater_new(CONFIG_NAME)) == NULL)
        return;

    //写配置
    //条二个参数为键
    //第三个参数为参数，这里注意格式
    //第四个参数为注释
    //成功时返回0，出错时返回错误代码
    //使用conf_error函数可打印错误代码的信息

    conf_insert(conf,NULL,NULL,"cmoc的配置文件\n\
                #注1：每个key后可填入多个键值；\n\
                #注2：键值间用','号连接，键值间不允许留有空格；\
                #注3：如果键值比如路径上有特殊符号，则必须用\"\"将这段键值包裹起来\n\
                #注4：最好使用相对于c_moc.exe的相对路径\n\
                #例如： process_path_recursive = .,\"../src folder\"\n");
    conf_insert(conf,NULL,NULL,"process_path：处理的路径");
    conf_insert(conf,"process_path_recursive",".","process_path_recursive：递归处理的路径");
    conf_insert(conf,NULL,NULL,"inlcude_path：预处理步骤中包含的路径（仅在有simplecpp存在的时候生效）");
    conf_insert(conf,"include_path_recursive",".","include_path_recursive：预处理步骤中递归包含的路径（仅在有simplecpp存在的时候生效）");

    //保存配置文件
    conf_save(conf);
    //释放内存
    conf_creater_free(conf);

    return;
}

/**
* @brief 备份当前配置文件，然后创建默认配置文件
*/
void config_backup_and_create(void)
{
    printf("backup the current configuration file as "CONFIG_NAME".bak\n");
    unlink(CONFIG_NAME".bak");
    rename(CONFIG_NAME, CONFIG_NAME".bak");
    config_creat();
}

/**
* @brief 获取配置文件中记录的对应配置
* @param[in] key 需要获取的键值
* @param[out] path_list 从中读取的对应配置的列表
* @return 是否有读取到数据
*/
bool config_get(const char *key, strlist_t *path_list)
{
    CONF *conf; //需要的数据结构
    CONF_VALUE *value; //键/值参数数据结构
    int code; //返回的错误代码
    int retval = false;

    assert(key!=NULL);
    assert(path_list!=NULL);

    conf_pos_msg_err(1);//允许报错
    while (1)
    {
        //打开并初始化数据结构
        //出错时返回NULL
        if((conf=conf_open(CONFIG_NAME)) == NULL) {
            printf(CONFIG_NAME"file does not exist, create a default configuration file\n");
            config_creat();
            continue;
        }
        //开始解析配置文件
        //成功时返回0
        //出错时返回错误代码
        //错误代码可使用conf_error函数打印错误信息
        if((code=conf_parse(conf)) != 0)
        {
            conf_error(code);
//            //释放内存 解析失败的时候已经释放了
//            conf_free(conf);
            printf("configuration file parse error, create a default configuration file\n");
            config_backup_and_create();//备份当前配置文件，然后创建默认配置文件
            continue;
        }
        //取出一个参数
        value=conf_value_get(conf,key);
        //如果有该参数时返回
        //否则返回一个NULL
        if ((value != NULL)&&(value->value[0]!=NULL))//最少有一个值
        {
            for (int i = 0; value->value[i]!=NULL; ++i)
            {
                printf("key %s%-2d=%s\n", key, i, value->value[i]);
                strlist_append_str(path_list,value->value[i], strlen(value->value[i]));
                retval = true;
            }
        }
        else {
            printf("key value %s does not exist\n", key);
        }
        break;
    }

//    //得到当前配置文件中键的个数
//    printf("key has %d\n",conf_count(conf));

    //释放内存
    conf_free(conf);

    return retval;
}

int main(int argc, char *argv[])
{
    (void)argc;

    strlist_t h_list;//头文件列表
    strlist_t c_list;//源文件列表
    strlist_t process_dir_list;//配置文件中读取的处理路径
    strlist_t include_list;//配置文件中读取的包含路径
    strlist_t read_list;
    strlist_t *pos;

    //设置当前工作路径为可执行文件所在的路径
    if (argv[0] != NULL)
    {
        struct stat s;

        if (stat(argv[0], &s) == 0)
        {
            //如果是文件路径则裁剪到上一级
            if (s.st_mode & S_IFREG)
            {
                char *p;
                if ((p=strrchr(argv[0], '/')) != NULL) {
                    *p = '\0';
                }
                else if ((p=strrchr(argv[0], '\\')) != NULL) {
                    *p = '\0';
                }
            }
        }

//        char cur_work_dir[1000];
//        getcwd(cur_work_dir, sizeof cur_work_dir);
//        printf("%s\n", argv[0]);
        chdir(argv[0]);
    }

    //初始化链表
    strlist_head_init(&h_list);
    strlist_head_init(&c_list);
    strlist_head_init(&process_dir_list);
    strlist_head_init(&include_list);
    strlist_head_init(&read_list);

    while (1)
    {
        //获取配置文件中记录的处理路径
        if (config_get("process_path_recursive",&read_list))
        {
            strlist_for_each_entry(pos,&read_list)
            {
                //提取循环包含路径下的子路径
                strlist_append_str(&process_dir_list, pos->str, strlen(pos->str));
                search_dir_recursive(pos->str, &process_dir_list);
            }
            strlist_destruct(&read_list);
        }
        config_get("process_path",&process_dir_list);
        if (list_empty(&process_dir_list.list))//必须要指定路径，否则使用默认配置文件
        {
            printf("process_path_recursive or process_path does not exist, create a default configuration file\n");
            config_backup_and_create();
            continue;
        }

        replace_slash(&process_dir_list);//替换路径中的'/'为'\'
        //根据配置文件中的路径获取源文件和头文件列表
        strlist_for_each_entry(pos, &process_dir_list)
        {
            search_file(pos->str, ".h", &h_list);
            search_file(pos->str, ".c", &c_list);
        }
        break;
    }

    //获取配置文件中记录的路径
    if (config_get("include_path_recursive",&read_list))
    {
        strlist_for_each_entry(pos,&read_list)
        {
            //提取循环包含路径下的子路径
            strlist_append_str(&include_list, pos->str, strlen(pos->str));
            search_dir_recursive(pos->str, &include_list);
        }
        strlist_destruct(&read_list);
    }
    config_get("include_path",&include_list);
    if (list_empty(&include_list.list))//没有指定路径则使用默认的路径"."
    {
        strlist_append_str(&include_list, ".", strlen("."));
    }

    replace_slash(&include_list);//替换路径中的'/'为'\'
    //设置include_path
    #define INCLUDE_PATH_SIZE 1024*1024
    char *include_path = malloc(INCLUDE_PATH_SIZE);
    int str_curr = 0;//当前写入的索引
    if (include_path == NULL)
    {
        perror("alloc fail");
		exit(-1);
    }
    include_path[0] = '\0';
    strlist_for_each_entry(pos, &include_list)
    {
        if (pos->str != NULL)
        {
            int numb = snprintf(&include_path[str_curr], INCLUDE_PATH_SIZE-str_curr, "-I%s ", pos->str);
            if (numb > 0) {
                str_curr += numb;
                if (str_curr >= INCLUDE_PATH_SIZE) {
                    break;
                }
            }
        }
    }

    //预处理：根据相应关键字，从头文件和源文件的列表中提取对应的函数列表
    strlist_for_each_entry(pos, &h_list)
    {
        if (cmoc_preprocess_file(pos->str, include_path)) {
            printf("%s\n", pos->str);
        }
    }
    strlist_for_each_entry(pos, &c_list)
    {
        if (cmoc_preprocess_file(pos->str, include_path)) {
            printf("%s\n", pos->str);
        }
    }
    free(include_path);
    strlist_destruct(&h_list);
    strlist_destruct(&c_list);
    strlist_destruct(&process_dir_list);
    strlist_destruct(&include_list);



    //将预处理提取的函数列表，生成对应的文件
    if (cmoc_check_need_save(NULL))
    {
        cmoc_generation_and_save(NULL);
    }

//    system("pause");
    return 0;
}
