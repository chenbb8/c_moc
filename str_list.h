#ifndef _STR_LIST_H_
#define _STR_LIST_H_

#include "list.h"
#include <stdbool.h>

typedef struct {
    struct list_node list;
    int label;
    char *str;
} strlist_t;

enum {
    STRLIST_LABEL_NONE,/*初始化strlist节点后的默认值*/
    STRLIST_LABEL_USER,/*用户label起始值*/
};

//默认非head的节点都是在内部自动动态分配，节点下如果有字符串，则也是在内部自动分配
#define STRLIST_MALLOC      malloc
#define STRLIST_FREE        free

/**
* @brief 初始化strlist的头指针
* @param[in,out] head 需要操作的链表
* @note 本函数不会在内部new节点和字符串（也就是说别的函数很可能会这样做）
*/
void strlist_head_init(strlist_t *head);

/**
* @brief 分割链表
* @param[in,out] node 需要操作的链表节点
* @param[in] point 需要操作的字符串分割点
* @param[in] size 需要分割出去的大小
* @param[in] label 分割后关键字符串所在节点的label值
* @note point指向的字符分配到新分割出的链表里。如果point+size指向的不是当前node->src的最后一个
*       字符，则本操作可能会产生三个新链表
*/
void strlist_split(strlist_t *node, const char *point, int size, int label);

/**
* @brief 根据关键字段分割链表
* @param[in,out] node 需要操作的链表节点
* @param[in] key_s/key_e 所使用的关键字段的起始和结束字符串的指针
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
void strlist_append_str(strlist_t *head, const char *append);

/**
* @brief 在链表中删除对应节点
* @param[in] note 需要删除的节点
* @note 不允许重复删除，否则因节点被重复free而出现问题
*/
void strlist_del(strlist_t *node);

/**
* @brief 在链表中删除多个节点
* @param[in] prev 需要删除的节点，起始点
* @param[in] next 需要删除的节点，结束点
* @note 不允许重复删除，否则因节点被重复free而出现问题。删除操作包括prev和next
*/
void strlist_del_mult(strlist_t *prev, strlist_t *next);

/**
* @brief 计算链表中包含的字符串的总长度
* @param[in] head 指向strlist链表头部的指针
* @return 计算出的字符串总长度
* @note 总长度不包含'/0'
*/
int strlist_total_slen(strlist_t *head);

/**
* @brief 计算链表中有效节点的数量
* @param[in] head 指向strlist链表头部的指针
* @return 计算出的有效节点的数量
*/
int strlist_total_numb(strlist_t *head);

/**
* @brief 获取链表中的首节点
* @param[in] head 指向strlist链表头部的指针
*/
#define strlist_fist(head) list_first_entry(&(head)->list, strlist_t, list)

/**
* @brief 获取下一个节点
* @param[in] node 指向strlist链表当前节点的指针
*/
#define strlist_next(node) list_first_entry(&(node)->list, strlist_t, list)

/**
* @brief 获取链表中的尾节点
* @param[in] head 指向strlist链表头部的指针
*/
#define strlist_last(head) list_last_entry(&(head)->list, strlist_t, list)

/**
* @brief 获取下一个节点
* @param[in] node 指向strlist链表当前节点的指针
*/
#define strlist_prev(node) list_last_entry(&(node)->list, strlist_t, list)

/**
* @brief 正向遍历链表
* @param[out] pos 用于遍历链表的strlist指针
* @param[in] head 指向strlist链表头部的指针
*/
#define strlist_for_each_entry(pos, head) \
    list_for_each_entry(pos, &(head)->list, list)

#endif

