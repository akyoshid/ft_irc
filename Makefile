CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic
DEP_FLAGS = -MMD -MP

NAME = ircserv
BUILD_DIR = build
INC_DIR = include
SRC_DIR = src

SRC = \
			$(SRC_DIR)/Channel.cpp \
			$(SRC_DIR)/Client.cpp \
			$(SRC_DIR)/main.cpp \
			$(SRC_DIR)/Server.cpp \
			$(SRC_DIR)/utils.cpp \
			$(SRC_DIR)/Message.cpp \
			$(SRC_DIR)/server/EventLoop.cpp \
			$(SRC_DIR)/server/ConnectionManager.cpp \
			$(SRC_DIR)/server/CommandParser.cpp

OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
DEP = $(OBJ:.o=.d)

.PHONY: all
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(DEP_FLAGS) -I$(INC_DIR) -c $< -o $@

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEP)

.PHONY: fclean
fclean: clean
	$(RM) $(NAME)

.PHONY: re
re: fclean all

.PHONY: val
val:
	valgrind --leak-check=full \
	--show-leak-kinds=definite,indirect,possible \
	--track-fds=yes \
	./$(NAME) 6667 password

.PHONY: format
format:
	clang-format -i $(SRC_DIR)/*.cpp $(INC_DIR)/*.hpp

.PHONY: lint
lint:
	clang-tidy $(SRC) -- -I$(INC_DIR) -std=c++98
