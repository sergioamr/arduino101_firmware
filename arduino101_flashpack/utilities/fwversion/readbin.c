#include <stdio.h>

#define START 0x8
#define LENGTH 0x14

int main(int argc, char* argv[])
{
	FILE *fp;
	char *file;
	int i;
	// Allocate the read buffer
	char readBuffer[LENGTH];

	if (argc < 2) {
		printf("Usage: %s <file_name>", argv[0]);
		return -1;
	}

	// Set the filename (could also get from a command-line parameter)
	file = argv[1];

	// Open the specified file in binary mode ("rb")
	if ((fp = fopen(file, "rb")) == NULL) {
		printf("Error opening %s", file);
		return - 1;
	}

	fseek(fp, START, SEEK_SET);

	// Read the file's data into the read buffer
	long read = fread(readBuffer, 1, LENGTH, fp);

	for (i = 0; i < LENGTH; i++) {
		printf("%c", readBuffer[i]);
	}
	printf("\n");

	fclose(fp);

	// Exit program
	return 0;
}

