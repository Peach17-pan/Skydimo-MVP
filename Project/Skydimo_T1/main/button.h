#pragma once
#include <stdbool.h>

void button_init(void);
int button_scan(void);  // 改为返回动作类型