all:
	g++ simulation.cpp sqlite_vfs.cpp buffer_mgr.cpp disk.cpp -lsqlite3 -o simulation

run:
	./simulation > output.txt

clean:
	rm -f simulation
	rm -f simulation.db
	rm -f stud_city
	rm -f stud_sport
