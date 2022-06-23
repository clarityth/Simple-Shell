#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command */

// 명령어 파싱 + background 명령어 체크 함수
int parseCmd(char cmd[], char *args[]){ 
	char *ptr = cmd; // 파싱된 명령어를 저장하기 위한 포인터
	char *dptr = strchr(ptr, ' '); // 공백, 개행 문자를 삭제하기 위한 포인터
	int i = 0; // args 배열의 index

	while (*ptr == ' '){ // 첫번째 공백을 가르킴
		ptr++;
	}

	while (dptr != NULL){ // 사용자 입력의 모든 공백을 기준으로 명령어 파싱
		*dptr = '\0'; // 공백 문자 삭제
		args[i++] = ptr; // 공백 앞의 명령어를 args에 저장
		ptr = dptr + 1; // 공백의 다음 문자를 가르킴
		dptr = strchr(dptr+1, ' ');
	}

	dptr = strchr(ptr, '\n');
	*dptr = '\0'; // 개행 문자 삭제
	args[i] = ptr;

	if (!strcmp(args[i], "&")){ // '&' 문자가 있는 경우, 1을 리턴
		args[i] = NULL; // NULL 처리
		return 1;
	}
	else { // '&' 문자가 없는 경우, 0을 리턴
		args[i+1] = NULL; // NULL 처리
		return 0;
	}
}

// pipe 명령어 체크 함수
int chkPipe(char *args[], char ***p_after){ 
	int i = 0; // '|' 문자의 index 저장 변수
	while (args[i] != NULL){ // 배열의 끝까지 
		if(!strcmp(args[i], "|")) // '|' 문자를 만나면 종료
			break;
		i++; // index 증가
	}

	if (args[i] != NULL){ // 포인터가 NULL이 아닌 경우 -> Pipe 문자가 있음
		args[i] = NULL; // NULL 처리
		*p_after = args + i + 1; // '|' 다음 문자를 가르킴
		return 1; // 1을 리턴
	}
	else { // 포인터가 NULL인 경우 -> Pipe 문자가 없음
		return 0; // 0을 리턴
	}
}


// redirect 명령어 체크 함수
int chkRedir(char *args[], char r_cmd[]){ 
	int i = 0; // '<', '>' 문자 index 저장 변수
	while (args[i] != NULL) { // 배열의 끝까지 
		if (!strcmp(args[i], ">") || !strcmp(args[i], "<")) // '>', '<'를 만나면 종료
			break;
		i++; // index 증가
	}

	if (args[i] != NULL){ // 포인터가 NULL이 아닌 경우 -> redirect 문자가 있음
		if (!strcmp(args[i], ">")){ // Output Redirect의 경우
			strcpy(r_cmd, args[i + 1]); // 뒷부분 명령어를 복사
			args[i] = NULL; // '>' 문자 삭제
			return 1; // 1을 리턴
		}
		else if (!strcmp(args[i], "<")){ // Input Redirect의 경우
			strcpy(r_cmd, args[i + 1]); // 뒷부분 명령어를 복사
			args[i] = NULL; // '<' 문자 삭제
			return 2; // 2를 리턴
		} 
	}
	else // 포인터가 NULL인 경우 -> redirect 문자가 없음
		return 0; // 0을 리턴
}

// Pipe 실행 함수
void pipeExecute(int b, char *args[], char **p_after){
	int fd[2];
	pid_t pid1;
	pid_t pid2;
	pid1 = fork(); // 첫번째 fork
	if (pid1 < 0) { 
		perror("First Fork Error\n"); // 에러 처리
		exit(1);
	}
	if (pipe(fd) < 0){ 
		perror("Pipe Error\n");
		exit(1);
	}
	if (pid1 > 0){ // 첫번째 fork의 부모 프로세스의 경우
		if (b == 0){ // Foreground Process
			waitpid(pid1, NULL, 0);
		}
		else { // Background Process
			printf("PID: %d\n", getpid()); // pid 출력
		}
	}
	else { // 첫번째 fork의 자식 프로세스의 경우
		pid2 = fork(); // 두번째 fork
		if (pid2 < 0){
			perror("Second Fork Error\n"); // 에러 처리
			exit(1);
		}
		else if (pid2 > 0){ // 두번쨰 fork의 부모 프로세스의 경우
			close(fd[1]); // fd[1] 닫기
			dup2(fd[0], STDIN_FILENO); // fd[0] 표준 입력 파이프 연결
			close(fd[0]); // fd[0] 닫기
			execvp(p_after[0], p_after);
			// Execvp 에러시 에러메시지 출력
			perror("Execvp Error\n");
			exit(1);
		}
		else { // 두번째 fork의 자식 프로세스의 경우
			close(fd[0]); // fd[0] 닫기
			dup2(fd[1], STDOUT_FILENO); // fd[1] 표준 출력 파이프 연결
			close(fd[1]); // fd[1] 닫기
			execvp(args[0], args);
			// Execvp 에러시 에러메시지 출력
			perror("Execvp Error\n");
			exit(1);
		}
	}
}

// Redirect 실행 함수
void redirExecute(int r, int b, char *args[], char *r_cmd){
	int fd;
	int status;
	pid_t pid;

	pid = fork();
	if (pid < 0) {  // 에러 처리
		perror("First Fork Error\n");
		exit(1);
	}
	// 부모 프로세스의 경우
	else if(pid > 0){
		if (b == 0){ // Foreground Process
			pid = wait(&status);  // 자식 프로세스가 종료될 때까지 대기
		} 
		else { // Background Process
			printf("PID: %d\n", getpid()); // pid 출력
		}
	}
	// 자식 프로세스의 경우
	else {
		if (r == 1){ // Output Redirect 명령어가 있는 경우
			fd = open(r_cmd, O_WRONLY | O_CREAT | O_TRUNC, 0644); // 쓰기전용, 파일생성, 기존파일 내용삭제 옵션
        	dup2(fd, STDOUT_FILENO); // 표준 출력으로 redirect
       		close(fd);
		}
		else if (r == 2){ // Input Redirect 명령어가 있는 경우
			if((fd = open(r_cmd, O_RDONLY)) == 1) { // 읽기전용 옵션
				perror(args[0]);
				exit(2);
        	}
			dup2(fd, STDIN_FILENO); // 표준 입력으로 redirect
			close(fd); 
		}
		execvp(args[0], args); // args 실행
	}
}

// 일반 명령어 실행 함수
void execute(int b, char *args[]){
	int status;
    pid_t pid;
    pid = fork();
    if(pid < 0) {
        perror("Fork Error\n");
    }
	// 부모 프로세스일 경우
    else if(pid > 0) {                
        if(b == 0){ 
			pid = wait(&status); // Foreground Process
		}
        else { // Background Process
             printf("PID: %d\n", getpid()); // pid 출력
        }
    }
	// 자식 프로세스일 경우
    else {
		execvp(args[0], args); // args 실행
	}
}

int main(void){
	char *args[MAX_LINE / 2 + 1]; // 파싱 명령어 저장 배열
	int should_run = 1;
	char cmd[MAX_LINE]; // 명령어 입력받을 배열
	int b; // Background 명령어 판별 변수
	int r; // Redirect 타입(Input, Output) 판별 변수
	int p; // Pipe 명령어 판별 변수
	char r_cmd[20]; // Redirect 명령어 이후 문자열 저장 변수
	char **p_after; // 파이프 명령어 이후 문자 저장 포인터

	while (should_run){
		printf("osh>");
		fflush(stdout); // 출력 버퍼를 비움
		fgets(cmd, MAX_LINE, stdin); // 명령어 입력받음

		b = parseCmd(cmd, args); // 명령어 파싱 + Background 명령어 체크
		r = chkRedir(args, r_cmd); // redirect 명령어 체크
		p = chkPipe(args, &p_after); // pipe 명령어 체크

		if (!strcmp(args[0], "exit")) { // exit 명령어 입력시 종료
			should_run = 0;
			continue;
		}
		if (p == 1) { // Pipe 명령어가 있는 경우
			pipeExecute(b, args, p_after);
		}
		else { // Pipe 명령어가 없는 경우
			if (r > 0){ // Redirect 명령어가 있는 경우
				redirExecute(r, b, args, r_cmd); // Redirect 타입(Output, Input)과 Background 명령어 유무를 인자로 전달
			}
			else { // 일반 명령어의 경우
				execute(b, args);
			}
		}
	}
	return 0;
}
