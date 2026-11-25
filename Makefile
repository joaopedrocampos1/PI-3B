# Makefile do PI-3B (adaptado do modelo da disciplina)
#
#   make            compila bin/grafos
#   make test       compila e roda os testes de tests/
#   make run ARGS="--input data/raw/twitter_combined.txt"
#   make clean
#
# No Windows (MinGW-w64), troque `make` por `mingw32-make`. Funciona tanto
# no PowerShell/cmd quanto no Git Bash: veja "Windows" logo abaixo.

CC      = gcc
# -O2: os tempos medidos (RF03) devem refletir o algoritmo, não código sem otimização
# -D_POSIX_C_SOURCE: com -std=c11, clock_gettime (tempo) e getrusage (memória) não são declarados
CFLAGS  = -std=c11 -O2 -Wall -Wextra -pedantic -Werror -D_POSIX_C_SOURCE=200809L -Iinclude
# bibliotecas vêm depois dos .c no comando, senão o linker as descarta
LDLIBS  = -lm

# --- Windows ---------------------------------------------------------------
# O gcc do MinGW grava bin/grafos.exe; sem o .exe no nome do alvo, o make não
# acharia o arquivo e recompilaria tudo a cada chamada.
# -lpsapi: GetProcessMemoryInfo (src/memtrack.c). O `#pragma comment(lib, ...)`
# de lá só vale no Visual Studio; MinGW recente nem precisa, o antigo precisa.
ifeq ($(OS),Windows_NT)
  EXE     = .exe
  LDLIBS += -lpsapi
endif

# Sem um sh.exe no PATH (PowerShell ou cmd puros), o mingw32-make deixa SHELL
# como "sh.exe" e roda as receitas no cmd.exe, que não conhece mkdir -p,
# rm -f nem ./programa. No Git Bash o SHELL vem com o caminho completo.
ifeq ($(SHELL),sh.exe)
  CRIAR_BIN = if not exist bin mkdir bin
  APAGAR    = -del /q /f
  caminho   = $(subst /,\,$(1))
else
  CRIAR_BIN = mkdir -p bin
  APAGAR    = rm -f
  caminho   = $(1)
endif
# ---------------------------------------------------------------------------

TARGET  = bin/grafos$(EXE)

SRC     = $(wildcard src/*.c)
HDR     = $(wildcard include/*.h)
# os testes têm main próprio: usam todos os módulos, menos o main.c
MODULOS = $(filter-out src/main.c, $(SRC))
TESTES  = $(patsubst tests/%.c, bin/%$(EXE), $(wildcard tests/test_*.c))

all: $(TARGET)

$(TARGET): $(SRC) $(HDR) | bin
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

bin/test_%$(EXE): tests/test_%.c $(MODULOS) $(HDR) | bin
	$(CC) $(CFLAGS) -g -o $@ $< $(MODULOS) $(LDLIBS)

# Um comando por teste (sem laço do shell, que muda entre sh e cmd): o make
# para no primeiro que falhar.
define rodar_teste
@echo == $(1)
@$(call caminho,./$(1))

endef

test: $(TESTES)
	$(foreach t,$(TESTES),$(call rodar_teste,$(t)))

bin:
	$(CRIAR_BIN)

clean:
	$(APAGAR) $(call caminho,$(TARGET) $(TESTES))

run: $(TARGET)
	$(call caminho,./$(TARGET)) $(ARGS)

.PHONY: all test clean run
