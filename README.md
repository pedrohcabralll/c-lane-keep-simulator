# 🚀 Simulador de ECU para Assistência de Pista (Lane Keeping Assist)

Este projeto é uma simulação, em linguagem C, de um sistema de "Lane Keeping Assist" (Assistência de Manutenção de Pista). Ele foi desenvolvido como um estudo de arquiteturas de sistemas embarcados para veículos autônomos.

O sistema simula a arquitetura de um veículo moderno, com dois processos independentes (Veículo/Sensor e ECU/Controlador) que rodam concorrentemente e se comunicam em tempo real.

## 🍿 Demonstração

![Demonstração do Simulador nos 3 Terminais](https://github.com/pedrohcabralll/c-lane-keep-simulator/blob/main/demonstracao.png?raw=true)

* **Terminal do Sensor (Veículo):** Simula o carro "puxando" para a direita e, em seguida, sendo corrigido pela ECU.
* **Terminal da ECU (Controlador):** Lê os dados do sensor e toma decisões em tempo real, enviando comandos de correção.
* **Terminal do Logger:** Mostra o arquivo `telemetria_log.csv` sendo preenchido pela thread de logging da ECU.

## 🏛️ Arquitetura do Sistema

O sistema é composto por dois programas em C que rodam concorrentemente e se comunicam através de **Named Pipes (FIFO)** do Linux, simulando uma arquitetura de IPC (Inter-Process Communication) real.

1.  **`./sensor` (O Veículo):**
    * Simula a física do veículo (um "desvio natural" constante para a direita).
    * Atua como o **sensor** (enviando a posição na pista) e o **atuador** (recebendo e aplicando correções do volante).
    * Comunica-se via `/tmp/sensor_pipe` (escrita) e `/tmp/control_pipe` (leitura).

2.  **`./ecu` (O "Cérebro" / Controlador):**
    * Simula a **ECU (Electronic Control Unit)** do carro.
    * **Thread de Controle (Main):** Um loop de controle de alta prioridade que lê os dados do sensor, aplica a lógica de decisão (`if/else`) e envia comandos de correção.
    * **Thread de Logger (Background):** Uma thread `pthread` separada que roda em paralelo, responsável por coletar os dados de telemetria e salvá-los em `telemetria_log.csv` a cada 2 segundos.
    * **Segurança de Concorrência:** Um **`Mutex`** (`pthread_mutex_t`) é usado para proteger a memória compartilhada entre a thread de controle e a de logger, evitando *race conditions*.

## 🛠️ Tecnologias e Conceitos Utilizados

* **Linguagem:** C (C99)
* **Compilação:** `make` e `gcc` (com um `Makefile` customizado)
* **Sistema Operacional:** Focado em Linux (testado no Ubuntu)
* **Comunicação entre Processos (IPC):** Named Pipes (FIFO)
* **Concorrência:** `pthreads` (Multithreading) e `pthread_mutex_t` (Mutex)
* **I/O de Sistema:** `fopen`, `fprintf`, `fgets`, `fflush`
* **Parsing de Dados:** `sscanf`, `strcmp`, `strcpy`

## ⚙️ Como Compilar e Rodar

1.  **Pré-requisitos:**
    É necessário ter o `gcc`, `make` e `git` instalados.
    ```bash
    sudo apt install build-essential git
    ```

2.  **Clonar o Repositório:**
    ```bash
    git clone https://github.com/pedrohcabralll/c-lane-keep-simulator.git
    cd c-lane-keep-simulator
    ```

3.  **Compilar o Projeto:**
    O `Makefile` compila os dois programas e linka a biblioteca `pthread`.
    ```bash
    make clean
    make
    ```

4.  **Executar a Simulação:**
    É necessário ter **3 terminais** abertos na pasta do projeto.

    * **Terminal 1 (Criar os "Canos"):**
        ```bash
        rm -f /tmp/sensor_pipe /tmp/control_pipe
        mkfifo /tmp/sensor_pipe /tmp/control_pipe
        ```
        *(Este passo só é necessário na primeira vez, ou se os canos forem apagados)*

    * **Terminal 1 (Rodar a ECU):**
        ```bash
        ./ecu
        ```
        *(A ECU irá iniciar e aguardar o sensor)*

    * **Terminal 2 (Rodar o Sensor):**
        ```bash
        ./sensor
        ```
        *(A simulação começará, e o carro "dançará" em torno da posição 0.50)*

    * **Terminal 3 (Ver o Log):**
        ```bash
        # Espere 10s e depois rode:
        cat telemetria_log.csv
        ```