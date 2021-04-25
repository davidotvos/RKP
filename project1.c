#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <omp.h>
#include <signal.h>

/*
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
*/

//Error üzenet
void error(char *msg, int status)
{
    fprintf(stderr, "Error: %s\n", msg);

    if (status > 0)
    {
        exit(status);
    }
}

int random()
{
    return (rand() % 256);
}
//2.feladat 1.lépés
char *TestArray(int *NumCH)
{
    srand(time(NULL));
    //belekódolja az 'a' 'b' 'c' karaktereket
    unsigned char *tomb = (unsigned char *)malloc(9);
    int i = 0;
    while (i < 3)
    {
        tomb[i] = ((rand() % 256) << 3) | (((unsigned char)'d') >> (6 - (i * 3)));
        tomb[i + 3] = ((rand() % 256) << 3) | (((unsigned char)'e') >> (6 - (i * 3)));
        tomb[i + 6] = ((rand() % 256) << 3) | (((unsigned char)'f') >> (6 - (i * 3)));
        i++;
    }
    *NumCH = 3;
    return tomb;
}

//2.feladat 2.lépés
char *Unwrap(char *Pbuff, int NumCH)
{
    unsigned char *szoveg = (unsigned char *)malloc(NumCH);
    if (szoveg == NULL)
    {
        free(Pbuff);
        error("A memoriafoglalas sikertelen.\n", 2);
    }

    int i;
    unsigned char mask = 0b00111000;

#pragma omp parallel for schedule(dynamic)
    for (i = 0; i < NumCH; i++)
    {
        szoveg[i] = (Pbuff[i * 3] << 6) & mask << 3 |
                  (Pbuff[i * 3 + 1] << 3) & mask | Pbuff[i * 3 + 2] & (mask >> 3);
    }

    szoveg[NumCH] = '\0';
    free(Pbuff);
    return szoveg;
}

//3.feladat 1.lepes
char *ReadPixels(int f, int* NumCH){
	unsigned int fileSize, offset;
	
	lseek(f,2,SEEK_SET);
	read(f,&fileSize,4);

	read(f,NumCH,4);

	read(f,&offset,4);

	int arraySize=fileSize-offset;
	char *pixelArray = (char*)malloc(arraySize);
	if(pixelArray==NULL)
		error("A memoriafoglalas sikertelen.\n", 2);

	lseek(f,offset,SEEK_SET);
	read(f,pixelArray,arraySize);

	return pixelArray; 
}

//3.feladat 2.lepes
int BrowseForOpen(void){
	DIR *d;
	struct dirent *entry;
	struct stat inode;
	char fileName[100];
	int file;
	int rowCount;

	chdir(getenv("HOME"));

	while(1){
		d = opendir(".");

		printf("\n_________________Current Working Directory_________________\n");
		
		rowCount = 0;
		while(entry = readdir(d)){
			stat(entry->d_name, &inode);

			if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
					printf(CL_RED "%s\t", entry->d_name);
			else if(inode.st_mode&S_IFDIR)
				printf(CL_BLUE "%s\t\t", entry->d_name);
			else if(inode.st_mode&S_IFREG)
				printf(CL_GREEN "%s\t\t", entry->d_name);
			else
				printf(CL_CYAN "%s\t\t", entry->d_name);

			rowCount++;
			if(rowCount % 4 == 0){
				putchar('\n');
			}
		}

		closedir(d);
		printf(CL_DEF "\n\nChoose a file/directory: ");
		fgets(fileName, 100, stdin);
		fileName[strlen(fileName)-1] = '\0';

		if(stat(fileName, &inode) < 0){
			error("File/Directory doesn't exist. Please choose a valid name.",0);
		}else{
			if(inode.st_mode&S_IFDIR){
				chdir(fileName);
			}else if(inode.st_mode&S_IFREG){
				file=open(fileName,O_RDONLY);
				return file;
			}
		}
	}
}

int main(int argc, char const *argv[])
{
    //1.feladat
    int file, out, NumCH;
    char neptunID[] = "KIMVBU";
    char *read;
    char *msg;

    if (argc > 1)
    {

        if (strcmp(argv[1], "--version") == 0)
        {
            printf("Verziószám: 3.0\nFejlesztő: Otvos David István\nElkészült: 2021.04.25\n");
            return 0;
        }
        else if (strcmp(argv[1], "--help") == 0)
        {
            printf("Opciok:\n--version : Program informaciok\n--help : Futtatasi opciok\n");
            return 0;
        }
        else
        {
            //ha van parancssori argumentuma
            file = open(argv[1], O_RDONLY);
        }
    }
    else
    {
        //ha nincs parancssori argumentum
        file = BrowseForOpen();
    }
    if (file < 0)
        error("Error while opening file.", 1);

    //signal(SIGALRM, WhatToDo);
    //signal(SIGINT, WhatToDo);
    alarm(1);

    read = ReadPixels(file, &NumCH);
    msg = Unwrap(read, NumCH);

    alarm(0);
    printf("Coded message: %s\n", msg);
    if (Post(neptunID, msg) > 0)
    {
        free(msg);
        close(file);
        error("The server hasn't received your message.", 3);
    }
    else
    {
        free(msg);
        close(file);
        printf("The server has received your message.\n");
    }
    return 0;
}