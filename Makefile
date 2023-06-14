NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3

SRCS_DIR = srcs
SRCS_SUBDIRS = $(shell find $(SRCS_DIR) -type d)
SRCS = $(foreach dir, $(SRCS_SUBDIRS), $(wildcard $(dir)/*.cpp))

OBJS_DIR = objs
OBJS = $(patsubst $(SRCS_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS))

INCLUDES = -I$(SRCS_DIR) $(addprefix -I, $(SRCS_SUBDIRS))

all: $(NAME)

debug: CXXFLAGS += -DDEBUG
debug: clean all

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) -r $(OBJS_DIR) unit-test/build

fclean: clean
	$(RM) $(NAME)

re:
	make fclean
	make all

.PHONY: all clean fclean re debug

docker:
	cd docker && docker compose up -d && docker exec -it ubuntu /bin/bash

test_echo:
	@(cd tests/echo_server && \
	docker-compose down > /dev/null && \
	docker-compose build > /dev/null && \
	docker-compose up -d --remove-orphans > /dev/null && \
	docker-compose logs -f client && \
	docker-compose down > /dev/null)

# 必要に応じて以下を追加
# docker-compose logs webserv &&

unit-test:
	(mkdir -p unit-test/build && \
	cd unit-test/build && \
	cmake .. && \
	make 2> ./make_error.log && \
	if [ "$(TEST_CASE)" != "" ]; then \
		ctest -V -R $(TEST_CASE); \
	else \
		ctest -E "case" && \
		for test in $$(ctest -N | grep case | awk '{print $$3}'); do \
			ctest -V -R $$test; \
		done \
	fi)

# unit-testのみで実行すると全てのテストが走ります
# TEST_CASEにテストケース名を指定するとそのテストのみ実行されます

.PHONY: docker test_echo unit-test
