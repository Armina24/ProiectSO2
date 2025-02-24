#include "filesystem.h"

Node* createNode(char* name, int type, off_t size, time_t lastMoidified) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Eroare malloc nod\n");
        exit(1);
    }
    strncpy(newNode->name, name, NAME_LEN - 1);
    newNode->name[NAME_LEN - 1] = '\0';
    newNode->type = type;
    newNode->size=size;
    newNode->lastMoidified=lastMoidified;
    newNode->firstChild = NULL;
    newNode->nextSibling = NULL;
    return newNode;
}

void addChild(Node* parent, Node* child) {
    if (parent == NULL || parent->type != 1) {
        fprintf(stderr, "Parintele trebuie sa fie director\n");
        exit(3);
    }
    if (parent->firstChild == NULL) {
        parent->firstChild = child;
    }
	else{
    Node* sibling = parent->firstChild;
        while (sibling->nextSibling != NULL) {
            sibling = sibling->nextSibling;
        }
        sibling->nextSibling = child;
    }


}


void addChildFront(Node* parent, Node* child) {
    if (parent->firstChild == NULL) {
        parent->firstChild = child;
    }
	else{
	  child->nextSibling = parent->firstChild;
	  parent->firstChild=child;
    }


}

void traverseDir(char* path, Node* parent) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        perror("Deschiderea dir root");
        exit(2);
    }
    struct dirent* d;
    struct stat fileStat;
    char fullPath[2024];
    while ((d = readdir(dir)) != NULL) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
            continue;
        }
	snprintf(fullPath, sizeof(fullPath), "%s/%s", path, d->d_name);
	if(lstat(fullPath, &fileStat)==-1){
	  perror("Lstat");
	  continue;
	}
        int type = (d->d_type == DT_DIR) ? 1 : 0;
	off_t size= fileStat.st_size;
	time_t lastMoidified =fileStat.st_mtime;
        Node* child = createNode(d->d_name, type, size, lastMoidified);
        addChild(parent, child);
    }
    if (closedir(dir) == -1) {
        perror("Inchiderea dir root");
        exit(3);
    }
}

void initRoot(Node* root, char* name) {
    traverseDir(name, root);
}

Node* findNode(Node* root, char* path) {
    if (root == NULL || strcmp(path, "/") == 0) {
        return root;
    }
    char tempPath[1024];
    char* token;
    strncpy(tempPath, path, 1024);
    Node* current = root;
    token = strtok(tempPath, "/");
    if (strcmp(token, "") == 0) {
        token = strtok(NULL, "/");
    }
    while (token != NULL && current != NULL) {
        Node* child = current->firstChild;
        while (child != NULL) {
            if (strcmp(child->name, token) == 0) {
                current = child;
                break;
            }
            child = child->nextSibling;
        }
        if (child == NULL) {
            return NULL;
        }
        token = strtok(NULL, "/");
    }
    return current;
}

void completeTree(Node* root, char* path) {
    Node* target = findNode(root, path);
    if (target == NULL) {
        fprintf(stderr, "Directorul nu s-a gasit in arbore pentru afisare continut\n");
        exit(4);
    }
    traverseDir(path, target);
}


void addFile(Node* root, char* path, char* spath,  char* filename) {
    Node* target = findNode(root, path);
    if (target == NULL) {
      //fprintf(stderr, "Directorul in care se adauga fisierul nu s-a gasit\n");
        //exit(4);
	return;
    }
    struct stat fileStat, fileS2;
    char currentPath[2048];
    snprintf(currentPath, sizeof(currentPath), "%s/%s", spath, filename);
   
    if (lstat(currentPath, &fileStat) == -1) {
        perror("Eroare la obținerea informațiilor despre fișier");
        return;
    }

    if(target->firstChild ==NULL && target->size>0){
      traverseDir(path, target);
    }
    char fullDest[2048];
    Node* fileNode;
    snprintf(fullDest, sizeof(fullDest), "%s/%s", path, filename);
    char baseName[512], ext[10];
    char* dot = strchr(filename, '.');
    srand(time(NULL));
    if(lstat(fullDest, &fileS2) ==0)
      {
	int random = rand()%10000;
      strncpy(baseName, filename, dot-filename);
      baseName[dot-filename] = '\0';
      strncpy(ext, dot, sizeof(ext));
      char newfile[1024];
      snprintf(newfile, sizeof(newfile), "%s_%s_%d%s", baseName, "new",random,  ext);
      fileNode = createNode(newfile, 0, fileStat.st_size, fileStat.st_mtime);
      snprintf(fullDest, sizeof(fullDest), "%s/%s", path, newfile);
    }
    else{
    fileNode = createNode(filename, 0, fileStat.st_size, fileStat.st_mtime);
    }
    
    // printf("%s\n %s\n", currentPath, path);

    if(fileOperation_exec("cp", currentPath ,fullDest)!=0) return;
 
    addChildFront(target, fileNode);
}

void deleteNode(Node* parent,  char* fileName, char* path) {
    Node* current = parent->firstChild;
    Node* prev = NULL;

    char fullpath[2024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, fileName);

    if(fileOperation_exec("rm", fullpath,NULL )!=0) return;

    while (current != NULL) {
        if (strcmp(current->name, fileName) == 0) {
            if (prev == NULL) {
                parent->firstChild = current->nextSibling;
            } else {
                prev->nextSibling = current->nextSibling;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->nextSibling;
    }
}

void printTree(Node* root, int depth) {
    if (root == NULL) return;

    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("| - %s (%d)\n", root->name, root->type);
    printTree(root->firstChild, depth + 1);
    printTree(root->nextSibling, depth);
}

void freeTree(Node* root) {
    if (root == NULL) return;
    freeTree(root->firstChild);
    freeTree(root->nextSibling);
    free(root);
}

int fileOperation_exec(char* operation, char* sourcePath, char* destinationPath) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Eroare la fork");
        exit(1);
    } else if (pid == 0) {
        if (strcmp(operation, "rm") == 0) {
            execlp(operation, operation, sourcePath, NULL);
        } else {
            execlp(operation, operation, sourcePath, destinationPath, NULL);
        }

        //perror("Eroare la execuția comenzii");
        return 1;
    } else {
        int status;
        if (wait(&status) == -1) {
	  // perror("Eroare la wait");
	  return 1;
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
	  //fprintf(stderr, "Eroare: Operația '%s' a eșuat.\n", operation);
	  return 1;
        }
    }
    return 0;
}

