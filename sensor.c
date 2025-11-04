#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>

int main() {
    FILE *pipe_sensor;
    FILE *pipe_controle;
    char* pipe_path = "/tmp/sensor_pipe"; 
    char* control_path = "/tmp/control_pipe";

    // Novas Variáveis para a Simulação
    float posicao_pista = 0.0;     // 0.0 é o centro perfeito da pista.
    float desvio_natural = 0.05;   // A cada segundo, o carro "puxa" 5cm para a direita.
    float correcao_volante = -0.10; // O comando "ESQUERDA" corrige 10cm

    char buffer_comando[100];

    printf("[SENSOR] Iniciando. Abrindo os canos...\n");

    // Abrir o "cano" (usando a variável) e dar valor ao pipe_sensor
    pipe_sensor = fopen(pipe_path, "w");
    pipe_controle = fopen(control_path, "r");

    // Checar se o passo 1 falhou
    if (pipe_sensor == NULL || pipe_controle == NULL) {
        perror("Erro ao abrir um dos 'canos'");
        return 1;
    }

    printf("[SENSOR] 'Canos' abertos. Iniciando simulação de veículo.\n");

    while (1) {
        // Lógica da Simulação
        posicao_pista += desvio_natural;

        // Novo Formato de Dado 
        fprintf(pipe_sensor, "POS:%.2f\n", posicao_pista);
        fflush(pipe_sensor);

        printf("[SENSOR] Posição (antes da correção): %.2f. Aguardando ECU...\n", posicao_pista);

       // Esperar e Ler o Comando da ECU
        // O programa para aqui(bloquear) até a ECU responder
        if (fgets(buffer_comando, 100, pipe_controle) != NULL) {
            buffer_comando[strcspn(buffer_comando, "\n")] = 0;

            // Aplicar a Correção
            if (strcmp(buffer_comando, "ESQUERDA") == 0) {
                posicao_pista += correcao_volante; // Aplica a correção (-0.10)
                printf("[SENSOR] ECU mandou: ESQUERDA. Posição corrigida: %.2f\n", posicao_pista);
            } else if (strcmp(buffer_comando, "DIREITA") == 0) {
                posicao_pista -= correcao_volante; // Correção oposta
                printf("[SENSOR] ECU mandou: DIREITA. Posição corrigida: %.2f\n", posicao_pista);
            } else {
                // A ECU mandou "NADA"
                printf("[SENSOR] ECU mandou: NADA. Posição final: %.2f\n", posicao_pista);
            }
        }
        
        printf("--------------------------------------------------\n");
        sleep(1); // Pausa 1 segundo antes de recomeçar o loop
    }


    fclose(pipe_sensor);
    fclose(pipe_controle);
    return 0;
}