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
        mem_count = 0;
    }
    else {
        mem_count = num_bytes_to_read;
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
    char line[128];
    unsigned int src_addr, target_location;
    int length;

    // Prompt the user for source address (hex) target location (hex) and length (dec) 
    printf("Please enter <source-address> <target-location> <length>\n");

    if (!fgets(line, sizeof(line), stdin)) {
        perror("fgets failed");
        return;
    }

    // parse the data
    if (sscanf(line, "%x %x %d", &src_addr, &target_location, &length) != 3) {
        fprintf(stderr, "expected for source address (hex) target location (hex) and length (dec)");
        return;
    }

    // Debug print 
    if (debug) {
        fprintf(stderr,
            "Debug: file=\"%s\", src=0x%X, tgt=0x%X, length=%d\n",
            file_name, src_addr, target_location, length);
    }

    // Open for read/write
    int fd = open(file_name, O_RDWR);
    if (fd < 0) {
        perror("open");
        return;
    }

    // Check file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size < 0) {
        perror("lseek");
        close(fd);
        return;
    }
    if ((off_t)target_location> file_size) {
        fprintf(stderr, "Error: target offset 0x%X beyond end of file (0x%zX)\n",
               target_location, (size_t)file_size);
        close(fd);
        return;
    }

    unsigned char *src_ptr;

    if (src_addr > mem_count) {
    fprintf(stderr, "Error: source offset 0x%X beyond end of buffer (0x%zX)\n",
            src_addr, mem_count);
    close(fd);
    return;
    }
    src_ptr = mem_buf + src_addr;

    // check that you actually have length * unit_size bytes available
    if ((size_t)src_addr + length*unit_size > mem_count) {
    fprintf(stderr, "Error: cannot write %d units from offset 0x%X; only 0x%zX bytes loaded\n",
            length, src_addr, mem_count);
    close(fd);
    return;
}

    // Seek to target and write
    if (lseek(fd, target_location, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        close(fd);
        return;
    }
    ssize_t to_write = (ssize_t)length * unit_size;
    ssize_t written  = write(fd, src_ptr, to_write);
    if (written < 0) {
        perror("write");
    }
    else if (written != to_write) {
        fprintf(stderr, "Warning: only wrote %zd of %zd bytes\n",
               written, to_write);
    }
    else {
        printf("Saved %d units into file\n", length);
    }

    if (close(fd) < 0) {
        perror("close");
    }
}

void memory_modify() {
    char line[128];
    unsigned int location;
    unsigned int val;

    /* Prompt for buffer offset and value */
    printf("Please enter <location> <val>\n");
    if (!fgets(line, sizeof(line), stdin)) {
        perror("fgets failed");
        return;
    }

    /* Parse two hex values */
    if (sscanf(line, "%x %x", &location, &val) != 2) {
        fprintf(stderr, "Invalid input. Expected: <location> <val>\n");
        return;
    }

    /* Debug output */
    if (debug) {
        fprintf(stderr, "Debug: location=0x%X, val=0x%X\n", location, val);
    }

    /* Bounds check */
    if ((size_t)location + (size_t)unit_size > mem_count) {
        fprintf(stderr,
                "Error: location 0x%X + %d exceeds loaded memory size (0x%zX)\n",
                location, unit_size, mem_count);
        return;
    }

    /* Perform the write of 'unit_size' bytes into mem_buf */
    unsigned char *ptr = mem_buf + location;
    /* Copy the raw bytes of 'val' into the buffer location */
    memcpy(ptr, &val, unit_size);

    printf("Modified memory at offset 0x%X with value 0x%X\n", location, val);
}

void quit() {
    exit(0);
}

// TASK 2
/*
 *  readelf -h deep_thought | grep 'Entry point'
 * Calculate where in the file to patch
 * elf header e_entry lives at byte 0x18 and 4 bytes long
 * to fix: load from start in 4 bytes jump (at least 8 units)
 * modify the buffer with : 18 8048350
 * copy back to file: 18 18 1
*/

// TASK 3
/*
 * readelf -s offensive |grep main
 *    vir addr  size  NDX section
 * - 0804841d   23    13
 * readelf -S offensive| grep main
 * .text     addr     off    size
 *          08048320 000320 000192
 * file_offset = (VALUE - Addr) + Off
 * file_offset = (0804841d - 08048320) + 320 (HEX)
 * file_offset = FD + 320 = 41D
 * (then use hexeditplus to change the first opcode to C3 instead of all nops)
*/

// TASK 4
/* 
*   readelf -s ntsc | grep count_digits
    ....
*/