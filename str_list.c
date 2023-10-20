#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "str_list.h"

/**
* @brief 初始化strlist的头指针
* @param[in,out] head 需要操作的链表
* @note 本函数不会在内部new节点和字符串（也就是说别的函数很可能会这样做）
*/
void strlist_head_init(strlist_t *head)
{
    INIT_LIST_HEAD(&head->list);
    head->label = STRLIST_LABEL_NONE;
    head->str = NULL;
}

///**
//* @brief 模式匹配，搜索字符串
//* @param[in] src 需要被查找的字符串
//* @param[in] pattern 需要匹配的
//* @return
//*   @retval NULL 没有匹配到
//*   @retval 非零值 在src中匹配到的字符串的地址
//*/
//static char* strlist_pattern(char *src, char *pattern)
//{
//    if ((src == NULL)||(pattern == NULL)) {
//        return NULL;
//    }
//}
//
///**
//* @brief 根据关键字分割链表
//* @param[in,out] head 需要操作的链表
//* @param[in] key 所使用的关键字符串
//* @param[in] label 分割后关键字符串所在节点的label值
//*/
//void strlist_split_keyword(strlist_t *head, const char *key, int label)
//{
//    strlist_t *pos;
//
//    list_for_each_entry(pos, (&(head->list)), list)
//    {
//        *pat = strlist_pattern(pos->str, key);
//        if (pat != NULL)
//    }
//}
/**
* @brief 分割链表
* @param[in,out] node 需要操作的链表节点
* @param[in] point 需要操作的字符串分割点
* @param[in] size 需要分割出去的大小
* @param[in] label 分割后关键字符串所在节点的label值
* @note point指向的字符分配到新分割出的链表里。如果point+size指向的不是当前node->src的最后一个
*       字符，则本操作可能会产生三个新链表
*/
void strlist_split(strlist_t *node, const char *point, int size, int label)
{
    if ( (node == NULL) || (node->str == NULL) ||(point == NULL) || (size == 0) ) {
        return;
    }
    assert(point >= node->str);
    assert((point - node->str+size) <= strlen(node->str));

    char *original_str = node->str;
    int original_len = strlen(node->str);
    int point_idx = point - node->str;//分割点对应的str数组索引

    //处理point+size之后的字符串
    if (point_idx+size < original_len)
    {
        strlist_t *new_node = STRLIST_MALLOC(sizeof(strlist_t));
        new_node->str = STRLIST_MALLOC(original_len-(point_idx+size)+1);//多加一个字节存放/0

        memcpy(new_node->str, point+size, original_len-(point_idx+size));
        new_node->str[original_len-(point_idx+size)] = '\0';
        new_node->label = STRLIST_LABEL_NONE;
        //加入到当前节点的后面
        list_add(&new_node->list, &node->list);
    }
    //处理需要分裂出来的字符串
    {
        strlist_t *new_node = STRLIST_MALLOC(sizeof(strlist_t));
        new_node->str = STRLIST_MALLOC(size+1);//多加一个字节存放/0

        memcpy(new_node->str, point, size);
        new_node->str[size] = '\0';
        new_node->label = label;
        //加入到当前节点的后面
        list_add(&new_node->list, &node->list);
    }
    //处理point之前的字符串，存放在原有节点中
    if (point_idx != 0)//point_idx可以用做分裂后，处于point点之前的字符串的有效大小
    {
        node->str = STRLIST_MALLOC(point_idx+1);//多加一个字节存放/0

        memcpy(node->str, original_str, point_idx);
        node->str[point_idx] = '\0';
        //释放原有字符串
        STRLIST_FREE(original_str);
    }
    else//point之前的字符串已经被全部裁切出去了
    {
        //删除当前节点
        strlist_del(node);
    }
}

/**
* @brief 根据关键字段分割链表
* @param[in,out] node 需要操作的链表
* @param[in] key_s/key_e 所使用的关键字段的起始和结束字符串
* @param[in] label 分割后关键字符字段所在节点的label值
*/
void strlist_split_section(strlist_t *node, const char *key_s,  const char *key_e, int label);

/**
* @brief 查找是否有节点的字符串与关键字符串相同
* @param[in,out] head 需要操作的链表
* @param[in] key 所使用的关键字符串
* @return 是否查找到
*/
bool strlist_is_found_keyword(strlist_t *head, const char *key);

/**
* @brief 将链表中对应label值所在的节点的字符串替换掉
* @param[in,out] head 需要操作的链表
* @param[in] label 需要操作的label值
* @return 是否有替换
*/
bool strlist_replace_keyword(strlist_t *head, int label, const char *replace);

/**
* @brief 在链表尾部追加新节点，填入对应的字符串
* @param[in,out] head 需要操作的链表
* @param[in] append 需要追加的字符串
*/
void strlist_append_str(strlist_t *head, const char *append)
{
    int size = strlen(append);
    strlist_t *new_node = STRLIST_MALLOC(sizeof(strlist_t));
    new_node->str = STRLIST_MALLOC(size+1);

    memcpy(new_node->str, append, size);
    new_node->str[size] = '\0';
    new_node->label = STRLIST_LABEL_NONE;
    //加入到链表的尾部
    list_add_tail(&new_node->list, &head->list);
}

/**
* @brief 在链表中删除对应节点
* @param[in] node 需要删除的节点
* @note 不允许重复删除，否则因节点被重复free而出现问题
*/
void strlist_del(strlist_t *node)
{
    list_del(&node->list);
    if (node->str != NULL) {
        STRLIST_FREE(node->str);
        node->str = NULL;
    }
    STRLIST_FREE(node);
}

/**
* @brief 在链表中删除多个节点
* @param[in] prev 需要删除的节点，起始点
* @param[in] next 需要删除的节点，结束点
* @note 不允许重复删除，否则因节点被重复free而出现问题。删除操作包括prev和next
*/
void strlist_del_mult(strlist_t *prev, strlist_t *next)
{
    strlist_t *next_temp = prev;

    do {
        prev = next_temp;
        next_temp = strlist_next(prev);
        strlist_del(prev);
    } while (prev != next);
}

/**
* @brief 计算链表中包含的字符串的总长度
* @param[in] head 指向strlist链表头部的指针
* @return 计算出的字符串总长度
* @note 总长度不包含'/0'
*/
int strlist_total_slen(strlist_t *head)
{
    strlist_t *pos;
    int len = 0;
    strlist_for_each_entry(pos, head)
    {
        len += strlen(pos->str);
    }
    return len;
}

/**
* @brief 计算链表中有效节点的数量
* @param[in] head 指向strlist链表头部的指针
* @return 计算出的有效节点的数量
*/
int strlist_total_numb(strlist_t *head)
{

}


