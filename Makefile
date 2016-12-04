FILE=producer-consumer

run: ${FILE}
	./${FILE}

# We include -lpthread here, for a program that uses POSIX threads.
# Newer versions of gcc allow you to simply invoke '-pthread' instead
#   of '-lpthread'
${FILE}: ${FILE}.c
	gcc -g -O0 -o ${FILE} ${FILE}.c -lpthread

gdb: ${FILE}
	gdb ${FILE}

vi: ${FILE}.c
	vi ${FILE}.c

emacs: ${FILE}.c
	emacs ${FILE}.c

clean:
	rm -f ${FILE} *~

# 'make' views $v as a make variable and expands $v into the value of v.
# By typing $$, make will reduce it to a single '$' and pass it to the shell.
# The shell will view $dir as a shell variable and expand it.
dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
	dir=`basename $$PWD`; ls -l ../$$dir.tar.gz
