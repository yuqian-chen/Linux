#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

int main(int argc, char* argv[]){
  while(1){
    printf("mysh> ");
    char user_input[1024];
    char *args[1024];
    char *token;
    int i = 0;
    if(fgets(user_input, 1024, stdin)!=NULL){
      token = strtok(user_input, " \n");
      while(token != NULL){
	args[i] = token;
	i++;
	token = strtok(NULL, " \n");
      }
      args[i] = NULL;
      if(args[0] == NULL){
	fprintf(stderr, "Error!\n");
      }
      else if(strcmp(args[0],"exit") == 0){
	if(args[1] != NULL){
	  fprintf(stderr, "Error!\n");
	}else{
	  exit(0);
	}
      }
      else if(strcmp(args[0], "cd") == 0){
	if(args[1] == NULL){
	  if(chdir(getenv("HOME")) != 0){
	    fprintf(stderr, "Error!\n");
	  }
	}
	else{
	  if(chdir(args[1]) != 0){
	    fprintf(stderr, "Error!\n");
	  }
	}
      }
      else if(strcmp(args[0], "pwd") == 0){
	char buff[PATH_MAX + 1];
	if(getcwd(buff, PATH_MAX + 1) != NULL){
	  printf("%s\n", buff);
	}
	else{
	  fprintf(stderr, "Error!\n");
	}
      }
      else{
	int pid = fork();
	if(pid < 0){
	  fprintf(stderr, "Error!\n");
	  exit(1);
	}
	else if(pid == 0){
	  int k;
	  int found = 0;
	  for(k = 0; args[k] != NULL && found == 0; k++){
	    if(strcmp(args[k], ">") == 0 || strcmp(args[k], ">>") == 0){
	      found = 1;
	      int fd;
	      if(strcmp(args[k], ">") == 0){
			fd = open(args[k + 1], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
	      }
	      else{
	      	fd = open(args[k + 1], O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
	      }
	      dup2(fd, 1);
	      for(;args[k+2] != NULL; k++){
	      	args[k] = args [k+2];
	      }
	      args[k] = NULL;
	    }
	  }
	  int pipeMatch = 0;
	  for(k = 0; args[k] != NULL && pipeMatch == 0; k++){
	    if(strcmp(args[k], "|") == 0){
	      pipeMatch = 1;
	    }
	  }
	  if(pipeMatch == 1){
	    int fds[2];
	    int cpid = pipe(fds);
	    args[k - 1] = NULL;
	    cpid = fork();
	    if(cpid < 0){
	      fprintf(stderr, "Error!\n");
	      exit(1);
	    }
	    else if(cpid == 0){
	      dup2(fds[1],1);
	      close(fds[0]);
	      execvp(args[0], args);
	      fprintf(stderr, "Error!\n");
	      exit(1);
	    }
	    else{
	      wait(NULL);
	      int count;
	      char *input_args[1024];
	      for(count = 0; args[k] != NULL; count++, k++){
		input_args[count] = args[k];
	      }
	      input_args[count] = NULL;
	      dup2(fds[0], 0);
	      close(fds[1]);
	      execvp(input_args[0], input_args);
	      fprintf(stderr, "Error!\n");
	      exit(1);
	    }
	  }
	  else{
	    execvp(args[0], args);
	    fprintf(stderr, "Error!\n");
	    exit(1);
	  }
	}
	else{
	  wait(NULL);
	}
      }
    }
    else{
      fprintf(stderr, "Error!\n");
    }
  }
  return 0; 
}
