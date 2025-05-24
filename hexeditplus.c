#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


bool done = false;
bool debug = false;
char file_name[128];
int unit_size;
unsigned char mem_buf[10000];
size_t mem_count;

/*
Choose action:
0-Toggle Debug Mode
1-Set File Name
2-Set Unit Size
3-Load Into Memory
4-Toggle Display Mode
5-Memory Display
6-Save Into File
7-Memory Modify
8-Quit 
*/

/* Function prototypes */
void toggle_debug_mode();
void set_file_name();
void set_unit_size();
void load_into_memory();
void toggle_display_mode();
void memory_display();
void save_into_file();
void memory_modify();
void quit();

/* Auxilary */
void print_debug_data();


struct fun_desc{
  char *name;
  void (*fun)();
};

struct fun_desc menu[] = {
  {"Toggle Debug Mode", toggle_debug_mode},
  {"Set file name", set_file_name},
  {"set unit size", set_unit_size},
  {"load into memory", load_into_memory},
  {"toggle display mode", toggle_display_mode},
  {"memory display", memory_display},
  {"save into file", save_into_file},
  {"memory modify", memory_modify},
  {"Quit", quit},
  {NULL, NULL}
};


int main() {
    char buf[12];
    int menu_size = 0;
    while(menu[menu_size].name != NULL) {
        menu_size++;
    }
    while (1) {
        // print menu
        print_debug_data();
        for(int i = 0; i < menu_size; i++) {
            printf("%d. %s\n",i ,menu[i].name);
        }

        //check for EOF
        if(fgets(buf, 12, stdin) == NULL) {
            printf("recieved EOF, exists\n");
            exit(0);
        }

        int pos = atoi(buf);
        // check bounds
        if ((pos < 0) || (pos >= menu_size)) {
            printf("not within bounds, exists\n");
            exit(0);
        }
        menu[pos].fun();
    }
}

void toggle_debug_mode() {
    if (debug)
        printf("debug mode is now off\n");
    else
        printf("debug mode is now on\n");
    debug = !debug;
}

void set_file_name() {
    char buf[100];

    printf("Enter file name: ");
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        printf("Error reading file name\n");
        return;
    }
    /* strip newline */
    buf[strcspn(buf, "\n")] = '\0';

    /* store */
    strncpy(file_name, buf, sizeof(file_name) - 1);
    file_name[sizeof(file_name) - 1] = '\0';

    if (debug) {
        fprintf(stderr, "Debug: file name set to '%s'\n", file_name);
    }
}

void set_unit_size() {
    int val;

    printf("Enter unit size (1, 2, or 4): ");
    if (scanf("%d", &val) != 1) {
        printf("Invalid input\n");
        /* clear stdin */
        while (getchar() != '\n');
        return;
    }
    /* clear leftover newline */
    while (getchar() != '\n');

    if (val == 1 || val == 2 || val == 4) {
        unit_size = val;
        if (debug) {
            fprintf(stderr, "Debug: set size to %d\n", unit_size);
        }
    } else {
        printf("Error: %d is not a valid unit size. Must be 1, 2, or 4.\n", val);
    }
}

void print_debug_data() {
    if (debug) {
        fprintf(stderr,
            "Debug: unit_size=%d, file_name=\"%s\", mem_count=%zu\n",
            unit_size, file_name[0]? file_name : "(none)", mem_count);
    }
}

void load_into_memory() {
    // Check if file_name is empty 
    if (strcmp(file_name, "") == 0) {
        fprintf(stderr, "file name is empty");
        return;
    }

    // Open file
    int fd = open(file_name, O_RDWR);
    if (fd < 0) {
        perror("open");
        return;
    }

    char line[128];
    unsigned int location;
    int length;

    // Prompt the user for location in hexadecimal, and length (in decimal).
    printf("Please enter <location> <length>\n");
    if (!fgets(line, sizeof(line), stdin)) {
        perror("fgets failed");
        return;
    }

    // parse hex into location, decimal into length 
    if (sscanf(line, "%x %d", &location, &length) != 2) {
        printf("Error: expected a hex offset and a decimal length\n");
        return;
    }

    // print debug
    if (debug) {
        fprintf(stderr,
            "Debug: file_name=\"%s\", location=0x%X, length=%d\n",
            file_name, location, length);
    }

    // read from file_name length * unit_size bytes of data in to mem_buf
    lseek(fd, location, SEEK_SET);
    int num_bytes_to_read = length * unit_size;
    if(read(fd, mem_buf, num_bytes_to_read) != num_bytes_to_read) {
        perror("read");
    }
    else {
        printf("loaded %d units into memory\n", length);
    }


    // close file
    if (close(fd) == -1)
        perror("close");
    
}

void toggle_display_mode() {
    printf("Not implemented yet\n");
}

void memory_display() {
    printf("Not implemented yet\n");
}

void save_into_file() {
    printf("Not implemented yet\n");
}

void memory_modify() {
    printf("Not implemented yet\n");
}

void quit() {
    exit(0);
}
