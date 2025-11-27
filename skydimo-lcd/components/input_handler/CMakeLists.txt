# 输入处理器组件
idf_component_register(SRCS "input_handler.c"
                    INCLUDE_DIRS "." "../../main"
                    REQUIRES driver freertos)