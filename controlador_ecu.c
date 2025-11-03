#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Necessário para sscanf
#include <unistd.h> // Função sleep()
#include <time.h> //Timestamp do log
#include <pthread.h> // Biblioteca de Threads

// --- "Armazém" de Dados Global ---
// Esta é a "caixa" onde a Thread de Controle (main)
// vai deixar os dados para a Thread de Logger pegar
struct Telemetria
{
    long timestamp;
    float posicao;
    char comando_enviado[20];
};

// Variáveis globais:
struct Telemetria g_dados_telemetria;
pthread_mutex_t g_dados_mutex; // "Cadeado" (Mutex) para proteger os dados

/**@brief Esta é a função da Thread de Logger.
 * Roda em paralelo com a main() e seu único trabalho
 * é acordar, pegar os dados e salvar no arquivo log
 */ 
void* logger_loop(void* arg) {
    FILE *log_file;
    struct Telemetria dados_locais; // Cópia local para não segurar o "cadeado"

    printf("[LOGGER] Thread iniciada. Abrindo arquivo de log...\n");

    log_file = fopen("telemetria_log.csv", "w");
    if (log_file == NULL) {
        perror("Erro ao criar arquivo de log");
        return NULL;
    }

    // Cabeçalho do nosso arquivo CSV
    fprintf(log_file, "Timestamp, PosicaoLida, ComandoEnviado\n");
    fflush(log_file);

    // Loop infinito da Thread de Logger
    while(1) {
        sleep(2);

        // Trava o "cadeado" para poder ler os dados globais com segurança
        pthread_mutex_lock(&g_dados_mutex);

        // Copia os dados globais para a variável local para liberar o "cadeado" o mais rápido possível
        dados_locais = g_dados_telemetria;

        // Destranca o "cadeado
        pthread_mutex_unlock(&g_dados_mutex);

        // Escreve os dados da cópia local no arquivo de log
        if (dados_locais.timestamp != 0) { // Só loga se tiver dados
            fprintf(log_file, "%ld,%.2f,%s\n",
                dados_locais.timestamp,
                dados_locais.posicao,
                dados_locais.comando_enviado);

            // Força a escrita no disco 
            fflush(log_file);

        }
    }

    fclose(log_file);
    return NULL;
}

/**
 * @brief A função main() agora é a Thread de Controle
 * Mais rápida possível
 */

int main() {
    // Dois Canos 
    FILE *pipe_sensor;
    FILE *pipe_controle; // Ponteiro para o "cano" de controle
    char* sensor_path = "/tmp/sensor_pipe";
    char* control_path = "/tmp/control_pipe"; // Caminho do "cano" de controle

    char buffer[100];
    float posicao_lida;
    char comando_atual[20]; // Guarda o comando decidido neste ciclo
    
    // Inicialização
    printf("[CONTROLE] Iniciando. Abrindo canos...\n");

    // Inicia o "cadeado" (Mutex)
    if (pthread_mutex_init(&g_dados_mutex, NULL) != 0) {
        perror("Erro ao inicializar mutex");
        return 1;
    }

    // Zera os dados de telemetria iniciais
    g_dados_telemetria.timestamp = 0;
        
    // Abre os dois canos: um para ler, outro para escrever
    pipe_sensor = fopen(sensor_path, "r");
    pipe_controle = fopen(control_path, "w"); // Abre o "cano" de controle em modo escrita ("w")

    // Checar se AMBOS abriram 
    if (pipe_sensor == NULL || pipe_controle == NULL) {
        perror("Erro ao abrir um dos 'canos'");
        return 1;
    }

    printf("[CONTROLE] 'Canos' abertos.");

    // Criar e "Ligar" a Thread de Logger
    pthread_t logger_thread_id; // Variável para guardar o ID da thread
    if (pthread_create(&logger_thread_id, NULL, logger_loop, NULL) != 0) {
        perror("Erro ao criar a thread de logger");
        return 1;
    }

    printf("[CONTROLE] Thread de Logger criada. Controlador de Pista ATIVADO.\n");

    // Loop rápido de controle (o trabalho da main)
    while (fgets(buffer, 100, pipe_sensor) != NULL) {
        
        if (sscanf(buffer, "POS:%f", &posicao_lida) == 1) {
            
            printf("[CONTROLE] Dado recebido: Posição é %.2f. ", posicao_lida);

            // Lógica de Decisão (Rápida)
            if (posicao_lida > 0.5) {
                printf("AÇÃO: Corrigir ESQUERDA!\n");
                fprintf(pipe_controle, "ESQUERDA\n"); // Envia o comando
                strcpy(comando_atual, "ESQUERDA"); // Salva o comando
            } else if (posicao_lida < -0.5) {
                printf("AÇÃO: Corrigir DIREITA!\n");
                fprintf(pipe_controle, "DIREITA\n"); // Envia o comando
                strcpy(comando_atual, "DIREITA");
            } else {
                printf("AÇÃO: Manter curso.\n");
                fprintf(pipe_controle, "NADA\n"); // Envia o comando
                strcpy(comando_atual, "NADA");
            }
            
            // Dar "flush" no cano de controle
            fflush(pipe_controle);
            
            // Atualizar os Dados Globais para o Logger
            // Trava o "cadeado"
            pthread_mutex_lock(&g_dados_mutex);

            // Atualiza a "caixa de dados"
            g_dados_telemetria.posicao = posicao_lida;
            strcpy(g_dados_telemetria.comando_enviado, comando_atual);
            g_dados_telemetria.timestamp = time(NULL); // Pega o tempo atual

            // Destranca o cadeado
            pthread_mutex_unlock(&g_dados_mutex);
            // Fim da atualização
        } 
    }

    // Limpeza (se um dia o loop acabar)
    printf("[CONTROLE] Loop principal encerrado. Fechando canos.\n");
    fclose(pipe_sensor);
    fclose(pipe_controle); // Fecha o segundo "cano"

    // Manda a thread de logger parar (embora ela esteja em loop infinito)
    pthread_cancel(logger_thread_id);
   
    // Destrói o cadeado
    pthread_mutex_destroy(&g_dados_mutex);

    printf("[CONTROLE] 'Canos' fechados. Encerrando.");
    return 0;
}