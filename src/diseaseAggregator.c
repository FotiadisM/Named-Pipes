#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <dirent.h>

#include "../include/diseaseAggregator.h"
#include "../include/pipes.h"

static int DA_DevideWork(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize, const char *input_dir);
static int DA_main(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize);
static int DA_wait_input(const worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize);

static void listCountries(const worker_infoPtr workers_array, const int numWorkers);
static void diseaseFrequency(const worker_infoPtr workers_array, const int numWorkers, const char *str, const wordexp_t *p, const size_t bufferSize);
static void searchPatientRecord(const worker_infoPtr workers_array, const int numWorkers, const char *str, const wordexp_t *p, const size_t bufferSize);
static void topk_AgeRanges(const worker_infoPtr workers_array, const int numWorkers, const char *str, const wordexp_t *p, const size_t bufferSize);
static void general(const worker_infoPtr workers_array, const int numWorkers, const char *str, const wordexp_t *p, const size_t bufferSize);

int DA_Run(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize, const char *input_dir)
{
    char *buffer = NULL;

    if ((buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
        return -1;
    }

    if (DA_DevideWork(workers_array, numWorkers, bufferSize, input_dir) == -1)
    {
        printf("DA_DevideWork() failed\n");
        return -1;
    }

    if (DA_main(workers_array, numWorkers, bufferSize) == -1)
    {
        printf("DA_main() failed");
        return -1;
    }

    if (DA_wait_input(workers_array, numWorkers, bufferSize) == -1)
    {
        printf("DA_wait_input() failed");
        return -1;
    }

    free(buffer);

    return 0;
}

static int DA_DevideWork(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize, const char *input_dir)
{
    int count = 0, flag = 1;
    DIR *dirp = NULL;
    struct dirent *dir_info = NULL;

    if ((dirp = opendir(input_dir)) == NULL)
    {
        perror("opendir()");
        return -1;
    }

    while ((dir_info = readdir(dirp)) != NULL)
    {
        if (!(strcmp(dir_info->d_name, ".") == 0 || strcmp(dir_info->d_name, "..") == 0))
        {
            encode(workers_array[count].w_fd, dir_info->d_name, bufferSize);

            if ((workers_array[count].countries_list = add_stringNode(workers_array[count].countries_list, dir_info->d_name)) == NULL)
            {
                printf("add_string_node failed");
                return -1;
            }

            if (++count == numWorkers)
            {
                flag = 0;
                count = 0;
            }
        }
    }

    for (int i = 0; i < numWorkers; i++)
    {
        // Need to handle DrisNumber < numWorkers case
        if (flag && i >= count)
        {
            write(workers_array[count].w_fd, "/exit", bufferSize);
        }
        else
        {
            encode(workers_array[i].w_fd, "OK", bufferSize);
        }
    }

    if (closedir(dirp) == -1)
    {
        perror("closedirp");
    }

    return 0;
}

static int DA_main(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize)
{
    fd_set fds;
    FILE *filePtr = NULL;
    int maxfd = 0, count = 0;
    char *buffer = NULL, *str = NULL;

    if ((buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
        return -1;
    }

    if ((filePtr = fopen("./logs/stats.txt", "w+")) == NULL)
    {
        perror("open file");
        return -1;
    }

    while (1)
    {
        FD_ZERO(&fds);

        for (int i = 0; i < numWorkers; i++)
        {
            FD_SET(workers_array[i].r_fd, &fds);

            if (workers_array[i].r_fd > maxfd)
            {
                maxfd = workers_array[i].r_fd;
            }
        }

        if (pselect(maxfd + 1, &fds, NULL, NULL, NULL, NULL) == -1)
        {
            perror("select()");
        }

        for (int i = 0; i < numWorkers; i++)
        {
            if (FD_ISSET(workers_array[i].r_fd, &fds))
            {
                str = decode(workers_array[i].r_fd, buffer, bufferSize);

                if (!strcmp(str, "OK"))
                {
                    count++;
                }
                else
                {
                    fprintf(filePtr, "%s", str);
                }

                free(str);
            }
        }

        if (count == numWorkers)
        {
            break;
        }
    }

    fclose(filePtr);

    free(buffer);

    return 0;
}

static int DA_wait_input(const worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize)
{
    wordexp_t p;
    size_t len = 0;
    char *str = NULL;

    while (getline(&str, &len, stdin) != -1)
    {
        strtok(str, "\n");
        wordexp(str, &p, 0);

        if (!strcmp(p.we_wordv[0], "/exit"))
        {
            for (int i = 0; i < numWorkers; i++)
            {
                encode(workers_array[i].w_fd, str, bufferSize);
            }

            wordfree(&p);

            break;
        }
        else if (!strcmp(p.we_wordv[0], "/listCountries"))
        {
            listCountries(workers_array, numWorkers);
        }
        else if (!strcmp(p.we_wordv[0], "/diseaseFrequency"))
        {
            diseaseFrequency(workers_array, numWorkers, str, &p, bufferSize);
        }
        else if (!strcmp(p.we_wordv[0], "/topk-AgeRanges"))
        {
            topk_AgeRanges(workers_array, numWorkers, str, &p, bufferSize);
        }
        else if (!strcmp(p.we_wordv[0], "/searchPatientRecord"))
        {
            searchPatientRecord(workers_array, numWorkers, str, &p, bufferSize);
        }
        else if (!strcmp(p.we_wordv[0], "/numPatientAdmissions"))
        {
            general(workers_array, numWorkers, str, &p, bufferSize);
        }
        else if (!strcmp(p.we_wordv[0], "/numPatientDischarges"))
        {
            general(workers_array, numWorkers, str, &p, bufferSize);
        }

        wordfree(&p);
    }

    free(str);

    return 0;
}

static int getWorker(const worker_infoPtr workers_array, const int numWorkers, const char *str)
{
    string_nodePtr node = NULL;

    if (str != NULL)
    {
        for (int i = 0; i < numWorkers; i++)
        {
            node = workers_array[i].countries_list;

            while (node != NULL)
            {
                if (!strcmp(node->str, str))
                {
                    return i;
                }
                node = node->next;
            }
        }
    }

    return -1;
}

static int await_answear(const worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize)
{
    fd_set fds;
    int maxfd = 0, count = 0, total = 0;
    char *buffer = NULL, *str = NULL;

    if ((buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
        return -1;
    }

    while (1)
    {
        FD_ZERO(&fds);

        for (int i = 0; i < numWorkers; i++)
        {
            FD_SET(workers_array[i].r_fd, &fds);

            if (workers_array[i].r_fd > maxfd)
            {
                maxfd = workers_array[i].r_fd;
            }
        }

        if (pselect(maxfd + 1, &fds, NULL, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            return -1;
        }

        for (int i = 0; i < numWorkers; i++)
        {
            if (FD_ISSET(workers_array[i].r_fd, &fds))
            {
                str = decode(workers_array[i].r_fd, buffer, bufferSize);

                total += (int)strtol(str, NULL, 10);

                count++;

                free(str);
            }
        }

        if (count == numWorkers)
        {
            break;
        }
    }

    free(buffer);

    return total;
}

static void listCountries(const worker_infoPtr workers_array, const int numWorkers)
{
    string_nodePtr node = NULL;

    for (int i = 0; i < numWorkers; i++)
    {
        node = workers_array[i].countries_list;
        while (node != NULL)
        {
            printf("%s %u\n", node->str, workers_array[i].pid);
            node = node->next;
        }
        printf("\n");
    }
}

static void diseaseFrequency(const worker_infoPtr workers_array, const int numWorkers, const char *str, const wordexp_t *p, const size_t bufferSize)
{
    int index = 0;
    char *buffer = NULL, *answ = NULL;

    if (p->we_wordc == 4 || p->we_wordc == 5)
    {
        if ((index = getWorker(workers_array, numWorkers, p->we_wordv[4])) == -1)
        {
            for (int i = 0; i < numWorkers; i++)
            {
                encode(workers_array[i].w_fd, str, bufferSize);
            }
            printf("%d\n", await_answear(workers_array, numWorkers, bufferSize));
        }
        else
        {
            encode(workers_array[index].w_fd, str, bufferSize);
            buffer = malloc(bufferSize);
            answ = decode(workers_array[index].r_fd, buffer, bufferSize);
            printf("%s\n", answ);

            free(answ);
            free(buffer);
        }
    }
    else
    {
        printf("Invalid input\n");
    }
}

static void general(const worker_infoPtr workers_array, const int numWorkers, const char *str, const wordexp_t *p, const size_t bufferSize)
{
    int index = 0;

    if (p->we_wordc == 4 || p->we_wordc == 5)
    {
        if ((index = getWorker(workers_array, numWorkers, p->we_wordv[4])) == -1)
        {
            for (int i = 0; i < numWorkers; i++)
            {
                encode(workers_array[i].w_fd, str, bufferSize);
            }
        }
        else
        {
            encode(workers_array[index].w_fd, str, bufferSize);
        }
    }
    else
    {
        printf("Invalid input\n");
    }
}

static void topk_AgeRanges(const worker_infoPtr workers_array, const int numWorkers, const char *str, const wordexp_t *p, const size_t bufferSize)
{
    int index = 0;

    if (p->we_wordc == 6)
    {
        if ((index = getWorker(workers_array, numWorkers, p->we_wordv[2])) == -1)
        {
            printf("Country not registered\n");
        }
        else
        {
            encode(workers_array[index].w_fd, str, bufferSize);
        }
    }
    else
    {
        printf("Invalid input\n");
    }
}

static void searchPatientRecord(const worker_infoPtr workers_array, const int numWorkers, const char *str, const wordexp_t *p, const size_t bufferSize)
{
    if (p->we_wordc == 2)
    {
        for (int i = 0; i < numWorkers; i++)
        {
            encode(workers_array[i].w_fd, str, bufferSize);
        }
    }
    else
    {
        printf("Invalid input\n");
    }
}
