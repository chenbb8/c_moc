#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <assert.h>
#include <io.h>
#include "str_list.h"
#include "c_moc.h"

enum {
    STRLIST_LABEL_AA = STRLIST_LABEL_USER,
    STRLIST_LABEL_BB
};

char text[] = "123456789abcdefg";
strlist_t head = {
    .list = LIST_HEAD_INIT(head.list),
    .label = STRLIST_LABEL_NONE,
    .str = NULL
};


/*
    功能：检索文件夹(包含子文件夹)下所有的指定后缀名的文件
    Author：GA
*/
void filesearch(char* path, const char* extension, strlist_t *filelist)
{
    struct _finddata_t filefind;
    char* tail = "\\*.*";
    char* endstr;
    char final[1000];
    char no_shash_dir[1000];

    assert(path!=NULL);
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

    int done = 0, i, handle;
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
                filesearch(out, extension, filelist);       // 递归遍历子目录
            }
            else
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

int main()
{
    strlist_t h_list;
    strlist_t c_list;

    strlist_head_init(&h_list);
    strlist_head_init(&c_list);

//    char path[] = "F:\\project\\280\\ZW280\\ZW280_c_single_slot\\Projects";   //指定检索目录
    char path[] = "F:\\project\\280\\ZW280\\ZW280_c_single_slot";   //指定检索目录
    filesearch(path, ".h", &h_list);
    filesearch(path, ".c", &c_list);

    strlist_t* pos;
    strlist_for_each_entry(pos, &h_list)
    {
        if (cmoc_preprocess_file(pos->str)) {
            printf("%s\n", pos->str);
        }
    }
    strlist_for_each_entry(pos, &c_list)
    {
        if (cmoc_preprocess_file(pos->str)) {
            printf("%s\n", pos->str);
        }
    }

//    cmoc_preprocess_file("F:\\project\\280\\ZW280\\ZW280_c_single_slot\\Projects\\USER\\function.c");
    //cmoc_preprocess_file("test_sig.c");
    cmoc_generation_and_save(NULL);
    return 0;
}

int main1()
{
    int len = strlen(text);
    strlist_append_str(&head, text, len);

    strlist_t *pos;
    pos = strlist_last(&head);
//    strlist_for_each_entry(pos, &head)
    {
        printf("%s\n", pos->str);
    }

    pos = strlist_fist(&head);
    strlist_split_point(pos, &pos->str[0], 4, STRLIST_LABEL_AA);

    strlist_for_each_entry(pos, &head)
    {
        printf("%s\n", pos->str);
    }

    int total_len = strlist_total_slen(&head);
    char *idx = NULL, *total_str = NULL;
    total_str = malloc(total_len+1);
    idx = total_str;

    strlist_for_each_entry(pos, &head)
    {
        len = strlen(pos->str);
        memcpy(idx, pos->str, len);
        idx += len;
    }
    strlist_del_mult(strlist_fist(&head), strlist_last(&head));
    printf("%s\n", total_str);
    if (list_empty(&head.list)) {
        printf("list is empty\n");
    }
    return 0;
}

//int main()
//{
//    HANDLE hMutex = CreateMutex(NULL, FALSE, "c_moc.mutex");
//
//
//    WaitForSingleObject(hMutex, 60*1000L);
//    printf("Hello world!\n");
//    Sleep(10*1000);
//    printf("process over!\n");
//    ReleaseMutex(hMutex);
////    system("pause");
//    return 0;
//}
