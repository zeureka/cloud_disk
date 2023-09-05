# 使用的编译器
CC=g++

# 预处理参数
CPPLFAGS=-I/usr/include/fastdfs			\
		 -I/usr/include/fastcommon		\
		 -I/usr/include/mysql			\
		 -I/usr/local/include/hiredis

# 选项
CFLAGS=-Wall

# 需要链接的动态库
LIBS=-lfdfsclient	\
	 -lfastcommon	\
	 -lhiredis		\
	 -lfcgi			\
	 -lmysqlclient	\
	 -lpthread

# 目录路径
TEST_PATH=./test
SRC_PATH=./src
CGI_BIN_PATH=./bin_cgi
CGI_SRC_PATH=./src/cgi

# 测试用
main=main
redis=redis

# 项目用
login=$(CGI_BIN_PATH)/login
enroll=$(CGI_BIN_PATH)/enroll
upload=$(CGI_BIN_PATH)/upload
md5=$(CGI_BIN_PATH)/md5
myfiles=$(CGI_BIN_PATH)/myfiles
dealfile=$(CGI_BIN_PATH)/dealfile
sharefiles=$(CGI_BIN_PATH)/sharefiles
dealsharefile=$(CGI_BIN_PATH)/dealsharefile
myinfo=$(CGI_BIN_PATH)/myinfo

# 最终目标
target=$(login)			\
	   $(enroll)		\
	   $(upload)		\
	   $(md5)			\
	   $(myfiles)		\
	   $(dealfile)		\
	   $(sharefiles)	\
	   $(dealsharefile) \
	   $(myinfo)
ALL:$(target)

# ************************ 项目程序 **************************
# login
$(login): $(CGI_SRC_PATH)/login_cgi.o	\
		  $(SRC_PATH)/log.o				\
		  $(SRC_PATH)/cJSON.o			\
		  $(SRC_PATH)/deal_mysql.o		\
		  $(SRC_PATH)/redis_op.o 		\
		  $(SRC_PATH)/cfg.o 			\
		  $(SRC_PATH)/util_cgi.o 		\
		  $(SRC_PATH)/des.o 			\
		  $(SRC_PATH)/base64.o 			\
		  $(SRC_PATH)/md5.o 			
	$(CC) $^ -o $@ $(LIBS)

# enroll
$(enroll): $(CGI_SRC_PATH)/enroll_cgi.o	\
		   $(SRC_PATH)/log.o 			\
		   $(SRC_PATH)/util_cgi.o 		\
		   $(SRC_PATH)/cJSON.o 			\
		   $(SRC_PATH)/deal_mysql.o 	\
		   $(SRC_PATH)/redis_op.o 		\
		   $(SRC_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)

# md5
$(md5): $(CGI_SRC_PATH)/md5_cgi.o 	\
	    $(SRC_PATH)/log.o 			\
	    $(SRC_PATH)/util_cgi.o 		\
	    $(SRC_PATH)/cJSON.o 		\
	    $(SRC_PATH)/deal_mysql.o 	\
	    $(SRC_PATH)/redis_op.o 		\
	    $(SRC_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)

# upload
$(upload): $(CGI_SRC_PATH)/upload_cgi.o \
		   $(SRC_PATH)/log.o			\
		   $(SRC_PATH)/util_cgi.o 		\
		   $(SRC_PATH)/cJSON.o 			\
		   $(SRC_PATH)/deal_mysql.o 	\
		   $(SRC_PATH)/redis_op.o  		\
		   $(SRC_PATH)/cfg.o
	$(CC)  $^ -o $@ $(LIBS)

# My Files
$(myfiles): $(CGI_SRC_PATH)/myfiles_cgi.o 	\
			$(SRC_PATH)/log.o				\
			$(SRC_PATH)/util_cgi.o 			\
			$(SRC_PATH)/cJSON.o 			\
			$(SRC_PATH)/deal_mysql.o 		\
			$(SRC_PATH)/redis_op.o  		\
			$(SRC_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)

# deal file
$(dealfile): $(CGI_SRC_PATH)/dealfile_cgi.o \
			 $(SRC_PATH)/log.o				\
			 $(SRC_PATH)/util_cgi.o 		\
			 $(SRC_PATH)/cJSON.o 			\
			 $(SRC_PATH)/deal_mysql.o 		\
			 $(SRC_PATH)/redis_op.o  		\
			 $(SRC_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)

# share files
$(sharefiles):	$(CGI_SRC_PATH)/sharefiles_cgi.o \
				$(SRC_PATH)/log.o				 \
				$(SRC_PATH)/util_cgi.o 			 \
				$(SRC_PATH)/cJSON.o 			 \
				$(SRC_PATH)/deal_mysql.o 		 \
				$(SRC_PATH)/redis_op.o  		 \
				$(SRC_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)

# deal share file
$(dealsharefile):	$(CGI_SRC_PATH)/dealsharefile_cgi.o \
					$(SRC_PATH)/log.o  					\
					$(SRC_PATH)/util_cgi.o 				\
					$(SRC_PATH)/cJSON.o 				\
					$(SRC_PATH)/deal_mysql.o 			\
					$(SRC_PATH)/redis_op.o  			\
					$(SRC_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)

# my info
$(myinfo):	$(CGI_SRC_PATH)/myinfo_cgi.o	\
			$(SRC_PATH)/log.o  				\
			$(SRC_PATH)/util_cgi.o 			\
			$(SRC_PATH)/cJSON.o 			\
			$(SRC_PATH)/deal_mysql.o 		\
			$(SRC_PATH)/redis_op.o  		\
			$(SRC_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)

# 生成所有的.o文件
%.o:%.c %.cpp
	$(CC) -c $< -o $@(CPPLFAGS) $(CFLAGS)

# clear ".o" files
clean:
	-rm -rf *.o $(target) $(TEST_PATH)/*.o $(CGI_SRC_PATH)/*.o $(SRC_PATH)/*.o

# 声明伪文件
.PHONY:clean ALL
