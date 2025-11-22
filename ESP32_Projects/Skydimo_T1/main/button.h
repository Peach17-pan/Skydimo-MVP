#pragma once
#include <stdbool.h>

void button_init(void);
int button_scan(void);  // 返回: 0=无操作, 1=短按, 2=长按