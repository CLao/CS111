#Test basic commands one-by-one

#Test simple command
echo Simple commands work.

#Test AND/OR command
touch ThisShouldDelete.txt && rm ThisShouldDelete.txt
echo AND Commands work.

echo No file should be created || touch ShouldNotExist.txt
echo OR Commands work.

#Test pipes
echo Successful pipe prints makefile
ls | grep f

#Test I/O redirection
echo contents of foo > foo.txt
cat < foo.txt > bar.txt
echo No diff statements should appear
diff foo.txt bar.txt
rm foo.txt
rm bar.txt

#Test Subshells
(echo Subshells work > subshellio.txt)
(cat subshellio.txt)
(rm subshellio.txt)

