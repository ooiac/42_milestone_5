NAME		= webserv

CXX		= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

SRCDIR		= src
OBJDIR		= obj

SRCS		= $(SRCDIR)/main.cpp \
		  $(SRCDIR)/Config.cpp \
		  $(SRCDIR)/HttpRequest.cpp \
		  $(SRCDIR)/HttpResponse.cpp \
		  $(SRCDIR)/CgiHandler.cpp \
		  $(SRCDIR)/Client.cpp \
		  $(SRCDIR)/Server.cpp \
		  $(SRCDIR)/SessionManager.cpp \
		  $(SRCDIR)/Utils.cpp

OBJS		= $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
