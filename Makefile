NAME = ft_irc

COMP = c++


C_FLAGS = -Wall -Wextra -Werror -std=c++98 -fsanitize=address -Wshadow
DEPS_FLAGS = -I./headers -MMD -MP
FLAGS = $(C_FLAGS) $(DEPS_FLAGS)


BUILD_DIR = build

CPP_CLASS = class/client.cpp \
			class/server.cpp

CPP_FILES = verifs/check_args.cpp

SRC = main.cpp \
	$(addprefix src/, $(CPP_FILES))\
	$(addprefix src/, $(CPP_CLASS))


HPP_FILES = ft_irc.hpp \
			Client.hpp \
			Server.hpp


HEADERS = $(addprefix headers/, $(HPP_FILES))

OBJ = $(SRC:%.cpp=$(BUILD_DIR)/%.o)

DEPS = $(OBJ:.o=.d)

##############
#RULES

.PHONY: all clean fclean re
.DEFAULT_GOAL = all

all : $(NAME)

$(NAME) : $(OBJ)
	$(COMP) $(FLAGS) $(OBJ) -o $(NAME)

$(BUILD_DIR)/%.o : %.cpp Makefile #$(HEADERS)
	@mkdir -p $(dir $@)
	$(COMP) $(FLAGS) -c $< -o $@

clean :
	rm -rf $(BUILD_DIR)

fclean : clean
	rm -f $(NAME)

re : fclean all

-include $(DEPS)
