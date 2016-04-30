//单列链队的使用
//作者：nuaazdh
//时间：2011年12月9日
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <stdlib.h>
#include "unidirectional_queue.h"

//-------------交互处理-----------------//
void PrintMenu(){
	//显示菜单提示
	printf("------Welcome------\n");
	printf(" Please input your choice:\n");
	printf(" E: New element enqueue.\n");
	printf(" D: Delete an element from the Queue.\n");
	printf(" S: Show all elements in the Queue.\n");
	printf(" H: Get the first element in the Queue.\n");
	printf(" L: Get the length of the Queue.\n");
	printf(" Q: Quit.\n");
	printf(" ohters: do nothing.\n");
}

void ShowBye(){
	//显示结束
	printf("------Bye bye------\n");
}

char getOption(){
	//获取用户输入
	char *input,*op;
	input=(char *)malloc(sizeof(char)*256);
	printf(">>");
	gets(input);
	op=input;
	if(*(op+1)!='\0'){
		return 'A';
	}
	else
		return toupper(*op);
}

void NewNodeEnQueue(LinkQueue *Q){
	//根据用户输入，将新元素入队
	char *p;
	p=(char *)malloc(sizeof(char)*256);
	printf("Please input a char(only one):");
	gets(p);
	if((*p>='a')&&(*p<='z')||(*p>='A')&&(*p<='Z')){
		EnQueue(Q,*p);
		printf("New element \'%c\' enqueu.\n",*p);
	}else
		printf("Sorry, your input is not accepted.\n");
}

void DeleteNode(LinkQueue *Q){
	//元素出队，并显示剩余队中元素
	QElemType e;
	if(TRUE==QueueEmpty(*Q)){
		printf("The Queue is empty.\n");
		return;
	}
	DeQueue(Q,&e);
	printf("%c delete from the Queue,",e);
	if(QueueEmpty(*Q)==TRUE)
		printf("The Queue is empty now.\n");
	else
		//printf("Current Queue is:");
		QueueTraverse(*Q);

}

void ShowHeadNode(LinkQueue *Q){
	//显示队首元素
	QElemType e;
	if(QueueEmpty(*Q)){
		printf("The Queue is empty.\n");
		return;
	}else{
		GetHead(*Q,&e);
		printf("The first element of the Queue is:%c\n",e);
	}
}

void ShowLength(LinkQueue *Q){
	//显示队列元素个数
	if(QueueEmpty(*Q)==TRUE){
		printf("The count of element is 0.\n");
	}else{
		printf("The count of element is：%d\n",QueueLength(*Q));
	}
}

//-------------队操作实现----------------//
Status InitQueue(LinkQueue *Q){
	//构造一个空队列Q
	Q->front = Q->rear = (QueuePtr) malloc(sizeof(QNode));
	if(Q->front==NULL){
		printf("Apply for memory failed.\n");
		exit(0);
	}
	Q->rear->next=NULL;
	return OK;
}

Status DestroyQueue(LinkQueue *Q){
	//销毁队列Q，Q不再存在
	while(Q->front){
		Q->rear=Q->front->next;
		free(Q->front);
		Q->front=Q->rear;
	}
	Q->front=Q->rear=NULL;
	return OK;
}

Status ClearQueue(LinkQueue *Q){
	//将Q清为空队列
	QueuePtr p;  //临时指针
	Q->rear=Q->front->next;
	while(Q->rear){
		p=Q->rear->next;//指向下一个待释放的元素
		free(Q->rear);
		Q->rear=p;
	}
	Q->rear=Q->front;//修改队尾指针
	return OK;
}

Status QueueEmpty(LinkQueue Q){
	//若队列Q为空队列，则返回TRUE,否则返回FALSE
	if(Q.front==Q.rear&&Q.front!=NULL)
		return TRUE;
	else
		return FALSE;

}

int QueueLength(LinkQueue Q){
	//返回队列Q元素个数，即队列长度
	QueuePtr p;//临时指针
	int count=0;//计算器
	p=Q.front->next;
	while(p!=NULL){
		p=p->next;
		count++;
	}
	return count;
}

Status GetHead(LinkQueue Q,QElemType *e){
	//若队列不为空，则用e返回Q的队首元素，并返回OK；否则返回ERROR
	if(QueueEmpty(Q)==TRUE)//队列为空
		return ERROR;
	*e=Q.front->next->data;
	return OK;
}

Status EnQueue(LinkQueue *Q,QElemType e){
	//插入元素e为Q的新的队尾元素
	QueuePtr p;
	if(Q->front==NULL||Q->rear==NULL)
		return ERROR;
	p=(QueuePtr)malloc(sizeof(QNode));
	if(!p){
		printf("Apply for memory error, element enqueue failed.\n");
		exit(0);
	}
	p->data=e;
	p->next=NULL;
	Q->rear->next=p;
	Q->rear=p;
	return OK;
}

Status DeQueue(LinkQueue *Q,QElemType *e){
	//若队列不为空，则删除Q的队首元素，用e返回，并返回OK；否则返回ERROR
	QueuePtr p;
	if(Q->front==Q->rear)//对为空
		return ERROR;
	p=Q->front->next;
	*e=p->data;
	Q->front->next=p->next;
	if(Q->rear==p)//队中只有一个元素
		Q->rear=Q->front;
	free(p);
	return OK;

}

Status QueueTraverse(LinkQueue Q){
	//从队首至队尾遍历队列Q中的元素
	QueuePtr p;
	p=Q.front->next;
	if(p==NULL){
		return ERROR;
	}
	printf("Elements of the Queue:");
	while(p!=NULL){//遍历队
		printf("%c ",p->data);
		p=p->next;
	}
	printf("\n");
	return OK;
}