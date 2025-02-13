#include <stdio.h> // biblioteca que contém as funções printf, scanf ...
#include <stdlib.h> // biblioteca que contém as funções malloc e free
#include <unistd.h> // biblioteca que contém as funções fork, execvp ...
#include <sys/wait.h> // biblioteca que contém a função waitpid
#include <string.h> // biblioteca que contém as funções strcmp, strncmp ...
#include <dirent.h> // biblioteca que contém as funções opendir, readdir ...
#include <fcntl.h> // biblioteca que contém as funções open, close ...
#include <ctype.h> // biblioteca que contém a função isdigit
#include <pwd.h> // biblioteca que contém a função getpwuid
#include <time.h> // biblioteca que contém a função sleep
#include <sys/stat.h> // biblioteca que contém a função stat

typedef struct {
    int found;
    const char *operator;
    int argc_cmd1;
    char **argv_cmd1;
    int argc_cmd2;
    char **argv_cmd2;
} comands;

comands parse(char *argv[], int argc) {
    comands result;

    result.found = 0;
    result.operator = NULL;
    result.argc_cmd1 = 0;
    result.argc_cmd2 = 0;
    result.argv_cmd1 = NULL;
    result.argv_cmd2 = NULL;

    int found = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0 || strcmp(argv[i], "|") == 0) {
            found = 1;
            result.found = 1;
            result.operator = argv[i];
            continue;
        }

        if (!found) {
            // Adiciona argumentos no contexto do cmd1
            result.argv_cmd1 = realloc(result.argv_cmd1, (result.argc_cmd1 + 1) * sizeof(char *));
            result.argv_cmd1[result.argc_cmd1] = strdup(argv[i]);
            result.argc_cmd1++;
        } else {
            // Adiciona argumentos no contexto do cmd2 
            result.argv_cmd2 = realloc(result.argv_cmd2, (result.argc_cmd2 + 1) * sizeof(char *));
            result.argv_cmd2[result.argc_cmd2] = strdup(argv[i]);
            result.argc_cmd2++;
        }
    }
    result.argv_cmd1[result.argc_cmd1] = NULL;
    if (found) result.argv_cmd2[result.argc_cmd2] = NULL;

    return result;
}

void executar(comands cmd) {
    if (!cmd.found) {
        // Execução simples de comando
        pid_t pid = fork();

        if (pid == -1) {
            perror("Falha ao criar processo");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Filho
            execvp(cmd.argv_cmd1[0], cmd.argv_cmd1); // execvp -> executa o comando que está no primeiro argumento e passa os argumentos que estão no segundo argumento
            perror("Falha na execução do comando");
            exit(EXIT_FAILURE); 
        } else {
            // Pai
            waitpid(pid, NULL, 0); // waitpid -> espera que o processo filho termine
        }

    } else {
        if (strcmp(cmd.operator, ">") == 0) {
            // Redirecionamento de saída
            pid_t pid = fork();

            if (pid == -1) {
                perror("Falha ao criar processo");  
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                // Filho
                freopen(cmd.argv_cmd2[0], "w", stdout); // freopen -> redireciona o output para o ficheiro que está no segundo argumento
                execvp(cmd.argv_cmd1[0], cmd.argv_cmd1); 
                perror("Falha na execução do comando");
                exit(EXIT_FAILURE);
            } else {
                // Pai
                waitpid(pid, NULL, 0);
            }

        } else if (strcmp(cmd.operator, "<") == 0) {
            // Redirecionamento de entrada
            pid_t pid = fork();

            if (pid == -1) {
                perror("Falha ao criar processo");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                // Filho
                freopen(cmd.argv_cmd2[0], "r", stdin); // freopen -> redireciona o input para o ficheiro que está no segundo argumento
                execvp(cmd.argv_cmd1[0], cmd.argv_cmd1);
                perror("Falha na execução do comando");
                exit(EXIT_FAILURE);
            } else {
                // Pai
                waitpid(pid, NULL, 0);
            }
        } else if (strcmp(cmd.operator, "|") == 0) {
            // Pipe
            int pipe_fd[2];
            if (pipe(pipe_fd) == -1) {
                perror("Falha ao criar pipe");
                exit(EXIT_FAILURE);
            }

            pid_t pid1 = fork();
            if (pid1 == -1) {
                perror("Falha ao criar processo");
                exit(EXIT_FAILURE);
            }

            if (pid1 == 0) {
                // Filho 1
                close(pipe_fd[0]); // fecha o descritor de leitura
                dup2(pipe_fd[1], STDOUT_FILENO); // dup2 -> duplica o descritor de escrita para o descritor de escrita do pipe
                close(pipe_fd[1]); // fecha o descritor de escrita
                execvp(cmd.argv_cmd1[0], cmd.argv_cmd1); 
                perror("Falha na execução do comando");
                exit(EXIT_FAILURE);
            }

            pid_t pid2 = fork();
            if (pid2 == -1) {
                perror("Falha ao criar processo");
                exit(EXIT_FAILURE);
            }

            if (pid2 == 0) {
                // Filho 2
                close(pipe_fd[1]); // fecha o descritor de escrita
                dup2(pipe_fd[0], STDIN_FILENO); // dup2 -> duplica o descritor de leitura para o descritor de leitura do pipe
                close(pipe_fd[0]); // fecha o descritor de leitura
                execvp(cmd.argv_cmd2[0], cmd.argv_cmd2);
                perror("Falha na execução do comando");
                exit(EXIT_FAILURE);
            }

            // Pai
            close(pipe_fd[0]); // fecha o descritor de leitura
            close(pipe_fd[1]); // fecha o descritor de escrita
            waitpid(pid1, NULL, 0); // espera que o filho 1 termine
            waitpid(pid2, NULL, 0); // espera que o filho 2 termine
        }
    }
}

void executarTOP(){
    FILE *loadAverage = fopen("/proc/loadavg", "r"); // abre o ficheiro loadAverage em modo de leitura
    
    if(loadAverage == NULL){ // se o ficheiro não existir
        perror("Erro ao abrir o ficheiro loadAverage");
        exit(EXIT_FAILURE);

    } else {
        double load1minuto, load5minutos, load15minutos; // variaveis para guardar os valores do loadAverage
        fscanf(loadAverage, "%lf %lf %lf", &load1minuto, &load5minutos, &load15minutos); // guarda os valores do loadAverage nas variaveis
        printf("Comando TOP:\n");
        printf("Load Average do Sistema: %.2f %.2f %.2f ||", load1minuto, load5minutos, load15minutos); // imprime os valores 
        fclose(loadAverage); // fecha o ficheiro
    }

    FILE *stat = fopen("/proc/stat", "r"); // abre o ficheiro stat em modo de leitura

    if(stat == NULL){
        perror("Erro ao abrir o ficheiro stat");
        exit(EXIT_FAILURE);
    } else {
        int totalProcessos, processosAExecutar; // variaveis para guardar os valores de processos e processos a executar
        char linha[256]; //256 -> tamanho da linha

        while(fgets(linha, sizeof(linha), stat) != NULL){
            if(strncmp(linha, "processes", strlen("processes")) == 0){ // se a linha começar por processes 
                sscanf(linha, "processes %d", &totalProcessos); // guarda o valor de processos na variavel totalProcessos

            } else if(strncmp(linha, "procs_running", strlen("procs_running")) == 0){
                sscanf(linha, "procs_running %d", &processosAExecutar); 
            }
        }
        fclose(stat); // fecha o ficheiro
        printf("Processos: %d ||", totalProcessos);
        printf("Processos em Execução: %d ", processosAExecutar);
    }
    
    DIR *dir = opendir("/proc");

    if (dir == NULL) {
        perror("Erro ao abrir a diretoria /proc");
        exit(EXIT_FAILURE);
    }

    printf("\nProcessos:\n");
    int cont = 0;
    struct dirent *ent; // estrutura que contém o nome do ficheiro e o inode
    int pids[20]; // Array para armazenar os PIDs dos processos em execução
    int numProcessos = 0; // Contador para o número de processos em execução

    while ((ent = readdir(dir)) != NULL && numProcessos < 20) {
        if (isdigit(*ent->d_name)) { // se o primeiro caractere do nome do ficheiro for um dígito
            char estado = ' ';
            int uid;
            int pid = atoi(ent->d_name); // atoi -> converte uma string para um inteiro, ent->d_name -> nome do ficheiro
            char path[256];

            sprintf(path, "/proc/%d/status", pid); // sprintf -> escreve para uma string o que está no segundo argumento e guarda na variavel path
            FILE *status = fopen(path, "r");

            if (status == NULL) {
                perror("Erro ao abrir o ficheiro status");
                exit(EXIT_FAILURE);

            } else {
                char linha[256];

                while (fgets(linha, sizeof(linha), status) != NULL) { // fgets -> lê uma linha do ficheiro e guarda na variavel linha
                if (strncmp(linha, "State", strlen("State")) == 0) { // strncmp -> compara os primeiros n caracteres de duas strings, neste caso n = tamanho de "State"
                    sscanf(linha, "State: %c", &estado); // sscanf -> lê o que está na variavel linha e guarda na variavel estado
                
                } else if (strncmp(linha, "Uid", strlen("Uid")) == 0) {
                    sscanf(linha, "Uid: %d", &uid);
                    }
                }
                fclose(status);
            }

            if (uid != -1) {
                pids[numProcessos] = pid; // Armazena o PID no array
                numProcessos++;
            }
        }
    }

    rewinddir(dir); // Volta ao início da diretoria /proc
    closedir(dir); // Fecha a diretoria /proc

    // Ordena os PIDs em ordem crescente
    for (int i = 0; i < numProcessos - 1; i++) {
        for (int j = 0; j < numProcessos - i - 1; j++) {
            if (pids[j] > pids[j + 1]) {
                int temp = pids[j]; // Troca os valores
                pids[j] = pids[j + 1];
                pids[j + 1] = temp; 
            } 
        }
    }

    // Imprime os dados dos processos
    for (int i = 0; i < numProcessos; i++) {
        int pid = pids[i]; 
        char path[256];

        sprintf(path, "/proc/%d/status", pid); // sprintf -> escreve para uma string o que está no segundo argumento e guarda na variavel path
        FILE *status = fopen(path, "r"); // abre o ficheiro status em modo de leitura

        if (status == NULL) {
            perror("Erro ao abrir o ficheiro status");
            exit(EXIT_FAILURE);

        } else {
            char linha[256];
            char estado = ' ';
            int uid = -1;

            while (fgets(linha, sizeof(linha), status) != NULL) {
                if (strncmp(linha, "State", strlen("State")) == 0) {
                     sscanf(linha, "State: %c", &estado);
                     
                } else if (strncmp(linha, "Uid", strlen("Uid")) == 0) {
                                sscanf(linha, "Uid: %d", &uid);
                            }
                }
                fclose(status);

                if (uid != -1) {
                    char path[256];
                    sprintf(path, "/proc/%d/cmdline", pid);
                    FILE *cmdline = fopen(path, "r");

                    if (cmdline == NULL) {
                        perror("Erro ao abrir o ficheiro cmdline");
                        exit(EXIT_FAILURE);
                    } else {
                        char linha[256];
                        char linhaComando[256];
                        
                        while (fgets(linha, sizeof(linha), cmdline) != NULL) {
                            sscanf(linha, "%s", linhaComando);
                        }
                        fclose(cmdline);
                        printf("Nome da Linha de Comando: %s", linhaComando);
                    }

                    struct passwd *user_info = getpwuid(uid);
                    
                    if (user_info != NULL) {
                        printf("||Nome do Utilizador: %s", user_info->pw_name);
                    }
                    
                printf("||PID: %d", pid);
                printf("||Estado: %c", estado);
                printf("\nProcesso nº %d\n", i + 1);

                }
        }
    }
    sleep(10);
}

void libertarMemoria(comands cmd) {
    for (int i = 0; i < cmd.argc_cmd1; i++) { // liberta a memoria alocada para os argumentos do cmd1
        free(cmd.argv_cmd1[i]);
    }
    free(cmd.argv_cmd1);

    for (int i = 0; i < cmd.argc_cmd2; i++) { // liberta a memoria alocada para os argumentos do cmd2
        free(cmd.argv_cmd2[i]);
    }
    free(cmd.argv_cmd2);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Insira: %s <comando>\n", argv[0]); 
        return 1;
    }

    comands cmd = parse(argv + 1, argc - 1);

    if (strcmp(cmd.argv_cmd1[0], "top") != 0) {
        executar(cmd);
        libertarMemoria(cmd);
    } else { 
        char q = ' ';
        do{
            executarTOP();
            printf("\nPrima q  se quiser sair: ");
            scanf(" %c", &q);
            printf("\n");
        } while(q != 'q');
    }
        

    return 0;
}
