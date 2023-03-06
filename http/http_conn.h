#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200; //设置读取文件的名称m_real_file大小
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;//写是1024字节，读是2048字节
    enum METHOD        //报文的请求方法，本项目只用到GET和POST
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE //主状态机的状态
    {
        CHECK_STATE_REQUESTLINE = 0, //请求行
        CHECK_STATE_HEADER, //请求头
        CHECK_STATE_CONTENT //请求体
    };
    enum HTTP_CODE  //报文解析的结果
    {
        NO_REQUEST,//不完整 继续接受然后解析
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE, //没有资源
        FORBIDDEN_REQUEST, //访问权限不够
        FILE_REQUEST, //请求的是文件
        INTERNAL_ERROR,
        CLOSED_CONNECTION //关闭连接
    };
    enum LINE_STATUS   //从状态机的状态
    {
        LINE_OK = 0,
        LINE_BAD, //错误
        LINE_OPEN //不完整 继续接受然后解析
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname); //初始化套接字地址，函数内部会调用私有方法init（）
    void close_conn(bool real_close = true);  //关闭http连接
    void process();
    bool read_once();  //关闭http连接
    bool write();  //关闭http连接
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);     //同步线程初始化数据库读取表，把数据库表中的用户密码都哈希保存一下
    int timer_flag;
    int improv;


private:
    void init();
    //从m_read_buf读取，并处理请求报文
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);//主状态机解析请求行中的请求内容
    HTTP_CODE parse_headers(char *text);//主状态机解析请求头中的请求内容
    HTTP_CODE parse_content(char *text);//主状态机解析报文中的请求内容
    HTTP_CODE do_request();  //生成响应报文
    char *get_line() { return m_read_buf + m_start_line; };//get_line用于将指针向后偏移，指向未处理的字符（但是已经解析过了）
    LINE_STATUS parse_line();//从状态机读取一行，分析是请求报文的哪一部分
    void unmap();//解除文件映射
    //根据响应报文格式，生成对应8个部分，以下函数均由do_request调用
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;//每个http对应请求的数据库连接指针
    int m_state;  //读为0, 写为1，任务是读还是写得状态

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];//存储读取的请求报文数据
    int m_read_idx;//缓冲区中m_read_buf中数据的最后一个字节的下一个位置，表示已经读了多少字节
    int m_checked_idx;//m_read_buf读取的位置m_checked_idx
    int m_start_line;//m_read_buf中已经解析的字符个数
    char m_write_buf[WRITE_BUFFER_SIZE];//存储发出的响应报文数据
    int m_write_idx;//指示buffer中的长度
    CHECK_STATE m_check_state;//主状态机的状态
    METHOD m_method;//请求方法

        //以下为解析请求报文中对应的6个变量
    char m_real_file[FILENAME_LEN]; //存储读取文件的名称
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;
    char *m_file_address;//读取服务器上的文件地址
    struct stat m_file_stat;//文件状态
    struct iovec m_iv[2];//分散读的内容结构体
    int m_iv_count;//上面结构体用到的下标个数
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send; //剩余发送字节数
    int bytes_have_send;  //已发送字节数
    char *doc_root;//根路径，定位到当前root

    map<string, string> m_users;
    int m_TRIGMode;//触发模式ET或者LT
    int m_close_log;//是否开启日志

    char sql_user[100];//数据库相关
    char sql_passwd[100];
    char sql_name[100];
};

#endif
