#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------------------------------------
// 反转指定范围的子字符串
//----------------------------------------------------------------------------------------------------------
void reverse_substring(char *str, int len)
{
  int start = 0;
  int end = len - 1;
  while (start < end)
  {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
}

int main() {
    char str1[] = "ab";
    char str2[] = "abcd";
    char str3[] = "abcdefjh";

    printf("Original: %s\n", str1);
    reverse_substring(str1,strlen(str1));
    printf("Transformed: %s\n\n", str1);

    printf("Original: %s\n", str2);
    reverse_substring(str2,strlen(str2));
    printf("Transformed: %s\n\n", str2);

    printf("Original: %s\n", str3);
    reverse_substring(str3,strlen(str3));
    printf("Transformed: %s\n", str3);

    return 0;
}
