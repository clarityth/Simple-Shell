# Simple shell
This project consists of designing a C program to serve as a shell interface that accepts user
commands and then executes each command in a separate process. Your implementation will
support input and output redirection, as well as pipes as a form of IPC between a pair of commands.
Completing this project will involve using the UNIX fork(), exec(), wait(), dup2(), and
pipe() system calls and can be completed on any Linux, UNIX, or macOS system.
## Overview
A shell interface gives the user a prompt, after which the next command is entered. The example
below illustrates the prompt osh> and the user’s next command: cat prog.c. (This command
displays the file prog.c on the terminal using the <i>UNIX cat</i> command.)
<h4>osh> cat prog.c</h4>
One technique for implementing a shell interface is to have the parent process first read what
the user enters on the command line (in this case, cat prog.c) and then create a separate child
process that performs the command. Unless otherwise specified, the parent process waits for the
child to exit before continuing. However, UNIX shells typically also allow the child process to run
in the background, or concurrently. To accomplish this, we add an ampersand (&) at the end of the
command. Thus, if we rewrite the above command as
<h4>osh> cat prog.c &</h4>
the parent and child processes will run concurrently. The separate child process is created using
the <i>fork()</i> system call, and the user’s command is executed using one of the system calls in the
<i>exec()</i> family.
The <i>main()</i> function presents the prompt osh> and outlines the steps to be taken after
input from the user has been read. The <i>main()</i> function continually loops as long as should_run
equals 1; when the user enters exit at the prompt, your program will set should_run to 0 and
terminate.



This project is organized into several parts:
1. Creating the child process and executing the command in the child
2. Adding support of input and output redirection
3. Allowing the parent and child processes to communicate via a pipe

## Executing Command in a Child Proces
The first task is to modify the main() function in Figure 1 so that a child process is forked and
executes the command specified by the user. This will require parsing what the user has entered
into separate tokens and storing the tokens in an array of character strings (args in Figure 1). For
example, if the user enters the command ps -ael at the osh> prompt, the values stored in the
args array are:
<h4>args[0] = "ps"</h4>
<h4>args[1] = "-ael"</h4>
<h4>args[2] = NULL</h4>
This args array will be passed to the execvp() function, which has the following prototype:
<h4>execvp(char *command, char *params[])</h4>
Here, command represents the command to be performed and params stores the parameters to
this command. For this project, the execvp() function should be invoked as <i>execvp(args[0],args)</i>. Be sure to check whether the user included & to determine whether or not the parent process
is to wait for the child to exit.

## Redirecting Input and Output
Your shell should then be modified to support the ‘>’ and ‘<’ redirection operators, where ‘>’
redirects the output of a command to a file and ‘<’ redirects the input to a command from a file.
For example, if a user enters
<h4>osh> ls > out.txt</h4>
the output from the ls command will be redirected to the file out.txt. Similarly, input can be
redirected as well. For example, if the user enters
<h4>osh> sort < in.txt</h4>
the file in.txt will serve as input to the sort command. Managing the redirection of both input
and output will involve using the <i>dup2()</i> function, which duplicates an existing file descriptor to
another file descriptor. For example, if fd is a file descriptor to the file out.txt, the call
<h4>dup2(fd, STDOUT_FILENO);</h4>
duplicates <i>fd</i> to standard output (the terminal). This means that any writes to standard output will
in fact be sent to the <i>out.txt</i> file. You can assume that commands will contain either one input or
one output redirection and will not contain both. In other words, you do not have to be concerned
  with command sequences such as <i>sort < in.txt > out.txt.</i>
  
## Communication via a Pipe
The final modification to your shell is to allow the output of one command to serve as input to
another using a pipe. For example, the following command sequence
<h4>osh> ls -l | less</h4>
has the output of the command ls -l serve as the input to the less command. Both the ls and
less commands will run as separate processes and will communicate using the UNIX pipe()
function. Perhaps the easiest way to create these separate processes is to have the parent process
create the child process (which will execute ls -l). This child will also create another child process
(which will execute less) and will establish a pipe between itself and the child process it creates.
Implementing pipe functionality will also require using the dup2() function as described in the
previous section. Finally, although several commands can be chained together using multiple pipes,
you can assume that commands will contain only one pipe character and will not be combined with
any redirection operators.

## 명령어 파싱과 오류 무시
이번 프로젝트는 프로세스 생성과 프로세스간 통신을 이해하는 것이 주요 목적이다. 입력된
명령어를 파싱해서 형식에 어긋나는 명령어 오류를 유연하게 처리하는 것은 필요하지만 이번
주제와는 별개의 문제이므로 여기서는 고려하지 않는다. 문제를 간단하게 하기 위해서 Simple
Shell 사용자는 다음 형식의 명령어만 올바르게 사용한다고 가정하고 명령어를 스캔한다. 아래 형식을 따르지 않은 명령어가 입력되면 전체를 무시하거나 또는 인식된 부분까지만
처리하고 나머지는 버린다.
<ul>
  <li>명령어+옵션 or 명령어+옵션 &</li>
  <li>명령어+옵션 > 파일명 or 명령어+옵션 > 파일명 &</li>
  <li>명령어+옵션 < 파일명 or 명령어+옵션 < 파일명 &</li>
  <li>명령어1+옵션 | 명령어2+옵션 or 명령어1+옵션 | 명령어2+옵션 &</li>
  <li>exit</li>
 </ul>

