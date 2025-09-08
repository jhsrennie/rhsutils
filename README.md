Like many hard pressed sysadmins, over the years I have written a number of applets to help me with my network management. I've collected them together and I'm putting them on Sourceforge in the hope that they will be useful to others. I'm sure many of these apps have been independantly invented many times, and I'm not claiming that my versions are better than anyone elses. However all of these apps I use on a daily basis and they've all been tested over many years.

The apps are all command line, but because I've got fed up of scrolling command windows up and down some of the apps accept a flag "--gui" that makes them create a window and send the output to the window instead of the command line. See the app source for a readme file that gives a detailed description of the app.

The apps that might run into long filename problems are UNICODE and will accept the \\?\ prefix to allow filenames longer than 260 characters.

dircomp  
Compare two directories

dirshowacl  
Show NTFS permissions for a directory (and all subdirectories)

dirspace  
Analyse the disk space used within a directory

diskinfo  
Display disk and volume info

diskthrasher  
Disk speed tester but it's obsolete now and is only of historical interest

dosdevice  
Apps to display, create and delete NT kernel device names. Unlikely to be of interest to anyone sane.

filecomp  
Compare file(s)

filefind  
Find files by name

filefindtext  
Search for text in file(s)

filerename  
Rename files. Allows more flexible renaming than the built in rename command.

fileshowopen  
Show open files.

getsntptime  
Set the system time from an NTP server

httpgrabber  
Grab a web page and save it to a file

httpserver  
Simple HTTP server. Useful only for testing.

pop3util  
POP3 utility. View and delete messages on a POP3 server.

reconcile  
Synchronise two directories

reggrep  
Search the registry for strings using regular expressions

regshowacl  
Show the ACLs on registry keys

rhscalc  
Reverse Polish calculator

smtpsendfile  
Send an RFC822 formatted file to an SMTP server to send an e-mail

smtputil  
SMTP utility. Send an e-mail by manually typing the various fields.