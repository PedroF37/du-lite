#include <utils.h>


/* Representa um diretório */
typedef struct DirInfo
{
    char *path;
    long long size;
}
DirInfo;


/* Cuida de liberar um array de diretórios */
void destruct_dirs(DirInfo *dirs, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (dirs[i].path != NULL)
        {
            free(dirs[i].path);
        }
    }

    free(dirs);
}


/* Função de comparação para qsort. Comparação
 * reversa. Queremos do maior para o menor */
int compare_dir_info(const void *a, const void *b)
{
    return( ((DirInfo *)b)->size - ((DirInfo *)a)->size );
}


/* Cuida de formatar o tamanho com as unidades certas */
char *format_size(long long size)
{
    char *units[] = {"B", "KB", "MB", "GB", "TB"};
    size_t unit_index = 0;

    while (size >= 1024 && unit_index < sizeof(units)
        / sizeof(units[0]) - 1)
    {
        size /= 1024;
        unit_index++;
    }

    char *formatted_size = malloc(20);
    if (formatted_size == NULL)
    {
        return(NULL);
    }

    snprintf(formatted_size, 20, "%.1f %s",
        (double)size, units[unit_index]);

    return(formatted_size);
}

/* Cuida de ordenar o array e imprimir os tamanhos bonitinhos
 * 600.0 MB, etc */
void print_sizes(char *base_path, DirInfo *dirs,
    int dir_count, int number_of_directories)
{
    /* Conta com o caso de termos 20 diretórios
     * e o usuário ter pedido 1000 kk */
    if (number_of_directories > dir_count)
    {
        number_of_directories = dir_count;
    }

    qsort(dirs, dir_count, sizeof(DirInfo), compare_dir_info);
    printf("Os %d diretórios maiores gastadores de espaço em %s:\n\n",
            number_of_directories, base_path);

    for (int i = 0; i < number_of_directories && i < dir_count; i++)
    {
        char *formatted_size;
        if ((formatted_size = format_size(dirs[i].size)) == NULL)
        {
            fprintf(stderr, "Erro de alocação de memória");
            destruct_dirs(dirs, dir_count);
            exit(EXIT_FAILURE);
        }

        printf("%s: %s\n", dirs[i].path, formatted_size);
        free(formatted_size);
    }

    return;
}

/* Cuida de calcular o tamanho do diretório. Calcula o tamanho
 * do direório base, não calcula subdiretórios. */
long long calculate_directory_size(char *directory)
{
    long long total_size = 0;

    DIR *dhandle;
    if ((dhandle = opendir(directory)) == NULL)
    {
        return(-1);
    }

    struct dirent *dentry;
    while ((dentry = readdir(dhandle)) != NULL)
    {
        if (is_dot_directory(dentry->d_name) == true)
        {
            continue;
        }

        char *fullpath;
        if ((fullpath = create_pathname(directory,
            dentry->d_name)) == NULL)
        {
            closedir(dhandle);
            return(-1);
        }

        /* Não seguimos links simbólicos */
        struct stat itemstat;
        if (lstat(fullpath, &itemstat) == -1)
        {
            free(fullpath);
            continue;
        }

        if (S_ISDIR(itemstat.st_mode))
        {
            total_size += calculate_directory_size(fullpath);
        }
        else
        {
            total_size += itemstat.st_size;
        }

        free(fullpath);
    }

    closedir(dhandle);

    return(total_size);
}


/* Varre a árvore do diretório ignorando arquivos, conta apenas
 * os subdiretórios */
void sweep_directory(char *base_path, int number_of_directories)
{
    DIR *dhandle;
    if ((dhandle = opendir(base_path)) == NULL)
    {
        fprintf(stderr, "Erro ao abrir %s\n", base_path);
        exit(EXIT_FAILURE);
    }

    /* Não usamos malloc, mas simplesmente realloc mais abaixo
     * porque não faz sentido alocarmos um tamanho inicial pois
     * não sabemos quantos diretórios temos. Então, alocamos um
     * a um. */
    int dir_count = 0;
    DirInfo *dirs = NULL;

    struct dirent *dentry;
    while ((dentry = readdir(dhandle)) != NULL)
    {

        /* Ignora o diretório . e o .. */
        if (is_dot_directory(dentry->d_name) == true)
        {
            continue;
        }

        char *fullpath;
        if ((fullpath = create_pathname(base_path,
            dentry->d_name)) == false)
        {
            fprintf(stderr, "Erro ao construir caminho para %s\n",
                dentry->d_name);
            destruct_dirs(dirs, dir_count);
            closedir(dhandle);
            exit(EXIT_FAILURE);
        }

        struct stat dirstat;

        /* Não segue os links simbólicos */
        if (lstat(fullpath, &dirstat) == -1)
        {
            free(fullpath);
            continue;
        }

        /* Ignora arquivos */
        if (!S_ISDIR(dirstat.st_mode))
        {
            free(fullpath);
            continue;
        }

        long long dir_size;
        if ((dir_size = calculate_directory_size(fullpath)) == -1)
        {
            /* Aqui o mais provável de acontecer é falta de permissões
             * no diretório. Mas como isto é apenas para praticar e não
             * programa de produção, whatever... */
            fprintf(stderr, "Erro calculando tamanho de %s\n", fullpath);
            free(fullpath);
            destruct_dirs(dirs, dir_count);
            closedir(dhandle);
            exit(EXIT_FAILURE);
        }

        /* Não queremos diretórios com tamanho 0.0 KB */
        if (dir_size >= 1)
        {
            /* Um diretótio por vez apenas */
            dirs = realloc(dirs, (dir_count + 1) * sizeof(DirInfo));
            if (dirs == NULL)
            {
                fprintf(stderr, "Erro ao alocar memória");
                free(fullpath);
                destruct_dirs(dirs, dir_count);
                closedir(dhandle);
                exit(EXIT_FAILURE);
            }

            /* Podia ser strdup(), mas fiz um "clone" em utils */
            dirs[dir_count].path = duplicate(fullpath);
            dirs[dir_count].size = dir_size;
            dir_count++;
        }

        free(fullpath);
    }

    /* Aqui, pode acontecer de dirs ser NULL */
    if (dirs != NULL)
    {
        print_sizes(base_path, dirs, dir_count, number_of_directories);
        destruct_dirs(dirs, dir_count);
    }

    closedir(dhandle);
}


int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr,
            "Uso: %s <diretório> <N maiores gastadores de espaço>\n",
            *argv);
        return(1);
    }

    argv++;
    char *directory = *argv;
    char *endptr;
    long number_dirs_to_show = strtol(*(argv + 1), &endptr, 10);

    /* Valida que diretório é ok */
    if (is_valid_directory(directory) == false)
    {
        fprintf(stderr, "Diretório %s não exste ou você"
        " não tem permissão de acesso\n", directory);
        return(1);
    }

    /* Valida que número é valido */
    if (*endptr != '\0' || number_dirs_to_show < 1)
    {
        fprintf(stderr, "Número de diretórios inválido\n");
        return(1);
    }

    /* Remove a barra (/) do fim do diretório.
     * A função create_pathname em utils já
     * coloca a barra. */
    remove_last_char(directory, '/');
    sweep_directory(directory, (int)number_dirs_to_show);
    return(0);
}
