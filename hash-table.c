#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "list.h"

typedef int (*hash_func_t)(const void *key);
typedef int (*hash_entry_func_t)(const void *struct_data);
typedef int (*key_compare_t)(void *key, const void *struct_data);

// 定義 Callback 格式：使用者會拿到節點指標與一個自定義的參數
typedef void (*hash_iter_t)(struct list_head *node, void *priv);

static inline uint32_t roundup_pow_of_two(uint32_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

struct hash_table {
    struct list_head *buckets; // 動態陣列，每個元素都是 list_head 
    int bucket_count;          // 桶子數量
    int count;                 // 目前元素數量
    hash_func_t hash_fn;       // Hash 演算法
    key_compare_t compare_fn;  // Key 比對演算法
    int node_offset;           // node偏移量，用於推算link list
    int key_offset;            // key偏移量，用於推算key
};

void hash_init(struct hash_table *table, int buckets, hash_func_t h, key_compare_t c, int node_offset, int key_offset) {
    table->bucket_count = roundup_pow_of_two(buckets);;
    table->hash_fn = h;
    table->compare_fn = c;
    table->buckets = malloc(sizeof(struct list_head) * table->bucket_count);
    table->node_offset = node_offset;
    table->key_offset = key_offset;

    for(int i =0; i < table->bucket_count; i ++) {
        INIT_LIST_HEAD(&table->buckets[i]);
    }
    table->count = 0;
}

static inline void *node_to_entry(const struct hash_table *table, const struct list_head *node) {
    if (!node) return NULL;
    // 將 node 指標轉成 char* 進行精準到 Byte 的減法，減去位移量後，就是大結構體的起點
    return (void *)((char *)node - table->node_offset);
}

static inline struct list_head *entry_to_node(const struct hash_table *table, const void *entry) {
    if (!entry) return NULL;
    return (struct list_head *)((char *)entry + table->node_offset);
}

static inline void *entry_to_key(const struct hash_table *table, const void *entry) {
    if (!entry) return NULL;
    return (void *)((char *)entry + table->key_offset);
}


struct list_head *hash_find(struct hash_table *table, void *key) {
    int idx = table->hash_fn(key) & (table->bucket_count - 1);
    struct list_head *pos;
    list_for_each(pos, &table->buckets[idx]) {
        void *user_data = node_to_entry(table, pos);
        if (table->compare_fn(key, user_data) == 0) {
            return pos;
        }
    }
    return NULL;
}

struct list_head *hash_add(struct hash_table *table, void* key, struct list_head *new_node) {
    struct list_head *old_node = hash_find(table, key);
    if (old_node) {
        list_replace(old_node, new_node);
        return old_node;
    }

    int idx = table->hash_fn(key) & (table->bucket_count - 1);
    list_add(new_node, &table->buckets[idx]);
    table->count++;
    return NULL;
}

void hash_delete(struct hash_table *table, void* key) {
    struct list_head *node = hash_find(table, key);
    if (node) {
        list_del(node);
        table->count--;
    }
}

int hash_size(struct hash_table *table) {
    return table->count;
}

float hash_load_factor(struct hash_table *table) {
    return (float)table->count / table->bucket_count;
}

void hash_stats(struct hash_table *table) {
    int empty_buckets = 0;
    int max_chain = 0;
    for (int i = 0; i < table->bucket_count; i++) {
        int chain_len = 0;
        struct list_head *pos;
        if (list_empty(&table->buckets[i])) {
            empty_buckets++;
            continue;
        }
        list_for_each(pos, &table->buckets[i]) {
            chain_len++;
        }
        if (chain_len > max_chain) max_chain = chain_len;
    }

    printf("--- Hash Table Stats ---\n");
    printf("Total Elements: %d\n", table->count);
    printf("Bucket Count: %d\n", table->bucket_count);
    printf("Load Factor%.2f\n", hash_load_factor(table));
    printf("Empty Buckets: %d\n", empty_buckets);
    printf("Max Chain Length: %d\n", max_chain);
    printf("------------------------\n");
}

void hash_resize(struct hash_table *table) {
    int old_bucket_count = table->bucket_count;
    int new_bucket_count = old_bucket_count * 2;
    struct list_head *new_buckets = malloc(sizeof(struct list_head) * new_bucket_count);
    struct list_head *old_buckets = table->buckets;
    int old_count = table->count;

    if (!new_buckets) return; // 記憶體配置失敗防禦

    for (int i = 0; i < new_bucket_count; i++) {
        INIT_LIST_HEAD(&new_buckets[i]);
    }

    for (int i = 0; i < old_bucket_count; i++) {
        struct list_head *pos, *n;
        list_for_each_safe(pos, n, &old_buckets[i]) {
            void *user_data = node_to_entry(table, pos);
            void *key = entry_to_key(table, user_data);
            int new_idx = table->hash_fn(key) & (new_bucket_count - 1);
            list_del(pos);
            list_add(pos, &new_buckets[new_idx]);
        }
    }

    table->buckets = new_buckets;
    table->bucket_count = new_bucket_count;
    free(old_buckets);
}

void hash_foreach(struct hash_table *table, hash_iter_t iter_fn, void *priv) {
    for (int i = 0; i < table->bucket_count; i++) {
        struct list_head *pos, *n;
        list_for_each_safe(pos, n, &table->buckets[i]) {
            iter_fn(pos, priv);
        }
    }
}

void hash_clear(struct hash_table *table, void (*free_fn)(struct list_head *)) {
    for (int i = 0; i < table->bucket_count; i++) {
        struct list_head *pos, *n;
        list_for_each_safe(pos, n, &table->buckets[i]) {
            list_del(pos);
            table->count--;
            if (free_fn) free_fn(pos);
        }
    }
}

struct user_node {
    int uid;                // 這是我們的 Key
    char name[32];
    struct list_head node;  // 侵入式鏈表節點
};

int my_hash_fn(const void *key) {
    int id = *(int *)key;
    return id; // 這裡直接回傳，之後 Library 會自己 & (count - 1)
}

int my_compare_fn(void *key, const void *struct_data) {
    int id = *(int *)key;
    const struct user_node *user = (const struct user_node *)struct_data;
    return (user->uid == id) ? 0 : 1;
}

void my_free_user_fn(struct list_head *node) {
    struct user_node *u = list_entry(node, struct user_node, node);
    free(u);
}



void print_menu() {
    printf("\n--- Hash Table Lab ---\n");
    printf("1. Add User (ID Name)\n");
    printf("2. Find User (ID)\n");
    printf("3. Delete User (ID)\n");
    printf("4. Show All Buckets (Debug)\n");
    printf("5. Show Table Stats (統計數據)\n"); // 🛠️ 新增：開放統計數據功能
    printf("6. Force Resize (手動擴容)\n");     // 🛠️ 新增：開放手動擴容測試
    printf("7. Clear Table\n");
    printf("8. Exit\n");
    printf("Selection: ");
}

int main() {
    struct hash_table table;
    // 初始開 4 個桶子就好，這樣資料塞多一點才能明顯看到觸發 Resize
    hash_init(&table, 4, my_hash_fn, my_compare_fn, offsetof(struct user_node, node), offsetof(struct user_node, uid));

    int choice, id;
    char name[32];

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) break;

        switch (choice) {
            case 1:
                printf("Enter ID and Name: ");
                scanf("%d %s", &id, name);
                
                struct user_node *new_user = malloc(sizeof(struct user_node));
                new_user->uid = id;
                strncpy(new_user->name, name, 32);
                
                struct list_head *old = hash_add(&table, &new_user->uid, &new_user->node);
                if (old) {
                    struct user_node *old_user = list_entry(old, struct user_node, node);
                    printf("[Update] Replaced old user: %s\n", old_user->name);
                    free(old_user);
                } else {
                    printf("[Success] Added new user.\n");
                    
                    // 🛠️ 亮點測試：自動擴容機制！
                    // 當 Load Factor（裝載因子）大於 0.75 時，自動觸發搬家，這才是標準資料結構的行為！
                    if (hash_load_factor(&table) > 0.75) {
                        printf("\n[System] Load factor (%.2f) > 0.75! Triggering auto-resize...\n", hash_load_factor(&table));
                        hash_resize(&table);
                        printf("[System] Auto-resize completed. New bucket count: %d\n", table.bucket_count);
                    }
                }
                break;

            case 2:
                printf("Enter ID to find: ");
                scanf("%d", &id);
                struct list_head *found = hash_find(&table, &id);
                if (found) {
                    struct user_node *u = list_entry(found, struct user_node, node);
                    printf("[Found] ID: %d, Name: %s\n", u->uid, u->name);
                } else {
                    printf("[Error] User not found.\n");
                }
                break;

            case 3:
                printf("Enter ID to delete: ");
                scanf("%d", &id);
                struct list_head *removed = hash_find(&table, &id);
                if (removed) {
                    list_del(removed);
                    table.count--; // ⚠️ 修正：之前手動 list_del 忘記幫 table->count 減一了
                    struct user_node *u = list_entry(removed, struct user_node, node);
                    printf("[Deleted] User: %s\n", u->name);
                    free(u);
                } else {
                    printf("[Error] Nothing to delete.\n");
                }
                break;

            case 4:
                printf("\n--- Hash Table Internal Structure ---\n");
                for (int i = 0; i < table.bucket_count; i++) {
                    printf("Bucket [%d]: ", i);
                    struct list_head *pos;
                    list_for_each(pos, &table.buckets[i]) {
                        struct user_node *u = list_entry(pos, struct user_node, node);
                        printf("(%d:%s) -> ", u->uid, u->name);
                    }
                    printf("NULL\n");
                }
                break;

            case 5: // 🛠️ 新增測試功能：印出效能數據
                hash_stats(&table);
                break;

            case 6: // 🛠️ 新增測試功能：測試強制手動 Resize 搬家
                printf("Triggering manual resize...\n");
                hash_resize(&table);
                printf("Resize finish. Current buckets: %d\n", table.bucket_count);
                break;

            case 7:
                printf("Clearing...\n");
                hash_clear(&table, my_free_user_fn);
                printf("Clear finish...\n");
                break;

            case 8:
                // 離開前養成好習慣，把整張表清空，防止 Memory Leak
                hash_clear(&table, my_free_user_fn);
                free(table.buckets);
                printf("Exiting...\n");
                return 0;

            default:
                printf("Invalid choice.\n");
        }
    }
    return 0;
}