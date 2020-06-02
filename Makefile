CC = gcc
CFLAGS = -g3 -Wall -Wextra
LDFLAGS =

BDIR = bin
ODIR = build
IDIR = include
SDIR = src

EXECUTABLE = diseaseAggregator

_DEPS = diseaseAggregator.h worker.h pipes.h stats.h list.h hashTable.h AVL.h patient.h date.h 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o diseaseAggregator.o worker.o pipes.o stats.o list.o hashTable.o AVL.o patient.o date.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(BDIR)/$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean run valgrind

run:
	./$(BDIR)/$(EXECUTABLE) -w 4 -b 5 -i ./data

valgrind:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./$(BDIR)/$(EXECUTABLE) -w 4 -b 5 -i ./data 

clean:
	rm -f logs/log*
	rm -f pipes/r_* pipes/w_*
	rm -f $(ODIR)/*.o
	rm -f $(BDIR)/$(EXECUTABLE)