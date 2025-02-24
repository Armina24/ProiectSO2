#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
   
 #include <sys/stat.h>
       


#define NAME_LEN 256

typedef struct Node {
    char name[NAME_LEN];
    int type;
  off_t size;
  time_t lastMoidified;
    struct Node* firstChild;
    struct Node* nextSibling;
} Node;

Node* createNode(char* name, int type, off_t size, time_t lastModified);
void addChild(Node* parent, Node* child);
void traverseDir(char* path, Node* parent);
void initRoot(Node* root, char* name);
Node* findNode(Node* root, char* path);
void addChildFront(Node* parent, Node* child);
void completeTree(Node* root, char* path);
void addFile(Node* root, char* path, char *spath, char* filename);
void deleteNode(Node* parent, char* fileName, char* path);
void printTree(Node* root, int depth);
void freeTree(Node* root);
int fileOperation_exec(char* operation, char* sourcePath, char* destinationPath);

#endif

