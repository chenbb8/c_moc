#ifndef _C_MOC_C_
#define _C_MOC_C_

#include <stdbool.h>

/**
* @brief 初始化cmoc模块
*/
void cmoc_init(void);

/**
* @brief 预处理文件，提取出关键字
* @param[in] file_name 需要预处理的文件
* @return 是否有检测到相应关键字
*/
bool cmoc_preprocess_file(const char *file_name);

/**
* @brief 将预处理提取的关键字，生成对应的文件
* @param[in] save_dir 保存生成文件的路径
*/
void cmoc_generation_and_save(const char *save_dir);

#endif // _C_MOC_C_
