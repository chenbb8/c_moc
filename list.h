/**
 * @file list.h
 * @author hz010153 (yunlei.zhou@3d-scantech.com)
 * @brief 简单的双向链表
 * @version V1.0.0
 * @date 2021-08-29
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MEMBER_OFFSET(type, mem)   ((size_t) & (((type *)0)->mem))
#define CONTAINER_OF(p, type, mem) ((type *)((size_t)(p)-MEMBER_OFFSET(type, mem)))

struct list_node
{
    struct list_node *prev;
    struct list_node *next;
};

#define LIST_HEAD_INIT(head) \
    { \
        .prev = &(head), \
        .next = &(head) \
    }

/**
* @brief 初始化链表节点
* @param[out] list 需要操作的链表
* @note  初始化链表节点让它指向本身。如果这是一个链表头，则输出一个空链表
*/
static inline void INIT_LIST_HEAD(struct list_node *list)
{
	list->next = list;
	list->prev = list;
}

/**
 * @brief 在两个已知节点间插入一个节点
 * @param[in,out] new  指向新节点的指针
 * @param[in,out] prev 指向想要插入点前一个节点的指针
 * @param[int,out] next 指向想要插入点后一个节点的指针
 */
static inline void __list_add(struct list_node *new, struct list_node *prev, struct list_node *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/**
 * @brief 在指定的头之后插入一个节点
 * @param[in,out] new  指向新节点的指针
 * @param[in,out] head 需要在此节点后增加一个新节点
 */
static inline void list_add(struct list_node *new, struct list_node *head)
{
	__list_add(new, head, head->next);
}

/**
 * @brief 在链表尾部添加一个节点
 * @param[in,out] new 指向新节点的指针
 * @param[in,out] head 指向链头的指针
 */
static inline void list_add_tail(struct list_node *new, struct list_node *head)
{
    __list_add(new, head->prev, head);
}

/**
 * @brief 删除指定的节点
 * @param[in,out] node 指向要移除的节点的指针
 */
static inline void list_del(struct list_node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node;
    node->next = node;
}

/**
 * @brief 测试本节点是否为第一个节点
 * @param[in] list 需要测试的节点
 * @param[in] head 指向链头的指针
 * @return 返回测试结果
 */
static inline int list_is_first(const struct list_node *list, const struct list_node *head)
{
	return list->prev == head;
}

/**
 * @brief 测试本节点是否为最后一个节点
 * @param[in] list 需要测试的节点
 * @param[in] head 指向链头的指针
 * @return 返回测试结果
 */
static inline int list_is_last(const struct list_node *list, const struct list_node *head)
{
	return list->next == head;
}

/**
 * @brief 测试本节点是否为链表头部
 * @param[in] list 需要测试的节点
 * @param[in] head 指向链头的指针
 * @return 返回测试结果
 */
static inline int list_is_head(const struct list_node *list, const struct list_node *head)
{
	return list == head;
}

/**
 * @brief 测试链表是否为空
 * @param[in] head 指向链头的指针
 * @return 返回测试结果
 */
static inline int list_empty(const struct list_node *head)
{
	return head->next == head;
}

/**
* @brief 从链表中获取第一个节点所在结构体的指针
* @param[in] ptr 要从中获取元素的链表头
* @param[in] type 含有该链表成员的结构体类型
* @param[in] member 链表在该结构体中的成员名
* @note 该链表不应该是空的
*/
#define list_first_entry(ptr, type, member) \
	CONTAINER_OF((ptr)->next, type, member)

/**
* @brief 从链表中获取最后一个节点所在结构体的指针
* @param[in] ptr 要从中获取元素的链表头
* @param[in] type 含有该链表成员的结构体类型
* @param[in] member 链表在该结构体中的成员名
* @note 该链表不应该是空的
*/
#define list_last_entry(ptr, type, member) \
	CONTAINER_OF((ptr)->prev, type, member)

/**
* @brief 从链表中获取下一个节点所在结构体的指针
* @param[in] pos 指向当前节点所在结构体的指针
* @param[in] member 链表在该结构体中的成员名
* @note 该链表不应该是空的
*/
#define list_next_entry(pos, member) \
	CONTAINER_OF((pos)->member.next, typeof(*(pos)), member)

/**
* @brief 测试当前结构体指针是否指向链表头部
* @param[in] pos 需要测试的节点所在结构体的指针
* @param[in] head 指向链头的指针
* @param[in] member 链表在该结构体中的成员名
*/
#define list_entry_is_head(pos, head, member)				\
	(&pos->member == (head))

/**
* @brief 正向遍历链表
* @param[out] pos 用于遍历链表节点的所在结构体的指针
* @param[in] head 指向list_node链头的指针
* @param[in] member 链表在该结构体中的成员名
*/
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     !list_entry_is_head(pos, head, member);			\
	     pos = list_next_entry(pos, member))


#ifdef __cplusplus
}
#endif

#endif
