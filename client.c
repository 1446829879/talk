#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define SPORT 7092

int sfd;
int flag, flags, growp_owner = 0;
FILE *rfp;
pthread_t pid;

struct msg
{
    int f; //消息类型
    int fd;//客户端描述符
    char buf[100];//提示信息等
    char username[20];
    char password[10];
    char question[30];
    char answer[10];
    char conman[20];
    char content[100];//聊天内容
    char filename[20];
    char file_content[1024];
};

void menu()
{
    printf("1: 注册账号\n");
    printf("2: 登录账号\n");
    printf("3: 忘记密码\n");
    printf("4: 退出聊天系统\n");
    printf("输入选项： ");
}
void menu_qq()
{
    printf("1: 查询在线人数\n");
    printf("2: 私聊\n");
    printf("3: 群聊\n");
    printf("4: 注销退出登录\n");
    printf("5: 踢人(群主权限)\n");
    printf("6: 禁言(群主权限)\n");
    printf("7: 解禁(群主权限)\n");
    printf("8: 文件传输\n");
}
void resign()    //注册账号
{
    while(1)
    {
        struct msg sm, rm;
        int ret;
        sm.f = 1;
        printf("input username: ");
        scanf("%s", sm.username);
        printf("input password: ");
        scanf("%s", sm.password);
        printf("input question: ");
        scanf("%s", sm.question);
        printf("input answer: ");
        scanf("%s", sm.answer);
        ret = send(sfd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);
        }
        ret = recv(sfd, &rm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("recv error");
            exit(0);
        }
        if(strcmp(rm.buf, "ok") == 0)
        {
            printf("resign ok\n");
            break;
        }
        else
        printf("%s\n", rm.buf);
    }
}
void changepassword(char name[])
{
    struct msg sm, rm;
    char answer[10] = {0};
    int ret;
    sm.f = 3;
    strcpy(sm.username, name);
    ret = send(sfd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);                        
    }
    ret = recv(sfd, &rm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("recv error");
        exit(0); 
    }
    printf("question: %s\n", rm.question);
    printf("input answer: ");
    scanf("%s", answer);
    if(strcmp(answer, rm.answer) == 0)
    {
        printf("input new password: ");
        scanf("%s", sm.password);
        strcpy(sm.buf, "change");
        ret = send(sfd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                
        }
        printf("change password ok\n"); 
    }
    else
    {
        strcpy(sm.buf, "error");
        ret = send(rm.fd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                               
        }
        printf("answer error\n");
    }
}
void *deal()
{
    struct msg rm, sm;
    int ret;
    while(1)
    {
        ret = recv(sfd, &rm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                    
        }
        if(rm.f == 4) printf("%s\n", rm.buf); // 在线人数
        else if(rm.f == 5 || rm.f == 6) printf("%s\n", rm.content);
        else if(rm.f == -5 || rm.f == -6) 
        {
            if(rm.f == 2) flag = 0;
            printf("%s\n", rm.buf);
        }
        else if(rm.f == 7)
        {
            flags = 0;
            printf("%s\n", rm.buf);
        }
        else if(rm.f == -8 || rm.f == -9 || rm.f == -10)
        {
            if(strcmp(rm.buf, "is") == 0) growp_owner = 1;
            else growp_owner = 0;
        }
        else if(rm.f == 8 || rm.f == 9 || rm.f == 10 )
        {
            printf("%s\n", rm.buf);
        }
        else if(rm.f == 11)
        {
            if(strcmp(rm.buf, "filename") == 0)
            {
                rfp = fopen(rm.filename, "a+");
                if(NULL == rfp)
                {
                    perror("open file error");
                    exit(0);
                }
                printf("%s\n", rm.content);
            }
            else if(strcmp(rm.buf, "content") == 0)
            {
                fwrite(rm.file_content, 1, 1024, rfp);
            }
            else 
            {
                fclose(rfp);
                printf("%s\n", rm.buf);
            }
        }
    }
}
int login(struct msg *man)
{
    struct msg sm, rm;
    int x, ret;
    while(1)
    {
        sm.f = 2;
        printf("input username: ");
        scanf("%s", sm.username);
        printf("input password: ");
        scanf("%s", sm.password);
        *man = sm;
        ret = send(sfd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);        
        }
        ret = recv(sfd, &rm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("recv error");
            exit(0);
        }
        if(strcmp(rm.buf, "ok") == 0)
        {
            printf("login ok\n");
            man->fd = rm.fd;
            return 1;
        }
        else
        printf("%s\n", rm.buf);
        printf("1: 忘记密码\n");
        printf("2: 重新输入\n");
        printf("输入选项: ");
        scanf("%d", &x);
        if(x == 1) changepassword(rm.username);
        else continue;
    }
}
void search_online()
{
    struct msg sm, rm;
    int ret;
    sm.f = 4; // 查询在线人数
    ret = send(sfd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);        
    }
}
void talk_private(struct msg man)
{
    struct msg sm, rm;
    int ret;
    char str[100] = {0};
    sm.f = 5;//私聊
    strcpy(sm.username, man.username);
    flag = 1;
    printf("输入私聊对象的用户名: \n");
    printf("输入要发送的消息(quit退出私聊): \n");
    while(flag)
    {
        scanf("%s", sm.conman);
        if(strcmp(sm.conman, "quit") == 0) break;
        scanf("%s", str);
        sprintf(sm.content, "%s 说: %s", man.username, str);
        strcpy(sm.buf, "talk");
        ret = send(sfd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);        
        }
        sleep(1);
    }
    sleep(1);
    printf("***************************\n");
    printf("你已经退出私聊\n");
    menu_qq();
}
void talk_public(struct msg man)
{
    struct msg sm;
    int ret;
    char s[100] = {0};
    flag = 1;
    sm.f = 6;//群聊
    strcpy(sm.username, man.username);
    printf("输入群聊消息(quit退出群聊)：\n");
    while(flag)
    {
        scanf("%s", s);
        if(strcmp(s, "quit") == 0) break;
        strcpy(sm.content, s);
        ret = send(sfd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);
        }
        usleep(100);
    }
    printf("************************\n");
    printf("你已经退出群聊\n");
    menu_qq();
}
void log_off(struct msg man)
{
    struct msg sm;
    int ret;
    sm.f = 7;//下线
    strcpy(sm.username, man.username);
    ret = send(sfd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);        
    }
}
void move_person(struct msg man)
{
    struct msg rm, sm;
    char move_name[20];
    strcpy(sm.username, man.username);
    strcpy(sm.buf, "is group_owner");
    sm.f = 8;
    int ret = send(sfd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);                                  
    }
    sleep(1);
    if(growp_owner)
    {
        printf("输入你要踢的用户名: ");
        scanf("%s", move_name);
        sm.f = 8;
        strcpy(sm.buf, move_name);
        ret = send(sfd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
    else 
    printf("你不是群主，没有该权限!\n");
}
void forbidden_words(struct msg man)
{
    struct msg sm;
    char forbidden_name[20];
    strcpy(sm.username, man.username);
    strcpy(sm.buf, "is group_owner");
    sm.f = 9; //禁言
    int ret = send(sfd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);                                  
    }
    sleep(1);
    if(growp_owner)
    {
        printf("输入你要禁言的用户名: ");
        scanf("%s", forbidden_name);
        sm.f = 9;
        strcpy(sm.buf, forbidden_name);
        ret = send(sfd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
    else 
    printf("你不是群主，没有该权限!\n");
}
void relieve_words(struct msg man)
{
    struct msg sm;
    char relieve_name[20];
    strcpy(sm.username, man.username);
    strcpy(sm.buf, "is group_owner");
    sm.f = 10; //解禁
    int ret = send(sfd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);                                  
    }
    sleep(1);
    if(growp_owner)
    {
        printf("输入你要解除禁言的用户名: ");
        scanf("%s", relieve_name);
        sm.f = 10;
        strcpy(sm.buf, relieve_name);
        ret = send(sfd, &sm, sizeof(struct msg), 0);
        if(ret < 0)
        {
            perror("send error");
            exit(0);                                  
        }
    }
    else 
    printf("你不是群主，没有该权限!\n");
}
void send_file(struct msg man)
{
    char file_content[1024] = {0};
    struct msg sm;
    int ret;
    FILE *fp;
    sm.f = 11;
    strcpy(sm.username, man.username);
    strcpy(sm.buf, "filename");
    printf("输入要传的文件名(包含路径): ");
    scanf("%s", sm.filename);
    printf("输入要传输的对象的用户名: ");
    scanf("%s", sm.conman);
    ret = send(sfd, &sm, sizeof(struct msg), 0);
    if(ret < 0)
    {
        perror("send error");
        exit(0);
    }
    fp = fopen(sm.filename, "r");
    if(NULL == fp)
    {
        perror("open file error");
        exit(0);
    }
    while(1)
    {
        if(feof(fp))
        {
            strcpy(sm.buf, "end");
            ret = send(sfd, &sm, sizeof(struct msg), 0);
            if(ret < 0)
            {
                perror("send error");
                exit(0);    
            }
        }
        else
        {
            memset(sm.file_content, 0, 1024);
            fread(sm.file_content, 1, 1024, fp);
            strcpy(sm.buf, "content");
            ret = send(sfd, &sm, sizeof(struct msg), 0);
            if(ret < 0)
            {
                perror("send error");
                exit(0);    
            }
        }
    }
}
int main(int argc, char *argv[])
{
    struct msg man;
    char name[20] = {0};
    struct sockaddr_in seraddr;
    sfd = socket(AF_INET, SOCK_STREAM, 6);
    if(sfd < 0)
    {
        perror("socket error");
        exit(0);
    }
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(SPORT);
    inet_pton(AF_INET, "127.0.0.1", &seraddr.sin_addr);
    memset(seraddr.sin_zero, 0, 8);
    if( (connect(sfd, (struct sockaddr *)&seraddr, sizeof(seraddr))) < 0    )
    {
        printf("can not connect to %s, exit\n", argv[1]);
        perror("connect error");
        exit(0);
    }
    printf("**********客户端**********\n");
    while(1)
    {
        int x, y, ret;
        menu();
        scanf("%d", &x);
        if(x == 1) resign();
        else if (x == 2) 
        {
            if(login(&man))
            {
                ret = pthread_create(&pid, NULL, deal, NULL);
                if(ret!=0)
                {
                    printf ("Create pthread error!\n");
                    exit(0);
                }
                usleep(100);
                printf("****************************\n");
                printf("%s welcome come into qq\n", man.username);
                menu_qq();
                flags = 1;
                while(flags)
                {
                    scanf("%d", &y);
                    if(y == 1) search_online();
                    else if(y == 2) talk_private(man);
                    else if(y == 3) talk_public(man);
                    else if(y == 4) log_off(man);
                    else if(y == 5) move_person(man);
                    else if(y == 6) forbidden_words(man);
                    else if(y == 7) relieve_words(man);
                    else if(y == 8) send_file(man);
                    sleep(1);
                }
            }
        }
        else if (x == 3) 
        {
            printf("输入用户名: ");
            scanf("%s", name);
            changepassword(name);
        }
        else break;
    }
return 0;
}
