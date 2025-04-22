CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -std=c99 -fopenmp  # <— agora o -fopenmp sempre
LDFLAGS  := -lm

VERSION  ?= seq
THREADS  ?= 1

ifeq ($(VERSION),par)
    SRC     := src/par_k_means_v3.c # Trocar a versão conforme o teste a ser realizado (v3 padrão - a melhor)
    TARGET  := exe/kmeans_par
else ifeq ($(VERSION),seq)
    SRC     := src/seq_k_means.c
    TARGET  := exe/kmeans_seq
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: all
ifeq ($(VERSION),seq)
	@echo "---> Executando versão SEQUENCIAL com $(THREADS) threads"
	@./$(TARGET) $(THREADS)
else
	@echo "---> Executando versão PARALELA com $(THREADS) threads"
	@./$(TARGET) $(THREADS)
endif

clean:
	rm -rf exe/kmeans_*
