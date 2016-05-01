#include  <unistd.h>
#include  <sys/types.h>       /* basic system data types */
#include  <sys/socket.h>      /* basic socket definitions */
#include  <netinet/in.h>      /* sockaddr_in{} and other Internet defns */
#include  <arpa/inet.h>       /* inet(3) functions */
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include <errno.h>
#include <hiredis/hiredis.h>
#include "cJSON/cJSON.h"
#include "unidirectional_queue/unidirectional_queue.h"

/*
 * Todo
 * 1,把你的字符串操作写成库，main里面太多函数了
 * */

#define PCRE2_CODE_UNIT_WIDTH 8

#include <pcre2.h>
#include <pthread.h>
#include <strings.h>
#include <stdbool.h>


const char *english_pattern = "^[^\u4e00-\u9fa5]++";

//typedef struct sockaddr  SA;
const int8_t chinese_word_byte = sizeof("字") - 1;
const int8_t chinese_char_byte = sizeof("字");
const char *redis_get_common = "get ";
struct TREE_CUT_WORDS_STUFF {
    char *start_word_item;
    bool have_first_not_cn_word;
    char *first_not_cn_word;
    char *had_deal_search_word;
};

//父线程传参給子线程的结构体
struct TREE_CUT_WORDS_DATA {
    //父级的分词结果
    char *cut_word_result;
    //原始的remainder_words指针，没有做偏移运算，用来free
    char *origin_remainder_words;
    //父级还没处理，剩余的搜索词
    char *remainder_words;
    //父级的分词权重
    int *words_weight;

    //主线程分词队列数组
    LinkQueue *pullwords_result_queue;
    //主线程分词队列数组写入锁
    pthread_mutex_t *pullwords_result_queue_write_lock;
    //主线程条件锁
    pthread_cond_t *pullwords_cond;
    //分词结果数,要用原子操作,支持并发
    int *pullwords_result_number;
    //已经分词完毕的结果数
    int *pullwords_result_finish_number;
};


char *substring(char *ch, int pos, int length) {
    char *pch = ch;
//定义一个字符指针，指向传递进来的ch地址。
    char *subch = (char *) calloc(sizeof(char), length + 1);
//通过calloc来分配一个length长度的字符数组，返回的是字符指针。
    int i;
//只有在C99下for循环中才可以声明变量，这里写在外面，提高兼容性。
    pch = pch + pos;
//是pch指针指向pos位置。
    for (i = 0; i < length; i++) {
        subch[i] = *(pch++);
//循环遍历赋值数组。
    }
    subch[length] = '\0';//加上字符串结束符。
    return subch;       //返回分配的字符数组地址。
}

//初始化分词结果结构体
void init_tree_cut_words_data(struct TREE_CUT_WORDS_DATA *cut_words_data) {
    (*cut_words_data).cut_word_result = (char *) malloc(WORD_RESULT_LEN);
    memset((*cut_words_data).cut_word_result, 0, WORD_RESULT_LEN);
    (*cut_words_data).words_weight = (int *) malloc(sizeof(int));
    memset((*cut_words_data).words_weight, 0, sizeof(int));
    (*cut_words_data).remainder_words = (char *) malloc(MAX_SEARCH_WORDS_LEN);
    memset((*cut_words_data).remainder_words, 0, MAX_SEARCH_WORDS_LEN);
    (*cut_words_data).origin_remainder_words = (*cut_words_data).remainder_words;
    /*
    (*cut_words_data).pullword_result_queue = (LinkQueue *) malloc(sizeof(LinkQueue));
    memset((*cut_words_data).pullword_result_queue, 0, sizeof(LinkQueue));
    InitQueue((*cut_words_data).pullword_result_queue);
     */

}

void add_words_weight(int *weight, int type) {
    switch (type) {
        case 1:
            *weight += 5;
            break;
        case 2:
            *weight += 10;
            break;
    }
}

/*从字符串的左边截取n个字符*/
char *left(char *dst, char *src, int n) {
    char *p = src;
    char *q = dst;
    int len = strlen(src);
    if (n > len) n = len;
    while (n--) *(q++) = *(p++);
    *(q++) = '\0'; /*有必要吗？很有必要*/
    return dst;
}

/*方法一，不改变字符串a,b, 通过malloc，生成第三个字符串c, 返回局部指针变量*/
char *join_char(char *a, char *b) {
    char *c = (char *) malloc(strlen(a) + strlen(b) + 1); //局部变量，用malloc申请内存
    memset(c, 0, strlen(a) + strlen(b) + 1);
    if (c == NULL) exit(1);
    char *tempc = c; //把首地址存下来
    while (*a != '\0') {
        *c++ = *a++;
    }
    while ((*c++ = *b++) != '\0') { ;
    }
    //注意，此时指针c已经指向拼接之后的字符串的结尾'\0' !
    return tempc;//返回值是局部malloc申请的指针变量，需在函数调用结束后free之
}

/*
 * return 1: success
 * return 0: error;
 * */
int8_t pecl_regx_match(PCRE2_SPTR subject, PCRE2_SPTR pattern, int *match_offset, int *match_str_lenght) {

    pcre2_code *re;
    PCRE2_SPTR name_table;

    int crlf_is_newline;
    int errornumber;
    int i;
    int namecount;
    int name_entry_size;
    int rc;
    int utf8;

    uint32_t option_bits;
    uint32_t newline;

    PCRE2_SIZE erroroffset;
    PCRE2_SIZE *ovector;

    size_t subject_length;
    pcre2_match_data *match_data;



    //char * _pattern = "[2-9]";
    char *_pattern = "[^\u4e00-\u9fa5]++";

    subject_length = strlen((char *) subject);

    re = pcre2_compile(
            pattern,               /* the pattern */
            PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
            0,                     /* default options */
            &errornumber,          /* for error number */
            &erroroffset,          /* for error offset */
            NULL);                 /* use default compile context */

    if (re == NULL) {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
        printf("PCRE2 compilation failed at offset %d: %s\n", (int) erroroffset,
               buffer);
        return 0;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    rc = pcre2_match(
            re,                   /* the compiled pattern */
            subject,              /* the subject string */
            subject_length,       /* the length of the subject */
            0,                    /* start at offset 0 in the subject */
            0,                    /* default options */
            match_data,           /* block for storing the result */
            NULL);                /* use default match context */
    if (rc < 0) {
        switch (rc) {
            case PCRE2_ERROR_NOMATCH:
                break;
                /*
                Handle other special cases if you like
                */
            default:
                printf("Matching error %d\n", rc);
                break;
        }
        pcre2_match_data_free(match_data);   /* Release memory used for the match */
        pcre2_code_free(re);                 /* data and the compiled pattern. */
        return 0;
    }
    ovector = pcre2_get_ovector_pointer(match_data);
    *match_offset = (int) ovector[0];


    if (rc == 0) {
        printf("ovector was not big enough for all the captured substrings\n");
    }
    for (i = 0; i < rc; i++) {
        size_t substring_length = ovector[2 * i + 1] - ovector[2 * i];
        *match_str_lenght = (int) substring_length;


        PCRE2_SPTR substring_start = subject + ovector[2 * i];

        if (i > 1) {
            //wrong. i can't large than one.lazy do.
        }
    }
}

void add_word_item_to_single_tree_result(char *tree_result, const char *word_item) {
    strcat(tree_result, ",");
    strcat(tree_result, word_item);
}

//中文裁剪
void chinese_word_cut(char *tree_result_word, char **search_word_point, int *words_weight, const char *chinese_words) {
    strcat(tree_result_word, ",");
    strcat(tree_result_word, chinese_words);
    add_words_weight(words_weight, 2);
    //搜索词前进
    *search_word_point = *search_word_point + strlen(chinese_words);
}

//匹配不到redis词条最后裁剪方法
void not_match_word_end_cut(char *tree_result_word, char **search_word_point, int *words_weight) {
    strcat(tree_result_word, ",");
    strcat(tree_result_word, *search_word_point);
    add_words_weight(words_weight, 1);
    //搜索词前进
    int remain_word_lenght = (int) strlen(*search_word_point);
    *search_word_point = *search_word_point + remain_word_lenght;
}


void get_redis_connect(redisContext **redis_connect) {
    *redis_connect = redisConnect("127.0.0.1", 6379);
    if (redis_connect == NULL || (*redis_connect)->err) {
        if (redis_connect) {
            printf("Error: %s\n", (*redis_connect)->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }
    }
}


/*
 * 裁剪英文作为一个词,
 * 1 STAND FOR SUCCESS,
 * 小于1代表失败
 **/
int english_word_cut(char *tree_result_word, char **search_word_point, int *words_weight) {
    int not_chinese_match_offset, not_chinese_match_str_lenght;
    int pecl_regx_match_result = pecl_regx_match((PCRE2_SPTR) (*search_word_point), (PCRE2_SPTR) english_pattern,
                                                 &not_chinese_match_offset,
                                                 &not_chinese_match_str_lenght);

    //匹配到英文先处理英文
    if (pecl_regx_match_result > 0) {
        add_words_weight(words_weight, 2);
        //取出非中文
        char not_chinese_word[not_chinese_match_str_lenght + 1];
        memset(not_chinese_word, 0, not_chinese_match_str_lenght + 1);
        strncpy(not_chinese_word, *search_word_point, not_chinese_match_str_lenght);
        strcat(not_chinese_word, "\0");
        add_word_item_to_single_tree_result(tree_result_word, not_chinese_word);
        *search_word_point =
                *search_word_point + not_chinese_match_str_lenght;

    }
    return 1;
}

//有多少个词条就开多少个线程来模拟分叉树算法，选出词条最多的一个或几个作为分词结果
int _tree_cut_words(struct TREE_CUT_WORDS_DATA *parent_cut_words_data) {

    english_word_cut((*parent_cut_words_data).cut_word_result, &((*parent_cut_words_data).remainder_words),
                     (*parent_cut_words_data).words_weight);
    //取出第一个汉子，查询redis词条，for循环生产不同的父级分词结果，然后传给新建的子进程,进入递归
    char *first_chinese_word;
    first_chinese_word = (char *) malloc(chinese_char_byte);
    memset(first_chinese_word, 0, chinese_char_byte);
    if (!first_chinese_word) {
        //lazy do.每一个可能的错误都要做处理或日志记录.not only malloc
        printf("Not Enough Memory!/n");
    }

    strncpy(first_chinese_word, parent_cut_words_data->remainder_words, chinese_word_byte);
    strcat(first_chinese_word, "\0");
    char *get_word_item_cmd = join_char(redis_get_common, first_chinese_word);

    //用完释放内存
    free(first_chinese_word);

    redisContext *redis_connect;
    get_redis_connect(&redis_connect);

    redisReply *redis_reply = redisCommand(redis_connect, get_word_item_cmd);
    char *match_words_json_str = redis_reply->str;
    free(get_word_item_cmd);
    cJSON *match_words_json = cJSON_Parse(match_words_json_str);



    //判断是否查找到redis词条
    if (redis_reply->type == REDIS_REPLY_STRING) {
        //pullwords_result_number要先计算好有多少种结果。
        int8_t add_result_number = -1;
        for (int tree_arg_index = 0; tree_arg_index < cJSON_GetArraySize(match_words_json); tree_arg_index++) {
            cJSON *subitem = cJSON_GetArrayItem(match_words_json, tree_arg_index);
            //判断词条是否包含在搜索词里
            if (strstr(parent_cut_words_data->remainder_words, subitem->valuestring) != NULL) {
                add_result_number++;
            }
        };
        printf("add_result_number is %d\n", add_result_number);
        //pullwords_result_number要做原子操作，递增，支持并发
        __sync_fetch_and_add((*parent_cut_words_data).pullwords_result_number, add_result_number);

        for (int tree_arg_index = 0; tree_arg_index < cJSON_GetArraySize(match_words_json); tree_arg_index++) {
            cJSON *subitem = cJSON_GetArrayItem(match_words_json, tree_arg_index);
            //判断词条是否包含在搜索词里,如果在，进入树分词算法
            if (strstr(parent_cut_words_data->remainder_words, subitem->valuestring) != NULL) {

                //复制一份分词结果,生出父级分词结果
                struct TREE_CUT_WORDS_DATA *tree_cut_words_data = (struct TREE_CUT_WORDS_DATA *) malloc(
                        sizeof(struct TREE_CUT_WORDS_DATA));
                memset(tree_cut_words_data, 0, sizeof(struct TREE_CUT_WORDS_DATA));
                init_tree_cut_words_data(tree_cut_words_data);

                strcpy(tree_cut_words_data->cut_word_result, parent_cut_words_data->cut_word_result);
                strcpy(tree_cut_words_data->remainder_words, parent_cut_words_data->remainder_words);
                *tree_cut_words_data->words_weight = *(parent_cut_words_data->words_weight);

                //把pullwords主线程的data封装成一个指针，减少指针创建,lazy do.
                tree_cut_words_data->pullwords_result_queue = parent_cut_words_data->pullwords_result_queue;
                tree_cut_words_data->pullwords_result_queue_write_lock = parent_cut_words_data->pullwords_result_queue_write_lock;
                tree_cut_words_data->pullwords_cond = parent_cut_words_data->pullwords_cond;
                (*tree_cut_words_data).pullwords_result_number = (*parent_cut_words_data).pullwords_result_number;
                (*tree_cut_words_data).pullwords_result_finish_number = (*parent_cut_words_data).pullwords_result_finish_number;

                chinese_word_cut(tree_cut_words_data->cut_word_result, &(tree_cut_words_data->remainder_words),
                                 tree_cut_words_data->words_weight, subitem->valuestring);

                //还有词没分完，进入递归分词
                if (strlen(tree_cut_words_data->remainder_words) > 0) {
                    pthread_t pthread_id;
                    int pthread_create_result;
                    pthread_create_result = pthread_create(&pthread_id, NULL,
                                                           (void *) _tree_cut_words, tree_cut_words_data);
                    if (pthread_create_result != 0) {
                        //deal the error,lazy do.
                        printf("Create pthread error!\n");
                    }
                } else {
                    //创建线程最终分词结果，写入主线程队列
                    struct TREE_CUT_WORDS_RESULT *last_tree_cut_words_result = (struct TREE_CUT_WORDS_RESULT *) malloc(
                            sizeof(struct TREE_CUT_WORDS_RESULT));
                    memset(last_tree_cut_words_result, 0, sizeof(struct TREE_CUT_WORDS_RESULT));
                    strcpy(last_tree_cut_words_result->pullwords_result, tree_cut_words_data->cut_word_result);
                    last_tree_cut_words_result->words_weight = *(tree_cut_words_data->words_weight);

                    //原子操作,递增完成作业数
                    __sync_fetch_and_add((*parent_cut_words_data).pullwords_result_finish_number, 1);

                    //分词结束，加锁，写入分词结果到主线程的分词队列
                    pthread_mutex_lock(tree_cut_words_data->pullwords_result_queue_write_lock);
                    printf("pullwords_result_number is %d,pullwords_result_finish_number %d\n",
                           *((*parent_cut_words_data).pullwords_result_number),
                           *((*parent_cut_words_data).pullwords_result_finish_number));
                    EnQueue((*tree_cut_words_data).pullwords_result_queue, last_tree_cut_words_result);


                    //加锁比较pullwords_result_finish_number跟pullwords_result_number，如果全部作业完成，通知主进程
                    if (*((*parent_cut_words_data).pullwords_result_finish_number) ==
                        *((*parent_cut_words_data).pullwords_result_number)) {
                        pthread_cond_signal((*parent_cut_words_data).pullwords_cond);
                    }
                    pthread_mutex_unlock(tree_cut_words_data->pullwords_result_queue_write_lock);
                }
            }

        }
    } else {
        not_match_word_end_cut(parent_cut_words_data->cut_word_result, &(parent_cut_words_data->remainder_words),
                               parent_cut_words_data->words_weight);
        //创建线程最终分词结果，写入主线程队列
        struct TREE_CUT_WORDS_RESULT *last_tree_cut_words_result = (struct TREE_CUT_WORDS_RESULT *) malloc(
                sizeof(struct TREE_CUT_WORDS_RESULT));
        memset(last_tree_cut_words_result, 0, sizeof(struct TREE_CUT_WORDS_RESULT));
        strcpy(last_tree_cut_words_result->pullwords_result, parent_cut_words_data->cut_word_result);
        last_tree_cut_words_result->words_weight = *(parent_cut_words_data->words_weight);

        //原子操作,递增完成作业数
        __sync_fetch_and_add((*parent_cut_words_data).pullwords_result_finish_number, 1);

        //分词结束，加锁，写入分词结果到主线程的分词队列
        pthread_mutex_lock(parent_cut_words_data->pullwords_result_queue_write_lock);


        EnQueue((*parent_cut_words_data).pullwords_result_queue, last_tree_cut_words_result);
        printf("pullwords_result_number is %d,pullwords_result_finish_number %d\n",
               *((*parent_cut_words_data).pullwords_result_number),
               *((*parent_cut_words_data).pullwords_result_finish_number));
        //加锁比较pullwords_result_finish_number跟pullwords_result_number，如果全部作业完成，通知主进程
        if (*((*parent_cut_words_data).pullwords_result_finish_number) ==
            *((*parent_cut_words_data).pullwords_result_number)) {
            pthread_cond_signal((*parent_cut_words_data).pullwords_cond);
        }
        pthread_mutex_unlock(parent_cut_words_data->pullwords_result_queue_write_lock);

    }

    free(parent_cut_words_data->cut_word_result);
    free(parent_cut_words_data->words_weight);
    free(parent_cut_words_data->origin_remainder_words);
    //remainder_words貌似已经做了偏移运算，不能free掉
    //free(parent_cut_words_data->remainder_words);
    free(parent_cut_words_data);


    return 0;
}

/*
 * 处理分词队列函数
 * */
void deal_pullwords_result_queue(LinkQueue *pullwords_result_queue, char *result_words) {
    //从队首至队尾遍历队列Q中的元素
    QueuePtr p;
    p = (*pullwords_result_queue).front->next;
    if (p == NULL) {
        printf("QueuePtr ERROR \n");
    }

    char temporary_result_words[MAX_SEARCH_WORDS_LEN];
    memset(temporary_result_words, 0, MAX_SEARCH_WORDS_LEN);
    int temporary_words_weight = 0;
    while (p != NULL) {//遍历队
        if (p->data->words_weight > temporary_words_weight) {
            temporary_words_weight = p->data->words_weight;
            strcpy(temporary_result_words, p->data->pullwords_result);
        }
        p = p->next;
    }
    strcpy(result_words, &temporary_result_words);
}

/*
 * result_code 1:pull word success
 * result_code 0:pull word fail
 * */
int pull_word(char *origin_search_words, int * connfd) {

    pthread_mutex_t pull_word_cond_mtx = PTHREAD_MUTEX_INITIALIZER;
    //新建一个条件锁，等待被触发
    pthread_cond_t pullwords_cond = PTHREAD_COND_INITIALIZER;

    char option;
    LinkQueue *pullwords_result_queue = (LinkQueue *) malloc(sizeof(LinkQueue));
    memset(pullwords_result_queue, 0, sizeof(LinkQueue));
    InitQueue(pullwords_result_queue);
    //每处理一个分词请求建立一个写入锁，防止覆盖，跟多个分词请求的相互干扰
    pthread_mutex_t pullwords_result_queue_write_lock = PTHREAD_MUTEX_INITIALIZER;



    //定义父级树分词结果
    struct TREE_CUT_WORDS_DATA *parent_tree_cut_words_data = (struct TREE_CUT_WORDS_DATA *) malloc(
            sizeof(struct TREE_CUT_WORDS_DATA));
    init_tree_cut_words_data(parent_tree_cut_words_data);
    //封装成一个函数，start
    parent_tree_cut_words_data->pullwords_result_queue = pullwords_result_queue;
    parent_tree_cut_words_data->pullwords_result_queue_write_lock = &pullwords_result_queue_write_lock;
    parent_tree_cut_words_data->pullwords_cond = &pullwords_cond;
    (*parent_tree_cut_words_data).pullwords_result_number = (int *) malloc(sizeof(int8_t));
    memset((*parent_tree_cut_words_data).pullwords_result_number, 0, sizeof(int8_t));
    (*parent_tree_cut_words_data).pullwords_result_finish_number = (int *) malloc(sizeof(int8_t));
    memset((*parent_tree_cut_words_data).pullwords_result_finish_number, 0, sizeof(int8_t));
    *((*parent_tree_cut_words_data).pullwords_result_number) = 1;
    *((*parent_tree_cut_words_data).pullwords_result_finish_number) = 0;
    //封装成一个函数，end

    strcpy(parent_tree_cut_words_data->remainder_words, "iphoneioioioioioiaaaaa飞行员A9墨镜");

    //加锁
    pthread_mutex_lock(&pull_word_cond_mtx);

    //新开一个线程进行分词，每一个子分词线程都继承父级线程分词的结果
    int pthread_create_result;
    pthread_t parent_thread_id;
    pthread_create_result = pthread_create(&parent_thread_id, NULL,
                                           (void *) _tree_cut_words, parent_tree_cut_words_data);

    //阻塞等待子进程通知
    pthread_cond_wait(&pullwords_cond, &pull_word_cond_mtx);

    char last_result_words[WORD_RESULT_LEN];
    memset(last_result_words,0,WORD_RESULT_LEN);
    //接受到通知开始处理分词作业检查
    //deal_pullwords_result_queue(pullwords_result_queue, last_result_words);
    strcat(last_result_words,"ok\n");
    printf("last result_words is %s\n", last_result_words);
    free(pullwords_result_queue);
    /*这种不会导致客户端断开
    char respond_word[5] = "ok\n";
    write(connfd, respond_word, 5); //write maybe fail,here don't process failed error
    */
    //这种会导致客户端断开
    write(*connfd, last_result_words, sizeof(last_result_words)); //write maybe fail,here don't process failed error

    return 0;
};

void deal_pullwords_request(int connfd) {

    size_t n;
    int8_t result_code;
    char search_words[MAX_SEARCH_WORDS_LEN];
    memset(search_words, 0, MAX_SEARCH_WORDS_LEN);
    for (; ;) {
        n = read(connfd, search_words, MAX_SEARCH_WORDS_LEN);

        if (n < 0) {
            if (errno != EINTR) {
                perror("read error");
                break;
            }
        }
        if (n == 0) {
            //connfd is closed by client
            close(connfd);
            printf("client exit\n");
            break;
        }
        //client exit
        if (strncmp("exit", search_words, 4) == 0) {
            close(connfd);
            printf("client exit\n");
            break;
        }

        printf("client input %s\n", search_words);
        pull_word(search_words, connfd);

        //char respond_word[5] = "ok\n";
        //write(connfd, respond_word, n); //write maybe fail,here don't process failed error
    }
}

int main(int argc, char **argv) {
    /*
    char *search_words = (char *) malloc(MAX_SEARCH_WORDS_LEN);
    int8_t result_code;
    memset(search_words, 0, MAX_SEARCH_WORDS_LEN);
    char *result_words = (char *) malloc(MAX_SEARCH_WORDS_LEN);
    memset(result_words, 0, MAX_SEARCH_WORDS_LEN);
    pull_word(search_words, result_words, &result_code);
    sleep(100);
   */
    int listenfd, connfd;
    int serverPort = 9999;
    int listenq = 1024;
    pid_t childpid;
    char buf[MAX_SEARCH_WORDS_LEN];
    socklen_t socklen;

    struct sockaddr_in cliaddr, servaddr;
    socklen = sizeof(cliaddr);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serverPort);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket error");
        return -1;
    }

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));


    if (bind(listenfd, (struct sockaddr *) &servaddr, socklen) < 0) {
        perror("bind error");
        return -1;
    }
    if (listen(listenfd, listenq) < 0) {
        perror("listen error");
        return -1;
    }

    printf("echo server startup,listen on port:%d\n", serverPort);
    for (; ;) {
        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &socklen);
        if (connfd < 0) {
            perror("accept error");
            continue;
        }

        sprintf(buf, "accept form %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
        printf(buf, "");
        childpid = fork();
        if (childpid == 0) { /* child process */
            close(listenfd);    /* close listening socket */
            deal_pullwords_request(connfd);   /* process the request */
            exit(0);
        } else if (childpid > 0) {
            close(connfd);          /* parent closes connected socket */
        } else {
            perror("fork error");
        }
    }
}
