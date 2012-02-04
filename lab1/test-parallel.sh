cat > tparallel.sh << 'EOF'

echo Sleeping for two seconds total

sleep 2
sleep 1
sleep 1
sleep 1
sleep 1 && sleep 1
sleep 2 || exit
echo I am awake

echo file > file.txt
cat < file.txt > otherfile.txt
rm file
cat otherfile.txt
rm otherfile.txt
EOF
./timetrash -t tparallel.sh


rm tparallel.sh
