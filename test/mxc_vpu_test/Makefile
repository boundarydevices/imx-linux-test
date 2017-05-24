CC ?=$(CROSS_COMPILE)gcc
LINK ?=$(CROSS_COMPILE)gcc

OBJ = main.o \
      dec.o \
      enc.o \
      capture.o \
      display.o \
      fb.o \
      utils.o \
      loopback.o \
      transcode.o


TARGET = mxc_vpu_test.out

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) ${LIBS} -lvpu -lipu -lrt -lpthread
	mkdir -p $(OBJDIR)
	mv $(TARGET) $(OBJDIR)
	cp autorun-vpu.sh $(OBJDIR)
	cp ./configs/config* $(OBJDIR)
	cp ./configs/akiyo.mp4 $(OBJDIR)

%.o: %.c
	$(CC) $(INC) -Wall -O2 -c $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJDIR)/$(TARGET) $(OBJ)

include ../make.rules
