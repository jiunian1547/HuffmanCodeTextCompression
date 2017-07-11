prog: main.c
	gcc main.c -o huffman

clean:
	rm *.huf
	rm huffman
