#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main()
{
    FILE* file = fopen("text.txt", "w");
    fprintf(file, "pusi ga majmune!\n");
    return 0;
}