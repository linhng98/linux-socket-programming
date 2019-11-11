#include <stdio.h>

int main()
{
    printf("%-.10s%-.10s\n", "month", "day");
    printf("%-.10d%-.10d\n", 1, 10);
    printf("%-.10d%-.10d\n", 12, 2);
    return 0;
}