# .bashrc

# User specific aliases and functions

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

PS1='\n\n\033[01;34m$LOGNAME@$HOSTNAME \033[0;31m\n$PWD \033[00m\n$ '

#alias ls='ls --color'

