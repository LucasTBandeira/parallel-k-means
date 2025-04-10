#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define NUM_POINTS 10000000 // Número total de pontos
#define K 25                // Número de clusters (centróides)
#define MAX_ITER 250        // Número máximo de iterações permitidas

// Estrutura que representa um ponto 2D e o cluster ao qual pertence.
typedef struct {
    double x;
    double y;
    int cluster;
} Point;

// Estrutura que representa um centróide em 2D.
typedef struct {
    double x;
    double y;
} Centroid;

// Função para calcular a distância euclidiana entre um ponto e um centróide.
double distance(Point p, Centroid c) {
    double dx = p.x - c.x;
    double dy = p.y - c.y;
    return sqrt(dx * dx + dy * dy);
}

int main() {
    // Aloca o vetor de pontos
    Point *points = malloc(NUM_POINTS * sizeof(Point));
    if (points == NULL) {
        printf("Erro ao alocar memória para os pontos.\n");
        return 1;
    }
    
    Centroid centroids[K];

    // Inicializa a semente para números aleatórios
    srand(time(NULL));

    // Geração aleatória dos pontos (intervalo [0, 100] para x e y)
    for (int i = 0; i < NUM_POINTS; i++) {
        points[i].x = (double)rand() / RAND_MAX * 100.0;
        points[i].y = (double)rand() / RAND_MAX * 100.0;
        points[i].cluster = -1;  // Inicialmente, o ponto não pertence a nenhum cluster
    }
    
    // Inicializa os centróides escolhendo aleatoriamente pontos gerados
    for (int i = 0; i < K; i++) {
        int index = rand() % NUM_POINTS;
        centroids[i].x = points[index].x;
        centroids[i].y = points[index].y;
    }
    
    int iterations = 0;
    int changed = 1;  // Flag que indica se houve alteração nas atribuições dos pontos
    
    // Abre o arquivo de log (modo append)
    FILE *logFile = fopen("execution.log", "a");
    if (logFile == NULL) {
        printf("Erro ao abrir o arquivo de log.\n");
        free(points);
        return 1;
    }
    
    // Loga um cabeçalho com o timestamp da execução
    time_t now = time(NULL);
    char *timestamp = ctime(&now); // ctime já adiciona uma quebra de linha
    fprintf(logFile, "===========================\n");
    fprintf(logFile, "Início da execução: %s", timestamp);
    
    // Medição do tempo total de execução
    clock_t overall_start = clock();
    
    // Loop principal do algoritmo k-means
    while (changed && iterations < MAX_ITER) {
        // Registra o início da iteração
        clock_t iter_start = clock();
        
        changed = 0;
        
        // PASSO 1: Atribuição dos pontos para o centróide mais próximo
        for (int i = 0; i < NUM_POINTS; i++) {
            double minDist = distance(points[i], centroids[0]);
            int bestCluster = 0;
            for (int j = 1; j < K; j++) {
                double d = distance(points[i], centroids[j]);
                if (d < minDist) {
                    minDist = d;
                    bestCluster = j;
                }
            }
            if (points[i].cluster != bestCluster) {
                points[i].cluster = bestCluster;
                changed = 1;
            }
        }
        
        // PASSO 2: Atualização dos centróides (média dos pontos de cada cluster)
        double sumX[K] = {0};
        double sumY[K] = {0};
        int count[K] = {0};  // Contador de pontos para cada cluster nesta iteração
        for (int i = 0; i < NUM_POINTS; i++) {
            int cl = points[i].cluster;
            sumX[cl] += points[i].x;
            sumY[cl] += points[i].y;
            count[cl]++;
        }
        for (int j = 0; j < K; j++) {
            if (count[j] != 0) {
                centroids[j].x = sumX[j] / count[j];
                centroids[j].y = sumY[j] / count[j];
            }
        }
        
        // Registra o fim da iteração e o tempo de duração
        clock_t iter_end = clock();
        double iter_duration = (double)(iter_end - iter_start) / CLOCKS_PER_SEC;
        
        // Loga os detalhes da iteração
        fprintf(logFile, "Iteração %d:\n", iterations + 1);
        fprintf(logFile, "   Início: %.4f s, Término: %.4f s, Duração: %.4f s\n",
                (double)iter_start / CLOCKS_PER_SEC, (double)iter_end / CLOCKS_PER_SEC, iter_duration);
        for (int j = 0; j < K; j++) {
            fprintf(logFile, "   Cluster %d: Centróide (%.4f, %.4f), Pontos: %d\n",
                    j, centroids[j].x, centroids[j].y, count[j]);
        }
        fprintf(logFile, "-----------------------------------\n");
        fflush(logFile);  // Garante que o log seja escrito no arquivo a cada iteração
        
        iterations++;
    }
    
    // Registra o fim do tempo total de execução
    clock_t overall_end = clock();
    double total_duration = (double)(overall_end - overall_start) / CLOCKS_PER_SEC;
    
    // Exibe os resultados finais no console
    printf("K-means convergiu em %d iterações.\n", iterations);
    for (int j = 0; j < K; j++) {
        // Recalcula a contagem final para cada cluster
        int final_count = 0;
        for (int i = 0; i < NUM_POINTS; i++) {
            if (points[i].cluster == j) final_count++;
        }
        printf("Centróide %d: (%.4f, %.4f) com %d pontos.\n", j, centroids[j].x, centroids[j].y, final_count);
    }
    printf("Tempo total de execução: %.4f segundos\n", total_duration);
    
    // Loga o resumo final da execução
    fprintf(logFile, "Resumo final:\n");
    fprintf(logFile, "K-means convergiu em %d iterações.\n", iterations);
    fprintf(logFile, "Tempo total de execução: %.4f segundos\n", total_duration);
    for (int j = 0; j < K; j++) {
        int final_count = 0;
        for (int i = 0; i < NUM_POINTS; i++) {
            if (points[i].cluster == j) final_count++;
        }
        fprintf(logFile, "   Cluster %d: Centróide (%.4f, %.4f), Pontos: %d\n",
                j, centroids[j].x, centroids[j].y, final_count);
    }
    fprintf(logFile, "\n");
    
    fclose(logFile);
    
    // Exemplo: exibe as atribuições dos 10 primeiros pontos
    printf("\nExemplo de atribuição dos 10 primeiros pontos:\n");
    for (int i = 0; i < 10; i++) {
        printf("Ponto %d: (%.2f, %.2f) -> Cluster %d\n", 
               i, points[i].x, points[i].y, points[i].cluster);
    }
    
    free(points);
    return 0;
}
