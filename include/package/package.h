#ifndef NETPKT_H
#define NETPKT_H
#define MAX_BUFFER 256


//Linked list to store packages
struct bpkg_list {
    struct bpkg_obj* obj;
    struct bpkg_list* next;
};

//Checks if a bpkg object is complete (1) /incompete (0)
int isComplete(struct bpkg_obj* obj);
//Adds a package to the package list
void add_package(char* path, char* file, struct bpkg_list** list);
//Prints all contents of a package
void print_package(struct bpkg_list* list);
//Removes a package from the linked list
void remove_package(struct bpkg_list** list, const char* identifier);
//Free all packages from the list
void free_package(struct bpkg_list* list);
//Ensure the string ends with a /
void ensureTrailingSlash(char *str);
#endif