#
# RAM $Id: Makefile 68 2009-01-05 13:22:21Z quixadhal $
#

CC      	= g++

CRYPT		= #-DNOCRYPT		# Uncomment to use clear-text passwords
RAND		= #-DOLD_RAND		# Uncomment if your random number generator is broken
SOCIAL		= #-DSOCIAL_DEBUG	# Uncomment for debugging of the socials
PLAYERLIST      = #-DPLAYER_LIST        # Uncomment for player list features

# Comment out the -Wmissing-format-attribute flag if you're using gcc 2.95
# or upgrade!
#
# The W_CONLY set of flags are only valid when using C, not for C++
# -Wmissing-declarations is C only under g++ 4.2.x and older!

W_ERROR         = -Werror
W_ANSI          = #-pedantic
W_UBER          = -Wall
W_FORMAT        = -Wformat -Wformat-security -Wmissing-format-attribute
W_MESSY         = -Wmissing-braces -Wparentheses -Wshadow -Wredundant-decls
W_TYPE          = -Wcast-qual -Wcast-align -Wchar-subscripts -Wreturn-type -Wswitch -Wwrite-strings
W_EXTRA         = -Wunused -Wuninitialized #-Wunreachable-code
W_NITPICK       = -Wpointer-arith -Winline
ifeq ($(CC), gcc)
W_CONLY		= -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes 
endif

WARN		= $(W_ERROR) $(W_ANSI) $(W_UBER) $(W_FORMAT) $(W_MESSY) $(W_TYPE) $(W_EXTRA) $(W_NITPICK) $(W_CONLY)

OPT	    	= -O3 				# You trust the compiler, don't you?
DEBUG		= -g				# Just enough to fix the occasional crash
#DEBUG		= -ggdb3			# Full debugging with ALL defined symbols!
PROF		= #-pg				# Enable profiling via gprof

OPTIONS		= $(OPT) $(DEBUG) $(PROF) $(CRYPT) $(RAND) $(SOCIAL) $(PLAYERLIST)

C_FLAGS 	= $(WARN) $(OPTIONS)
L_FLAGS 	= $(DEBUG) $(PROF)

LIBS		= 				# Uncomment if you're using Linux
#LIBS		= 				# Uncomment if you're using BSD or OS X
#LIBS		= -lsocket -lresolv -lnsl	# Uncomment if you're using Solaris

O_FILES		= act_comm.o act_info.o act_move.o act_obj.o act_wiz.o \
		  alias.o ban.o bug.o comm.o db.o \
		  fight.o handler.o interp.o magic.o mob_cmds.o \
		  mob_prog.o note.o olc.o olc_act.o olc_hedit.o \
		  olc_mpcode.o olc_save.o olc_utils.c playerlist.o random.o \
		  save.o sha256.o skills.o special.o strings.o \
		  tables.o update.o

ram: dep $(O_FILES)
	@rm -f $@
	$(CC) $(L_FLAGS) -o $@ $(O_FILES) $(LIBS)
	@ls -l $@

%o : %c
	$(CC) $(C_FLAGS) -c $< -o $@

tags :
	@rm -f $@
	@ctags *.[ch]
	@ls -l $@

clean:
	@rm -f *.o

spotless:
	@rm -f *.o tags ram gmon.out dep

dep:
	@rm -f $@
	@find . -name \*.c -a -type f | sort | xargs -P 1 -r $(CC) $(OPTIONS) -MM >$@
	@ls -l $@

#--- Dependancies go below here ---

-include dep

