//单列链队的使用
//作者：nuaazdh
//时间：2011年12月9日
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <stdlib.h>
#include "../main.h";

#define OK 1
#define ERROR 0
#define TRUE 1
#define FALSE 0

typedef int Status;
typedef struct TREE_CUT_WORDS_RESULT * QElemType;

typedef struct QNode{//队列元素结构
	QElemType data;
	struct QNode* next;
}QNode,*QueuePtr;

typedef struct{//链队结构
	QueuePtr front;//队首指针
	QueuePtr rear;//队尾指针
}LinkQueue;

Status InitQueue(LinkQueue *Q);
//构造一个空队列Q
Status DestroyQueue(LinkQueue *Q);
//销毁队列Q，Q不再存在
Status ClearQueue(LinkQueue *Q);
//将Q清为空队列
Status QueueEmpty(LinkQueue Q);
//若队列Q为空队列，则返回TRUE,否则返回FALSE
int QueueLength(LinkQueue Q);
//返回队列Q元素个数，即队列长度
Status GetHead(LinkQueue Q,QElemType *e);
//若队列不为空，则用e返回Q的队首元素，并返回OK；否则返回ERROR
Status EnQueue(LinkQueue *Q,QElemType e);
//插入元素e为Q的新的队尾元素
Status DeQueue(LinkQueue *Q,QElemType *e);
//若队列不为空，则删除Q的队首元素，用e返回，并返回OK；否则返回ERROR
Status QueueTraverse(LinkQueue Q);
//从队首至队尾遍历队列Q中的元素

void PrintMenu();
//显示菜单提示
char getOption();
//获取用户输入
void NewNodeEnQueue(LinkQueue *Q);
//根据用户输入，将新元素入队
void DeleteNode(LinkQueue *Q);
//元素出队，并显示剩余队中元素
void ShowHeadNode(LinkQueue *Q);
//显示队首元素
void ShowBye();
//显示结束
void ShowLength(LinkQueue *Q);
//显示队列元素个数
//=============main()================//
