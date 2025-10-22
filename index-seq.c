#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define ARQUIVO "jewelry_small.csv"
#define ARQ_PRODUTOS "produtos.bin"
#define ARQ_PEDIDOS "pedidos.bin"
#define INDEX_PROD "index_prod.idx"
#define INDEX_PED "index_ped.idx"
#define ARQ_OVERFLOW_PROD "produtos.ovf"
#define ARQ_OVERFLOW_PED "pedidos.ovf"
#define TAM_INTERVALO 10

/* REGISTRO INTEIRO DO ARQUIVO CSV - SE FOSSE O CASO DE TRAZER ELE INTEIRO PARA A MEMORIA
typedef struct{
    char dthora[24]; //Data e hora do pedido YYYY-MM-DD HH:MM:SS UTC
    long long idPedido; // Id do pedido (19)
    long long idProduto; // Id do produto (19)
    int quantidade; // Quantidade SKU do produto no pedido
    long long idCategoria; // Id da Categoria do produto (19)
    char aliasCategoria[24]; // Categoria do produto (tipo de joia)
    int idMarca; // Id da marca do produto (1)
    float preco; // Preco total do pedido (U$D)
    long long idUsuario; // Id do usuario que fez o pedido (19)
    char genero[24]; // Gênero do produto (M/F/U)
    char cor[24]; // Cor do produto/material (branco, dourado, etc)
    char material[24]; // Material do produto (ouro, prata, etc)
    char joia[24]; // Joia (diamante, rubi, esmeralda, etc)
} Registro;
 */

typedef struct{
    long long idProduto; // Id do produto (19)
    long long idCategoria; // Id da Categoria do produto (19)
    char aliasCategoria[24]; // Categoria do produto (tipo de joia)
    int idMarca; // Id da marca do produto (1)
    char genero[24]; // Gênero do produto (M/F/U)
    char cor[24]; // Cor do produto/material (branco, dourado, etc)
    char material[24]; // Material do produto (ouro, prata, etc)
    char joia[24]; // Joia (diamante, rubi, esmeralda, etc)
    int excluido; // 0 = Ativo, 1 = Excluido
} RegistroProduto;

typedef struct{
    char dthora[24]; //Data e hora do pedido YYYY-MM-DD HH:MM:SS UTC
    long long idPedido; // Id do pedido (19)
    long long idProduto; // Id do produto (19)
    int quantidade; // Quantidade SKU do produto no pedido
    float preco; // Preco total do pedido (U$D)
    long long idUsuario; // Id do usuario que fez o pedido (19)
    int excluido; // 0 = Ativo, 1 = Excluido
} RegistroPedido;

typedef struct{
    long long id; // Id
    long pos_pReg; // Posição do primeiro registro do bloco no arquivo de dados
    int contReg;  // Quantos registros tem no bloco
} IndexSeq;

IndexSeq *pIndexProd = NULL;
IndexSeq *pIndexPed = NULL;
int tam_index_prod = 0;
int tam_index_ped = 0;

void carregaIndexProd(){
    FILE *i = fopen(INDEX_PROD, "rb");
    if (i == NULL) {
        return;
    }
    
    if (fread(&tam_index_prod, sizeof(int), 1, i) == 1) {
        pIndexProd = (IndexSeq *)malloc(tam_index_prod * sizeof(IndexSeq));
        fread(pIndexProd, sizeof(IndexSeq), tam_index_prod, i);
        printf("Indice de produtos carregado de %s (%d entradas).\n", INDEX_PROD, tam_index_prod);
    }
    
    fclose(i);
}

void carregaIndexPed(){
    FILE *i = fopen(INDEX_PED, "rb");
    if (i == NULL) {
        return;
    }
    
    if (fread(&tam_index_ped, sizeof(int), 1, i) == 1) {
        pIndexPed = (IndexSeq *)malloc(tam_index_ped * sizeof(IndexSeq));
        fread(pIndexPed, sizeof(IndexSeq), tam_index_ped, i);
        printf("Indice de pedidos carregado de %s (%d entradas).\n", INDEX_PED, tam_index_ped);
    }
    
    fclose(i);
}

// Comparacao para qsort
int comparaProdutos(const void *a, const void *b){
    RegistroProduto *prodA = (RegistroProduto *)a;
    RegistroProduto *prodB = (RegistroProduto *)b;
    
    // Compara por idProduto
    if (prodA->idProduto < prodB->idProduto) return -1;
    if (prodA->idProduto > prodB->idProduto) return 1;
    return 0;
}

// Comparação para qsort
int comparaPedidos(const void *a, const void *b){
    RegistroPedido *pedA = (RegistroPedido *)a;
    RegistroPedido *pedB = (RegistroPedido *)b;
    
    // Compara por idPedido
    if (pedA->idPedido < pedB->idPedido) return -1;
    if (pedA->idPedido > pedB->idPedido) return 1;
    return 0;
}

// Em index-seq.c
void criaArquivosBinarios(){
    FILE *f = fopen(ARQUIVO, "r");
    if(f == NULL){
        printf("Erro ao abrir o arquivo CSV!\n");
        return;
    }

    char linha[256];
    int totalRegistros = 0;
    fgets(linha, sizeof(linha), f); // Ignorar cabecalho
    while(fgets(linha, sizeof(linha), f) != NULL){
        totalRegistros++;
    }
    printf("Total de %d registros lidos do CSV.\n", totalRegistros);

    rewind(f);
    fgets(linha, sizeof(linha), f);

    RegistroProduto *regProd = (RegistroProduto *)malloc(totalRegistros * sizeof(RegistroProduto));
    RegistroPedido *regPed = (RegistroPedido *)malloc(totalRegistros * sizeof(RegistroPedido));

    if (regProd == NULL || regPed == NULL) {
        printf("Erro de alocacao de memoria!\n");
        fclose(f);
        return;
    }

    int i = 0;

    while(fgets(linha, sizeof(linha), f) != NULL){
        // ler todos os campos como strings
        char s_dthora[25], s_idPedido[25], s_idProduto[25], s_quantidade[25];
        char s_idCategoria[25], s_aliasCategoria[25], s_idMarca[25];
        char s_preco[25], s_idUsuario[25];
        char s_genero[25], s_cor[25], s_material[25], s_joia[25];

        sscanf(linha, "%24[^,],%24[^,],%24[^,],%24[^,],%24[^,],%24[^,],%24[^,],%24[^,],%24[^,],%24[^,],%24[^,],%24[^,],%24[^,\n]",
               s_dthora,
			   s_idPedido,
			   s_idProduto,
			   s_quantidade,
               s_idCategoria,
			   s_aliasCategoria,
			   s_idMarca,
               s_preco,
			   s_idUsuario,
               s_genero,
			   s_cor,
			   s_material, s_joia);

        /* conversao:
        atoll = ascii to long long
        atoi  = ascii to int
        atof  = ascii to float
        se string = vazia, retorna 0 ou 0.0
		*/
        
        // Pedido
        strcpy(regPed[i].dthora, s_dthora);
        regPed[i].idPedido = atoll(s_idPedido);
        regPed[i].quantidade = atoi(s_quantidade);
        regPed[i].preco = atof(s_preco);
        regPed[i].idUsuario = atoll(s_idUsuario);
        regPed[i].excluido = 0;
        
        // Produto
        regProd[i].idProduto = atoll(s_idProduto);
        regProd[i].idCategoria = atoll(s_idCategoria);
        strcpy(regProd[i].aliasCategoria, s_aliasCategoria);
        regProd[i].idMarca = atoi(s_idMarca);
        strcpy(regProd[i].genero, s_genero);
        strcpy(regProd[i].cor, s_cor);
        strcpy(regProd[i].material, s_material);
        strcpy(regProd[i].joia, s_joia);
        regProd[i].excluido = 0;

        regPed[i].idProduto = regProd[i].idProduto;
        
        i++;
    }

    fclose(f);

    printf("Ordenando dados...\n");
    qsort(regProd, totalRegistros, sizeof(RegistroProduto), comparaProdutos);
    qsort(regPed, totalRegistros, sizeof(RegistroPedido), comparaPedidos);
    printf("Dados ordenados.\n");

    FILE *bProdutos = fopen(ARQ_PRODUTOS, "wb");
    FILE *bPedidos = fopen(ARQ_PEDIDOS, "wb");
    
    if(bProdutos == NULL || bPedidos == NULL){
        printf("Erro ao criar arquivos binarios!\n");
        free(regProd);
        free(regPed);
        return;
    }

    fwrite(regProd, sizeof(RegistroProduto), totalRegistros, bProdutos);
    fwrite(regPed, sizeof(RegistroPedido), totalRegistros, bPedidos);

    fclose(bProdutos);
    fclose(bPedidos);

    free(regProd);
    free(regPed);

    printf("Arquivos binarios %s e %s criados com sucesso.\n", ARQ_PRODUTOS, ARQ_PEDIDOS);
}

long getTotalRegistros(const char *nomeArquivo, long tamanhoStruct) {
    FILE *f = fopen(nomeArquivo, "rb");
    if (f == NULL) return 0;
    
    fseek(f, 0, SEEK_END);
    long tamanhoArquivo = ftell(f); 
    fclose(f);
    
    return tamanhoArquivo / tamanhoStruct;
}

void criaIndexProd(){
    long totalReg = getTotalRegistros(ARQ_PRODUTOS, sizeof(RegistroProduto));
    if (totalReg == 0) {
        printf("Arquivo de produtos esta vazio. Indice nao foi criado.\n");
        return;
    }

    tam_index_prod = (int)ceil((double)totalReg / TAM_INTERVALO);
    pIndexProd = (IndexSeq *)malloc(tam_index_prod * sizeof(IndexSeq));
    
    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    if (f == NULL) {
        printf("Erro ao abrir %s para criar indice.\n", ARQ_PRODUTOS);
        return;
    }

    RegistroProduto prod;

    for (int i = 0;i < tam_index_prod;i++) {

        long pos_reg_bloco = i * TAM_INTERVALO; // Posicao do primeiro registro do bloco
        long pos_byte_bloco = pos_reg_bloco * sizeof(RegistroProduto); // Posicao em bytes

        fseek(f, pos_byte_bloco, SEEK_SET);

        if (fread(&prod, sizeof(RegistroProduto), 1, f) == 1) {
            pIndexProd[i].id = prod.idProduto; // Chave
            pIndexProd[i].pos_pReg = pos_byte_bloco; // Posicao

            // Verifica quantos registros tem no bloco
            if (i == tam_index_prod - 1) {
                // Ultimo bloco
                pIndexProd[i].contReg = totalReg - pos_reg_bloco;
            } else {
                // Outros blocos
                pIndexProd[i].contReg = TAM_INTERVALO;
            }
        }
    }

    fclose(f);

    printf("Indice de Produtos criado com %d entradas.\n", tam_index_prod);
}

void criaIndexPed(){
    long totalReg = getTotalRegistros(ARQ_PEDIDOS, sizeof(RegistroPedido));
    if (totalReg == 0) {
        printf("Arquivo de pedidos esta vazio. Indice nao foi criado.\n");
        return;
    }

    tam_index_ped = (int)ceil((double)totalReg / TAM_INTERVALO);
    pIndexPed = (IndexSeq *)malloc(tam_index_ped * sizeof(IndexSeq));
    
    FILE *f = fopen(ARQ_PEDIDOS, "rb");
    if (f == NULL) {
        printf("Erro ao abrir %s para criar indice.\n", ARQ_PEDIDOS);
        return;
    }

    RegistroPedido ped;

    for (int i = 0;i < tam_index_ped;i++) {

        long pos_reg_bloco = i * TAM_INTERVALO; // Posicao do primeiro registro do bloco
        long pos_byte_bloco = pos_reg_bloco * sizeof(RegistroPedido); // Posicao em bytes

        fseek(f, pos_byte_bloco, SEEK_SET);

        if (fread(&ped, sizeof(RegistroPedido), 1, f) == 1) {
            pIndexPed[i].id = ped.idPedido; // Chave
            pIndexPed[i].pos_pReg = pos_byte_bloco; // Posicao

            // Verifica quantos registros tem no bloco
            if (i == tam_index_ped - 1) {
                // Ultimo bloco
                pIndexPed[i].contReg = totalReg - pos_reg_bloco;
            } else {
                // Outros blocos
                pIndexPed[i].contReg = TAM_INTERVALO;
            }
        }
    }

    fclose(f);
    
    printf("Indice de Pedidos criado com %d entradas.\n", tam_index_ped);
}

void salvaIndexProd(){
    if (pIndexProd == NULL) return;
    
    FILE *f = fopen(INDEX_PROD, "wb");
    if (f == NULL) {
        printf("Erro ao salvar indice de produtos.\n");
        return;
    }

    fwrite(&tam_index_prod, sizeof(int), 1, f);
    fwrite(pIndexProd, sizeof(IndexSeq), tam_index_prod, f);
    fclose(f);

    printf("Indice de produtos salvo em %s\n", INDEX_PROD);
}

void salvaIndexPed(){
    if (pIndexPed == NULL) return;
    
    FILE *f = fopen(INDEX_PED, "wb");
    if (f == NULL) {
        printf("Erro ao salvar indice de pedidos.\n");
        return;
    }
    
    fwrite(&tam_index_ped, sizeof(int), 1, f);
    fwrite(pIndexPed, sizeof(IndexSeq), tam_index_ped, f);
    
    fclose(f);
    printf("Indice de pedidos salvo em %s\n", INDEX_PED);
}

void mostraIndexProd(){
	if (pIndexProd == NULL || tam_index_prod == 0) {
        printf("Erro no indice de produtos, nao foi criado ou esta vazio.\n");
        return;
    }
	
	printf("\n--- INDICE SEQUENCIAL PRODUTOS ---\n");
    printf("--------------------------------------------------------------------------\n");
    printf("| %-30s | Inicia no Registro | No de Registros |\n", "Primeiro Produto do Bloco");
    printf("--------------------------------------------------------------------------\n");
    for (int i = 0; i < tam_index_prod; i++) {
        printf("| %-30lld | %-18ld | %-15d |\n", pIndexProd[i].id, pIndexProd[i].pos_pReg, pIndexProd[i].contReg);
    }
    printf("--------------------------------------------------------------------------\n");
}

void mostraIndexPed(){
	if (pIndexPed == NULL || tam_index_ped == 0) {
        printf("Erro no indice de pedidos, nao foi criado ou esta vazio.\n");
        return;
    }
	
	printf("\n--- INDICE SEQUENCIAL PEDIDOS ---\n");
    printf("--------------------------------------------------------------------------\n");
    printf("| %-30s | Inicia no Registro | No de Registros |\n", "Primeiro Pedido do Bloco");
    printf("--------------------------------------------------------------------------\n");
    for (int i = 0; i < tam_index_ped; i++) {
        printf("| %-30lld | %-18ld | %-15d |\n", pIndexPed[i].id, pIndexPed[i].pos_pReg, pIndexPed[i].contReg);
    }
    printf("--------------------------------------------------------------------------\n");
}

void mostrarProdutos(){
    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    if (f == NULL) {
        printf("Erro: Nao foi possivel abrir o arquivo binario %s\n", ARQ_PRODUTOS);
        return;
    }

    RegistroProduto prod;
    int contador = 0;
    int contadorExcluidos = 0;

    printf("\n--- PRODUTOS (%s) ---\n", ARQ_PRODUTOS);
    while (fread(&prod, sizeof(RegistroProduto), 1, f) == 1) {
        if (prod.excluido == 1) { // Pula registros excluidos logicamente
            contadorExcluidos++;
            continue;
        }
        printf("ID: %lld | Categoria: %s | Genero: %s | Material: %s %s | Joia: %s\n",
               prod.idProduto,
               prod.aliasCategoria,
               prod.genero,
               prod.cor,
               prod.material,
               prod.joia);
        contador++;
    }
    fclose(f);
    
    // Mostra arquivos no overflow
    f = fopen(ARQ_OVERFLOW_PROD, "rb");
    if (f != NULL) {
        printf("\n--- PRODUTOS EM OVERFLOW (%s) ---\n", ARQ_OVERFLOW_PROD);
        while (fread(&prod, sizeof(RegistroProduto), 1, f) == 1) {
            if (prod.excluido == 1) { // Pula registros excluidos logicamente
                contadorExcluidos++;
                continue;
            }
            printf("ID: %lld | Categoria: %s | Genero: %s | Material: %s %s | Joia: %s\n",
                   prod.idProduto,
                   prod.aliasCategoria,
                   prod.genero,
                   prod.cor,
                   prod.material,
                   prod.joia);
            contador++;
        }
        fclose(f);
    }

    printf("---------------------------------------------\n");
    printf("Total de %d produtos ativos mostrados.\n", contador);
    if(contadorExcluidos > 0) {
        printf("(%d produtos excluidos foram ignorados)\n", contadorExcluidos);
    }
}

void mostrarPedidos(){
    FILE *f = fopen(ARQ_PEDIDOS, "rb");
    if (f == NULL) {
        printf("Erro: Nao foi possivel abrir o arquivo binario %s\n", ARQ_PEDIDOS);
        return;
    }

    RegistroPedido ped;
    int contador = 0;
    int contadorExcluidos = 0;

    printf("\n--- PEDIDOS (%s) ---\n", ARQ_PEDIDOS);
    while (fread(&ped, sizeof(RegistroPedido), 1, f) == 1) {
        if (ped.excluido == 1) { // Pula registros excluidos logicamente
            contadorExcluidos++;
            continue;
        }
        printf("ID Pedido: %lld | ID Produto: %lld | ID Usuario: %lld | Data/Hora: %s | Quantidade: %d | Preco: U$D %.2f\n",
               ped.idPedido,
               ped.idProduto,
               ped.idUsuario,
               ped.dthora,
               ped.quantidade,
               ped.preco);
        contador++;
    }
    fclose(f);
    
    // Mostra arquivos no overflow
    f = fopen(ARQ_OVERFLOW_PED, "rb");
    if (f != NULL) {
        printf("\n--- PEDIDOS EM OVERFLOW (%s) ---\n", ARQ_OVERFLOW_PED);
        while (fread(&ped, sizeof(RegistroPedido), 1, f) == 1) {
            if (ped.excluido == 1) { // Pula registros excluidos logicamente
                contadorExcluidos++;
                continue;
            }
            printf("ID Pedido: %lld | ID Produto: %lld | ID Usuario: %lld | Data/Hora: %s | Quantidade: %d | Preco: U$D %.2f\n",
                   ped.idPedido,
                   ped.idProduto,
                   ped.idUsuario,
                   ped.dthora,
                   ped.quantidade,
                   ped.preco);
            contador++;
        }
        fclose(f);
    }

    printf("---------------------------------------------\n");
    printf("Total de %d pedidos ativos mostrados.\n", contador);
    if(contadorExcluidos > 0) {
        printf("(%d pedidos excluidos foram ignorados)\n", contadorExcluidos);
    }
}

int pesquisaBinariaIndice(long long idChave, IndexSeq *indice, int tamIndice) {
    int esq = 0;
    int dir = tamIndice - 1;
    int bloco = -1;

    while (esq <= dir) {
        int meio = esq + (dir - esq) / 2;
        
        // Checa primeira chave do bloco
        if (indice[meio].id == idChave) {
            return meio;
        }
        
        // Se a chave do meio for menor que a chave procurada, segue para a direita
        if (indice[meio].id < idChave) {
            bloco = meio;
            esq = meio + 1;
        } else { // Se for maior, segue para a esquerda
            dir = meio - 1;
        }
    }
    return bloco; // Retorna o bloco mais proximo menor, ou, -1 se nao encontra
}

void buscarProduto(long long idProduto) {
    if (pIndexProd == NULL) {
        printf("Indice de produtos nao carregado.\n");
        // nao retorna aqui, pois pode haver registros no overflow
    }

    RegistroProduto prod;
    bool achou = false;

    if (pIndexProd != NULL) {
        int indice_bloco = pesquisaBinariaIndice(idProduto, pIndexProd, tam_index_prod);

        if (indice_bloco != -1) {
            FILE *f = fopen(ARQ_PRODUTOS, "rb");
            if (f == NULL) {
                printf("Erro ao abrir %s\n", ARQ_PRODUTOS);
                return;
            }

            long pos_bloco = pIndexProd[indice_bloco].pos_pReg;
            int n_regs_bloco = pIndexProd[indice_bloco].contReg;
            
            fseek(f, pos_bloco, SEEK_SET);

            for (int i = 0; i < n_regs_bloco; i++) {
                fread(&prod, sizeof(RegistroProduto), 1, f);
                
                if (prod.idProduto == idProduto) {
                    if (prod.excluido == 0) { // Encontrado e ativo
                        printf("\n--- PRODUTO ENCONTRADO (em %s) ---\n", ARQ_PRODUTOS);
                        printf("ID Produto: %lld\n", prod.idProduto);
                        printf("Categoria:  %s (ID: %lld)\n", prod.aliasCategoria, prod.idCategoria);
                        printf("ID Marca:   %d\n", prod.idMarca);
                        printf("Cor:        %s\n", prod.cor);
                        printf("Material:   %s\n", prod.material);
                        printf("Genero:     %s\n", prod.genero);
                        printf("--------------------------\n");
                        achou = true;
                    } else {
                        // Encontrado mas excluido
                        printf("Produto ID %lld encontrado em %s, mas esta marcado como excluido.\n", idProduto, ARQ_PRODUTOS);
                        achou = true;
                    }
                    break;
                }
                if (prod.idProduto > idProduto) break;
            }
            fclose(f);
        }
    }

    if (!achou) {
        FILE *f_ovf = fopen(ARQ_OVERFLOW_PROD, "rb");
        if (f_ovf == NULL) {
            // Arquivo nao existe
            printf("Produto com ID %lld nao encontrado.\n", idProduto);
            return;
        }

        // Busca linear
        while (fread(&prod, sizeof(RegistroProduto), 1, f_ovf) == 1) {
            if (prod.idProduto == idProduto) {
                if (prod.excluido == 0) { // Encontrado e ativo
                    printf("\n--- PRODUTO ENCONTRADO (em %s) ---\n", ARQ_OVERFLOW_PROD);
                    printf("ID Produto: %lld\n", prod.idProduto);
                    printf("Categoria:  %s (ID: %lld)\n", prod.aliasCategoria, prod.idCategoria);
                    printf("ID Marca:   %d\n", prod.idMarca);
                    printf("Cor:        %s\n", prod.cor);
                    printf("Material:   %s\n", prod.material);
                    printf("Genero:     %s\n", prod.genero);
                    printf("--------------------------\n");
                    achou = true;
                } else {
                    // Encontrado mas excluido
                    printf("Produto ID %lld encontrado em %s, mas esta marcado como excluido.\n", idProduto, ARQ_OVERFLOW_PROD);
                    achou = true;
                }
                break;
            }
        }
        fclose(f_ovf);
    }
    
    if (!achou) {
        printf("Produto com ID %lld nao encontrado.\n", idProduto);
    }
}

void buscarPedido(long long idPedido){
    if (pIndexPed == NULL) {
        printf("Indice de pedidos nao carregado.\n");
        // nao retorna aqui, pois pode haver registros no overflow
    }

    RegistroPedido ped;
    bool achou = false;

    if (pIndexPed != NULL) {
        int indice_bloco = pesquisaBinariaIndice(idPedido, pIndexPed, tam_index_ped);

        if (indice_bloco != -1) {
            FILE *f = fopen(ARQ_PEDIDOS, "rb");
            if (f == NULL) {
                printf("Erro ao abrir %s\n", ARQ_PEDIDOS);
                return;
            }

            long pos_bloco = pIndexPed[indice_bloco].pos_pReg;
            int n_regs_bloco = pIndexPed[indice_bloco].contReg;
            
            fseek(f, pos_bloco, SEEK_SET);

            for (int i = 0; i < n_regs_bloco; i++) {
                fread(&ped, sizeof(RegistroPedido), 1, f);
                
                if (ped.idPedido == idPedido) {
                    if (ped.excluido == 0) { // Encontrado e ativo
                        printf("\n--- PEDIDO ENCONTRADO (em %s) ---\n", ARQ_PEDIDOS);
                        printf("ID Pedido: %lld\n", ped.idPedido);
                        printf("ID Produto: %lld\n", ped.idProduto);
                        printf("ID Usuario: %lld\n", ped.idUsuario);
                        printf("Data/Hora: %s\n", ped.dthora);
                        printf("Quantidade: %d\n", ped.quantidade);
                        printf("Preco: U$D %.2f\n", ped.preco);
                        printf("--------------------------\n");
                        achou = true;
                    } else {
                        // Encontrado mas excluido
                        printf("Pedido ID %lld encontrado em %s, mas esta marcado como excluido.\n", idPedido, ARQ_PEDIDOS);
                        achou = true;
                    }
                    break;
                }
                if (ped.idPedido > idPedido) break;
            }
            fclose(f);
        }
    }

    if (!achou) {
        FILE *f_ovf = fopen(ARQ_OVERFLOW_PED, "rb");
        if (f_ovf == NULL) {
            // Arquivo nao existe
            printf("Pedido com ID %lld nao encontrado.\n", idPedido);
            return;
        }
        // Busca linear
        while (fread(&ped, sizeof(RegistroPedido), 1, f_ovf) == 1) {
            if (ped.idPedido == idPedido) {
                if (ped.excluido == 0) { // Encontrado e ativo
                    printf("\n--- PEDIDO ENCONTRADO (em %s) ---\n", ARQ_OVERFLOW_PED);
                    printf("ID Pedido: %lld\n", ped.idPedido);
                    printf("ID Produto: %lld\n", ped.idProduto);
                    printf("ID Usuario: %lld\n", ped.idUsuario);
                    printf("Data/Hora: %s\n", ped.dthora);
                    printf("Quantidade: %d\n", ped.quantidade);
                    printf("Preco: U$D %.2f\n", ped.preco);
                    printf("--------------------------\n");
                    achou = true;
                } else {
                    // Encontrado mas excluido
                    printf("Pedido ID %lld encontrado em %s, mas esta marcado como excluido.\n", idPedido, ARQ_OVERFLOW_PED);
                    achou = true;
                }
                break;
            }
        }
        fclose(f_ovf);
    }
    if (!achou) {
        printf("Pedido com ID %lld nao encontrado.\n", idPedido);
    }
}

// Inclusao de um novo registro num arquivo de overflow
void inserirProduto(RegistroProduto novoProd) {
    novoProd.excluido = 0; // Garante que o novo registro esteja ativo

    FILE *f_ovf = fopen(ARQ_OVERFLOW_PROD, "ab"); // ab = Append Binary
    if (f_ovf == NULL) {
        printf("Erro ao abrir arquivo de overflow %s\n", ARQ_OVERFLOW_PROD);
        return;
    }

    fwrite(&novoProd, sizeof(RegistroProduto), 1, f_ovf);
    fclose(f_ovf);

    printf("Produto ID %lld inserido no arquivo de overflow.\n", novoProd.idProduto);
    printf("Necessario fazer a reordenacao do indice.\n");
}

void removerProduto(long long idParaRemover) {
    bool achou = false;
    RegistroProduto prod;

    if (pIndexProd != NULL) {
        int indice_bloco = pesquisaBinariaIndice(idParaRemover, pIndexProd, tam_index_prod);

        if (indice_bloco != -1) {
            FILE *f = fopen(ARQ_PRODUTOS, "r+b");
            if (f == NULL) {
                printf("Erro ao abrir %s para remocao.\n", ARQ_PRODUTOS);
                return;
            }

            long pos_bloco = pIndexProd[indice_bloco].pos_pReg;
            int n_regs_bloco = pIndexProd[indice_bloco].contReg;
            
            fseek(f, pos_bloco, SEEK_SET);

            for (int i = 0; i < n_regs_bloco; i++) {
                long posAntesLeitura = ftell(f);
                fread(&prod, sizeof(RegistroProduto), 1, f);
                
                if (prod.idProduto == idParaRemover) {
                    if (prod.excluido == 0) {
                        prod.excluido = 1;
                        
                        fseek(f, posAntesLeitura, SEEK_SET);
                        fwrite(&prod, sizeof(RegistroProduto), 1, f);
                        
                        printf("Produto ID %lld marcado como excluido em %s.\n", idParaRemover, ARQ_PRODUTOS);
                        achou = true;
                    } else { // Ja estava excluido
                        achou = true; // Considera que encontrou
                    }
                    break;
                }
                if (prod.idProduto > idParaRemover) break;
            }
            fclose(f);
        }
    }

    if (!achou) {
        FILE *f_ovf = fopen(ARQ_OVERFLOW_PROD, "r+b");
        if (f_ovf == NULL) { // Se o arquivo nao existe
            printf("Produto ID %lld nao encontrado.\n", idParaRemover);
            return;
        }

        while (true) {
            long posAntesLeitura = ftell(f_ovf);
            if (fread(&prod, sizeof(RegistroProduto), 1, f_ovf) != 1) {
                break;
            }

            if (prod.idProduto == idParaRemover) {
                if (prod.excluido == 0) {
                    prod.excluido = 1;
                    fseek(f_ovf, posAntesLeitura, SEEK_SET);
                    fwrite(&prod, sizeof(RegistroProduto), 1, f_ovf);
                    printf("Produto ID %lld marcado como excluido em %s.\n", idParaRemover, ARQ_OVERFLOW_PROD);
                    achou = true;
                }
                break;
            }
        }
        fclose(f_ovf);
    }
    
    if (!achou) {
        printf("Produto ID %lld nao encontrado.\n", idParaRemover);
    }
}

void inserirPedido(RegistroPedido novoPed) {
    novoPed.excluido = 0; // Garante que o novo registro esteja ativo

    FILE *f_ovf = fopen(ARQ_OVERFLOW_PED, "ab"); // ab = Append Binary
    if (f_ovf == NULL) {
        printf("Erro ao abrir arquivo de overflow %s\n", ARQ_OVERFLOW_PED);
        return;
    }

    fwrite(&novoPed, sizeof(RegistroPedido), 1, f_ovf);
    fclose(f_ovf);

    printf("Pedido ID %lld inserido no arquivo de overflow.\n", novoPed.idPedido);
    printf("Necessario fazer a reordenacao do indice.\n");
}

void removerPedido(long long idParaRemover) {
    bool achou = false;
    RegistroPedido ped;

    if (pIndexPed != NULL) {
        int indice_bloco = pesquisaBinariaIndice(idParaRemover, pIndexPed, tam_index_ped);

        if (indice_bloco != -1) {
            FILE *f = fopen(ARQ_PEDIDOS, "r+b");
            if (f == NULL) {
                printf("Erro ao abrir %s para remocao.\n", ARQ_PEDIDOS);
                return;
            }

            long pos_bloco = pIndexPed[indice_bloco].pos_pReg;
            int n_regs_bloco = pIndexPed[indice_bloco].contReg;
            
            fseek(f, pos_bloco, SEEK_SET);

            for (int i = 0;i < n_regs_bloco; i++) {
                long posAntesLeitura = ftell(f);
                fread(&ped, sizeof(RegistroPedido), 1, f);
                
                if (ped.idPedido == idParaRemover) {
                    if (ped.excluido == 0) {
                        ped.excluido = 1;
                        
                        fseek(f, posAntesLeitura, SEEK_SET);
                        fwrite(&ped, sizeof(RegistroPedido), 1, f);
                        
                        printf("Pedido ID %lld marcado como excluido em %s.\n", idParaRemover, ARQ_PEDIDOS);
                        achou = true;
                    } else { // Ja estava excluido
                        achou = true; // Considera que encontrou
                    }
                    break;
                }
                if (ped.idPedido > idParaRemover) break;
            }
            fclose(f);
        }
    }

    if (!achou) {
        FILE *f_ovf = fopen(ARQ_OVERFLOW_PED, "r+b");
        if (f_ovf == NULL) { // Se o arquivo nao existe
            printf("Pedido ID %lld nao encontrado.\n", idParaRemover);
            return;
        }

        while (true) {
            long posAntesLeitura = ftell(f_ovf);
            if (fread(&ped, sizeof(RegistroPedido), 1, f_ovf) != 1) {
                break;
            }

            if (ped.idPedido == idParaRemover) {
                if (ped.excluido == 0) {
                    ped.excluido = 1;
                    fseek(f_ovf, posAntesLeitura, SEEK_SET);
                    fwrite(&ped, sizeof(RegistroPedido), 1, f_ovf);
                    printf("Pedido ID %lld marcado como excluido em %s.\n", idParaRemover, ARQ_OVERFLOW_PED);
                    achou = true;
                }
                break;
            }
        }
        fclose(f_ovf);
    }
    
    if (!achou) {
        printf("Pedido ID %lld nao encontrado.\n", idParaRemover);
    }
}

void reordenacaoProd() {
    printf("\n--- INICIANDO REORDENACAO DE PRODUTOS ---\n");

    long totalRegBin = getTotalRegistros(ARQ_PRODUTOS, sizeof(RegistroProduto));
    long totalRegOvf = getTotalRegistros(ARQ_OVERFLOW_PROD, sizeof(RegistroProduto));
    
    if (totalRegBin == 0 && totalRegOvf == 0) {
        printf("Nenhum dado para reordenar.\n");
        return;
    }

    RegistroProduto *todosProdutos = (RegistroProduto *) malloc((totalRegBin + totalRegOvf) * sizeof(RegistroProduto));
    if (todosProdutos == NULL) {
        printf("Erro de alocacao de memoria para reordenacao.\n");
        return;
    }

    RegistroProduto prod;
    long contValidos = 0;
    
    FILE *f_bin = fopen(ARQ_PRODUTOS, "rb");
    if (f_bin != NULL) {
        while (fread(&prod, sizeof(RegistroProduto), 1, f_bin) == 1) {
            if (prod.excluido == 0) {
                todosProdutos[contValidos++] = prod;
            }
        }
        fclose(f_bin);
    }
    printf("Lidos %ld registros validos de %s.\n", contValidos, ARQ_PRODUTOS);

    FILE *f_ovf = fopen(ARQ_OVERFLOW_PROD, "rb");
    long contValidosOvf = 0;
    if (f_ovf != NULL) {
        while (fread(&prod, sizeof(RegistroProduto), 1, f_ovf) == 1) {
            if (prod.excluido == 0) {
                todosProdutos[contValidos++] = prod;
                contValidosOvf++;
            }
        }
        fclose(f_ovf);
    }
    printf("Lidos %ld registros validos de %s.\n", contValidosOvf, ARQ_OVERFLOW_PROD);

    printf("Ordenando %ld registros totais...\n", contValidos);
    qsort(todosProdutos, contValidos, sizeof(RegistroProduto), comparaProdutos);

    f_bin = fopen(ARQ_PRODUTOS, "wb");
    if (f_bin == NULL) {
        printf("Erro fatal: Nao foi possivel reescrever %s.\n", ARQ_PRODUTOS);
        free(todosProdutos);
        return;
    }

    fwrite(todosProdutos, sizeof(RegistroProduto), contValidos, f_bin);
    fclose(f_bin);

    free(todosProdutos);

    printf("Arquivo %s reescrito com dados ordenados.\n", ARQ_PRODUTOS);

    f_ovf = fopen(ARQ_OVERFLOW_PROD, "wb"); // Abro dessa forma para poder limpar o arquivo
    if (f_ovf != NULL) {
        fclose(f_ovf);
        printf("Arquivo de overflow %s foi limpo.\n", ARQ_OVERFLOW_PROD);
    }

    printf("Reconstruindo indice...\n");

    if (pIndexProd != NULL) free(pIndexProd);

    criaIndexProd();
    salvaIndexProd();

    printf("--- REORDENACAO CONCLUIDA ---\n");
}

void reordenacaoPed() {
    printf("\n--- INICIANDO REORDENACAO DE PEDIDOS ---\n");

    long totalRegBin = getTotalRegistros(ARQ_PEDIDOS, sizeof(RegistroPedido));
    long totalRegOvf = getTotalRegistros(ARQ_OVERFLOW_PED, sizeof(RegistroPedido));
    
    if (totalRegBin == 0 && totalRegOvf == 0) {
        printf("Nenhum dado para reordenar.\n");
        return;
    }

    RegistroPedido *todosPedidos = (RegistroPedido *) malloc((totalRegBin + totalRegOvf) * sizeof(RegistroPedido));
    if (todosPedidos == NULL) {
        printf("Erro de alocacao de memoria para reordenacao.\n");
        return;
    }

    RegistroPedido ped;
    long contValidos = 0;
    
    FILE *f_bin = fopen(ARQ_PEDIDOS, "rb");
    if (f_bin != NULL) {
        while (fread(&ped, sizeof(RegistroPedido), 1, f_bin) == 1) {
            if (ped.excluido == 0) {
                todosPedidos[contValidos++] = ped;
            }
        }
        fclose(f_bin);
    }
    printf("Lidos %ld registros validos de %s.\n", contValidos, ARQ_PEDIDOS);

    FILE *f_ovf = fopen(ARQ_OVERFLOW_PED, "rb");
    long contValidosOvf = 0;
    if (f_ovf != NULL) {
        while (fread(&ped, sizeof(RegistroPedido), 1, f_ovf) == 1) {
            if (ped.excluido == 0) {
                todosPedidos[contValidos++] = ped;
                contValidosOvf++;
            }
        }
        fclose(f_ovf);
    }
    printf("Lidos %ld registros validos de %s.\n", contValidosOvf, ARQ_OVERFLOW_PED);

    printf("Ordenando %ld registros totais...\n", contValidos);
    qsort(todosPedidos, contValidos, sizeof(RegistroPedido), comparaPedidos);

    f_bin = fopen(ARQ_PEDIDOS, "wb");
    if (f_bin == NULL) {
        printf("Erro fatal: Nao foi possivel reescrever %s.\n", ARQ_PEDIDOS);
        free(todosPedidos); 
        return;
    }

    fwrite(todosPedidos, sizeof(RegistroPedido), contValidos, f_bin);
    fclose(f_bin);

    free(todosPedidos);

    printf("Arquivo %s reescrito com dados ordenados.\n", ARQ_PEDIDOS);

    f_ovf = fopen(ARQ_OVERFLOW_PED, "wb"); // Abro dessa forma para poder limpar o arquivo
    if (f_ovf != NULL) {
        fclose(f_ovf);
        printf("Arquivo de overflow %s foi limpo.\n", ARQ_OVERFLOW_PED);
    }

    printf("Reconstruindo indice...\n");

    if (pIndexPed != NULL) free(pIndexPed);

    criaIndexPed();
    salvaIndexPed();

    printf("--- REORDENACAO CONCLUIDA ---\n");
}

int main(){
    //quantos produtos sao feitos com diamante?
    //quantos pedidos foram feitos por usuarios do genero F?
    //qual o preco medio dos produtos de ouro? 
    
    carregaIndexProd();
    carregaIndexPed();
	
	if (pIndexProd == NULL || pIndexPed == NULL) {
        printf("\nNenhum indice encontrado. Sera criado um novo.\n");

        criaArquivosBinarios(); // cria ordenados

        printf("\n");

        criaIndexProd();
        criaIndexPed();

        salvaIndexProd();
        salvaIndexPed();
    }
	
	//mostraIndexProd();
    //mostraIndexPed();

    //mostrarProdutos();
    //mostrarPedidos();

    // CONSULTA

    printf("\n--- TESTANDO BUSCAS ---\n");
    buscarProduto(1956663846231867998); // qual o produto com esse id?
    buscarProduto(1956663846231867995); 
    buscarProduto(1782242000000000000); // id inexistente
    
    buscarPedido(1924899396621697920); // qual o pedido com esse id?
    buscarPedido(1927556184420647176);
    buscarPedido(1273378282000004270); // id inexistente

    // REMOCAO LOGICA

    printf("\n--- TESTANDO REMOCAO LOGICA ---\n");
    long long idRemocaoProd = 1956663846231867998;
    buscarProduto(idRemocaoProd); // acha
    removerProduto(idRemocaoProd); // remove
    buscarProduto(idRemocaoProd); // nao acha mais (tudo isso eh na primeira vez soh)

    long long idRemocaoPed = 1927556184420647176;
    buscarPedido(idRemocaoPed); // acha
    removerPedido(idRemocaoPed); // remove
    buscarPedido(idRemocaoPed); // nao acha mais (tudo isso eh na primeira vez soh)

    // INSERCAO

    printf("\n--- TESTANDO INSERCAO OVERFLOW ---\n");
    RegistroProduto novoProduto;
    novoProduto.idProduto = 1;
    novoProduto.idCategoria = 12345;
    strcpy(novoProduto.aliasCategoria, "Anel Overflow");
    novoProduto.idMarca = 9;
    strcpy(novoProduto.genero, "U");
    strcpy(novoProduto.cor, "Preto");
    strcpy(novoProduto.material, "Onix");
    strcpy(novoProduto.joia, "N/A");
    
    buscarProduto(novoProduto.idProduto); // nao acha
    inserirProduto(novoProduto); // insere no .ovf
    buscarProduto(novoProduto.idProduto); // agora deve achar

    RegistroPedido novoPedido;
    novoPedido.idPedido = 1;
    novoPedido.idProduto = novoProduto.idProduto;
    novoPedido.idUsuario = 54321;
    strcpy(novoPedido.dthora, "2024-06-01 12:00:00 UCT");
    novoPedido.quantidade = 2;
    novoPedido.preco = 199.99;

    buscarPedido(novoPedido.idPedido); // nao acha
    inserirPedido(novoPedido); // insere no .ovf
    buscarPedido(novoPedido.idPedido); // agora deve achar

    // REORDENACAO

    printf("\n--- TESTANDO REORDENACAO DE PRODUTOS ---\n");
    printf("=== ANTES DA REORDENACAO ===\n");
    //mostrarProdutos();
    //mostraIndexProd();

    reordenacaoProd();

    printf("\n=== APOS A REORDENACAO ===\n");
    //mostrarProdutos(); // mostra o Overflow primeiro
    //mostraIndexProd(); // indice refeito

    printf("\n=== VERIFICANDO BUSCAS POS-REORDENACAO ===\n");
    buscarProduto(idRemocaoProd); // nao encontrado
    buscarProduto(novoProduto.idProduto); // encontrado

    printf("=== ANTES DA REORDENACAO ===\n");
    //mostrarPedidos();
    //mostraIndexPed();

    reordenacaoPed();

    printf("\n=== APOS A REORDENACAO ===\n");
    //mostrarPedidos(); // mostra o Overflow primeiro
    //mostraIndexPed(); // indice refeito
    printf("\n=== VERIFICANDO BUSCAS POS-REORDENACAO ===\n");
    buscarPedido(idRemocaoPed); // nao encontrado
    buscarPedido(novoPedido.idPedido); // encontrado
    
    return 0;
}
