NAME =		ircserv

OBJ_DIR =	obj/

INC = \
			include/Channel.hpp \
			include/Client.hpp \
			include/Server.hpp \
			include/utils.hpp

SRC = \
			src/Channel.cpp \
			src/Client.cpp \
			src/main.cpp \
			src/Server.cpp \
			src/utils.cpp

OBJ =		$(patsubst %.cpp,$(OBJ_DIR)%.o,$(SRC))

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic -g

RM = rm -f

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

$(OBJ_DIR)%.o: %.cpp $(INC)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) -r $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

val:
	valgrind --leak-check=full --leak-check=full \
	--show-leak-kinds=definite,indirect,possible \
	--track-fds=yes \
	./$(NAME) 6667 password

.PHONY: all clean fclean re val
