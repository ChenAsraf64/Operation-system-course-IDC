/*
 * ex1.c
 * Name: Chen Asraf
 * ID: 204693022
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 65536
#define DESTINATION_FILE_MODE S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH

extern int opterr, optind;
char reverse_buffer[MAX_BUFFER_SIZE] = {0};

void exit_with_usage(const char *message) {
	fprintf (stderr, "%s\n", message);
	fprintf (stderr, "Usage:\n\tex1 [-f] BUFFER_SIZE SOURCE DEST\n");
	exit(EXIT_FAILURE);
}

off_t read_stream_reverse(FILE *stream, char *buf, size_t count, bool *mark_end) {
	/*
		Reads from stream in reverse order into buf 'count' bytes.
	*/
	long original_position = 0;
	long end_position = 0;
	int i = 0, temp = 0;
	
	original_position = ftell(stream);
	end_position = original_position - count;
	if (end_position < 0) {
		end_position = -1;
		*mark_end = true;
	}
	count = original_position - end_position;
	fseek(stream, end_position + 1, SEEK_SET);
	
	temp = fread(reverse_buffer, 1, count, stream);
	if (temp != count) {
	 	 printf("Unable to read source file contents\n");
		 exit(EXIT_FAILURE);
	}
	i = count - 1;
	while (i >= 0) {
		buf[count-i-1] = reverse_buffer[i];
		i--;
	}

	if (*mark_end == true) {
		fseek(stream, 0, SEEK_SET);
	}
	else {
		fseek(stream, end_position, SEEK_SET);
	}
	return count;
}

void append_file_reverse(const char *source_file, const char *dest_file, int buffer_size, int max_size, int force_flag) {
	/*
	 * Append source_file content to dest_file, buffer_size bytes at a time, in reverse order (start from the end of 'source_file').
	 * If source_file is larger than 'max_size', then only 'max_size' bytes will be written.
	 * NOTE: If max_size is smaller or equal to 0, then max_size should be IGNORED.
	 * If force_flag is true, then also create dest_file if does not exist. Otherwise (if force_flag is false, and dest_file doesn't exist) print error, and exit.
	 *
	 * Examples:
	 * 1. if source_file's content is 'ABCDEFGH', 'max_size' is 3 and dest_file doesnt exist then
	 *	  dest_file's content will be 'HGF' (3 bytes from the end, in reverse order)
	 * 2. if source_file's content is '123456789', 'max_size' is -1, and dest_file's content is: 'HGF' then
	 *	  dest file's content will be 'HGF987654321'
	 *
	 * TODO:
	 * 	1. Open source_file for reading
	 *	2. Reposition the read file offset of source_file to the end of file (minus 1).
	 * 	3. Open dest_file for writing (Hint: is force_flag true?)
	 * 	4. Loop read_reverse from source and writing to the destination buffer_size bytes each time (lookout for mark_end)
	 * 	5. Close source_file and dest_file
	 *
	 *  ALWAYS check the return values of syscalls for errors!
	 *  If an error was found, use perror(3) to print it with a message, and then exit(EXIT_FAILURE)
	 */

	int sourceFileDescriptor = open(source_file,O_RDONLY) ; // open the source file for reading only
	if (sourceFileDescriptor < 0) { //checking if the file opening suceed
		perror("Unable to open source file for reading") ; //print the input string + the error messege
		exit(EXIT_FAILURE) ; //terminate process immidiatly
	}
	
	lseek(sourceFileDescriptor, -1, SEEK_END) ; //reposition the read file to the end of the file
	
	int destinationFileDescriptor ; 
	if (force_flag) { //if force_flag is true 
	    destinationFileDescriptor = open(dest_file, O_CREAT, DESTINATION_FILE_MODE) ; //open new destination file
	}
	destinationFileDescriptor = open(dest_file, O_WRONLY | O_APPEND, DESTINATION_FILE_MODE) ; //open the file for writing only
	if (destinationFileDescriptor < 0) { //checking if the destination file open is suceed
			perror("Unable to open destination file for writing") ; 
			close(sourceFileDescriptor) ; //closing the source file
			exit(EXIT_FAILURE) ; 
	}
	
	
	char buffer[MAX_BUFFER_SIZE]; 
	FILE* fileToReadFrom; 
	fileToReadFrom = fdopen(sourceFileDescriptor, "r") ; //using the source file descriptor in order to get pointer to FILE object
	while (read_stream_reverse(fileToReadFrom, buffer, buffer_size, false) != 0) { //reading reverse from source file
		int writingInToFile ; 
		if (max_size <= 0) { //max_size should be IGNORED
			writingInToFile = write(destinationFileDescriptor, buffer, buffer_size) ; // writes up to buffer_size bytes from the buffer to destination file
		}
		else {
			writingInToFile = write(destinationFileDescriptor, buffer, max_size) ; //only max_size bytes will be written
		}
		if (writingInToFile < 0) { //cheking if the writing suceed
			perror("Unable to append buffer content to destination file") ; 
			exit(EXIT_FAILURE) ;
		}
	}
	
	int closingSource = close(sourceFileDescriptor) ; //closing source file
	if (closingSource < 0) {
		perror("Unable to close source file") ; 
		exit(EXIT_FAILURE) ;
	}
	int closingDestination = close(destinationFileDescriptor) ; //closing destination file
		if (closingDestination < 0) {
			perror("Unable to close destination file") ; 
			exit(EXIT_FAILURE) ;
		}
	
		exit(EXIT_SUCCESS) ; 
}

void parse_arguments(
		int argc, char **argv,
		char **source_file, char **dest_file, int *buffer_size, int *force_flag) {
	/*
	 * parses command line arguments and set the arguments required for append_file
	 */
	int option_character;

	opterr = 0; /* Prevent getopt() from printing an error message to stderr */

	while ((option_character = getopt(argc, argv, "f")) != -1) {
		switch (option_character) {
		case 'f':
			*force_flag = 1;
			break;
		default:  /* '?' */
			exit_with_usage("Unknown option specified");
		}
	}

	if (argc - optind != 3) {
		exit_with_usage("Invalid number of arguments");
	} else {
		*source_file = argv[argc-2];
		*dest_file = argv[argc-1];
		*buffer_size = atoi(argv[argc-3]);

		if (strlen(*source_file) == 0 || strlen(*dest_file) == 0) {
			exit_with_usage("Invalid source / destination file name");
		} else if (*buffer_size < 1 || *buffer_size > MAX_BUFFER_SIZE) {
			exit_with_usage("Invalid buffer size");
		}
	}
}

int main(int argc, char **argv) {
	int force_flag = 0; /* force flag default: false */
	char *source_file = NULL;
	char *dest_file = NULL;
	int buffer_size = MAX_BUFFER_SIZE;

	parse_arguments(argc, argv, &source_file, &dest_file, &buffer_size, &force_flag);

	append_file_reverse(source_file, dest_file, 8, 0, force_flag);

	return EXIT_SUCCESS;
}

