#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

void createDirs(char *file_path)
{
    char *dir_path = (char *) malloc(strlen(file_path) + 1);
    char *next_sep = strchr(file_path, '/');

    while (next_sep != NULL)
    {
        int dir_path_len = next_sep - file_path;
        memcpy(dir_path, file_path, dir_path_len);

        dir_path[dir_path_len] = '\0';
        mkdir(dir_path, S_IRWXU|S_IRWXG|S_IROTH);
        next_sep = strchr(next_sep + 1, '/');
    }

    free(dir_path);
}

void swapPathSeparator(char* path)
{
    size_t pathLen = strlen(path);
    for(int i=0; i<pathLen; i++)
    {
        if(path[i] == '\\')
        {
            path[i] = '/';
        }else if(path[i] == '/')
        {
            path[i] = '\\';
        }
    }
}

size_t loadFile(const char* path, void* pFile)
{
    FILE* fp = fopen(path, "rb");
    if(!fp)
        return 0;

    // Get file size
    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Load file into memory
    char* file = (char*)malloc(fsize);
    if(!file)
    {
        fclose(fp);
        return 0;
    }

    fread(file, fsize, 1, fp);
    fclose(fp);

    *(void**)pFile = file;
    return fsize;
}

void writeFile(const char* fileName, const void* pFile, size_t len)
{
    FILE* fp = fopen(fileName, "wb");
    if(!fp)
        return;

    fwrite(pFile, len, 1, fp);

    fclose(fp);
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("Usage: %s <archive>\n", argv[0]);
        return 1;
    }

    char* file = NULL;
    size_t fsize = loadFile(argv[1], &file);

    // Check umd version
    unsigned int umdVersion = *(unsigned int*)(file+fsize-8);
    if(umdVersion != 1 && umdVersion != 2)
    {
        printf("Unsupported umd version %d\n", umdVersion);
        free(file);
        return 1;
    }

    char* fileTable = file+*(unsigned int*)(file+fsize-16)+2;

    // Create output dir name
    char suffix[] = "_extracted";
    char* dirName = (char*)malloc(strlen(argv[1]) + sizeof(suffix));
    memset(dirName, 0x0, strlen(argv[1]) + sizeof(suffix));
    strcat(dirName, argv[1]);
    strtok(dirName, ".");
    strcat(dirName, suffix);

    // Exctract all files
    size_t offset = 0x0;
    size_t count = 0;
    while(1)
    {
        unsigned char* nameLen = (unsigned char*)fileTable+offset;
        offset+=sizeof(unsigned char);

        char* fileName = fileTable+offset;
        swapPathSeparator(fileName);
        offset+=*nameLen;

        unsigned int* fileOffset = (unsigned int*)(fileTable+offset);
        offset+=sizeof(unsigned int);

        unsigned int* fileSize = (unsigned int*)(fileTable+offset);

        if(umdVersion == 1)
            offset+=sizeof(unsigned int)*2;
        else
            offset+=sizeof(unsigned int)*4;

        // Echo extracted files
        if(*fileSize <= 1024)
            printf("%ld %s %d b\n", count, fileName, *fileSize); // Display size in bytes
        else if(*fileSize <= 1048576)
            printf("%ld %s %.1f Kib\n", count, fileName, *fileSize/pow(2, 10)); // Display size in kilobytes
        else
            printf("%ld %s %.1f Mib\n", count, fileName, *fileSize/pow(2, 20)); // Display size in megabytes

        // Build path (dst + "/" + fileName)
        char* path = (char*)malloc(strlen(dirName) + strlen(fileName) + 2);
        strcpy(path, dirName);
        strcat(path, "/");
        strcat(path, fileName);

        // Extract files
        createDirs(path);
        writeFile(path, file+*fileOffset, *fileSize);

        free(path);

        count++;

        if(umdVersion == 1)
        {
            // Break when we are 32 bytes away from the end of the file
            if(fsize - (size_t)((fileTable-file)+offset) <= 32)
                break;
        }else
        {
            // Break when we are 41 bytes away from the end of the file
            if(fsize - (size_t)((fileTable-file)+offset) <= 41)
                break;
        }
    }

    printf("\nExtracted %ld files to %s\n", count, dirName);

    free(dirName);
    free(file);
    return 0;
}
