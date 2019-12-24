/* ECE-357 2019 Fall (Problem Set#6)
Problem 2~5: slab allocator
author: Di (David) Mei
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#define BUF_SIZE 4096
#define NUM_CORES 6
#define ITER_NUM 1000000
#define NSLOTS 20

int tas(volatile char *lock);

struct dll {
    int value;
    struct dll *fwd, *rev;
    char dllock;
};

struct slab {
    char freemap[NSLOTS];
    char slablock;
    struct dll slots[NSLOTS];
};

void spin_lock(char *lock)
{
    while(tas(lock) != 0)
        ;
}

void spin_unlock(char *lock)
{
    *lock = 0;
}

void *slab_alloc(struct slab *slab){
    spin_lock(&(slab->slablock));
    // anchor slot is assumed to be pre-allocated in slots[0]
    for (int i = 1; i < NSLOTS; i++){ 
        if (slab->freemap[i] == (char)0){
            slab->freemap[i] = (char)1;
            spin_unlock(&(slab->slablock));
            return &(slab->slots[i]);
        }
    }
    spin_unlock(&(slab->slablock));
    return NULL;
}

int slab_dealloc(struct slab *slab, void *object){
    spin_lock(&(slab->slablock));
    for (int i = 1; i < NSLOTS; i++){
        if (&(slab->slots[i]) == (struct dll *)object){
            if (slab->freemap[i] == (char)0){
                spin_unlock(&(slab->slablock));
                return -1;
            }
            else{
                slab->freemap[i] = (char)0;
                spin_unlock(&(slab->slablock));
                return 1;
            }
        }
    }
    spin_unlock(&(slab->slablock));
    return -1;
}

struct dll *dll_insert(struct dll *anchor, int value, struct slab *slab){
    spin_lock(&(anchor->dllock)); // dllock
    fprintf(stderr, "process(%d) insert %d\n", getpid(), value);
    struct dll *new_dll = (struct dll *)slab_alloc(slab);
    if (new_dll == NULL){
        fprintf(stderr, "process(%d) fails to allocate a slot for %d\n", getpid(), value);
        spin_unlock(&(anchor->dllock)); // dllunlock
        return NULL;
    }
    new_dll->value = value;
    struct dll *node = anchor;
    for (int i = 1; i < NSLOTS; i++){
        if(node->fwd == anchor){
            break;
        }
        if(new_dll->value <= node->fwd->value){
            new_dll->fwd = node->fwd;
            new_dll->rev = node;
            node->fwd->rev = new_dll;
            node->fwd = new_dll;
            spin_unlock(&(anchor->dllock)); // dllunlock
            return new_dll;
        }
        node = node->fwd;
    }
    new_dll->fwd = anchor;
    new_dll->rev = node;
    anchor->rev = new_dll;
    node->fwd = new_dll;
    spin_unlock(&(anchor->dllock)); // dllunlock
    return new_dll;
}

void dll_delete(struct dll *anchor, struct dll *node, struct slab *slab){
    spin_lock(&(anchor->dllock)); // dllock
    fprintf(stderr, "process(%d) try to delete %d\n", getpid(), node->value);
    int unlock = slab_dealloc(slab, node);
    if (unlock == -1){
        fprintf(stderr, "process(%d) fails to deallocate a slot for %d\n", getpid(), node->value);
        spin_unlock(&(anchor->dllock)); // dllunlock
        return;
    }
    node->rev->fwd = node->fwd;
    node->fwd->rev = node->rev;
    spin_unlock(&(anchor->dllock)); // dllunlock
}

struct dll *dll_find(struct dll *anchor,int value){
    spin_lock(&(anchor->dllock)); // dllock
    struct dll *node = anchor->fwd;
    for (int i = 1; i < NSLOTS; i++){
        if (value < node->value || node == anchor){
            break;
        }
        else if (node->value == value){
            fprintf(stderr, "process(%d) find node with value %d\n", getpid(), value);
            spin_unlock(&(anchor->dllock)); // dllunlock
            return node;
        }
        node = node->fwd;
    }
    fprintf(stderr, "process(%d) cannot find node with value %d\n", getpid(), value);
    spin_unlock(&(anchor->dllock)); // dllunlock
    return NULL;
}

int main(){
    struct slab *map;
    if ((map = (struct slab *)mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
        fprintf(stderr, "Error: unable to create a shared memory (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    map->slablock = 0;
    for (int i = 0; i < NSLOTS; i++){
        map->freemap[i] = (char)0;
    }
    // anchor (slots[0]) is assumed to be pre-allocated in slots[0]
    struct dll *anchor = &(map->slots[0]);
    map->freemap[0] = (char)1;
    anchor->dllock = 0;
    anchor->value = 0;
    anchor->fwd = anchor;
    anchor->rev = anchor;

    for (int i = 0; i < NUM_CORES; i++){
        int pid = fork();
        if (pid == 0){
            for (int j = 0; j < ITER_NUM; j++){
                int rand_func = rand() % 3 + 1; 
                int rand_val = rand() % NSLOTS + 1; 
                if (rand_func == 1){
                    struct dll *node = dll_insert(anchor, rand_val, map);
                }
                else if (rand_func == 2){
                    dll_delete(anchor, &(map->slots[rand_val-1]), map);
                }
                else{
                    struct dll *node = dll_find(anchor, rand_val);
                }
            }
            exit(EXIT_SUCCESS);
        }
        else if (pid == -1){
            fprintf(stderr, "Error: unable to fork the process (%s)", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < NUM_CORES; i++){
        wait(NULL);
    }

    // debug: used to check last updated values of slots in the cicular linked list
    // whether printed values are in an increasing order
    fprintf(stderr, "last updated values of slots in circular linked list:\n");
    struct dll *node = anchor->fwd;
    for (int i = 1; i < NSLOTS; i++){
        if (node == anchor){
            break;
        }
        fprintf(stderr, "value: %d\n", node->value);
        node = node->fwd;
    }

    if (munmap(map, BUF_SIZE) == -1){
        fprintf(stderr, "Error: unable to unmap (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return 0;
}