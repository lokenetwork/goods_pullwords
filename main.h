//
// Created by root on 4/30/16.
//
#ifndef PULLWORDS_MAIN_H
#define PULLWORDS_MAIN_H
//http包头长度
#define HTTP_REQUEST_LEN 1500
//分词结果的最大长度
#define WORD_RESULT_LEN 200
//可处理的最大的客户端发送的搜索词长度
#define MAX_SEARCH_WORDS_LEN 200
//最多开多少个线程处理分词
#define MAX_CUT_WORD_THREAD_NUMBER 30
//各线程分词结果结构体
struct TREE_CUT_WORDS_RESULT {
    //分词结果
    char pullwords_result[MAX_SEARCH_WORDS_LEN];
    //分词权重
    int words_weight;
};
#define printd(fmt,args) printf(__FILE__ "(%d): " fmt, __LINE__, ##args);
#endif //PULLWORDS_MAIN_H
