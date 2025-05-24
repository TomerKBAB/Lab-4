#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


bool done = false;
bool debug = false;
char file_name[128];
int unit_size = 1;
unsigned char mem_buf[10000];
size_t mem_count;
int display_mode = 0;  // 0 = hexadecimal (initial), 1 = decimal 

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
void print_units(FILE* output, unsigned char* buffer, int count, int start_addr);


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

static char* hex_formats[] = {
  "%#hhx\n", "%#hx\n", "No such unit", "%#x\n"
};
static char* dec_formats[] = {
  "%#hhd\n", "%#hd\n", "No such unit", "%#d\n"
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

    printf(" Enter file name: \n> ");
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

    printf("Enter unit size (1, 2, or 4): \n> ");
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
    printf("Please enter <location> <length>\n> ");
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
    if (display_mode) {
        // Currently decimal → switch off to hexadecimal 
        display_mode = 0;
        printf("Display flag now off, hexadecimal representation\n");
    }
    else {
        // Currently hexadecimal → switch on to decimal 
        display_mode = 1;
        printf("Display flag now on, decimal representation\n");
    }

}

void memory_display() {
    char line[128];
    int u;
    int addr;

    // Prompt the user for units in in decimal, and length (in hexdecimal).
    printf("Enter address and length:\n> ");

    if (!fgets(line, sizeof(line), stdin)) {
        perror("fgets failed");
        return;
    }

    // parse decimal into u, hexdecimal into addr 
    if (sscanf(line, "%x %d", &addr, &u) != 2) {
        printf("Error: expected a decimal units, and a hexadicmal addr\n");
        return;
    }

    print_units(stdout, mem_buf, u, addr);

}

/* Prints the buffer to screen by converting it to text with printf */
void print_units(FILE* output, unsigned char* buffer, int count, int start_addr) {
    unsigned char *start = buffer + start_addr; // keep `buffer` intact
    unsigned char *end = start + unit_size * count;
    int idx = unit_size - 1; // 0 for 1-byte, 1 for 2-byte, 3 for 4-byte

    if (display_mode) {
        printf("Decimal\n=======\n");
    } else {
        printf("Hexadecimal\n=======\n");
    }

    while (start < end) {
        int val = 0;
        // Safely load up to 4 bytes into an int
        memcpy(&val, start, unit_size);

        if (display_mode) {
            fprintf(output, dec_formats[idx], val);
        } else {
            fprintf(output, hex_formats[idx], val);
        }

        start += unit_size;
    }

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