#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

#define BLOCK_SIZE 4096  // Write in 4KB chunks
#define PROGRESS_BAR_WIDTH 50  // Width of progress bar

// Function to get the total drive size (in KB)
off_t get_drive_size(const char *drive_path) {
    struct stat st;
    if (stat(drive_path, &st) == 0) {
        return st.st_size / 1024;  // Convert bytes to KB
    }else{
        perror("Error getting drive size");
        return -1;
    }
}

// Function to print progress bar with ETA
void print_progress(off_t written_kb, off_t total_kb, time_t start_time) {
    if (total_kb <= 0) return;  // Prevent division by zero

    int percentage = (int)((written_kb * 100) / total_kb);
    int bar_fill = (percentage * PROGRESS_BAR_WIDTH) / 100;

    // Calculate elapsed time and ETA
    time_t current_time = time(NULL);
    double elapsed_time = difftime(current_time, start_time);  // seconds
    double estimated_time_remaining = (elapsed_time * total_kb) / written_kb - elapsed_time;
    
    int minutes_remaining = (int)(estimated_time_remaining / 60);
    int seconds_remaining = (int)(estimated_time_remaining) % 60;

    // Print progress bar
    printf("\r[");
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < bar_fill) {
            printf("█");  // Filled bar
        } else {
            printf("-");  // Empty space
        }
    }
    printf("] %d%% (%ld KB/%ld KB) ETA: %02d:%02d", percentage, written_kb, total_kb, minutes_remaining, seconds_remaining);
    fflush(stdout);
}

// Function to securely overwrite drive
void wipe_drive(const char *drive_path, unsigned char pattern, int pass, off_t total_kb, time_t start_time) {
    int fd = open(drive_path, O_WRONLY);
    if (fd == -1) {
        perror("Error opening drive");
        return;
    }

    unsigned char buffer[BLOCK_SIZE];
    memset(buffer, pattern, BLOCK_SIZE);  // Fill buffer with pattern

    printf("\n%d: Writing 0x%02X to %s\n", pass, pattern, drive_path);

    off_t written_kb = 0;
    ssize_t bytes_written;

    while ((bytes_written = write(fd, buffer, BLOCK_SIZE)) > 0) {
        written_kb += bytes_written / 1024;  // Convert bytes to KB
        print_progress(written_kb, total_kb, start_time);
    }

    if (bytes_written == -1) {
        perror("Error writing to drive");
    }

    close(fd);
    printf("\n");
}

// Function to write strong random data
void wipe_drive_random(const char *drive_path, int pass, off_t total_kb, time_t start_time) {
    int fd = open(drive_path, O_WRONLY);
    if (fd == -1) {
        perror("Error opening drive");
        return;
    }

    unsigned char buffer[BLOCK_SIZE];
    ssize_t bytes_written;
    
    printf("\n%d: Writing random data to %s\n", pass, drive_path);

    FILE *rand_source = fopen("/dev/urandom", "rb");
    if (!rand_source) {
        perror("Error opening /dev/urandom");
        close(fd);
        return;
    }

    off_t written_kb = 0;

    while (1) {
        fread(buffer, 1, BLOCK_SIZE, rand_source); // Get strong random data
        bytes_written = write(fd, buffer, BLOCK_SIZE);
        if (bytes_written <= 0) break; // Stop if write fails
        written_kb += bytes_written / 1024;
        print_progress(written_kb, total_kb, start_time);
    }

    fclose(rand_source);
    if (bytes_written == -1) {
        perror("Error writing to drive");
    }

    close(fd);
    printf("\n");
}

// Safe exit function to handle SIGINT (Ctrl+C)
volatile sig_atomic_t stop = 0;

void handle_sigint(int sig) {
    stop = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <drive> <passes>\n", argv[0]);
        return 1;
    }

    srand(time(NULL)); // Seed random number generator

    // Get total drive size for progress tracking
    off_t total_kb = get_drive_size(argv[1]);
    if (total_kb <= 0) {
        fprintf(stderr, "Error: Could not determine drive size.\n");
        return 1;
    }

    // Set up signal handler for safe exit
    signal(SIGINT, handle_sigint);

    time_t start_time = time(NULL);  // Start time for ETA calculation

    int passes = atoi(argv[2]);

    for (int k = 0; k < passes; k++) {
        if (stop) break;  // Check if user wants to stop
        wipe_drive_random(argv[1], k+1, total_kb, start_time);
        wipe_drive(argv[1], 0xAA, k+1, total_kb, start_time); // 10101010 pattern
        wipe_drive(argv[1], 0x55, k+1, total_kb, start_time); // 01010101 pattern
    }

    if (stop) {
        printf("\nWipe process interrupted. Please ensure drive is safely handled.\n");
        return 1;  // Exit if interrupted
    }

    // Final cleansing passes
    wipe_drive(argv[1], 0xFF, passes+1, total_kb, start_time); // Fill with 1s
    wipe_drive(argv[1], 0x00, passes+2, total_kb, start_time); // Fill with 0s

    printf("\nDrive securely wiped %d times and finalized.\n", passes);
    return 0;
}
