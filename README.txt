This program acts as a shell, where the user can input commands for the comnputer to run.
the included makefile produces an executeable named smallsh, when the make command is run
the shell takes commands that are up to 2048 characters long, and 512 words
the shell ahs three commands that are hardcoded in, exit cd and status
they run if the first word that the user inputs is exit, cd, or status respectivly. they only work if entered in all lowercase
exit exits out of the shell
cd changes the working directory. if followed by another argument, then it will change into that specified directory.
other wise goes to the home directory if nothiung is specified.
status prints the exit status of the last command run in the forground.
the shell is able to convert '$$' in arguments to the PID of the shell
the shell supports file redirection in the form of [command] >/< [filename]
the shell can run command in the background if the last argument is '&'
the shell does not process commands if they start with a '#'
there is also a test script, p3testscript included.