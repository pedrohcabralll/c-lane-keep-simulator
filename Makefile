# --- Makefile para o Simulador LISHA (com Threads) ---

# Variáveis para facilitar a manutenção
CC=gcc
CFLAGS=-Wall -Wextra # CFLAGS são as "flags" do compilador. -Wextra mostra ainda mais avisos.
LDFLAGS=-lpthread 

# O alvo "all" é o padrão. Quando você digita "make", é isso que ele tenta construir.
# Ele depende dos alvos "sensor" e "ecu".
all: sensor ecu

# Alvo "sensor": como construir o programa "sensor"
# Ele depende do arquivo "sensor.c"
sensor: sensor.c
	$(CC) $(CFLAGS) -o sensor sensor.c

# Alvo "ecu": como construir o programa "ecu"
# Ele depende do arquivo "controlador_ecu.c"
ecu: controlador_ecu.c
	$(CC) $(CFLAGS) -o ecu controlador_ecu.c $(LDFLAGS)

# Alvo "clean": para limpar os arquivos compilados e recomeçar.
# É uma boa prática ter isso.
clean:
	rm -f sensor ecu telemetria_log.csv 
