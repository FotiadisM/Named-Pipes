#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../include/worker.h"
#include "../include/pipes.h"

int Worker_Run(const size_t bufferSize, const char *input_dir)
{
    int r_fd = 0, w_fd = 0;
    char *buffer = NULL;
    toDoPtr toDos = NULL;

    if ((buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
        return -1;
    }

    if ((w_fd = Pipe_Init("./pipes/r_", getpid(), O_WRONLY)) == -1)
    {
        printf("Pipe_Init() failed\n");
        return -1;
    }

    if ((r_fd = Pipe_Init("./pipes/w_", getpid(), O_RDONLY)) == -1)
    {
        printf("Pipe_Init() failed");
        return -1;
    }

    while (1)
    {
        read(r_fd, buffer, bufferSize);

        if (strcmp(buffer, "OK") == 0)
        {
            break;
        }

        else if (strcmp(buffer, "/exit") == 0)
        {
            if (close(r_fd) == -1 || close(w_fd) == -1)
            {
                perror("close");
            }

            free(buffer);

            return 0;
        }

        else
        {
            if ((toDos = add_toDo(toDos, buffer)) == NULL)
            {
                write(w_fd, "ERR", strlen("ERR") + 1);
            }
            else
            {
                write(w_fd, "OK", strlen("OK") + 1);
            }
        }
    }

    clear_toDos(toDos);

    if (close(r_fd) == -1 || close(w_fd) == -1)
    {
        perror("close");
    }

    free(buffer);

    return 0;
}

toDoPtr add_toDo(toDoPtr toDos, char *country)
{
    toDoPtr newToDo = NULL;

    if ((newToDo = malloc(sizeof(toDo))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    if ((newToDo->country = malloc(strlen(country) + 1)) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    strncpy(newToDo->country, country, strlen(country) + 1);
    newToDo->status = 0;

    newToDo->next = toDos;
    toDos = newToDo;

    return newToDo;
}

void clear_toDos(toDoPtr toDos)
{
    toDoPtr tmpToDo = NULL;

    while (toDos != NULL)
    {
        tmpToDo = toDos;
        toDos = toDos->next;

        free(tmpToDo->country);
        free(tmpToDo);
    }
}