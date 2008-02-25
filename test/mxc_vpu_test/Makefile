CC=$(CROSS_COMPILE)gcc
LINK=$(CROSS_COMPILE)gcc

OBJ = main.o \
      dec.o \
      enc.o \
      capture.o \
      display.o \
      fb.o \
      utils.o \
      loopback.o

# list of platforms which want this test case
INCLUDE_LIST:=IMX27ADS IMX31ADS IMX37_3STACK

ifeq ($(PLATFORM),$(findstring $(PLATFORM),$(INCLUDE_LIST)))
TARGET = mxc_vpu_test.out
else
TARGET =
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(LINK) -lpthread -o $(TARGET) $(OBJ) -lvpu
	mkdir -p $(OBJDIR)
	mv $(TARGET) $(OBJDIR)
	cp autorun-vpu.sh $(OBJDIR)
	cp ./configs/config* $(OBJDIR)
	cp ./configs/akiyo.mp4 $(OBJDIR)

%.o: %.c
	$(CC) -Wall -O2 -c $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJDIR)/$(TARGET) $(OBJ)
