# Compilador e flags
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# Nome do arquivo fonte e do executável
SRC     = src/seq_k_means.c
TARGET  = exe/seq_k_means

# Alvos padrões
.PHONY: all run clean

# Alvo padrão: compilar o executável
all: $(TARGET)

# Regras de compilação
$(TARGET): $(SRC)
	@mkdir -p exe
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Alvo para executar o programa
run: $(TARGET)
	./$(TARGET)

# Alvo para limpar os arquivos gerados
clean:
	rm -f $(TARGET)
