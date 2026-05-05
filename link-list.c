#include <stdlib.h>
#include <stdio.h>

typedef struct Node{
    void *data;
    struct Node *next;
} Node;

typedef struct LinkList{
    Node *head;
    Node *tail;
    int size;
} LinkList;

LinkList* list_init(){
    LinkList *list = (LinkList *)malloc(sizeof(LinkList));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void list_add(LinkList *list, void *data){
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    if(list->head == NULL) {
        list->head = newNode;
        list->tail = newNode;
    } else {
        list->tail->next = newNode;
        list->tail = newNode;
    }
    list->size += 1;
}

typedef void (*PrintDataFunc)(void *);

void my_int_printer(void *data) {
    printf("%d", *(int *)data);
}

void my_string_printer(void *data) {
    printf("%s", (char *)data);
}

void my_float_printer(void *data) {
    printf("%f", *(float *)data);
}

int* new_int(int value) {
    int *p = malloc(sizeof(int));
    if (p) *p = value; // 養成習慣：檢查 malloc 是否成功
    return p;
}

void list_show(LinkList *list, PrintDataFunc print_func){
    Node *cur = list->head;
    int count = 0;
    while(cur != NULL){
        count += 1;
        printf("Node #%d: ", count);
        print_func(cur->data); 
        printf("\n");
        cur = cur->next;
    }
}

void* list_pop_front(LinkList *list){
    if (list->head == NULL) return NULL;
    Node *temp = list->head;
    void *data = temp->data;

    list->head = list->head->next;

    if (list->head == NULL) {
        list->tail = NULL;
    }
    free(temp);
    list->size--;
    return data;
}

void list_destroy(LinkList *list) {
    Node *cur = list->head;
    while (cur != NULL) {
        Node *next = cur->next;
        if (cur->data) free(cur->data);

        free(cur);
        cur = next;
    }
    free(list);
}

void list_reverse(LinkList *list){
    Node *cur = list->head;
    Node *pre = NULL;
    list->head = list->tail;
    list->tail = cur;
    while (cur != NULL) {
        Node *next = cur->next;
        cur->next = pre;
        pre = cur;
        cur = next;
    }
}

int main(){
    LinkList *list = list_init();

    printf("This is a link list\n");

    list_add(list, new_int(8));
    list_add(list, new_int(7));
    list_add(list, new_int(6));
    list_add(list, new_int(5));
    list_add(list, new_int(4));
    list_add(list, new_int(3));
    list_add(list, new_int(2));
    list_add(list, new_int(1));
    printf("The head is %d\n", *(int *)list->head->data);
    list_show(list, my_int_printer);

    printf("Pop list\n");
    void *popped_data = list_pop_front(list);
    if (popped_data) {
        free(popped_data); 
    }
    list_show(list, my_int_printer);
    list_reverse(list);
    printf("After reverse\n");
    list_show(list, my_int_printer);
    getchar();
    return 0;
}