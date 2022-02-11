.PHONY: all clean

GLFW_DIR ?= /storage/other/glfw/install
GLFW_CFLAGS := -L$(GLFW_DIR)/lib -lglfw3 -ldl -lm -lpthread
INCLUDE_DIRS := -I $(GLFW_DIR)/include -I glad/include -I cglm/include
CFLAGS := $(INCLUDE_DIRS) -g -Wall -Werror -O3

all: l_system_3d

utilities.o: utilities.c utilities.h
	gcc $(CFLAGS) -c -o utilities.o utilities.c -I glad/include

l_system_mesh.o: l_system_mesh.c l_system_mesh.h utilities.h
	gcc $(CFLAGS) -c -o l_system_mesh.o l_system_mesh.c -I glad/include \
		-I cglm/include

turtle_3d.o: turtle_3d.c turtle_3d.h
	gcc $(CFLAGS) -c -o turtle_3d.o turtle_3d.c -I cglm/include

parse_config.o: parse_config.c parse_config.h turtle_3d.h
	gcc $(CFLAGS) -c -o parse_config.o parse_config.c

l_system_3d: l_system_3d.c l_system_mesh.o turtle_3d.o utilities.o \
	parse_config.o
	gcc $(CFLAGS) -o l_system_3d l_system_3d.c \
		glad/src/glad.c \
		utilities.o \
		l_system_mesh.o \
		turtle_3d.o \
		parse_config.o \
		-I glad/include \
		-I cglm/include \
		$(GLFW_CFLAGS)

clean:
	rm -f *.o
	rm -f l_system_3d

