#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mysql.h>

#define SPORT 7092
int sfd = 0, maxfd = 0;
int fd_addr[1024];
int count = 0;


MYSQL *conn_prt; //创造一个mysql句柄
MYSQL_RES *res;
MYSQL_ROW row;

struct msg
{
    int f; //消息类型
    int fd;//客户端描述符
    char buf[100];
    char username[20];
    char password[10];
    char question[30];
    char answer[10];
    char conman[20];
    char content[100];
    char filename[20];
    char file_content[1024];
};

void query(char s[])
{
    int t = mysql_real_query(conn_prt, s, strlen(s));
    if(t)
    {
        printf("failed to query:%s\n",mysql_error(conn_prt));
        exit(0);   
    }
}
void conmysql()
{
    int t;
    char query[50] ="show tables";
    char table_name[20] = {0};
    conn_prt = mysql_init(NULL);//初始化
    if( !mysql_real_connect(conn_prt, "localhost", "root", "123", "users", 0, NULL, 0) )
    {
        printf("failed to connect:%s\n",mysql_error(conn_prt));
        exit(0) ;
    }
    printf("connect success!\n");
}
void resign(struct msg rm)
{
    char s[100] = {0};
    struct msg sm;
    int f = 1, ret;
    strcat(s, "select username from user");
    query(s);
    res = mysql_store_result(conn_prt);
    while(row = mysql_fetch_row(res))
    {
        for(int i = 0; i < mysql_num_fields(res); i++)
        {
            if(strcmp(row[i], rm.username) == 0)
            {
                f = 0;
                break;
            }
        }
    }
    if(f == 0)
    {
        strcpy(sm.buf, "用户名已存在");
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);        
        }
    }
    else
    {
        char inserts[100] = {0};
        char str[100] = {0};
        strcat(inserts, "insert into user");
        strcat(inserts, "(username, password, question, answer, fd, status)");
        strcat(inserts, "values");
        sprintf(str, "('%s','%s','%s','%s',%d,%d)", rm.username, rm.password, rm.question, rm.answer, rm.fd, 0);
        strcat(inserts, str);
        // printf("%s\n", inserts);
        query(inserts);
        strcpy(sm.buf, "ok");
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);        
        }
    }
}
void changepassword(struct msg rm)
{
    struct msg sm;
    char str[100] = {0}, s[100] = {0};
    int ret;
    strcat(str, "select * from user");
    query(str);
    res = mysql_store_result(conn_prt);
    while(row = mysql_fetch_row(res))
    {
        for(int i = 0; i < mysql_num_fields(res); i++)
        {
            if(strcmp(row[i], rm.username) == 0)
            {
                strcpy(sm.question, row[2]);
                strcpy(sm.answer, row[3]);
                ret = send(rm.fd, &sm, sizeof(struct msg), 0);
                if(ret < 0)
                {
                    perror("send error");
                    exit(0);           
                }
                ret = recv(rm.fd, &rm, sizeof(struct msg), 0);
                if(ret < 0)
                {
                    perror("send error");
                    exit(0);                
                }
                if(strcmp(rm.buf, "change") == 0)
                {
                    sprintf(s, "update user set password = '%s' where username = '%s'", rm.password, rm.username);
                    query(s);
                }
                break;            
            }
        }
    }
}
void login(struct msg rm)
{
    char s[100] = {0};
    struct msg sm;
    int f = 0, ret;
    strcat(s, "select * from user");
    query(s);
    res = mysql_store_result(conn_prt);
    while(row = mysql_fetch_row(res))
    {
        for(int i = 0; i < mysql_num_fields(res); i++)
        {
            if(strcmp(row[i], rm.username) == 0)
            {
                f = 1;
                if(strcmp(row[1], rm.password) == 0)
                {
                    sm.fd = atoi(row[4]);
                    strcpy(sm.buf, "ok");
                    char str[100] = {0};
                    sprintf(str, "update user set status = 1 where username = '%s'", rm.username);
                    query(str);
                    sprintf(str, "update user set fd = %d where username = '%s'", rm.fd, rm.username);
                    query(str);
                }
                else 
                strcpy(sm.buf, "密码错误");
                break;
            }
        }
    }
    if(f == 0)
    strcpy(sm.buf, "用户名不存在");
    ret = send(rm.fd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);                
    }
}
void search_online(struct msg rm)
{
    struct msg sm;
    sm.f = 4;
    char str[100] = {0};
    int ret, count = 0;
    strcat(str, "select status from user");
    query(str);
    res = mysql_store_result(conn_prt);
    while(row = mysql_fetch_row(res))
    {
        for(int i = 0; i < mysql_num_fields(res); i++)
        {
            int status = atoi(row[i]);
            if(status)
            count++;
        }
    }
    sprintf(sm.buf, "在线人数: %d", count);
    ret = send(rm.fd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);                           
    }
}
void talk_private(struct msg rm)
{
    struct msg sm;
    sm.f = -5;
    char str[100] = {0}, s[100] = {0};
    int ret, f = 0;
    sprintf(str, "select status from user where username = '%s'", rm.username);
    query(str);
    res = mysql_store_result(conn_prt);
    if(row = mysql_fetch_row(res))
    {
        int x = atoi(row[0]);
        if(x < 0)
        {
            if(x == -1) strcpy(sm.buf, "你已经被禁言");
            if(x == -2) strcpy(sm.buf, "你已经被踢出群聊");
            ret = send(rm.fd, &sm, sizeof(struct msg), 0);
            if(ret < 0)
            {
                perror("send error");
                exit(0);
            }
            return;
        }
    }
    sprintf(str, "select status from user where username = '%s'", rm.conman);
    query(str);
    res = mysql_store_result(conn_prt);
    if(row = mysql_fetch_row(res))
    {
        f = 1;
        int x = atoi(row[0]), y;
        if(x > 0 || x == -1) 
        {
            sm.f = 5;
            sprintf(str, "select fd from user where username = '%s'", rm.conman);
            query(str);
            res = mysql_store_result(conn_prt);
            if(row = mysql_fetch_row(res)) y = atoi(row[0]);
            strcpy(sm.content, rm.content);
            ret = send(y, &sm, sizeof(struct msg), 0);
            if(ret < 0)
            {
                perror("send error");
                exit(0);                          
            }
        }
        else if(x == 0)
        sprintf(sm.buf, "%s 未上线", rm.conman);
        else 
        sprintf(sm.buf, "%s被踢出了群聊", rm.conman);
    }
if(sm.f == -5)
{
    if(f == 0)
    sprintf(sm.buf, "%s: 不存在该用户\n", rm.conman);
    ret = send(rm.fd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);                  
    }
}
}
void talk_public(struct msg rm)
{
    struct msg sm;
    sm.f = -6;
    char str[100] = {0}, s[100] = {0};
    int ret, f = 0;
    sprintf(str, "select status from user where username = '%s'", rm.username);
    query(str);
    res = mysql_store_result(conn_prt);
    if(row = mysql_fetch_row(res))
    {
        int x = atoi(row[0]);
        if(x < 0)
        {
            if(x == -1) strcpy(sm.buf, "你已经被禁言");
            if(x == -2) strcpy(sm.buf, "你已经被踢出群聊");
            ret = send(rm.fd, &sm, sizeof(struct msg), 0);
            if(ret < 0)
            {
                perror("send error");
                exit(0);
            }
            return;
        }
    }
    sprintf(str, "select fd from user where status = 1 and status != -2 and username != '%s'", rm.username);
    query(str);
    res = mysql_store_result(conn_prt);
    while(row = mysql_fetch_row(res))
    {
        sm.f = 6;
        int x = atoi(row[0]);
        sprintf(sm.content, "群聊中: (%s说) %s", rm.username, rm.content);
        ret = send(x, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                          
        }
    }
    if(sm.f == -6)
    {
        sprintf(str, "update user set status = 2 where username = '%s'", rm.username);
        query(str);
        strcpy(sm.buf, "(当前除了你没人在群聊里, 你已成为群主)");
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                                          
        }
    }
}
void log_off(struct msg rm)
{
    char str[100] = {0};
    struct msg sm;
    int ret;
    sm.f = 7;
    sprintf(str, "update user set status = 0 where username = '%s'", rm.username);
    query(str);
    strcpy(sm.buf, "log off ok");
    ret = send(rm.fd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);                                                                  
    }
}
void move_person(struct msg rm)
{
    struct msg sm;
    char str[100] = {0};
    int ret;
    if(strcmp(rm.buf, "is group_owner") == 0)
    {
        sm.f = -8;
        sprintf(str, "select status from user where username = '%s'", rm.username);
        query(str);
        res = mysql_store_result(conn_prt);
        if(row = mysql_fetch_row(res))
        {
            if(strcmp(row[0], "2") == 0) strcpy(sm.buf, "is");
            else strcpy(sm.buf, "not");
        }
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
    else
    {
        sprintf(str, "update user set status = -2 where username = '%s'", rm.buf);
        query(str);
        sm.f = 8;
        strcpy(sm.buf, "move person ok");
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
}
void forbidden_words(struct msg rm)
{
    struct msg sm;
    char str[100] = {0};
    int ret;
    if(strcmp(rm.buf, "is group_owner") == 0)
    {
        sm.f = -9;
        sprintf(str, "select status from user where username = '%s'", rm.username);
        query(str);
        res = mysql_store_result(conn_prt);
        if(row = mysql_fetch_row(res))
        {
            if(strcmp(row[0], "2") == 0) strcpy(sm.buf, "is");
            else strcpy(sm.buf, "not");
        }
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
    else
    {
        sprintf(str, "update user set status = -1 where username = '%s'", rm.buf);
        query(str);
        sm.f = 9;
        strcpy(sm.buf, "forbidden person word ok");
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
}
void relieve_word(struct msg rm)
{
    struct msg sm;
    char str[100] = {0};
    int ret;
    if(strcmp(rm.buf, "is group_owner") == 0)
    {
        sm.f = -10;
        sprintf(str, "select status from user where username = '%s'", rm.username);
        query(str);
        res = mysql_store_result(conn_prt);
        if(row = mysql_fetch_row(res))
        {
            if(strcmp(row[0], "2") == 0) strcpy(sm.buf, "is");
            else strcpy(sm.buf, "not");
        }
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
    else
    {
        sprintf(str, "update user set status = 1 where username = '%s'", rm.buf);
        query(str);
        sm.f = 10;
        strcpy(sm.buf, "relieve person word ok");
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
}
void send_file(struct msg rm)
{
    struct msg sm;
    int ret, x;
    char str[100] = {0}, *name;
    sm.f = 11;
    sprintf(str, "select fd from user where username = '%s'", rm.conman);
    query(str);
    res = mysql_store_result(conn_prt);
    row = mysql_fetch_row(res);
    x = atoi(row[0]);
    if(strcmp(rm.buf, "filename") == 0)
    {
        strcpy(sm.buf, "filename");
        if(NULL == strrchr(rm.filename, '/')) name = rm.filename;
        else name = strrchr(rm.filename, '/') + 1;
        sprintf(sm.content, "%s 向你传送文件%s", rm.username, name);
        strcpy(sm.filename, name);
        ret = send(x, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);
        }
    }
    else if(strcmp(rm.buf, "content") == 0)
    {
        sm = rm;
        ret = send(x, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);        
        }
    }
    else
    {
        sm = rm;
        strcpy(sm.buf, "end");
        strcpy(sm.content, "文件传输结束");
        ret = send(x, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);        
        }
    }
}
int main()
{
    int i, ret;
    conmysql();
    struct sockaddr_in seraddr;
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0)
    {
        perror("socket errno");
        exit(0);
    }
    seraddr.sin_family = AF_INET;
    seraddr.sin_addr.s_addr = htonl(INADDR_ANY); //0.0.0.0
    seraddr.sin_port = htons(SPORT);
    memset(seraddr.sin_zero, 0, 8);

    ret = bind(sfd, (struct sockaddr *)&seraddr, sizeof(seraddr));
    if (ret < 0)
    {
        perror("bind error");
        exit(0);
    }

    ret = listen(sfd, 10);
    if (ret < 0)
    {
        perror("listen error");
        exit(0);
    }

    fd_set fds, readfds;     //创建文件描述符集合
    FD_ZERO(&fds);  //将文件描述符集合置空
    FD_SET(sfd, &fds); //套接字加入集合中
    maxfd = sfd;
    readfds = fds;
    // fd_addr[count++] = sfd; //加入文件描述符表中

    while(1)
    {
        readfds = fds;
        ret = select(maxfd+1, &readfds, NULL, NULL, NULL);
        if(ret < 0)
        {
            continue;
        }
        else if(ret == 0)
        printf("select timeout\n");
        else
        {
            if(FD_ISSET(sfd, &readfds))
            {
                int newfd = accept(sfd, NULL, 0);
                if(newfd < 0)
                {
                    printf("accept");
                    continue;
                }
                else
                {
                    printf("new incoming connection, fd = %d\n", newfd);
                    FD_SET(newfd, &fds);
                    if(newfd > maxfd) maxfd = newfd;
                    fd_addr[count++] = newfd;
                }
            }
            else
            {
                struct msg rm;
                for(i = 0; i < count; i++)
                {
                    if(FD_ISSET(fd_addr[i], &readfds))
                    {
                        ret = recv(fd_addr[i], &rm, sizeof(rm), 0);
                        if(ret == 0)
                        {
                            printf("client %d offline.\n", fd_addr[i]);
                            close(fd_addr[i]);
                            FD_CLR(fd_addr[i], &fds);
                            int j;
                            for(j = i; j < count-1; ++j)
                            fd_addr[j] = fd_addr[j+1];
                            --count;
                        }
                        else if(ret > 0)
                        {
                            rm.fd = fd_addr[i];
                            if(rm.f == 1)  resign(rm);
                            else if(rm.f == 2) login(rm);
                            else if(rm.f == 3) changepassword(rm);
                            else if(rm.f == 4) search_online(rm);
                            else if(rm.f == 5) talk_private(rm);
                            else if(rm.f == 6) talk_public(rm);
                            else if(rm.f == 7) log_off(rm);
                            else if(rm.f == 8) move_person(rm);
                            else if(rm.f == 9) forbidden_words(rm);
                            else if(rm.f == 10) relieve_word(rm);
                            else if(rm.f == 11) send_file(rm);
                            else printf("???");
                        }
                        else
                        {
                            perror("recv error");
                            exit(0);
                        }
                        break;
                    }
                }
            }//end: 客户端发送消息
}//end :select
}//end: while(1)
close(sfd);
return 0;
}
