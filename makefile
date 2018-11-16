CCP: msi.cpp mesi.cpp moesi.cpp L2.h LLC.h
	g++ msi.cpp -o msi -fno-stack-protector
	g++ mesi.cpp -o mesi -fno-stack-protector
	g++ moesi.cpp -o moesi -fno-stack-protector

clean:
	rm msi mesi moesi
