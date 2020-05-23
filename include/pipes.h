#ifndef PIPES_H
#define PIPES_H

int Pipe_Init(const char *path, const int pid, const int flags);

int encode(const int fd, const char *buffer, const size_t bufferSize);

char *decode(const int fd, char *buffer, const size_t bufferSize);

#endif