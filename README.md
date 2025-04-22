# K-Means

Este repositório contém as implementações sequencial e paralela do algoritmo K-Means. Para executar o algoritmo, pode-se utilizar o comando `make` para invocar as diretivas especificadas no arquivo [Makefile](Makefile).

Para analisar melhor o impacto da paralelização em diferentes regiões do algoritmo, foram salvas as versões incrementais do algoritmo paralelizado. A terceira versão é aquela que apresenta o melhor resultado.
- [v1](src/par_k_means_v1.c)
- [v2](src/par_k_means_v2.c)
- [v3](src/par_k_means_v3.c)

## Rodar Código Sequencial

- make VERSION=seq
- make run VERSION=seq THREADS=1 MODE=X

## Rodar Código Paralelo

- make VERSION=par
- make run VERSION=par THREADS=X MODE=X
