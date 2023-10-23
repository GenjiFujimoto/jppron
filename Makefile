SRC = findpron.c
OBJ = $(SRC:.c=.o)

.c.o:
	$(CC) -c $<

findpron: ${OBJ}
	$(CC) -o $@ ${OBJ}

clean:
	rm -f findpron ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f findpron ${DESTDIR}${PREFIX}/bin
	cp -f jppron ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/findpron
	rm -f ${DESTDIR}${PREFIX}/bin/jppron

.PHONY: all options clean install uninstall
