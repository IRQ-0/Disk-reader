#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdbool.h>

#define NUMBERS_PER_LINE 32
#define NUMBERS_PER_GROUP 8

#define CHAR_CUTOFF_LOW 32
#define CHAR_CUTOFF_HIGH 126
#define CHAR_REPLACEMENT '.'

typedef struct {
    bool saveToFile;
    char* filename;

    char* driveLetter;
    bool driveLetterSet;

    unsigned int sectorSize;
    bool sectorSizeSet;

    unsigned int sectorNumber;
    bool sectorNumberSet;

    bool readRange;
    unsigned int start;
    unsigned int end;
} params;

int readSector(char*, unsigned int, unsigned int, unsigned char*);
void printHelp(void);
void printHex(unsigned char*, params);
void printAscii(unsigned char*);
void printOffset(unsigned int);
void writeToFile(params, unsigned char*, unsigned int);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();        
        return 2;
    }
    params p;
    p.saveToFile = FALSE;
    p.readRange = FALSE;

    p.driveLetterSet = FALSE;
    p.sectorSizeSet = FALSE;
    p.sectorNumberSet = FALSE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 0;

        } else if (strcmp(argv[i], "-d") == 0) {
            p.driveLetter = (char*) malloc(strlen(argv[++i]));
            strcpy(p.driveLetter, argv[i]);
            p.driveLetterSet = TRUE;

        } else if (strcmp(argv[i], "-s") == 0) {
            p.sectorNumber = atoi(argv[++i]);
            p.sectorNumberSet = TRUE;

        } else if (strcmp(argv[i], "-c") == 0) {
            p.sectorSize = atoi(argv[++i]);
            p.sectorSizeSet = TRUE;

        } else if (strcmp(argv[i], "-f") == 0) {
            p.filename = (char*) malloc(strlen(argv[++i]));
            strcpy(p.filename, argv[i]);
            p.saveToFile = TRUE;

        } else if (strcmp(argv[i], "-r") == 0) {
            p.readRange = TRUE;
            p.start = atoi(argv[++i]);
            p.end = atoi(argv[++i]);

        } else {
            printf("Unrecognized argument: %s\n", argv[i]);
            return 2;
        }
    }
    
    puts("");
    
    if (p.readRange && !p.saveToFile) {
        printf("Output file not specified");
        return 1;
    }

    if (p.readRange && p.sectorNumberSet) {
        puts("Read range and read one sector option set at the same time");
        return 1;
    }

    if ((!p.driveLetterSet && !p.readRange && !p.sectorSizeSet) || (!p.driveLetterSet && !p.sectorNumberSet && !p.sectorSizeSet)) {
        puts("Paramaters not set");
        return 1;
    }


    unsigned char* sectorBuffer = NULL;
    unsigned char* oneSectorBuffer = NULL;
    
    if (p.readRange) {
        sectorBuffer = (unsigned char*) malloc(p.sectorSize * (p.end - p.start));
        oneSectorBuffer = (unsigned char*) malloc(p.sectorSize);

        if (!oneSectorBuffer) {
            puts("Malloc error");
            return 1;
        }
    } else {
        sectorBuffer = (unsigned char*) malloc(p.sectorSize);
    }
    
    if (!sectorBuffer) {
        puts("Malloc error");
        return 1;
    }

    puts("Reading...");

    if (p.readRange) {
        int ret = 0;
        for (int i = 0; i < (p.end - p.start); i++) {
            ret = readSector(p.driveLetter, (p.start + i), p.sectorSize, oneSectorBuffer);

            if (ret != 0) {
                printf("Reading sector error %d on sector %d\n", ret, i);
                return 1;
            }

            memcpy(sectorBuffer + (i * p.sectorSize), oneSectorBuffer, p.sectorSize);
        }

        puts("Reading done\n");

        writeToFile(p, sectorBuffer, ((p.end - p.start) * p.sectorSize));

        return 0;

    } else {
        int ret = readSector(p.driveLetter, p.sectorNumber, p.sectorSize, sectorBuffer);

        if (ret != 0) {
            printf("Reading sector error %d\n", ret);
            return 1;
        }
    }

    puts("Reading done\n");

    if (p.saveToFile) {
        writeToFile(p, sectorBuffer, p.sectorSize);

    } else {
        puts("\n\nSector readed successfully, make sure you are in fullscreen mode and PRESS ANY KEY");
        getch();
        printHex(sectorBuffer, p);
    }

	return 0;
}

int readSector(char* driveLetter, unsigned int sectorNumber, unsigned int sectorSize, unsigned char* out) {
	char nameBuffer[100];
	DWORD bytesRead;
    HANDLE handle = NULL;

	if (sprintf(nameBuffer, "\\\\.\\%s:", driveLetter) <= 0)
        return 1;

	memset(out, 0, sectorSize);

    handle = CreateFile(nameBuffer, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (handle == INVALID_HANDLE_VALUE)
        return GetLastError();

    if (SetFilePointer(handle, (sectorNumber * sectorSize), NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    	return GetLastError();

    if (ReadFile(handle, out, sectorSize, &bytesRead, NULL) == 0)
        return GetLastError();

    return 0;
}

void printHelp(void) {
    puts("\nUsage:");
    puts(" -h = print this");
    puts(" -d <letter> = specify drive");
    puts(" -s <sector> = sector number");
    puts(" -r <start> <end> = read range of sectors from <start> to <end>");
    puts(" -c <size> = sector size");
    puts(" -f <filename> = save output to file");
    puts("\nREMEBER TO RUN APPLICATION AS ADMIN!");
}

void writeToFile(params p, unsigned char* in, unsigned int size) {
    puts("Writing...");

    FILE *f;
    f = fopen(p.filename, "wb");
    
    if (!f) {
        printf("File error - cannot create file");
        exit(1);
    }
    
    if (fwrite(in, 1, size, f) < p.sectorSize) {
        printf("File error - error while writing");
        exit(1);
    }

    if (fclose(f) != 0) {
        printf("File error - error while closing");
        exit(1);
    }

    printf("Writing done\n");
}

void printOffset(unsigned int in) {
    char buffer[10];
    if (sprintf(buffer, "%d", in) == 2) {
        printf(" %s", buffer);
    } else {
        printf(buffer);
    }
    printf(":");
}

void printAscii(unsigned char* in) {
    int j = 0;
    printf("    |");
    for (int i = 0; i < 32; i++) {
        if (in[i] < 32 || in[i] > 126) {
            printf("%c", CHAR_REPLACEMENT);
        } else {
            printf("%c", in[i]);
        }
        if (j == (NUMBERS_PER_GROUP - 1))  {
            printf(" ");
            j = 0;
        } else {
            j++;
        }
    }
    printf("|\n");
}

void printHex(unsigned char* in, params p) {
    system("cls");
    puts("Here comes the hex dump of readed sector:\n\n");
    char buffer[10];
    unsigned char lineBuffer[NUMBERS_PER_LINE];
    int j = 1, k = 1, lines = 1;
    printf("  0: ");

    for (int i = 0; i < p.sectorSize; i++) {
        memset(buffer, 0, sizeof(buffer));
        
        if (in[i] == 0) {
            printf("00");
        } else {
            if (sprintf(buffer, "%.X", in[i]) != 2) {
                printf("0%s", buffer);
            } else {
                printf("%s", buffer);
            }
        }

        if (j == NUMBERS_PER_LINE) {
            j = 1;
            printAscii(lineBuffer);
            if (!(i == (p.sectorSize - 1))) {
                printOffset(lines++ * NUMBERS_PER_LINE);
            }
        } else {
            j++;
            lineBuffer[j] = in[i];
            printf(" ");
        }

        if (k == NUMBERS_PER_GROUP) {
            k = 1;
            printf(" ");
        } else {
            k++;
        }
        
    }

}