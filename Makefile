
ALL:	config_parse.c config_parse.h
	@-astyle -n --style=linux --mode=c --pad-oper --pad-paren-in --unpad-paren --break-blocks --delete-empty-lines --min-conditional-indent=0 --max-instatement-indent=80 --indent-col1-comments --indent-switches --lineend=linux *.{c,h} >/dev/null
	@gcc -g3 -Wall -Wextra -c config_parse.c -fstack-protector
	@ar rc	libconfig_parse.a config_parse.o
	@rm *.o
#	@gcc config_file.c -fPIC -shared -o libconfig_file.so		
#	gcc -o test/test -g -Wall -Wextra test/test.c -I. -I/usr/include/google -L. -lcmockery -lconfig_file -lpthread -fstack-protector
	@make -C test	
#	produce document
	@doxygen

release:	config_parse.c config_parse.h
	@gcc -Wall -Wextra -c config_parse.c -fstack-protector
	@ar rc	libconfig_parse.a config_parse.o
	@rm *.o

clean:
	@if [ -f libconfig_parse.a ];then \
		rm libconfig_parse.a test/test 2>/dev/null; \
		rm -rf doc/html/* 2>/dev/null; \
	fi
