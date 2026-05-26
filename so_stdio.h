#ifndef MYSTDIO
#define MYSTDIO

#define BUFF_SIZE 4096
#define SO_EOF -1
#define OP 0
#define READ_OP 1
#define WRITE_OP 2
#define READ_END 0
#define WRITE_END 1

typedef struct{
int fd;
char mode[2];
char buffer[BUFF_SIZE];
int buffer_size;
int cursor;
int error;
int current_buffer_pos;
int endOfFile;
int flag_op;
pid_t childPid;
} SO_FILE;

SO_FILE *so_fopen(const char *pathname, const char *mode);
int so_fclose(SO_FILE *stream);
int so_fgetc(SO_FILE *stream);
int so_fputc(int c, SO_FILE *stream);
size_t so_fread(void *ptr, size_t size, size_t nmeb, SO_FILE *stream);
size_t so_fwrite(const void *ptr, size_t size, size_t nmeb, SO_FILE *stream);
int so_fseek(SO_FILE *stream, long offset, int whence);
long so_ftell(SO_FILE *stream);
int so_fflush(SO_FILE *stream);
int so_fileno(SO_FILE *stream);
int so_feof(SO_FILE *stream);
int so_ferror(SO_FILE *stream);
SO_FILE *so_popen(const char *command, const char *type);
int so_pclose(SO_FILE *stream);

#endif
