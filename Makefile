CFLAGS=-g -O2 -Wall -Wextra -Isrc -DNDEBUG $(OPTFLAGS)
LIBS=-ldl $(OPTLIBS)
PREFIX?=/usr/local
BINPREFIX?=dito-

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

PROGRAMS_SRC=$(wildcard bin/*.c)
PROGRAMS=$(patsubst %.c,%,$(PROGRAMS_SRC))

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

TARGET=build/libdito.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))


all: $(TARGET) $(SO_TARGET) $(PROGRAMS)

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra $(OPTFLAGS)
dev: all

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@

$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

$(PROGRAMS): CFLAGS += $(TARGET)
$(PROGRAMS): $(TARGET)

build:
	@mkdir -p build
	@mkdir -p bin

.PHONY: tests
tests: CFLAGS += $(TARGET)
tests: $(TESTS)
	sh ./tests/runtests.sh

$(TESTS): $(TARGET)

valgrind:
	VALGRIND="valgrind --leak-check=full --suppressions=valgrind_osx.supp --log-file=/tmp/valgrind-%p.log" $(MAKE)

install: all
	install -d $(DESTDIR)$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include
	install src/dito.h $(DESTDIR)$(PREFIX)/include/
	install -d $(DESTDIR)$(PREFIX)/bin/
	install bin/generate $(DESTDIR)$(PREFIX)/bin/$(BINPREFIX)generate
	install bin/extract $(DESTDIR)$(PREFIX)/bin/$(BINPREFIX)extract
	install bin/format $(DESTDIR)$(PREFIX)/bin/$(BINPREFIX)format
	install bin/ls $(DESTDIR)$(PREFIX)/bin/$(BINPREFIX)ls
	install bin/mkdir $(DESTDIR)$(PREFIX)/bin/$(BINPREFIX)mkdir
	install bin/cp $(DESTDIR)$(PREFIX)/bin/$(BINPREFIX)cp
	install bin/rm $(DESTDIR)$(PREFIX)/bin/$(BINPREFIX)rm
	install bin/rmdir $(DESTDIR)$(PREFIX)/bin/$(BINPREFIX)rmdir

install-homebrew:
	PREFIX="`brew --cellar`/dito/0.1.0/" $(MAKE) install
	brew unlink dito
	brew link dito


clean:
	rm -rf build $(OBJECTS) $(TESTS) $(PROGRAMS)
	rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`

