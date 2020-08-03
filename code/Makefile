CC=g++

BIN=../bin
OBJ=../obj

CFLAGS+= -g -c -std=c++11
LDFLAGS+= -llua5.1 
#target=../bin/a
target=a


$(target):main.cpp $(OBJ)/bitsbuf.o \
 $(OBJ)/NAL.o\
 $(OBJ)/slice.o \
 $(OBJ)/Parser.o $(OBJ)/cabac.o \
 $(OBJ)/macroblock.o \
 $(OBJ)/picture.o $(OBJ)/block.o $(OBJ)/residual.o\
 $(OBJ)/matrix.o $(OBJ)/Decoder.o $(OBJ)/Debug.o \
 $(OBJ)/functions.o 
	$(CC) -g        $^    -o  $@  $(LDFLAGS)
$(OBJ)/%.o:%.cpp
	@if [ ! -d $(OBJ) ]; then mkdir -p $(OBJ); fi;
	@if [ ! -d $(BIN) ]; then mkdir -p $(BIN); fi;
	$(CC) $(CFLAGS) $^    -o  $@
clean:
	@if [ -d $(BIN) ]; then rm -r $(BIN); fi;
	@rm $(OBJ)/*.o ./a