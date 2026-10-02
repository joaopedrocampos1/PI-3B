# Makefile do PI-3B (adaptado do modelo da disciplina)
#
#   make            compila bin/grafos
#   make test       compila e roda os testes de tests/
#   make run ARGS="--input data/raw/twitter_combined.txt"
#   make clean

CC      = gcc
# -O2: os tempos medidos (RF03) devem refletir o algoritmo, não código sem otimização
# -D_POSIX_C_SOURCE: com -std=c11, clock_gettime (tempo) e getrusage (memória) não são declarados
CFLAGS  = -std=c11 -O2 -Wall -Wextra -pedantic -Werror -D_POSIX_C_SOURCE=200809L -Iinclude
# bibliotecas vêm depois dos .c no comando, senão o linker as descarta
LDLIBS  = -lm
TARGET  = bin/grafos

SRC     = $(wildcard src/*.c)
HDR     = $(wildcard include/*.h)
# os testes têm main próprio: usam todos os módulos, menos o main.c
MODULOS = $(filter-out src/main.c, $(SRC))
TESTES  = $(patsubst tests/%.c, bin/%, $(wildcard tests/test_*.c))

all: $(TARGET)

$(TARGET): $(SRC) $(HDR) | bin
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

bin/test_%: tests/test_%.c $(MODULOS) $(HDR) | bin
	$(CC) $(CFLAGS) -g -o $@ $< $(MODULOS) $(LDLIBS)

test: $(TESTES)
	@for t in $(TESTES); do echo "== $$t"; ./$$t || exit 1; done

bin:
	mkdir -p bin

clean:
	rm -f $(TARGET) $(TESTES)

run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: all test clean run
