MESSAGE(STATUS "current build : rpc_middle")

#设置生成的so动态库最后输出的路径 debug和release版本目录不一样


ADD_COMPILE_OPTIONS(-fPIC)

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_PRC_MIDDLE_SRCS 变量
AUX_SOURCE_DIRECTORY(. DIR_PRC_MIDDLE_SRCS)
# 生成静态库链接库rpc_middle
ADD_LIBRARY (rpc_middle ${DIR_PRC_MIDDLE_SRCS})

# 将库文件安装到lib目录
# INSTALL(TARGETS rpc DESTINATION lib)
# 将头文件include
# INSTALL(FILES *.h DESTINATION include) 