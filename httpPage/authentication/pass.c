#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include "sqlite3.h" 
char* getcgidata(FILE* fp, char* requestmethod); 
int main() 
{ 
    char *input; 
    char *req_method; 
    char namein[12]; 
    char pass[12]; 
    char passtemp[12]; 
    int i = 0; 
    int j = 0; 

    printf("Content-type: text/html\n\n"); 

    req_method = getenv("REQUEST_METHOD"); 
    input = getcgidata(stdin, req_method); 
    // 我们获取的input字符串可能像如下的形式 
    // Username="admin"&Password="aaaaa" 
    // 其中"Username="和"&Password="都是固定的 
    // 而"admin"和"aaaaa"都是变化的，也是我们要获取的 

    // 前面9个字符是Usernamein= 
    // 在"Usernamein="和"&"之间的是我们要取出来的用户名 
    for ( i = 9; i < (int)strlen(input); i++ ) 
    { 
        if ( input[i] == '&' ) 
        { 
            namein[j] = '\0'; 
            break; 
        }                                    
        namein[j++] = input[i]; 
    } 
    // 前面9个字符 + "&Password="10个字符 + Usernamein的字符数 
    // 是我们不要的，故省略掉，不拷贝 
    for ( i = 19 + strlen(namein), j = 0; i < (int)strlen(input); i++ ) 
    { 
        pass[j++] = input[i]; 
    } 
    pass[j] = '\0'; 

    

    sqlite3_stmt*    stmt; 

    sqlite3 *db=NULL; 
    char *zErrMsg = 0; 
    int rc; 
    rc = sqlite3_open("/usr/local/boa/cgi-bin/login.db", &db); //打开指定的数据库文件,如果不存在将创建一个同名的数据库文件 

    //创建一个表,如果该表存在，则不创建，并给出提示信息，存储在 zErrMsg 中 
    //char *sql = " CREATE TABLE user(ID INTEGER PRIMARY KEY,Name text,Passwd text);" ; 
    //sqlite3_exec( db , sql , 0 , 0 , &zErrMsg ); 

    //查询数据int ret=0; 
    int t=0; 
    sql = "SELECT Passwd FROM user WHERE Name=?"; 
    ret = sqlite3_prepare(db,sql,strlen(sql),&stmt,NULL); 
    sqlite3_bind_text(stmt, 1, namein,strlen(namein),SQLITE_STATIC); 
    while( sqlite3_step(stmt) == SQLITE_ROW){ 
        strcpy(passtemp,sqlite3_column_text(stmt,0)); 
        if(strcmp(passtemp,pass)==0) 
        { printf("<html>\n") ; 
            printf("<head><title>welcome</title></head>\n") ; 
            printf("<body>\n") ; 
            printf("<h1>welcome home!!</h1>\n") ; 
            printf("</body>\n") ; 
            printf("</html>\n") ; 
            t=1; 
        } } 
    if(t==0) 
    { 
        printf("<html>\n") ; 
        printf("<head><title>welcome</title></head>\n") ; 
        printf("<body>\n") ; 
        printf("<h1>sorry,you need the key!</h1>\n") ; 
        printf("</body>\n") ; 
        printf("</html>\n") ;} 

    sqlite3_finalize(stmt); 

    sqlite3_close(db); //关闭数据库 
    return 0; 

} 

/**
 * 如果是GET方法,就獲取QUERY_STRING的值
 * 如果是POST方法,就
 *      1.獲取CONTENT_LENGTH的值
 *      2.分配內存空間
 *      3.如果CONTENT_LENGTH的值0, 就返回內容爲'\0'的字符串指針
 *      4.讀取fp對應的文件的內容(不超過1024byte, 不超過CONTENT_LENGTH長度), 存入input指向的內存
 **/
char* getcgidata(FILE* fp, char* requestmethod) 
{ 
    char* input; 
    int len; 
    int size = 1024; 
    int i = 0; 

    if (!strcmp(requestmethod, "GET")) 
    { 
        input = getenv("QUERY_STRING"); 
        return input; 
    } 
    else if (!strcmp(requestmethod, "POST")) 
    { 
        len = atoi(getenv("CONTENT_LENGTH")); 
        input = (char*)malloc(sizeof(char)*(size + 1)); 

        if (len == 0) 
        { 
            input[0] = '\0'; 
            return input; 
        } 

        while(1) 
        { 
            input[i] = (char)fgetc(fp); 
            if (i == size) 
            { 
                input[i+1] = '\0'; 
                return input; 
            } 

            --len; 
            if (feof(fp) || (!(len))) 
            { 
                i++; 
                input[i] = '\0'; 
                return input; 
            } 
            i++; 

        } 
    } 
    return NULL; 
}
