cat > pop.sh << 'EOF'
echo pop && echo pop;
ls | grep f > foo.txt;
echo yodawg;
sleep 1 || exit


cat < foo.txt > bar.txt
echo thereshouldbenodiff;
diff foo.txt bar.txt
sleep 1 && sleep 2


ls -l | find foo.txt > log.txt
find < log.txt
find < log.txt | grep f > log2.txt

echo thisiscool || sleep 1

echo fuuuuuuuuuuuuuuun > test.txt && cat< test.txt > test2.txt && rm test.txt && rm test2.txt && echo SUCCESS

cp foo.txt foo2.txt || mkdir failure_dir
rm foo.txt && rm foo2.txt && rm bar.txt && rm log.txt && rm log2.txt || echo fail
EOF

./timetrash pop.sh
rm pop.sh
