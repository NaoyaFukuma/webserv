NAME = webserv

CXX = g++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3

SRCS_DIR = srcs
SRCS = $(wildcard $(SRCS_DIR)/*.cpp)

OBJS_DIR = objs
OBJS = $(patsubst $(SRCS_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS))

INCLUDES = -I$(SRCS_DIR) $(addprefix -I, $(SRCS_SUBDIRS))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) -r $(OBJS_DIR)

fclean: clean
	$(RM) $(NAME)

.PHONY: all clean fclean re

docker:
	cd docker && docker compose up -d && docker exec -it debian /bin/bash

.PHONY: docker