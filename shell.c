//Alunos: João Otávio Langer de Moraes (811797)
//        João Paulo Migliatti (802534)

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"
#include "limits.h"
#include "fcntl.h"

//Tamanho máximo da linha de comando
#define CMD_LENGTH 128

//Essa função pega a linha de comando de input do usuário
int read_commands(char *cmd){
    char c;
    int i = 0;
    while (1){
        c = fgetc(stdin);
        cmd[i] = c;
        if (c == '\n'){ //Verifica se o usuário digitou a tecla ENTER
            cmd[i] = '\0';
            break;
        }
        i++;
    }
    return 0;
}

//Essa função quebra a linha de comando em suas palavras usando o caractere ESPAÇO como delimitador
int split_commands(char *cmd, char **args){
    int count = 0;

    char *delim = " ";
    char *token;
    
    token = strtok(cmd, delim); //Função que separa as palavras

    while(token != NULL){
        args[count++] = token;
        token = strtok(NULL, delim);
    }
    args[count] = NULL; //Define o próximo ponteiro como NULL para ser recebido pela função execvp
    return 0;
}

//Essa função varre o vetor de strings procurando palavras reservadas
int sweep(char **args, int *bg, int *redir, int *is_cd){
    int i = 0;
    int flag = 1;

    if (strcmp(args[i], "cd") == 0){ //Tratamento para o comando cd
        *is_cd = 1;
    }
    while (args[i] != NULL){
        if (strcmp(args[i], "exit") == 0){ //A palavra exit termina a execução do terminal 
            exit(1);
        }
        if (flag && (strcmp(args[i], "&") == 0)){ //A palavra & joga o programa para background
            args[i] = NULL;
            *bg = 1;
            flag = 0;
        }
        if (flag && (strcmp(args[i], ">")) == 0){ //A palavra reservada > redireciona a saida para um arquivo
            args[i] = NULL;
            *redir = i+1;
            flag = 0;
        }
        i++;
    }

    return 0;
}

//Tentativa de uso de função para tratamento de sinal mas gerava erros no código
/*
void sigchild_handler(int sinal){
    pid_t filho;
    int status;
    if(sinal == SIGCHLD){
        filho = waitpid(-1, &status, WNOHANG);
        if(filho < 0){
            perror("Erro no waitpid");
            exit(1);
        }
        if(filho == 0){
            return;
        }

    }
    else{
        perror("Error no SIGCHLD handler, o valor não é sigchild");
    }
    return;
}
*/

//Essa função cria um processo
int create_task(char **args, int bg, int redir, int is_cd){
    int error = 0;
    pid_t son_pid = fork(); //Cria um fork no processo e guarda o pid 
    int file;

    if(son_pid == 0){ //Processo filho
        if (redir > 0){
            file = open(args[redir], O_CREAT | O_TRUNC | O_RDWR, 0666); //Função que abre um arquivo 
            if (file == -1){                                            //para redirecionamento
                perror ("Falha ao abrir o arquivo");
                exit(1);
            }
            if (dup2(file, 1) == -1){ //Função redireciona o output -1 é o valor vetor de saídado arquivo
                perror("Erro ao redirecionar o arquivo");
                exit(1);
            }
        }

        if (is_cd){
            if(args[1] == NULL){
                printf ("Digite um diretório válido\n");
            }else{
                chdir(args[1]); //A função chdir executa o código cd
            }
        }else {
            error = execvp(args[0], args);
        }
        if (error == -1){
            perror("Erro na execução do programa\n");
            exit(1);
        }
        if (redir > 0){
            fflush(stdout); //Limpa o buffer de saída para garantir que o arquivo foi escrito corretamente
            close(file);
        }
    }else if(son_pid > 0){ //Processo pai
        if (!bg){ //Espera o filho terminar caso não seja background
            wait(NULL);
        }
        if (is_cd){
            exit(1); //Exit adicional para terminar o processo pai ao executar um cd
        }

    }else{
        perror("Erro na criação do fork\n");
        exit(1);
    }
    return 0;
}

//Essa função imprime o diretório corrente
int current_directory(){
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL){ //Função getcwd retorna NULL caso não consiga copiar o diretório
        printf("\033[1;35m%s\033[0m", cwd);
    }
    return 0;
}

//Essa função imprime o nome do usuário corrente
int get_userid(){
    char userid[30];

    if (getlogin_r(userid, sizeof(userid)) == 0){
        printf("\033[1;37m%s\033[0m:", userid);
    }
    return 0;
}

//Função para chamadas de funções, loop, e eventuais alocações
int shell(){
    char *cmd;
    char **args;
    int bg;
    int redir;
    int is_cd;
    while(1){
        bg = 0;
        redir = 0;
        is_cd  =0;
        get_userid();
        current_directory();
        printf ("$ ");
        cmd = (char*)malloc(CMD_LENGTH * sizeof(char));
        if (cmd == NULL){
            perror("Erro na alocação do vetor\n");
            exit(1);
        }
        read_commands(cmd);
        int tam = strlen(cmd);
        args = (char**)malloc((tam+1) * sizeof(char*));
        if (args == NULL){
            perror("Erro na alocação de memória\n");
            exit(1);
        }
        split_commands(cmd, args);
        sweep(args, &bg, &redir, &is_cd);
        create_task(args, bg, redir, is_cd);
        signal(SIGCHLD, SIG_IGN); //Uso de SIG_IGN para tratamento de sinal
        free(args);
        free(cmd);
    }
    return 0;
}

int main(){
    shell();
    return 0;
}