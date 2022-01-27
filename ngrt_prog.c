/*
 * Testing program for linux char driver
 *
 * Copyright (C) 2021 Kirill Yustitskii
 *
 * Authors:  Kirill Yustitskii  <inst: yustitskii_kirill>
 */

#include <stdio.h>
#include <errno.h>

int test1(void)
{
    FILE *fp;
    int ret;
    char buf[1024];
    
    /* open device driver file */
    fp = fopen("/dev/ngrtdrv", "r+");
    if (!fp)
    {
        printf("Test 1: can't open file\n");
        return -ENOENT;
    }
    
    /* write to file string and get this string */
    fputs("Testing string", fp);
    if (!fgets(buf, 1024, fp))
    {
        printf("Test 1: error by reading file\n");
        return -EIO;
    }

    printf("Test 1: data = %s\n", buf);
    
    /* close file */
    ret = fclose(fp);
    if (ret)
    {
        printf("Test 1: can't close the file\n");
        return ret;
    }
    
    return 0;
}

int test2(void)
{
    FILE *fp;
    int ret;
    char buf[1024];
    
    /* open device driver file */
    fp = fopen("/dev/ngrtdrv", "r+");
    if (!fp)
    {
        printf("Test 2: can't open file\n");
        return -ENOENT;
    }
    
    /* write to file string and get this string */
    fputs("Testing string", fp);
    if (!fgets(buf, 10, fp))
    {
        printf("Test 2: error by reading file\n");
        return -EIO;
    }

    printf("Test 2: data = %s\n", buf);
        
    /* close file */
    ret = fclose(fp);
    if (ret)
    {
        printf("Test 2: can't close the file\n");
        return ret;
    }
    
    return 0;
}

int test3(void)
{
    FILE *fp;
    int ret;
    char buf[1024];
    
    /* open device driver file */
    fp = fopen("/dev/ngrtdrv", "r+");
    if (!fp)
    {
        printf("Test 3: can't open file\n");
        return -ENOENT;
    }
    
    /* write to file string and get this string */
    fputs("Test 3: testing string", fp);
    if (!fgets(buf, 3000, fp))
    {
        printf("Test 3: error by reading file\n");
        return -EIO;
    }

    printf("Test 3: data = %s\n", buf);
    
    /* close file */
    ret = fclose(fp);
    if (ret)
    {
        printf("Test 3: can't close the file\n");
        return ret;
    }
    
    return 0;
}

int test4(void)
{
    FILE *fp;
    int ret;
    char buf[1024];
    
    /* open device driver file */
    fp = fopen("/dev/ngrtdrv", "r+");
    if (!fp)
    {
        printf("Test 4: can't open file\n");
        return -ENOENT;
    }
    
    /* write to file string and get this string */
    fputs("Testing string", fp);
    fputs("Again testing string", fp);
    if (!fgets(buf, 1025, fp))
    {
        printf("Test 4: error by reading file\n");
        return -EIO;
    }

    printf("Test 4: data = %s\n", buf);
    
    /* close file */
    ret = fclose(fp);
    if (ret)
    {
        printf("Test 4: can't close the file\n");
        return ret;
    }
    
    return 0;
}

int test5(void)
{
    FILE *fp;
    int ret;
    char buf[1024];
    
    /* open device driver file */
    fp = fopen("/dev/ngrtdrv", "r+");
    if (!fp)
    {
        printf("Test 5: can't open file\n");
        return -ENOENT;
    }
    
    /* write to file string and get this string */
    fputs("Testing string", fp);
    fputs("Again testing string", fp);
    if (!fgets(buf, 10, fp))
    {
        printf("Test 5: error by reading file\n");
        return -EIO;
    }

    printf("Test 5: data = %s\n", buf);
    
    if (!fgets(buf, 10, fp))
    {
        printf("Test 5: error by reading file\n");
        return -EIO;
    }

    printf("Test 5: data = %s\n", buf);
    
    /* close file */
    ret = fclose(fp);
    if (ret)
    {
        printf("Test 5: can't close the file\n");
        return ret;
    }
    
    return 0;
}

int test6(void)
{
    FILE *fp;
    int ret;
    char buf[1024];
    
    /* open device driver file */
    fp = fopen("/dev/ngrtdrv", "r+");
    if (!fp)
    {
        printf("Test 6: can't open file\n");
        return -ENOENT;
    }
    
    /* write to file string and get this string */
    fputs("Testing string", fp);
    if (!fgets(buf, 100, fp))
    {
        printf("Test 6: error by reading file\n");
        return -EIO;
    }

    printf("Test 6: data = %s\n", buf);
    
    if (!fgets(buf, 100, fp))
    {
        printf("Test 6: error by reading file\n");
        return -EIO;
    }

    printf("Test 6: data = %s\n", buf);     
    
    /* close file */
    ret = fclose(fp);
    if (ret)
    {
        printf("Test 6: can't close the file\n");
        return ret;
    }
    
    return 0;
}

int main(int argc, char **argv)
{   
     
    if (!test1())
        printf("Test 1 [100%%]\n");
    
    if (!test2())
        printf("Test 2 [100%%]\n");
    
    if (!test3())
        printf("Test 3 [100%%]\n");
    
    if (!test4())
        printf("Test 4 [100%%]\n");
    
    if (!test5())
        printf("Test 5 [100%%]\n");
    
    if (test6())
        printf("Test 6 [100%%]\n");
            
    return 0;
}
