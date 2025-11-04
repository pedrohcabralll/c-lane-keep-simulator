#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Para sscanf
#include <unistd.h> // Para sleep
#include <time.h> // Para time()
#include <pthread.h> // Para Threads e Mutex

// Pacote de dados para a telemetria
struct Telemetria
{
    long timestamp;
    float posicao;
    char comando_enviado[20];
};

// Agrupa todo o estado compartilhado entre as threads
// para evitar variáveis globais.
// Uma instância disso será criada na main() e passada para a thread_logger.
struct EcuContext {
    FILE *log_file;               // Ponteiro para o arquivo de log
    struct Telemetria dados_telemetria; // O "pacote" de dados mais recente
    pthread_mutex_t dados_mutex;    // O "cadeado" para proteger os dados
}; 


// Função principal da thread de logging.
// Roda em paralelo com a main() para salvar dados no disco.
void* logger_loop(void* arg) {
    // Recebe o contexto da main() e faz o "cast" do ponteiro
    struct EcuContext *contexto = (struct EcuContext *)arg;
    
    // Cria uma cópia local para evitar segurar o mutex
    struct Telemetria dados_locais; 

    printf("[LOGGER] Thread iniciada. Pronta para logar.\n");

    // Loop principal do Logger
    while(1) {
        // Dorme por 2 segundos.
        sleep(2); 

        // Trava o mutex ANTES de acessar os dados compartilhados
        pthread_mutex_lock(&contexto->dados_mutex);
        
        // Copia os dados da "caixa" compartilhada para a cópia local
        dados_locais = contexto->dados_telemetria; 
        
        // Destranca o mutex
        pthread_mutex_unlock(&contexto->dados_mutex);

        // Usar a cópia local (dados_locais) sem atrapalhar a thread de controle.
        if (dados_locais.timestamp != 0) { // Só loga se já tiver dados
            fprintf(contexto->log_file, "%ld,%.2f,%s\n",
                dados_locais.timestamp, 
                dados_locais.posicao, 
                dados_locais.comando_enviado);
            
            // Força a escrita no disco. Lento, mas não tem problema
            // pois está em uma thread separada.
            fflush(contexto->log_file);
        }
    }

    return NULL; // (Nunca será alcançado)
}


// A função main() atua como a Thread de Controle (tempo real).
// Ela é "dona" de todos os recursos (pipes, arquivos, mutex, threads).
int main() {
    
    // 1. Declaração de Recursos Locais
    
    // Recursos dos Pipes
    FILE *pipe_sensor;
    FILE *pipe_controle; 
    char* sensor_path = "/tmp/sensor_pipe";
    char* control_path = "/tmp/control_pipe";

    // Variáveis de estado do loop
    char buffer[100];
    float posicao_lida;
    char comando_atual[20]; 

    // O "Contexto" que será compartilhado com a thread logger
    struct EcuContext contexto;

    // ID da thread
    pthread_t logger_thread_id; 

    // 2. Inicialização dos Recursos 
    
    printf("[CONTROLE] Iniciando. Abrindo canos e arquivos...\n");

    // Inicializa o "cadeado" (mutex) dentro da struct contexto
    if (pthread_mutex_init(&contexto.dados_mutex, NULL) != 0) {
        perror("Erro ao inicializar mutex");
        return 1;
    }
    
    // Zera os dados de telemetria iniciais
    contexto.dados_telemetria.timestamp = 0; 

    // Abre os canos de comunicação
    pipe_sensor = fopen(sensor_path, "r");
    pipe_controle = fopen(control_path, "w");
    if (pipe_sensor == NULL || pipe_controle == NULL) {
        perror("Erro ao abrir os 'canos' de pipe");
        return 1;
    }

    // Abre o arquivo de log e guarda o ponteiro no contexto
    contexto.log_file = fopen("telemetria_log.csv", "w");
    if (contexto.log_file == NULL) {
        perror("Erro ao criar arquivo de log");
        return 1;
    } 

    // Escreve o cabeçalho do CSV (só é executado se o fopen der certo)
    fprintf(contexto.log_file, "Timestamp,PosicaoLida,ComandoEnviado\n");
    fflush(contexto.log_file);

    printf("[CONTROLE] Recursos inicializados.\n");

    // 3. Criar a Thread de Logger
    // Passa o endereço (&) da struct 'contexto' local
    // para a thread. Agora ela tem acesso ao mutex e ao arquivo de log.
    if (pthread_create(&logger_thread_id, NULL, logger_loop, &contexto) != 0) {
        perror("Erro ao criar a thread de logger");
        return 1;
    }

    printf("[CONTROLE] Thread de Logger criada. Controlador de Pista ATIVADO.\n");

    // 4. Loop Rápido de Controle (Trabalho Principal)
    while(fgets(buffer, 100, pipe_sensor) != NULL) {
        
        // Interpreta o dado do sensor
        if (sscanf(buffer, "POS:%f", &posicao_lida) == 1) {
            
            printf("[CONTROLE] Dado recebido: Posição é %.2f. ", posicao_lida);

            // Lógica de Decisão (deve ser rápida)
            if (posicao_lida > 0.5) {
                printf("AÇÃO: Corrigir ESQUERDA!\n");
                fprintf(pipe_controle, "ESQUERDA\n"); 
                strcpy(comando_atual, "ESQUERDA"); 
            } else if (posicao_lida < -0.5) {
                printf("AÇÃO: Corrigir DIREITA!\n");
                fprintf(pipe_controle, "DIREITA\n");
                strcpy(comando_atual, "DIREITA");
            } else {
                printf("AÇÃO: Manter curso.\n");
                fprintf(pipe_controle, "NADA\n");
                strcpy(comando_atual, "NADA");
            }
            // Envia o comando para o sensor imediatamente
            fflush(pipe_controle);
            
            // 5. Atualiza os dados para o Logger 
            // Trava o "cadeado" antes de mexer nos dados compartilhados
            pthread_mutex_lock(&contexto.dados_mutex);
            
            // Atualiza a "caixa" de dados
            contexto.dados_telemetria.posicao = posicao_lida;
            strcpy(contexto.dados_telemetria.comando_enviado, comando_atual);
            contexto.dados_telemetria.timestamp = time(NULL); 

            // Destranca o "cadeado"
            pthread_mutex_unlock(&contexto.dados_mutex);
        } 
    }
    
    // 6. Limpeza (se o loop principal for interrompido) 
    printf("[CONTROLE] Loop principal encerrado. Fechando tudo.\n");
    fclose(pipe_sensor);
    fclose(pipe_controle);
    fclose(contexto.log_file); // A main fecha o log

    // Cancela a thread logger (que está em loop infinito)
    pthread_cancel(logger_thread_id);
    // Destrói o "cadeado"
    pthread_mutex_destroy(&contexto.dados_mutex);
    
    printf("[CONTROLE] Encerrado.\n");
    return 0;
}