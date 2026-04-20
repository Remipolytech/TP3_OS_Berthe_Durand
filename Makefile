CC = gcc
# la moindre alerte de compilation bloquera la génération
CFLAGS = -Wall -Werror
# readline pour l'invite de commande, pthread pour le multi-threading
LDFLAGS = -lreadline -lpthread

SRC = biceps.c gescom.c servbeuip.c creme.c
OBJ = $(SRC:.c=.o)

default: biceps

biceps: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cible spécifique pour traquer les fuites mémoire avec Valgrind
# On désactive les optimisations (-O0) et on ajoute les symboles de débogage (-g)
memory-leak: CFLAGS += -g -O0
memory-leak: $(SRC:.c=.o-leak)
	$(CC) $(CFLAGS) -o biceps-memory-leaks $^ $(LDFLAGS)

%.o-leak: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage complet du dépôt pour ne laisser que le code source
clean:
	rm -f biceps biceps-memory-leaks *.o *.o-leak