CC = gcc
CFLAGS = -g3 -Wall -Wextra
LDFLAGS =

BDIR = bin
ODIR = build
IDIR = include
SDIR = src

EXECUTABLE = diseaseAggregator

_DEPS = diseaseAggregator.h worker.h pipes.h patient.h date.h list.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o diseaseAggregator.o worker.o pipes.o patient.o date.o list.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(BDIR)/$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean run valgrind

run:
	./$(BDIR)/$(EXECUTABLE) -w 4 -b 512 -i ./data

valgrind:
	valgrind --leak-check=full ./$(BDIR)/$(EXECUTABLE) -w 4 -b 5 -i ./data 

clean:
	rm -f pipes/r_* pipes/w_*
	rm -f $(ODIR)/*.o
	rm -f $(BDIR)/$(EXECUTABLE)