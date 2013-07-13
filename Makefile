BIN = sprdump

CC     = gcc
BTYPE  = -O2  # Build Type
CFLAGS = -std=gnu99 -Wall -Wno-unused-result ${BTYPE}
LIBS   = -lm

SRC  = asprintf.c bmpfile.c buffer.c main.c
OBJ  = obj
OBJS = $(SRC:%.c=$(OBJ)/%.o)

all: $(BIN)
clean:
	$(RM) $(OBJ)/*.o
	$(RM) $(BIN)

${BIN}: $(OBJ) $(OBJS)
	@echo "  LD     $@"
	@$(CC) ${CFLAGS} -o $@ $(OBJS) ${LIBS}

${OBJ}/%.o: %.c
	@echo "  CC     $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

$(OBJ):
	@mkdir -p $(OBJ)

