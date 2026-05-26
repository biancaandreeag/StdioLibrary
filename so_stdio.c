#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "so_stdio.h"


SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	int fd;

	if(strcmp(mode, "r") == 0){
		fd=open(pathname,O_RDONLY,0644);
	}else if(strcmp(mode, "r+") ==0){
		fd=open(pathname,O_RDWR,0644);
	}else if(strcmp(mode, "w")==0){
		fd=open(pathname,O_WRONLY|O_CREAT|O_TRUNC,0644);
	}else if(strcmp(mode, "w+")==0){
		fd=open(pathname,O_RDWR|O_CREAT|O_TRUNC,0644);
	}else if(strcmp(mode, "a")==0){
		fd=open(pathname,O_WRONLY|O_CREAT|O_APPEND,0644);
	}else if(strcmp(mode, "a+")==0){
		fd=open(pathname,O_RDWR|O_CREAT|O_APPEND,0644);
	}else{
		perror("Requested mode does not exist.");
		return NULL;
	}

	if(fd==-1){
		perror("Error in opening file.");
		return NULL;
	}

	SO_FILE *stream=(SO_FILE *)malloc(sizeof(SO_FILE));

	if(stream==NULL){
		close(fd);
		return NULL;
	}

	stream->fd=fd;
	strncpy(stream->mode,mode,2);
	stream->flag_op=OP;
	stream->buffer_size=0;
	stream->cursor=0;
	stream->error=0;
	stream->current_buffer_pos=0;
	stream->endOfFile=0;
	stream->childPid=0;

	return stream;
}

int so_fclose(SO_FILE *stream)
{
	if (stream == NULL || stream->fd == -1) 
		return SO_EOF;
	

	if(stream->flag_op==WRITE_OP){
		int reset=so_fflush(stream);
		
		if(reset==-1){
			stream->error=-1;
			free(stream);
			stream=NULL;
			return SO_EOF;
		}
	}

	int fc = close(stream->fd);

	if (fc == -1) {
		stream->error=-1;
		free(stream);
		stream=NULL;
		return SO_EOF;
	}

	free(stream);
	stream=NULL;

	return 0;
}

int so_fgetc(SO_FILE *stream)
{
	if (stream == NULL || stream->fd == -1) {
		stream->error = -1;
		return SO_EOF;
	}

	if (stream->current_buffer_pos == 0 || stream->current_buffer_pos == stream->buffer_size) {
		memset(stream->buffer,0,BUFF_SIZE);
		int bytesRead = read(stream->fd, stream->buffer, BUFF_SIZE);

		if (bytesRead == 0) {
			stream->endOfFile = -1;
			stream->error=-1;
			return SO_EOF;
		}

		if (bytesRead == -1) {
			stream->error = -1;
			return SO_EOF;
		}

		stream->buffer_size=bytesRead;
		stream->current_buffer_pos = 0;
	}

	 if (stream->current_buffer_pos >= stream->buffer_size) {
		stream->endOfFile = -1;
		stream->error=-1;
		return SO_EOF;
	}
	

	stream->flag_op = READ_OP;
	int pos = stream->current_buffer_pos;
	stream->current_buffer_pos++;
	stream->cursor++;
	
	return (unsigned char)stream->buffer[pos];
}

int so_fputc(int c, SO_FILE *stream)
{
   if(stream==NULL || stream->fd ==-1)
		return SO_EOF;
	

   if(stream->buffer_size==BUFF_SIZE){
	  int reset=so_fflush(stream);
	  if(reset==-1){
		stream->endOfFile=-1;
		stream->error=-1;
		return SO_EOF;
	  }
   }

   stream->buffer[stream->buffer_size]=(char)c;
   stream->buffer_size++;
   stream->cursor++;
   stream->flag_op=WRITE_OP;

   return (unsigned char)c;
}

size_t so_fread(void *ptr, size_t size, size_t nmeb, SO_FILE *stream)
{
	if(stream==NULL || stream->fd==-1)
		return 0;
	
	size_t elmRead=0;
	unsigned char* buffer=(unsigned char*) ptr;

	for(size_t i=0;i<nmeb;i++){
		for(size_t j=0;j<size;j++){
			int ch=so_fgetc(stream);
			if(ch==-1)
				return elmRead;
			
			buffer[elmRead*size+j]=(char)ch;
		}
		elmRead++;
	}

	stream->flag_op=READ_OP;
	return elmRead;
}

long so_ftell(SO_FILE *stream)
{
	if(stream==NULL || stream->fd ==-1)
		return SO_EOF;
	

	if(stream->flag_op!=READ_OP){
		int reset=so_fseek(stream,0,SEEK_CUR);
		if(reset==-1){
			stream->error=-1;
			return SO_EOF;
		}
	}
	stream->flag_op=OP;
	return stream->cursor;
}

int so_fseek(SO_FILE *stream, long offset,int whence)
{
	if(stream==NULL || stream->fd==-1)
		return SO_EOF;
	

	if(stream->flag_op==WRITE_OP){
		int rd=so_fflush(stream);
		if(rd==-1){
			stream->error=-1;
			return SO_EOF;
		}
	}else if(stream->flag_op==READ_OP){
		stream->buffer_size=0;
		stream->current_buffer_pos=0;
		memset(stream->buffer,0,stream->buffer_size);
	}

	int c=lseek(stream->fd,offset,whence);
	if(c==-1){
		stream->error=-1;
		return SO_EOF;
	}

	stream->cursor=c;
	stream->flag_op=OP;
	return 0;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmeb, SO_FILE *stream)
{
	if(stream==NULL || stream->fd ==-1)
		return SO_EOF;
	

	size_t totalBytesToWrite=size*nmeb;
	int elmWr=0;
	char ch;
	for(int i=0;i<totalBytesToWrite;i++){
		ch=((char*)ptr)[i];
		int writeBytes=so_fputc(ch,stream);
		if(writeBytes==-1 ){
			if(stream->error==-1 || stream->endOfFile==-1)
			return 0;
		}
	}

	stream->flag_op=WRITE_OP;

	return nmeb;
}

int so_fflush(SO_FILE *stream) 
{
	
	if (stream == NULL || stream->fd == -1) 
		return SO_EOF;
	

	if (stream->flag_op == WRITE_OP) {
		int already_written = 0;
		int remaining = stream->buffer_size;

		while (remaining > 0) {
			int bytes_written = write(stream->fd, stream->buffer + already_written, remaining);

			if (bytes_written ==-1) {
				stream->error = -1;
				stream->endOfFile=-1;
				return SO_EOF;
			}
		   
			remaining -= bytes_written;
			already_written += bytes_written;
		}

		memset(stream->buffer, 0, BUFF_SIZE); 
		stream->buffer_size = 0;
		stream->current_buffer_pos = 0;
		stream->flag_op = OP; 
		
	}

	return 0;
}

int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}

int so_feof(SO_FILE *stream)
{
	return stream->endOfFile;
}
int so_ferror(SO_FILE *stream)
{
	return stream->error;
}

SO_FILE *so_popen(const char *command, const char *type) 
{
	int pipe_fd[2];
	int ret = pipe(pipe_fd);
	if (ret == -1) 
		return NULL;
	

	int pid=-1;
	pid = fork();
	int fileDescriptor=0;

	if (pid < 0) {
		close(pipe_fd[WRITE_END]);
		close(pipe_fd[READ_END]);
		return NULL;
	} else if (pid == 0) {
		if (strcmp(type, "r") == 0) {
			close(pipe_fd[READ_END]);
			dup2(pipe_fd[WRITE_END], STDOUT_FILENO);
			close(pipe_fd[WRITE_END]);
		}

		if (strcmp(type, "w") == 0) {
			close(pipe_fd[WRITE_END]);
			dup2(pipe_fd[READ_END], STDIN_FILENO);
			close(pipe_fd[READ_END]);
		}

		int ok;
		ok=execlp("sh", "sh", "-c", command, NULL);
		exit(1);      
	}else {
		if (strcmp(type, "r") == 0) {
			fileDescriptor = pipe_fd[READ_END];
			close(pipe_fd[WRITE_END]);
		}
		if (strcmp(type, "w") == 0) {
			fileDescriptor = pipe_fd[WRITE_END];
			close(pipe_fd[READ_END]);
		}
	}

	SO_FILE* stream = (SO_FILE*)malloc(sizeof(SO_FILE));
	if (stream == NULL) {
		close(pipe_fd[WRITE_END]);
		close(pipe_fd[READ_END]);
		return NULL;
	}

	stream->fd = fileDescriptor;
	stream->childPid = pid;
	stream->buffer_size = 0;
	stream->cursor = 0;
	stream->error = 0;
	stream->current_buffer_pos = 0;
	stream->endOfFile = 0;

	if (strcmp(type, "w") == 0) {
		stream->flag_op = WRITE_OP;
	}else {
		stream->flag_op = READ_OP;
	}

	return stream;
}

int so_pclose(SO_FILE *stream)
{
	pid_t pid=stream->childPid;
	if(pid==-1){
		return SO_EOF;
	}
	else{
		int ok=so_fclose(stream);
		if(ok==-1){
			return SO_EOF;
		}
		int status;
		ok=waitpid(pid,&status,0);
		if(ok==-1){
			return SO_EOF;
		}
		return status;
	}
}
