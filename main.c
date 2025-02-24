#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
 #include <sys/types.h>
      
#include "filesystem.h"

#define WIDTH 100
#define HEIGHT 30
#define NUM_OPTIONS 5

const char *options[NUM_OPTIONS] = {"q-Exit", "c-Copy", "m-Move", "d-Delete", "[Click SPACE to select]"};


Node* currentNode;
char currentPath[2024];
int numEntries = 0;
int scrollOffset=0;

void drawMenu(WINDOW* win, Node* node, int selection, int offset) {
    werase(win);
    box(win, 0, 0);
    wbkgd(win, COLOR_PAIR(1));
    mvwprintw(win, 1, 4, " Name ");
    mvwprintw(win, 1, 40, "Size (bytes)");
    mvwprintw(win, 1, 60, "Last Modified");
   
    Node* child = node->firstChild;
    int i = 0, y = 3;
    int visibleHeight = HEIGHT - 8;
    int displayed = 0;

    while (child != NULL && i < offset) {
        child = child->nextSibling;
        i++;
    }

    while (child != NULL && displayed < visibleHeight) {
        if (i == selection) {
            wattron(win, A_REVERSE);
        }
	if(child->type==1){
	  wattron(win, A_BOLD);
	}
	
        mvwprintw(win, y, 2, "%s", child->name);
	mvwprintw(win, y, 40, "%ld", child->size);

	char timeBuffer[20];
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localtime(&child->lastMoidified));
        mvwprintw(win, y, 60, "%s", timeBuffer);
  

        if (child->type == 1) {
            wattroff(win, A_BOLD);
        }
	
        if (i == selection) {
            wattroff(win, A_REVERSE);
        }
        child = child->nextSibling;
	
        i++;
	y++;
        displayed++;
    }

    wrefresh(win);
}



void navigateBack(char* path) {
    char* lastSlash = strrchr(path, '/');
    if (lastSlash != NULL && lastSlash != path) {
        *lastSlash = '\0';
    } else {
        strcpy(path, "/");
    }
}



int countEntries(Node* node) {
    int count = 0;
    Node* child = node->firstChild;
    while (child != NULL) {
        count++;
        child = child->nextSibling;
    }
    return count;
}

Node* getSelectedNode(Node* node, int index) {
    Node* child = node->firstChild;
    for (int i = 0; i < index && child != NULL; i++) {
        child = child->nextSibling;
    }
    return child;
}

void updatePath(char* path, const char* newDir) {
    if (strcmp(path, "/") == 0) {
        snprintf(path, 1024, "/%s", newDir);
    } else {
        strncat(path, "/", 1024 - strlen(path) - 1);
        strncat(path, newDir, 1024 - strlen(path) - 1);
    }
}

void showPopup(WINDOW* win, const char* message) {
    int popupHeight = 7;
    int popupWidth = 40;
    int startX = (WIDTH - popupWidth) / 2;
    int startY = (HEIGHT - popupHeight) / 2;

    WINDOW* popup = newwin(popupHeight, popupWidth, startY, startX);
    box(popup, 0, 0);
    mvwprintw(popup, 1, 1, " Error:");
    mvwprintw(popup, 2, 1, "%s", message);
    mvwprintw(popup, popupHeight - 2, 1, "Press any key to close.");
    wbkgd(popup, COLOR_PAIR(1));
    wrefresh(popup);
    wrefresh(win);

 
    wgetch(popup);
    delwin(popup);
}


void showDeletePopup(WINDOW* win,  char* fileName, Node* parentNode, char* path) {
    int popupHeight = 10, popupWidth = 50;
    int startY = (LINES - popupHeight) / 2;
    int startX = (COLS - popupWidth) / 2;
    WINDOW* popup = newwin(popupHeight, popupWidth, startY, startX);
    box(popup, 0, 0);
    wbkgd(popup, COLOR_PAIR(1));
    mvwprintw(popup, 2, 2, "Are you sure you want to delete:");
    mvwprintw(popup, 3, 4, "%s?", fileName);
    mvwprintw(popup, 6, 2, "Press 'o' to confirm or 'q' to cancel.");
    wrefresh(popup);

    int ch;
    while ((ch = wgetch(popup)) != 'o' && ch != 'q');
    if (ch == 'o') {
      deleteNode(parentNode, fileName, path);
       mvprintw(LINES - 2, 2, "File deleted successfully");
        refresh();
        napms(2000); 
        mvprintw(LINES - 2, 2, "                         ");
        refresh();
    }
    delwin(popup);

}


void showCopyPopup(WINDOW* win,  char* sourcePath, char* destPath, char* fileName, Node* root) {
    int popupHeight = 10, popupWidth = 50;
    int startY = (LINES - popupHeight) / 2;
    int startX = (COLS - popupWidth) / 2;
    WINDOW* popup = newwin(popupHeight, popupWidth, startY, startX);
    box(popup, 0, 0);
    wbkgd(popup, COLOR_PAIR(1));
    mvwprintw(popup, 2, 2, "Source Path: %s", sourcePath);
    mvwprintw(popup, 4, 2, "Copy to: ");
    wrefresh(popup);
    echo();
    mvwgetnstr(popup, 4, 12, destPath, 1024);
    noecho();
    mvwprintw(popup, 6, 2, "Press 'o' to confirm or 'q' to cancel.");
    wrefresh(popup);
    int ch;
    while ((ch = wgetch(popup)) != 'o' && ch != 'q');
    if (ch == 'o') {
      Node* destNode = findNode(root, destPath);
    if (destNode == NULL || destNode->type != 1) {
        mvprintw(LINES - 2, 2, "Invalid destination path!");
        refresh();
        napms(2000); 
        mvprintw(LINES - 2, 2, "                         ");
        refresh();
        return;
    }else{
     
       addFile(root, destPath,sourcePath, fileName);
        mvprintw(LINES - 2, 2, "File copied successfully");
        refresh();
        napms(2000); 
        mvprintw(LINES - 2, 2, "                         ");
        refresh();
    } }
    else {
        destPath[0] = '\0';
    }
    delwin(popup);
}

void fileMove(Node* root,  char* sourcePath,  char* destPath,  char* fileName) {
    Node* sourceNode = findNode(root, sourcePath);
    Node* destNode = findNode(root, destPath);
    if (destNode == NULL || destNode->type != 1) {
        mvprintw(LINES - 2, 2, "Invalid destination path!");
        refresh();
        napms(2000); 
        mvprintw(LINES - 2, 2, "                         ");
        refresh();
        return;
    }

    
    
    Node* fileNode = sourceNode->firstChild;
    Node* prevNode = NULL;
    while (fileNode != NULL && strcmp(fileNode->name, fileName) != 0) {
        prevNode = fileNode;
        fileNode = fileNode->nextSibling;
    }
    if (fileNode == NULL) {
        mvprintw(LINES - 2, 2, "File not found in the source directory!");
        refresh();
        return;
    }

    
    char currentPath[2048];
    char fullDest[2048];
    char newfile[1024];
    strcpy(newfile, fileName);
    struct stat fileS2;
    snprintf(fullDest, sizeof(fullDest), "%s/%s", destPath, fileName);
    char baseName[512], ext[10];
    char* dot = strchr(fileName, '.');
    srand(time(NULL));
    if(lstat(fullDest, &fileS2) ==0){
      int random = rand()%10000;
      strncpy(baseName, fileName, dot-fileName);
      baseName[dot-fileName] = '\0';
      strncpy(ext, dot, sizeof(ext));
      snprintf(newfile, sizeof(newfile), "%s_%s_%d%s", baseName, "new",random, ext);
      snprintf(fullDest, sizeof(fullDest), "%s/%s", destPath, newfile); 
    }

    // printf("%s\n ", fileName);
    snprintf(currentPath, sizeof(currentPath), "%s/%s", sourcePath, fileName);
    if(destNode->firstChild ==NULL && destNode->size>0){
      traverseDir(destPath, destNode);
    }

    //printf("%s\n ", currentPath);

    if(fileOperation_exec("mv", currentPath, fullDest )!=0) return;
    if (prevNode == NULL) {
        sourceNode->firstChild = fileNode->nextSibling;
    } else {
        prevNode->nextSibling = fileNode->nextSibling;
    }
    fileNode->nextSibling = NULL;
    strcpy(fileNode->name, newfile);
    addChildFront(destNode, fileNode);
     mvprintw(LINES - 2, 2, "File moved  successfully");
        refresh();
        napms(2000); 
        mvprintw(LINES - 2, 2, "                         ");
        refresh();
    
}



void showMovePopup(WINDOW* win, char* sourcePath, char* destPath, char* fileName, Node* root) {
    int popupHeight = 10, popupWidth = 50;
    int startY = (LINES - popupHeight) / 2;
    int startX = (COLS - popupWidth) / 2;
    WINDOW* popup = newwin(popupHeight, popupWidth, startY, startX);
    box(popup, 0, 0);
    wbkgd(popup, COLOR_PAIR(1));
    mvwprintw(popup, 2, 2, "Source Path: %s", sourcePath);
    mvwprintw(popup, 4, 2, "Move to: ");
    wrefresh(popup);
    echo();
    mvwgetnstr(popup, 4, 12, destPath, 1024);
    noecho();
    mvwprintw(popup, 6, 2, "Press 'o' to confirm or 'q' to cancel.");
    wrefresh(popup);
    int ch;
    while ((ch = wgetch(popup)) != 'o' && ch != 'q');
    if (ch == 'o') {
      fileMove(root, sourcePath, destPath, fileName);
      
    } else {
        destPath[0] = '\0';
    }
    delwin(popup);
}


void  buttom_menu(WINDOW *win, int current_selection) {
    int row = 1;
    // int col = 0;

    werase(win);
    wbkgd(win, COLOR_PAIR(1));

    int total_length = 0;
    for (int i = 0; i < NUM_OPTIONS; i++) {
        total_length += strlen(options[i]) + 2;
    }
    int start_col = ((WIDTH - total_length) / 2) - ((WIDTH - total_length) / 4);

    for (int i = 0; i < NUM_OPTIONS; i++) {
        if (i == current_selection) {
            wattron(win, A_REVERSE);
        }
        mvwprintw(win, row, start_col, "%s", options[i]);
        if (i == current_selection) {
            wattroff(win, A_REVERSE);
        }
        start_col += strlen(options[i]) + 2;
    }

    wrefresh(win);
}

void handleBottomMenuSelection(int selection, WINDOW* win, Node* currentNode, int currentSelection, char* currentPath, Node* root) {
    Node* selectedNode = getSelectedNode(currentNode, currentSelection);
    char destPath[2024];

    switch (selection) {
        case 0:
            endwin();
            exit(0);

        case 1:
	    if (strcmp(selectedNode->name, "home") != 0 && strcmp(currentPath,"/")==0) {
                        break; }
            else if (selectedNode != NULL && selectedNode->type == 0 ) {
                showCopyPopup(win, currentPath, destPath, selectedNode->name, root);
            } else {
                showPopup(win, "Cannot copy a directory");
            }
            break;

        case 2:
	   if (strcmp(selectedNode->name, "home") != 0 && strcmp(currentPath,"/")==0) {
                        break; }
           else  if (selectedNode != NULL && selectedNode->type == 0) {
                showMovePopup(win, currentPath, destPath, selectedNode->name, root);
            } else {
                showPopup(win, "Cannot move a directory.");
            }
            break;

        case 3:
	   if (strcmp(selectedNode->name, "home") != 0 && strcmp(currentPath,"/")==0) {
                        break; }
	   else if (selectedNode != NULL && selectedNode->type == 0) {
	      showDeletePopup(win, selectedNode->name, currentNode, currentPath);
            } else {
                showPopup(win, "Cannot delete a directory.");
            }
            break;
    }
}


int main() {
  Node* root = createNode("/", 1, 0, 0);
    initRoot(root, "/");
    currentNode=root;
    strcpy(currentPath, "/");
    int currentSelection=0;
    char destPath[2024];
    int current_selection2=0;

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
    }
    timeout(100);
    WINDOW* win = newwin(HEIGHT, WIDTH, 0, 0);
    WINDOW* buttomWin = newwin(3, WIDTH-3,HEIGHT-4, 2);
    numEntries = countEntries(currentNode);
    drawMenu(win, currentNode, currentSelection, scrollOffset);
    /* while(ch==ERR){
      ch=wgetch(win);
      }*/
    int ch;
    while ((ch = getch()) != 'q') {
        int visibleHeight = HEIGHT - 8;

        switch (ch) {
            case KEY_UP:
                if (currentSelection > 0) {
                    currentSelection--;
                    if (currentSelection < scrollOffset) {
                        scrollOffset--;
                    }
                }
                break;

            case KEY_DOWN:
                if (currentSelection < numEntries-1) {
                    currentSelection++;
                    if (currentSelection >= scrollOffset + visibleHeight-1) {
                        scrollOffset++;
                    }
                }
                break;
	     case KEY_LEFT:
                current_selection2 = (current_selection2 - 1 + NUM_OPTIONS-1) % (NUM_OPTIONS-1);
                break;
            case KEY_RIGHT:
                current_selection2 = (current_selection2 + 1) % (NUM_OPTIONS-1);
                break;
	case ' ':
                handleBottomMenuSelection(current_selection2, win, currentNode, currentSelection, currentPath, root);
                break;

		case '\n': {
               Node* selectedNode = getSelectedNode(currentNode, currentSelection);
                if (strcmp(selectedNode->name, "home") != 0 && strcmp(currentPath,"/")==0) {
                        break;
                    }
                if (selectedNode != NULL && selectedNode->type == 1) {
		  updatePath(currentPath, selectedNode->name);
		  if(selectedNode->firstChild == NULL){
                    traverseDir(currentPath, selectedNode);
		  }
                    currentNode = selectedNode;
                    currentSelection = 0;
                    scrollOffset = 0;
                    numEntries = countEntries(currentNode);
                }
                break;
	     
	     }

	    
	        case 'd': {
                Node* selectedNode = getSelectedNode(currentNode, currentSelection);
		if (strcmp(selectedNode->name, "home") != 0 && strcmp(currentPath,"/")==0) {
                        break;
                    }
                if (selectedNode!= NULL && selectedNode->type == 1) {
                    showPopup(win, "Cannot delete a directory.");
                }
                else{
                    if (selectedNode != NULL && selectedNode->type == 0) {
                        char fileName[1024];
                        strncpy(fileName, selectedNode->name, sizeof(fileName));
                        showDeletePopup(win, fileName, currentNode, currentPath);
			// numEntries = countEntries(currentNode);
                        
                    }
                }
                break;
		}
	   case 'c': {
	     
                Node* selectedNode = getSelectedNode(currentNode, currentSelection);
		if (strcmp(selectedNode->name, "home") != 0 && strcmp(currentPath,"/")==0) {
                        break;
                    }
                if (selectedNode!= NULL && selectedNode->type == 1) { 
                    showPopup(win, "Cannot copy a directory.");
                }
                else{
                    if (selectedNode != NULL && selectedNode->type == 0) {
                        char fileName[1024];
                        strncpy(fileName, selectedNode->name, sizeof(fileName));
                        showCopyPopup(win, currentPath, destPath, fileName, root);
   
		      
                    }
                }
                break;
            }
	     case 'm': {
                Node* selectedNode = getSelectedNode(currentNode, currentSelection);
		if (strcmp(selectedNode->name, "home") != 0 && strcmp(currentPath,"/")==0) {
                        break;
                    }
                if (selectedNode!= NULL && selectedNode->type == 1) {  
                    showPopup(win, "Cannot move a directory.");
                }
                else{
                    if (selectedNode != NULL && selectedNode->type == 0) {
                        char fileName[1024];
                        strncpy(fileName, selectedNode->name, sizeof(fileName));
                        showMovePopup(win, currentPath, destPath, fileName, root);
                    }
                }
                break;
            }
	   case KEY_BACKSPACE:
            case 127:
                if (strcmp(currentPath, "/") != 0) {
                    navigateBack(currentPath);
                    Node* parentNode = findNode(root, currentPath);
                    if (parentNode != NULL) {
                        currentNode = parentNode;
                        currentSelection = 0;
                        scrollOffset = 0;
                        numEntries = countEntries(currentNode);
                    }
                }
                break;
	       
	}
	     drawMenu(win, currentNode, currentSelection, scrollOffset);
	     buttom_menu(buttomWin, current_selection2);
	    
	     }

    delwin(win);
    endwin();
    freeTree(root);
 
    return 0;
}
