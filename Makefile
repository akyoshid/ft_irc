CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic
DEP_FLAGS = -MMD -MP

NAME = ircserv
BOT_NAME = ircbot
BUILD_DIR = build
INC_DIR = include
SRC_DIR = src

SRC = \
			$(SRC_DIR)/Channel.cpp \
			$(SRC_DIR)/User.cpp \
			$(SRC_DIR)/main.cpp \
			$(SRC_DIR)/Server.cpp \
			$(SRC_DIR)/utils.cpp \
			$(SRC_DIR)/EventLoop.cpp \
			$(SRC_DIR)/ConnectionManager.cpp \
			$(SRC_DIR)/CommandParser.cpp \
			$(SRC_DIR)/UserManager.cpp \
			$(SRC_DIR)/ChannelManager.cpp \
			$(SRC_DIR)/ResponseFormatter.cpp \
			$(SRC_DIR)/CommandRouter.cpp
OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
DEP = $(OBJ:.o=.d)

BOT_SRC = \
			$(SRC_DIR)/BotClient.cpp \
			$(SRC_DIR)/bot.cpp \
			$(SRC_DIR)/utils.cpp \
			$(SRC_DIR)/EventLoop.cpp
BOT_OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(BOT_SRC))
BOT_DEP = $(BOT_OBJ:.o=.d)

.PHONY: all
all: $(NAME) $(BOT_NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@
$(BOT_NAME): $(BOT_OBJ)
	$(CXX) $(CXXFLAGS) $(BOT_OBJ) -o $@
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(DEP_FLAGS) -I$(INC_DIR) -c $< -o $@
.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEP) $(BOT_DEP)

.PHONY: fclean
fclean: clean
	$(RM) $(NAME) $(BOT_NAME)

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
