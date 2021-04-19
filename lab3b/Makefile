# NAME: Weikeng Yang, Zhirong Wang
# EMAIL: weikengyang@gmail.com, zwang40@mail.ccsf.edu
# UID: 405346443,105179351

default:
	rm -f lab3b
	ln team_lab3b lab3b
	chmod u+x lab3b


clean:
	rm -f *.tar.gz lab3b

check:
	chmod +x P3B_check.sh 
	./P3B_check.sh 405346443

dist: clean
	tar -czvf lab3b-405346443.tar.gz lab3b.py team_lab3b Makefile README
	tar -czvf lab3b-105179351.tar.gz lab3b.py team_lab3b Makefile README
