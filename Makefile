# Makefile for src/mod/webseen.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../webseen.o

modules: ../../../webseen.so

../webseen.o: ../module.h ../modvals.h ../../eggdrop.h webseen.c \
  mini_httpd.c templates.c webseen_templates.c wstcl.c http_processing.c \
  webseen_templates.c webseen.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c webseen.c
	rm -f ../webseen.o
	mv webseen.o ../

../../../webseen.so: ../webseen.o
	$(LD) -o ../../../webseen.so ../webseen.o
	$(STRIP) ../../../webseen.so

#safety hash
