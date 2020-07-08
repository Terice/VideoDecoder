cpi=g++
ofile=../ofile
CFLAGS+= -g -c 
LDFLAGS+= -llua5.1
a:main.cpp $(ofile)/bitsbuf.o \
 $(ofile)/NAL.o\
 $(ofile)/slice.o \
 $(ofile)/Parser.o $(ofile)/cabac.o \
 $(ofile)/macroblock.o \
 $(ofile)/picture.o $(ofile)/block.o $(ofile)/residual.o\
 $(ofile)/matrix.o $(ofile)/Decoder.o $(ofile)/Debug.o \
 $(ofile)/functions.o 
	$(cpi) -g        $^    -o  $@  $(LDFLAGS)

$(ofile)/%.o:%.cpp
	$(cpi) $(CFLAGS) $^    -o  $@
clean:
	rm $(ofile)/*.o ./a

earse:
	rm $(ofile)/Decoder.o $(ofile)/macroblock.o $(ofile)/Parser.o $(ofile)/cabac.o $(ofile)/slice.o    