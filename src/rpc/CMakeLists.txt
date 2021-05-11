MESSAGE(STATUS "current build : rpc_lib")



ADD_COMPILE_OPTIONS(-fPIC)
# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
AUX_SOURCE_DIRECTORY(. DIR_RPC_SRCS)
# 生成静态库链接库Dir1
MESSAGE(STATUS "DIR_RPC_SRCS : ${DIR_RPC_SRCS}")
ADD_LIBRARY(rpc ${DIR_RPC_SRCS})

# 将库文件安装到lib目录
# INSTALL(TARGETS rpc DESTINATION lib)
# 将头文件include
# INSTALL(FILES *.h DESTINATION include) 