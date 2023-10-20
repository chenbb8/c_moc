#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
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

int main1()
{
    //cmoc_preprocess_file("F:\\project\\280\\ZW280\\ZW280_c_single_slot\\Projects\\USER\\function.c");
    cmoc_preprocess_file("test_sig.c");
}

int main()
{
    int len = strlen(text);
    strlist_append_str(&head, text);

    strlist_t *pos;
    pos = strlist_last(&head);
//    strlist_for_each_entry(pos, &head)
    {
        printf("%s\n", pos->str);
    }

    pos = strlist_fist(&head);
    strlist_split(pos, &pos->str[0], 4, STRLIST_LABEL_AA);

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
